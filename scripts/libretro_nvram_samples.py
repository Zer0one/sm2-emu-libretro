#!/usr/bin/env python3
"""Generate SM2 Libretro NVRAM samples from scripted RetroArch replay input.

Each sample runs in an isolated RetroArch environment, starting from an optional
frontend `.srm` or legacy `.nv`/`.eeprom` baseline. Sequences and macros are read from a TOML
campaign, see ``libretro_nvram_samples.<game>.toml``. Interactive mode launches
RetroArch without scripted input and waits for manual close.

This utility targets macOS because clean shutdown uses CoreGraphics to post Esc
on the launched RetroArch process.
"""

from __future__ import annotations

import argparse
import ctypes
import json
import os
import re
import shutil
import socket
import struct
import subprocess
import sys
import tempfile
import time
import tomllib
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any


BUTTONS = {
    "B": 0,
    "Y": 1,
    "SELECT": 2,
    "START": 3,
    "UP": 4,
    "DOWN": 5,
    "LEFT": 6,
    "RIGHT": 7,
    "A": 8,
    "X": 9,
    "L": 10,
    "R": 11,
    "L2": 12,
    "R2": 13,
    "L3": 14,
    "R3": 15,
}

SUFFIX_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]*$")
WAIT_RE = re.compile(r"^WAIT\(\s*([0-9]+(?:\.[0-9]+)?)\s*\)$", re.IGNORECASE)
SCREENSHOT_RE = re.compile(
    r"^SCREENSHOT\(\s*([A-Za-z0-9][A-Za-z0-9._-]*)\s*\)$", re.IGNORECASE
)
BUTTON_HOLD_RE = re.compile(
    r"^(SHORT|LONG)\(\s*([A-Za-z0-9_-]+)"
    r"(?:\s*\*\s*(0|[1-9][0-9]*))?\s*\)$",
    re.IGNORECASE,
)
LATCH_RE = re.compile(r"^(HOLD|RELEASE)\(\s*([A-Za-z0-9_-]+)\s*\)$", re.IGNORECASE)
REPEAT_RE = re.compile(r"^(.+?)\s*\*\s*(0|[1-9][0-9]*)$")
PLAYER_BUTTON_RE = re.compile(r"^P([12])_(.+)$")


@dataclass(frozen=True)
class Action:
    kind: str
    value: str | float | tuple["ButtonPress", ...] = ""


@dataclass(frozen=True)
class ButtonPress:
    name: str
    long: bool = False


@dataclass(frozen=True)
class Settings:
    repo_root: Path
    retroarch: Path
    core: Path
    rom_dir: Path
    system_assets: Path
    base_config: Path | None
    game: str
    baseline_sample: str | None
    standard_srm: Path | None
    standard_nvram: Path | None
    standard_eeprom: Path | None
    output_dir: Path
    startup_wait: float
    button_hold: float
    button_hold_long: float
    default_wait: float
    settle_before_close: float
    close_gap: float
    shutdown_timeout: float
    window_width: int
    window_height: int


@dataclass(frozen=True)
class Sample:
    suffix: str
    actions: tuple[Action, ...]


@dataclass
class Result:
    sample: str
    status: str
    saves: list[str]
    screenshots: list[str]
    log: str
    console_log: str
    elapsed_seconds: float
    note: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate isolated SM2 NVRAM samples from a TOML campaign."
    )
    parser.add_argument("config", type=Path, help="TOML campaign file")
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument(
        "--sample",
        action="append",
        default=[],
        metavar="SUFFIX",
        help="run only this sample; repeat to select multiple samples",
    )
    selection.add_argument(
        "--resume-from",
        metavar="SUFFIX",
        help="start with this sample and continue with all following samples",
    )
    selection.add_argument(
        "--interactive",
        metavar="SUFFIX",
        help=(
            "launch one unscripted sample and wait for RetroArch to be closed "
            "manually"
        ),
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="override campaign.output_dir (useful for temporary validation)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="validate and print expanded sequences without launching RetroArch",
    )
    parser.add_argument(
        "--overwrite",
        action="store_true",
        help="replace existing output samples",
    )
    parser.add_argument(
        "--keep-workdirs",
        action="store_true",
        help="preserve isolated RetroArch work directories under output/work",
    )
    return parser.parse_args()


def config_path(value: str | Path | None, base: Path) -> Path | None:
    if not value:
        return None
    expanded = Path(os.path.expandvars(os.path.expanduser(str(value))))
    return expanded.resolve() if expanded.is_absolute() else (base / expanded).resolve()


def table(data: dict[str, Any], name: str) -> dict[str, Any]:
    value = data.get(name)
    if not isinstance(value, dict):
        raise ValueError(f"missing or invalid [{name}] table")
    return value


def positive_number(data: dict[str, Any], name: str, default: float) -> float:
    value = data.get(name, default)
    if not isinstance(value, (int, float)) or isinstance(value, bool) or value <= 0:
        raise ValueError(f"{name} must be a positive number")
    return float(value)


def positive_integer(data: dict[str, Any], name: str, default: int) -> int:
    value = data.get(name, default)
    if not isinstance(value, int) or isinstance(value, bool) or value <= 0:
        raise ValueError(f"{name} must be a positive integer")
    return value


def split_tokens(value: Any, label: str, allow_empty: bool = False) -> list[str]:
    if isinstance(value, str):
        parts = [value]
    elif isinstance(value, list) and all(isinstance(item, str) for item in value):
        parts = value
    else:
        raise ValueError(f"{label} must be a string or an array of strings")
    tokens: list[str] = []
    for part in parts:
        tokens.extend(token.strip() for token in part.split(",") if token.strip())
    if not tokens and not allow_empty:
        raise ValueError(f"{label} must not be empty")
    return tokens


def normalize_button_name(value: str) -> str | None:
    normalized = value.upper().replace("-", "_")
    if normalized in BUTTONS:
        return normalized
    player_button = PLAYER_BUTTON_RE.fullmatch(normalized)
    if player_button and player_button.group(2) in BUTTONS:
        return normalized
    return None


def button_target(name: str) -> tuple[int, int]:
    player_button = PLAYER_BUTTON_RE.fullmatch(name)
    if player_button:
        user = int(player_button.group(1)) - 1
        button = player_button.group(2)
    else:
        user = 0
        button = name
    return user, BUTTONS[button]


def parse_button_presses(token: str) -> list[ButtonPress] | None:
    explicit = BUTTON_HOLD_RE.fullmatch(token)
    if explicit:
        hold_type = explicit.group(1).upper()
        normalized = normalize_button_name(explicit.group(2))
        if normalized is None:
            raise ValueError(f"unknown {hold_type.lower()}-press button: {token}")
        count = int(explicit.group(3) or 1)
        return [ButtonPress(normalized, hold_type == "LONG") for _ in range(count)]
    normalized = normalize_button_name(token)
    if normalized is not None:
        return [ButtonPress(normalized)]
    return None


def expand_actions(
    tokens: list[str], macros: dict[str, list[str]], stack: tuple[str, ...] = (),
) -> list[Action]:
    actions: list[Action] = []
    for token in tokens:
        repeat = REPEAT_RE.fullmatch(token)
        if repeat:
            base, count = repeat.group(1).strip(), int(repeat.group(2))
            actions.extend(expand_actions([base], macros, stack) * count)
            continue
        if "+" in token:
            presses: list[ButtonPress] = []
            for part in (piece.strip() for piece in token.split("+")):
                parsed = parse_button_presses(part)
                if not part or parsed is None:
                    raise ValueError(f"invalid simultaneous button sequence: {token}")
                if len(parsed) != 1:
                    raise ValueError(
                        "repeat the complete simultaneous sequence outside "
                        f"the button expression: {token}"
                    )
                presses.extend(parsed)
            names = [press.name for press in presses]
            if len(set(names)) != len(names):
                raise ValueError(f"duplicate button in simultaneous sequence: {token}")
            actions.append(Action("chord", tuple(presses)))
            continue

        latch = LATCH_RE.fullmatch(token)
        if latch:
            operation = latch.group(1).lower()
            normalized = normalize_button_name(latch.group(2))
            if normalized is None:
                raise ValueError(f"unknown {operation} button: {token}")
            actions.append(Action(operation, normalized))
            continue

        if token.startswith("@"):
            requested = token[1:].strip().lower()
            if not requested or requested not in macros:
                raise ValueError(f"unknown macro: {token}")
            if requested in stack:
                chain = " -> ".join(f"@{name}" for name in (*stack, requested))
                raise ValueError(f"recursive macro: {chain}")
            actions.extend(expand_actions(macros[requested], macros, (*stack, requested)))
            continue

        wait = WAIT_RE.fullmatch(token)
        if wait:
            seconds = float(wait.group(1))
            if seconds < 0:
                raise ValueError(f"invalid wait: {token}")
            actions.append(Action("wait", seconds))
            continue

        screenshot = SCREENSHOT_RE.fullmatch(token)
        if screenshot:
            actions.append(Action("screenshot", screenshot.group(1)))
            continue

        presses = parse_button_presses(token)
        if presses is not None:
            actions.extend(
                Action("button_long" if press.long else "button", press.name)
                for press in presses
            )
            continue

        normalized = token.upper().replace("-", "_")
        if normalized in {"ESC", "KEY_ESC"}:
            actions.append(Action("escape"))
        elif normalized == "CLOSE":
            actions.append(Action("close"))
        else:
            raise ValueError(f"unknown sequence token: {token}")
    return actions


def validate_close(actions: list[Action]) -> list[Action]:
    close_positions = [index for index, action in enumerate(actions) if action.kind == "close"]
    if not close_positions:
        actions.append(Action("close"))
        close_positions = [len(actions) - 1]
    if len(close_positions) > 1:
        raise ValueError("a sample sequence may contain CLOSE only once")
    if close_positions[0] != len(actions) - 1:
        raise ValueError("CLOSE must be the final action in a sample sequence")

    held: set[str] = set()
    for action in actions:
        if action.kind == "hold":
            name = str(action.value)
            if name in held:
                raise ValueError(f"button already held: {name}")
            held.add(name)
        elif action.kind == "release":
            name = str(action.value)
            if name not in held:
                raise ValueError(f"button released without HOLD: {name}")
            held.remove(name)
        elif action.kind in {"button", "button_long"}:
            name = str(action.value)
            if name in held:
                raise ValueError(f"button pressed again while held: {name}")
        elif action.kind == "chord":
            chord = action.value
            assert isinstance(chord, tuple)
            overlap = held.intersection(press.name for press in chord)
            if overlap:
                raise ValueError(
                    "simultaneous sequence presses an already held button: "
                    + ", ".join(sorted(overlap))
                )
        elif action.kind == "close" and held:
            raise ValueError(
                "CLOSE reached with held button(s): " + ", ".join(sorted(held))
            )
    return actions


def load_campaign(args: argparse.Namespace) -> tuple[Settings, list[Sample]]:
    script = Path(__file__).resolve()
    repo_root = script.parents[1]
    config_path_abs = args.config.expanduser().resolve()
    with config_path_abs.open("rb") as stream:
        data = tomllib.load(stream)

    retroarch = table(data, "retroarch")
    campaign = table(data, "campaign")
    base = config_path_abs.parent

    game = str(campaign.get("game", "")).strip()
    if not game or not SUFFIX_RE.fullmatch(game):
        raise ValueError("campaign.game must be a valid short ROM-set name")

    baseline_sample_value = campaign.get("baseline_sample")
    baseline_sample = str(baseline_sample_value).strip() if baseline_sample_value else None
    if baseline_sample and not SUFFIX_RE.fullmatch(baseline_sample):
        raise ValueError("campaign.baseline_sample must be a valid sample suffix")
    standard_srm = config_path(campaign.get("standard_srm"), base)
    if baseline_sample and standard_srm:
        raise ValueError("use either campaign.baseline_sample or standard_srm, not both")
    standard_nvram = config_path(campaign.get("standard_nvram"), base)
    standard_eeprom = config_path(campaign.get("standard_eeprom"), base)
    configured_output = campaign.get(
        "output_dir",
        f"~/Documents/RetroArch/sm2-nvram-analysis/{game}/generated",
    )
    output_dir = args.output.expanduser().resolve() if args.output else config_path(configured_output, base)

    base_config_value = retroarch.get("base_config")

    settings = Settings(
        repo_root=repo_root,
        retroarch=config_path(retroarch.get("executable", ""), base),
        core=config_path(retroarch.get("core", ""), base),
        rom_dir=config_path(retroarch.get("rom_dir", ""), base),
        system_assets=config_path(retroarch.get("system_assets", "../build-libretro/libretro/system/sm2-emu"), base),
        base_config=config_path(base_config_value, base) if base_config_value else None,
        game=game,
        baseline_sample=baseline_sample,
        standard_srm=standard_srm,
        standard_nvram=standard_nvram,
        standard_eeprom=standard_eeprom,
        output_dir=output_dir,
        startup_wait=positive_number(campaign, "startup_wait", 10.0),
        button_hold=positive_number(campaign, "button_hold", 0.06),
        button_hold_long=positive_number(campaign, "button_hold_long", 0.5),
        default_wait=positive_number(campaign, "default_wait", 0.18),
        settle_before_close=positive_number(campaign, "settle_before_close", 0.5),
        close_gap=positive_number(campaign, "close_gap", 0.7),
        shutdown_timeout=positive_number(campaign, "shutdown_timeout", 15.0),
        window_width=positive_integer(campaign, "window_width", 496),
        window_height=positive_integer(campaign, "window_height", 384),
    )

    if args.interactive is not None:
        suffix = args.interactive.strip()
        if not SUFFIX_RE.fullmatch(suffix):
            raise ValueError(f"invalid interactive sample suffix: {suffix!r}")
        return settings, [Sample(suffix, ())]

    macro_data = data.get("macros", {})
    if not isinstance(macro_data, dict):
        raise ValueError("[macros] must be a TOML table")
    macros: dict[str, list[str]] = {}
    for name, value in macro_data.items():
        normalized = name.strip().lower()
        if not normalized or normalized in macros:
            raise ValueError(f"invalid or duplicate macro name: {name!r}")
        macros[normalized] = split_tokens(value, f"macro {name}", allow_empty=True)

    sample_data = data.get("samples")
    if not isinstance(sample_data, list) or not sample_data:
        raise ValueError("at least one [[samples]] entry is required")

    samples: list[Sample] = []
    seen: set[str] = set()
    for index, entry in enumerate(sample_data, 1):
        if not isinstance(entry, dict):
            raise ValueError(f"samples entry {index} must be a TOML table")
        suffix = str(entry.get("suffix", "")).strip()
        if not SUFFIX_RE.fullmatch(suffix):
            raise ValueError(f"invalid samples[{index}].suffix: {suffix!r}")
        if suffix in seen:
            raise ValueError(f"duplicate sample suffix: {suffix}")
        seen.add(suffix)
        tokens = split_tokens(entry.get("sequence"), f"sample {suffix} sequence")
        actions = validate_close(expand_actions(tokens, macros))
        samples.append(Sample(suffix, tuple(actions)))

    if baseline_sample and baseline_sample not in seen:
        raise ValueError("campaign.baseline_sample does not name a configured sample")

    requested = set(args.sample)
    unknown = sorted(requested - seen)
    if unknown:
        raise ValueError("unknown sample suffix(es): " + ", ".join(unknown))

    if requested:
        samples = [sample for sample in samples if sample.suffix in requested]
    elif args.resume_from:
        if args.resume_from not in seen:
            raise ValueError(f"unknown resume sample suffix: {args.resume_from}")
        start = next(index for index, sample in enumerate(samples) if sample.suffix == args.resume_from)
        samples = samples[start:]

    return settings, samples


def validate_paths(settings: Settings) -> None:
    required_files = [settings.retroarch, settings.core]
    if settings.base_config:
        required_files.append(settings.base_config)
    if settings.standard_nvram:
        required_files.append(settings.standard_nvram)
    if settings.standard_srm:
        required_files.append(settings.standard_srm)
    if settings.standard_eeprom:
        required_files.append(settings.standard_eeprom)
    for path in required_files:
        if not path.is_file():
            raise FileNotFoundError(path)
    if not settings.rom_dir.is_dir():
        raise FileNotFoundError(settings.rom_dir)
    if not settings.system_assets.is_dir():
        raise FileNotFoundError(settings.system_assets)
    rom = settings.rom_dir / f"{settings.game}.zip"
    if not rom.is_file():
        raise FileNotFoundError(rom)


def action_label(action: Action) -> str:
    if action.kind == "button":
        return str(action.value)
    if action.kind == "button_long":
        return f"LONG({action.value})"
    if action.kind == "chord":
        presses = action.value
        assert isinstance(presses, tuple)
        return "+".join(f"LONG({press.name})" if press.long else press.name for press in presses)
    if action.kind == "hold":
        return f"HOLD({action.value})"
    if action.kind == "release":
        return f"RELEASE({action.value})"
    if action.kind == "wait":
        return f"WAIT({float(action.value):g})"
    if action.kind == "escape":
        return "KEY_ESC"
    if action.kind == "screenshot":
        return f"SCREENSHOT({action.value})"
    return "CLOSE"


def print_dry_run(settings: Settings, samples: list[Sample]) -> None:
    print(f"Game:     {settings.game}")
    print(f"ROM:      {settings.rom_dir / (settings.game + '.zip')}")
    print(f"SRAM:     {settings.standard_srm or '(blank)'}")
    print(f"Baseline: {settings.baseline_sample or '(none)'}")
    print(f"NVRAM:    {settings.standard_nvram or '(blank)'}")
    print(f"EEPROM:   {settings.standard_eeprom or '(blank)'}")
    print(f"Output:   {settings.output_dir}")
    for sample in samples:
        nv_out = settings.output_dir / "saves" / f"{settings.game}-{sample.suffix}.nv"
        ep_out = settings.output_dir / "saves" / f"{settings.game}-{sample.suffix}.eeprom"
        srm_out = settings.output_dir / "saves" / f"{settings.game}-{sample.suffix}.srm"
        print(f"\n[{sample.suffix}] ->")
        print(f"  sram:    {srm_out}")
        print(f"  nvram:   {nv_out}")
        print(f"  eeprom:  {ep_out}")
        if sample.actions:
            print("  " + " -> ".join(action_label(action) for action in sample.actions))
        else:
            print("  INTERACTIVE (no scripted input; close RetroArch manually)")


def free_udp_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return int(sock.getsockname()[1])


def quote_config(value: Path | str) -> str:
    return str(value).replace("\\", "\\\\").replace('"', '\\"')


def write_isolated_config(
    settings: Settings, work: Path, remote_port: int, command_port: int,
) -> list[str]:
    for directory in (
        "system/sm2-emu", "saves", "states", "logs", "config", "remaps",
        "screenshots",
    ):
        (work / directory).mkdir(parents=True, exist_ok=True)

    for asset in settings.system_assets.iterdir():
        if asset.is_file():
            shutil.copy2(asset, work / "system" / "sm2-emu" / asset.name)

    core_options = work / "config" / "core-options.cfg"
    # Leave core options untouched for compatibility across Vulkan/Software builds.
    # If a future workflow needs a fixed renderer, add it explicitly in the
    # campaign's base_config file.
    core_options.write_text("", encoding="utf-8")

    overrides = {
        "config_save_on_exit": "false",
        "system_directory": work / "system",
        "savefile_directory": work / "saves",
        "savestate_directory": work / "states",
        "log_dir": work / "logs",
        "log_to_file": "true",
        "log_to_file_timestamp": "false",
        "log_verbosity": "true",
        "core_options_path": core_options,
        "global_core_options": "true",
        "input_remapping_directory": work / "remaps",
        "content_history_path": work / "config" / "content_history.lpl",
        "content_favorites_path": work / "config" / "content_favorites.lpl",
        "sort_savefiles_enable": "false",
        "sort_savefiles_by_content_enable": "false",
        "network_remote_enable": "true",
        "network_remote_enable_user_p1": "true",
        "network_remote_enable_user_p2": "true",
        "network_remote_base_port": str(remote_port),
        "input_max_users": "2",
        "network_cmd_enable": "true",
        "network_cmd_port": str(command_port),
        "fps_show": "false",
        "framecount_show": "false",
        "video_fullscreen": "false",
        "video_windowed_fullscreen": "false",
        "video_window_custom_size_enable": "true",
        "video_windowed_position_width": str(settings.window_width),
        "video_windowed_position_height": str(settings.window_height),
        "video_scale": "1.000000",
        "video_window_save_positions": "false",
        "video_shader_enable": "false",
        "input_screenshot": "f8",
        "auto_screenshot_filename": "true",
        "screenshot_directory": work / "screenshots",
        "pause_nonactive": "false",
        "savestate_auto_load": "false",
        "savestate_auto_save": "false",
        "audio_enable": "false",
        "autosave_interval": "0",
    }

    overrides_path = work / "nvram-samples-overrides.cfg"
    lines = ["# Isolated SM2 NVRAM sampling overrides"]
    lines.extend(f'{key} = "{quote_config(value)}"' for key, value in overrides.items())
    overrides_path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    if not settings.base_config:
        return ["-c", str(overrides_path)]

    base_copy = work / "base-retroarch.cfg"
    shutil.copy2(settings.base_config, base_copy)
    return ["-c", str(base_copy), f"--appendconfig={overrides_path}"]


def remote_packet(user: int, button: int, pressed: bool) -> bytes:
    # libretro network remote protocol payload accepted by RetroArch remote input.
    return struct.pack("@iiiiHxx", user, 1, 0, button, int(pressed))


def send_button_state(sock: socket.socket, base_port: int, name: str, pressed: bool) -> None:
    user, button = button_target(name)
    sock.sendto(remote_packet(user, button, pressed), ("127.0.0.1", base_port + user))


def send_button(sock: socket.socket, port: int, name: str, hold_seconds: float, settings: Settings) -> None:
    send_button_state(sock, port, name, True)
    time.sleep(hold_seconds)
    send_button_state(sock, port, name, False)
    time.sleep(settings.default_wait)


def send_chord(sock: socket.socket, port: int, presses: tuple[ButtonPress, ...], settings: Settings) -> None:
    for press in presses:
        send_button_state(sock, port, press.name, True)

    started = time.monotonic()
    releases = sorted((
        (settings.button_hold_long if press.long else settings.button_hold, press)
        for press in presses
    ), key=lambda item: item[0])

    while releases:
        due_time = releases[0][0]
        remaining = started + due_time - time.monotonic()
        if remaining > 0:
            time.sleep(remaining)
        due = [item for item in releases if item[0] == due_time]
        releases = [item for item in releases if item[0] != due_time]
        for _, press in due:
            send_button_state(sock, port, press.name, False)
    time.sleep(settings.default_wait)


def send_latched_button(sock: socket.socket, port: int, name: str, pressed: bool, settings: Settings) -> None:
    send_button_state(sock, port, name, pressed)
    time.sleep(settings.default_wait)


def post_key_to_pid(pid: int, keycode: int, label: str) -> None:
    application_services = ctypes.CDLL(
        "/System/Library/Frameworks/ApplicationServices.framework/ApplicationServices"
    )
    core_foundation = ctypes.CDLL(
        "/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation"
    )
    application_services.CGEventCreateKeyboardEvent.restype = ctypes.c_void_p
    application_services.CGEventCreateKeyboardEvent.argtypes = [
        ctypes.c_void_p, ctypes.c_ushort, ctypes.c_bool,
    ]
    application_services.CGEventPostToPid.argtypes = [ctypes.c_int, ctypes.c_void_p]
    core_foundation.CFRelease.argtypes = [ctypes.c_void_p]
    for pressed in (True, False):
        event = application_services.CGEventCreateKeyboardEvent(None, keycode, pressed)
        if not event:
            raise RuntimeError(f"CoreGraphics could not create a {label} key event")
        application_services.CGEventPostToPid(pid, event)
        core_foundation.CFRelease(event)
        time.sleep(0.03)


def post_escape_to_pid(pid: int) -> None:
    post_key_to_pid(pid, 53, "Esc")


def take_screenshot(
    process: subprocess.Popen[bytes], work: Path, command_port: int, label: str,
) -> Path:
    directory = work / "screenshots"
    before = set(directory.glob("*.png"))
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.sendto(b"SCREENSHOT\n", ("127.0.0.1", command_port))
    deadline = time.monotonic() + 5.0
    while time.monotonic() < deadline:
        created = sorted(
            set(directory.glob("*.png")) - before,
            key=lambda path: path.stat().st_mtime_ns,
        )
        if created:
            source = created[-1]
            destination = directory / f"{label}.png"
            if source != destination:
                source.replace(destination)
            return destination
        if process.poll() is not None:
            raise RuntimeError(f"RetroArch exited before screenshot {label}")
        time.sleep(0.10)
    raise RuntimeError(f"RetroArch did not create screenshot {label}")


def wait_while_alive(process: subprocess.Popen[bytes], seconds: float) -> None:
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        exit_code = process.poll()
        if exit_code is not None:
            raise RuntimeError(f"RetroArch exited early with code {exit_code}")
        time.sleep(min(0.10, deadline - time.monotonic()))


def activate_process(process: subprocess.Popen[bytes], timeout: float = 3.0) -> None:
    script = (
        'tell application "System Events" to set frontmost of first process '
        f'whose unix id is {process.pid} to true'
    )
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError(
                f"RetroArch exited before its window became active (code {process.returncode})"
            )
        completed = subprocess.run(
            ["/usr/bin/osascript", "-e", script],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        if completed.returncode == 0:
            return
        time.sleep(0.10)
    raise RuntimeError(
        "could not activate the new RetroArch instance; check macOS automation permissions"
    )


def close_retroarch(process: subprocess.Popen[bytes], settings: Settings) -> None:
    if process.poll() is not None:
        raise RuntimeError(f"RetroArch exited before CLOSE (code {process.returncode})")
    time.sleep(settings.settle_before_close)
    post_escape_to_pid(process.pid)
    time.sleep(settings.close_gap)
    if process.poll() is None:
        post_escape_to_pid(process.pid)
    try:
        exit_code = process.wait(timeout=settings.shutdown_timeout)
    except subprocess.TimeoutExpired as error:
        raise RuntimeError(
            "RetroArch did not exit after Esc, wait, Esc; check macOS Accessibility permission"
        ) from error
    if exit_code != 0:
        raise RuntimeError(f"RetroArch exited with code {exit_code}")


def terminate_process(process: subprocess.Popen[bytes]) -> None:
    if process.poll() is not None:
        return
    process.terminate()
    try:
        process.wait(timeout=5.0)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=5.0)


def execute_actions(
    process: subprocess.Popen[bytes], port: int, sample: Sample, settings: Settings,
    work: Path, command_port: int,
) -> list[Path]:
    screenshots: list[Path] = []
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        for action in sample.actions:
            if action.kind in {"button", "button_long"}:
                if process.poll() is not None:
                    raise RuntimeError(f"RetroArch exited before button {action.value}")
                hold_seconds = (
                    settings.button_hold_long
                    if action.kind == "button_long"
                    else settings.button_hold
                )
                send_button(sock, port, str(action.value), hold_seconds, settings)
            elif action.kind == "chord":
                if process.poll() is not None:
                    raise RuntimeError("RetroArch exited before simultaneous buttons")
                presses = action.value
                assert isinstance(presses, tuple)
                send_chord(sock, port, presses, settings)
            elif action.kind in {"hold", "release"}:
                if process.poll() is not None:
                    raise RuntimeError(
                        f"RetroArch exited before {action.kind.upper()}({action.value})"
                    )
                send_latched_button(
                    sock, port, str(action.value), action.kind == "hold", settings
                )
            elif action.kind == "wait":
                wait_while_alive(process, float(action.value))
            elif action.kind == "escape":
                if process.poll() is not None:
                    raise RuntimeError("RetroArch exited before KEY_ESC")
                post_escape_to_pid(process.pid)
                time.sleep(settings.default_wait)
            elif action.kind == "screenshot":
                screenshots.append(
                    take_screenshot(process, work, command_port, str(action.value))
                )
                time.sleep(settings.default_wait)
            elif action.kind == "close":
                close_retroarch(process, settings)
    return screenshots


def copy_if_present(source: Path, destination: Path) -> str | None:
    if not source.is_file():
        return None
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
    return str(destination)


def find_saves(work: Path, game: str) -> list[Path]:
    candidates: list[Path] = []
    for ext in ("srm",):
        found = sorted((work / "saves").rglob(f"{game}.{ext}"))
        if found:
            candidates.append(found[0])
    return candidates


def run_sample(
    settings: Settings,
    sample: Sample,
    overwrite: bool,
    keep_workdirs: bool,
    interactive: bool = False,
) -> Result:
    stem = f"{settings.game}-{sample.suffix}"
    nv_out = settings.output_dir / "saves" / f"{stem}.nv"
    ep_out = settings.output_dir / "saves" / f"{stem}.eeprom"
    srm_out = settings.output_dir / "saves" / f"{stem}.srm"

    expected = [srm_out, nv_out, ep_out]
    if srm_out.exists() and not overwrite:
        return Result(
            stem,
            "skipped",
            [str(path) for path in expected if path.exists()],
            [],
            str(settings.output_dir / "logs" / f"{stem}.log") if (settings.output_dir / "logs" / f"{stem}.log").is_file() else "",
            str(settings.output_dir / "logs" / f"{stem}.console.log") if (settings.output_dir / "logs" / f"{stem}.console.log").is_file() else "",
            0.0,
            "sample already exists; use --overwrite to replace it",
        )

    started = time.monotonic()
    temporary = tempfile.TemporaryDirectory(prefix=f"sm2-nvram-{stem}-")
    work = Path(temporary.name)
    port = free_udp_port()
    command_port = free_udp_port()
    config_args = write_isolated_config(settings, work, port, command_port)

    baseline_srm = settings.standard_srm
    if settings.baseline_sample and sample.suffix != settings.baseline_sample:
        baseline_srm = (settings.output_dir / "saves" /
                        f"{settings.game}-{settings.baseline_sample}.srm")
        if not baseline_srm.is_file():
            raise RuntimeError(
                f"baseline sample is missing: {baseline_srm}; run "
                f"{settings.baseline_sample} first"
            )
    if baseline_srm:
        shutil.copy2(baseline_srm, work / "saves" / f"{settings.game}.srm")
    if settings.standard_nvram:
        shutil.copy2(settings.standard_nvram,
                     work / "saves" / f"{settings.game}.nv")
    if settings.standard_eeprom:
        shutil.copy2(settings.standard_eeprom,
                     work / "saves" / f"{settings.game}.eeprom")

    core_log = work / "logs" / f"{stem}.log"
    console_tmp = work / "logs" / f"{stem}.console.log"

    rom = settings.rom_dir / f"{settings.game}.zip"
    command = [
        str(settings.retroarch),
        "-v",
        *config_args,
        "-L",
        str(settings.core),
        f"--log-file={core_log}",
        str(rom),
    ]

    status = "failed"
    note = ""
    saved_files: list[str] = []
    screenshot_files: list[str] = []
    captured: list[Path] = []
    process: subprocess.Popen[bytes] | None = None

    try:
        with console_tmp.open("wb") as console:
            process = subprocess.Popen(command, stdout=console, stderr=subprocess.STDOUT)
            activate_process(process)
            wait_while_alive(process, settings.startup_wait)

            if interactive:
                print(
                    "       interactive: configure the game, then close RetroArch "
                    "normally (Esc, wait, Esc)",
                    flush=True,
                )
                exit_code = process.wait()
                if exit_code != 0:
                    raise RuntimeError(f"RetroArch exited with code {exit_code}")
            else:
                captured = execute_actions(
                    process, port, sample, settings, work, command_port
                )

        generated = find_saves(work, settings.game)
        generated_srm = next((path for path in generated if path.suffix == ".srm"), None)
        if not generated_srm:
            raise RuntimeError("RetroArch exited cleanly but no frontend .srm was found")

        for path in generated:
            destination = settings.output_dir / "saves" / path.name
            if path.suffix == ".srm":
                destination = srm_out
            elif path.suffix == ".nv":
                destination = nv_out
            elif path.suffix == ".eeprom":
                destination = ep_out
            copied = copy_if_present(path, destination)
            if copied:
                saved_files.append(copied)

        image = srm_out.read_bytes()
        header_size, nvram_size, eeprom_size = 64, 16 * 1024, 128
        if (len(image) != header_size + nvram_size + eeprom_size or
                image[:8] != b"SM2SRAM\0"):
            raise RuntimeError("frontend .srm has an unsupported SM2 save layout")
        nv_out.write_bytes(image[header_size:header_size + nvram_size])
        ep_out.write_bytes(image[header_size + nvram_size:])
        saved_files.extend((str(nv_out), str(ep_out)))

        if not saved_files:
            raise RuntimeError("NVRAM files were not copied to output")

        for path in captured:
            copied = copy_if_present(
                path,
                settings.output_dir / "screenshots" / f"{stem}-{path.name}",
            )
            if copied:
                screenshot_files.append(copied)

        status = "ok"
        note = (
            "interactive sample saved after manual RetroArch shutdown"
            if interactive
            else "sample saved from fresh save directory"
        )

    except KeyboardInterrupt:
        if process and process.poll() is None:
            try:
                close_retroarch(process, settings)
            except RuntimeError:
                terminate_process(process)
        raise
    except (OSError, RuntimeError) as error:
        note = str(error)
        if process:
            terminate_process(process)
    finally:
        log = copy_if_present(core_log, settings.output_dir / "logs" / f"{stem}.log")
        console_log = copy_if_present(
            console_tmp,
            settings.output_dir / "logs" / f"{stem}.console.log",
        )
        if keep_workdirs:
            kept = settings.output_dir / "work" / stem
            if kept.exists():
                shutil.rmtree(kept)
            shutil.copytree(work, kept)
        temporary.cleanup()

    return Result(
        stem,
        status,
        saved_files,
        screenshot_files,
        log or "",
        console_log or "",
        round(time.monotonic() - started, 3),
        note,
    )


def write_summary(output: Path, results: list[Result]) -> None:
    rows = [asdict(result) for result in results]
    (output / "summary.json").write_text(json.dumps(rows, indent=2) + "\n", encoding="utf-8")


def main() -> int:
    if sys.platform != "darwin":
        raise RuntimeError("this NVRAM sampling utility currently supports macOS only")

    args = parse_args()
    settings, samples = load_campaign(args)
    validate_paths(settings)

    if args.dry_run:
        print_dry_run(settings, samples)
        return 0

    for directory in ("saves", "logs"):
        (settings.output_dir / directory).mkdir(parents=True, exist_ok=True)
    print(f"Output: {settings.output_dir}")
    mode = "interactive" if args.interactive is not None else "scripted"
    print(f"Game: {settings.game}; samples: {len(samples)}; mode: {mode}")

    results: list[Result] = []
    for index, sample in enumerate(samples, 1):
        print(f"[{index:02}/{len(samples):02}] {sample.suffix}", flush=True)
        result = run_sample(
            settings,
            sample,
            args.overwrite,
            args.keep_workdirs,
            interactive=args.interactive is not None,
        )
        results.append(result)
        write_summary(settings.output_dir, results)
        print(f"       {result.status}: {result.note}", flush=True)

    return 1 if any(result.status not in {"ok", "skipped"} for result in results) else 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (FileNotFoundError, FileExistsError, KeyError, OSError, RuntimeError,
            TypeError, ValueError, tomllib.TOMLDecodeError) as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(2)

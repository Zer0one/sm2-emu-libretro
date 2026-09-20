#!/usr/bin/env python3
"""Exercise SM2 linked cabinets through isolated RetroArch instances.

The lifecycle follows the established NVRAM/smoke-test runners: every process
is recorded immediately, all paths are isolated, and cleanup always waits for
termination before returning.  The script refuses to start while another copy
of the selected RetroArch executable is running.
"""

from __future__ import annotations

import argparse
import atexit
import json
import os
import shutil
import socket
import subprocess
import tempfile
import time
from dataclasses import asdict, dataclass
from pathlib import Path

from libretro_nvram_samples import (
    activate_process,
    free_udp_port,
    position_process_window,
    post_escape_to_pid,
    wait_while_alive,
)


PREFERENCES_DOMAIN = "com.libretro.dist.RetroArch"
IGNORE_STATE_KEY = "ApplePersistenceIgnoreState"
WINDOW_WIDTH = 496
WINDOW_HEIGHT = 384
# RetroArch adds the native macOS frame around the configured content area.
WINDOW_FRAME_WIDTH = 512
WINDOW_FRAME_HEIGHT = 412
WINDOW_GAP = 16
WINDOW_ORIGIN_X = 24
WINDOW_ORIGIN_Y = 40


@dataclass(frozen=True)
class GameSpec:
    max_cabinets: int
    nvram_game: str
    nvram_scheme: str


GAME_SPECS = {
    "daytona": GameSpec(8, "daytona", "daytona"),
    "daytonas": GameSpec(8, "daytonas", "daytona"),
    "daytonase": GameSpec(8, "daytona", "daytona"),
    "daytonam": GameSpec(8, "daytona", "daytona"),
    "stcc": GameSpec(9, "stcc", "car"),
    "stcca": GameSpec(9, "stcca", "car"),
    "stccb": GameSpec(9, "stccb", "car"),
    "stcco": GameSpec(9, "stcco", "car"),
    "srallyc": GameSpec(5, "srallyc", "car"),
    "srallycb": GameSpec(5, "srallyc", "car"),
    "srallycc": GameSpec(5, "srallyc", "car"),
    "indy500": GameSpec(8, "indy500", "role_id"),
    "indy500d": GameSpec(8, "indy500d", "role_id"),
    "indy500to": GameSpec(8, "indy500", "role_id"),
    "motoraid": GameSpec(4, "motoraid", "role_id"),
    "motoraiddx": GameSpec(4, "motoraiddx", "role_id"),
    "waverunr": GameSpec(4, "waverunr", "role_id"),
    "skisuprg": GameSpec(4, "skisuprg", "role_id"),
    "sgt24h": GameSpec(4, "sgt24h", "sgt24h"),
    "overrev": GameSpec(4, "overrev", "overrev"),
    "overrevb": GameSpec(4, "overrev", "overrev"),
    "overrevba": GameSpec(4, "overrev", "overrev"),
    "manxtt": GameSpec(3, "manxtt", "link_role"),
    "manxttc": GameSpec(3, "manxtt", "link_role"),
    "von": GameSpec(3, "von", "network_attribute"),
    "vonj": GameSpec(3, "von", "network_attribute"),
    "vonu": GameSpec(3, "von", "network_attribute"),
}

VIRTUAL_ON_TWIN_SETS = {"von", "vonj", "vonu"}
VIRTUAL_ON_RELAY_SPEC = GameSpec(3, "von", "relay_program")
OPTIONAL_RELAY_SETS = {
    "stcc", "stcca", "stccb", "stcco",
    "srallyc", "srallycb", "srallycc",
    "motoraid", "motoraiddx",
}


@dataclass
class InstanceResult:
    role: str
    pid: int
    exit_code: int | None = None
    forced_kill: bool = False
    roster_ready: bool = False
    log: str = ""


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--retroarch", type=Path, required=True)
    parser.add_argument("--core", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument(
        "--relay-rom", type=Path,
        help="Dedicated vonr ROM used as the third Virtual On Relay instance",
    )
    parser.add_argument(
        "--include-relay", action="store_true",
        help="Use the final cabinet as Relay, or Live for Motor Raid",
    )
    parser.add_argument(
        "--base-config", type=Path,
        default=Path("~/Library/Application Support/RetroArch/config/retroarch.cfg"),
        help="RetroArch configuration copied before isolated overrides",
    )
    parser.add_argument(
        "--system-assets", type=Path, required=True,
        help="Directory containing games.xml and the remaining SM2 system files",
    )
    parser.add_argument(
        "--set-name", choices=tuple(GAME_SPECS), default="daytona",
        help="Set with verified linked-cabinet NVRAM options",
    )
    parser.add_argument("--cabinets", type=int, choices=range(2, 10), default=3)
    parser.add_argument("--port", type=int, default=55435)
    parser.add_argument("--startup-wait", type=float, default=5.0)
    parser.add_argument("--settle", type=float, default=10.0)
    parser.add_argument(
        "--window-layout", choices=("side-by-side", "cascade"),
        default="side-by-side",
        help="Arrange isolated RetroArch windows side by side or in a cascade",
    )
    parser.add_argument(
        "--window-columns", type=int, default=2,
        help="Number of columns used by the side-by-side layout (default: 2)",
    )
    parser.add_argument(
        "--output", type=Path,
        help="Persistent result directory; default is a unique temporary directory",
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Create and validate the isolated environments without launching RetroArch",
    )
    return parser.parse_args()


def quote_config(value: Path | str) -> str:
    return str(value).replace("\\", "\\\\").replace('"', '\\"')


def window_position(index: int, layout: str, columns: int) -> tuple[int, int]:
    if layout == "cascade":
        return WINDOW_ORIGIN_X + index * 60, WINDOW_ORIGIN_Y + index * 40
    column = index % columns
    row = index // columns
    return (
        WINDOW_ORIGIN_X + column * (WINDOW_FRAME_WIDTH + WINDOW_GAP),
        WINDOW_ORIGIN_Y + row * (WINDOW_FRAME_HEIGHT + WINDOW_GAP),
    )


def process_rows() -> list[tuple[int, str]]:
    completed = subprocess.run(
        ["/bin/ps", "-ax", "-o", "pid=,command="],
        check=True,
        capture_output=True,
        text=True,
    )
    rows: list[tuple[int, str]] = []
    for line in completed.stdout.splitlines():
        fields = line.strip().split(maxsplit=1)
        if len(fields) == 2 and fields[0].isdigit():
            rows.append((int(fields[0]), fields[1]))
    return rows


def retroarch_processes(executable: Path) -> list[tuple[int, str]]:
    prefix = str(executable)
    return [(pid, command) for pid, command in process_rows()
            if command == prefix or command.startswith(prefix + " ")]


def port_available(port: int) -> bool:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            sock.bind(("127.0.0.1", port))
        except OSError:
            return False
    return True


def read_ignore_state() -> bool | None:
    completed = subprocess.run(
        ["/usr/bin/defaults", "read", PREFERENCES_DOMAIN, IGNORE_STATE_KEY],
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0:
        return None
    value = completed.stdout.strip().lower()
    if value in {"1", "true", "yes"}:
        return True
    if value in {"0", "false", "no"}:
        return False
    raise RuntimeError(f"unsupported {IGNORE_STATE_KEY} preference: {value!r}")


def write_ignore_state(value: bool | None) -> None:
    if value is None:
        subprocess.run(
            ["/usr/bin/defaults", "delete", PREFERENCES_DOMAIN, IGNORE_STATE_KEY],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        return
    subprocess.run(
        ["/usr/bin/defaults", "write", PREFERENCES_DOMAIN, IGNORE_STATE_KEY,
         "-bool", "true" if value else "false"],
        check=True,
    )


def write_config(root: Path, assets: Path, base_config: Path, port: int,
                 command_port: int, cabinets: int, window_x: int,
                 window_y: int) -> list[str]:
    for directory in ("system/sm2-emu", "saves", "states", "logs", "config", "remaps"):
        (root / directory).mkdir(parents=True, exist_ok=True)
    for asset in assets.iterdir():
        if asset.is_file():
            shutil.copy2(asset, root / "system" / "sm2-emu" / asset.name)

    values = {
        "config_save_on_exit": "false",
        "system_directory": root / "system",
        "savefile_directory": root / "saves",
        "savestate_directory": root / "states",
        "log_dir": root / "logs",
        "log_to_file": "false",
        "log_to_file_timestamp": "false",
        "log_verbosity": "true",
        "core_options_path": root / "config" / "core-options.cfg",
        "global_core_options": "true",
        "game_specific_options": "false",
        "input_remapping_directory": root / "remaps",
        "input_exit_emulator": "escape",
        "quit_press_twice": "false",
        "auto_overrides_enable": "false",
        "auto_remaps_enable": "false",
        "rgui_config_directory": root / "config",
        "content_history_enable": "false",
        "content_history_path": root / "config" / "content_history.lpl",
        "content_favorites_path": root / "config" / "content_favorites.lpl",
        "sort_savefiles_enable": "false",
        "sort_savefiles_by_content_enable": "false",
        "savestate_auto_load": "false",
        "savestate_auto_save": "false",
        "pause_nonactive": "false",
        "video_driver": "gl",
        "video_fullscreen": "false",
        "video_windowed_fullscreen": "false",
        "video_window_custom_size_enable": "true",
        "video_windowed_position_width": str(WINDOW_WIDTH),
        "video_windowed_position_height": str(WINDOW_HEIGHT),
        "video_windowed_position_x": str(window_x),
        "video_windowed_position_y": str(window_y),
        "video_scale": "1.000000",
        "video_window_save_positions": "false",
        "video_shader_enable": "false",
        "audio_driver": "null",
        "audio_enable": "false",
        "microphone_driver": "null",
        "microphone_enable": "false",
        "camera_allow": "false",
        "location_allow": "false",
        "ui_companion_enable": "false",
        "netplay_ip_port": str(port),
        "netplay_max_connections": str(cabinets),
        "netplay_public_announce": "false",
        "netplay_use_mitm_server": "false",
        "netplay_nat_traversal": "false",
        "netplay_check_frames": "0",
        "netplay_nickname": root.name,
        "network_cmd_enable": "true",
        "network_cmd_port": str(command_port),
    }
    overrides = root / "netpacket-overrides.cfg"
    overrides.write_text(
        "".join(f'{key} = "{quote_config(value)}"\n' for key, value in values.items()),
        encoding="utf-8",
    )
    base_copy = root / "base-retroarch.cfg"
    shutil.copy2(base_config, base_copy)
    return ["-c", str(base_copy), f"--appendconfig={overrides}"]


def write_options(
    root: Path,
    set_name: str,
    cabinets: int,
    index: int,
    include_relay: bool,
) -> None:
    spec = VIRTUAL_ON_RELAY_SPEC if set_name == "vonr" else GAME_SPECS[set_name]
    options = {
        f"sm2_linked_cabinets_{set_name}": str(cabinets),
        "sm2_renderer": "software",
        "sm2_initial_nvram_setup": "enabled",
        "sm2_nvram_settings": "enabled",
    }
    if spec.nvram_scheme == "daytona":
        options[f"sm2_nvram_{spec.nvram_game}_link_id"] = (
            "master" if index == 0 else "slave"
        )
        options[f"sm2_nvram_{spec.nvram_game}_car_number"] = str(index + 1)
    elif spec.nvram_scheme == "car":
        car_limit = 4 if spec.nvram_game == "srallyc" else 8
        is_relay = index == cabinets - 1 if include_relay else index == car_limit
        options[f"sm2_nvram_{spec.nvram_game}_link_type"] = (
            "relay" if is_relay else f"car_{index + 1}"
        )
        if spec.nvram_game == "srallyc":
            options["sm2_nvram_srallyc_cabinet_type"] = "twin"
    elif spec.nvram_scheme == "role_id":
        is_live = (
            include_relay
            and spec.nvram_game in {"motoraid", "motoraiddx"}
            and index == cabinets - 1
        )
        network_type = "master" if index == 0 else "slave"
        if is_live:
            network_type = "live"
        options[f"sm2_nvram_{spec.nvram_game}_network_type"] = network_type
        options[f"sm2_nvram_{spec.nvram_game}_cabinet_id"] = str(index + 1)
    elif spec.nvram_scheme == "sgt24h":
        role = "master" if index == 0 else "slave"
        options["sm2_nvram_sgt24h_link_type"] = f"car_no{index + 1}_{role}"
        options["sm2_nvram_sgt24h_link_max"] = str(cabinets)
    elif spec.nvram_scheme == "overrev":
        role = "master" if index == 0 else "slave"
        options["sm2_nvram_overrev_link_max"] = f"{cabinets}_links"
        options["sm2_nvram_overrev_link_type"] = f"{role}_carno_{index + 1}"
    elif spec.nvram_scheme == "link_role":
        options[f"sm2_nvram_{spec.nvram_game}_link_type"] = (
            "master" if index == 0 else "slave" if index == 1 else "relay"
        )
    elif spec.nvram_scheme == "network_attribute":
        options["sm2_nvram_von_network_link_attribute"] = (
            "master" if index == 0 else "slave"
        )
    elif spec.nvram_scheme == "relay_program":
        options["sm2_nvram_von_network_link_attribute"] = "no_link"
    else:
        raise RuntimeError(f"unsupported NVRAM scheme: {spec.nvram_scheme}")
    path = root / "config" / "core-options.cfg"
    path.write_text(
        "".join(f"{key} = {json.dumps(value)}\n" for key, value in options.items()),
        encoding="utf-8",
    )


def terminate_instance(process: subprocess.Popen[bytes]) -> bool:
    if process.poll() is not None:
        return False
    process.terminate()
    try:
        process.wait(timeout=5.0)
        return False
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=5.0)
        return True


def log_contains(path: Path, text: str) -> bool:
    try:
        return text in path.read_text(encoding="utf-8", errors="replace")
    except FileNotFoundError:
        return False


def close_instance(process: subprocess.Popen[bytes], command_port: int) -> bool:
    if process.poll() is not None:
        return False
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.sendto(b"QUIT\n", ("127.0.0.1", command_port))
        process.wait(timeout=8.0)
        return False
    except (OSError, subprocess.TimeoutExpired):
        try:
            post_escape_to_pid(process.pid)
            time.sleep(1.0)
            if process.poll() is None:
                post_escape_to_pid(process.pid)
            process.wait(timeout=5.0)
            return False
        except (OSError, RuntimeError, subprocess.TimeoutExpired):
            return terminate_instance(process)


def validate_paths(
    args: argparse.Namespace,
) -> tuple[Path, Path, Path, Path, Path, Path | None]:
    retroarch = args.retroarch.expanduser().resolve(strict=True)
    core = args.core.expanduser().resolve(strict=True)
    rom = args.rom.expanduser().resolve(strict=True)
    base_config = args.base_config.expanduser().resolve(strict=True)
    assets = args.system_assets.expanduser().resolve(strict=True)
    relay_rom = args.relay_rom.expanduser().resolve(strict=True) if args.relay_rom else None
    if not assets.is_dir() or not (assets / "games.xml").is_file():
        raise RuntimeError(f"invalid SM2 system-assets directory: {assets}")
    if rom.stem != args.set_name:
        raise RuntimeError(f"ROM filename {rom.name!r} does not match --set-name {args.set_name!r}")
    if args.set_name in VIRTUAL_ON_TWIN_SETS and args.cabinets == 3:
        if relay_rom is None or relay_rom.stem != "vonr":
            raise RuntimeError("three-instance Virtual On requires --relay-rom vonr.zip")
    elif relay_rom is not None:
        raise RuntimeError("--relay-rom is used only by three-instance Virtual On")
    return retroarch, core, rom, base_config, assets, relay_rom


def main() -> int:
    args = parse_args()
    spec = GAME_SPECS[args.set_name]
    if args.cabinets > spec.max_cabinets:
        raise RuntimeError(
            f"{args.set_name} supports at most {spec.max_cabinets} linked cabinets"
        )
    if args.window_columns < 1:
        raise RuntimeError("--window-columns must be at least 1")
    if args.include_relay:
        if args.set_name not in OPTIONAL_RELAY_SETS:
            raise RuntimeError(
                f"{args.set_name} does not use the optional Relay/Live role"
            )
        if args.cabinets < 3:
            raise RuntimeError("--include-relay requires at least three cabinets")
    retroarch, core, rom, base_config, assets, relay_rom = validate_paths(args)
    running = retroarch_processes(retroarch)
    if running:
        details = ", ".join(str(pid) for pid, _ in running)
        raise RuntimeError(f"RetroArch is already running (PID {details}); no test was started")
    if not port_available(args.port):
        raise RuntimeError(f"netplay port {args.port} is already in use")

    if args.output:
        output = args.output.expanduser().resolve()
        output.mkdir(parents=True, exist_ok=False)
    else:
        output = Path(tempfile.mkdtemp(prefix=f"sm2-netpacket-{args.set_name}-"))

    roles = ["host", *(f"client{index}" for index in range(1, args.cabinets))]
    instance_sets = [args.set_name for _ in roles]
    instance_roms = [rom for _ in roles]
    if relay_rom is not None:
        instance_sets[-1] = "vonr"
        instance_roms[-1] = relay_rom
    command_ports = [free_udp_port() for _ in roles]
    if len(set(command_ports)) != len(command_ports) or args.port in command_ports:
        raise RuntimeError("could not allocate distinct RetroArch command ports")
    configs: list[Path] = []
    window_positions = [
        window_position(index, args.window_layout, args.window_columns)
        for index in range(args.cabinets)
    ]
    for index, role in enumerate(roles):
        root = output / role
        window_x, window_y = window_positions[index]
        configs.append(write_config(root, assets, base_config, args.port,
                                    command_ports[index], args.cabinets,
                                    window_x, window_y))
        write_options(
            root, instance_sets[index], args.cabinets, index, args.include_relay
        )

    manifest = {
        "retroarch": str(retroarch),
        "core": str(core),
        "rom": str(rom),
        "base_config": str(base_config),
        "set_name": args.set_name,
        "instance_sets": instance_sets,
        "instance_roms": [str(path) for path in instance_roms],
        "cabinets": args.cabinets,
        "include_relay": args.include_relay,
        "port": args.port,
        "command_ports": command_ports,
        "roles": roles,
        "window_layout": args.window_layout,
        "window_columns": args.window_columns,
        "window_positions": [
            {"role": role, "x": position[0], "y": position[1],
             "content_width": WINDOW_WIDTH, "content_height": WINDOW_HEIGHT,
             "frame_width": WINDOW_FRAME_WIDTH,
             "frame_height": WINDOW_FRAME_HEIGHT}
            for role, position in zip(roles, window_positions)
        ],
    }
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    if args.dry_run:
        print(json.dumps({**manifest, "output": str(output), "status": "dry-run"}, indent=2))
        return 0

    processes: list[subprocess.Popen[bytes]] = []
    logs: list[object] = []
    results: list[InstanceResult] = []
    success = False
    completed_run = False
    previous_ignore_state = read_ignore_state()
    atexit.register(write_ignore_state, previous_ignore_state)
    write_ignore_state(True)
    try:
        for index, role in enumerate(roles):
            log_path = output / f"{role}.log"
            log = log_path.open("wb")
            logs.append(log)
            netplay = ["-H"] if index == 0 else ["-C", "127.0.0.1"]
            command = [
                str(retroarch), "-v", *configs[index],
                "-L", str(core), *netplay, str(instance_roms[index]),
            ]
            process = subprocess.Popen(command, cwd=output, stdout=log,
                                       stderr=subprocess.STDOUT)
            processes.append(process)
            results.append(InstanceResult(role, process.pid, log=str(log_path)))
            (output / "pids.json").write_text(
                json.dumps([asdict(item) for item in results], indent=2) + "\n"
            )
            activate_process(process)
            position_process_window(process, *window_positions[index])
            wait_while_alive(process, args.startup_wait)

        deadline = time.monotonic() + args.settle
        while time.monotonic() < deadline:
            exited = [(item.role, process.returncode) for item, process in zip(results, processes)
                      if process.poll() is not None]
            if exited:
                raise RuntimeError(f"RetroArch exited during the linked-cabinet run: {exited}")
            time.sleep(0.20)
        completed_run = True
    finally:
        for index in reversed(range(len(processes))):
            item = results[index]
            process = processes[index]
            item.forced_kill = close_instance(process, command_ports[index])
            item.exit_code = process.returncode
        for log in logs:
            log.close()

        roster_marker = f", {args.cabinets} cabinets"
        for item in results:
            item.roster_ready = (
                log_contains(Path(item.log), "Linked-cabinet roster ready: participant ")
                and log_contains(Path(item.log), roster_marker)
            )
        host_log = Path(results[0].log) if results else None
        host_complete = bool(host_log) and all(
            log_contains(host_log, f"Slave cabinet connected ({count}/{args.cabinets})")
            for count in range(2, args.cabinets + 1)
        )
        logs_clean = all(
            not log_contains(Path(item.log), marker)
            for item in results
            for marker in (
                "Additional cabinet rejected",
                "Linked cabinets use different games",
                "Failed to connect to host",
            )
        )
        success = (completed_run and len(results) == args.cabinets
                   and all(item.roster_ready for item in results)
                   and host_complete and logs_clean)

        residual = [(pid, command) for pid, command in retroarch_processes(retroarch)
                    if pid in {item.pid for item in results}]
        report = {
            **manifest,
            "output": str(output),
            "status": "ok" if success and not residual else "failed",
            "instances": [asdict(item) for item in results],
            "residual_processes": [pid for pid, _ in residual],
        }
        (output / "result.json").write_text(json.dumps(report, indent=2) + "\n")
        print(json.dumps(report, indent=2))
        write_ignore_state(previous_ignore_state)
        atexit.unregister(write_ignore_state)
        if residual:
            raise RuntimeError(f"launched RetroArch processes still alive: {residual}")
    return 0 if success else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError) as error:
        print(f"error: {error}", file=os.sys.stderr)
        raise SystemExit(1)

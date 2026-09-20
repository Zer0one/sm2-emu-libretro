#!/usr/bin/env python3
"""Verify a Libretro save/load round-trip in an isolated RetroArch process."""

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import socket
import subprocess
import time

from libretro_nvram_samples import (
    activate_process,
    free_udp_port,
    post_escape_to_pid,
    post_key_to_pid,
    take_screenshot,
    wait_while_alive,
)


def quote(value: Path | str) -> str:
    return str(value).replace("\\", "\\\\").replace('"', '\\"')


def wait_for_state(directory: Path, process: subprocess.Popen[bytes]) -> Path:
    deadline = time.monotonic() + 10.0
    while time.monotonic() < deadline:
        states = sorted(directory.rglob("*.state*"), key=lambda path: path.stat().st_mtime_ns)
        if states and states[-1].stat().st_size:
            return states[-1]
        if process.poll() is not None:
            raise RuntimeError(f"RetroArch exited before writing the state ({process.returncode})")
        time.sleep(0.10)
    raise RuntimeError("RetroArch did not write a save state")


def send_command(command_port: int, command: str) -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.sendto(f"{command}\n".encode(), ("127.0.0.1", command_port))


def toggle_pause(command_port: int) -> None:
    send_command(command_port, "PAUSE_TOGGLE")


def crop_game_region(source: Path, destination: Path) -> None:
    """Exclude RetroArch notifications while retaining a large game region."""
    subprocess.run(
        [
            "/usr/bin/sips", "--cropToHeightWidth", "900", "800",
            "--cropOffset", "300", "650", str(source), "--out", str(destination),
        ],
        check=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def quit_retroarch(process: subprocess.Popen[bytes], command_port: int) -> bool:
    if process.poll() is not None:
        return False
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.sendto(b"QUIT\n", ("127.0.0.1", command_port))
    try:
        process.wait(timeout=8.0)
        return False
    except subprocess.TimeoutExpired:
        try:
            post_escape_to_pid(process.pid)
            time.sleep(1.0)
            if process.poll() is None:
                post_escape_to_pid(process.pid)
            process.wait(timeout=5.0)
            return False
        except (OSError, RuntimeError, subprocess.TimeoutExpired):
            process.terminate()
        try:
            process.wait(timeout=5.0)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=5.0)
        return True


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("retroarch", "core", "system", "rom", "output"):
        parser.add_argument(f"--{name}", type=Path, required=True)
    parser.add_argument("--replay", type=Path)
    parser.add_argument("--set-name", required=True)
    parser.add_argument("--boot-wait", type=float, default=38.0)
    parser.add_argument("--advance-wait", type=float, default=3.0)
    args = parser.parse_args()

    retroarch = args.retroarch.resolve(strict=True)
    core = args.core.resolve(strict=True)
    system = args.system.resolve(strict=True)
    rom = args.rom.resolve(strict=True)
    replay = args.replay.resolve(strict=True) if args.replay else None
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    for name in ("saves", "states", "screenshots", "config"):
        (output / name).mkdir()

    command_port = free_udp_port()
    core_options = output / "core-options.cfg"
    core_options.write_text(
        'sm2_renderer = "software"\n'
        'sm2_linked_cabinets = "disabled"\n',
        encoding="utf-8",
    )
    config_values = {
        "config_save_on_exit": "false",
        "system_directory": system,
        "savefile_directory": output / "saves",
        "savestate_directory": output / "states",
        "screenshot_directory": output / "screenshots",
        "core_options_path": core_options,
        "global_core_options": "true",
        "network_cmd_enable": "true",
        "network_cmd_port": str(command_port),
        "video_driver": "gl",
        "audio_driver": "coreaudio",
        "input_driver": "cocoa",
        "video_fullscreen": "false",
        "video_windowed_fullscreen": "false",
        "video_scale": "2",
        "video_vsync": "false",
        "audio_sync": "true",
        "audio_enable": "true",
        "pause_nonactive": "false",
        "savestate_auto_save": "false",
        "savestate_auto_load": "false",
        "savestate_auto_index": "false",
        "savestate_file_compression": "false",
        "savestate_thumbnail_enable": "false",
        "state_slot": "0",
        "input_pause_toggle": "p",
        "input_save_state": "f2",
        "input_load_state": "f4",
        "video_shader_enable": "false",
        "video_threaded": "false",
        "auto_overrides_enable": "false",
        "auto_remaps_enable": "false",
    }
    config = output / "retroarch.cfg"
    config.write_text(
        "".join(f'{key} = "{quote(value)}"\n' for key, value in config_values.items()),
        encoding="utf-8",
    )
    command = [str(retroarch), "-v", "-c", str(config), "-L", str(core)]
    if replay:
        local_replay = output / "input.replay"
        shutil.copy2(replay, local_replay)
        command.extend(["-P", str(local_replay)])
    command.append(str(rom))
    (output / "command.json").write_text(json.dumps(command, indent=2) + "\n")

    forced = False
    with (output / "run.log").open("wb") as log:
        process = subprocess.Popen(command, cwd=output, stdout=log, stderr=subprocess.STDOUT)
        try:
            activate_process(process)
            wait_while_alive(process, args.boot_wait)
            toggle_pause(command_port)
            wait_while_alive(process, 1.0)
            send_command(command_port, "SAVE_STATE")
            state = wait_for_state(output / "states", process)
            wait_while_alive(process, 2.5)
            saved = take_screenshot(process, output, command_port, "saved")

            toggle_pause(command_port)
            wait_while_alive(process, args.advance_wait)
            toggle_pause(command_port)
            wait_while_alive(process, 1.0)
            advanced = take_screenshot(process, output, command_port, "advanced")

            # Resume, issue the configured load hotkey and immediately pause,
            # then capture the restored framebuffer without wall-clock drift.
            toggle_pause(command_port)
            post_key_to_pid(process.pid, 118, "F4")
            toggle_pause(command_port)
            wait_while_alive(process, 1.0)
            restored = take_screenshot(process, output, command_port, "restored")
        finally:
            forced = quit_retroarch(process, command_port)

    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    saved_hash = digest(saved)
    advanced_hash = digest(advanced)
    restored_hash = digest(restored)
    saved_crop = output / "screenshots" / "saved-game-region.png"
    restored_crop = output / "screenshots" / "restored-game-region.png"
    crop_game_region(saved, saved_crop)
    crop_game_region(restored, restored_crop)
    saved_game_hash = digest(saved_crop)
    restored_game_hash = digest(restored_crop)
    log_text = (output / "run.log").read_text(encoding="utf-8", errors="replace")
    result = {
        "set_name": args.set_name,
        "state": str(state),
        "state_size": state.stat().st_size,
        "saved_screenshot": str(saved),
        "advanced_screenshot": str(advanced),
        "restored_screenshot": str(restored),
        "saved_sha256": saved_hash,
        "advanced_sha256": advanced_hash,
        "restored_sha256": restored_hash,
        "saved_game_region_sha256": saved_game_hash,
        "restored_game_region_sha256": restored_game_hash,
        "advanced_changed": advanced_hash != saved_hash,
        "restored_game_region_exactly": restored_game_hash == saved_game_hash,
        "save_log_message": (
            "Salvataggio dello stato" in log_text or "Saved state" in log_text
        ),
        "load_log_message": (
            "Caricamento dello stato" in log_text or "Loaded state" in log_text
        ),
        "exit_code": process.returncode,
        "forced_kill": forced,
    }
    (output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    if not result["advanced_changed"]:
        raise RuntimeError("The game image did not advance after saving")
    if not result["restored_game_region_exactly"]:
        raise RuntimeError("The loaded state did not restore the saved video frame exactly")
    if process.returncode != 0 or forced:
        raise RuntimeError("RetroArch did not close cleanly")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Compare a fresh headless run with an unmodified upstream --boot-test run.

ROMs and all output stay external to Git. Requires only the Python standard library.
"""
import argparse
import array
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import wave


def positive(value):
    number = int(value)
    if number <= 0:
        raise argparse.ArgumentTypeError("must be positive")
    return number


def sha(data):
    return hashlib.sha256(data).hexdigest()


def run(command, cwd, log, timeout):
    with log.open("w") as output:
        subprocess.run(command, cwd=cwd, stdout=output, stderr=subprocess.STDOUT,
                       check=True, timeout=timeout)


def audio(path):
    with wave.open(str(path), "rb") as stream:
        params = (stream.getnchannels(), stream.getsampwidth(), stream.getframerate(),
                  stream.getnframes(), stream.getcomptype())
        return params, stream.readframes(stream.getnframes())


def nvram(directory):
    return {str(p.relative_to(directory)): sha(p.read_bytes())
            for p in sorted(directory.rglob("*")) if p.is_file()}


def field(log, name):
    match = re.search(r"^" + re.escape(name) + r"\s*:\s*(.+)$", log, re.MULTILINE)
    if not match:
        raise RuntimeError(f"Missing field in run log: {name}")
    return match.group(1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--headless", type=Path, required=True)
    parser.add_argument("--upstream", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--game", required=True)
    parser.add_argument("--frames", type=positive, default=1800)
    parser.add_argument("--timeout", type=positive, default=300)
    parser.add_argument("--output", type=Path, required=True,
                        help="new output directory; existing paths are refused")
    args = parser.parse_args()
    headless = args.headless.resolve(strict=True)
    upstream = args.upstream.resolve(strict=True)
    rom = args.rom.resolve(strict=True)
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    original = out / "upstream"
    original.mkdir()
    adapted = out / "headless"  # The runner creates this directory itself.
    reference_command = [str(upstream), "--game", args.game, "--boot-test", str(args.frames),
                         "--config", str(original / "config"), "--nvram", str(original / "nvram"),
                         "--dump-tilemap", str(original / "video"),
                         "--dump-audio", str(original / "audio.wav"), str(rom)]
    headless_command = [str(headless), "--game", args.game, "--frames", str(args.frames),
                        "--output", str(adapted), str(rom)]
    print(f"{args.game}: running upstream and headless for {args.frames} frames", flush=True)
    run(reference_command, original, out / "upstream.log", args.timeout)
    run(headless_command, out, out / "headless.log", args.timeout)

    expected_video = (original / "video/software_frame.ppm").read_bytes()
    actual_video = (adapted / "software_frame.ppm").read_bytes()
    expected_audio, expected_pcm = audio(original / "audio.wav")
    actual_audio, actual_pcm = audio(adapted / "audio.wav")
    expected_nvram, actual_nvram = nvram(original / "nvram"), nvram(adapted / "nvram")
    reference_log = (out / "upstream.log").read_text()
    headless_log = (out / "headless.log").read_text()
    checks = {
        "video_identical": expected_video == actual_video,
        "audio_format_identical": expected_audio == actual_audio,
        "audio_pcm_identical": expected_pcm == actual_pcm,
        "nvram_identical": expected_nvram == actual_nvram,
        "requested_frames": field(reference_log, "frames run") == str(args.frames)
                            == field(headless_log, "frames run"),
        "cycles_identical": field(reference_log, "master cycles") == field(headless_log, "master cycles"),
        "cpu_state_identical": field(reference_log, "cpu state") == field(headless_log, "cpu state"),
    }
    samples = array.array("h", actual_pcm)
    if sys.byteorder != "little":
        samples.byteswap()
    report = {
        "game": args.game, "frames": args.frames, "checks": checks,
        "commands": {"upstream": reference_command, "headless": headless_command},
        "video_sha256": sha(actual_video), "pcm_sha256": sha(actual_pcm),
        "audio": {"channels": actual_audio[0], "sample_bytes": actual_audio[1],
                  "sample_rate": actual_audio[2], "stereo_frames": actual_audio[3],
                  "peak": max((abs(s) for s in samples), default=0)},
        "nvram": actual_nvram,
    }
    (out / "comparison.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"game": args.game, "checks": checks, "audio": report["audio"]}, indent=2))
    return 0 if all(checks.values()) else 1


if __name__ == "__main__":
    raise SystemExit(main())

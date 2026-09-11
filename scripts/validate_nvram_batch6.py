#!/usr/bin/env python3
"""Validate the sixth multi-game SM2 diagnostic-menu NVRAM campaign."""
from __future__ import annotations

import argparse
import json
import struct
import tomllib
from pathlib import Path

HEADER = 64
NVRAM = 16 * 1024
EEPROM = 128
COUNTS = {"manxtt": 60, "motoraid": 25, "segawski": 8, "waverunr": 22, "skisuprg": 105}


def crc16(data: bytes) -> int:
    crc = 0
    for value in data:
        crc ^= value << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def field(storage: str, offset: int, value: int, mirror: int | None = None) -> tuple:
    items = [(offset, value)]
    if mirror is not None:
        items.append((mirror, value))
    return storage, tuple(items)


def fields(storage: str, *items: tuple[int, int]) -> tuple:
    return storage, items


EXPECTED: dict[str, dict[str, tuple]] = {game: {} for game in COUNTS}

# Manx TT: one EEPROM settings area with mode-specific duplicated fields.
X = EXPECTED["manxtt"]
for name, value in (("on", 1), ("off", 0)):
    X[f"advertise-sound-{name}"] = field("eeprom", 0x08, value)
for name, value in (("japan", 0), ("usa", 1), ("export", 2)):
    X[f"country-{name}"] = field("eeprom", 0x09, value)
for name, value in (("deluxe", 0), ("twin", 1)):
    X[f"cabinet-type-{name}"] = field("eeprom", 0x0A, value)
for name, value in (("not-link", 0), ("master", 1), ("slave", 2), ("relay", 3)):
    X[f"link-type-{name}"] = field("eeprom", 0x0B, value)
for name, value in (
    ("red-no1", 0), ("blue-no2", 1), ("yellow-no3", 2), ("green-no4", 3),
    ("green-no5", 4), ("yellow-no6", 5), ("blue-no7", 6), ("red-no8", 7),
):
    X[f"bike-color-{name}"] = field("eeprom", 0x0C, value)
for name, value in (("race", 0), ("tt", 1)):
    X[f"race-mode-{name}"] = field("eeprom", 0x0D, value)
for prefix, offsets in (("laxey", (0x0E, 0x10)), ("tt", (0x0F, 0x11))):
    for name, value in (("normal", 1), ("hard", 2), ("hardest", 3), ("easy", 0)):
        X[f"{prefix}-game-difficulty-{name}"] = fields("eeprom", *((offset, value) for offset in offsets))
for name, value in (("2-of-3", 2), ("3-of-3", 3), ("not-revise", 0), ("1-of-3", 1)):
    X[f"laxey-revise-mode-{name}"] = field("eeprom", 0x12, value)
    X[f"tt-revise-mode-{name}"] = fields("eeprom", (0x13, value), (0x14, value))
for prefix, offsets in (("laxey", (0x16, 0x18)), ("tt", (0x17, 0x19))):
    for number in range(1, 11):
        X[f"{prefix}-number-of-lap-{number}"] = fields("eeprom", *((offset, number - 1) for offset in offsets))
for name, value in (("on", 1), ("off", 0)):
    X[f"start-switch-op-{name}"] = field("eeprom", 0x1A, value)

# Motor Raid: Sega linked-racing menu, mirrored 36-byte EEPROM settings bank.
M = EXPECTED["motoraid"]
for name, value in (("normal", 2), ("hard", 3), ("very-hard", 4), ("very-easy", 0), ("easy", 1)):
    M[f"game-difficulty-{name}"] = field("eeprom", 0x19, value, 0x3D)
for name, value in (("standard", 0), ("long", 1)):
    M[f"race-mode-{name}"] = field("eeprom", 0x18, value, 0x3C)
for name, value in (("normal", 0), ("strong", 1)):
    M[f"enemy-level-{name}"] = field("eeprom", 0x1A, value, 0x3E)
for name, value in (("standard", 0), ("half", 1)):
    M[f"engine-volume-{name}"] = field("eeprom", 0x1C, value, 0x40)
for name, value in (("on", 1), ("off", 0)):
    M[f"advertise-sound-{name}"] = field("eeprom", 0x1B, value, 0x3F)
for name, value in (("japan", 0), ("usa", 1), ("exp", 2)):
    M[f"country-{name}"] = field("eeprom", 0x16, value, 0x3A)
for name, value in (("stand-alone", 0), ("master", 1), ("slave", 2), ("live", 3)):
    M[f"network-type-{name}"] = field("eeprom", 0x1E, value, 0x42)
for number in range(1, 5):
    M[f"cabinet-id-{number}"] = field("eeprom", 0x1F, number - 1, 0x43)

# Sega Water Ski: minimal menu, mirrored 32-byte EEPROM settings bank.
W = EXPECTED["segawski"]
for name, value in (("normal", 2), ("hard", 3), ("hardest", 4), ("very-easy", 0), ("easy", 1)):
    W[f"game-difficulty-{name}"] = field("eeprom", 0x19, value, 0x39)
for name, value in (("off", 0), ("on", 1)):
    W[f"advertise-sound-{name}"] = field("eeprom", 0x1A, value, 0x3A)

# Wave Runner: same 36-byte storage family, with its own option set.
R = EXPECTED["waverunr"]
for name, value in (("normal", 2), ("hard", 3), ("hardest", 4), ("very-easy", 0), ("easy", 1)):
    R[f"game-difficulty-{name}"] = field("eeprom", 0x19, value, 0x3D)
for name, value in (("normal", 0), ("long", 1)):
    R[f"race-mode-{name}"] = field("eeprom", 0x18, value, 0x3C)
for name, value in (("on", 1), ("off", 0)):
    R[f"handicap-{name}"] = field("eeprom", 0x1A, value, 0x3E)
for name, value in (("off", 0), ("on", 1)):
    R[f"advertise-sound-{name}"] = field("eeprom", 0x1B, value, 0x3F)
for name, value in (("japan", 0), ("usa", 1), ("exp", 2)):
    R[f"country-{name}"] = field("eeprom", 0x16, value, 0x3A)
for name, value in (("stand-alone", 0), ("master", 1), ("slave", 2)):
    R[f"network-type-{name}"] = field("eeprom", 0x1E, value, 0x42)
for number in range(1, 5):
    R[f"cabinet-id-{number}"] = field("eeprom", 0x1F, number - 1, 0x43)

# Sega Ski Super G: extended 40-byte settings bank and four independent timers.
S = EXPECTED["skisuprg"]
for name, value in (("on", 1), ("off", 0)):
    S[f"advertise-sound-{name}"] = field("eeprom", 0x1F, value, 0x47)
for name, value in (("japan", 0), ("usa", 1), ("export", 2)):
    S[f"country-{name}"] = field("eeprom", 0x19, value, 0x41)
for name, value in (("no-link", 0), ("master", 1), ("slave", 2)):
    S[f"network-type-{name}"] = field("eeprom", 0x20, value, 0x48)
for number in range(1, 5):
    S[f"cabinet-id-{number}"] = field("eeprom", 0x21, number - 1, 0x49)
    S[f"drive-board-power-{number}"] = field("eeprom", 0x22, number - 1, 0x4A)
for key, offset in (("live-display", 0x23), ("victoria-display", 0x24)):
    for name, value in (("on", 1), ("off", 0)):
        S[f"{key}-{name}"] = field("eeprom", offset, value, offset + 0x28)
for prefix, offset in (
    ("initial-time", 0x1B),
    ("stage-time-white-forest", 0x1C),
    ("stage-time-night-valley", 0x1D),
    ("stage-time-wild-king", 0x1E),
):
    for number in range(21):
        S[f"{prefix}-{number}"] = field("eeprom", offset, number, offset + 0x28)


def validate_field(stem: str, nvram: bytes, eeprom: bytes, expected: tuple) -> None:
    storage, items = expected
    data = nvram if storage == "nv" else eeprom
    for offset, value in items:
        assert data[offset] == value, f"{stem}: {storage}[{offset:#x}] != {value:#x}"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", nargs="?", type=Path, default=Path.home() / "Documents/RetroArch/sm2-nvram-analysis")
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--game", action="append", choices=COUNTS, help="validate only this game; repeat as needed")
    args = parser.parse_args()
    total = 0
    selected = args.game or list(COUNTS)
    for game in selected:
        count = COUNTS[game]
        root = args.root.expanduser().resolve() / game / "generated"
        rows = json.loads((root / "summary.json").read_text())
        config = tomllib.loads((args.repo / "scripts" / f"libretro_nvram_samples.{game}.toml").read_text())
        wanted = {entry["suffix"] for entry in config["samples"]}
        names = {row["sample"].removeprefix(game + "-") for row in rows}
        assert len(rows) == count and names == wanted == {"base", *EXPECTED[game]}, f"{game}: sample set mismatch ({len(rows)})"
        for row in rows:
            name = row["sample"].removeprefix(game + "-")
            stem = f"{game}-{name}"
            assert row["status"] == "ok", f"{stem}: {row['note']}"
            sram = (root / "saves" / f"{stem}.srm").read_bytes()
            nvram = (root / "saves" / f"{stem}.nv").read_bytes()
            eeprom = (root / "saves" / f"{stem}.eeprom").read_bytes()
            assert len(sram) == HEADER + NVRAM + EEPROM and sram[:8] == b"SM2SRAM\0", stem
            assert len(nvram) == NVRAM and len(eeprom) == EEPROM, stem
            assert int.from_bytes(sram[8:12], "little") == 1, stem
            assert int.from_bytes(sram[12:16], "little") == NVRAM, stem
            assert int.from_bytes(sram[16:20], "little") == EEPROM, stem
            assert sram[24:56].split(b"\0", 1)[0].decode() == game, stem
            assert int.from_bytes(sram[20:24], "little") == crc16(sram[HEADER:]), stem
            assert sram[HEADER:HEADER + NVRAM] == nvram and sram[HEADER + NVRAM:] == eeprom, stem
            screenshots = [Path(path) for path in row["screenshots"]]
            assert len(screenshots) == 1, f"{stem}: screenshot count {len(screenshots)}"
            assert struct.unpack(">II", screenshots[0].read_bytes()[16:24]) == (1024, 768), stem
            if game == "skisuprg":
                assert screenshots[0].stat().st_size >= 8500, f"{stem}: diagnostic menu screenshot missing"
            if game == "segawski":
                assert eeprom[0x08:0x28] == eeprom[0x28:0x48], f"{stem}: EEPROM mirror"
            elif game in {"motoraid", "waverunr"}:
                assert eeprom[0x08:0x2C] == eeprom[0x2C:0x50], f"{stem}: EEPROM mirror"
            elif game == "skisuprg":
                assert eeprom[0x08:0x30] == eeprom[0x30:0x58], f"{stem}: EEPROM mirror"
            elif game == "manxtt":
                assert nvram[:0x16AC] == nvram[0x16AC:0x2D58], f"{stem}: backup RAM mirror"
            if name in EXPECTED[game]:
                validate_field(stem, nvram, eeprom, EXPECTED[game][name])
        print(f"{game}: validated {len(rows)} samples")
        total += len(rows)
    print(f"Validated {total} samples: containers, memories, screenshots, fields and mirrors")


if __name__ == "__main__":
    main()

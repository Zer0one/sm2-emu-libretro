#!/usr/bin/env python3
"""Validate the fifth multi-game SM2 diagnostic-menu NVRAM campaign."""
from __future__ import annotations

import argparse
import json
import struct
import tomllib
from pathlib import Path

HEADER = 64
NVRAM = 16 * 1024
EEPROM = 128
COUNTS = {"overrev": 36, "sgt24h": 33, "stcc": 39, "rchase2": 22}


def crc16(data: bytes) -> int:
    crc = 0
    for value in data:
        crc ^= value << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def field(storage: str, offset: int, value: int, mask: int = 0xFF) -> tuple:
    return (storage, ((offset, mask, value),))


def fields(storage: str, *items: tuple[int, int, int]) -> tuple:
    return (storage, items)


EXPECTED: dict[str, dict[str, tuple]] = {game: {} for game in COUNTS}

# Over Rev: two nested menus in a mirrored EEPROM settings bank.
O = EXPECTED["overrev"]
for name, value in (("japan", 0), ("usa", 1), ("europe", 2), ("korea", 3)):
    O[f"country-{name}"] = field("eeprom", 0x26, value)
for name, value in (("normal-2in1", 0), ("sega-indy", 2), ("sega-indy-dx-gt24h-dx", 3), ("sega-stcc", 4)):
    O[f"hardware-type-{name}"] = field("eeprom", 0x27, value)
for name, value in (("not-link", 0), ("2-links", 1), ("3-links", 2), ("4-links", 3)):
    O[f"link-max-{name}"] = field("eeprom", 0x36, value)
for name, value in (("master-car-no1", 0), ("slave-car-no2", 1), ("slave-car-no3", 2), ("slave-car-no4", 3)):
    O[f"link-type-{name}"] = field("eeprom", 0x35, value)
for number in range(1, 6):
    O[f"time-difficulty-{number}"] = field("eeprom", 0x28, number - 1)
    O[f"steer-kick-back-{number}"] = field("eeprom", 0x2B, number - 1)
for name, value in (("normal", 1), ("hard", 2), ("very-hard", 3), ("easy", 0)):
    O[f"game-difficulty-{name}"] = field("eeprom", 0x2A, value)
O["demo-sound-on"] = field("eeprom", 0x1A, 0)
O["demo-sound-off"] = field("eeprom", 0x1A, 1)
lap_offsets = tuple(range(0x2C, 0x35))
for name, values in (
    ("normal-mode", (1, 3, 3, 2, 6, 2, 2, 1, 3)),
    ("long-mode", (2, 4, 4, 3, 9, 3, 3, 2, 4)),
    ("very-long-mode", (3, 6, 6, 4, 12, 4, 4, 2, 6)),
):
    O[f"maximum-lap-{name}"] = fields("eeprom", *((off, 0xFF, val) for off, val in zip(lap_offsets, values)))

# Sega GT 24h: normal settings are in backup RAM; link settings use mirrored EEPROM banks.
S = EXPECTED["sgt24h"]
link_values = {
    "not-link": (1, 1, 0),
    "car-no1-master": (0, 1, 0),
    "car-no2-slave": (0, 0, 1),
    "car-no3-slave": (0, 0, 2),
    "car-no4-slave": (0, 0, 3),
}
for name, values in link_values.items():
    S[f"link-type-{name}"] = fields("eeprom", *((off, 0xFF, val) for off, val in zip((0x1A, 0x1B, 0x1C), values)))
for number in range(2, 5):
    S[f"link-max-{number}"] = field("eeprom", 0x19, number)
for name, value in (("japan", 0), ("usa", 1), ("eur", 2)):
    S[f"country-{name}"] = field("nv", 0x1A, value)
for name, value in (("normal", 0), ("easy", 1), ("hard", 2), ("hardest", 3)):
    S[f"game-difficulty-{name}"] = field("nv", 0x14, value)
for number in range(1, 5):
    S[f"cpu-car-level-{number}"] = field("nv", 0x16, number - 1)
    S[f"steering-force-{number}"] = field("nv", 0x18, number - 1)
for key, offset in (("race-mode", 0x1C), ("game-bgm", 0x1E), ("demo-sound", 0x20)):
    first, second = (("normal", "long") if key == "race-mode" else ("on", "off"))
    S[f"{key}-{first}"] = field("nv", offset, 0)
    S[f"{key}-{second}"] = field("nv", offset, 1)
for name, value in (("a", 1), ("b", 3), ("c", 0)):
    S[f"io-type-{name}"] = field("nv", 0x0A, value)

# Sega Touring Car Championship: compact EEPROM bit fields.
T = EXPECTED["stcc"]
T["advertise-sound-on"] = field("eeprom", 0x10, 1, 0x01)
T["advertise-sound-off"] = field("eeprom", 0x10, 0, 0x01)
T["url-address-on"] = field("eeprom", 0x12, 0x10, 0x10)
T["url-address-off"] = field("eeprom", 0x12, 0, 0x10)
for name, main, extra18, extra28 in (("japan", 0, 2, 9), ("usa", 2, 0x12, 0x18), ("export", 4, 0x22, 9)):
    T[f"country-{name}"] = fields("eeprom", (0x10, 0x06, main), (0x18, 0xFF, extra18), (0x28, 0xFF, extra28))
T["cabinet-type-twin"] = field("eeprom", 0x10, 0, 0x08)
T["cabinet-type-deluxe"] = field("eeprom", 0x10, 8, 0x08)
for index, name in enumerate(("stand-alone", "car-1", "car-2", "car-3", "car-4", "car-5", "car-6", "car-7", "car-8", "relay")):
    code = index
    T[f"link-type-{name}"] = fields("eeprom", (0x10, 0xE0, (code << 5) & 0xE0), (0x11, 0x01, (code >> 3) & 1))
for name, value in (("normal", 0), ("easy", 2), ("hard", 4), ("hardest", 6)):
    T[f"difficulty-{name}"] = field("eeprom", 0x11, value, 0x06)
for name, value in (("normal", 0), ("short", 8), ("long", 16), ("grand-prix", 24)):
    T[f"game-mode-{name}"] = field("eeprom", 0x11, value, 0x18)
for name, value in (("random", 0), ("alfa-romeo", 32), ("mercedes", 64), ("opel", 96), ("toyota", 128)):
    T[f"default-car-{name}"] = field("eeprom", 0x11, value, 0xE0)
for name, value in (("before-3", 0), ("after-3", 2), ("before-7", 4), ("after-7", 6)):
    T[f"name-entry-{name}"] = field("eeprom", 0x12, value, 0x06)
T["default-view-birds"] = field("eeprom", 0x12, 0x20, 0x20)
T["default-view-drivers"] = field("eeprom", 0x12, 0, 0x20)

# Rail Chase 2: four direct fields in EEPROM.
R = EXPECTED["rchase2"]
R["advertise-sound-on"] = field("eeprom", 0x08, 1)
R["advertise-sound-off"] = field("eeprom", 0x08, 0)
for name, value in (("japan", 0), ("usa", 1), ("export", 2)):
    R[f"country-{name}"] = field("eeprom", 0x09, value)
for number in range(1, 9):
    R[f"game-difficulty-{number}-of-8"] = field("eeprom", 0x0C, number - 1)
for name, value in (("10-sec", 0), ("20-sec", 1), ("30-sec", 2), ("40-sec", 3), ("50-sec", 4), ("60-sec", 5), ("70-sec", 6), ("none", 7)):
    R[f"shifting-difficulty-{name}"] = field("eeprom", 0x0A, value)


def validate_field(stem: str, nvram: bytes, eeprom: bytes, expected: tuple) -> None:
    storage, items = expected
    data = nvram if storage == "nv" else eeprom
    for offset, mask, value in items:
        assert data[offset] & mask == value, f"{stem}: {storage}[{offset:#x}] & {mask:#x} != {value:#x}"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", nargs="?", type=Path, default=Path.home() / "Documents/RetroArch/sm2-nvram-analysis")
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    total = 0
    for game, count in COUNTS.items():
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
            if game == "overrev":
                assert eeprom[0x08:0x3C] == eeprom[0x3C:0x70], f"{stem}: EEPROM mirror"
            elif game == "sgt24h":
                assert nvram[0:2] == b"\x85\xad", f"{stem}: backup RAM signature"
                assert eeprom[0x10:0x12] == b"\x85\xad", f"{stem}: link EEPROM signature"
                stored_sum = int.from_bytes(eeprom[0x08:0x0A], "little")
                assert stored_sum == sum(eeprom[0x0A:0x28]) & 0xFFFF, f"{stem}: link EEPROM checksum"
                assert eeprom[0x08:0x28] == eeprom[0x28:0x48], f"{stem}: link EEPROM mirror"
            elif game == "stcc":
                assert nvram[:0x490] == nvram[0x490:0x920], f"{stem}: backup RAM mirror"
            else:
                assert nvram[:0xB94] == nvram[0xB94:0x1728], f"{stem}: backup RAM mirror"
            if name in EXPECTED[game]:
                validate_field(stem, nvram, eeprom, EXPECTED[game][name])
        print(f"{game}: validated {len(rows)} samples")
        total += len(rows)
    print(f"Validated {total} samples: containers, memories, screenshots, fields, checksums and mirrors")


if __name__ == "__main__":
    main()

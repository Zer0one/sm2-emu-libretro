#!/usr/bin/env python3
"""Validate the second multi-game SM2 diagnostic-menu NVRAM campaign."""
from __future__ import annotations

import argparse
import json
import struct
import tomllib
from pathlib import Path

HEADER = 64
NVRAM = 16 * 1024
EEPROM = 128


def crc16(data: bytes) -> int:
    crc = 0
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def swapped_words(data: bytes) -> bytes:
    assert len(data) % 2 == 0
    return b"".join(data[i:i + 2][::-1] for i in range(0, len(data), 2))


BEL = {
    **{f"coin-credit-setting-{n:02d}": [("eeprom", 0x0A, n - 1)] for n in range(1, 28)},
    **{f"start-credits-{n}": [("eeprom", 0x0F, n)] for n in range(1, 6)},
    **{f"continue-credits-{n}": [("eeprom", 0x10, n)] for n in range(1, 6)},
    "country-u-s": [("eeprom", 0x15, 1)],
    "country-export": [("eeprom", 0x15, 2)],
    "country-japan": [("eeprom", 0x15, 0)],
    "advertise-sound-on": [("eeprom", 0x13, 1)],
    "advertise-sound-off": [("eeprom", 0x13, 0)],
    **{f"difficulty-{n}": [("eeprom", 0x22, n - 1)] for n in range(1, 11)},
}

DAYTONA = {
    **{f"link-id-{name}": [("nv", 0x0B, value)] for name, value in [("master", 1), ("slave", 2), ("single", 0)]},
    **{f"car-number-{n}": [("nv", 0x0C, n - 1)] for n in range(1, 9)},
    **{f"cabinet-{name}": [("nv", 0x1A, value)] for name, value in [("twin", 1), ("upright", 2), ("deluxe", 0)]},
    **{f"country-{name}": [("nv", 0x1B, value)] for name, value in [("jpn", 1), ("export", 2), ("usa", 0)]},
    **{f"difficulty-{name}": [("nv", 0x20, value)] for name, value in [("normal", 1), ("hard", 2), ("hardest", 3), ("easy", 0)]},
    "advertise-sound-on": [("nv", 0x19, 0)],
    "advertise-sound-off": [("nv", 0x19, 1)],
    **{f"game-mode-{name}": [("nv", 0x1C, value)] for name, value in [("normal", 0), ("grand-prix", 1), ("endurance", 2)]},
    "rival-arrow-on": [("nv", 0x1D, 0)],
    "rival-arrow-off": [("nv", 0x1D, 1)],
}

DESERT_PRESETS = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17, 18, 21, 27]
DESERT = {
    **{f"coin-credit-setting-{n:02d}": [("nv", 0x10, n - 1)] for n in DESERT_PRESETS},
    **{f"country-{name}": [("nv", 0x1B, value)] for name, value in [("japan", 0), ("usa", 1), ("export", 2)]},
    "advertise-sound-on": [("nv", 0x19, 1)],
    "advertise-sound-off": [("nv", 0x19, 0)],
    **{f"difficulty-desert-{name}": [("nv", 0x21, value)] for value, name in enumerate(["easy", "normal", "hard", "hardest", "monkey"])},
    **{f"difficulty-canyon-{name}": [("nv", 0x22, value)] for value, name in enumerate(["easy", "normal", "hard", "hardest", "monkey"])},
}

DOA = {
    **{f"vjcom-set-count-{n}": [("eeprom", 0x1A, n)] for n in range(2, 6)},
    **{f"vjman-set-count-{n}": [("eeprom", 0x1B, n)] for n in range(2, 6)},
    **{f"vjcom-difficulty-{name}": [("eeprom", 0x17, value)] for value, name in enumerate(["normal", "easy", "hard", "very-hard"])},
    **{f"vjcom-energy-{name}": [("eeprom", 0x18, value)] for value, name in enumerate(["normal", "easy", "hard", "very-hard"])},
    **{f"vjman-energy-{name}": [("eeprom", 0x19, value)] for value, name in enumerate(["normal", "easy", "hard", "very-hard"])},
    "demo-sound-on": [("eeprom", 0x1C, 1)],
    "demo-sound-off": [("eeprom", 0x1C, 0)],
    **{f"nation-{name}": [("eeprom", 0x1E, value)] for name, value in [("japan", 0), ("usa", 1), ("export", 2)]},
    "continue-on": [("eeprom", 0x1D, 1)],
    "continue-off": [("eeprom", 0x1D, 0)],
    "vs-finish-off": [("eeprom", 0x1F, 0)],
    **{f"vs-finish-{n}": [("eeprom", 0x1F, n)] for n in range(1, 11)},
    "burst-mode-off": [("eeprom", 0x20, 0)],
    "burst-mode-on": [("eeprom", 0x20, 1)],
}

EXPECTED = {"bel": BEL, "daytona": DAYTONA, "desert": DESERT, "doa": DOA}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", nargs="?", type=Path, default=Path.home() / "Documents/RetroArch/sm2-nvram-analysis")
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    total = 0
    for game, expected in EXPECTED.items():
        root = args.root.expanduser().resolve() / game / "generated"
        rows = json.loads((root / "summary.json").read_text())
        config = tomllib.loads((args.repo / "scripts" / f"libretro_nvram_samples.{game}.toml").read_text())
        wanted = {sample["suffix"] for sample in config["samples"]}
        names = {row["sample"].removeprefix(game + "-") for row in rows}
        assert names == wanted, f"{game}: sample set mismatch: {sorted(names ^ wanted)}"
        assert wanted == {"base", *expected}, f"{game}: validator/config mismatch"
        for row in rows:
            name = row["sample"].removeprefix(game + "-")
            assert row["status"] == "ok", f"{game}/{name}: {row['note']}"
            stem = f"{game}-{name}"
            srm = (root / "saves" / f"{stem}.srm").read_bytes()
            nv = (root / "saves" / f"{stem}.nv").read_bytes()
            ep = (root / "saves" / f"{stem}.eeprom").read_bytes()
            assert len(srm) == HEADER + NVRAM + EEPROM and srm[:8] == b"SM2SRAM\0", stem
            assert int.from_bytes(srm[8:12], "little") == 1, stem
            assert int.from_bytes(srm[12:16], "little") == NVRAM, stem
            assert int.from_bytes(srm[16:20], "little") == EEPROM, stem
            assert srm[24:56].split(b"\0", 1)[0].decode("ascii") == game, stem
            assert int.from_bytes(srm[20:24], "little") == crc16(srm[HEADER:]), stem
            assert len(nv) == NVRAM and len(ep) == EEPROM, stem
            assert srm[HEADER:HEADER + NVRAM] == nv and srm[HEADER + NVRAM:] == ep, stem
            shots = list((root / "screenshots").glob(f"{stem}-*.png"))
            assert len(shots) == 1 and shots[0].read_bytes().startswith(b"\x89PNG\r\n\x1a\n"), stem
            assert struct.unpack(">II", shots[0].read_bytes()[16:24]) == (1024, 768), stem
            if game == "bel":
                assert ep[:0x40] == ep[0x40:0x80], f"{stem}: EEPROM mirror mismatch"
            elif game == "daytona":
                assert nv[:0x80] == nv[0x80:0x100], f"{stem}: backup-RAM mirror mismatch"
                for start in (0, 0x80):
                    assert int.from_bytes(nv[start + 8:start + 10], "little") == crc16(nv[start + 10:start + 0x80]), f"{stem}: settings CRC mismatch"
                assert ep == swapped_words(nv[:0x80]), f"{stem}: EEPROM word order mismatch"
            elif game == "desert":
                assert int.from_bytes(nv[8:10], "little") == crc16(nv[10:0x80]), f"{stem}: settings CRC mismatch"
                assert ep == swapped_words(nv[:0x80]), f"{stem}: EEPROM word order mismatch"
            elif game == "doa":
                assert ep[0x08:0x2C] == ep[0x2C:0x50], f"{stem}: EEPROM mirror mismatch"
                for start in (0x08, 0x2C):
                    bank = ep[start:start + 36]
                    assert bank[0] == sum(bank[1:]) & 0xFF, f"{stem}: additive checksum mismatch"
            for storage, offset, value in expected.get(name, []):
                image = nv if storage == "nv" else ep
                assert image[offset] == value, f"{stem}: {storage}[{offset:#x}]={image[offset]}, expected {value}"
        print(f"{game}: validated {len(rows)} samples")
        total += len(rows)
    print(f"Validated {total} samples: containers, memories, screenshots, mirrors, fields and integrity checks")


if __name__ == "__main__":
    main()

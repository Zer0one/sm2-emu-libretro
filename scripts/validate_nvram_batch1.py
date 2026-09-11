#!/usr/bin/env python3
"""Validate the first multi-game SM2 diagnostic-menu NVRAM campaign."""
from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path

HEADER = 64
NVRAM = 16 * 1024
EEPROM = 128

SKY = {
    **{f"difficulty-{name}": ("eeprom", 0x1B, value) for name, value in
       [("normal", 1), ("hard", 2), ("hardest", 3), ("easy", 0)]},
    "advertise-sound-on": ("eeprom", 0x1C, 1),
    "advertise-sound-off": ("eeprom", 0x1C, 0),
    **{f"country-{name}": ("eeprom", 0x19, value) for name, value in
       [("japan", 0), ("usa", 1), ("export", 2)]},
    "cabinet-standard": ("eeprom", 0x1A, 0),
    "cabinet-deluxe": ("eeprom", 0x1A, 1),
}
VCOP = {
    "advertise-sound-on": ("nv", 0x19, 0),
    "advertise-sound-off": ("nv", 0x19, 1),
    **{f"country-{name}": ("nv", 0x1B, value) for name, value in
       [("jpn", 1), ("exp", 2), ("usa", 0)]},
    **{f"cabinet-{name}": ("nv", 0x1A, value) for name, value in
       [("dx", 0), ("sp-u-r", 1), ("u-r", 2)]},
    **{f"difficulty-{name}": ("nv", 0x30, value) for name, value in
       [("normal", 4), ("medium-hard", 5), ("hard", 6), ("very-hard", 7),
        ("hardest", 8), ("easiest", 0), ("very-easy", 1), ("easy", 2),
        ("medium-easy", 3)]},
    **{f"life-{value}": ("nv", 0x31, value - 1) for value in range(1, 10)},
}
AIR = {
    "start-button-select-start": ("eeprom", 0x16, 0),
    "start-button-select-shot": ("eeprom", 0x16, 1),
    "player-selection-2p": ("eeprom", 0x1A, 0),
    "player-selection-4p": ("eeprom", 0x1A, 1),
    "attract-sound-off": ("eeprom", 0x17, 0),
    "attract-sound-on": ("eeprom", 0x17, 1),
    **{f"difficulty-{name}": ("eeprom", 0x18, value) for name, value in
       [("normal", 2), ("hard", 3), ("hardest", 4), ("easiest", 0), ("easy", 1)]},
    **{f"game-time-{name}": ("eeprom", 0x19, value) for name, value in
       [("2m00", 2), ("2m30", 3), ("3m00", 4), ("3m30", 5), ("4m00", 6),
        ("4m30", 7), ("5m00", 8), ("1m00", 0), ("1m30", 1)]},
    "replay-on": ("eeprom", 0x1B, 1),
    "replay-off": ("eeprom", 0x1B, 0),
    "win-freeplay-cpu-yes-vs-yes": ("eeprom", 0x1C, 0),
    "win-freeplay-cpu-yes-vs-no": ("eeprom", 0x1C, 1),
    "win-freeplay-cpu-no-vs-no": ("eeprom", 0x1C, 2),
    "character-type-normal": ("eeprom", 0x1D, 0),
    "character-type-deformation": ("eeprom", 0x1D, 1),
}
ZERO = {
    "credit-mode-same": ("eeprom", 0x10, 0),
    "credit-mode-individual": ("eeprom", 0x10, 1),
    "continue-mode-normal": ("eeprom", 0x11, 0),
    "continue-mode-free-play": ("eeprom", 0x11, 1),
    **{f"coin-slot-1-{name}": ("eeprom", 0x12, value) for value, name in enumerate(
       ["1c-1c", "2c-1c", "3c-1c", "1c-2c", "1c-3c", "1c-4c", "1c-5c", "1c-6c"])},
    **{f"coin-slot-2-{name}": ("eeprom", 0x13, value) for value, name in enumerate(
       ["1c-1c", "2c-1c", "3c-1c", "1c-2c", "1c-3c", "1c-4c", "1c-5c", "1c-6c"])},
    "demo-sound-on": ("eeprom", 0x14, 1),
    "demo-sound-off": ("eeprom", 0x14, 0),
    **{f"difficulty-{name}": ("eeprom", 0x15, value) for name, value in
       [("easy", 0), ("normal", 1), ("hard", 2), ("very-hard", 3)]},
    **{f"fighters-{value}": ("eeprom", 0x16, value) for value in range(1, 5)},
    "extend-points-600000": ("eeprom", 0x17, 0),
    "extend-points-800000": ("eeprom", 0x17, 1),
    "ranking-data-do-initialize": ("eeprom", 0x18, 1),
    "ranking-data-do-not-initialize": ("eeprom", 0x18, 0),
}

SPECS = {
    "skytargt": (SKY, (8, 0x30, 0x58), None),
    "vcop2": (VCOP, None, (0, 0x80, 0x100)),
    "airwlkrs": (AIR, (8, 0x20, 0x38), None),
    "zeroguna": (ZERO, (8, 0x1C, 0x30), None),
}

def crc16(data: bytes) -> int:
    crc = 0
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", nargs="?", type=Path,
                        default=Path.home() / "Documents/RetroArch/sm2-nvram-analysis")
    args = parser.parse_args()
    total = 0
    for game, (expected, eeprom_mirror, nv_mirror) in SPECS.items():
        root = args.root.expanduser().resolve() / game / "generated"
        rows = json.loads((root / "summary.json").read_text())
        names = {row["sample"].removeprefix(game + "-") for row in rows}
        wanted = {"base", *expected}
        assert names == wanted, f"{game}: sample set mismatch: {sorted(names ^ wanted)}"
        assert len(rows) == len(wanted)
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
            saved_game = srm[24:56].split(b"\0", 1)[0].decode("ascii")
            assert saved_game == game, stem
            assert int.from_bytes(srm[20:24], "little") == crc16(srm[HEADER:]), stem
            assert len(nv) == NVRAM and len(ep) == EEPROM, stem
            assert srm[HEADER:HEADER + NVRAM] == nv, stem
            assert srm[HEADER + NVRAM:] == ep, stem
            shots = list((root / "screenshots").glob(f"{stem}-*.png"))
            assert len(shots) == 1 and shots[0].read_bytes().startswith(b"\x89PNG\r\n\x1a\n"), stem
            width, height = struct.unpack(">II", shots[0].read_bytes()[16:24])
            assert (width, height) == (1024, 768), stem
            if eeprom_mirror:
                first, second, end = eeprom_mirror
                assert ep[first:second] == ep[second:end], f"{stem}: EEPROM mirror mismatch"
            if nv_mirror:
                first, second, end = nv_mirror
                assert nv[first:second] == nv[second:end], f"{stem}: backup-RAM mirror mismatch"
                stored = int.from_bytes(nv[8:10], "little")
                assert stored == crc16(nv[10:second]), f"{stem}: settings CRC mismatch"
            if name in expected:
                storage, offset, value = expected[name]
                image = nv if storage == "nv" else ep
                assert image[offset] == value, (
                    f"{stem}: {storage}[{offset:#x}]={image[offset]}, expected {value}"
                )
        print(f"{game}: validated {len(rows)} samples")
        total += len(rows)
    print(f"Validated {total} samples: containers, extracted memories, screenshots, mirrors, fields and known CRCs")

if __name__ == "__main__":
    main()

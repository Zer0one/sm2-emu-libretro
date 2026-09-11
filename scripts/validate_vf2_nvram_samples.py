#!/usr/bin/env python3
"""Validate the complete VF2 Game Assignment sample campaign."""
import argparse
import json
from pathlib import Path

HEADER = 64
NVRAM = 16 * 1024
EEPROM = 128

EXPECTED = {
    **{f"match-count-1p-{value}": (0x3340, 0xff, value) for value in range(2, 6)},
    **{f"match-count-vs-{value}": (0x3341, 0xff, value) for value in range(2, 6)},
    "difficulty-normal": (0x3342, 0xff, 1),
    "difficulty-hard": (0x3342, 0xff, 2),
    "difficulty-hardest": (0x3342, 0xff, 3),
    "difficulty-easy": (0x3342, 0xff, 0),
    "advertise-sound-on": (0x3351, 0x01, 0),
    "advertise-sound-off": (0x3351, 0x01, 0x01),
    "continue-on": (0x3351, 0x02, 0),
    "continue-off": (0x3351, 0x02, 0x02),
    "drink-ok": (0x3351, 0x08, 0),
    "drink-ng": (0x3351, 0x08, 0x08),
    "country-japan": (0x3350, 0xff, 0),
    "country-usa": (0x3350, 0xff, 1),
    "country-export": (0x3350, 0xff, 2),
    "display-type-projector": (0x3351, 0x04, 0),
    "display-type-crt": (0x3351, 0x04, 0x04),
    "vs-finish-off": (0x3351, 0x20, 0),
    "vs-finish-on": (0x3351, 0x20, 0x20),
    "ranking-mode-off": (0x3351, 0x10, 0),
    "ranking-mode-on": (0x3351, 0x10, 0x10),
    "version-normal": (0x3351, 0x40, 0),
    "version-2": (0x3351, 0x40, 0x40),
}

def crc16(data: bytes) -> int:
    crc = 0
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xffff if crc & 0x8000 else (crc << 1) & 0xffff
    return crc

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    root = args.output.expanduser().resolve()
    summary = json.loads((root / "summary.json").read_text())
    rows = {row["sample"].removeprefix("vf2-"): row for row in summary}
    expected_names = {"base", *EXPECTED}
    assert rows.keys() == expected_names, f"sample set mismatch: {sorted(rows.keys() ^ expected_names)}"

    for name, row in rows.items():
        assert row["status"] == "ok", f"{name}: {row['note']}"
        srm_path = root / "saves" / f"vf2-{name}.srm"
        nv_path = root / "saves" / f"vf2-{name}.nv"
        ep_path = root / "saves" / f"vf2-{name}.eeprom"
        srm, nvram, eeprom = srm_path.read_bytes(), nv_path.read_bytes(), ep_path.read_bytes()
        assert len(srm) == HEADER + NVRAM + EEPROM and srm[:8] == b"SM2SRAM\0", name
        assert int.from_bytes(srm[8:12], "little") == 1 and srm[24:27] == b"vf2", name
        assert int.from_bytes(srm[20:24], "little") == crc16(srm[HEADER:]), name
        assert len(nvram) == NVRAM and len(eeprom) == EEPROM, name
        assert srm[HEADER:HEADER + NVRAM] == nvram and srm[HEADER + NVRAM:] == eeprom, name
        assert nvram[0x3306:0x3308] == b"\x18\0", name
        assert nvram[0x3308:0x3318] == b"VIRTUA FIGHTER 2", name
        assert int.from_bytes(nvram[0x3302:0x3304], "little") == crc16(nvram[0x3340:0x335d]), name
        screenshots = list((root / "screenshots").glob(f"vf2-{name}-*.png"))
        assert len(screenshots) == 1 and screenshots[0].read_bytes().startswith(b"\x89PNG\r\n\x1a\n"), name
        if name in EXPECTED:
            offset, mask, value = EXPECTED[name]
            assert nvram[offset] & mask == value, (
                f"{name}: offset {offset:#x} is {nvram[offset] & mask:#x}, expected {value:#x}"
            )
    print(f"Validated {len(rows)} VF2 samples: containers, extracted memories, screenshots, fields and CRCs")

if __name__ == "__main__":
    main()

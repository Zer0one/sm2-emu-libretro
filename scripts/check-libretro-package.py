#!/usr/bin/env python3
"""Validate the contents and checksums of a packaged SM2 Libretro core."""

from __future__ import annotations

import hashlib
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET


if len(sys.argv) != 3:
    raise SystemExit("usage: check-libretro-package.py DIST_DIRECTORY CORE_BINARY")

root = Path(sys.argv[1]).resolve()
binary = sys.argv[2]

required = {
    binary,
    "sm2_libretro.info",
    "system/sm2-emu/games.xml",
    "LICENSE",
    "NOTICE",
    "README.md",
    "SOURCE_COMMIT.txt",
    "SHA256SUMS",
    "licenses/LZMA-SDK.txt",
    "licenses/Musashi.txt",
    "licenses/VulkanMemoryAllocator.txt",
    "licenses/libretro-api.txt",
    "licenses/miniz.txt",
    "licenses/pugixml.txt",
    "licenses/ymfm.txt",
}

if not root.is_dir():
    raise SystemExit(f"package directory not found: {root}")

files = {
    path.relative_to(root).as_posix()
    for path in root.rglob("*")
    if path.is_file()
}
missing = required - files
extra = files - required
assert not missing, f"missing package files: {sorted(missing)}"
assert not extra, f"unexpected package files: {sorted(extra)}"

for name in files:
    assert Path(name).suffix.lower() not in {".zip", ".7z", ".chd", ".iso", ".rom"}, (
        f"game-content-like file in package: {name}"
    )

commit = (root / "SOURCE_COMMIT.txt").read_text(encoding="ascii").strip()
assert re.fullmatch(r"[0-9a-f]{40}", commit), f"invalid source commit: {commit!r}"

info = (root / "sm2_libretro.info").read_text(encoding="utf-8")
assert 'firmware0_path = "sm2-emu/games.xml"' in info
assert 'license = "BSD-3-Clause"' in info
assert 'savestate = "true"' in info

ET.parse(root / "system/sm2-emu/games.xml")
assert (root / binary).stat().st_size > 0

listed: dict[str, str] = {}
for line in (root / "SHA256SUMS").read_text(encoding="ascii").splitlines():
    digest, separator, name = line.partition("  ")
    assert separator and re.fullmatch(r"[0-9a-f]{64}", digest), f"invalid checksum line: {line!r}"
    assert name not in listed, f"duplicate checksum: {name}"
    listed[name] = digest

hashed_files = files - {"SHA256SUMS"}
assert set(listed) == hashed_files, (
    f"checksum coverage differs: missing={sorted(hashed_files - set(listed))}, "
    f"extra={sorted(set(listed) - hashed_files)}"
)
for name, expected in listed.items():
    actual = hashlib.sha256((root / name).read_bytes()).hexdigest()
    assert actual == expected, f"checksum mismatch: {name}"

print(f"PASS: complete package, {len(files)} files, checksums and licences verified")

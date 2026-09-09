#!/usr/bin/env python3
"""Fail CI on missing/extra ABI exports, external runtimes or unsafe empty lifecycle."""
import ctypes as C
from pathlib import Path
import platform
import re
import subprocess
import sys

core = Path(sys.argv[1]).resolve()
root = Path(__file__).resolve().parents[1]
expected = set(re.findall(r"retro_\w+", (root / "src/libretro/exports.map").read_text()))

def command(*args, verbose=True):
    result = subprocess.check_output(args, text=True)
    if verbose:
        print(result)
    return result

kind = platform.system()
command("file", str(core))
if kind == "Darwin":
    symbols = command("nm", "-gU", str(core))
    exports = set(re.findall(r"\b_?(retro_\w+)$", symbols, re.M))
    deps = command("otool", "-L", str(core))
    assert all(line.strip().startswith(("/usr/lib/", "/System/Library/"))
               for line in deps.splitlines()[2:]), deps
elif kind == "Windows":
    pe = command("objdump", "-p", str(core), verbose=False)
    exports = set(re.findall(r"\b(retro_\w+)$", pe, re.M))
    print("DLL exports:", ", ".join(sorted(exports)))
    deps = re.findall(r"DLL Name: (\S+)", pe)
    print("DLL imports:", ", ".join(deps))
    assert not any(re.search(r"libgcc|libstdc|libwinpthread|vulkan|pugi|miniz|zlib", d, re.I) for d in deps), deps
else:
    symbols = command("nm", "-D", "--defined-only", str(core))
    exports = set(re.findall(r"\b(retro_\w+)$", symbols, re.M))
    deps = command("ldd", str(core))
    assert "not found" not in deps, deps
    assert not re.search(r"lib(vulkan|SDL|pugi|miniz|stdc\+\+|gcc_s)", deps), deps
assert exports == expected, (exports - expected, expected - exports)

class Info(C.Structure):
    _fields_ = [("name", C.c_char_p), ("version", C.c_char_p),
                ("extensions", C.c_char_p), ("fullpath", C.c_bool), ("block_extract", C.c_bool)]

lib = C.CDLL(str(core))
for symbol in expected:
    getattr(lib, symbol)
ENV = C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p)
callback = ENV(lambda command, data: False)
lib.retro_set_environment.argtypes = [ENV]
lib.retro_set_environment(callback)
lib.retro_load_game.argtypes = [C.c_void_p]
lib.retro_load_game.restype = C.c_bool
lib.retro_serialize_size.restype = C.c_size_t
lib.retro_get_system_info.argtypes = [C.POINTER(Info)]
assert lib.retro_api_version() == 1
for _ in range(3):
    lib.retro_init()
    info = Info()
    lib.retro_get_system_info(C.byref(info))
    assert info.name and info.version and info.extensions == b"zip|7z"
    assert info.fullpath and info.block_extract
    assert not lib.retro_load_game(None)
    assert lib.retro_serialize_size() == 0
    lib.retro_run()
    lib.retro_reset()
    lib.retro_unload_game()
    lib.retro_deinit()
print("PASS: 25 ABI exports, dependencies and three empty lifecycle cycles")

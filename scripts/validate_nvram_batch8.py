#!/usr/bin/env python3
"""Validate the final SM2 diagnostic-menu NVRAM campaign."""
from __future__ import annotations
import argparse, json, struct, tomllib
from pathlib import Path

HEADER=64; NVRAM=16*1024; EEPROM=128
COUNTS={"dynamcop":46,"pltkids":37,"rascot2":1,"topskatr":11,"zerogun":35}

def crc16(data: bytes)->int:
    c=0
    for value in data:
        c ^= value << 8
        for _ in range(8):
            c=((c<<1)^0x1021)&0xffff if c&0x8000 else (c<<1)&0xffff
    return c

def expected(game: str, name: str):
    if game=="dynamcop":
        if name.startswith("advertise-sound-"): return 0x1f, int(name.endswith("on"))
        if name.startswith("game-difficulty-"):
            shown=int(name.rsplit("-",1)[1]); return 0x1a, shown % 8
        if name.startswith("life-amount-"):
            shown=int(name.rsplit("-",1)[1]); return 0x24, (shown//8-8) % 33
        if name.startswith("violence-mode-"): return 0x1b, int(name.endswith("on"))
    if game=="topskatr":
        if name.startswith("advertise-sound-"): return 0x14, int(name.endswith("on"))
        if name.startswith("game-difficulty-"):
            shown=int(name.split("-")[2]); return 0x15, (shown-1) % 8
    if game=="zerogun":
        mapping={
            "credit-mode":(0x10,{"same":0,"individual":1}),
            "continue-mode":(0x11,{"normal":0,"free-play":1}),
            "demo-sound":(0x14,{"on":1,"off":0}),
            "difficulty":(0x15,{"easy":0,"normal":1,"hard":2,"very-hard":3}),
            "fighters":(0x16,{"1":1,"2":2,"3":3,"4":4}),
            "extend-points":(0x17,{"600000":0,"800000":1}),
            "ranking-data":(0x18,{"do-initialize":1,"do-not-initialize":0}),
        }
        for prefix,(off,vals) in mapping.items():
            if name.startswith(prefix+"-"): return off,vals[name[len(prefix)+1:]]
        for prefix,off in (("coin-slot-1",0x12),("coin-slot-2",0x13)):
            if name.startswith(prefix+"-"):
                slug=name[len(prefix)+1:]
                vals={"1c-1c":0,"2c-1c":1,"3c-1c":2,"1c-2c":3,"1c-3c":4,"1c-4c":5,"1c-5c":6,"1c-6c":7}
                return off,vals[slug]
    return None

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("root",nargs="?",type=Path,default=Path.home()/"Documents/RetroArch/sm2-nvram-analysis")
    ap.add_argument("--repo",type=Path,default=Path(__file__).resolve().parents[1])
    ap.add_argument("--game",action="append",choices=COUNTS)
    a=ap.parse_args(); total=0
    for game in a.game or list(COUNTS):
        root=a.root.expanduser().resolve()/game/"generated"
        rows=json.loads((root/"summary.json").read_text())
        cfg=tomllib.loads((a.repo/"scripts"/f"libretro_nvram_samples.{game}.toml").read_text())
        wanted={x["suffix"] for x in cfg["samples"]}
        names={x["sample"].removeprefix(game+"-") for x in rows}
        assert len(rows)==COUNTS[game] and names==wanted,f"{game}: sample mismatch"
        base_ep=None if game=="rascot2" else (root/"saves"/f"{game}-base.eeprom").read_bytes()
        for row in rows:
            name=row["sample"].removeprefix(game+"-"); stem=f"{game}-{name}"
            assert row["status"]=="ok",f"{stem}: {row['note']}"
            s=(root/"saves"/f"{stem}.srm").read_bytes(); n=(root/"saves"/f"{stem}.nv").read_bytes(); e=(root/"saves"/f"{stem}.eeprom").read_bytes()
            assert len(s)==HEADER+NVRAM+EEPROM and s[:8]==b"SM2SRAM\0",stem
            assert len(n)==NVRAM and len(e)==EEPROM,stem
            assert int.from_bytes(s[8:12],"little")==1 and int.from_bytes(s[12:16],"little")==NVRAM and int.from_bytes(s[16:20],"little")==EEPROM,stem
            assert s[24:56].split(b"\0",1)[0].decode()==game,stem
            assert int.from_bytes(s[20:24],"little")==crc16(s[HEADER:]),stem
            assert s[HEADER:HEADER+NVRAM]==n and s[HEADER+NVRAM:]==e,stem
            shots=[Path(x) for x in row["screenshots"]]
            assert len(shots)==1 and struct.unpack(">II",shots[0].read_bytes()[16:24])==(1024,768),stem
            assert shots[0].stat().st_size>=3500,f"{stem}: menu screenshot too small"
            if game=="dynamcop": assert e[0x08:0x30]==e[0x30:0x58],f"{stem}: EEPROM mirror"
            elif game=="zerogun": assert e[0x08:0x1c]==e[0x1c:0x30],f"{stem}: EEPROM mirror"
            elif game=="pltkids": assert e==base_ep,f"{stem}: current core unexpectedly persisted a menu value"
            exp=expected(game,name)
            if exp:
                off,val=exp; assert e[off]==val,f"{stem}: eeprom[{off:#x}]={e[off]:#x}, expected {val:#x}"
                if game=="dynamcop": assert e[off+0x28]==val,f"{stem}: mirror field"
                elif game=="zerogun": assert e[off+0x14]==val,f"{stem}: mirror field"
        print(f"{game}: validated {len(rows)} samples")
        total+=len(rows)
    print(f"Validated {total} samples: containers, memories, screenshots, fields and mirrors")

if __name__=="__main__": main()

#!/usr/bin/env python3
"""Validate the seventh multi-game SM2 diagnostic-menu NVRAM campaign."""
from __future__ import annotations
import argparse, json, struct, tomllib
from pathlib import Path
HEADER=64; NVRAM=16*1024; EEPROM=128
COUNTS={"dynabb":28,"dynabb97":26,"hpyagu98":22,"schamp":60,"vstriker":37}

def crc16(data: bytes)->int:
 c=0
 for value in data:
  c^=value<<8
  for _ in range(8): c=((c<<1)^0x1021)&0xffff if c&0x8000 else (c<<1)&0xffff
 return c

def put(d,name,storage,*items): d[name]=(storage,items)
EXPECTED={g:{} for g in COUNTS}
for g in ("dynabb","dynabb97"):
 x=EXPECTED[g]
 for n,v in (("normal",1),("hard",2),("hardest",3),("easy",0)): put(x,f"game-difficulty-{n}","eeprom",(0x18,v),(0x34,v))
 for n,v in (("on",1),("off",0)): put(x,f"advertise-sound-{n}","eeprom",(0x19,v),(0x35,v))
 for n,v in (("us",2),("city",0),("megalo",1)): put(x,f"cabinet-type-{n}","eeprom",(0x1b,v),(0x37,v))
 for n,v in zip(("off","swallows","carp","giants","baystars","dragons","tigers","bluewave","marines","lions","fighters","hawks","buffaloes"),(12,*range(12))): put(x,f"favorite-{n}","eeprom",(0x1a,v),(0x36,v))
 innings=(("1c2",1),("1c3",2),("1c1",0))
 if g=="dynabb": innings=(("1c2",1),("1c3",2),("1c1-2c3-3c5-4c9",3),("1c2-2c5-3c9",4),("1c1",0))
 for n,v in innings: put(x,f"innings-{n}","eeprom",(0x1d,v),(0x39,v))
S=EXPECTED["schamp"]
for p in ("1p","vs"):
 for n in range(2,6): put(S,f"match-count-{p}-{n}","nv",(0x3340+(p=="vs"),n))
for pref,off,derived in (("enemy-rank",0x3342,None),("energy-1p",0x3343,(0x3354,[160,144,128,176])),("energy-vs",0x3344,(0x3354,[160,180,160,220]))):
 for i,(n,v) in enumerate((("normal",1),("hard",2),("hardest",3),("easy",0))):
  items=[(off,v)]
  if derived: items.append((derived[0],derived[1][i]))
  put(S,f"{pref}-{n}","nv",*items)
for n,v in zip((30,40,50,60,70,80,90,99,10,20),(2,3,4,5,6,7,8,9,0,1)): put(S,f"time-{n}","nv",(0x3351,v))
for n in (5,6,7,8,9,10,1,2,3,4): put(S,f"barrier-{n}","nv",(0x3358,n),(0x3353,0x10))
for pref,pairs in {
 "barrier-reset":(("off",0x10),("on",0x18)),"automatic":(("on",0x10),("off",0x00)),
 "hyper-mode":(("on",0x10),("off",0x50)),"damage":(("normal",0x10),("real",0x90)),
 "advertise-sound":(("on",0x10),("off",0x11)),"continue":(("on",0x10),("off",0x12)),
 "display-type":(("crt",0x10),("projector",0x14)),"vs-finish":(("off",0x10),("on",0x30)),}.items():
 for n,v in pairs: put(S,f"{pref}-{n}","nv",(0x3353,v))
for n,v in (("usa",1),("export",2),("japan",0)): put(S,f"country-{n}","nv",(0x3352,v))
V=EXPECTED["vstriker"]
def vm(name,off,val): put(V,name,"nv",(off,val),(off+0x80,val))
for pref,off,pairs in (
 ("advertise-sound",0x19,(("on",0),("off",1))),
 ("country",0x1b,(("jpn",1),("exp",2),("usa",0))),
 ("monitor",0x1a,(("crt",1),("projecter",0))),
 ("difficulty",0x20,(("normal",1),("hard",2),("hardest",3),("easy",0))),
 ("time-set",0x21,(("2m00",3),("2m15",4),("2m30",5),("2m45",6),("3m00",7),("1m15",0),("1m30",1),("1m45",2))),
 ("v-goal-system",0x1c,(("off",1),("on",0))),
 ("v-goal-time",0x22,(("0m15",0),("0m30",1),("0m45",2),("1m00",3),("free",4))),
 ("pk-system",0x1d,(("off",1),("on",0))),
 ("pk-member",0x1e,(("3",0),("4",1),("5",2),("real",3))),
 ("billboard",0x1f,(("on",0),("off",1))),
 ("one-match-mode",0x0a,(("off",1),("on",0))),):
 for n,v in pairs: vm(f"{pref}-{n}",off,v)

def main():
 ap=argparse.ArgumentParser(description=__doc__); ap.add_argument("root",nargs="?",type=Path,default=Path.home()/"Documents/RetroArch/sm2-nvram-analysis"); ap.add_argument("--repo",type=Path,default=Path(__file__).resolve().parents[1]); ap.add_argument("--game",action="append",choices=COUNTS); a=ap.parse_args(); total=0
 for g in a.game or list(COUNTS):
  root=a.root.expanduser().resolve()/g/"generated"; rows=json.loads((root/"summary.json").read_text()); cfg=tomllib.loads((a.repo/"scripts"/f"libretro_nvram_samples.{g}.toml").read_text()); wanted={x["suffix"] for x in cfg["samples"]}; names={x["sample"].removeprefix(g+"-") for x in rows}
  assert len(rows)==COUNTS[g] and names==wanted, f"{g}: sample mismatch"
  base_ep=(root/"saves"/f"{g}-base.eeprom").read_bytes()
  for row in rows:
   name=row["sample"].removeprefix(g+"-"); stem=f"{g}-{name}"; assert row["status"]=="ok",f"{stem}: {row['note']}"
   s=(root/"saves"/f"{stem}.srm").read_bytes(); n=(root/"saves"/f"{stem}.nv").read_bytes(); e=(root/"saves"/f"{stem}.eeprom").read_bytes(); assert len(s)==HEADER+NVRAM+EEPROM and s[:8]==b"SM2SRAM\0",stem; assert len(n)==NVRAM and len(e)==EEPROM,stem; assert int.from_bytes(s[8:12],"little")==1 and int.from_bytes(s[12:16],"little")==NVRAM and int.from_bytes(s[16:20],"little")==EEPROM,stem; assert s[24:56].split(b"\0",1)[0].decode()==g,stem; assert int.from_bytes(s[20:24],"little")==crc16(s[HEADER:]),stem; assert s[HEADER:HEADER+NVRAM]==n and s[HEADER+NVRAM:]==e,stem
   shots=[Path(x) for x in row["screenshots"]]; assert len(shots)==1 and struct.unpack(">II",shots[0].read_bytes()[16:24])==(1024,768),stem; assert shots[0].stat().st_size>=6000,f"{stem}: menu screenshot too small"
   if g in {"dynabb","dynabb97"}: assert e[0x08:0x24]==e[0x24:0x40],f"{stem}: EEPROM mirror"
   elif g=="hpyagu98": assert e==base_ep,f"{stem}: unexpected EEPROM variation"
   elif g=="schamp": assert int.from_bytes(n[0x3302:0x3304],"little")==crc16(n[0x3340:0x3360]),f"{stem}: settings CRC"
   elif g=="vstriker": assert n[:0x80]==n[0x80:0x100],f"{stem}: backup mirror"; assert int.from_bytes(n[8:10],"little")==crc16(n[10:0x80]),f"{stem}: settings CRC"
   if name in EXPECTED[g]:
    storage,items=EXPECTED[g][name]; data=n if storage=="nv" else e
    for off,val in items: assert data[off]==val,f"{stem}: {storage}[{off:#x}]={data[off]:#x}, expected {val:#x}"
  print(f"{g}: validated {len(rows)} samples"); total+=len(rows)
 print(f"Validated {total} samples: containers, memories, screenshots, fields, mirrors and known CRCs")
if __name__=="__main__": main()

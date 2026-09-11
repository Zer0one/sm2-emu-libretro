#!/usr/bin/env python3
"""Validate the third multi-game SM2 diagnostic-menu NVRAM campaign."""
from __future__ import annotations
import argparse,json,struct,tomllib
from pathlib import Path
HEADER=64; NVRAM=16*1024; EEPROM=128

def crc16(data:bytes)->int:
 c=0
 for x in data:
  c^=x<<8
  for _ in range(8): c=((c<<1)^0x1021)&0xffff if c&0x8000 else (c<<1)&0xffff
 return c
VCOP={
 'advertise-sound-on':('nv',0x19,0,0xff),'advertise-sound-off':('nv',0x19,1,0xff),
 **{f'country-{n}':('nv',0x1b,v,0xff) for n,v in [('jpn',1),('exp',2),('usa',0)]},
 **{f'cabinet-{n}':('nv',0x1a,v,0xff) for n,v in [('dx',0),('sp-u-r',1),('u-r',2)]},
 **{f'difficulty-{n}':('nv',0x20,v,0xff) for n,v in zip(['normal','medium-hard','hard','very-hard','hardest','easiest','very-easy','easy','medium-easy'],[4,5,6,7,8,0,1,2,3])},
 **{f'life-{n}':('nv',0x21,n-1,0xff) for n in range(1,10)}}
FVIPERS={
 **{f'match-count-1p-{n}':('nv',0x3340,n,0xff) for n in range(2,6)},
 **{f'match-count-vs-{n}':('nv',0x3341,n,0xff) for n in range(2,6)},
 **{f'difficulty-{n}':('nv',0x3342,v,0xff) for n,v in [('normal',1),('hard',2),('hardest',3),('easy',0)]},
 'advertise-sound-on':('nv',0x3351,0,0x01),'advertise-sound-off':('nv',0x3351,1,0x01),
 'continue-on':('nv',0x3351,0,0x02),'continue-off':('nv',0x3351,2,0x02),
 **{f'country-{n}':('nv',0x3350,v,0xff) for n,v in [('japan',0),('usa',1),('export',2)]},
 'display-type-projector':('nv',0x3351,0,0x04),'display-type-crt':('nv',0x3351,4,0x04),
 'vs-finish-off':('nv',0x3351,0,0x20),'vs-finish-on':('nv',0x3351,0x20,0x20),
 'ranking-mode-off':('nv',0x3351,0,0x10),'ranking-mode-on':('nv',0x3351,0x10,0x10)}
SRALLY={
 'advertise-sound-on':('ep',0x08,1,0xff),'advertise-sound-off':('ep',0x08,0,0xff),
 **{f'country-{n}':('ep',0x09,v,0xff) for n,v in [('jpn',0),('usa',1),('export',2)]},
 'cabinet-type-twin':('ep',0x0a,1,0xff),'cabinet-type-deluxe':('ep',0x0a,0,0xff),
 **{f'link-type-{n}':('ep',0x0b,v,0xff) for n,v in [('notlink',0),('car-1',1),('car-2',2),('car-3',3),('car-4',4),('relay',5)]},
 **{f'game-difficulty-{n}':('ep',0x0c,v,0xff) for n,v in [('normal',0),('easy',1),('hard',2),('hardest',3)]},
 **{f'game-mode-{n}':('ep',0x0d,v,0xff) for n,v in [('normal',0),('short',1),('long',2),('longest',3)]}}
HOTD={
 **{f'game-difficulty-{n}':('ep',0x1a,v,0xff) for n,v in [('normal',2),('medium-hard',3),('very-hard',4),('very-easy',0),('medium-easy',1)]},
 **{f'life-setting-initial-{a}-max-{b}':('ep',0x1c,v,0xff) for v,(a,b) in enumerate([(1,3),(2,3),(3,3),(1,4),(2,4),(3,4),(4,4),(1,5),(2,5),(3,5),(4,5),(5,5)])},
 **{f'blood-color-{n}':('ep',0x1d,v,0xff) for n,v in [('green',1),('blue',2),('purple',3),('red',0)]},
 'advertise-sound-on':('ep',0x1b,1,0xff),'advertise-sound-off':('ep',0x1b,0,0xff),
 **{f'country-{n}':('ep',0x19,v,0xff) for n,v in [('japan',0),('usa',1),('export',2)]}}
EXPECTED={'vcop':VCOP,'fvipers':FVIPERS,'srallyc':SRALLY,'hotd':HOTD}

def main():
 ap=argparse.ArgumentParser(description=__doc__); ap.add_argument('root',nargs='?',type=Path,default=Path.home()/'Documents/RetroArch/sm2-nvram-analysis'); ap.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[1]); a=ap.parse_args(); total=0
 for game,expected in EXPECTED.items():
  root=a.root.expanduser().resolve()/game/'generated'; rows=json.loads((root/'summary.json').read_text()); cfg=tomllib.loads((a.repo/'scripts'/f'libretro_nvram_samples.{game}.toml').read_text()); wanted={x['suffix'] for x in cfg['samples']}; names={x['sample'].removeprefix(game+'-') for x in rows}
  assert names==wanted=={'base',*expected},f'{game}: sample set mismatch'
  for row in rows:
   name=row['sample'].removeprefix(game+'-'); stem=f'{game}-{name}'; assert row['status']=='ok',f'{stem}: {row["note"]}'
   s=(root/'saves'/f'{stem}.srm').read_bytes(); nv=(root/'saves'/f'{stem}.nv').read_bytes(); ep=(root/'saves'/f'{stem}.eeprom').read_bytes()
   assert len(s)==HEADER+NVRAM+EEPROM and s[:8]==b'SM2SRAM\0' and len(nv)==NVRAM and len(ep)==EEPROM,stem
   assert int.from_bytes(s[8:12],'little')==1 and int.from_bytes(s[12:16],'little')==NVRAM and int.from_bytes(s[16:20],'little')==EEPROM,stem
   assert s[24:56].split(b'\0',1)[0].decode()==game and int.from_bytes(s[20:24],'little')==crc16(s[HEADER:]),stem
   assert s[HEADER:HEADER+NVRAM]==nv and s[HEADER+NVRAM:]==ep,stem
   shots=list((root/'screenshots').glob(f'{stem}-*.png')); assert len(shots)==1 and struct.unpack('>II',shots[0].read_bytes()[16:24])==(1024,768),stem
   if game=='vcop':
    assert nv[:0x80]==nv[0x80:0x100],stem
   elif game=='fvipers': assert int.from_bytes(nv[0x3302:0x3304],'little')==crc16(nv[0x3340:0x335d]),stem
   elif game=='srallyc': assert int.from_bytes(ep[2:4],'little')==0x24,stem
   elif game=='hotd': assert ep[0x08:0x30]==ep[0x30:0x58],stem
   if name in expected:
    store,off,val,mask=expected[name]; img=nv if store=='nv' else ep; assert img[off]&mask==val,f'{stem}: {store}[{off:#x}]'
    if game=='hotd': assert img[off+0x28]&mask==val,f'{stem}: mirror field'
  print(f'{game}: validated {len(rows)} samples'); total+=len(rows)
 print(f'Validated {total} samples: containers, memories, screenshots, fields, mirrors and known integrity checks')
if __name__=='__main__': main()

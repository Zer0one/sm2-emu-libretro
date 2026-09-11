#!/usr/bin/env python3
"""Validate the fourth multi-game SM2 diagnostic-menu NVRAM campaign."""
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

def put(d,n,off,val): d[n]=(off,val)
G={}
put(G,'advertise-sound-on',0x14,1);put(G,'advertise-sound-off',0x14,0)
for n,v in [('japan',0),('usa',1),('export',2)]:put(G,f'country-{n}',0x15,v)
for n in range(1,9):put(G,f'game-difficulty-{n}-of-8',0x19,n-1);put(G,f'shifting-difficulty-{n}-of-8',0x17,n-1)
for n,v in zip(range(2,9),[2,3,4,5,6,7,1]):put(G,f'player-life-{n}',0x1a,v)
put(G,'gun-reaction-on',0x1d,1);put(G,'gun-reaction-off',0x1d,0)
put(G,'cabinet-type-dx',0x1e,0);put(G,'cabinet-type-sd',0x1e,1)
L={}
for n,v in [('1-normal',1),('2-hard',2),('3-hardest',3),('0-easy',0)]:put(L,f'game-difficulty-{n}',0x1d,v)
for key,off,on in [('advertise-sound',0x1b,1),('vs-finish',0x1e,1),('survival-mode',0x1f,1),('cut-cross-street',0x22,1)]:
 put(L,f'{key}-on',off,on);put(L,f'{key}-off',off,1-on)
for key,off in [('match-point-cpu',0x20),('match-point-vs',0x21)]:
 for n in range(2,6):put(L,f'{key}-{n}',off,n)
put(L,'display-type-crt',0x23,0);put(L,'display-type-projector',0x23,1)
for n,v in [(1,0),(2,1),(3,3),(4,4),(5,5),(6,6),(7,7)]:put(L,f'master-volume-{n}',0x24,v)
I={}
for n,v in [('normal',2),('hard',3),('hardest',4),('very-easy',0),('easy',1)]:put(I,f'game-difficulty-{n}',0x19,v)
for key,off,a,b in [('race-mode',0x18,'normal','long'),('handicap',0x1a,'off','on'),('advertise-sound',0x1b,'off','on'),('cabinet-type',0x17,'twin','deluxe')]:put(I,f'{key}-{a}',off,0);put(I,f'{key}-{b}',off,1)
for n,v in [('usa',1),('export',2),('japan',0)]:put(I,f'country-{n}',0x16,v)
for n,v in [('stand-alone',0),('master',1),('slave',2)]:put(I,f'network-type-{n}',0x1e,v)
for n in range(1,9):put(I,f'cabinet-id-{n}',0x1f,n-1)
for key,off,maxn in [('engine-volume',0x1c,3),('default-view',0x1d,4)]:
 for n in range(1,maxn+1):put(I,f'{key}-{n}',off,n-1)
V={}
times=[*range(30,100,5),'death-match']; codes=[*range(5,19),255]
for key,off in [('play-time-1p-stage-1-5',0x10),('play-time-1p-penalty',0x11),('play-time-1p-stage-6-8',0x12),('play-time-versus',0x14)]:
 for n,v in zip(times,codes):put(V,f'{key}-{n}-secs' if isinstance(n,int) else f'{key}-{n}',off,v)
for n,v in [(90,17),(95,18)]:put(V,f'play-time-1p-last-stage-{n}-secs',0x13,v)
put(V,'play-time-1p-last-stage-death-match',0x13,255)
for key,off in [('match-count-1p-stage-1-5',0x15),('match-count-1p-penalty',0x16),('match-count-1p-stage-6-8',0x17),('match-count-versus',0x19)]:
 for n in range(1,6):put(V,f'{key}-{n}',off,n-1)
for n,v in [('no-link',2),('slave',0),('master',1)]:put(V,f'network-link-attribute-{n}',0x22,v)
for key,off,on in [('winning-by-decision',0x1a,1),('ranking-mode',0x20,1)]:put(V,f'{key}-on',off,on);put(V,f'{key}-off',off,1-on)
put(V,'continue-on',0x1d,0);put(V,'continue-off',0x1d,1);put(V,'versus-always-finish-off',0x1e,0);put(V,'versus-always-finish-on',0x1e,1)
for n,v in [('normal',0),('hard',1),('very-hard',2),('easy',3)]:put(V,f'game-difficulty-{n}',0x1b,v)
for n,v in [('loud',2),('off',0),('soft',1)]:put(V,f'advertise-sound-{n}',0x1c,v)
for n,v in [('replay-and-posing',3),('none',0),('replay-only',1),('posing-only',2)]:put(V,f'replay-and-posing-mode-{n}',0x1f,v)
put(V,'display-brightness-0',0x21,0)
for n in range(1,6):put(V,f'display-brightness-plus-{n}',0x21,n)
for n,v in zip(range(5,0,-1),range(6,11)):put(V,f'display-brightness-minus-{n}',0x21,v)
EXPECTED={'gunblade':G,'lastbrnx':L,'indy500':I,'von':V}
COUNTS={'gunblade':33,'lastbrnx':30,'indy500':35,'von':117}

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('root',nargs='?',type=Path,default=Path.home()/'Documents/RetroArch/sm2-nvram-analysis');ap.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[1]);a=ap.parse_args();total=0
 for game,expected in EXPECTED.items():
  root=a.root.expanduser().resolve()/game/'generated';rows=json.loads((root/'summary.json').read_text());cfg=tomllib.loads((a.repo/'scripts'/f'libretro_nvram_samples.{game}.toml').read_text());wanted={x['suffix'] for x in cfg['samples']};names={x['sample'].removeprefix(game+'-') for x in rows}
  assert len(rows)==COUNTS[game] and names==wanted=={'base',*expected},f'{game}: sample set mismatch ({len(rows)})'
  for row in rows:
   name=row['sample'].removeprefix(game+'-');stem=f'{game}-{name}';assert row['status']=='ok',f'{stem}: {row["note"]}'
   s=(root/'saves'/f'{stem}.srm').read_bytes();nv=(root/'saves'/f'{stem}.nv').read_bytes();ep=(root/'saves'/f'{stem}.eeprom').read_bytes()
   assert len(s)==HEADER+NVRAM+EEPROM and s[:8]==b'SM2SRAM\0' and len(nv)==NVRAM and len(ep)==EEPROM,stem
   assert int.from_bytes(s[8:12],'little')==1 and int.from_bytes(s[12:16],'little')==NVRAM and int.from_bytes(s[16:20],'little')==EEPROM,stem
   assert s[24:56].split(b'\0',1)[0].decode()==game and int.from_bytes(s[20:24],'little')==crc16(s[HEADER:]),stem
   assert s[HEADER:HEADER+NVRAM]==nv and s[HEADER+NVRAM:]==ep,stem
   shots=list((root/'screenshots').glob(f'{stem}-*.png'));assert len(shots)==1 and struct.unpack('>II',shots[0].read_bytes()[16:24])==(1024,768),stem
   if game=='gunblade':assert nv[:0x3e0]==nv[0xc20:0x1000],f'{stem}: backup RAM mirror'
   elif game=='lastbrnx':assert ep[0x08:0x44]==ep[0x44:0x80],f'{stem}: EEPROM mirror'
   elif game=='indy500':assert ep[0x08:0x2c]==ep[0x2c:0x50],f'{stem}: EEPROM mirror'
   else:
    assert ep[:0x3c]==ep[0x3c:0x78],f'{stem}: EEPROM mirror';assert nv[0x14:0x34]==nv[0x220:0x240],f'{stem}: backup RAM mirror'
   if name in expected:
    off,val=expected[name];assert ep[off]==val,f'{stem}: EEPROM[{off:#x}]={ep[off]:#x}, expected {val:#x}'
    if game=='lastbrnx':assert ep[off+0x3c]==val,f'{stem}: EEPROM mirror field'
    elif game=='indy500':assert ep[off+0x24]==val,f'{stem}: EEPROM mirror field'
    elif game=='von':assert ep[off+0x3c]==val and nv[off+6]==val and nv[off+0x212]==val,f'{stem}: cross-storage field'
  print(f'{game}: validated {len(rows)} samples');total+=len(rows)
 print(f'Validated {total} samples: containers, memories, screenshots, fields, mirrors and cross-storage copies')
if __name__=='__main__':main()

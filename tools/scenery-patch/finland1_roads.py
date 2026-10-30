#!/usr/bin/env python3
"""Exact-source Finland1 experimental visual-only geometry. Originals read-only.
Hi/lo selection is made by original C3D identity, never a generic temp filename.
Original contact/FIN/AI route are never replaced. Native appearance/performance unverified.
"""
from pathlib import Path
import json,hashlib,tempfile,struct
from road_prepare import build
from road_pair import validate_pair
import importlib.util
HERE=Path(__file__).parent
spec=importlib.util.spec_from_file_location('export_evidence',HERE/'export-evidence.py')
ee=importlib.util.module_from_spec(spec);spec.loader.exec_module(ee)
INDEX=json.loads((HERE/'finland1-roads.json').read_text())

def prepare(data,cache):
 data=Path(data).resolve(strict=True);cache=Path(cache)
 if cache.is_symlink()or any(p.is_symlink()for p in cache.parents):raise ValueError('Symlink road cache refused')
 cache=cache.resolve()
 if cache==data or data in cache.parents:raise ValueError('Road cache must be outside originals')
 payloads={}
 for variant,row in INDEX.items():
  matches=[p for p in data.rglob('*')if p.name.lower()=='fin01'+variant+'.bfl']
  if not matches:continue
  if len(matches)!=1 or any(p.is_symlink()for p in [matches[0],*matches[0].parents]):raise ValueError('Need one regular Finland1 '+variant+' archive')
  members={n.lower():b for n,b in ee.members(matches[0].read_bytes())if n.lower()in row['sources']}
  if {n:hashlib.sha256(b).hexdigest()for n,b in members.items()}!=row['sources']:raise ValueError('Unsupported Finland1 source quartet')
  h,r=build(members,'fin01');validate_pair(h,r,2043)
  outputs={'curved.hpc':h,'curved.c3d':r}
  if {n:hashlib.sha256(b).hexdigest()for n,b in outputs.items()}!=row['outputs']:raise ValueError('Finland1 recipe differs from reviewed pair')
  # Validate paired research internally, but publish ONLY render. Physics stays original.
  payloads[variant+'-curved.c3d']=r
 if not payloads:raise ValueError('No supported owned Finland1 hi/lo archive')
 payloads['road-pair.tsv']=b'OPENCMR2-FINLAND1-ROAD-PAIR-1\n'
 key=hashlib.sha256()
 for n in ['finland1_roads.py','finland1-roads.json','road_prepare.py','road_pair.py','export-evidence.py']:key.update((HERE/n).read_bytes())
 for n,b in sorted(payloads.items()):key.update(n.encode()+hashlib.sha256(b).digest())
 pack=cache/('finland1-roads-'+key.hexdigest()[:24]);cache.mkdir(parents=True,exist_ok=True)
 if pack.exists():
  if pack.is_symlink()or {p.name for p in pack.iterdir()}!=set(payloads):raise ValueError('Changed/incomplete Finland1 cache')
  for n,b in payloads.items():
   p=pack/n
   if p.is_symlink()or p.read_bytes()!=b:raise ValueError('Changed Finland1 output')
 else:
  with tempfile.TemporaryDirectory(prefix='.finland1-',dir=cache)as tmp:
   stage=Path(tmp)/'pack';stage.mkdir()
   for n,b in payloads.items():(stage/n).write_bytes(b)
   if pack.exists():raise ValueError('Concurrent road preparation; retry')
   stage.rename(pack)
 print('Experimental Finland1 visual-only geometry; original contact/FIN/AI/physics untouched; native appearance/performance unverified')
 print('OPENCMR2_ROADS='+str(pack));return pack
if __name__=='__main__':
 import argparse
 p=argparse.ArgumentParser();p.add_argument('--input-dir',type=Path,required=True);p.add_argument('--cache-dir',type=Path,required=True);a=p.parse_args()
 try:prepare(a.input_dir,a.cache_dir)
 except(ValueError,OSError,AssertionError,struct.error)as e:raise SystemExit('Finland1 road preparation refused: '+str(e))

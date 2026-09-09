#!/usr/bin/env python3
"""Verify every action-hook instruction and relative-call target in the user IDBs."""
from pathlib import Path
import argparse,re,json
p=argparse.ArgumentParser();p.add_argument('--idb',required=True);p.add_argument('--output',required=True);a=p.parse_args()
r=Path(__file__).resolve().parents[1];src=(r/'src/ModernControlsHook.cpp').read_text();report=[]
for game,folder in [('GTA3','III'),('GTAVC','VC'),('GTASA','SA')]:
 start=src.index(('#if' if game=='GTA3' else '#elif')+' defined('+game+')');end=src.index('\n};',start)
 targets={int(site,16):(act,int(dst,16)) for act,site,dst in re.findall(r'Action::(\w+),\s*(0x[0-9A-Fa-f]+),\s*(0x[0-9A-Fa-f]+)',src[start:end])}
 found={};function=''
 for path in sorted((Path(a.idb)/folder).rglob('*.part*')):
  with path.open(errors='replace') as f:
   for line in f:
    m=re.match(r'\.text:([0-9A-F]{8})\s+(\S+)\s+proc near',line)
    if m:function=m[2]
    if not line.startswith('.text:'):continue
    try:addr=int(line[6:14],16)
    except ValueError:continue
    if addr not in targets:continue
    m=re.match(r'\.text:[0-9A-F]{8}\s+E8 ((?:[0-9A-F]{2} ){3}[0-9A-F]{2})\s+call\s+(.*)',line)
    if not m:continue
    actual=addr+5+int.from_bytes(bytes.fromhex(m[1]),'little',signed=True)
    assert actual==targets[addr][1],(game,hex(addr),hex(actual),targets[addr])
    found[addr]=dict(game=game,action=targets[addr][0],site=hex(addr),target=hex(actual),function=function,instruction=line.strip())
 assert found.keys()==targets.keys(),(game,'missing',[hex(x) for x in targets.keys()-found.keys()])
 report+=list(found.values());print(game,len(found),'verified CALL instructions and targets')
Path(a.output).write_text(json.dumps(report,indent=2)+'\n')

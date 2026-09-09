#!/usr/bin/env python3
"""Build-manifest check. Behavioral assertions live in host_regression.cpp.
Replaces v24's source-text assertions enforcing the faulty blank-field policy.
"""
from pathlib import Path
import xml.etree.ElementTree as ET
r=Path(__file__).resolve().parents[1]
ns={'m':'http://schemas.microsoft.com/developer/msbuild/2003'}
projects=list((r/'projects').glob('GInputNext_*.vcxproj'))
assert len(projects)==3
for p in projects:
 t=ET.parse(p)
 for kind,needed in [('ClCompile',['ModernControlsHook.cpp','InputArbitration.cpp']),('ClInclude',['ModernControlsHook.h','InputArbitration.h','InputContext.h','ControlActions.h','UnifiedState.h'])]:
  names={n.attrib.get('Include','').replace('\\','/').split('/')[-1] for n in t.findall('.//m:'+kind,ns)}
  assert set(needed)<=names,(p,kind,names)
 print(p.name+': Modern and shared context sources included')
print('Modern manifest smoke: PASS; run run_host_regressions.py for behavior')

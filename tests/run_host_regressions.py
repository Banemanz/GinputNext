#!/usr/bin/env python3
"""Execute real production functions in a simulated Linux game/Win32 environment.
These fixtures test logic, NOT Windows ABI, game tasks, or physical hardware.
Use cross_compile.py to check the real SDK's x86 types separately.
"""
from pathlib import Path
import tempfile,subprocess
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='gin-test-') as tmp:
 for g in ('GTA3','GTAVC','GTASA'):
  dest=Path(tmp)/g
  cmd=['g++','-std=c++17','-O0','-g','-D'+g,'-D__fastcall=','-D__cdecl=','-I'+str(r/'tests/host'),'-I'+str(r/'src'),str(r/'tests/host_regression.cpp')]
  cmd += [str(r/'src'/f) for f in ('Config.cpp','ModernControlsHook.cpp','GTAAdapter.cpp','InputArbitration.cpp')]
  subprocess.run(cmd+['-o',str(dest)],check=True);print(g,flush=True);subprocess.run([str(dest)],cwd=tmp,check=True)

#!/usr/bin/env python3
"""Compile every production TU for Windows x86 using the unmodified pinned SDK.
Requires llvm-mingw/libc++, the SDL2 VC package, and a local SDK checkout.
Produces COFF objects; does NOT link or claim an MSVC build or game execution.
"""
from pathlib import Path
import argparse,subprocess,concurrent.futures,json,hashlib
p=argparse.ArgumentParser();p.add_argument('--compiler',required=True);p.add_argument('--sdk',required=True);p.add_argument('--sdl',required=True);p.add_argument('--output',required=True);a=p.parse_args()
r=Path(__file__).resolve().parents[1];sdk=Path(a.sdk).resolve();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
revision=subprocess.check_output(['git','-C',str(sdk),'rev-parse','HEAD'],text=True).strip()
assert revision=='624a6a49265fd7a6fc63bda1611013ceabeacb8a',revision
compat=r/'tests/cross_compat'
def build(job):
 game,source=job;variant={'GTA3':'III','GTAVC':'vc','GTASA':'sa'}[game]
 cmd=[str(Path(a.compiler).absolute()),'-std=c++23','-fms-extensions','-D'+game,'-DPLUGIN_SGV_'+('10US' if game=='GTASA' else '10EN'),'-DRW','-DNOMINMAX','-D_CRT_SECURE_NO_WARNINGS']
 includes=[r/'src',r/'include',Path(a.sdl).resolve()/'include',compat]
 includes += [sdk/x for x in [f'plugin_{variant}',f'plugin_{variant}/game_{variant}',f'plugin_{variant}/game_{variant}/rw','shared','shared/game','shared/dxsdk','safetyhook']]
 for i in includes:cmd+=['-I',str(i)]
 cmd+=['-include',str(compat/'sdk-libcxx.h'),'-c',str(source),'-o',str(out/(game+'-'+source.stem+'.o'))]
 run=subprocess.run(cmd,capture_output=True,text=True);log=out/(game+'-'+source.stem+'.log');log.write_text(run.stdout+run.stderr)
 return dict(game=game,source=source.name,sha256=hashlib.sha256(source.read_bytes()).hexdigest(),returncode=run.returncode,log=log.name)
jobs=[(g,s) for g in ('GTA3','GTAVC','GTASA') for s in sorted((r/'src').glob('*.cpp')) if g=='GTA3' or s.name!='GTA3WeaponAimHook.cpp']
with concurrent.futures.ThreadPoolExecutor(4) as pool: results=list(pool.map(build,jobs))
for result in results:print(result['game'],result['source'],'PASS' if result['returncode']==0 else 'FAIL')
(out/'results.json').write_text(json.dumps(dict(sdk=revision,results=results),indent=2))
raise SystemExit(any(v['returncode'] for v in results))

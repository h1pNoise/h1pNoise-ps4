"""Verify PS4 filesystem query and host/console/emulator storage policies."""
from pathlib import Path
import argparse,subprocess
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser()
for key in ['llvm','sdk','node']:p.add_argument('--'+key,required=True,type=Path)
a=p.parse_args()
def run(*args):subprocess.run([str(x) for x in args],check=True,cwd=root)
common=['-O1','-nostdlib','-I',root/'tests/wasm','-Wl,--no-entry','-Wl,--export-all','-Wl,--allow-undefined']
for target,defines in [('ps4',[]),('shad',['-DHARBOR_SHADPS4'])]:
    run(a.llvm.resolve()/'clang.exe','--target=wasm64',*common,'-DHARBOR_STORAGE_TEST',*defines,'-isystem',a.sdk.resolve()/'include',root/'src/ps4_storage.c',root/'src/storage.c','-o',root/f'build/storage-query-{target}.wasm')
for target,defines in [('normal',['-D__ORBIS__']),('shad',['-DHARBOR_SHADPS4']),('host',[])]:
    run(a.llvm.resolve()/'clang.exe','--target=wasm32',*common,*defines,root/'src/storage.c','-o',root/f'build/storage-{target}.wasm')
run(a.node.resolve(),root/'tests/ps4_storage_wasm.js',root/'build/storage-query-ps4.wasm',root/'build/storage-query-shad.wasm')
run(a.node.resolve(),root/'tests/storage_wasm.js',root/'build/storage-normal.wasm',root/'build/storage-shad.wasm',root/'build/storage-host.wasm')

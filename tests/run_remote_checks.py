"""Run link checks without downloading content or requiring a PS4.
Usage: python tests/run_remote_checks.py --llvm PATH --sdk PATH --node PATH --pkg PATH
Requires Node with WebAssembly memory64 (tested on Node 24).
"""
from pathlib import Path
import argparse, subprocess
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser()
for key in ['llvm','sdk','node','pkg']:p.add_argument('--'+key,required=True,type=Path)
a=p.parse_args();a.llvm=a.llvm.resolve();a.sdk=a.sdk.resolve();a.pkg=a.pkg.resolve();a.node=a.node.resolve()
def run(*args):subprocess.run([str(x) for x in args],check=True,cwd=root)
common=['-O1','-nostdlib','-I',root/'tests/wasm','-Wl,--no-entry','-Wl,--export-all','-Wl,--allow-undefined']
run(a.llvm/'clang.exe','--target=wasm32',*common,root/'src/pkg_validation.c','-o',root/'build/pkg-validation.wasm')
run(a.llvm/'clang.exe','--target=wasm64',*common,'-D__ORBIS__','-isystem',a.sdk/'include',root/'src/ps4_remote.c',root/'src/pkg_validation.c','-o',root/'build/remote-queue-test.wasm')
run(a.node,root/'tests/pkg_validation_wasm.js',root/'build/pkg-validation.wasm',a.pkg)
run(a.node,root/'tests/remote_queue_wasm.js',root/'build/remote-queue-test.wasm',a.pkg)
run(a.node,root/'tests/web_link.js',root/'web/index.html')

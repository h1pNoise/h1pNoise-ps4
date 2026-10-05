"""Run real signed-PKG verification and independent-install transaction in WASM."""
from pathlib import Path
import argparse,subprocess
root=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser()
for name in ['llvm','sdk','node','pkg']:p.add_argument('--'+name,type=Path,required=True)
a=p.parse_args();out=root/'build/pkg-helper-test.wasm'
sources=['pkg_update.c','update_file.c','update_manifest.c','vendor/monocypher.c','vendor/monocypher-ed25519.c']
cmd=[a.llvm.resolve()/'clang.exe','--target=wasm64','-O1','-nostdlib','-D__ORBIS__','-isystem',a.sdk.resolve()/'include','-Wl,--no-entry','-Wl,--export-all','-Wl,--export-table','-Wl,--allow-undefined','-Wl,-z,stack-size=262144',*[root/'src'/s for s in sources],root/'tests/pkg_update_fixture.c','-o',out]
subprocess.run(list(map(str,cmd)),check=True)
subprocess.run([str(a.node.resolve()),str(root/'tests/pkg_update_wasm.js'),str(out),str(a.pkg.resolve())],check=True)

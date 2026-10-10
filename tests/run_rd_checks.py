"""Compile production C for wasm64 and run synthetic PS4/Real-Debrid checks."""
from pathlib import Path
import argparse, subprocess

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
for name in ['llvm', 'sdk', 'node']:
    p.add_argument('--' + name, required=True, type=Path)
a = p.parse_args()
clang = a.llvm.resolve() / 'clang.exe'
sdk = a.sdk.resolve() / 'include'
build = root / 'build'
build.mkdir(exist_ok=True)
objects = []
for name in ['core', 'magnet', 'engine', 'real_debrid', 'rd_json', 'rd_http', 'pkg_validation', 'rd_fixture']:
    source = root / ('tests' if name == 'rd_fixture' else 'src') / (name + '.c')
    obj = build / (name + '-rd-test.o')
    objects.append(obj)
    subprocess.run(list(map(str, [clang, '--target=wasm64', '-O1', '-nostdlib',
        '-D__ORBIS__', '-isystem', sdk, '-c', source, '-o', obj])), check=True)
wasm = build / 'rd-test.wasm'
subprocess.run(list(map(str, [clang, '--target=wasm64', '-nostdlib',
    '-Wl,--no-entry', '-Wl,--export-all', '-Wl,--export-table',
    '-Wl,--allow-undefined', '-Wl,-z,stack-size=262144', *objects, '-o', wasm])), check=True)
for script, args in [('rd_wasm.js', [wasm]), ('web_link.js', [root / 'web/index.html'])]:
    subprocess.run(list(map(str, [a.node.resolve(), root / 'tests' / script, *args])), check=True)

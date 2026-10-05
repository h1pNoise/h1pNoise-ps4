from pathlib import Path
import argparse,subprocess,json
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives import serialization
root=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser()
for k in ['sdk','llvm','node','pkg']:p.add_argument('--'+k,type=Path,required=True)
a=p.parse_args();out=root/'build/payload-worker-test.wasm';key=Ed25519PrivateKey.generate();pem=root/'build/payload-worker-test.pem';config=root/'build/payload-worker-test.h'
pem.write_bytes(key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.NoEncryption()))
public=key.public_key().public_bytes(serialization.Encoding.Raw,serialization.PublicFormat.Raw)
config.write_text('#define H1PNOISE_UPDATE_CONFIG_H\nstatic const unsigned char UPDATE_PUBLIC_KEY[32]={'+','.join(map(str,public))+'};\n')
sources=['pkg_update_helper.c','pkg_update.c','update_file.c','update_manifest.c','vendor/monocypher.c','vendor/monocypher-ed25519.c']
try:
 subprocess.run(list(map(str,[a.llvm.resolve()/'clang.exe','--target=wasm64','-O1','-nostdlib','-D__ORBIS__','-DHARBOR_PKG_PAYLOAD_TEST','-include',config,'-isystem',a.sdk.resolve()/'include','-Wl,--no-entry','-Wl,--export-all','-Wl,--export-table','-Wl,--allow-undefined','-Wl,-z,stack-size=262144',*[root/'src'/s for s in sources],'-o',out])),check=True)
 subprocess.run([str(a.node.resolve()),str(root/'tests/pkg_payload_worker_wasm.js'),str(out),str(a.pkg.resolve()),str(pem),'https://raw.githubusercontent.com/h1pNoise/h1pNoise-ps4/main/releases/pkg-payload-test/current.h1p',json.dumps(dict(installedVersion='0.1.40',installedBuild=50,installedSfo='00.50',candidateVersion='0.1.41'))],check=True)
 runtime=root/'build/payload-runtime-test.wasm'
 subprocess.run(list(map(str,[a.llvm.resolve()/'clang.exe','--target=wasm64','-O1','-nostdlib','-D__ORBIS__','-DHARBOR_PAYLOAD_RUNTIME_TEST','-isystem',a.sdk.resolve()/'include','-Wl,--no-entry','-Wl,--export-all','-Wl,--allow-undefined',root/'src/payload_runtime.c','-o',runtime])),check=True)
 subprocess.run([str(a.node.resolve()),str(root/'tests/payload_runtime_wasm.js'),str(runtime)],check=True)
finally:pem.unlink(missing_ok=True)

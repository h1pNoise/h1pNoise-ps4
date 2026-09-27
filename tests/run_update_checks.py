"""Test the real updater/HTTPS/installer C with PS4 system calls mocked in Node.
Uses a fresh test-only signing key; never reads the release private key.
"""
from pathlib import Path
import argparse,subprocess
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives import serialization
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser()
for key in ['llvm','sdk','node','pkg']:p.add_argument('--'+key,required=True,type=Path)
a=p.parse_args();a.llvm=a.llvm.resolve();a.sdk=a.sdk.resolve();a.pkg=a.pkg.resolve();a.node=a.node.resolve()
key=Ed25519PrivateKey.generate()
public=key.public_key().public_bytes(serialization.Encoding.Raw,serialization.PublicFormat.Raw)
build=root/'build';build.mkdir(exist_ok=True)
test_key=build/'test-only-ed25519.pem'
test_key.write_bytes(key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.NoEncryption()))
config=build/'update-test-config.h'
config.write_text('''#define H1PNOISE_UPDATE_CONFIG_H
#define UPDATE_FEED_URL "https://github.com/test/app/releases/latest/download/update.h1p"
static const unsigned char UPDATE_PUBLIC_KEY[32]={'''+','.join(str(b) for b in public)+'''};
#define H1PNOISE_VERSION_H
#define APP_VERSION "0.1.9"
#define APP_BUILD 19
#define APP_SFO_VERSION "00.19"
#define APP_TITLE_ID "HBRW00001"
#define APP_CONTENT_ID "IV0000-HBRW00001_00-HARBORPS40000000"
''')
sources=['updater.c','update_http.c','update_platform.c','update_manifest.c','vendor/monocypher.c','vendor/monocypher-ed25519.c']
cmd=[a.llvm/'clang.exe','--target=wasm64','-O1','-nostdlib','-D__ORBIS__','-isystem',a.sdk/'include','-include',config,'-Wl,--no-entry','-Wl,--export-all','-Wl,--export-table','-Wl,--allow-undefined','-Wl,-z,stack-size=262144',*[root/'src'/s for s in sources],root/'tests/updater_fixture.c','-o',build/'updater-test.wasm']
subprocess.run([str(x) for x in cmd],check=True)
try:subprocess.run([str(a.node),str(root/'tests/updater_wasm.js'),str(build/'updater-test.wasm'),str(a.pkg),str(test_key)],check=True)
finally:test_key.unlink(missing_ok=True)

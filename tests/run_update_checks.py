"""Test the real updater/HTTPS/installer C with PS4 system calls mocked in Node.
Uses a fresh test-only signing key; never reads the release private key.
"""
from pathlib import Path
import argparse,subprocess,re,json
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives import serialization
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser()
p.add_argument('--runtime',action='store_true')
p.add_argument('--installed-version',default='0.1.9')
p.add_argument('--installed-build',default=19,type=int)
p.add_argument('--installed-sfo',default='00.19')
for key in ['llvm','sdk','node','pkg']:p.add_argument('--'+key,required=True,type=Path)
a=p.parse_args();a.llvm=a.llvm.resolve();a.sdk=a.sdk.resolve();a.pkg=a.pkg.resolve();a.node=a.node.resolve()
if not re.fullmatch(r'\d+\.\d+\.\d+',a.installed_version) or not re.fullmatch(r'\d{2}\.\d{2}',a.installed_sfo) or not 1<a.installed_build<2147483647:p.error('Invalid installed test version/build/SFO')
if a.runtime and (a.installed_version,a.installed_build,a.installed_sfo)!=('0.1.9',19,'00.19'):p.error('Runtime fixture requires its default installed test version')
candidate=re.search(r'^#define APP_VERSION "([^"]+)"',(root/'src/version.h').read_text(),re.M).group(1)
key=Ed25519PrivateKey.generate()
public=key.public_key().public_bytes(serialization.Encoding.Raw,serialization.PublicFormat.Raw)
build=root/'build';build.mkdir(exist_ok=True)
test_key=build/'test-only-ed25519.pem'
test_key.write_bytes(key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.NoEncryption()))
config=build/'update-test-config.h'
feeds=re.findall(r'^#define UPDATE_FEED_URL "([^"]+)"', (root/'src/update_config.h').read_text(), re.M)
feed=feeds[0] if a.runtime else feeds[1]
config.write_text('''#define H1PNOISE_UPDATE_CONFIG_H
#define UPDATE_FEED_URL "'''+feed+'''"
static const unsigned char UPDATE_PUBLIC_KEY[32]={'''+','.join(str(b) for b in public)+'''};
#define H1PNOISE_VERSION_H
#define APP_VERSION "'''+a.installed_version+'''"
#define APP_BUILD '''+str(a.installed_build)+'''
#define APP_SFO_VERSION "'''+a.installed_sfo+'''"
#define APP_TITLE_ID "HBRW00001"
#define APP_CONTENT_ID "IV0000-HBRW00001_00-HARBORPS40000000"
''')
sources=['updater.c','update_http.c','update_platform.c','ps4_user.c','ps4_bgft.c','installer_access.c','update_manifest.c','vendor/monocypher.c','vendor/monocypher-ed25519.c']
if a.runtime:sources.extend(['runtime_update.c','bootstrap.c'])
cmd=[a.llvm/'clang.exe','--target=wasm64','-O1','-nostdlib','-D__ORBIS__','-isystem',a.sdk/'include','-include',config,'-Wl,--no-entry','-Wl,--export-all','-Wl,--export-table','-Wl,--allow-undefined','-Wl,-z,stack-size=262144',*[root/'src'/s for s in sources],root/'tests/updater_fixture.c','-o',build/'updater-test.wasm']
if a.runtime:cmd[2:2]=['-DHARBOR_RUNTIME_UPDATES','-DHARBOR_RUNTIME_TEST']
subprocess.run([str(x) for x in cmd],check=True)
try:subprocess.run([str(a.node),str(root/('tests/runtime_wasm.js' if a.runtime else 'tests/updater_wasm.js')),str(build/'updater-test.wasm'),str(a.pkg),str(test_key),feed,json.dumps(dict(installedVersion=a.installed_version,installedBuild=a.installed_build,installedSfo=a.installed_sfo,candidateVersion=candidate))],check=True)
finally:test_key.unlink(missing_ok=True)

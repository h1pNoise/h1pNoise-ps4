"""Configure a permanent release feed. The private key must stay outside this project."""
import argparse,re
from pathlib import Path
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives import serialization
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser();p.add_argument('--repo',required=True);p.add_argument('--private-key',type=Path,required=True);a=p.parse_args()
 if not re.fullmatch(r'[A-Za-z0-9-]+/[A-Za-z0-9_.-]+',a.repo):p.error('Use owner/repository')
 private=a.private_key.resolve()
 if private.is_relative_to(ROOT.resolve()):p.error('Keep the private key outside the source project')
 if private.exists():key=serialization.load_pem_private_key(private.read_bytes(),password=None)
 else:
  key=Ed25519PrivateKey.generate();private.parent.mkdir(parents=True,exist_ok=True)
  with private.open('xb') as f:f.write(key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.NoEncryption()))
 public=key.public_key().public_bytes(serialization.Encoding.Raw,serialization.PublicFormat.Raw)
 text='#ifndef H1PNOISE_UPDATE_CONFIG_H\n#define H1PNOISE_UPDATE_CONFIG_H\n'
 text+='#ifdef HARBOR_RUNTIME_UPDATES\n'
 text+=f'#define UPDATE_FEED_URL "https://raw.githubusercontent.com/{a.repo}/main/releases/runtime/current.h1p"\n'
 text+='#elif defined(HARBOR_PKG_PAYLOAD_TEST)\n'
 text+=f'#define UPDATE_FEED_URL "https://raw.githubusercontent.com/{a.repo}/main/releases/pkg-payload-v3-test/current.h1p"\n'
 text+='#elif defined(HARBOR_PKG_DIRECT_TEST)\n'
 text+=f'#define UPDATE_FEED_URL "https://raw.githubusercontent.com/{a.repo}/main/releases/pkg-direct-allocated-test/current.h1p"\n'
 text+='#elif defined(HARBOR_PKG_INSTALLER_TEST)\n'
 text+=f'#define UPDATE_FEED_URL "https://raw.githubusercontent.com/{a.repo}/main/releases/pkg-install-test/current.h1p"\n'
 text+='#else\n'
 text+=f'#define UPDATE_FEED_URL "https://raw.githubusercontent.com/{a.repo}/main/releases/manual-v1/current.h1p"\n'
 text+='#endif\n'
 text+=f'#define UPDATE_REPOSITORY "https://github.com/{a.repo}"\n'
 text+='static const unsigned char UPDATE_PUBLIC_KEY[32]={'+','.join(str(b) for b in public)+'};\n#endif\n'
 (ROOT/'src/update_config.h').write_text(text,encoding='ascii')
 print('Configured release feed for',a.repo,'(public key only embedded).')
if __name__=='__main__':main()

"""Create a signed update asset. This tool does not upload or publish anything."""
from pathlib import Path
import argparse,hashlib,re,struct,json
from cryptography.hazmat.primitives import serialization
CID='IV0000-HBRW00001_00-HARBORPS40000000'
def metadata(data):
 if data[:4]!=b'\x7fCNT' or data[0x40:0x65].split(b'\0')[0].decode()!=CID:raise ValueError('Not a h1pNoise PKG')
 if struct.unpack_from('>I',data,0x74)[0]!=0x1a or struct.unpack_from('>Q',data,0x430)[0]!=len(data):raise ValueError('Invalid PKG size/type')
 count,table=struct.unpack_from('>I',data,0x10)[0],struct.unpack_from('>I',data,0x18)[0]
 for i in range(count):
  ident,_,_,_,off,size=struct.unpack_from('>6I',data,table+i*32)
  if ident!=0x1000:continue
  s=data[off:off+size];magic,_,keys,values,count=struct.unpack_from('<5I',s);assert magic==0x46535000
  out={}
  for i in range(count):
   key,fmt,length,_,offset=struct.unpack_from('<HHIII',s,20+i*16)
   if fmt==0x204:out[s[keys+key:].split(b'\0')[0].decode()]=s[values+offset:values+offset+length].rstrip(b'\0').decode()
  if out.get('TITLE_ID')!='HBRW00001' or out.get('CONTENT_ID')!=CID or out.get('CATEGORY')!='gd':raise ValueError('Wrong application identity')
  return out
 raise ValueError('No SFO')
def main():
 p=argparse.ArgumentParser()
 for x in ['pkg','private-key','out']:p.add_argument('--'+x,type=Path,required=True)
 for x in ['version','url','notes']:p.add_argument('--'+x,required=True)
 p.add_argument('--build',type=int,required=True)
 p.add_argument('--channel',choices=['manual-v1','pkg-install-test','pkg-direct-test','pkg-direct-allocated-test','pkg-payload-test','pkg-payload-v3-test','pkg-payload-v4-test'],help='Validate eligibility and keep experimental auto-install PKGs separate')
 p.add_argument('--runtime',action='store_true',help='Sign executable update for the bootstrap channel (not a PKG)')
 p.add_argument('--sfo',help='Runtime version field, e.g. 00.40')
 a=p.parse_args();data=a.pkg.read_bytes()
 if a.runtime:
  if len(data)<8192 or data[:4]!=bytes.fromhex('4f153d1d') or data[6:8]!=bytes([1,0x12]) or struct.unpack_from('<Q',data,16)[0]!=len(data):raise ValueError('Not a complete PS4 SELF')
  if not re.fullmatch(r'[0-9]{2}\.[0-9]{2}',a.sfo or ''):raise ValueError('Runtime requires --sfo XX.XX')
  sfo={'APP_VER':a.sfo}
 else:sfo=metadata(data)
 if not 8192<=len(data)<=128*1024*1024 or not 0<a.build<2147483648:raise ValueError('Size/build out of range')
 if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+',a.version):raise ValueError('Use a numeric version')
 if a.channel:
  if a.runtime:raise ValueError('Manual PKG and executable channels must stay separate')
  if tuple(map(int,a.version.split('.'))) < (0,1,30) or a.build<40 or sfo['APP_VER']<'00.40':raise ValueError('manual-v1 requires version 0.1.30 / build 40 / APP_VER 00.40 or newer')
  if 'shadps4' in a.pkg.name.lower():raise ValueError('Do not announce an emulator-only PKG to real consoles')
  test_channel='pkg-payload-test' if a.pkg.name.endswith('-payload-test.pkg') else 'pkg-direct-test' if a.pkg.name.endswith('-direct-test.pkg') else 'pkg-install-test' if a.pkg.name.endswith('-install-test.pkg') else 'manual-v1'
  if a.channel!=test_channel and not ((test_channel=='pkg-direct-test' and a.channel=='pkg-direct-allocated-test') or (test_channel=='pkg-payload-test' and a.channel in ('pkg-payload-v3-test','pkg-payload-v4-test'))):raise ValueError('Experimental PKGs must stay on their separate test channel')
 if not a.url.startswith('https://') or len(a.url)>=2048 or any(ord(c)<=32 or ord(c)>=127 or c in '\\#' for c in a.url):raise ValueError('Use a direct HTTPS URL')
 if len(a.notes.encode())>=768 or any(ord(c)<32 or ord(c)==127 for c in a.notes):raise ValueError('Use a single-line description below 768 bytes')
 body=('\n'.join(['H1PNOISE-PS4-RUNTIME-1' if a.runtime else 'H1PNOISE-PS4-UPDATE-1',CID,a.version,sfo['APP_VER'],str(a.build),str(len(data)),hashlib.sha512(data).hexdigest(),a.url,a.notes])+'\n').encode()
 key=serialization.load_pem_private_key(a.private_key.read_bytes(),password=None)
 a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_bytes(key.sign(body)+body)
 print(json.dumps(dict(version=a.version,build=a.build,sfo=sfo['APP_VER'],size=len(data),manifest=str(a.out)),indent=2))
if __name__=='__main__':main()

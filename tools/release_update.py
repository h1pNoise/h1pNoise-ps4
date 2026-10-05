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
 p.add_argument('--channel',choices=['manual-v1'],default='manual-v1',help='Signed PKG update channel')
 a=p.parse_args();data=a.pkg.read_bytes()
 sfo=metadata(data)
 if not 8192<=len(data)<=128*1024*1024 or not 0<a.build<2147483648:raise ValueError('Size/build out of range')
 if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+',a.version):raise ValueError('Use a numeric version')
 if a.pkg.name != 'h1pNoise-'+a.version+'.pkg':raise ValueError('Use h1pNoise-<version>.pkg for a production release')
 if not a.url.startswith('https://') or len(a.url)>=2048 or any(ord(c)<=32 or ord(c)>=127 or c in '\\#' for c in a.url):raise ValueError('Use a direct HTTPS URL')
 if len(a.notes.encode())>=768 or any(ord(c)<32 or ord(c)==127 for c in a.notes):raise ValueError('Use a single-line description below 768 bytes')
 body=('\n'.join(['H1PNOISE-PS4-UPDATE-1',CID,a.version,sfo['APP_VER'],str(a.build),str(len(data)),hashlib.sha512(data).hexdigest(),a.url,a.notes])+'\n').encode()
 key=serialization.load_pem_private_key(a.private_key.read_bytes(),password=None)
 a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_bytes(key.sign(body)+body)
 print(json.dumps(dict(version=a.version,build=a.build,sfo=sfo['APP_VER'],size=len(data),manifest=str(a.out)),indent=2))
if __name__=='__main__':main()

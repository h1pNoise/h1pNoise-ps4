const fs=require('node:fs'),assert=require('node:assert/strict');
(async()=>{
 let instance;const mem=()=>new Uint8Array(instance.exports.memory.buffer);
 const str=p=>{let e=p;while(mem()[e])e++;return Buffer.from(mem().subarray(p,e)).toString();};
 const write=(p,s)=>{mem().set(Buffer.from(s+'\0'),p);};
 const imports={env:{
  memset:(p,v,n)=>{mem().fill(v,p,p+n);return p;},
  memcpy:(d,s,n)=>{mem().copyWithin(d,s,s+n);return d;},
  memcmp:(a,b,n)=>{for(let i=0;i<n;i++)if(mem()[a+i]!==mem()[b+i])return mem()[a+i]-mem()[b+i];return 0;},
  memchr:(p,c,n)=>{for(let i=0;i<n;i++)if(mem()[p+i]===c)return p+i;return 0;},
  snprintf:(p,n,fmt,args)=>{const msg=str(new DataView(mem().buffer).getUint32(args,true));write(p,msg.slice(0,n-1));return msg.length;}
 }};
 ({instance}=await WebAssembly.instantiate(fs.readFileSync(process.argv[2]),imports));
 const e=instance.exports,base=Number(e.__heap_base.value),err=base+10000,out=err+1024;
 let checks=0;
 const valid=(s,expect)=>{write(base,s);assert.equal(e.pkg_url_valid(base,Buffer.byteLength(s),err,512),expect,s);checks++;};
 for(const s of ['http://192.168.1.50:8080/app.pkg','https://example.org/download?token=AbC%20d&key=abc','https://example.org/app.pkg','http://example.org:65535/a'])valid(s,0);
 for(const s of ['', 'ftp://example.org/a.pkg','file:///data/a.pkg','https:///x','https://u:p@example.org/app.pkg','https://example.org:0/x','https://example.org:65536/x','https://example.org:99999999999999999/x','https://example.org:/x','https://example.org/a\r\nX: y','https://example.org/a\0.pkg','https://example.org/a b','https://example.org/a#x','https://example.org/a%','https://example.org/a%zz','https://example.org\\x','https://example.org/'+ 'x'.repeat(2048)])valid(s,-1);
 const real=fs.readFileSync(process.argv[3]).subarray(0,8192);mem().set(real,base);
 assert.equal(e.pkg_header_read(base,8192,out,err,512),0,str(err));checks++;
 assert.equal(str(out),'IV0000-HBRW00001_00-HARBORPS40000000');
 assert.equal(str(out+37),'HBRW00001');
 const view=new DataView(mem().buffer);
 const header=(offset,data,expect)=>{mem().set(real,base);mem().set(data,base+offset);assert.equal(e.pkg_header_read(base,8192,out,err,512),expect,str(err));checks++;};
 header(0,Buffer.from('<htm'),-1);header(0x40,Buffer.from('INVALID'),-1);header(0x64,[65],-1);header(0x74,[0,0,0,0x1b],-1);header(0x78,[0x41,0,0,0],-1);
 header(0x430,[0,0,0,0,0,0,0,0],-1);
 header(0x430,[0,0,0,0x10,0,0,0,0],0);assert.equal(view.getBigUint64(out+48,true),68719476736n); // >4 GB never truncated
 for(const flags of [[0,0x10,0,0],[0x60,0,0,0]]){header(0x78,flags,0);assert.equal(view.getInt32(out+56,true),1);}
 assert.equal(e.pkg_header_read(base,8191,out,err,512),-1);checks++;
 const range=(s,expect,total)=>{write(base,s);assert.equal(e.pkg_range_total(base,Buffer.byteLength(s),out),expect,s);if(total!==undefined)assert.equal(view.getBigUint64(out,true),total);checks++;};
 range('HTTP/1.1 206 Partial Content\r\nContent-Range: bytes 0-8191/68719476736\r\n\r\n',0,68719476736n);
 range('content-range: bytes 0-8191/8192\r\n',0,8192n);
 for(const s of ['Content-Range: bytes 1-8191/9000\r\n','Content-Range: bytes 0-8190/9000\r\n','Content-Range: bytes 0-8191/*\r\n','Content-Range: bytes 0-8191/8191\r\n','Content-Range: bytes 0-8191/18446744073709551616\r\n','Content-Range: bytes 0-8191/9000 junk\r\n','Content-Range: bytes 0-8191/9000\r\nContent-Range: bytes 0-8191/9000\r\n','Content-Length: 8192\r\n'])range(s,-1);
 console.log(`${checks} PKG URL/header/range checks passed (real package + malformed inputs + 64-bit sizes).`);
})().catch(e=>{console.error(e);process.exitCode=1;});

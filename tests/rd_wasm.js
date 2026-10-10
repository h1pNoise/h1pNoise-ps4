// Execute the actual PS4 HTTPS client, RD worker, SHA-1 verifier and installer
// ordering with synthetic service replies. No account or public network used.
const fs=require('node:fs'),assert=require('node:assert/strict'),crypto=require('node:crypto');
(async()=>{
 let instance,heap,options,files,handles,handleId,requests,reply,offset,clock,headers,url,method,installed,resources,selected,uploads,jobs,installOrder;
 const mem=()=>new Uint8Array(instance.exports.memory.buffer),view=()=>new DataView(mem().buffer);
 const str=p=>{p=Number(p);let end=p;while(mem()[end])end++;return Buffer.from(mem().subarray(p,end)).toString();};
 const write=(p,s)=>mem().set(Buffer.from(s+'\0'),Number(p));
 const ptr=p=>view().getBigUint64(Number(p),true);
 const put32=(p,n)=>view().setInt32(Number(p),n,true),put64=(p,n)=>view().setBigUint64(Number(p),BigInt(n),true);
 const alloc=n=>{n=Number(n);let p=heap;heap=(heap+n+15)&~15;if(heap>mem().length)instance.exports.memory.grow(BigInt(Math.ceil((heap-mem().length)/65536)));return BigInt(p);};
 const cstring=s=>{let p=alloc(Buffer.byteLength(s)+1);write(p,s);return p;};
 function fmt(s,args){let at=Number(args);return s.replace(/%([0-9]*)(?:\.([0-9]+))?(ll|z)?([sduXx])/g,(_,width,precision,length,t)=>{let x=t==='s'?str(ptr(at)):length?view().getBigUint64(at,true):t==='d'?view().getInt32(at,true):view().getUint32(at,true);at+=8;if(t==='s'&&precision)x=x.slice(0,+precision);if(t==='X'||t==='x')x=x.toString(16)[t==='X'?'toUpperCase':'toLowerCase']();return String(x).padStart(+width||0,'0');});}
 const hash='a'.repeat(40),token='test_API_1234567890123456',payload=Buffer.alloc(256);payload.set(Buffer.from([127,67,78,84]));
 const magnetPayload=Buffer.alloc(8192);magnetPayload.set([127,67,78,84]);magnetPayload.write('IV0000-HBRW00001_00-HARBORPS40000000',0x40);magnetPayload.writeUInt32BE(0x1a,0x74);magnetPayload.writeBigUInt64BE(8192n,0x430);
 const multiNames=['base.pkg','update.pkg','Backport/backport.pkg'],multiIds=[7,9,11],multiData=multiNames.map((_,i)=>{const b=Buffer.from(magnetPayload);if(i)b.writeUInt32BE(0x60100000,0x78);b[8000]=i;return b;});
 let data=payload;
 const env={
  lock(){},unlock(){},malloc:alloc,calloc:(n,size)=>{let p=alloc(n*size);mem().fill(0,Number(p),Number(p+n*size));return p;},free(){},
  memcpy:(d,s,n)=>{mem().copyWithin(Number(d),Number(s),Number(s+n));return d;},memmove:(d,s,n)=>{mem().copyWithin(Number(d),Number(s),Number(s+n));return d;},memset:(d,c,n)=>{mem().fill(c,Number(d),Number(d+n));return d;},
  memcmp:(a,b,n)=>{for(let i=0;i<Number(n);i++){let d=mem()[Number(a)+i]-mem()[Number(b)+i];if(d)return d;}return 0;},memchr:(p,c,n)=>{for(let i=0;i<Number(n);i++)if(mem()[Number(p)+i]===c)return p+BigInt(i);return 0n;},
  strlen:p=>BigInt(Buffer.byteLength(str(p))),strcmp:(a,b)=>str(a).localeCompare(str(b)),strcasecmp:(a,b)=>str(a).toLowerCase().localeCompare(str(b).toLowerCase()),strncmp:(a,b,n)=>str(a).slice(0,Number(n)).localeCompare(str(b).slice(0,Number(n))),strncasecmp:(a,b,n)=>str(a).slice(0,Number(n)).toLowerCase().localeCompare(str(b).slice(0,Number(n)).toLowerCase()),
  strcpy:(d,s)=>{write(d,str(s));return d;},strchr:(p,c)=>{let i=str(p).indexOf(String.fromCharCode(c));return i<0?0n:p+BigInt(Buffer.byteLength(str(p).slice(0,i)));},strrchr:(p,c)=>{let i=str(p).lastIndexOf(String.fromCharCode(c));return i<0?0n:p+BigInt(Buffer.byteLength(str(p).slice(0,i)));},strstr:(p,s)=>{let i=str(p).indexOf(str(s));return i<0?0n:p+BigInt(i);},
  snprintf:(d,n,s,args)=>{let b=Buffer.from(fmt(str(s),args));mem().set(b.subarray(0,Math.max(0,Number(n)-1)),Number(d));if(n)mem()[Number(d)+Math.min(b.length,Number(n)-1)]=0;return b.length;},
  sprintf:(d,s,args)=>{let text=fmt(str(s),args);write(d,text);return text.length;},
  make_dir:()=>0,mkdir:()=>0,pthread_detach:()=>0,thread_start:(t,fn,arg)=>{put64(t,1);instance.exports.__indirect_function_table.get(Number(fn))(arg);return 0;},
  isalnum:c=>/[A-Za-z0-9]/.test(String.fromCharCode(c))?1:0,
  time:()=>clock++,sleep_ms:()=>{if(options.pause)instance.exports.fixture_pause();},
  storage_check:(_p,_n,e)=>{if(options.noSpace){write(e,'Espaco insuficiente');return -1;}return 0;},
  fopen:(p,mode)=>{p=str(p);mode=str(mode);if(options.writeFail&&mode==='wb')return 0n;if(mode==='rb'&&!files.has(p))return 0n;if(mode==='wb'||(mode==='ab'&&!files.has(p)))files.set(p,Buffer.alloc(0));let id=BigInt(++handleId);handles.set(id,{path:p,at:0});return id;},
  fread:(p,size,count,id)=>{let h=handles.get(id),b=files.get(h.path),n=Math.min(Number(size*count),b.length-h.at);mem().set(b.subarray(h.at,h.at+n),Number(p));h.at+=n;return BigInt(n)/size;},
  fwrite:(p,size,count,id)=>{let h=handles.get(id),b=Buffer.from(mem().subarray(Number(p),Number(p+size*count)));files.set(h.path,Buffer.concat([files.get(h.path),b]));h.at+=b.length;return count;},
  fseeko:(id,n,origin)=>{let h=handles.get(id);h.at=origin===2?files.get(h.path).length+Number(n):Number(n);return 0;},ftello:id=>BigInt(handles.get(id).at),fclose:id=>{assert.ok(handles.delete(id));return 0;},
  ferror:()=>0,remove:p=>{files.delete(str(p));return 0;},unlink:p=>{files.delete(str(p));return 0;},
  open:(p,_flags,args)=>{assert.equal(view().getInt32(Number(args),true),384);if(options.configFail)return -1;p=str(p);files.set(p,Buffer.alloc(0));const id=++handleId;handles.set(BigInt(id),{path:p,at:0});return id;},
  write:(id,p,n)=>{const h=handles.get(BigInt(id)),b=Buffer.from(mem().subarray(Number(p),Number(p+n)));files.set(h.path,Buffer.concat([files.get(h.path),b]));return n;},
  fsync:()=>0,close:id=>{assert.ok(handles.delete(BigInt(id)));return 0;},
  rename:(a,b)=>{a=str(a);b=str(b);if(a.endsWith('.h1pNoise-real-debrid.tmp')){if(options.configRenameFail)return -1;files.set(b,files.get(a));files.delete(a);return 0;}assert.match(a,/file[0-9]{2}\.pkg\.rd\.part$/);assert.match(b,/file[0-9]{2}\.pkg$/);if(options.renameFail)return -1;files.set(b,files.get(a));files.delete(a);return 0;},
  install_pkg:(p,name,error)=>{installed++;installOrder.push(str(name));if(options.multi){const i=multiNames.findIndex(n=>n===str(name));assert.ok(i>=0);assert.deepEqual(files.get(str(p)),multiData[i]);}else{assert.equal(str(name),'test.pkg');assert.deepEqual(files.get(str(p)),data);}if(options.installFail){write(error,'Instalacao recusada');return -1;}return 0;},
  resolve4:(_host,out)=>{mem().set([1,1,1,1],Number(out));return 0;},ntohl:n=>((n&255)<<24)|((n&65280)<<8)|((n>>>8)&65280)|(n>>>24),
  sceSysmoduleLoadModuleInternal:()=>0,sceHttpSetResponseHeaderMaxSize:(_id,n)=>{assert.equal(n,32768n);return 0;},sceHttpSetAutoRedirect:(_id,on)=>{assert.equal(on,0);return 0;},
  sceHttpCreateConnectionWithURL:(_id,u)=>{url=str(u);return create();},sceHttpCreateRequestWithURL:(_id,m,u)=>{method=m;url=str(u);headers={};return create();},sceHttpAddRequestHeader:(_id,k,v)=>{headers[str(k)]=str(v);return 0;},
  sceHttpSendRequest:(_id,body,n)=>{
   let b=Buffer.from(mem().subarray(Number(body),Number(body+n)));requests.push({url,method,headers:{...headers},body:b});offset=0;reply={status:200,body:Buffer.alloc(0)};
   let value;
   if(url.includes('/rest/1.0/'))assert.equal(headers.Authorization,'Bearer '+token);else assert.equal(headers.Authorization,undefined,'token leaked to download host');
   if(url.endsWith('/user')){if(options.httpError){reply.status=options.httpError;value={error:'bad_token'};}else value={type:options.free?'free':'premium',premium:options.free?0:3600};}
   else if(url.endsWith('/torrents/addMagnet')){uploads++;assert.equal(method,1);assert.equal(headers['Content-Type'],'application/x-www-form-urlencoded');assert.equal(new URLSearchParams(b.toString()).get('magnet'),'magnet:?xt=urn:btih:'+hash);value={id:options.multi?'job'+uploads:'job123'};if(options.multi)jobs.set(value.id,{target:-1});}
   else if(url.endsWith('/torrents/addTorrent')){uploads++;assert.equal(method,4);assert.equal(headers['Content-Type'],'application/x-bittorrent');assert.deepEqual(b,Buffer.from('test-torrent\0binary'));value={id:options.badId?'../oops':options.multi?'job'+uploads:'job123'};if(options.multi)jobs.set(value.id,{target:-1});}
   else if(options.multi&&url.includes('/torrents/info/')){const j=jobs.get(url.split('/').pop());value={hash,status:j.target<0?'waiting_files_selection':'downloaded',files:multiNames.map((name,i)=>({id:multiIds[i],path:'/'+name,bytes:8192,selected:i===j.target?1:0})),links:j.target<0?[]:options.bundle?['https://host/download/0','https://host/download/1']:['https://host/download/'+j.target]};if(options.multiWrongHash)value.hash='b'.repeat(40);if(options.extraSelected&&j.target>=0)value.files[(j.target+1)%3].selected=1;}
   else if(url.includes('/torrents/info/')){value={hash:options.wrongHash?'b'.repeat(40):hash,status:selected?'downloaded':'waiting_files_selection',files:[{id:1,path:options.wrongPath?'/other.pkg':'/test.pkg',bytes:data.length,selected:selected?1:0}],links:options.parts?['https://host/a','https://host/b']:['https://host/download']};if(options.archive)value.files[0].path='/archive.zip';if(options.tooMany)value.files=Array.from({length:33},(_,i)=>({id:i+1,path:'/file'+i+'.pkg',bytes:8192}));if(options.traversal)value.files[0].path='/../test.pkg';if(options.duplicate)value.files.push({...value.files[0],id:2});if(options.dead)value.status='dead';if(options.pause&&!selected)value.status='queued';}
   else if(options.multi&&url.includes('/torrents/selectFiles/')){assert.equal(method,1);const id=new URLSearchParams(b.toString()).get('files');assert.ok(!id.includes(','),'must select one PKG only');const target=multiIds.indexOf(+id);assert.ok(target>=0);jobs.get(url.split('/').pop()).target=target;reply.status=204;if(options.pauseMulti&&target===1)instance.exports.fixture_pause();}
   else if(url.includes('/torrents/selectFiles/')){assert.equal(method,1);assert.equal(b.toString(),'files=1');selected=true;reply.status=204;}
   else if(options.multi&&url.endsWith('/unrestrict/link')){const link=new URLSearchParams(b.toString()).get('link');assert.match(link,/https:\/\/host\/download\/[0-2]$/);const i=+link.split('/').pop();value={filename:options.multiWrongName?'other.pkg':multiNames[i].split('/').pop(),filesize:8192,download:'https://cdn.real-debrid.com/file'+i+'.pkg'};}
   else if(url.endsWith('/unrestrict/link')){assert.equal(method,1);assert.equal(headers['Content-Type'],'application/x-www-form-urlencoded');assert.match(b.toString(),/^link=https%3A%2F%2Fhost%2Fdownload$/);value={filename:options.wrongName?'other.pkg':'test.pkg',filesize:options.wrongSize?123:data.length,download:'https://cdn.real-debrid.com/test.pkg'};}
   else if(options.multi&&/^https:\/\/cdn.real-debrid.com\/file[0-2]\.pkg$/.test(url)){const i=+url.match(/file([0-2])/)[1];reply.body=Buffer.from(multiData[i]);if(options.multiCorrupt&&i===2)reply.body[8000]^=255;if(options.multiTruncated&&i===1)reply.body=reply.body.subarray(0,180);}
   else if(url==='https://cdn.real-debrid.com/test.pkg'){reply.body=options.truncated?data.subarray(0,180):Buffer.from(data);if(options.corrupt)reply.body[200]^=255;if(options.downloadError)reply.status=403;}
   else throw Error('Unexpected request '+url);
   if(value?.files&&options.extras){value.files.unshift({id:90,path:'/cover.jpg',bytes:10,selected:options.selectedExtras?1:0});value.files.push(...Array.from({length:40},(_,i)=>({id:100+i,path:'/note'+i+'.nfo',bytes:0,selected:0})));}
   if(value?.files&&options.archiveExt)value.files[0].path='/archive'+options.archiveExt;
   if(value)reply.body=Buffer.from(options.malformed&&url.endsWith('/user')?'not JSON':JSON.stringify(value));
   if(options.apiRedirect&&url.endsWith('/user')){reply.status=302;reply.location='https://cdn.real-debrid.com/leak';}
   if(options.downloadRedirect&&url==='https://cdn.real-debrid.com/test.pkg'){reply.status=302;reply.location='http://cdn.real-debrid.com/unsafe';}
   return 0;
  },
  sceHttpGetStatusCode:(_id,p)=>{put32(p,reply.status);return 0;},sceHttpGetAllResponseHeaders:(_id,out,n)=>{let s='Location: '+reply.location+'\r\n',p=cstring(s);put64(out,p);put64(n,Buffer.byteLength(s));return 0;},
  sceHttpReadData:(_id,p,limit)=>{let n=Math.min(64,limit,reply.body.length-offset);mem().set(reply.body.subarray(offset,offset+n),Number(p));offset+=n;return n;}
 };
 const create=()=>{let id=++handleId;resources.add(id);return id;};
 for(let name of ['sceNetPoolCreate','sceSslInit','sceHttpInit','sceHttpCreateTemplate'])env[name]=create;
 for(let name of ['sceNetPoolDestroy','sceSslTerm','sceHttpTerm','sceHttpDeleteTemplate','sceHttpDeleteConnection','sceHttpDeleteRequest'])env[name]=id=>{assert.ok(resources.delete(id));return 0;};
 for(let name of ['sceHttpSetConnectTimeOut','sceHttpSetResolveTimeOut','sceHttpSetRecvTimeOut','sceHttpSetSendTimeOut'])env[name]=()=>0;
 const module=await WebAssembly.compile(fs.readFileSync(process.argv[2]));for(let imp of WebAssembly.Module.imports(module))if(!env[imp.name])env[imp.name]=()=>{throw Error('Unexpected call '+imp.name);};
 instance=await WebAssembly.instantiate(module,{env});const e=instance.exports,base=Number(e.__heap_base.value);let checks=0;
 function reset(cfg={}){heap=base+2048;options=cfg;data=cfg.badPkg?Buffer.alloc(256):payload;files=new Map([['/data/pkg/test-torrent/source.torrent',Buffer.from('test-torrent\0binary')]]);handles=new Map();resources=new Set();handleId=0;requests=[];clock=1000n;installed=uploads=0;selected=false;jobs=new Map();installOrder=[];
  const hashes=Buffer.concat([crypto.createHash('sha1').update(data.subarray(0,128)).digest(),crypto.createHash('sha1').update(data.subarray(128)).digest()]),p=alloc(40);mem().set(hashes,Number(p));e.fixture_reset(p);
  assert.equal(e.rd_configure(cstring(token),BigInt(token.length),BigInt(base),512n),0);
 }
 function run(cfg={},install=1){reset(cfg);e.fixture_run(install);assert.equal(e.fixture_busy(),0);assert.equal(handles.size,0);assert.equal(resources.size,0);checks++;return str(e.fixture_phase());}
 assert.equal(run(),'installed');assert.equal(installed,1);assert.equal(e.fixture_done(),256n);
 assert.equal(run({},0),'downloaded');assert.equal(installed,0);
 for(let cfg of [{httpError:401},{httpError:403},{httpError:429},{httpError:503},{free:true},{malformed:true},{badId:true},{wrongHash:true},{wrongPath:true},{wrongName:true},{wrongSize:true},{parts:true},{truncated:true},{corrupt:true},{downloadError:true},{writeFail:true},{renameFail:true},{noSpace:true},{badPkg:true},{apiRedirect:true},{downloadRedirect:true}]){assert.equal(run(cfg),'error',JSON.stringify(cfg)+' '+str(e.fixture_message()));assert.equal(installed,0,JSON.stringify(cfg));assert.equal(e.fixture_done(),cfg.badPkg?256n:0n,JSON.stringify(cfg));}
 assert.equal(run({installFail:true}),'error');assert.equal(installed,1);
 assert.equal(run({pause:true}),'paused');assert.equal(uploads,1);options.pause=false;e.fixture_run(1);assert.equal(str(e.fixture_phase()),'installed');assert.equal(uploads,1,'resume re-uploaded torrent');checks++;
 reset();assert.equal(e.rd_configure(cstring('bad\r\nheader'),11n,BigInt(base),512n),-1);assert.equal(e.rd_configure(cstring('short'),5n,BigInt(base),512n),-1);assert.equal(e.rd_configure(cstring(''),0n,BigInt(base),512n),0);checks+=3;
 // Unicode, duplicate keys, malformed numbers, oversized integers and nesting.
 for(let [json,valid] of [['{"name":"Ol\\u00e1 \\ud83d\\ude00"}',true],['{"x":01}',false],['{"x":1,}',false],['{"x":}',false],['[true,false,null]',true],['"unterminated',false],['{"x":1e}',false],['{"x":"bad\\q"}',false],['['.repeat(30)+']'.repeat(30),false]]){const d=alloc(64),j=cstring(json);assert.equal(e.rd_json_parse(d,j,BigInt(Buffer.byteLength(json)))===0,valid,json);if(valid)e.rd_json_free(d);checks++;}
 let d=alloc(64),j=cstring('{"key":"Ol\\u00e1 \\ud83d\\ude00","n":18446744073709551616,"dup":1,"dup":2}');assert.equal(e.rd_json_parse(d,j,BigInt(str(j).length)),0);let out=alloc(128);assert.equal(e.rd_json_string(d,e.rd_json_key(d,0,cstring('key')),out,128n),0);assert.equal(str(out),'Olá 😀');assert.equal(e.rd_json_key(d,0,cstring('dup')),-1);assert.equal(e.rd_json_uint(d,e.rd_json_key(d,0,cstring('n')),alloc(8)),-1);checks+=4;e.rd_json_free(d);
 // Pausing RD magnet preparation resumes the same remote task without re-uploading.
 reset({pause:true});data=magnetPayload;const pausedMagnet='magnet:?xt=urn:btih:'+hash;assert.equal(e.fixture_magnet(cstring(pausedMagnet),BigInt(pausedMagnet.length),BigInt(base),512n),0);assert.equal(str(e.fixture_phase()),'paused');options.pause=false;assert.equal(e.begin_download(1,BigInt(base),512n),0);assert.equal(str(e.fixture_phase()),'installed');assert.equal(uploads,1);checks++;
 // Restart restores both the API and switch; disabling preserves the credential.
 reset();e.fixture_reload();assert.equal(e.fixture_enabled(),1);assert.equal(e.fixture_configured(),1);e.fixture_run(0);assert.equal(str(e.fixture_phase()),'downloaded');checks++;
 reset();assert.equal(e.rd_enable(0,BigInt(base),512n),0);e.fixture_reload();assert.equal(e.fixture_enabled(),0);assert.equal(e.fixture_configured(),1);assert.equal(e.rd_enable(1,BigInt(base),512n),0);e.fixture_reload();assert.equal(e.fixture_enabled(),1);checks++;
 assert.equal(e.rd_forget(BigInt(base),512n),0);e.fixture_reload();assert.equal(e.fixture_enabled(),0);assert.equal(e.fixture_configured(),0);assert.ok(!files.get('/data/pkg/.h1pNoise-real-debrid.conf').includes(token));assert.equal(e.rd_enable(1,BigInt(base),512n),-1);checks++;
 for(const key of ['configFail','configRenameFail']){reset();options[key]=true;assert.equal(e.rd_enable(0,BigInt(base),512n),-1);assert.equal(e.fixture_enabled(),1);options[key]=false;e.fixture_reload();assert.equal(e.fixture_enabled(),1);checks++;}
 for(const text of ['H1RD1\n1\nshort','H1RD1\n1\n'+token+'\n','H1RD1\n1\n','H1RD9\n1\n'+token,'H1RD1\n1\n'+'a'.repeat(257)]){reset();files.set('/data/pkg/.h1pNoise-real-debrid.conf',Buffer.from(text));e.fixture_reload();assert.equal(e.fixture_enabled(),0);assert.equal(e.fixture_configured(),0);checks++;}
 // Direct magnets do not contact trackers or upload a .torrent, even without tr.
 for(const cfg of [{},{archive:true},{tooMany:true},{traversal:true},{duplicate:true},{wrongHash:true},{truncated:true},{badHeader:true},{headerSize:true},{downloadError:true}]){
  reset(cfg);data=Buffer.from(magnetPayload);if(cfg.badHeader)data.fill(0,0,4);if(cfg.headerSize)data.writeBigUInt64BE(9999n,0x430);
  const url='magnet:?xt=urn:btih:'+hash;assert.equal(e.fixture_magnet(cstring(url),BigInt(url.length),BigInt(base),512n),0);
  if(cfg.archive||cfg.tooMany||cfg.traversal||cfg.duplicate||cfg.wrongHash){assert.equal(str(e.fixture_phase()),'error');assert.equal(installed,0);}
  else{assert.equal(str(e.fixture_phase()),'ready');assert.equal(e.fixture_loaded(),1);e.fixture_run(1);assert.equal(str(e.fixture_phase()),Object.keys(cfg).length?'error':'installed');assert.equal(installed,Object.keys(cfg).length?0:1);}
  assert.equal(uploads,1);assert.equal(handles.size,0);assert.equal(resources.size,0);checks++;
 }
 reset();data=magnetPayload;const finalMagnet='magnet:?xt=urn:btih:'+hash;assert.equal(e.fixture_magnet(cstring(finalMagnet),BigInt(finalMagnet.length),BigInt(base),512n),0);assert.equal(e.rd_enable(0,BigInt(base),512n),0);assert.equal(e.begin_download(1,BigInt(base),512n),-1);assert.equal(installed,0);checks++;
 reset();assert.equal(e.rd_enable(0,BigInt(base),512n),0);assert.equal(e.fixture_magnet(cstring(finalMagnet),BigInt(finalMagnet.length),BigInt(base),512n),-1,'native trackerless magnets must still be rejected');assert.equal(uploads,0);checks++;
 // Multiple PKGs use independent selections, including a file in a subfolder.
 function setMulti(){const joined=Buffer.concat(multiData),hashes=[];for(let i=0;i<joined.length;i+=128)hashes.push(crypto.createHash('sha1').update(joined.subarray(i,i+128)).digest());const b=Buffer.concat(hashes),p=alloc(b.length);mem().set(b,Number(p));e.fixture_multi(p);}
 for(const cfg of [{multi:true},{multi:true,multiCorrupt:true},{multi:true,multiTruncated:true},{multi:true,multiWrongName:true},{multi:true,multiWrongHash:true},{multi:true,extraSelected:true},{multi:true,bundle:true}]){reset(cfg);setMulti();e.fixture_run(1);const success=Object.keys(cfg).length===1;assert.equal(str(e.fixture_phase()),success?'installed':'error',JSON.stringify(cfg)+' '+str(e.fixture_message()));assert.equal(installed,success?3:0);if(success){assert.deepEqual(installOrder,['base.pkg','update.pkg','Backport/backport.pkg']);assert.equal(e.fixture_done(),24576n);assert.equal(uploads,3);}assert.equal(handles.size,0);assert.equal(resources.size,0);checks++;}
 reset({multi:true,pauseMulti:true});setMulti();e.fixture_run(1);assert.equal(str(e.fixture_phase()),'paused');assert.equal(installed,0);options.pauseMulti=false;e.fixture_run(1);assert.equal(str(e.fixture_phase()),'installed');assert.equal(uploads,3,'pause must reuse individual RD jobs');checks++;
 reset({multi:true});data=magnetPayload;const multiMagnet='magnet:?xt=urn:btih:'+hash;assert.equal(e.fixture_magnet(cstring(multiMagnet),BigInt(multiMagnet.length),BigInt(base),512n),0);assert.equal(str(e.fixture_phase()),'ready');e.fixture_run(1);assert.equal(str(e.fixture_phase()),'installed');assert.equal(installed,3);assert.equal(uploads,3);checks++;
 // Extra files do not block direct PKGs, even when the total file count is >32.
 for(const cfg of [{extras:true},{extras:true,selectedExtras:true},{multi:true,extras:true}]){
  reset(cfg);data=magnetPayload;assert.equal(e.fixture_magnet(cstring(multiMagnet),BigInt(multiMagnet.length),BigInt(base),512n),0);assert.equal(str(e.fixture_phase()),'ready',str(e.fixture_message()));e.fixture_run(1);
  assert.equal(str(e.fixture_phase()),cfg.selectedExtras?'error':'installed',str(e.fixture_message()));assert.equal(installed,cfg.selectedExtras?0:cfg.multi?3:1);assert.equal(e.fixture_done(),cfg.selectedExtras?0n:cfg.multi?24576n:8192n);assert.equal(handles.size,0);assert.equal(resources.size,0);checks++;
 }
 // Full paths distinguish equal basenames; each PKG has its own job and local file.
 const oldName=multiNames[2];multiNames[2]='Backport/base.pkg';reset({multi:true,extras:true});data=magnetPayload;assert.equal(e.fixture_magnet(cstring(multiMagnet),BigInt(multiMagnet.length),BigInt(base),512n),0);assert.equal(str(e.fixture_phase()),'ready');e.fixture_run(1);assert.equal(str(e.fixture_phase()),'installed',str(e.fixture_message()));assert.equal(installed,3);multiNames[2]=oldName;checks++;
 for(const ext of ['.zip','.rar','.pkg.001']){reset({archiveExt:ext});data=magnetPayload;assert.equal(e.fixture_magnet(cstring(multiMagnet),BigInt(multiMagnet.length),BigInt(base),512n),0);assert.equal(str(e.fixture_phase()),'error');assert.match(str(e.fixture_message()),/nao tem PKG diretos/);assert.equal(installed,0);checks++;}
 assert.equal(run({extras:true}),'error','uploaded torrents must retain exact file matching for SHA-1 verification');checks++;
 const trace=files.get('/data/pkg/real-debrid-debug.log').toString();assert.ok(!trace.includes(token));assert.ok(!trace.includes('https://'));checks++;
 console.log('Real-Debrid checks passed: '+checks+' scenarios, real SHA-1 and HTTPS code; no public network.');
})();

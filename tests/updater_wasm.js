// Execute production crypto, parser, worker, HTTPS redirects and BGFT ABI.
// Only libc/filesystem, threads and PS4 system APIs are mocked. No network access.
const fs=require('node:fs'),crypto=require('node:crypto'),assert=require('node:assert/strict');
(async()=>{
 const pkg=fs.readFileSync(process.argv[3]),key=crypto.createPrivateKey(fs.readFileSync(process.argv[4]));
 // Read the supplied package's APP_VER so this fixture can exercise a newer
 // local build while simulating the selected installed version.
 const installed=JSON.parse(process.argv[6]),nextBuild=installed.installedBuild+1;
 const packagePath='/data/pkg/h1pNoise-'+installed.candidateVersion+'.pkg';
 const updateName=/(?:h1pNoise-update-\d+\.pkg(?:\.part)?|h1pNoise-\d+\.\d+\.\d+\.pkg(?:\.part)?|\.h1pNoise-update-\d+\.h1p)$/;
 let packageSfo;
 for(let i=0;i<pkg.readUInt32BE(0x10);i++){
  const entry=pkg.readUInt32BE(0x18)+i*32;if(pkg.readUInt32BE(entry)!==0x1000)continue;
  const sfo=pkg.readUInt32BE(entry+16),keys=sfo+pkg.readUInt32LE(sfo+8),values=sfo+pkg.readUInt32LE(sfo+12);
  for(let j=0;j<pkg.readUInt32LE(sfo+16);j++){
   const field=sfo+20+j*16,k=keys+pkg.readUInt16LE(field),value=values+pkg.readUInt32LE(field+12);
   if(pkg.toString('utf8',k,pkg.indexOf(0,k))==='APP_VER')packageSfo=pkg.toString('utf8',value,pkg.indexOf(0,value));
  }
 }
 assert.match(packageSfo||'',/^\d{2}\.\d{2}$/);assert.ok(packageSfo>installed.installedSfo,'Supply a package newer than the selected installed version');
 const url='https://github.com/test/app/releases/download/v0.1.10/app.pkg',feed=process.argv[5];
 assert.match(feed,/^https:\/\/raw\.githubusercontent\.com\/h1pNoise\/h1pNoise-ps4\/main\/releases\/manual-v1\/current\.h1p$/);
 const fields=['H1PNOISE-PS4-UPDATE-1','IV0000-HBRW00001_00-HARBORPS40000000',installed.candidateVersion,packageSfo,String(nextBuild),String(pkg.length),crypto.createHash('sha512').update(pkg).digest('hex'),url,'Atualização de teste'];
 const signed=(f=fields)=>{const body=Buffer.from(f.join('\n')+'\n');return Buffer.concat([crypto.sign(null,body,key),body]);};
 let usbElevated=false,usbAuthorized=false,directories,descriptors;
 let instance,heap,cfg,resources,headerLimits,files,handles,handleId,requests,requestUrl,bodyOffset,reply,clock,registered,started,notifications,userReady,privileged,savedCred,slotQueries,prepared;
 const m=()=>new Uint8Array(instance.exports.memory.buffer),v=()=>new DataView(m().buffer);
 const str=p=>{p=Number(p);let end=p;while(m()[end])end++;return Buffer.from(m().subarray(p,end)).toString();};
 const write=(p,s)=>m().set(Buffer.from(s+'\0'),Number(p));
 const put32=(p,x)=>v().setInt32(Number(p),x,true),put64=(p,x)=>v().setBigUint64(Number(p),BigInt(x),true);
 const ptr=p=>v().getBigUint64(Number(p),true);
 function alloc(n){n=Number(n);const p=heap;heap=(heap+n+15)&~15;if(heap>m().length)instance.exports.memory.grow(BigInt(Math.ceil((heap-m().length)/65536)));return BigInt(p);}
 function fmt(s,args){let at=Number(args);return s.replace(/%([0-9]*)(?:\.([0-9]+))?([sduXx])/g,(_,width,precision,t)=>{let x;t==='s'?(x=str(ptr(at))):(x=t==='d'?v().getInt32(at,true):v().getUint32(at,true));at+=8;if(t==='s'&&precision)x=x.slice(0,Number(precision));return (t==='X'||t==='x'?x.toString(16)[t==='X'?'toUpperCase':'toLowerCase']():String(x)).padStart(Number(width)||0,'0');});}
 const env={
  pthread_mutex_lock:()=>0,pthread_mutex_trylock:()=>0,pthread_mutex_unlock:()=>0,
  memcpy:(d,s,n)=>{m().copyWithin(Number(d),Number(s),Number(s)+Number(n));return d;},
  memmove:(d,s,n)=>{m().copyWithin(Number(d),Number(s),Number(s)+Number(n));return d;},
  memset:(d,c,n)=>{m().fill(c,Number(d),Number(d)+Number(n));return d;},
  __errno_location:()=>BigInt(base+1024),
  open:(path,flags)=>{path=str(path);if(flags===131072){assert.match(path,/^\/mnt\/usb[01]$/);if(!cfg.usb?.includes(path)||cfg.usbRegularFile||(cfg.usbSandbox&&!usbElevated)){put32(BigInt(base+1024),2);return -1;}const id=++handleId;directories.set(id,path);return id;}
   assert.equal(usbElevated,true,'absolute USB file open needs temporary full root');assert.equal(usbAuthorized,true);assert.match(path.split('/').at(-1),updateName);assert.ok(flags&256,'refuse symbolic links');if(cfg.openFail||cfg.absoluteOpenError){put32(BigInt(base+1024),cfg.absoluteOpenError||30);return -1;}if(flags&512)files.set(path,Buffer.alloc(0));if(!files.has(path)){put32(BigInt(base+1024),2);return -1;}const id=++handleId;descriptors.set(id,path);return id;},
  close:id=>{assert.ok(directories.delete(id)||descriptors.delete(id),'close unknown descriptor');return 0;},
  jbc_jailbreak_cred:p=>{if(cfg.usbResolveFail)return -1;m().fill(0,Number(p),Number(p)+80);return 0;},
  jbc_set_cred:p=>{const restoring=Buffer.from(m().subarray(Number(p),Number(p)+80)).equals(savedCred);usbAuthorized=!restoring;usbElevated=!Buffer.from(m().subarray(Number(p)+32,Number(p)+56)).equals(savedCred.subarray(32,56));if(restoring&&cfg.usbRestoreFail)return -1;if(!restoring&&cfg.usbActivateFail)return -1;return 0;},jbc_resolve_error:()=>0,
  fdopen:(fd,mode)=>{assert.equal(usbElevated,false);assert.equal(usbAuthorized,false);assert.ok(descriptors.has(fd));if(cfg.fdopenFail){put32(BigInt(base+1024),12);return 0n;}const path=descriptors.get(fd);descriptors.delete(fd);const id=BigInt(++handleId);handles.set(id,{path,at:0});return id;},
  jbc_get_cred:p=>{m().fill(17,Number(p),Number(p)+80);put64(p+56n,0x3800000000000011n);put64(p+64n,0x1000n);put64(p+72n,0x2000n);savedCred=Buffer.from(m().subarray(Number(p),Number(p)+80));return 0;},
  jbc_set_auth:p=>{
   p=Number(p);assert.deepEqual(Buffer.from(m().subarray(p,p+56)),savedCred.subarray(0,56),'filesystem/UID changed');
   const paid=v().getBigUint64(p+56,true);privileged=paid===0x3800000000000010n;assert.equal(paid,privileged?0x3800000000000010n:0x3800000000000011n);
   assert.equal(v().getBigUint64(p+64,true),privileged?(0x1000n|(1n<<62n)):0x1000n);assert.equal(v().getBigUint64(p+72,true),0x2000n);
   if(privileged&&cfg.authActivateFailure)return -1;if(!privileged&&cfg.authRestoreFailure){privileged=true;return -1;}return 0;
  },
  memcmp:(a,b,n)=>{for(let i=0;i<Number(n);i++){const d=m()[Number(a)+i]-m()[Number(b)+i];if(d)return d;}return 0;},
  memchr:(p,c,n)=>{for(let i=0;i<Number(n);i++)if(m()[Number(p)+i]===c)return p+BigInt(i);return 0n;},
  strrchr:(p,c)=>{const i=str(p).lastIndexOf(String.fromCharCode(c));return i<0?0n:p+BigInt(i);},
  strlen:p=>BigInt(Buffer.byteLength(str(p))),strcmp:(a,b)=>str(a).localeCompare(str(b)),
  strncmp:(a,b,n)=>str(a).slice(0,Number(n)).localeCompare(str(b).slice(0,Number(n))),
  strncasecmp:(a,b,n)=>str(a).slice(0,Number(n)).toLowerCase().localeCompare(str(b).slice(0,Number(n)).toLowerCase()),
  strcpy:(d,s)=>{write(d,str(s));return d;},strchr:(p,c)=>{const i=str(p).indexOf(String.fromCharCode(c));return i<0?0n:p+BigInt(i);},
  snprintf:(d,n,s,args)=>{const result=fmt(str(s),args),bytes=Buffer.from(result);m().set(bytes.subarray(0,Math.max(0,Number(n)-1)),Number(d));if(n)m()[Number(d)+Math.min(bytes.length,Number(n)-1)]=0;return bytes.length;},
  malloc:alloc,free:()=>{},time:()=>clock++,lock:()=>{},unlock:()=>{},pthread_detach:()=>0,sleep_ms:()=>{},
  thread_start:(out,fn,arg)=>{if(cfg.threadFail)return -1;put64(out,1);instance.exports.__indirect_function_table.get(Number(fn))(arg);return 0;},
  mkdir:path=>{assert.equal(str(path),'/data/pkg','never create a USB mount');return 0;},
  stat:(path,out)=>{path=str(path);if(!cfg.usb?.includes(path))return -1;instance.exports.fixture_directory(out,cfg.usbRegularFile?0:1);return 0;},
  storage_check:(path,needed,error,cap)=>{assert.equal(str(path),cfg.destination||'/data/pkg');assert.equal(needed,BigInt(pkg.length)+67108864n);if(cfg.noSpace){write(error,'Espaco insuficiente');return -1;}return 0;},
  fopen:(path,mode)=>{path=str(path);mode=str(mode);if(cfg.openFail)return 0n;if(mode==='rb'&&!files.has(path)){put32(BigInt(base+1024),2);return 0n;}if(mode==='wb')files.set(path,Buffer.alloc(0));const id=BigInt(++handleId);handles.set(id,{path,at:0});return id;},
  fwrite:(data,size,count,id)=>{if((cfg.recordWriteFail&&handles.get(id).path.endsWith('.h1p'))||cfg.writeFail||(cfg.unplugAfterBytes&&files.get(handles.get(id).path).length>=cfg.unplugAfterBytes))return 0n;const h=handles.get(id),n=Number(size*count),b=Buffer.from(m().subarray(Number(data),Number(data)+n));files.set(h.path,Buffer.concat([files.get(h.path),b]));h.at+=n;return count;},
  fread:(data,size,count,id)=>{const h=handles.get(id),b=files.get(h.path),n=Math.min(Number(size*count),b.length-h.at);m().set(b.subarray(h.at,h.at+n),Number(data));h.at+=n;return BigInt(n)/size;},
  fflush:()=>cfg.flushFail?-1:0,ferror:()=>0,rewind:id=>{handles.get(id).at=0;},fclose:id=>{assert.ok(handles.delete(id));return 0;},
  remove:path=>{path=str(path);assert.match(path.split('/').at(-1),updateName);if(cfg.cleanupFailAt===path){put32(BigInt(base+1024),13);return -1;}if(!files.delete(path)){put32(BigInt(base+1024),2);return -1;}return 0;},
  unlink:path=>{const p=path;path=str(path);if(path.startsWith('/data/pkg/'))return env.remove(p);assert.equal(usbElevated,true);assert.equal(usbAuthorized,true);assert.match(path.split('/').at(-1),updateName);if(cfg.cleanupFailAt===path){put32(BigInt(base+1024),13);return -1;}if(!files.delete(path)){put32(BigInt(base+1024),2);return -1;}return 0;},
  rename:(a,b)=>{a=str(a);b=str(b);if(a.startsWith('/mnt/usb')){assert.equal(usbElevated,true);assert.equal(usbAuthorized,true);assert.equal(a.slice(0,10),b.slice(0,10));assert.match(b.split('/').at(-1),updateName);}if(cfg.renameFail){put32(BigInt(base+1024),5);return -1;}assert.ok(files.has(a));files.set(b,files.get(a));files.delete(a);return 0;},
  sceSysmoduleLoadModuleInternal:()=>0,sceHttpSetAutoRedirect:(_id,enable)=>{assert.equal(enable,0);return 0;},
  sceHttpSetResponseHeaderMaxSize:(id,size)=>{assert.equal(id,4);assert.equal(size,32768n);headerLimits.set(id,Number(size));return 0;},
  sceHttpAddRequestHeader:(_id,name,val)=>{assert.equal(str(name),'Accept-Encoding');assert.equal(str(val),'identity');return 0;},
  sceHttpSendRequest:id=>{
   assert.equal(usbElevated,false,'network ran with changed roots');assert.equal(usbAuthorized,false,'network ran with changed credentials');
   requests.push(requestUrl);bodyOffset=0;
   reply=cfg.redirect?.(requestUrl)||{status:cfg.status||200,body:requestUrl===feed?cfg.manifest:cfg.payload};
   if(cfg.sendError)return cfg.sendError|0;
   const size=Buffer.byteLength(reply.headers||'Location: '+(reply.location||'')+'\r\n');
   if(!cfg.ignoreHeaderLimit&&size>headerLimits.get(id))return 0x80431073|0;
   return 0;
  },
  sceHttpGetStatusCode:(_id,out)=>{put32(out,reply.status);return 0;},
  sceHttpGetAllResponseHeaders:(_id,out,len)=>{const s=reply.headers||'Location: '+reply.location+'\r\n',p=alloc(Buffer.byteLength(s)+1);write(p,s);put64(out,p);put64(len,Buffer.byteLength(s));return 0;},
  sceHttpReadData:(_id,dest,limit)=>{if(cfg.readFail)return -1;const n=Math.min(limit,16384,reply.body.length-bodyOffset);m().set(reply.body.subarray(bodyOffset,bodyOffset+n),Number(dest));bodyOffset+=n;return n;},
  ps4_installer_ready:()=>0,sceAppInstUtilGetTitleIdFromPkg:(path,out,isApp)=>{assert.equal(str(path),packagePath);write(out,cfg.wrongInstalledId?'OTHER0001':'HBRW00001');put32(isApp,cfg.isApp??1);return 0;},
  sceAppInstUtilGetPrimaryAppSlot:(title,out)=>{assert.ok(privileged);assert.equal(str(title),'HBRW00001');slotQueries++;put32(out,cfg.slot??0);return cfg.slotFailure?-123:0;},
  sceAppInstUtilAppPrepareOverwritePkg:path=>{assert.ok(privileged);assert.equal(str(path),packagePath);prepared++;return cfg.prepareFailure?-123:0;},
  sceUserServiceInitialize:params=>{assert.equal(v().getUint32(Number(params),true),0x2bc);const rc=cfg.userInitResult||0;userReady=!rc||(rc>>>0)===0x80960003;return rc;},
  sceUserServiceGetForegroundUser:out=>{if(!userReady)return 0x80960002|0;put32(out,cfg.userId??0x10000000);return 0;},
  sceBgftServiceIntDownloadRegisterTaskByStorageEx:(p,out)=>{
   assert.ok(privileged,'BGFT registration lost permissions');
   registered++;p=Number(p);assert.equal(v().getInt32(p,true),0x10000000);assert.equal(v().getInt32(p+4,true),5);
   assert.equal(str(ptr(p+8)),fields[1]);assert.equal(str(ptr(p+16)),'/user'+packagePath);
   assert.equal(v().getUint32(p+56,true),8);assert.equal(v().getUint32(p+104,true),registered===1?0:(cfg.slot??0));
   if(cfg.registerFail)return -123;if(cfg.registerResult)return cfg.registerResult|0;
   if(cfg.sameInstalled&&(registered===1||cfg.sameInstalledAgain))return 0x80990088|0;
   put32(out,cfg.invalidTask?-1:42);return 0;
  },
  sceBgftServiceDownloadStartTask:id=>{assert.equal(id,42);assert.ok(privileged,'BGFT start lost permissions');started++;return cfg.startFail?-123:0;},
  sceBgftServiceIntDownloadRegisterTask:()=>{throw Error('Unexpected remote registration');},
  sceBgftServiceIntDebugDownloadRegisterPkg:()=>{throw Error('Unexpected remote patch registration');},
  sceKernelSendNotificationRequest:()=>{notifications++;return 0;}
 };
 const creators=['sceNetPoolCreate','sceSslInit','sceHttpInit','sceHttpCreateTemplate','sceHttpCreateConnectionWithURL','sceHttpCreateRequestWithURL'];
 creators.forEach((name,i)=>env[name]=(...a)=>{if(name==='sceHttpCreateRequestWithURL')requestUrl=str(a[2]);assert.ok(!resources.has(i+1));resources.add(i+1);if(i===3)headerLimits.set(i+1,5000);if(i>=4)headerLimits.set(i+1,headerLimits.get(a[0]));return i+1;});
 for(const name of ['sceNetPoolDestroy','sceSslTerm','sceHttpTerm','sceHttpDeleteTemplate','sceHttpDeleteConnection','sceHttpDeleteRequest'])env[name]=id=>{assert.ok(resources.delete(id),'cleanup '+name);return 0;};
 for(const name of ['sceHttpSetConnectTimeOut','sceHttpSetResolveTimeOut','sceHttpSetRecvTimeOut','sceHttpSetSendTimeOut'])env[name]=()=>0;
 const api={};for(const [name,fn] of Object.entries(env))api[name]=(...args)=>cfg?.fail===name?-999:fn(...args);
 const wasm=fs.readFileSync(process.argv[2]),module=await WebAssembly.compile(wasm);
 for(const imp of WebAssembly.Module.imports(module))assert.ok(api[imp.name],'Missing test import '+imp.name);
 instance=await WebAssembly.instantiate(module,{env:api});let e=instance.exports;const base=Number(e.__heap_base.value),error=BigInt(base);let checks=0;
 function reset(options={}){heap=base+2048;cfg={manifest:signed(),payload:pkg,...options};usbElevated=false;usbAuthorized=false;directories=new Map();descriptors=new Map();privileged=false;userReady=false;resources=new Set();headerLimits=new Map();files=new Map([['/data/pkg/existing-torrent/file00.pkg',Buffer.from('keep')]]);handles=new Map();handleId=0;requests=[];clock=1000n;registered=started=notifications=slotQueries=prepared=0;e.fixture_reset();}
 function op(n,ret=0,phase){write(error,'');assert.equal(e.updater_begin(n,error,512n),ret,str(error));assert.equal(resources.size,0,'network cleanup');assert.equal(handles.size,0,'file cleanup');assert.equal(directories.size,0,'directory cleanup');assert.equal(descriptors.size,0,'descriptor cleanup');if(!cfg.authRestoreFailure)assert.equal(privileged,false,'permissions not restored');assert.equal(e.fixture_state(0),0,'worker finished');if(phase)assert.equal(str(e.fixture_phase()),phase,str(e.fixture_message()));assert.equal(files.get('/data/pkg/existing-torrent/file00.pkg').toString(),'keep');checks++;}
 function available(options={}){reset(options);op(0,0,'available');assert.equal(e.fixture_state(1),1);assert.equal(notifications,1);}
 function ready(options={}){available(options);op(1,0,'ready');assert.equal(e.fixture_state(2),1);assert.equal(e.fixture_done(),BigInt(pkg.length));assert.deepEqual(files.get(packagePath),pkg);assert.deepEqual(files.get('/data/pkg/.h1pNoise-update-'+nextBuild+'.h1p'),cfg.manifest);}
 ready();op(2,0,'error');assert.equal(registered,0);assert.equal(started,0);assert.equal(prepared,0);assert.equal(slotQueries,0);assert.equal(e.fixture_state(3),-1);assert.equal(e.fixture_state(2),1);assert.match(str(e.fixture_message()),/instalacao pela propria app foi suspensa/);assert.ok(str(e.fixture_message()).includes(packagePath));assert.deepEqual(files.get(packagePath),pkg);op(2,0,'error');assert.equal(registered,0);
 available();op(0,0,'available');assert.equal(notifications,1);op(2,-1,'available');assert.equal(registered,0);
 reset();op(1,-1);e.fixture_busy(1);op(0,-1);e.fixture_busy(0);
 reset({threadFail:true});op(0,-1,'error');
 for(const build of [String(installed.installedBuild-1),String(installed.installedBuild)]){reset({manifest:signed(fields.map((s,i)=>i===4?build:s))});op(0,0,Number(build)<installed.installedBuild?'channel-old':'current');assert.equal(e.fixture_state(1),0);assert.equal(notifications,0);assert.equal(e.fixture_state(2),0);assert.match(str(e.fixture_message()),Number(build)<installed.installedBuild?/canal anuncia uma versao anterior/:/mais recente deste canal/);}
 for(const [index,value] of [[0,'OTHER'],[1,'OTHER'],[2,'x'],[2,'1..0'],[2,'1.0.'],[2,'.1.0'],[2,'1.0'],[2,'1.0.0.0'],[2,'../../1'],[3,'1.2'],[3,installed.installedSfo],[4,'0'],[4,'2147483648'],[5,'8191'],[5,'999999999999999999999999'],[6,'g'.repeat(128)],[7,'http://github.com/file'],[7,'https://github.com@evil.example/file'],[7,'https://github.com/file%GG'],[8,'x'.repeat(768)],[8,'bad\rtext']]){reset({manifest:signed(fields.map((s,i)=>i===index?value:s))});op(0,0,'error');assert.equal(e.fixture_state(1),0);}
 for(const changed of [(()=>{const b=signed();b[0]^=1;return b;})(),Buffer.alloc(64),Buffer.alloc(4097),signed([...fields,'extra'])]){reset({manifest:changed});op(0,0,'error');}
 for(const options of [{status:404},{status:403},{status:206},{readFail:true},...creators.map(fail=>({fail})),{fail:'sceHttpSetAutoRedirect'},{fail:'sceHttpSetResponseHeaderMaxSize'},{fail:'sceHttpSendRequest'},{fail:'sceHttpSetRecvTimeOut'}]){reset(options);op(0,0,'error');}
 // Reproduce GitHub's response > the PS4 default 5000 bytes; a larger bounded
 // setting must be inherited by every redirect request, not set after receipt.
 const asset='https://release-assets.githubusercontent.com/test/package';
 const largeHeaders=(dest,n)=>{const h='Location: '+dest+'\r\nX-Security: ';return h+'x'.repeat(n-Buffer.byteLength(h)-2)+'\r\n';};
 ready({redirect:u=>u===url?{status:302,headers:largeHeaders(asset,5226)}:u===asset?{status:200,body:pkg}:undefined});
 assert.equal(requests.at(-1),asset);
 const feedRedirect='https://raw.githubusercontent.com/h1pNoise/h1pNoise-ps4/main/releases/manual-v1/redirect-test.h1p';
 available({redirect:u=>u===feed?{status:302,headers:largeHeaders(feedRedirect,32768)}:{status:200,body:signed()}});
 for(const ignoreHeaderLimit of [false,true]){reset({ignoreHeaderLimit,redirect:()=>({status:302,headers:largeHeaders(asset,32769)})});op(0,0,'error');assert.match(str(e.fixture_message()),/limite de cabecalhos/);assert.equal(requests.length,1);}
 for(const [sendError,message] of [[0x80431073,'limite de cabecalhos'],[0x80431075,'ligacao HTTPS']]){reset({sendError});op(0,0,'error');assert.ok(str(e.fixture_message()).includes(message));}
 available({redirect:u=>u===feed?{status:302,location:feedRedirect}:{status:200,body:signed()}});
 available({redirect:u=>u===feed?{status:302,location:'/test/app/releases/download/v0.1.10/update.h1p'}:{status:200,body:signed()}});
 for(const location of ['http://github.com/insecure','https://github.com.evil.example/update','//evil.example/update','https://evil.example/update','https://release-assets.githubusercontent.com/test/feed']){reset({redirect:()=>({status:302,location})});op(0,0,'error');assert.equal(requests.length,1);}
 reset({redirect:()=>({status:302,location:feed})});op(0,0,'error');assert.equal(requests.length,6);
 reset({redirect:()=>({status:302,headers:'Location: '+feed+'\r\nLocation: '+feed+'\r\n'})});op(0,0,'error');
 for(const options of [{noSpace:true},{openFail:true},{writeFail:true},{flushFail:true},{recordWriteFail:true},{renameFail:true},{payload:pkg.subarray(0,100)},{payload:Buffer.concat([pkg,Buffer.from([0])])},{payload:Buffer.from(pkg).fill(0,10000,10001)}]){available(options);op(1,0,'error');assert.equal(e.fixture_state(2),0);assert.equal(registered,0);assert.equal(files.has(packagePath),false);}
 for(const offset of [0,0x40,0x74,0x430]){const b=Buffer.from(pkg);b[offset]^=1;const f=[...fields];f[6]=crypto.createHash('sha512').update(b).digest('hex');available({payload:b,manifest:signed(f)});op(1,0,'error');assert.equal(e.fixture_state(2),0);}
 ready();files.get(packagePath)[10000]^=1;op(2,0,'error');assert.equal(e.fixture_state(2),0);assert.equal(registered,0);op(2,-1);
 // Run the real fixed-destination resolver and USB writing path, including
 // detachment and write/flush failures. No installation APIs may be called.
 function saveTo(id,ret=0,phase='ready'){
  write(error,'');const p=alloc(id.length+1);write(p,id);
  assert.equal(e.updater_download_to(p,error,512n),ret,str(error));
  if(!ret)assert.equal(str(e.fixture_phase()),phase,str(e.fixture_message()));
  assert.equal(e.fixture_state(0),0);assert.equal(handles.size,0);assert.equal(directories.size,0);assert.equal(descriptors.size,0);if(!cfg.usbRestoreFail){assert.equal(usbElevated,false);assert.equal(usbAuthorized,false);}assert.equal(resources.size,0);
  assert.equal(registered,0);assert.equal(started,0);assert.equal(prepared,0);checks++;
 }
 for(const [id,root] of [['usb0','/mnt/usb0'],['usb1','/mnt/usb1']]){
  const dest=root+packagePath.slice('/data/pkg'.length);
  available({usb:[root],usbSandbox:true,destination:root});saveTo(id);assert.deepEqual(files.get(dest),pkg);assert.equal(str(e.fixture_update_path()),dest);assert.match(str(e.fixture_message()),/Package Installer/);
  // Copy again to internal storage, retaining the verified USB file.
  cfg.destination='/data/pkg';saveTo('internal');assert.deepEqual(files.get(dest),pkg);assert.deepEqual(files.get(packagePath),pkg);
  // Directory visibility must not change the required permissions for file I/O.
  available({usb:[root],destination:root});saveTo(id);assert.deepEqual(files.get(dest),pkg);
  available({usb:[],destination:root});saveTo(id,0,'error');assert.match(str(e.fixture_message()),/pen USB/);assert.equal(e.fixture_state(2),0);assert.equal(requests.length,1);
  available({usb:[root],usbRegularFile:true,destination:root});saveTo(id,0,'error');assert.equal(requests.length,1);
  for(const options of [{usbResolveFail:true,usbSandbox:true},{usbActivateFail:true,usbSandbox:true},{usbActivateFail:true},{openFail:true},{fdopenFail:true},...([1,2,13,14,22,30,78].map(absoluteOpenError=>({absoluteOpenError}))),{noSpace:true},{writeFail:true},{flushFail:true},{unplugAfterBytes:16384},{recordWriteFail:true},{renameFail:true}]){
   available({usb:[root],destination:root,...options});saveTo(id,0,'error');assert.equal(e.fixture_state(2),0);assert.equal(files.has(dest),false);if(!options.renameFail)assert.equal(files.has(dest+'.part'),false,JSON.stringify(options));if(options.absoluteOpenError){assert.equal(requests.length,1,'no download after file creation failure');assert.match(str(e.fixture_message()),new RegExp('erro '+options.absoluteOpenError));}
  }
 }
 available();for(const id of ['../usb0','/mnt/usb0','usb2','USB0','internal/../',''])saveTo(id,-1);
 e.fixture_busy(1);saveTo('internal',-1);e.fixture_busy(0);
 // Cleanup cannot touch arbitrary PKGs, directories, installed or newer builds.
 function cleanup(id,ret=0){
  const before=requests.length,p=alloc(id.length+1);write(p,id);write(error,'');
  assert.equal(e.updater_cleanup_to(p,error,512n),ret,str(error));
  assert.equal(requests.length,before,'cleanup must not use the network');
  assert.equal(e.fixture_state(0),0);assert.equal(handles.size,0);assert.equal(directories.size,0);assert.equal(descriptors.size,0);assert.equal(usbElevated,false);assert.equal(usbAuthorized,false);checks++;
 }
 for(const [id,root] of [['internal','/data/pkg'],['usb0','/mnt/usb0'],['usb1','/mnt/usb1']]){
  ready({usb:[root],usbSandbox:true});
  const kept=[root+'/game.pkg',root+'/custom-update.pkg',root+'/h1pNoise-update-abc.pkg',root+'/h1pNoise-update-1.pkg.bak',root+'/h1pNoise-update-'+installed.installedBuild+'.pkg',root+'/h1pNoise-update-'+installed.installedBuild+'.pkg.part',root+'/h1pNoise-update-'+nextBuild+'.pkg',root+'/folder/h1pNoise-update-1.pkg','/other/h1pNoise-update-1.pkg'];
  for(const name of kept)if(!files.has(name))files.set(name,Buffer.from('keep'));
  const before=new Map([...files].map(([k,v])=>[k,Buffer.from(v)]));
  files.set(root+'/h1pNoise-update-1.pkg',Buffer.from('old'));files.set(root+'/h1pNoise-update-2.pkg.part',Buffer.from('partial'));
  const phase=str(e.fixture_phase()),message=str(e.fixture_message()),path=str(e.fixture_update_path()),done=e.fixture_done();
  cleanup(id);assert.match(str(e.fixture_cleanup_message()),/Apagados 2 ficheiros/);
  assert.equal(files.has(root+'/h1pNoise-update-1.pkg'),false);assert.equal(files.has(root+'/h1pNoise-update-2.pkg.part'),false);
  for(const [name,data] of before)assert.deepEqual(files.get(name),data,name);
  assert.equal(e.fixture_state(1),1);assert.equal(e.fixture_state(2),1);assert.equal(str(e.fixture_phase()),phase);assert.equal(str(e.fixture_message()),message);assert.equal(str(e.fixture_update_path()),path);assert.equal(e.fixture_done(),done);
  cleanup(id);assert.match(str(e.fixture_cleanup_message()),/Nao foram encontrados/);
  // Version-named packages require a signed download record and matching hash.
  const oldFields=fields.map((s,i)=>i===2?'0.9.9':i===4?'1':s);
  const oldRecord=root+'/.h1pNoise-update-1.h1p',oldPkg=root+'/h1pNoise-0.9.9.pkg';
  files.set(oldRecord,signed(oldFields));files.set(oldPkg,Buffer.from(pkg));
  files.set(root+'/h1pNoise-0.9.8.pkg',Buffer.from(pkg)); // untracked must survive
  cleanup(id);assert.match(str(e.fixture_cleanup_message()),/Apagados 1 ficheiros/);
  assert.equal(files.has(oldRecord),false);assert.equal(files.has(oldPkg),false);assert.equal(files.has(root+'/h1pNoise-0.9.8.pkg'),true);
  files.set(oldRecord,signed(oldFields));files.set(oldPkg+'.part',Buffer.from(pkg));
  cleanup(id);assert.equal(files.has(oldPkg+'.part'),false);assert.equal(files.has(oldRecord),false);
  const replaced=Buffer.from(pkg);replaced[10000]^=1;
  files.set(oldRecord,signed(oldFields));files.set(oldPkg,replaced);
  cleanup(id);assert.deepEqual(files.get(oldPkg),replaced);assert.equal(files.has(oldRecord),true);
  for(const record of [Buffer.alloc(4097),Buffer.from(signed(oldFields)).fill(0,0,64),signed(oldFields.map((s,i)=>i===4?'2':s))]){
   files.set(oldRecord,record);files.set(oldPkg,Buffer.from(pkg));cleanup(id);assert.deepEqual(files.get(oldPkg),pkg);assert.deepEqual(files.get(oldRecord),record);
  }
  files.delete(oldRecord);files.delete(oldPkg);
  cfg.cleanupFailAt=root+'/h1pNoise-update-2.pkg';files.set(root+'/h1pNoise-update-1.pkg',Buffer.from('old'));files.set(cfg.cleanupFailAt,Buffer.from('blocked'));files.set(root+'/h1pNoise-update-3.pkg',Buffer.from('later'));
  cleanup(id);assert.match(str(e.fixture_cleanup_message()),/Apagados 1 ficheiros.*erro 13/);assert.equal(files.has(cfg.cleanupFailAt),true);assert.equal(files.has(root+'/h1pNoise-update-3.pkg'),true);assert.equal(e.fixture_state(2),1);
 }
 reset();cleanup('internal');assert.match(str(e.fixture_cleanup_message()),/Nao foram encontrados/);assert.equal(e.fixture_state(1),0);
 for(const id of ['../usb0','/mnt/usb0','usb2','USB0','internal/../',''])cleanup(id,-1);
 e.fixture_busy(1);cleanup('internal',-1);e.fixture_busy(0);
 reset({threadFail:true});files.set('/data/pkg/h1pNoise-update-1.pkg',Buffer.from('old'));cleanup('internal',-1);assert.equal(files.has('/data/pkg/h1pNoise-update-1.pkg'),true);
 reset({usb:[]});cleanup('usb0');assert.match(str(e.fixture_cleanup_message()),/pen USB/);
 available({usb:['/mnt/usb0'],usbSandbox:true,usbRestoreFail:true,destination:'/mnt/usb0'});saveTo('usb0',0,'error');assert.equal(requests.length,1);assert.match(str(e.fixture_message()),/restaurar/);assert.equal(e.fixture_state(2),0);
 // New instance resets the fail-closed credential state for a file-open failure.
 instance=await WebAssembly.instantiate(module,{env:api});e=instance.exports;
 available({usb:['/mnt/usb0'],usbRestoreFail:true,destination:'/mnt/usb0'});saveTo('usb0',0,'error');assert.equal(requests.length,1);assert.equal(e.fixture_state(2),0);
 assert.equal(WebAssembly.Module.imports(module).some(x=>/update_usb_native|openat|renameat|unlinkat/.test(x.name)),false,'USB must use absolute libkernel-backed file operations');
 assert.equal(WebAssembly.Module.imports(module).some(x=>/AppPrepareOverwrite|AppUnInstall|GetPrimaryAppSlot|GetTitleIdFromPkg/.test(x.name)),false,'Updater must not import native replacement/removal APIs');
 console.log(`${checks} updater checks passed (${installed.installedVersion} -> ${installed.candidateVersion}): real Ed25519/SHA-512, bounded signed metadata, HTTPS redirect allowlist/cleanup, streamed file validation, busy/duplicate guards, re-verification, blocked in-process installation, saved PKG preservation and failure recovery.`);
})().catch(error=>{console.error(error);process.exitCode=1;});

// Execute production crypto, parser, worker, HTTPS redirects and BGFT ABI.
// Only libc/filesystem, threads and PS4 system APIs are mocked. No network access.
const fs=require('node:fs'),crypto=require('node:crypto'),assert=require('node:assert/strict');
(async()=>{
 const pkg=fs.readFileSync(process.argv[3]),key=crypto.createPrivateKey(fs.readFileSync(process.argv[4]));
 // Read the supplied package's APP_VER so this fixture can exercise a newer
 // local build while simulating the selected installed version.
 const installed=JSON.parse(process.argv[6]),nextBuild=installed.installedBuild+1;
 const packagePath='/data/pkg/h1pNoise-update-'+nextBuild+'.pkg';
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
 assert.match(feed,/^https:\/\/raw\.githubusercontent\.com\/h1pNoise\/h1pNoise-ps4\/main\/releases\/pkg-install-test\/current\.h1p$/);
 const fields=['H1PNOISE-PS4-UPDATE-1','IV0000-HBRW00001_00-HARBORPS40000000',installed.candidateVersion,packageSfo,String(nextBuild),String(pkg.length),crypto.createHash('sha512').update(pkg).digest('hex'),url,'Atualização de teste'];
 const signed=(f=fields)=>{const body=Buffer.from(f.join('\n')+'\n');return Buffer.concat([crypto.sign(null,body,key),body]);};
 let instance,heap,cfg,resources,headerLimits,files,handles,handleId,requests,requestUrl,bodyOffset,reply,clock,registered,started,notifications,userReady,privileged,savedCred,slotQueries,prepared;
 const m=()=>new Uint8Array(instance.exports.memory.buffer),v=()=>new DataView(m().buffer);
 const str=p=>{p=Number(p);let end=p;while(m()[end])end++;return Buffer.from(m().subarray(p,end)).toString();};
 const write=(p,s)=>m().set(Buffer.from(s+'\0'),Number(p));
 const put32=(p,x)=>v().setInt32(Number(p),x,true),put64=(p,x)=>v().setBigUint64(Number(p),BigInt(x),true);
 const ptr=p=>v().getBigUint64(Number(p),true);
 function alloc(n){n=Number(n);const p=heap;heap=(heap+n+15)&~15;if(heap>m().length)instance.exports.memory.grow(BigInt(Math.ceil((heap-m().length)/65536)));return BigInt(p);}
 function fmt(s,args){let at=Number(args);return s.replace(/%([0-9]*)([sduXx])/g,(_,width,t)=>{let x;t==='s'?(x=str(ptr(at))):(x=t==='d'?v().getInt32(at,true):v().getUint32(at,true));at+=8;return (t==='X'||t==='x'?x.toString(16)[t==='X'?'toUpperCase':'toLowerCase']():String(x)).padStart(Number(width)||0,'0');});}
 const env={
  pthread_mutex_lock:()=>0,pthread_mutex_trylock:()=>0,pthread_mutex_unlock:()=>0,
  memcpy:(d,s,n)=>{m().copyWithin(Number(d),Number(s),Number(s)+Number(n));return d;},
  memmove:(d,s,n)=>{m().copyWithin(Number(d),Number(s),Number(s)+Number(n));return d;},
  memset:(d,c,n)=>{m().fill(c,Number(d),Number(d)+Number(n));return d;},
  open:()=>{throw Error('Unexpected installer initialization');},close:()=>0,
  jbc_jailbreak_cred:()=>{throw Error('Unexpected filesystem root change');},jbc_set_cred:()=>{throw Error('Unexpected filesystem root change');},jbc_resolve_error:()=>0,
  jbc_get_cred:p=>{m().fill(17,Number(p),Number(p)+80);put64(p+56n,0x3800000000000011n);put64(p+64n,0x1000n);put64(p+72n,0x2000n);savedCred=Buffer.from(m().subarray(Number(p),Number(p)+80));return 0;},
  jbc_set_auth:p=>{
   p=Number(p);assert.deepEqual(Buffer.from(m().subarray(p,p+56)),savedCred.subarray(0,56),'filesystem/UID changed');
   const paid=v().getBigUint64(p+56,true);privileged=paid===0x3800000000000010n;assert.equal(paid,privileged?0x3800000000000010n:0x3800000000000011n);
   assert.equal(v().getBigUint64(p+64,true),privileged?(0x1000n|(1n<<62n)):0x1000n);assert.equal(v().getBigUint64(p+72,true),0x2000n);
   if(privileged&&cfg.authActivateFailure)return -1;if(!privileged&&cfg.authRestoreFailure){privileged=true;return -1;}return 0;
  },
  memcmp:(a,b,n)=>{for(let i=0;i<Number(n);i++){const d=m()[Number(a)+i]-m()[Number(b)+i];if(d)return d;}return 0;},
  memchr:(p,c,n)=>{for(let i=0;i<Number(n);i++)if(m()[Number(p)+i]===c)return p+BigInt(i);return 0n;},
  strlen:p=>BigInt(Buffer.byteLength(str(p))),strcmp:(a,b)=>str(a).localeCompare(str(b)),
  strncmp:(a,b,n)=>str(a).slice(0,Number(n)).localeCompare(str(b).slice(0,Number(n))),
  strncasecmp:(a,b,n)=>str(a).slice(0,Number(n)).toLowerCase().localeCompare(str(b).slice(0,Number(n)).toLowerCase()),
  strcpy:(d,s)=>{write(d,str(s));return d;},strchr:(p,c)=>{const i=str(p).indexOf(String.fromCharCode(c));return i<0?0n:p+BigInt(i);},
  snprintf:(d,n,s,args)=>{const result=fmt(str(s),args),bytes=Buffer.from(result);m().set(bytes.subarray(0,Math.max(0,Number(n)-1)),Number(d));if(n)m()[Number(d)+Math.min(bytes.length,Number(n)-1)]=0;return bytes.length;},
  malloc:alloc,free:()=>{},time:()=>clock++,lock:()=>{},unlock:()=>{},pthread_detach:()=>0,sleep_ms:()=>{},
  thread_start:(out,fn,arg)=>{if(cfg.threadFail)return -1;put64(out,1);instance.exports.__indirect_function_table.get(Number(fn))(arg);return 0;},
  storage_check:(path,needed,error,cap)=>{assert.equal(str(path),'/data/pkg');assert.equal(needed,BigInt(pkg.length)+67108864n);if(cfg.noSpace){write(error,'Espaco insuficiente');return -1;}return 0;},
  fopen:(path,mode)=>{path=str(path);mode=str(mode);if(cfg.openFail)return 0n;if(mode==='rb'&&!files.has(path))return 0n;if(mode==='wb')files.set(path,Buffer.alloc(0));const id=BigInt(++handleId);handles.set(id,{path,at:0});return id;},
  fwrite:(data,size,count,id)=>{if(cfg.writeFail)return 0n;const h=handles.get(id),n=Number(size*count),b=Buffer.from(m().subarray(Number(data),Number(data)+n));files.set(h.path,Buffer.concat([files.get(h.path),b]));h.at+=n;return count;},
  fread:(data,size,count,id)=>{const h=handles.get(id),b=files.get(h.path),n=Math.min(Number(size*count),b.length-h.at);m().set(b.subarray(h.at,h.at+n),Number(data));h.at+=n;return BigInt(n)/size;},
  fflush:()=>cfg.flushFail?-1:0,ferror:()=>0,rewind:id=>{handles.get(id).at=0;},fclose:id=>{assert.ok(handles.delete(id));return 0;},
  remove:path=>{path=str(path);assert.match(path,/^\/data\/pkg\/h1pNoise-update-\d+\.pkg\.part$/);files.delete(path);return 0;},
  rename:(a,b)=>{a=str(a);b=str(b);if(cfg.renameFail)return -1;assert.ok(files.has(a));files.set(b,files.get(a));files.delete(a);return 0;},
  sceSysmoduleLoadModuleInternal:()=>0,sceHttpSetAutoRedirect:(_id,enable)=>{assert.equal(enable,0);return 0;},
  sceHttpSetResponseHeaderMaxSize:(id,size)=>{assert.equal(id,4);assert.equal(size,32768n);headerLimits.set(id,Number(size));return 0;},
  sceHttpAddRequestHeader:(_id,name,val)=>{assert.equal(str(name),'Accept-Encoding');assert.equal(str(val),'identity');return 0;},
  sceHttpSendRequest:id=>{
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
 let launched=0,closed=0,sleeps=0;
 class MainClosed extends Error {}
 env.getpid=()=>1234;
 env.mkdir=p=>{assert.ok(['/data/harbor','/data/harbor/pkg-updater'].includes(str(p)));return 0;};
 env.access=p=>files.has(str(p))?0:-1;
 env.open=p=>{p=str(p);assert.equal(p,'/data/harbor/pkg-updater/request');if(files.has(p))return -1;files.set(p,Buffer.alloc(0));return 7;};
 env.fileno=()=>7;env.fsync=()=>cfg.requestFlushFailure?-1:0;
 env.random_bytes=(p,n)=>{assert.equal(n,16n);m().fill(0x11,Number(p),Number(p)+16);return 0;};
 env.remove=p=>{p=str(p);assert.ok(p.endsWith('.pkg.part')||['/data/harbor/pkg-updater/ack','/data/harbor/pkg-updater/request'].includes(p),'Update PKGs must never be deleted');files.delete(p);return 0;};
 env.sceAppInstUtilAppExists=(p,out)=>{assert.equal(str(p),'HBRU00001');put32(out,cfg.helperMissing?0:1);return 0;};
 env.sceSystemServiceLaunchApp=(title,args,param)=>{
  assert.equal(str(title),'HBRU00001');assert.equal(args,0n);assert.equal(v().getUint32(Number(param),true),32);assert.equal(v().getUint32(Number(param)+4,true),0x10000000);assert.ok(privileged);launched++;
  const request=files.get('/data/harbor/pkg-updater/request').toString();assert.equal(request,'1234\n'+'11'.repeat(16)+'\n');
  const manifest=files.get('/data/harbor/pkg-updater/manifest.h1p');assert.deepEqual(manifest,signed());assert.ok(crypto.verify(null,manifest.subarray(64),crypto.createPublicKey(key),manifest.subarray(0,64)));
  if(cfg.launchFail)return -123;
  files.set('/data/harbor/pkg-updater/claimed',files.get('/data/harbor/pkg-updater/request'));files.delete('/data/harbor/pkg-updater/request');
  if(!cfg.noAck)files.set('/data/harbor/pkg-updater/ack',Buffer.from((cfg.wrongAck?'22':'11').repeat(16)));return 0;
 };
 env.sleep_ms=ms=>{assert.equal(ms,500);sleeps++;};
 env._exit=code=>{assert.equal(code,0);assert.equal(privileged,false,'Close only after credentials restored');assert.equal(handles.size,0);assert.equal(resources.size,0);closed++;throw new MainClosed();};
 const creators=['sceNetPoolCreate','sceSslInit','sceHttpInit','sceHttpCreateTemplate','sceHttpCreateConnectionWithURL','sceHttpCreateRequestWithURL'];
 creators.forEach((name,i)=>env[name]=(...a)=>{if(name==='sceHttpCreateRequestWithURL')requestUrl=str(a[2]);assert.ok(!resources.has(i+1));resources.add(i+1);if(i===3)headerLimits.set(i+1,5000);if(i>=4)headerLimits.set(i+1,headerLimits.get(a[0]));return i+1;});
 for(const name of ['sceNetPoolDestroy','sceSslTerm','sceHttpTerm','sceHttpDeleteTemplate','sceHttpDeleteConnection','sceHttpDeleteRequest'])env[name]=id=>{assert.ok(resources.delete(id),'cleanup '+name);return 0;};
 for(const name of ['sceHttpSetConnectTimeOut','sceHttpSetResolveTimeOut','sceHttpSetRecvTimeOut','sceHttpSetSendTimeOut'])env[name]=()=>0;
 const api={};for(const [name,fn] of Object.entries(env))api[name]=(...args)=>cfg?.fail===name?-999:fn(...args);
 const wasm=fs.readFileSync(process.argv[2]),module=await WebAssembly.compile(wasm);
 for(const imp of WebAssembly.Module.imports(module))assert.ok(api[imp.name],'Missing test import '+imp.name);
 instance=await WebAssembly.instantiate(module,{env:api});const e=instance.exports,base=Number(e.__heap_base.value),error=BigInt(base);let checks=0;
 function reset(options={}){heap=base+2048;cfg={manifest:signed(),payload:pkg,...options};privileged=false;userReady=false;resources=new Set();headerLimits=new Map();files=new Map([['/data/pkg/existing-torrent/file00.pkg',Buffer.from('keep')]]);handles=new Map();handleId=0;requests=[];clock=1000n;registered=started=notifications=slotQueries=prepared=0;e.fixture_reset();}
 function op(n,ret=0,phase){write(error,'');assert.equal(e.updater_begin(n,error,512n),ret,str(error));assert.equal(resources.size,0,'network cleanup');assert.equal(handles.size,0,'file cleanup');if(!cfg.authRestoreFailure)assert.equal(privileged,false,'permissions not restored');assert.equal(e.fixture_state(0),0,'worker finished');if(phase)assert.equal(str(e.fixture_phase()),phase,str(e.fixture_message()));assert.equal(files.get('/data/pkg/existing-torrent/file00.pkg').toString(),'keep');checks++;}
 function available(options={}){reset(options);op(0,0,'available');assert.equal(e.fixture_state(1),1);assert.equal(notifications,1);}
 function ready(options={}){available(options);op(1,0,'ready');assert.equal(e.fixture_state(2),1);assert.equal(e.fixture_done(),BigInt(pkg.length));assert.deepEqual(files.get(packagePath),pkg);}
 for(const options of [{helperMissing:true},{launchFail:true},{noAck:true},{wrongAck:true},{requestFlushFailure:true},{authActivateFailure:true},{authRestoreFailure:true},{userId:-1}]){
  launched=closed=sleeps=0;available(options);op(1,0,'error');assert.equal(e.fixture_state(2),1);assert.deepEqual(files.get(packagePath),pkg);assert.equal(closed,0);
  if(options.noAck||options.wrongAck)assert.equal(sleeps,30);
  if(options.helperMissing||options.requestFlushFailure||options.authActivateFailure||options.userId)assert.equal(launched,0);
  if(options.launchFail)assert.equal(files.has('/data/harbor/pkg-updater/request'),false);
 }
 available();files.set('/data/harbor/pkg-updater/claimed',Buffer.from('existing independent task'));op(1,0,'error');assert.equal(closed,0);assert.equal(files.get('/data/harbor/pkg-updater/claimed').toString(),'existing independent task');
 available();files.set('/data/harbor/pkg-updater/request',Buffer.from('existing pending task'));op(1,0,'error');assert.equal(closed,0);assert.equal(files.get('/data/harbor/pkg-updater/request').toString(),'existing pending task');
 for(const options of [{payload:Buffer.from(pkg).fill(0,10000,10001)},{payload:pkg.subarray(0,100)},{noSpace:true}]){launched=closed=0;available(options);op(1,0,'error');assert.equal(launched,0);assert.equal(closed,0);}
 launched=closed=0;available();let exited=false;try{e.updater_begin(1,error,512n);}catch(ex){if(!(ex instanceof MainClosed))throw ex;exited=true;}
 assert.equal(exited,true);assert.equal(closed,1);assert.equal(launched,1);assert.deepEqual(files.get(packagePath),pkg);assert.equal(registered,0);assert.equal(started,0);assert.equal(prepared,0);
 assert.equal(WebAssembly.Module.imports(module).some(x=>/AppPrepareOverwrite|AppUnInstall|KillApp/.test(x.name)),false);
 console.log(`${checks+1} automatic-handoff checks passed (${installed.installedVersion} -> ${installed.candidateVersion}): download/hash/signature/identity, separate helper launch, durable nonce-bound request, credentials restored before own-process exit, failure keeps app open and PKG saved, no in-process installation/removal.`);
})().catch(error=>{console.error(error);process.exitCode=1;});

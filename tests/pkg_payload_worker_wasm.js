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
 assert.match(feed,/^https:\/\/raw\.githubusercontent\.com\/h1pNoise\/h1pNoise-ps4\/main\/releases\/pkg-payload-v3-test\/current\.h1p$/);
 const fields=['H1PNOISE-PS4-UPDATE-1','IV0000-HBRW00001_00-HARBORPS40000000',installed.candidateVersion,packageSfo,String(nextBuild),String(pkg.length),crypto.createHash('sha512').update(pkg).digest('hex'),url,'Atualização de teste'];
 const signed=(f=fields)=>{const body=Buffer.from(f.join('\n')+'\n');return Buffer.concat([crypto.sign(null,body,key),body]);};
 let installedPkg=Buffer.from(pkg);installedPkg.fill(0x44,installedPkg.length-1);
 for(let i=0;i<installedPkg.readUInt32BE(0x10);i++){const e=installedPkg.readUInt32BE(0x18)+i*32;if(installedPkg.readUInt32BE(e)!==0x1000)continue;const sfo=installedPkg.readUInt32BE(e+16),keys=sfo+installedPkg.readUInt32LE(sfo+8),values=sfo+installedPkg.readUInt32LE(sfo+12);for(let j=0;j<installedPkg.readUInt32LE(sfo+16);j++){const f=sfo+20+j*16,k=keys+installedPkg.readUInt16LE(f);if(installedPkg.toString("utf8",k,installedPkg.indexOf(0,k))==="APP_VER")installedPkg.write(installed.installedSfo+"\0",values+installedPkg.readUInt32LE(f+12));}}
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
 let nativeCalls=0,sleeps=0,fileModes,allocated,scheduleConfigured,initializedAuthorized=false;
 const attempt='/data/harbor/pkg-direct-attempt.h1p',copyPath=packagePath+'.payload-install.pkg',installedPath='/user/app/HBRW00001/app.pkg';
 env.mkdir=p=>{assert.equal(str(p),'/data/harbor');return 0;};
 env.access=p=>files.has(str(p))?0:-1;
 env.open=(p,flags,mode)=>{p=str(p);if(p==='/user'+copyPath){if(cfg.aliasDenied){put32(error,13);return -1;}assert.ok(files.has(copyPath));const id=++handleId;handles.set(BigInt(id),{path:copyPath,at:0});return id;}if(p==='/dev/gsched_is.ctl'){assert.ok(privileged);if(cfg.deviceDenied){put32(error,2);return -1;}const id=++handleId;handles.set(BigInt(id),{path:p,at:0});return id;}if(p.startsWith('/system/')){const id=++handleId;handles.set(BigInt(id),{path:p,at:0});return id;}if(files.has(p))return -1;if(cfg.copyOpenFail&&p===copyPath)return -1;files.set(p,Buffer.alloc(0));fileModes.set(p,v().getUint32(Number(mode),true)&~0o077);const id=++handleId;handles.set(BigInt(id),{path:p,at:0});return id;};
 env.close=id=>{assert.ok(handles.delete(BigInt(id)));return 0;};
 env.write=(id,p,n)=>{const h=handles.get(BigInt(id));if(h.path===copyPath)assert.ok(allocated.has(copyPath),'Only write after disk reservation');if(cfg.copyWriteFail&&h.path===copyPath)return -1n;const count=cfg.shortWrites?Math.min(32768,Number(n)):Number(n);files.set(h.path,Buffer.concat([files.get(h.path),Buffer.from(m().subarray(Number(p),Number(p)+count))]));return BigInt(count);};
 env.fileno=id=>Number(id);
 env.ftruncate=(id,size)=>{assert.ok(privileged);assert.equal(handles.get(BigInt(id)).path,copyPath);assert.equal(size,0n);assert.equal(files.get(copyPath).length,0);return cfg.truncateFail?-1:0;};
 env.ioctl=(id,cmd,args)=>{
  assert.ok(privileged,'allocation authorization');const a=Number(ptr(args)),h=handles.get(BigInt(id));
  if(cmd===0xC0209406n){assert.equal(h.path,'/dev/gsched_is.ctl');const target=v().getBigUint64(a,true);assert.equal(handles.get(target).path,copyPath);assert.equal(files.get(copyPath).length,0);assert.equal(v().getUint32(a+8,true),1);assert.equal(v().getUint32(a+12,true),7);assert.deepEqual(Buffer.from(m().subarray(a+16,a+32)),Buffer.alloc(16));if(cfg.scheduleFail)return -25;scheduleConfigured=true;return 0;}
  assert.equal(cmd,0xC02066A1n);assert.equal(h.path,copyPath);assert.ok(scheduleConfigured);assert.equal(files.get(copyPath).length,0,'allocate before writing');assert.equal(v().getBigUint64(a,true),BigInt(pkg.length));assert.equal(v().getBigUint64(a+8,true),0n);assert.equal(v().getBigUint64(a+16,true),0x80n);assert.equal(v().getBigUint64(a+24,true),0n);if(cfg.allocateFail)return -28;allocated.add(copyPath);return 0;
 };
 env.fsync=id=>cfg.copySyncFail&&handles.get(BigInt(id))?.path===copyPath||cfg.markerSyncFail&&handles.get(BigInt(id))?.path===attempt?-1:0;
 env.fseeko=(id,offset,mode)=>{const h=handles.get(id);h.at=(mode===2?files.get(h.path).length:mode===1?h.at:0)+Number(offset);return 0;};
 env.ftello=id=>BigInt(handles.get(id).at);
 env.fprintf=()=>0;
 env.fopen=(p,mode)=>{p=str(p);mode=str(mode);if(mode==='rb'&&!files.has(p))return 0n;if(mode==='wb'||mode==='a'&&!files.has(p))files.set(p,Buffer.alloc(0));const id=BigInt(++handleId);handles.set(id,{path:p,at:mode==='a'?files.get(p).length:0});return id;};
 env.remove=p=>{p=str(p);assert.ok(p.endsWith('.pkg.part')||p===copyPath||p===attempt,'Original PKG must not be deleted');files.delete(p);return 0;};
 env.sceAppInstUtilInitialize=()=>{assert.ok(privileged,'Initialize must share installation authorization');initializedAuthorized=true;return cfg.initFail?-123:0;};
 env.geteuid=()=>17;
 env.__errno_location=()=>error;
 env.fchmod=(id,mode)=>{const h=handles.get(BigInt(id));assert.equal(h.path,copyPath);assert.equal(mode,0o644);if(cfg.chmodFail)return -1;fileModes.set(h.path,mode);return 0;};
 env.sceKernelLoadStartModule=()=>-123n;
 env.sceAppInstUtilAppInstallPkg=(p,reserved)=>{
  assert.ok(privileged);assert.equal(str(p),'/user'+copyPath);assert.equal(reserved,0n);assert.deepEqual(files.get(copyPath),pkg);assert.deepEqual(files.get(attempt),signed());nativeCalls++;
  assert.ok(initializedAuthorized);assert.equal(fileModes.get(attempt),0o600,'Attempt must remain private');
  // Reproduce the EACCES seen on the PS4 when a different service UID cannot
  // read the installer copy. A restrictive creation umask must be corrected.
  if(!(fileModes.get(copyPath)&0o004))return 0x8002000D|0;
  assert.equal(fileModes.get(copyPath),0o644);assert.ok(allocated.has(copyPath),'Direct install needs allocated inode');
  if(cfg.installFail)return 0x8002000D|0;
  if(!cfg.unconfirmed){files.set(installedPath,Buffer.from(pkg));if(cfg.badInstalled)files.get(installedPath).fill(0x99,10000,10001);}
  if(cfg.consumeSource)files.delete(copyPath);return 0;
 };
 env.sleep_ms=ms=>{assert.equal(ms,500);sleeps++;};
 env.sceSystemServiceLaunchApp=()=>{throw Error('Must not launch helper');};
 env.sceSystemServiceLoadExec=()=>{throw Error('Must not restart app');};
 env._exit=()=>{throw Error('Must not close app');};
 const wasm=fs.readFileSync(process.argv[2]),module=await WebAssembly.compile(wasm);
 env.installer_with_access=(fn,error,cap)=>{assert.ok(privileged);return instance.exports.__indirect_function_table.get(Number(fn))(error,cap);};
 env.installer_with_permissions=(fn,arg,error,cap)=>{assert.ok(privileged);return instance.exports.__indirect_function_table.get(Number(fn))(arg,error,cap);};
 env.ps4_active_user=(user)=>{put32(user,0x10000000);return 0;};
 env.usleep=ms=>{assert.equal(ms,500000);sleeps++;};
 env.getpid=()=>cfg.samePid?1234:5000;
 env.kill=(pid,sig)=>{assert.equal(pid,1234);assert.equal(sig,0);if(cfg.parentUnknown){put32(error,13);return -1;}if(cfg.parentAlive)return 0;put32(error,3);return -1;};
 env.sceSystemServiceLaunchApp=(title,args,param)=>{assert.equal(str(title),'HBRW00001');assert.ok(nativeCalls);assert.deepEqual(files.get(installedPath),pkg);reopened++;return cfg.reopenFail?-123:0;};
 env.fsync=id=>cfg.copySyncFail&&handles.get(BigInt(id))?.path===copyPath?-1:0;
 env.access=p=>files.has(str(p))?0:-1;
 env.rename=(a,b)=>{a=str(a);b=str(b);assert.ok(files.has(a));files.set(b,files.get(a));files.delete(a);return 0;};
 env.remove=p=>{p=str(p);assert.ok([copyPath,root+'/claimed',root+'/ack'].includes(p),'Preserve original PKG');files.delete(p);return 0;};
 env.open=(p,flags,mode)=>{p=str(p);if(p==='/dev/gsched_is.ctl'){assert.ok(privileged);if(cfg.deviceDenied){put32(error,2);return -1;}}else {assert.equal(p,copyPath);if(files.has(p)||cfg.copyOpenFail)return -1;files.set(p,Buffer.alloc(0));fileModes.set(p,0o600);}
  const id=++handleId;handles.set(BigInt(id),{path:p,at:0});return id;};
 env.sceAppInstUtilAppInstallPkg=(p,reserved)=>{
  assert.equal(str(p),'/user'+copyPath);assert.equal(reserved,0n);assert.ok(privileged);assert.equal(cfg.parentAlive||cfg.parentUnknown||false,false);
  assert.ok(allocated.has(copyPath));assert.equal(fileModes.get(copyPath),0o644);assert.deepEqual(files.get(copyPath),pkg);assert.deepEqual(files.get(packagePath),pkg);nativeCalls++;
  if(cfg.installFail)return 0x8002000D|0;
  if(!cfg.unconfirmed){files.set(installedPath,Buffer.from(pkg));files.delete(copyPath);}return 0;
 };
 const root='/data/harbor/pkg-payload-v3-updater';let reopened=0;
 for(const imp of WebAssembly.Module.imports(module))assert.ok(env[imp.name],'Missing native payload test mock '+imp.name);
 instance=await WebAssembly.instantiate(module,{env});const e=instance.exports,base=Number(e.__heap_base.value),error=BigInt(base);let checks=0;
 function run(options={}){
  heap=base+2048;cfg=options;privileged=true;initializedAuthorized=false;fileModes=new Map();allocated=new Set();scheduleConfigured=false;
  files=new Map([[packagePath,Buffer.from(pkg)],[installedPath,Buffer.from(options.alreadyInstalled?pkg:installedPkg)],
   [root+'/request',Buffer.from('1234\n'+'11'.repeat(16)+'\n')],[root+'/manifest.h1p',signed()]]);
  handles=new Map();handleId=0;nativeCalls=sleeps=reopened=notifications=0;
  if(options.badHash)files.get(packagePath)[10000]^=1;
  if(options.badSignature)files.get(root+'/manifest.h1p')[0]^=1;
  if(options.claimed)files.set(root+'/claimed',Buffer.from('existing'));
  const rc=e.pkg_update_payload_main();assert.equal(handles.size,0);assert.deepEqual(files.get(packagePath),options.badHash?Buffer.from(pkg).map((x,i)=>i===10000?x^1:x):pkg);checks++;
  return rc;
 }
 assert.equal(run(),0);assert.equal(nativeCalls,1);assert.equal(reopened,1);assert.equal(files.has(root+'/claimed'),false);
 for(const options of [{samePid:true},{parentAlive:true},{parentUnknown:true},{badHash:true},{badSignature:true},{claimed:true},{alreadyInstalled:true},{deviceDenied:true},{truncateFail:true},{scheduleFail:true},{allocateFail:true},{copyOpenFail:true},{copyWriteFail:true},{copySyncFail:true},{chmodFail:true},{initFail:true}]){
  assert.equal(run(options),1,JSON.stringify(options));assert.equal(nativeCalls,0);assert.equal(reopened,0);
 }
 assert.equal(run({installFail:true}),1);assert.equal(nativeCalls,1);assert.equal(reopened,0);assert.ok(files.has(root+'/claimed'));
 assert.equal(run({unconfirmed:true}),1);assert.equal(nativeCalls,1);assert.equal(reopened,0);assert.ok(files.has(root+'/claimed'));assert.equal(sleeps,120);
 assert.equal(run({reopenFail:true}),0);assert.equal(nativeCalls,1);assert.equal(reopened,1);
 assert.equal(WebAssembly.Module.imports(module).some(x=>/PrepareOverwrite|UnInstall|_exit|LoadExec/.test(x.name)),false);
 console.log(`${checks} native payload worker checks passed: parent exit, independent PID, signed identity, full disk preparation, private copy permissions, confirmation before reopen, originals preserved, no removal of installed title.`);
})().catch(error=>{console.error(error);process.exitCode=1;});

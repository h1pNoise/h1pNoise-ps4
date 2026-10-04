// Execute production crypto, parser, worker, HTTPS redirects and BGFT ABI.
// Only libc/filesystem, threads and PS4 system APIs are mocked. No network access.
const fs=require('node:fs'),crypto=require('node:crypto'),assert=require('node:assert/strict');
(async()=>{
 const pkg=fs.readFileSync(process.argv[3]),key=crypto.createPrivateKey(fs.readFileSync(process.argv[4]));
 const packageSfo='00.40';
 const url='https://github.com/test/app/releases/download/v0.1.30/app.self',feed=process.argv[5];
 const fields=['H1PNOISE-PS4-RUNTIME-1','IV0000-HBRW00001_00-HARBORPS40000000','0.1.30',packageSfo,'40',String(pkg.length),crypto.createHash('sha512').update(pkg).digest('hex'),url,'Atualização de teste'];
 const signed=(f=fields)=>{const body=Buffer.from(f.join('\n')+'\n');return Buffer.concat([crypto.sign(null,body,key),body]);};
 let instance,heap,cfg,resources,headerLimits,files,handles,handleId,requests,requestUrl,bodyOffset,reply,clock,registered,started,notifications,userReady,privileged,savedCred,slotQueries,prepared,execPaths=[];
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
  mkdir:()=>0,fileno:id=>Number(id),fsync:()=>cfg.syncFail?-1:0,
  sceSystemServiceLoadExec:path=>{const s=str(path);assert.ok(['/app0/eboot.bin','/app0/h1pNoise.self','/data/harbor/runtime/h1pNoise-40.self'].includes(s));execPaths.push(s);return -123;},
  malloc:alloc,free:()=>{},time:()=>clock++,lock:()=>{},unlock:()=>{},pthread_detach:()=>0,sleep_ms:()=>{},
  thread_start:(out,fn,arg)=>{if(cfg.threadFail)return -1;put64(out,1);instance.exports.__indirect_function_table.get(Number(fn))(arg);return 0;},
  storage_check:(path,needed,error,cap)=>{assert.equal(str(path),'/data/harbor/runtime');assert.equal(needed,BigInt(pkg.length)+67108864n);if(cfg.noSpace){write(error,'Espaco insuficiente');return -1;}return 0;},
  fopen:(path,mode)=>{path=str(path);mode=str(mode);if(cfg.openFail)return 0n;if(mode==='rb'&&!files.has(path))return 0n;if(mode==='wb')files.set(path,Buffer.alloc(0));const id=BigInt(++handleId);handles.set(id,{path,at:0});return id;},
  fwrite:(data,size,count,id)=>{if(cfg.writeFail)return 0n;const h=handles.get(id),n=Number(size*count),b=Buffer.from(m().subarray(Number(data),Number(data)+n));files.set(h.path,Buffer.concat([files.get(h.path),b]));h.at+=n;return count;},
  fread:(data,size,count,id)=>{const h=handles.get(id),b=files.get(h.path),n=Math.min(Number(size*count),b.length-h.at);m().set(b.subarray(h.at,h.at+n),Number(data));h.at+=n;return BigInt(n)/size;},
  fflush:()=>cfg.flushFail?-1:0,ferror:()=>0,rewind:id=>{handles.get(id).at=0;},fclose:id=>{assert.ok(handles.delete(id));return 0;},
  remove:path=>{path=str(path);assert.ok(path.startsWith('/data/harbor/runtime/'),path);assert.ok(!path.endsWith('.self')&&!path.endsWith('.h1p'),'Do not remove verified files');files.delete(path);return 0;},
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
  ps4_installer_ready:()=>0,sceAppInstUtilGetTitleIdFromPkg:(path,out,isApp)=>{assert.equal(str(path),'/data/harbor/runtime/h1pNoise-40.self');write(out,cfg.wrongInstalledId?'OTHER0001':'HBRW00001');put32(isApp,cfg.isApp??1);return 0;},
  sceAppInstUtilGetPrimaryAppSlot:(title,out)=>{assert.ok(privileged);assert.equal(str(title),'HBRW00001');slotQueries++;put32(out,cfg.slot??0);return cfg.slotFailure?-123:0;},
  sceAppInstUtilAppPrepareOverwritePkg:path=>{assert.ok(privileged);assert.equal(str(path),'/data/harbor/runtime/h1pNoise-40.self');prepared++;return cfg.prepareFailure?-123:0;},
  sceUserServiceInitialize:params=>{assert.equal(v().getUint32(Number(params),true),0x2bc);const rc=cfg.userInitResult||0;userReady=!rc||(rc>>>0)===0x80960003;return rc;},
  sceUserServiceGetForegroundUser:out=>{if(!userReady)return 0x80960002|0;put32(out,cfg.userId??0x10000000);return 0;},
  sceBgftServiceIntDownloadRegisterTaskByStorageEx:(p,out)=>{
   assert.ok(privileged,'BGFT registration lost permissions');
   registered++;p=Number(p);assert.equal(v().getInt32(p,true),0x10000000);assert.equal(v().getInt32(p+4,true),5);
   assert.equal(str(ptr(p+8)),fields[1]);assert.equal(str(ptr(p+16)),'/user/data/pkg/h1pNoise-update-20.pkg');
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
 instance=await WebAssembly.instantiate(module,{env:api});const e=instance.exports,base=Number(e.__heap_base.value),error=BigInt(base);let checks=0;
 function reset(options={}){heap=base+2048;cfg={manifest:signed(),payload:pkg,...options};privileged=false;userReady=false;resources=new Set();headerLimits=new Map();files=new Map([['/data/pkg/existing-torrent/file00.pkg',Buffer.from('keep')]]);handles=new Map();handleId=0;requests=[];clock=1000n;registered=started=notifications=slotQueries=prepared=0;e.fixture_reset();}
 function op(n,ret=0,phase){write(error,'');assert.equal(e.updater_begin(n,error,512n),ret,str(error));assert.equal(resources.size,0,'network cleanup');assert.equal(handles.size,0,'file cleanup');if(!cfg.authRestoreFailure)assert.equal(privileged,false,'permissions not restored');assert.equal(e.fixture_state(0),0,'worker finished');if(phase)assert.equal(str(e.fixture_phase()),phase,str(e.fixture_message()));assert.equal(files.get('/data/pkg/existing-torrent/file00.pkg').toString(),'keep');checks++;}
 function available(options={}){reset(options);op(0,0,'available');assert.equal(e.fixture_state(1),1);assert.equal(notifications,1);}
 function ready(options={}){available(options);op(1,0,'error');assert.equal(e.fixture_state(2),1);assert.equal(e.fixture_done(),BigInt(pkg.length));assert.deepEqual(files.get('/data/harbor/runtime/h1pNoise-40.self'),pkg);}
 ready();assert.match(str(e.fixture_message()),/reinicio falhou/);assert.equal(registered,0);assert.equal(prepared,0);assert.equal(files.get('/data/harbor/runtime/pending').toString(),'40');
 const selection=alloc(700);
 assert.equal(e.runtime_select(selection),1);assert.equal(str(selection),'/data/harbor/runtime/h1pNoise-40.self');
 assert.equal(files.get('/data/harbor/runtime/attempt').toString(),'40');
 assert.equal(e.runtime_select(selection),0,'Failed startup must select bundled fallback');assert.equal(str(selection),'/app0/h1pNoise.self');
 assert.equal(e.runtime_boot_confirm(39),0);assert.equal(files.has('/data/harbor/runtime/active'),false,'Old version must not confirm candidate');
 assert.equal(e.runtime_boot_confirm(40),0);assert.equal(files.get('/data/harbor/runtime/active').toString(),'40');assert.equal(files.has('/data/harbor/runtime/pending'),false);
 assert.equal(e.runtime_select(selection),2);assert.equal(str(selection),'/data/harbor/runtime/h1pNoise-40.self');
 const nextFields=fields.map((s,i)=>i===4?'41':s);files.set('/data/harbor/runtime/h1pNoise-41.h1p',signed(nextFields));files.set('/data/harbor/runtime/h1pNoise-41.self',Buffer.from(pkg));
 assert.equal(e.runtime_activate(41,error,512n),0);assert.equal(files.get('/data/harbor/runtime/active').toString(),'40');
 assert.equal(e.runtime_select(selection),1);assert.equal(str(selection),'/data/harbor/runtime/h1pNoise-41.self');
 assert.equal(e.runtime_select(selection),2,'Failed second update must retain last confirmed version');assert.equal(str(selection),'/data/harbor/runtime/h1pNoise-40.self');assert.deepEqual(files.get('/data/harbor/runtime/h1pNoise-40.self'),pkg);
 files.get('/data/harbor/runtime/h1pNoise-40.self')[10000]^=1;assert.equal(e.runtime_select(selection),0,'Corrupt active must fall back');
 for(const failure of [{syncFail:true},{flushFail:true},{renameFail:true}]){ready();Object.assign(cfg,failure);assert.equal(e.runtime_select(selection),0,'Cannot mark attempt: must not launch candidate');checks++;}
 ready();files.get('/data/harbor/runtime/h1pNoise-40.h1p')[0]^=1;assert.equal(e.runtime_select(selection),0,'Forged manifest must not launch');
 ready();files.set('/data/harbor/runtime/pending',Buffer.from('../game'));assert.equal(e.runtime_select(selection),0,'Pointer traversal refused');
 ready();files.set('/data/harbor/runtime/pending',Buffer.from('2147483648'));assert.equal(e.runtime_select(selection),0,'Pointer overflow refused');
 ready();files.set('/data/harbor/runtime/pending',Buffer.from('9999999999999999999999'));assert.equal(e.runtime_select(selection),0,'Long pointer refused');
 ready();files.set('/data/harbor/runtime/pending',Buffer.from('39'));assert.equal(e.runtime_select(selection),0,'Bundled baseline is not a candidate');
 ready();execPaths=[];assert.equal(e.main(0,0n),1);assert.deepEqual(execPaths,['/data/harbor/runtime/h1pNoise-40.self','/app0/h1pNoise.self'],'Rejected candidate launch must fall back to installed package');assert.deepEqual(files.get('/data/harbor/runtime/h1pNoise-40.self'),pkg);
 ready();op(2,0,'error');assert.equal(registered,0);assert.equal(prepared,0);assert.deepEqual(files.get('/data/harbor/runtime/h1pNoise-40.self'),pkg);
 available();op(0,0,'available');assert.equal(notifications,1);op(2,-1,'available');assert.equal(registered,0);
 reset();op(1,-1);e.fixture_busy(1);op(0,-1);e.fixture_busy(0);
 reset({threadFail:true});op(0,-1,'error');
 for(const build of ['18','19']){reset({manifest:signed(fields.map((s,i)=>i===4?build:s))});op(0,0,build==='18'?'channel-old':'current');assert.equal(e.fixture_state(1),0);}
 for(const [index,value] of [[0,'OTHER'],[1,'OTHER'],[2,'x'],[3,'1.2'],[3,'00.19'],[4,'0'],[4,'2147483648'],[5,'8191'],[5,'999999999999999999999999'],[6,'g'.repeat(128)],[7,'http://github.com/file'],[7,'https://github.com@evil.example/file'],[7,'https://github.com/file%GG'],[8,'x'.repeat(768)],[8,'bad\rtext']]){reset({manifest:signed(fields.map((s,i)=>i===index?value:s))});op(0,0,'error');assert.equal(e.fixture_state(1),0);}
 for(const changed of [(()=>{const b=signed();b[0]^=1;return b;})(),Buffer.alloc(64),Buffer.alloc(4097),signed([...fields,'extra'])]){reset({manifest:changed});op(0,0,'error');}
 for(const options of [{status:404},{status:403},{status:206},{readFail:true},...creators.map(fail=>({fail})),{fail:'sceHttpSetAutoRedirect'},{fail:'sceHttpSetResponseHeaderMaxSize'},{fail:'sceHttpSendRequest'},{fail:'sceHttpSetRecvTimeOut'}]){reset(options);op(0,0,'error');}
 // Reproduce GitHub's response > the PS4 default 5000 bytes; a larger bounded
 // setting must be inherited by every redirect request, not set after receipt.
 const asset='https://release-assets.githubusercontent.com/test/package';
 const largeHeaders=(dest,n)=>{const h='Location: '+dest+'\r\nX-Security: ';return h+'x'.repeat(n-Buffer.byteLength(h)-2)+'\r\n';};
 ready({redirect:u=>u===url?{status:302,headers:largeHeaders(asset,5226)}:u===asset?{status:200,body:pkg}:undefined});
 assert.equal(requests.at(-1),asset);
 const feedRedirect='https://raw.githubusercontent.com/h1pNoise/h1pNoise-ps4/main/releases/runtime/redirect-test.h1p';
 available({redirect:u=>u===feed?{status:302,headers:largeHeaders(feedRedirect,32768)}:{status:200,body:signed()}});
 for(const ignoreHeaderLimit of [false,true]){reset({ignoreHeaderLimit,redirect:()=>({status:302,headers:largeHeaders(asset,32769)})});op(0,0,'error');assert.match(str(e.fixture_message()),/limite de cabecalhos/);assert.equal(requests.length,1);}
 for(const [sendError,message] of [[0x80431073,'limite de cabecalhos'],[0x80431075,'ligacao HTTPS']]){reset({sendError});op(0,0,'error');assert.ok(str(e.fixture_message()).includes(message));}
 available({redirect:u=>u===feed?{status:302,location:feedRedirect}:{status:200,body:signed()}});
 available({redirect:u=>u===feed?{status:302,location:'/test/app/releases/download/v0.1.10/update.h1p'}:{status:200,body:signed()}});
 for(const location of ['http://github.com/insecure','https://github.com.evil.example/update','//evil.example/update','https://evil.example/update','https://release-assets.githubusercontent.com/test/feed']){reset({redirect:()=>({status:302,location})});op(0,0,'error');assert.equal(requests.length,1);}
 reset({redirect:()=>({status:302,location:feed})});op(0,0,'error');assert.equal(requests.length,6);
 reset({redirect:()=>({status:302,headers:'Location: '+feed+'\r\nLocation: '+feed+'\r\n'})});op(0,0,'error');
 for(const options of [{noSpace:true},{openFail:true},{writeFail:true},{flushFail:true},{syncFail:true},{renameFail:true},{payload:pkg.subarray(0,100)},{payload:Buffer.concat([pkg,Buffer.from([0])])},{payload:Buffer.from(pkg).fill(0,10000,10001)}]){available(options);op(1,0,'error');assert.equal(e.fixture_state(2),0);assert.equal(registered,0);assert.equal(files.has('/data/harbor/runtime/h1pNoise-40.self'),!!options.syncFail);assert.equal(files.has('/data/harbor/runtime/pending'),false);}
 for(const offset of [0,6,7,0x10]){const b=Buffer.from(pkg);b[offset]^=1;const f=[...fields];f[6]=crypto.createHash('sha512').update(b).digest('hex');available({payload:b,manifest:signed(f)});op(1,0,'error');assert.equal(e.fixture_state(2),0);}
 ready();files.get('/data/harbor/runtime/h1pNoise-40.self')[10000]^=1;op(2,0,'error');assert.equal(e.fixture_state(2),0);assert.equal(registered,0);op(2,-1);
 assert.equal(WebAssembly.Module.imports(module).some(x=>/AppPrepareOverwrite|AppUnInstall|GetPrimaryAppSlot|GetTitleIdFromPkg/.test(x.name)),false,'Updater must not import native replacement/removal APIs');
 console.log(`${checks} updater checks passed: real Ed25519/SHA-512, bounded signed metadata, HTTPS redirect allowlist/cleanup, streamed file validation, busy/duplicate guards, re-verification, signed runtime activation, startup attempt fallback, confirmed-version retention, saved PKG preservation and failure recovery.`);
})().catch(error=>{console.error(error);process.exitCode=1;});

// Execute the real PS4 queue code in wasm64, preserving its 64-bit ABI/layout.
// Only PS4 system calls and libc are supplied as mocks; no real downloads/installations.
const fs=require('node:fs'),assert=require('node:assert/strict');
(async()=>{
 let instance,cfg,reads,registered,started,resources,offset,clock,registrationKind,userReady,userQueries,privileged,savedCred,authCalls;
 const m=()=>new Uint8Array(instance.exports.memory.buffer),v=()=>new DataView(m().buffer);
 const str=p=>{p=Number(p);let e=p;while(m()[e])e++;return Buffer.from(m().subarray(p,e)).toString();};
 const write=(p,s)=>m().set(Buffer.from(s+'\0'),Number(p));
 const put32=(p,n)=>v().setInt32(Number(p),n,true),put64=(p,n)=>v().setBigUint64(Number(p),BigInt(n),true);
 let headerPointer;
 const env={
  pthread_mutex_lock:()=>0,pthread_mutex_trylock:()=>0,pthread_mutex_unlock:()=>0,
  strlen:p=>BigInt(Buffer.byteLength(str(p))),
  memset:(p,c,n)=>{m().fill(c,Number(p),Number(p)+Number(n));return p;},
  memcpy:(d,s,n)=>{m().copyWithin(Number(d),Number(s),Number(s)+Number(n));return d;},
  open:()=>{throw Error('Unexpected installer initialization');},close:()=>0,
  jbc_jailbreak_cred:()=>{throw Error('Unexpected filesystem root change');},jbc_set_cred:()=>{throw Error('Unexpected filesystem root change');},jbc_resolve_error:()=>0,
  jbc_get_cred:p=>{
   m().fill(17,Number(p),Number(p)+80);put64(p+56n,0x3800000000000011n);put64(p+64n,0x1000n);put64(p+72n,0x2000n);
   savedCred=Buffer.from(m().subarray(Number(p),Number(p)+80));return 0;
  },
  jbc_set_auth:p=>{
   p=Number(p);authCalls++;assert.deepEqual(Buffer.from(m().subarray(p,p+56)),savedCred.subarray(0,56),'filesystem/UID changed');
   const paid=v().getBigUint64(p+56,true);privileged=paid===0x3800000000000010n;
   assert.equal(paid,privileged?0x3800000000000010n:0x3800000000000011n);
   assert.equal(v().getBigUint64(p+64,true),privileged?(0x1000n|(1n<<62n)):0x1000n);assert.equal(v().getBigUint64(p+72,true),0x2000n);
   if(privileged&&cfg.authActivateFailure)return -1;
   if(!privileged&&cfg.authRestoreFailure){privileged=true;return -1;}
   return 0;
  },
  memcmp:(a,b,n)=>{a=Number(a);b=Number(b);for(let i=0;i<Number(n);i++)if(m()[a+i]!==m()[b+i])return m()[a+i]-m()[b+i];return 0;},
  memchr:(p,c,n)=>{for(let i=0;i<Number(n);i++)if(m()[Number(p)+i]===c)return p+BigInt(i);return 0n;},
  snprintf:(out,cap,fmt,args)=>{let s=str(fmt);if(s==='%s')s=str(v().getBigUint64(Number(args),true));write(out,s.slice(0,Number(cap)-1));return s.length;},
  time:()=>clock++,
  sceSysmoduleLoadModuleInternal:()=>0,
  sceHttpSetAutoRedirect:(_id,enabled)=>{assert.equal(enabled,0);return 0;},
  sceHttpGetStatusCode:(_id,out)=>{put32(out,cfg.status);return 0;},
  sceHttpGetAllResponseHeaders:(_id,ptr,len)=>{write(headerPointer,cfg.headers);put64(ptr,headerPointer);put64(len,Buffer.byteLength(cfg.headers));return 0;},
  sceHttpReadData:(_id,dest,n)=>{assert.ok(n<=8192);reads++;if(cfg.truncated)return 0;const count=Math.min(n,1024,cfg.header.length-offset);m().set(cfg.header.subarray(offset,offset+count),Number(dest));offset+=count;return count;},
  sceAppInstUtilAppExists:(title,out)=>{assert.equal(str(title),'HBRW00001');put32(out,cfg.exists);return 0;},
  sceUserServiceInitialize:params=>{
   assert.equal(v().getUint32(Number(params),true),0x2bc);
   const rc=cfg.userInitResult||0;
   userReady=rc===0||(rc>>>0)===0x80960003;
   return rc;
  },
  sceUserServiceGetForegroundUser:out=>{
   userQueries++;if(!userReady)return 0x80960002|0;
   put32(out,cfg.userId??0x10000000);return cfg.userQueryResult||0;
  },
  sceBgftServiceDownloadStartTask:id=>{assert.equal(id,42);assert.ok(privileged,'BGFT start lost permissions');started++;return 0;},
 };
 for(const fn of ['ps4_installer_ready','sceHttpSetConnectTimeOut','sceHttpSetResolveTimeOut','sceHttpSetRecvTimeOut','sceHttpSetSendTimeOut','sceHttpSendRequest'])env[fn]=()=>0;
 env.sceHttpAddRequestHeader=(_id,key,val)=>{assert.ok(['Range','Accept-Encoding'].includes(str(key)));if(str(key)==='Range')assert.equal(str(val),'bytes=0-8191');return 0;};
 const creators=['sceNetPoolCreate','sceSslInit','sceHttpInit','sceHttpCreateTemplate','sceHttpCreateConnectionWithURL','sceHttpCreateRequestWithURL'];
 creators.forEach((name,i)=>env[name]=(...args)=>{if(name==='sceHttpCreateConnectionWithURL'||name==='sceHttpCreateRequestWithURL')assert.equal(str(args[name==='sceHttpCreateRequestWithURL'?2:1]),cfg.url);resources.add(i+1);return i+1;});
 for(const name of ['sceNetPoolDestroy','sceSslTerm','sceHttpTerm','sceHttpDeleteTemplate','sceHttpDeleteConnection','sceHttpDeleteRequest'])env[name]=id=>{assert.ok(resources.delete(id),name+': invalid cleanup');return 0;};
 function register(kind,p,out){
  assert.ok(privileged,'BGFT registration lost permissions');
  registered++;registrationKind=kind;p=Number(p);assert.equal(v().getInt32(p,true),0x10000000);
  assert.equal(str(v().getBigUint64(p+8,true)),'IV0000-HBRW00001_00-HARBORPS40000000');
  const submitted=str(v().getBigUint64(p+16,true));
  assert.equal(submitted,cfg.url+(/\.(pkg|PKG)$/.test(cfg.url)?'':'#content.pkg'));
  assert.equal(require('node:url').urlToHttpOptions(new URL(submitted)).path,require('node:url').urlToHttpOptions(new URL(cfg.url)).path,'BGFT changed the server request');
  assert.equal(v().getUint32(p+56,true),0x10000);assert.equal(str(v().getBigUint64(p+80,true)),'PS4GD');
  assert.equal(v().getBigUint64(p+96,true),68719476736n);if(cfg.registerResult)return cfg.registerResult;put32(out,cfg.invalidTask?-1:42);return 0;
 }
 env.sceBgftServiceIntDownloadRegisterTask=(p,out)=>register('base',p,out);
 env.sceBgftServiceIntDebugDownloadRegisterPkg=(p,out)=>register('patch',p,out);
 env.sceBgftServiceIntDownloadRegisterTaskByStorageEx=()=>{throw Error('Unexpected local storage registration');};
 // Error injection wraps the same production call site and preserves observation counters.
 const api={};for(const [name,fn] of Object.entries(env))api[name]=(...args)=>{
  if(cfg.fail===name){if(name==='sceBgftServiceDownloadStartTask')started++;return -123;}
  return fn(...args);
 };
 ({instance}=await WebAssembly.instantiate(fs.readFileSync(process.argv[2]),{env:api}));
 const e=instance.exports,base=e.__heap_base.value;headerPointer=base+4096n;
 const task=base+3000n,error=base+3100n,real=fs.readFileSync(process.argv[3]).subarray(0,8192);
 let checks=0;
 function run(options={},expected=0){
  cfg={header:Buffer.from(real),headers:'Content-Range: bytes 0-8191/68719476736\r\n',status:206,exists:0,url:'https://example.org/download?token=A%2Bb',...options};
  cfg.header.writeBigUInt64BE(68719476736n,0x430);if(options.patch)cfg.header.writeUInt32BE(0x00100000,0x78);if(options.html)cfg.header.write('<html>');
  reads=registered=started=offset=userQueries=authCalls=0;privileged=false;userReady=false;clock=1000n;resources=new Set();registrationKind='';write(error,'');put32(task,-1);write(base,cfg.url);
  assert.equal(e.platform_queue_pkg(base,task,error,512n),expected,str(error));assert.equal(resources.size,0,'resources leaked');if(!cfg.authRestoreFailure)assert.equal(privileged,false,'permissions not restored');checks++;
  return {task:v().getInt32(Number(task),true),error:str(error)};
 }
 assert.equal(run().task,42);assert.equal(registered,1);assert.equal(started,1);assert.equal(offset,8192);assert.equal(registrationKind,'base');
 run({patch:true,exists:1});assert.equal(registrationKind,'patch');
 for(const url of ['https://example.org/app.pkg','https://example.org/app.PKG','https://example.org/app.pkg?token=A%2Fb%3D+z&expires=1791011577']){run({url});assert.equal(registered,1);assert.equal(started,1);}
 for(const url of ['https://example.org/'+ 'x'.repeat(2020),'https://example.org/app.pkg#wrong']){run({url},-1);assert.equal(reads,0);assert.equal(registered,0);assert.equal(authCalls,0);}
 run({userInitResult:0x80960003|0});assert.equal(registered,1);assert.equal(userQueries,1);
 const initFailure=run({userInitResult:0x80960004|0},-1);assert.match(initFailure.error,/iniciar o servico/);assert.equal(userQueries,0);assert.equal(registered,0);
 for(const userId of [-1,0xff]){run({userId},-1);assert.equal(registered,0);assert.equal(started,0);}
 const notLoggedIn=run({userQueryResult:0x80960009|0},-1);assert.match(notLoggedIn.error,/Seleciona/);assert.equal(registered,0);
 const notInitialized=run({userQueryResult:0x80960002|0},-1);assert.match(notInitialized.error,/consultar/);assert.doesNotMatch(notInitialized.error,/Seleciona/);assert.equal(registered,0);
 for(const options of [{status:302},{status:403},{status:200},{headers:'Content-Range: bytes 0-8191/9000\r\n'},{truncated:true},{html:true},{exists:1},{patch:true,exists:0}]){run(options,-1);assert.equal(registered,0);assert.equal(started,0);}
 for(const fail of [...creators,'sceHttpSetAutoRedirect','sceHttpSetRecvTimeOut','sceHttpSendRequest','sceHttpGetAllResponseHeaders','sceUserServiceInitialize','sceUserServiceGetForegroundUser','ps4_installer_ready','sceBgftServiceIntDownloadRegisterTask']){run({fail},-1);assert.equal(started,0);}
 run({invalidTask:true},-1);assert.equal(started,0);
 for(const registerResult of [0x80990007,0x80990015,0x80990039]){const r=run({registerResult:registerResult|0},-1);assert.equal(r.task,-1);assert.equal(started,0);assert.equal(authCalls,2);if(registerResult===0x80990007){assert.match(r.error,/permissoes/);assert.doesNotMatch(r.error,/espaco/);}}
 run({authActivateFailure:true},-1);assert.equal(registered,0);assert.equal(authCalls,2);
 run({fail:'jbc_get_cred'},-1);assert.equal(registered,0);assert.equal(authCalls,0);
 const restoration=run({authRestoreFailure:true},-1);assert.equal(restoration.task,42);assert.equal(registered,1);assert.equal(started,1);assert.match(restoration.error,/restaurar/);
 const result=run({fail:'sceBgftServiceDownloadStartTask'},-1);assert.equal(result.task,42);assert.match(result.error,/registado, mas nao iniciou/);assert.equal(registered,1);assert.equal(started,1);
 console.log(`${checks} PS4 queue checks passed: direct URL/64-bit ABI, range/header failures, cleanup, installed-title guards, patch registration, register/start failures.`);
})().catch(e=>{console.error(e);process.exitCode=1;});

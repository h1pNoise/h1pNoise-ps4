// Execute the real PS4 queue code in wasm64, preserving its 64-bit ABI/layout.
// Only PS4 system calls and libc are supplied as mocks; no real downloads/installations.
const fs=require('node:fs'),assert=require('node:assert/strict');
(async()=>{
 let instance,cfg,reads,registered,started,resources,offset,clock,registrationKind;
 const m=()=>new Uint8Array(instance.exports.memory.buffer),v=()=>new DataView(m().buffer);
 const str=p=>{p=Number(p);let e=p;while(m()[e])e++;return Buffer.from(m().subarray(p,e)).toString();};
 const write=(p,s)=>m().set(Buffer.from(s+'\0'),Number(p));
 const put32=(p,n)=>v().setInt32(Number(p),n,true),put64=(p,n)=>v().setBigUint64(Number(p),BigInt(n),true);
 let headerPointer;
 const env={
  memset:(p,c,n)=>{m().fill(c,Number(p),Number(p)+Number(n));return p;},
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
  sceUserServiceGetForegroundUser:out=>{put32(out,0x10000000);return 0;},
  sceBgftServiceDownloadStartTask:id=>{assert.equal(id,42);started++;return 0;},
 };
 for(const fn of ['ps4_installer_ready','sceHttpSetConnectTimeOut','sceHttpSetResolveTimeOut','sceHttpSetRecvTimeOut','sceHttpSetSendTimeOut','sceHttpSendRequest'])env[fn]=()=>0;
 env.sceHttpAddRequestHeader=(_id,key,val)=>{assert.ok(['Range','Accept-Encoding'].includes(str(key)));if(str(key)==='Range')assert.equal(str(val),'bytes=0-8191');return 0;};
 const creators=['sceNetPoolCreate','sceSslInit','sceHttpInit','sceHttpCreateTemplate','sceHttpCreateConnectionWithURL','sceHttpCreateRequestWithURL'];
 creators.forEach((name,i)=>env[name]=(...args)=>{if(name==='sceHttpCreateConnectionWithURL'||name==='sceHttpCreateRequestWithURL')assert.equal(str(args[name==='sceHttpCreateRequestWithURL'?2:1]),'https://example.org/download?token=A%2Bb');resources.add(i+1);return i+1;});
 for(const name of ['sceNetPoolDestroy','sceSslTerm','sceHttpTerm','sceHttpDeleteTemplate','sceHttpDeleteConnection','sceHttpDeleteRequest'])env[name]=id=>{assert.ok(resources.delete(id),name+': invalid cleanup');return 0;};
 function register(kind,p,out){
  registered++;registrationKind=kind;p=Number(p);assert.equal(v().getInt32(p,true),0x10000000);
  assert.equal(str(v().getBigUint64(p+8,true)),'IV0000-HBRW00001_00-HARBORPS40000000');
  assert.equal(str(v().getBigUint64(p+16,true)),'https://example.org/download?token=A%2Bb');
  assert.equal(v().getUint32(p+56,true),0x10000);assert.equal(str(v().getBigUint64(p+80,true)),'PS4GD');
  assert.equal(v().getBigUint64(p+96,true),68719476736n);put32(out,cfg.invalidTask?-1:42);return 0;
 }
 env.sceBgftServiceIntDownloadRegisterTask=(p,out)=>register('base',p,out);
 env.sceBgftServiceIntDebugDownloadRegisterPkg=(p,out)=>register('patch',p,out);
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
  cfg={header:Buffer.from(real),headers:'Content-Range: bytes 0-8191/68719476736\r\n',status:206,exists:0,...options};
  cfg.header.writeBigUInt64BE(68719476736n,0x430);if(options.patch)cfg.header.writeUInt32BE(0x00100000,0x78);if(options.html)cfg.header.write('<html>');
  reads=registered=started=offset=0;clock=1000n;resources=new Set();registrationKind='';write(error,'');put32(task,-1);write(base,'https://example.org/download?token=A%2Bb');
  assert.equal(e.platform_queue_pkg(base,task,error,512n),expected,str(error));assert.equal(resources.size,0,'resources leaked');checks++;
  return {task:v().getInt32(Number(task),true),error:str(error)};
 }
 assert.equal(run().task,42);assert.equal(registered,1);assert.equal(started,1);assert.equal(offset,8192);assert.equal(registrationKind,'base');
 run({patch:true,exists:1});assert.equal(registrationKind,'patch');
 for(const options of [{status:302},{status:403},{status:200},{headers:'Content-Range: bytes 0-8191/9000\r\n'},{truncated:true},{html:true},{exists:1},{patch:true,exists:0}]){run(options,-1);assert.equal(registered,0);assert.equal(started,0);}
 for(const fail of [...creators,'sceHttpSetAutoRedirect','sceHttpSetRecvTimeOut','sceHttpSendRequest','sceHttpGetAllResponseHeaders','sceUserServiceGetForegroundUser','ps4_installer_ready','sceBgftServiceIntDownloadRegisterTask']){run({fail},-1);assert.equal(started,0);}
 run({invalidTask:true},-1);assert.equal(started,0);
 const result=run({fail:'sceBgftServiceDownloadStartTask'},-1);assert.equal(result.task,42);assert.match(result.error,/registado, mas nao iniciou/);assert.equal(registered,1);assert.equal(started,1);
 console.log(`${checks} PS4 queue checks passed: direct URL/64-bit ABI, range/header failures, cleanup, installed-title guards, patch registration, register/start failures.`);
})().catch(e=>{console.error(e);process.exitCode=1;});

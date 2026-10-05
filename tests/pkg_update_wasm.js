// Production signature/parser/file validation/transaction. Only OS operations mocked.
const fs=require('node:fs'),crypto=require('node:crypto'),assert=require('node:assert/strict');
(async()=>{
 const pkg=fs.readFileSync(process.argv[3]),keys=crypto.generateKeyPairSync('ed25519');
 const publicKey=keys.publicKey.export({type:'spki',format:'der'}).subarray(-32);
 const fields=['H1PNOISE-PS4-UPDATE-1','IV0000-HBRW00001_00-HARBORPS40000000','0.1.32','00.42','42',String(pkg.length),crypto.createHash('sha512').update(pkg).digest('hex'),'https://github.com/test/app.pkg','Test'];
 const sign=(f=fields)=>{const b=Buffer.from(f.join('\n')+'\n');return Buffer.concat([crypto.sign(null,b,keys.privateKey),b]);};
 const request='1234\n'+'0123456789abcdef'.repeat(2)+'\n',path='/data/pkg/h1pNoise-update-42.pkg';
 let instance,heap,cfg,files,handles,nextHandle,calls,waits,aliveCalls,sfoCalls,confirmCalls,checks=0;
 const mem=()=>new Uint8Array(instance.exports.memory.buffer),view=()=>new DataView(mem().buffer);
 const string=p=>{p=Number(p);let end=p;while(mem()[end])end++;return Buffer.from(mem().subarray(p,end)).toString();};
 const write=(p,s)=>mem().set(Buffer.from(s+'\0'),Number(p));
 const pointer=p=>view().getBigUint64(Number(p),true);
 function alloc(size){size=Number(size);const p=heap;heap=(heap+size+15)&~15;if(heap>mem().length)instance.exports.memory.grow(BigInt(Math.ceil((heap-mem().length)/65536)));return BigInt(p);}
 function fmt(s,args){let p=Number(args);return s.replace(/%([0-9]*)([sduXx])/g,(_,width,t)=>{let x=t==='s'?string(pointer(p)):t==='d'?view().getInt32(p,true):view().getUint32(p,true);p+=8;return (t==='x'||t==='X'?x.toString(16):String(x)).padStart(Number(width)||0,'0');});}
 const env={
  memcpy:(d,s,n)=>{mem().copyWithin(Number(d),Number(s),Number(s)+Number(n));return d;},
  memmove:(d,s,n)=>{mem().copyWithin(Number(d),Number(s),Number(s)+Number(n));return d;},
  memset:(d,c,n)=>{mem().fill(c,Number(d),Number(d)+Number(n));return d;},
  memcmp:(a,b,n)=>{for(let i=0;i<Number(n);i++){const d=mem()[Number(a)+i]-mem()[Number(b)+i];if(d)return d;}return 0;},
  memchr:(p,c,n)=>{for(let i=0;i<Number(n);i++)if(mem()[Number(p)+i]===c)return p+BigInt(i);return 0n;},
  strlen:p=>BigInt(Buffer.byteLength(string(p))),strcmp:(a,b)=>string(a).localeCompare(string(b)),
  strncmp:(a,b,n)=>string(a).slice(0,Number(n)).localeCompare(string(b).slice(0,Number(n))),
  strcpy:(d,s)=>{write(d,string(s));return d;},
  snprintf:(d,n,s,args)=>{const b=Buffer.from(fmt(string(s),args));mem().set(b.subarray(0,Number(n)-1),Number(d));mem()[Number(d)+Math.min(b.length,Number(n)-1)]=0;return b.length;},
  malloc:n=>cfg.noMemory?0n:alloc(n),free:()=>{},
  fopen:(p,mode)=>{assert.equal(string(mode),'rb','No file may be changed or deleted');p=string(p);if(cfg.openFail||!files.has(p))return 0n;const id=BigInt(++nextHandle);handles.set(id,{path:p,at:0});return id;},
  fread:(p,size,count,id)=>{const h=handles.get(id),b=files.get(h.path),n=Math.min(Number(size*count),b.length-h.at);mem().set(b.subarray(h.at,h.at+n),Number(p));h.at+=n;return BigInt(n)/size;},
  fclose:id=>{assert.ok(handles.delete(id));return 0;},ferror:()=>cfg.readError?1:0,rewind:id=>{handles.get(id).at=0;},
  helper_installed_sfo:(p,error)=>{calls.push('sfo');if(cfg.sfoFail){write(error,'Unknown installed version');return -1;}write(p,(cfg.sfos||['00.41'])[Math.min(sfoCalls++,(cfg.sfos||['00.41']).length-1)]);return 0;},
  helper_alive:pid=>{assert.equal(pid,1234);calls.push('alive');aliveCalls++;return cfg.aliveUnknown?-1:cfg.alwaysAlive?1:aliveCalls<=(cfg.aliveFor||0)?1:0;},
  helper_wait:ms=>{assert.equal(ms,500);waits++;},
  helper_ack:nonce=>{assert.equal(string(nonce),'0123456789abcdef'.repeat(2));assert.ok(calls.includes('alive'));assert.equal(cfg.aliveUnknown,false);calls.push('ack');if(cfg.changedAfterAck)files.get(path)[10000]^=1;return cfg.ackFail?-1:0;},
  helper_install:(p,error)=>{assert.equal(string(p),path);assert.ok(calls.includes('ack'));assert.ok(calls.includes('alive'));assert.equal(cfg.alwaysAlive||cfg.aliveUnknown,false);assert.equal(sfoCalls,2);calls.push('install');if(cfg.installFail){write(error,'Native install refused');return -1;}return 0;},
  helper_installed_matches:(_m,error)=>{calls.push('confirm');assert.ok(calls.includes('install'));confirmCalls++;if(cfg.neverConfirmed||confirmCalls<=(cfg.confirmAfter||0)){write(error,'Installed bytes do not match');return -1;}return 0;},
  helper_report:(p,rc)=>{calls.push('report:'+string(p));assert.equal(rc,0);}
 };
 const module=await WebAssembly.compile(fs.readFileSync(process.argv[2]));
 for(const imp of WebAssembly.Module.imports(module))assert.ok(env[imp.name],'Missing OS mock '+imp.name);
 assert.equal(WebAssembly.Module.imports(module).some(x=>/UnInstall|PrepareOverwrite|KillApp|remove|rename|fwrite/.test(x.name)),false);
 instance=await WebAssembly.instantiate(module,{env});const e=instance.exports,base=Number(e.__heap_base.value);
 function run(options={}){
  cfg={alwaysAlive:false,aliveUnknown:false,...options};heap=base+1024;files=new Map([[path,Buffer.from(cfg.pkg||pkg)]]);handles=new Map();nextHandle=waits=aliveCalls=sfoCalls=confirmCalls=0;calls=[];
  const inputs=[cfg.manifest||sign(),publicKey,Buffer.from(cfg.request??request)].map(b=>{const p=alloc(b.length);mem().set(b,Number(p));return [p,BigInt(b.length)];});
  const error=alloc(512);write(error,'');const rc=e.fixture_helper(inputs[0][0],inputs[0][1],inputs[1][0],inputs[2][0],inputs[2][1],error,512n);
  assert.equal(handles.size,0);assert.ok(files.has(path));checks++;return {rc,error:string(error)};
 }
 assert.equal(run({aliveFor:3,confirmAfter:2}).rc,0);assert.equal(waits,5);assert.equal(calls.filter(x=>x==='install').length,1);assert.equal(calls.at(-1),'report:instalacao confirmada');
 for(const options of [{alwaysAlive:true},{aliveUnknown:true},{ackFail:true},{sfoFail:true},{sfos:['00.42']},{sfos:['00.43']},{sfos:['00.41','00.42']},{sfos:['00.41','00.43']},{changedAfterAck:true},{noMemory:true},{openFail:true},{readError:true}]){assert.equal(run(options).rc,-1);assert.equal(calls.includes('install'),false,JSON.stringify(options));}
 assert.equal(waits,0);
 for(const options of [{installFail:true},{neverConfirmed:true}]){assert.equal(run(options).rc,-1);assert.equal(calls.filter(x=>x==='install').length,1);assert.equal(calls.includes('report:instalacao confirmada'),false);}
 assert.equal(waits,120);
 for(const bad of ['','0\n'+request.split('\n')[1]+'\n','1\n'+request.split('\n')[1]+'\n','-4\n'+request.split('\n')[1]+'\n',request+'\n',request.replace('1234','2147483648'),request.replace('a','A'),request.replace('1234','xyz'),request.slice(0,-1),request.replace('1234','9'.repeat(80)),request.replace('1234','2\u0000')]){assert.equal(run({request:bad}).rc,-1);assert.equal(calls.includes('ack'),false);assert.equal(calls.includes('install'),false);}
 const corrupted=sign();corrupted[0]^=1;
 for(const manifest of [corrupted,Buffer.alloc(64),Buffer.alloc(4097),sign(fields.map((s,i)=>i===1?'OTHER':s)),sign(fields.map((s,i)=>i===4?'39':s))]){assert.equal(run({manifest}).rc,-1);assert.equal(calls.includes('ack'),false);assert.equal(calls.includes('install'),false);}
 for(const broken of [pkg.subarray(0,100),Buffer.concat([pkg,Buffer.from([0])]),Buffer.from(pkg).fill(0,10000,10001)]){assert.equal(run({pkg:broken}).rc,-1);assert.equal(calls.includes('ack'),false);assert.equal(calls.includes('install'),false);}
 // Even a correctly signed file with changed PKG identity/SFO is refused.
 for(const offset of [0,0x40,0x74,0x430]){const b=Buffer.from(pkg);b[offset]^=1;const f=[...fields];f[6]=crypto.createHash('sha512').update(b).digest('hex');assert.equal(run({pkg:b,manifest:sign(f)}).rc,-1);assert.equal(calls.includes('install'),false);}
 console.log(`${checks} independent-installer checks passed: signed real PKG, no downgrade, verified handoff, waits for old process exit, re-verification after handoff, one direct install attempt, confirmation required, no uninstall/overwrite/kill calls, PKG preserved on failure.`);
})().catch(e=>{console.error(e);process.exitCode=1;});

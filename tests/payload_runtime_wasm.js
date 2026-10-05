const fs=require('node:fs'),assert=require('node:assert/strict');
(async()=>{
 let instance,cfg,events,active,checks=0,progress,probes;
 const mem=()=>new Uint8Array(instance.exports.memory.buffer),v=()=>new DataView(mem().buffer);
 const str=p=>{p=Number(p);let e=p;while(mem()[e])e++;return Buffer.from(mem().subarray(p,e)).toString();};
 const env={
 payload_progress:(p,code)=>{progress.push({stage:str(p),code});},jbc_resolve_error:()=>10,
 jbc_get_cred:p=>{events.push('save');if(cfg.getFail)return -1;mem().fill(0,Number(p),Number(p)+80);mem()[Number(p)]=17;return 0;},
 jbc_jailbreak_cred:p=>{events.push('resolve');if(cfg.resolveFail)return -1;mem()[Number(p)]=99;return 0;},
 jbc_set_cred:p=>{const restore=mem()[Number(p)]===17;events.push(restore?'restore':'activate');if(restore){active=false;return cfg.restoreFail?-1:0;}assert.equal(v().getBigUint64(Number(p)+48,true),0n);assert.equal(v().getBigUint64(Number(p)+56,true),0x3800000000000010n);assert.equal(v().getBigUint64(Number(p)+64,true),1n<<62n);active=true;return cfg.activateFail?-1:0;},
 payload_bind_symbols:()=>{assert.ok(active);events.push('bind');return cfg.bindFail?-1:0;},
 pkg_update_payload_main:()=>{assert.ok(active);events.push('worker');return cfg.workerFail?-1:0;},
 payload_access:p=>{assert.ok(active);const path=str(p);probes.push(path);if(path.endsWith('/request'))return cfg.rootMissing||cfg.physicalRoot&&!path.startsWith('/user/')?-1:0;assert.equal(path,(cfg.physicalRoot?'/user':'')+'/data/pkg/check.pkg');return 0;},
 payload_open:p=>{assert.equal(str(p),'/user/data/pkg/update-install-debug.log');events.push('log');return 7;},
 write:(fd,p,n)=>{assert.equal(fd,7);assert.match(str(p),/falha ao resolver/);return n;},close:()=>0,
 };
 const module=await WebAssembly.compile(fs.readFileSync(process.argv[2]));
 for(const imp of WebAssembly.Module.imports(module))if(!env[imp.name])env[imp.name]=()=>{throw Error('Unexpected import '+imp.name);};
 instance=await WebAssembly.instantiate(module,{env});
 function run(options,result,sequence){cfg=options;active=false;events=[];progress=[];probes=[];assert.equal(instance.exports.payload_main(),result);assert.deepEqual(events,sequence);assert.equal(active,false);assert.equal(progress[0].stage,'arranque do BIN');if(result)assert.ok(progress.some(x=>/falha/.test(x.stage)),'Every early failure must be recorded');checks++;}
 run({},0,['save','resolve','activate','bind','worker','restore']);
 run({getFail:true},-1,['save']);run({resolveFail:true},-1,['save','resolve']);
 run({activateFail:true},-1,['save','resolve','activate','restore']);
 run({bindFail:true},-1,['save','resolve','activate','bind','restore']);
 run({workerFail:true},-1,['save','resolve','activate','bind','worker','restore']);
 run({restoreFail:true},-1,['save','resolve','activate','bind','worker','restore']);
 run({rootMissing:true},-1,['save','resolve','activate','restore']);assert.equal(probes.length,2);
 for(const physicalRoot of [false,true]){
  run({physicalRoot},0,['save','resolve','activate','bind','worker','restore']);assert.equal(probes.length,physicalRoot?2:1);
  const base=Number(instance.exports.__heap_base.value),input=BigInt(base),output=BigInt(base+1024);mem().set(Buffer.from('/data/pkg/check.pkg\0'),base);
  assert.equal(instance.exports.payload_data_path(input,output,800n),output);assert.equal(str(output),(physicalRoot?'/user':'')+'/data/pkg/check.pkg');
  assert.equal(instance.exports.payload_data_path(input,output,8n),0n,'Reject truncated path');
  active=true;assert.equal(instance.exports.access(input,0),0);active=false;checks++;
 }
 assert.equal(WebAssembly.Module.imports(module).some(x=>/exit|KillApp|PrepareOverwrite|UnInstall/.test(x.name)),false);
 console.log(`${checks} payload privilege lifetime checks passed: temporary full filesystem access, installation auth, return to host and restoration on every completion/failure path.`);
})().catch(e=>{console.error(e);process.exitCode=1;});

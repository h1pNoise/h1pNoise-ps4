// Exercise the production access lifetime; kernel calls are mocked.
const fs=require('node:fs'),assert=require('node:assert/strict');
(async()=>{
 let instance,cfg,events,elevated=false,held=false;
 const mem=()=>new Uint8Array(instance.exports.memory.buffer);
 const str=p=>{p=Number(p);let end=p;while(mem()[end])end++;return Buffer.from(mem().subarray(p,end)).toString();};
 const write=(p,s)=>mem().set(Buffer.from(s+'\0'),Number(p));
 const env={
  pthread_mutex_lock:()=>{assert.equal(held,false);if(cfg.lockFailure)return 16;held=true;return 0;},
  pthread_mutex_trylock:()=>held?16:(held=true,0),
  pthread_mutex_unlock:()=>{assert.ok(held);held=false;return 0;},
  open:path=>{assert.equal(str(path),'/system/common/lib/libSceAppInstUtil.sprx');events.push('open');return cfg.accessible?12:-1;},
  close:fd=>{assert.equal(fd,12);events.push('close');return 0;},
  link_stage:()=>{},
  snprintf:(out,cap,fmt)=>{const s=str(fmt).slice(0,Number(cap)-1);write(out,s);return s.length;},
  memcpy:(dst,src,n)=>{mem().copyWithin(Number(dst),Number(src),Number(src)+Number(n));return dst;},
  jbc_get_cred:p=>{events.push('save');if(cfg.getFailure)return -1;mem().fill(0,Number(p),Number(p)+80);mem()[Number(p)]=17;return 0;},
  jbc_jailbreak_cred:p=>{events.push('resolve');if(cfg.resolveFailure)return -1;assert.equal(mem()[Number(p)],17);mem()[Number(p)]=99;return 0;},
  jbc_resolve_error:()=>9,
  jbc_set_auth:p=>{
   const v=new DataView(mem().buffer),restoring=v.getBigUint64(Number(p)+56,true)===0n;
   assert.equal(mem()[Number(p)],17);events.push(restoring?'restore-auth':'activate-auth');
   if(!restoring){assert.equal(v.getBigUint64(Number(p)+56,true),0x3800000000000010n);assert.equal(v.getBigUint64(Number(p)+64,true),1n<<62n);}
   if(restoring){if(cfg.restoreFailure)return -1;elevated=false;return 0;}
   elevated=true;return cfg.activationFailure?-1:0;
  },
  jbc_set_cred:p=>{
   const restoring=mem()[Number(p)]===17;assert.ok(restoring||mem()[Number(p)]===99);
   events.push(restoring?'restore':'activate');
   if(restoring){if(cfg.restoreFailure)return -1;elevated=false;return 0;}
   elevated=true;return cfg.activationFailure?-1:0;
  },
  installer_init_mock:(error)=>{events.push('initialize');assert.ok(cfg.accessible||elevated);if(cfg.initFailure){write(error,'Module initialization failed');return -23;}return 0;},
  installer_op_mock:(context,error)=>{assert.equal(context,42n);assert.ok(elevated);events.push('operation');if(cfg.opFailure){write(error,'BGFT operation failed');return -23;}return 0;}
 };
 ({instance}=await WebAssembly.instantiate(fs.readFileSync(process.argv[2]),{env}));
 const e=instance.exports,error=e.__heap_base.value;
 let checks=0;
 function run(options,expected,sequence){
  cfg=options;events=[];elevated=false;write(error,'');
  assert.equal(e.run_installer_access(error,512n),expected);
  assert.equal(held,false);assert.deepEqual(events,sequence);if(!cfg.restoreFailure)assert.equal(elevated,false);
  checks++;return str(error);
 }
 run({lockFailure:true},-1,[]);
 run({accessible:true},0,['open','close','initialize']);
 assert.match(run({accessible:true,initFailure:true},-23,['open','close','initialize']),/initialization/);
 assert.match(run({getFailure:true},-1,['open','save']),/HEN/);
 run({resolveFailure:true},-1,['open','save','resolve']);
 run({activationFailure:true},-1,['open','save','resolve','activate','restore']);
 run({},0,['open','save','resolve','activate','initialize','restore']);
 assert.match(run({initFailure:true},-23,['open','save','resolve','activate','initialize','restore']),/initialization/);
 assert.match(run({restoreFailure:true},-1,['open','save','resolve','activate','initialize','restore']),/Fecha/);
 assert.match(run({activationFailure:true,restoreFailure:true},-1,['open','save','resolve','activate','restore']),/Fecha/);
 function permissions(options,expected,sequence){
  cfg=options;events=[];elevated=false;write(error,'');
  assert.equal(e.run_installer_permissions(error,512n),expected);assert.equal(held,false);assert.deepEqual(events,sequence);
  if(!cfg.restoreFailure)assert.equal(elevated,false);checks++;
 }
 permissions({lockFailure:true},-1,[]);
 permissions({},0,['save','activate-auth','operation','restore-auth']);
 permissions({opFailure:true},-23,['save','activate-auth','operation','restore-auth']);
 permissions({getFailure:true},-1,['save']);
 permissions({activationFailure:true},-1,['save','activate-auth','restore-auth']);
 permissions({restoreFailure:true},-1,['save','activate-auth','operation','restore-auth']);
 permissions({opFailure:true,restoreFailure:true},-1,['save','activate-auth','operation','restore-auth']);
 permissions({activationFailure:true,restoreFailure:true},-1,['save','activate-auth','restore-auth']);
 console.log(`${checks} installer access checks passed: no HEN, resolution/activation failures, restoration on success and failure.`);
})().catch(e=>{console.error(e);process.exitCode=1;});

// Run the actual credential resolver against a synthetic kernel memory map.
// No kernel access or privilege change is performed on the host.
const fs=require('node:fs'),assert=require('node:assert/strict');
const binary=fs.readFileSync(process.argv[2]);
const U=n=>BigInt.asUintN(64,n),heap=0xffff800000000000n;
const td=heap+0x10000n,proc=heap+0x20000n,init=heap+0x30000n;
const fd=heap+0x40000n,cred=heap+0x50000n,initfd=heap+0x60000n,initcred=heap+0x70000n;
const root=heap+0x80000n,savedPrison=heap+0x90000n,staticPrison=0xffffffff81234000n;
let checks=0;
async function fixture(cfg={}){
 let instance,writes=0;
 const regions=new Map([td,proc,init,fd,cred,initfd,initcred,savedPrison,staticPrison].map(p=>[p,Buffer.alloc(256)]));
 function region(p,n){p=U(p);for(const [base,b] of regions)if(p>=base&&p+BigInt(n)<=base+BigInt(b.length))return [b,Number(p-base)];return null;}
 const put64=(p,x)=>{const [b,off]=region(p,8);b.writeBigUInt64LE(x,off);};
 const put32=(p,x)=>{const [b,off]=region(p,4);b.writeInt32LE(x,off);};
 put64(td+8n,proc);put64(proc,init);put64(init+8n,cfg.changed?0n:proc);
 put64(proc+0x40n,cred);put64(proc+0x48n,fd);put32(proc+0xb0n,42);
 put64(init+0x40n,cfg.badInitFields?0n:initcred);put64(init+0x48n,initfd);put32(init+0xb0n,1);
 put64(cred+0x30n,savedPrison);put64(fd+0x10n,root);put64(fd+0x18n,root);put64(fd+0x20n,0n);
 put64(initcred+0x30n,cfg.prison??staticPrison);put64(initfd+0x18n,cfg.root??root);
 put32(savedPrison+0x14n,10);put32(staticPrison+0x14n,20);
 if(cfg.listEnd)put64(proc,0n);
 const mem=()=>new Uint8Array(instance.exports.memory.buffer);
 const env={
  getpid:()=>cfg.wrongPid?43:42,
  jbc_krw_available:()=>cfg.noKernel?0:1,
  jbc_krw_get_td:()=>cfg.badThread?0n:td,
  jbc_krw_read64:(p,kind)=>{assert.equal(kind,1);const r=region(p,8);return r?r[0].readBigUInt64LE(r[1]):-1n;},
  jbc_krw_memcpy:(dest,src,n,kind)=>{
   dest=U(dest);src=U(src);n=Number(n);assert.ok(kind===1||kind===2);
   if(cfg.failPrison&&src===initcred+0x30n)return -1;
   if(cfg.failRoot&&src===initfd+0x18n)return -1;
   if(cfg.failPid&&src===proc+0xb0n)return -1;
   if(cfg.failAuthWrite&&dest===cred+88n)return -1;
   const r=region(src,n),w=region(dest,n);
   if(r&&!w){mem().set(r[0].subarray(r[1],r[1]+n),Number(dest));return 0;}
   if(w&&!r){if(cfg.preserveRoots&&dest===fd+0x10n)assert.deepEqual(Buffer.from(mem().subarray(Number(src),Number(src)+n)),w[0].subarray(w[1],w[1]+n));w[0].set(mem().subarray(Number(src),Number(src)+n),w[1]);writes++;return 0;}
   throw Error('Unexpected synthetic memory access');
  },
  memcpy:(dest,src,n)=>{mem().copyWithin(Number(dest),Number(src),Number(src)+Number(n));return dest;},
  jbc_raw_open:()=>{throw Error('Unexpected directory open');},
  jbc_raw_close:()=>{throw Error('Unexpected directory close');}
 };
 ({instance}=await WebAssembly.instantiate(binary,{env}));
 return {e:instance.exports,mem,writes:()=>writes,snapshot:()=>[...regions].map(([p,b])=>[p,Buffer.from(b)])};
}
(async()=>{
 for(const prison of [staticPrison,savedPrison]){
  const f=await fixture({prison}),e=f.e,p=e.__heap_base.value,saved=p+128n;
  assert.equal(e.jbc_get_cred(saved),0);f.mem().copyWithin(Number(p),Number(saved),Number(saved)+80);
  assert.equal(e.jbc_jailbreak_cred(p),0);
  const v=new DataView(f.mem().buffer);assert.equal(v.getBigUint64(Number(p)+24,true),prison);
  assert.equal(v.getBigUint64(Number(p)+40,true),root);
  assert.equal(e.jbc_test_set_cred_internal(p),0);assert.equal(f.writes(),5);
  assert.equal(e.jbc_test_set_cred_internal(saved),0);assert.equal(f.writes(),10);
  checks++;
 }
 for(const prison of [staticPrison,savedPrison]){
  const f=await fixture({prison,preserveRoots:true}),e=f.e,p=e.__heap_base.value,saved=p+128n;
  assert.equal(e.jbc_get_cred(saved),0);f.mem().copyWithin(Number(p),Number(saved),Number(saved)+80);
  const before=f.snapshot(),v=new DataView(f.mem().buffer);
  assert.equal(e.jbc_jailbreak_cred(p),0);
  for(const offset of [32,40,48])v.setBigUint64(Number(p)+offset,v.getBigUint64(Number(saved)+offset,true),true);
  assert.equal(e.jbc_set_cred(p),0);
  const changed=f.snapshot();assert.deepEqual(changed.find(([b])=>b===fd),before.find(([b])=>b===fd));
  assert.equal(e.jbc_set_cred(saved),0);assert.deepEqual(f.snapshot(),before);checks++;
 }
 for(const [cfg,reason] of [
  [{prison:0n},9],[{prison:0x1234n},9],[{prison:0xfffffffffffff000n},9],
  [{root:0n},10],[{root:staticPrison},10],[{badThread:true},2],
  [{listEnd:true},5],[{changed:true},1],[{badInitFields:true},6],
  [{failPrison:true},7],[{failRoot:true},8],[{failPid:true},1]
 ]){
  const f=await fixture(cfg),e=f.e;
  assert.equal(e.jbc_jailbreak_cred(e.__heap_base.value),-1);
  assert.equal(e.jbc_resolve_error(),reason);assert.equal(f.writes(),0);checks++;
 }
 for(const cfg of [{wrongPid:true},{noKernel:true}]){
  const f=await fixture(cfg);assert.equal(f.e.jbc_get_cred(f.e.__heap_base.value),-1);assert.equal(f.writes(),0);checks++;
 }
 const auth=await fixture(),e=auth.e,p=e.__heap_base.value,saved=p+128n;
 assert.equal(e.jbc_get_cred(saved),0);auth.mem().copyWithin(Number(p),Number(saved),Number(saved)+80);
 const before=auth.snapshot(),v=new DataView(auth.mem().buffer);
 v.setInt32(Number(p),99,true);v.setBigUint64(Number(p)+24,0x1234n,true);v.setBigUint64(Number(p)+40,0x5678n,true);
 v.setBigUint64(Number(p)+56,0x3800000000000010n,true);v.setBigUint64(Number(p)+64,1n<<62n,true);
 assert.equal(e.jbc_set_auth(p),0);assert.equal(auth.writes(),1);
 const changed=auth.snapshot();
 for(let i=0;i<before.length;i++){
  if(before[i][0]===cred){assert.deepEqual(changed[i][1].subarray(0,88),before[i][1].subarray(0,88));assert.deepEqual(changed[i][1].subarray(112),before[i][1].subarray(112));assert.equal(changed[i][1].readBigUInt64LE(88),0x3800000000000010n);}
  else assert.deepEqual(changed[i],before[i]);
 }
 assert.equal(e.jbc_set_auth(saved),0);assert.equal(auth.writes(),2);assert.deepEqual(auth.snapshot(),before);checks++;
 for(const cfg of [{wrongPid:true},{noKernel:true},{badThread:true},{failAuthWrite:true}]){
  const f=await fixture(cfg);assert.equal(f.e.jbc_set_auth(f.e.__heap_base.value),-1);assert.equal(f.writes(),0);checks++;
 }
 console.log(`${checks} libjbc resolver checks passed: static/heap prison, actual credential read/write/restore, invalid addresses, bounded traversal, no-kernel and PID mismatch guards.`);
})().catch(e=>{console.error(e);process.exitCode=1;});

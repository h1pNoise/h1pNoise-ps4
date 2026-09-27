// Executes the real storage.c policy in both build configurations.
const fs=require('node:fs');
const assert=require('node:assert/strict');
(async()=>{
 for(const [file,emulator] of [[process.argv[2],false],[process.argv[3],true]]){
  let instance,queryError=0,free=0n;
  const text=ptr=>{const b=new Uint8Array(instance.exports.memory.buffer);let end=ptr;while(b[end])end++;return Buffer.from(b.slice(ptr,end)).toString();};
  const imports={env:{
   free_bytes:(_path,out)=>{new DataView(instance.exports.memory.buffer).setBigUint64(out,free,true);return queryError;},
   snprintf:(out,cap,format)=>{const value=text(format);const b=Buffer.from(value);const target=new Uint8Array(instance.exports.memory.buffer);if(cap){target.set(b.subarray(0,cap-1),out);target[out+Math.min(b.length,cap-1)]=0;}return b.length;}
  }};
  ({instance}=await WebAssembly.instantiate(fs.readFileSync(file),imports));
  const e=instance.exports,base=Number(e.__heap_base.value),out=base+128,err=base+256;
  new Uint8Array(e.memory.buffer).set(Buffer.from('/data/pkg\0'),base);
  assert.equal(e.storage_from_blocks(4096n,200000000n,100000000n,out),0);
  assert.equal(new DataView(e.memory.buffer).getBigUint64(out,true),409600000000n);
  assert.equal(e.storage_from_blocks(0n,100n,10n,out),-1);
  assert.equal(e.storage_from_blocks(4096n,0n,0n,out),-1);
  assert.equal(e.storage_from_blocks(4096n,100n,101n,out),-1);
  assert.equal(e.storage_from_blocks(-1n,2n,1n,out),-1);
  queryError=-1;
  assert.equal(e.storage_check(base,100n,err,512),emulator?0:-1);
  assert.ok(emulator?text(err)==='':text(err).includes('Nao foi possivel medir'));
  queryError=0;free=0n;
  assert.equal(e.storage_check(base,100n,err,512),-1);
  assert.ok(text(err).includes('Espaco insuficiente'));
  free=100n;assert.equal(e.storage_check(base,100n,err,512),0);
  assert.equal(e.storage_check(base,101n,err,512),-1);
  free=500000000000n;assert.equal(e.storage_check(base,52000000000n,err,512),0);
  console.log((emulator?'shadPS4':'PS4 normal')+': storage policy checks passed');
 }
})().catch(e=>{console.error(e);process.exitCode=1;});

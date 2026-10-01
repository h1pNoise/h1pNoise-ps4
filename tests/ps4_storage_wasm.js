// Execute the production PS4 query against raw native-ABI byte fixtures.
const fs=require('node:fs'),assert=require('node:assert/strict');
(async()=>{
 for(const [file,shad] of [[process.argv[2],false],[process.argv[3],true]]){
  let instance,cfg,opens,closes,nativeCalls,libraryCalls,statCalls,fdOpen,errnoPointer,held,elevated,savedCred,accessEvents;
  const mem=()=>new Uint8Array(instance.exports.memory.buffer),view=()=>new DataView(mem().buffer);
  const str=p=>{p=Number(p);let end=p;while(mem()[end])end++;return Buffer.from(mem().subarray(p,end)).toString();};
  const fixture=(ptr,fields)=>{
   ptr=Number(ptr);mem().fill(0,ptr,ptr+4096);
   const data={version:0x20030518,block:4096n,total:200000000n,free:110000000n,available:100000000n,...fields};
   view().setUint32(ptr,data.version,true);view().setBigUint64(ptr+16,data.block,true);
   view().setBigUint64(ptr+32,data.total,true);view().setBigUint64(ptr+40,data.free,true);view().setBigInt64(ptr+48,data.available,true);
  };
  const env={
   installer_credentials_trylock:()=>{assert.equal(held,false);if(cfg.busy)return 16;held=true;return 0;},
   installer_credentials_unlock:()=>{assert.ok(held);held=false;},
   jbc_get_cred:p=>{assert.ok(held);accessEvents.push('save');if(!cfg.permission||cfg.getFailure)return -1;
    mem().fill(17,Number(p),Number(p)+80);view().setInt32(Number(p),17,true);
    for(const off of [32,40,48])view().setBigUint64(Number(p)+off,0xffff800000010000n+BigInt(off),true);
    savedCred=Buffer.from(mem().subarray(Number(p),Number(p)+80));return 0;},
   jbc_jailbreak_cred:p=>{accessEvents.push('resolve');if(cfg.resolveFailure)return -1;
    view().setInt32(Number(p),0,true);view().setBigUint64(Number(p)+24,0xffffffff81234000n,true);
    for(const off of [32,40,48])view().setBigUint64(Number(p)+off,0xffff800000020000n,true);return 0;},
   jbc_set_cred:p=>{assert.ok(held);const restoring=view().getInt32(Number(p),true)===17;accessEvents.push(restoring?'restore':'activate');
    for(const off of [32,40,48])assert.equal(view().getBigUint64(Number(p)+off,true),savedCred.readBigUInt64LE(off));
    if(restoring){assert.deepEqual(Buffer.from(mem().subarray(Number(p),Number(p)+80)),savedCred);if(cfg.restoreFailure)return -1;elevated=false;return 0;}
    elevated=true;return cfg.activationFailure?-1:0;},
   __multi3:(ptr,aLow,aHigh,bLow,bHigh)=>{
    const a=BigInt.asUintN(64,aLow)|(BigInt.asUintN(64,aHigh)<<64n),b=BigInt.asUintN(64,bLow)|(BigInt.asUintN(64,bHigh)<<64n);
    const product=BigInt.asUintN(128,a*b);view().setBigUint64(Number(ptr),BigInt.asUintN(64,product),true);view().setBigUint64(Number(ptr)+8,product>>64n,true);
   },
   snprintf:()=>{throw Error('Unexpected storage policy formatting in query fixture');},
   memset:(p,c,n)=>{mem().fill(c,Number(p),Number(p)+Number(n));return p;},
   memcpy:(d,s,n)=>{mem().copyWithin(Number(d),Number(s),Number(s)+Number(n));return d;},
   strcmp:(a,b)=>str(a)===str(b)?0:1,
   __errno_location:()=>errnoPointer,__error:()=>errnoPointer,
   open:(path,flags)=>{assert.equal(flags,0);opens.push(str(path));assert.equal(fdOpen,false);if(cfg.openFails){view().setInt32(Number(errnoPointer),2,true);return -1;}fdOpen=true;return 8;},
   close:fd=>{assert.equal(fd,8);assert.ok(fdOpen);fdOpen=false;closes++;return 0;},
   ps4_storage_native_query:(fd,ptr)=>{assert.equal(fd,8);assert.ok(fdOpen);nativeCalls++;if(elevated){if(cfg.elevatedError)return cfg.elevatedError;fixture(ptr,cfg.elevatedData);return 0;}if(cfg.nativeError)return cfg.nativeError;fixture(ptr,cfg.nativeData);return 0;},
   fstatfs:(fd,ptr)=>{assert.equal(fd,8);assert.ok(fdOpen);libraryCalls++;const parent=opens.at(-1)==='/data'&&cfg.libraryParentData;if(cfg.libraryError&&!parent){view().setInt32(Number(errnoPointer),cfg.libraryErrno||38,true);return -1;}fixture(ptr,parent||cfg.libraryData);return 0;},
   stat:(path,ptr)=>{statCalls.push(str(path));if(cfg.statError)return -1;view().setUint32(Number(ptr),str(path)==='/data'?cfg.parentDevice:cfg.destinationDevice,true);return 0;}
  };
  ({instance}=await WebAssembly.instantiate(fs.readFileSync(file),{env}));
  const e=instance.exports,base=e.__heap_base.value,out=base+4096n;errnoPointer=base+4200n;
  let checks=0;
  function run(options={},expected=0,bytes=409600000000n,path='/data/pkg'){
   cfg={destinationDevice:7,parentDevice:7,nativeData:{},libraryData:{},...options};
   opens=[];closes=nativeCalls=libraryCalls=0;statCalls=[];accessEvents=[];fdOpen=held=elevated=false;
   mem().set(Buffer.from(path+'\0'),Number(base));view().setBigUint64(Number(out),999n,true);
   assert.equal(e.free_bytes(base,out),expected);assert.equal(view().getBigUint64(Number(out),true),bytes);
   assert.equal(fdOpen,false);assert.equal(held,false);if(!cfg.restoreFailure)assert.equal(elevated,false);assert.equal(closes,cfg.openFails||cfg.busy?0:opens.length);checks++;
  }
  run();assert.equal(nativeCalls,shad?0:1);assert.equal(libraryCalls,shad?1:0);
  const dataOptions=data=>shad?{libraryData:data}:{nativeData:data};
  run({...dataOptions({available:0n}),libraryData:{available:0n}},0,0n);
  run({...dataOptions({available:-1n}),libraryData:{available:-1n}},0,0n);
  run({nativeError:-38});assert.equal(libraryCalls,1);
  if(!shad){run({nativeData:{version:0}});assert.equal(libraryCalls,1);}
  run({nativeError:-1,libraryError:true,parentDevice:8},-1,0n);assert.deepEqual(opens,['/data/pkg']);assert.deepEqual(statCalls,['/data/pkg','/data']);
  run({nativeError:-1,libraryError:true,statError:true},-1,0n);assert.deepEqual(opens,['/data/pkg']);
  run({nativeError:-1,libraryError:true},-1,0n);assert.deepEqual(opens,['/data/pkg','/data']);
  run({nativeError:-1,libraryError:true,libraryParentData:{available:300000n}},0,1228800000n);assert.deepEqual(opens,['/data/pkg','/data']);
  run({nativeError:-1,libraryError:true},-1,0n,'/mnt/usb0/pkg');assert.deepEqual(opens,['/mnt/usb0/pkg']);assert.deepEqual(statCalls,[]);
  run({openFails:true},-1,0n);assert.equal(nativeCalls,0);assert.equal(libraryCalls,0);
  for(const data of [{version:0},{block:0n},{total:0n},{free:200000001n},{available:110000001n},{block:0xffffffffffffffffn}]){
   run({nativeData:data,libraryData:data},-1,0n);assert.equal(libraryCalls,2);
  }
  if(!shad){
   const denied={nativeError:-1,libraryError:true,libraryErrno:1,parentDevice:8,permission:true};
   run(denied);assert.deepEqual(accessEvents,['save','resolve','activate','restore']);assert.equal(nativeCalls,2);
   run({...denied,nativeError:-38});assert.deepEqual(accessEvents,['save','resolve','activate','restore']);
   run({...denied,nativeError:-38,libraryErrno:38},-1,0n);assert.deepEqual(accessEvents,[]);
   run({...denied,elevatedData:{available:0n}},0,0n);
   run({...denied,elevatedError:-1},-1,0n);assert.deepEqual(accessEvents,['save','resolve','activate','restore']);
   run({...denied,activationFailure:true},-1,0n);assert.deepEqual(accessEvents,['save','resolve','activate','restore']);assert.equal(nativeCalls,1);
   run({...denied,resolveFailure:true},-1,0n);assert.deepEqual(accessEvents,['save','resolve']);
   run({...denied,getFailure:true},-1,0n);assert.deepEqual(accessEvents,['save']);
   run({busy:true},-1,0n);assert.deepEqual(opens,[]);assert.deepEqual(accessEvents,[]);
   run({...denied,restoreFailure:true,parentDevice:7},-1,0n);assert.deepEqual(opens,['/data/pkg']);
   const calls=nativeCalls;assert.equal(e.free_bytes(base,out),-1);assert.equal(nativeCalls,calls);assert.equal(held,false);checks++;
  }
  console.log(`${checks} ${shad?'shadPS4':'PS4'} filesystem checks passed: native ABI, real full disk, >4 GB, failed/invalid responses, fallback and same-volume guards.`);
 }
})().catch(e=>{console.error(e);process.exitCode=1;});

const fs=require('node:fs'),assert=require('node:assert/strict');
(async()=>{
 let instance,cfg,opened,handles,files,closed,flushed,writes,checks=0;
 const bytes=()=>new Uint8Array(instance.exports.memory.buffer);
 const str=p=>{p=Number(p);let e=p;while(bytes()[e])e++;return Buffer.from(bytes().subarray(p,e)).toString();};
 const env={
  payload_open:(p,flags)=>{const path=str(p);opened.push(path);if(cfg.allFail||cfg.physicalOnly&&!path.startsWith('/user/'))return -1;assert.ok(flags===0x209||flags===0x601);handles.push(path);if(flags===0x601||!files.has(path))files.set(path,Buffer.alloc(0));return handles.length;},
  write:(fd,p,n)=>{writes++;if(cfg.writeFail)return -1n;const used=Math.min(Number(n),cfg.partial?5:Number(n));const path=handles[fd-1];files.set(path,Buffer.concat([files.get(path),Buffer.from(bytes().subarray(Number(p),Number(p)+used))]));return BigInt(used);},
  fsync:()=>{flushed++;return cfg.flushFail?-1:0;},close:()=>{closed++;return 0;}
 };
 const mod=await WebAssembly.compile(fs.readFileSync(process.argv[2]));
 for(const imp of WebAssembly.Module.imports(mod))assert.ok(env[imp.name],'Unexpected libc dependency '+imp.name);
 instance=await WebAssembly.instantiate(mod,{env});const p=Number(instance.exports.__heap_base.value);
 for(const options of [{},{physicalOnly:true},{allFail:true},{writeFail:true},{flushFail:true},{partial:true}]){
  cfg=options;opened=[];handles=[];files=new Map();closed=flushed=writes=0;bytes().set(Buffer.from('arranque\0'),p);
  instance.exports.payload_progress(BigInt(p),-123);
  if(cfg.allFail){assert.equal(closed,0);assert.equal(writes,0);assert.equal(opened.length,4);}
  else {assert.equal(closed,2);assert.equal(flushed,2);if(!cfg.writeFail&&!cfg.partial)for(const data of files.values())assert.equal(data.toString(),'pkg-payload-runtime arranque 0xFFFFFF85\n');}
  checks++;
 }
 assert.equal(WebAssembly.Module.imports(mod).some(x=>/jbc|fopen|malloc|exit/.test(x.name)),false);
 console.log(`${checks} early payload diagnostics checks passed: pre-libc syscall log, path fallback, bounded writes and cleanup.`);
})().catch(e=>{console.error(e);process.exitCode=1;});

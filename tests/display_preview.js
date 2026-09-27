// Render the production C framebuffer, substituting only standard libc calls.
const fs=require('node:fs'),path=require('node:path');
(async()=>{
 let instance;const m=()=>new Uint8Array(instance.exports.memory.buffer),dv=()=>new DataView(m().buffer);
 const str=p=>{let e=p;while(m()[e])e++;return Buffer.from(m().subarray(p,e)).toString('utf8');};
 const put=(p,s)=>m().set(Buffer.from(s+'\0'),p);
 const env={
  memset:(p,c,n)=>{m().fill(c,p,p+n);return p;},memcpy:(d,s,n)=>{m().copyWithin(d,s,s+n);return d;},
  strlen:p=>{let e=p;while(m()[e])e++;return e-p;},
  strcmp:(a,b)=>{const x=str(a),y=str(b);return x===y?0:x<y?-1:1;},
  strcpy:(d,s)=>{put(d,str(s));return d;},strcat:(d,s)=>{put(d,str(d)+str(s));return d;},
  memmove:(d,s,n)=>{m().copyWithin(d,s,s+n);return d;},abs:Math.abs,labs:Math.abs,
  strchr:(p,c)=>{for(let i=p;;i++){if(m()[i]===c)return i;if(!m()[i])return 0;}},
  snprintf:(out,cap,fmt,args)=>{
   const value=str(fmt).replace(/%%|%(?:\.(\d+))?([sdf])/g,(token,precision,type)=>{
    if(token==='%%')return '%';let result;
    if(type==='f'){args=(args+7)&~7;result=dv().getFloat64(args,true).toFixed(Number(precision||6));args+=8;}
    else {const v=dv().getInt32(args,true);args+=4;result=type==='s'?str(v):String(v);}return result;
   });
   const raw=Buffer.from(value);if(cap){m().set(raw.subarray(0,cap-1),out);m()[out+Math.min(raw.length,cap-1)]=0;}return raw.length;
  },
 };
 ({instance}=await WebAssembly.instantiate(fs.readFileSync(process.argv[2]),{env}));
 for(const [i,name] of ['idle','download','paused','error','complete','link','link-error','offline','update-available','update-download'].entries()){
  const p=instance.exports.render_preview(i),raw=Buffer.from(m().subarray(p,p+1280*720*4));
  fs.writeFileSync(path.join(process.argv[3],'ui-native-'+name+'.bgra'),raw);
 }
 console.log('Rendered 10 states using the production PS4 display code.');
})().catch(e=>{console.error(e);process.exitCode=1;});

// Shared console colour: exercise the shipped script with delayed and failed requests.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const html=fs.readFileSync(process.argv[2],'utf8'),script=fs.readFileSync(path.join(path.dirname(process.argv[2]),'app.js'),'utf8');
const ids=[...html.matchAll(/\bid="([^"]+)"/g)].map(m=>m[1]);assert.equal(ids.length,new Set(ids).size);
let focused='',now=1000;
function node(id=''){
 const classes=new Set();return {id,value:'',textContent:'',disabled:false,hidden:false,files:[],children:[],dataset:{},style:{},attributes:{},handlers:{},parentElement:{hidden:false},
 classList:{add:s=>classes.add(s),remove:s=>classes.delete(s),toggle:(s,yes)=>yes===undefined?(classes.has(s)?classes.delete(s):classes.add(s)):yes?classes.add(s):classes.delete(s),contains:s=>classes.has(s)},
 setAttribute(k,v){this.attributes[k]=v;},focus(){focused=id;},scrollIntoView(){},select(){},remove(){},replaceChildren(){this.children=[];},append(...items){this.children.push(...items);},
 addEventListener(k,fn){(this.handlers[k]??=[]).push(fn);},click(){return this.onclick?.();},getContext(){return {fillRect(){}}}
 };
}
const nodes=Object.fromEntries(ids.map(id=>[id,node(id)]));
const state={loaded:false,busy:false,directSupported:true,directBusy:false,directTask:-1,directPhase:'idle',directMessage:'',phase:'idle',name:'',total:0,done:0,peers:0,message:'Envia um torrent.',free:null,files:[],storagePath:'/data/pkg',accent:'#9debcf'};
let networkError=false,lastPost=null,postHold=null,statusHold=null,authStatus=0,requests=0;const posts=[],css=new Map();
const ctx=vm.createContext({
 document:{getElementById:id=>{assert.ok(nodes[id],id);return nodes[id];},createElement:()=>node(),body:node(),documentElement:{style:{setProperty(k,v){css.set(k,v);}}}},localStorage:{getItem(){return null;},setItem(){}},
 URL,URLSearchParams,AbortController,TextEncoder,location:{hash:'',origin:'http://192.168.1.10:8787',pathname:'/',search:''},history:{},navigator:{},window:{},matchMedia:()=>({matches:false}),
 performance:{now:()=>now},setInterval(){},setTimeout(){return 1;},clearTimeout(){},
 fetch:async(url,options)=>{
  requests++;if(authStatus)return {ok:false,status:authStatus,json:async()=>({error:'Codigo recusado'})};
  if(networkError)throw Error('Sem rede');
  if(options.method==='POST'){lastPost={url,options};posts.push(lastPost);if(postHold)await postHold;if(url==='/api/accent')state.accent=options.body;if(url==='/api/reset')Object.assign(state,{loaded:false,busy:false,total:0,done:0,phase:'idle'});return {ok:true,json:async()=>({ok:true})};}
  if(url==='/api/qr')return {ok:true,json:async()=>({size:21,modules:'0'.repeat(441)})};
  const snapshot={...state};if(statusHold)await statusHold;return {ok:true,json:async()=>snapshot};
 }
});
vm.runInContext(script,ctx);
const fire=async(id,event,e={})=>{if(nodes[id]['on'+event])await nodes[id]['on'+event](e);for(const fn of nodes[id].handlers[event]||[])await fn(e);};
const setURL=async value=>{nodes['pkg-url'].value=value;await fire('pkg-url','input');};

(async()=>{
 nodes['accent-blue'].click();assert.equal(posts.length,0,'unpaired changes stay local');
 nodes.code.value='0123';await nodes.connect.click();assert.equal(css.get('--mint'),'#9debcf','console preference wins on pairing');
 state.busy=true;state.phase='downloading';await ctx.refresh();
 await nodes['accent-purple'].click();assert.equal(posts.at(-1).options.body,'#c5b0ff');assert.equal(posts.at(-1).options.headers['X-Harbor-Code'],'0123');assert.equal(state.busy,true);
 assert.match(nodes['accent-status'].textContent,/televisão/);
 let release;postHold=new Promise(r=>release=r);
 const first=nodes['accent-blue'].click();const count=posts.length;
 nodes['accent-pink'].click();nodes['accent-orange'].click();assert.equal(posts.length,count,'serial writes');
 await ctx.refresh();assert.equal(css.get('--mint'),'#ffc08a','poll cannot overwrite pending choice');
 postHold=null;release();await first;assert.equal(posts.length,count+1);assert.equal(state.accent,'#ffc08a','latest choice wins');
 let statusRelease;statusHold=new Promise(r=>statusRelease=r);const oldPoll=ctx.refresh();
 await nodes['accent-blue'].click();statusHold=null;statusRelease();await oldPoll;
 assert.equal(css.get('--mint'),'#8cc8ff','stale poll cannot restore old colour');await ctx.refresh();assert.equal(css.get('--mint'),state.accent);
 nodes['accent-custom'].value='#123456';nodes['accent-custom'].oninput();nodes['accent-custom'].value='#654321';nodes['accent-custom'].oninput();
 const before=posts.length;await ctx.saveAccent();assert.equal(posts.length,before+1);assert.equal(state.accent,'#654321','debounced custom colour');
 authStatus=500;await nodes['accent-white'].click();assert.equal(css.get('--mint'),'#654321');assert.match(nodes['accent-status'].textContent,/novamente/);
 authStatus=0;await ctx.refresh();assert.match(nodes['accent-status'].textContent,/novamente/,'poll preserves failed-save explanation');
 await nodes['accent-white'].click();assert.equal(state.accent,'#edf2f4');
 state.accent='#ffadd2';await ctx.refresh();assert.equal(css.get('--mint'),'#ffadd2','another device changes the preference');
 await nodes['accent-reset'].click();assert.equal(state.accent,'#9debcf');
 authStatus=401;await nodes['accent-blue'].click();const stopped=requests;await ctx.refresh();await ctx.refresh();assert.equal(requests,stopped,'auth failures never auto retry');
 authStatus=0;nodes.code.value='0123';await nodes.connect.click();delete state.accent;await ctx.refresh();const legacy=posts.length;
 await nodes['accent-purple'].click();assert.equal(posts.length,legacy,'older consoles keep local colours');
 console.log('Shared accent checks passed: pairing, live download, serial/debounced saves, stale polling, write failure/retry, other devices, reset and authentication.');
})().catch(e=>{console.error(e);process.exitCode=1;});

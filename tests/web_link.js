// Exercise the shipped UI script with DOM/network fixtures; no real downloads.
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
const state={loaded:false,busy:false,directSupported:true,directBusy:false,directTask:-1,directPhase:'idle',directMessage:'',phase:'idle',name:'',total:0,done:0,peers:0,message:'Envia um torrent.',free:null,files:[],storagePath:'/data/pkg'};
let networkError=false,lastPost=null,postHold=null,authStatus=0,requests=0;
const ctx=vm.createContext({
 document:{getElementById:id=>{assert.ok(nodes[id],id);return nodes[id];},createElement:()=>node(),body:node()},
 URL,URLSearchParams,AbortController,TextEncoder,location:{hash:'',origin:'http://192.168.1.10:8787',pathname:'/',search:''},history:{},navigator:{},window:{},matchMedia:()=>({matches:false}),
 performance:{now:()=>now},setInterval(){},setTimeout(){return 1;},clearTimeout(){},
 fetch:async(url,options)=>{
  requests++;if(authStatus)return {ok:false,status:authStatus,json:async()=>({error:'Codigo recusado'})};
  if(networkError)throw Error('Sem rede');
  if(options.method==='POST'){lastPost={url,options};if(postHold)await postHold;if(url==='/api/reset')Object.assign(state,{loaded:false,busy:false,total:0,done:0,phase:'idle'});return {ok:true,json:async()=>({ok:true})};}
  if(url==='/api/qr')return {ok:true,json:async()=>({size:21,modules:'0'.repeat(441)})};
  return {ok:true,json:async()=>({...state})};
 }
});
vm.runInContext(script,ctx);
const fire=async(id,event,e={})=>{if(nodes[id]['on'+event])await nodes[id]['on'+event](e);for(const fn of nodes[id].handlers[event]||[])await fn(e);};
const setURL=async value=>{nodes['pkg-url'].value=value;await fire('pkg-url','input');};
(async()=>{
 nodes.code.value='wrong';await nodes.connect.click();assert.match(nodes['pair-status'].textContent,/4 números/);
 nodes.code.value='0123';await nodes.connect.click();
 assert.equal(nodes['pair-form'].hidden,true);assert.equal(nodes['space-value'].textContent,'Indisponível');assert.equal(nodes.percent.textContent,'—');
 await setURL('https://example.org/demo.pkg');assert.equal(nodes['send-link'].disabled,false);
 state.free=0;await ctx.refresh();assert.equal(nodes['space-value'].textContent,'0 B');
 await nodes['tab-link'].click();assert.equal(nodes['panel-torrent'].hidden,true);assert.equal(nodes['tab-link'].attributes['aria-selected'],'true');
 await fire('tab-link','keydown',{key:'Home',preventDefault(){}});assert.equal(focused,'tab-torrent');assert.equal(nodes['panel-torrent'].hidden,false);
 await fire('tab-torrent','keydown',{key:'ArrowRight',preventDefault(){}});assert.equal(focused,'tab-magnet');assert.equal(nodes['panel-magnet'].hidden,false);
 nodes['magnet-url'].value='not a magnet';await fire('magnet-url','input');lastPost=null;await nodes['send-magnet'].click();assert.equal(lastPost,null);assert.match(nodes['magnet-error'].textContent,/magnet/);
 const magnet='magnet:?xt=urn:btih:'+('a'.repeat(40))+'&tr=udp%3A%2F%2Ftracker.example%3A80%2Fannounce';nodes['magnet-url'].value=' '+magnet+' ';await fire('magnet-url','input');await nodes['send-magnet'].click();assert.equal(lastPost.url,'/api/magnet');assert.equal(lastPost.options.body,magnet);
 Object.assign(state,{magnetPending:true,busy:true,loaded:false,phase:'metadata',name:'Demo magnet',message:'A obter os dados'});await ctx.refresh();assert.equal(nodes.empty.hidden,true);assert.equal(nodes['torrent-progress'].hidden,false);assert.equal(nodes['phase-label'].textContent,'A obter dados do magnet');assert.equal(nodes.pause.disabled,false);assert.equal(nodes.pause.textContent,'Cancelar procura');assert.equal(nodes.start.hidden,true);assert.equal(nodes['send-magnet'].disabled,true);assert.equal(nodes.upload.disabled,true);await nodes.pause.click();assert.equal(lastPost.url,'/api/pause');
 Object.assign(state,{busy:false,phase:'paused'});await ctx.refresh();assert.equal(nodes.reset.disabled,false);assert.equal(nodes['send-magnet'].disabled,false);Object.assign(state,{magnetPending:false,phase:'idle'});await ctx.refresh();
 const file={name:'demo.torrent',size:1024};nodes.torrent.files=[file];await fire('torrent','change');assert.equal(nodes.upload.disabled,false);
 await nodes.upload.click();assert.equal(lastPost.options.body,file);assert.equal(lastPost.options.headers['X-Harbor-Code'],'0123');
 nodes.torrent.files=[{name:'bad.txt',size:50}];await fire('torrent','change');assert.equal(nodes.upload.disabled,true);assert.match(nodes['upload-error'].textContent,/.torrent/);
 await fire('drop','drop',{preventDefault(){},dataTransfer:{files:[file]}});assert.equal(nodes.upload.disabled,false);
 state.directSupported=false;await ctx.refresh();assert.equal(nodes['send-link'].disabled,true);assert.match(nodes['link-status'].textContent,/PS4 real/);
 state.directSupported=true;state.directBusy=true;state.directPhase='checking';await ctx.refresh();assert.equal(nodes.upload.disabled,true);assert.equal(nodes['send-link'].disabled,true);
 state.directBusy=false;state.loaded=true;state.busy=true;state.name='Demo';state.phase='downloading';state.total=100000000;state.done=10000000;
 now=2000;await ctx.refresh();now=4500;state.done+=5000000;await ctx.refresh();assert.equal(nodes.speed.textContent,'2.0 MB/s');assert.equal(nodes.pause.disabled,false);assert.equal(nodes.reset.disabled,true);
 await nodes.pause.click();assert.equal(lastPost.url,'/api/pause');
 state.busy=false;state.phase='paused';await ctx.refresh();assert.equal(nodes.speed.textContent,'—');assert.equal(nodes.start.textContent,'Retomar e instalar');
 state.done=state.total;state.phase='downloaded';await ctx.refresh();assert.equal(nodes.install.disabled,false);assert.equal(nodes.start.hidden,true);
 state.directTask=42;state.directPhase='error';state.directMessage='Pedido registado, mas não iniciou';await ctx.refresh();
 assert.equal(nodes['link-heading'].textContent,'Requer atenção');assert.equal(nodes['link-result'].dataset.state,'error');
 await setURL('file:///data/pkg/demo.pkg');lastPost=null;await nodes['send-link'].click();assert.equal(lastPost,null);assert.match(nodes['link-error'].textContent,/HTTP/);
 await setURL(' https://example.org/download?token=A%2Bb&key=x ');
 let release;postHold=new Promise(resolve=>{release=resolve;});const sending=nodes['send-link'].click();await Promise.resolve();assert.equal(nodes['send-link'].disabled,true);assert.equal(nodes.upload.disabled,true);
 await nodes['send-link'].click();release();await sending;postHold=null;
 assert.equal(lastPost.url,'/api/pkg-url');assert.equal(lastPost.options.body,'https://example.org/download?token=A%2Bb&key=x');
 await nodes.reset.click();assert.equal(nodes['reset-confirm'].hidden,false);await nodes['cancel-reset'].click();assert.equal(nodes['reset-confirm'].hidden,true);
 await nodes.reset.click();await nodes['confirm-reset'].click();assert.equal(lastPost.url,'/api/reset');assert.equal(nodes.empty.hidden,false);assert.equal(nodes.upload.disabled,true);
 networkError=true;await ctx.refresh();for(const id of ['send-link','upload','reset','install','start'])assert.equal(nodes[id].disabled,true,id);assert.equal(nodes['pair-form'].hidden,false);
 networkError=false;await ctx.refresh();assert.equal(nodes['pair-form'].hidden,true);assert.equal(nodes.connection.textContent,'PS4 ligada');
 state.update={supported:false,busy:false,task:-1,available:false,ready:false,current:'0.1.10',phase:'idle'};await ctx.refresh();assert.equal(nodes['update-check'].disabled,true);
 state.update.supported=true;state.update.phase='channel-old';await ctx.refresh();assert.equal(nodes['update-label'].textContent,'Canal desatualizado');assert.equal(nodes['update-download'].hidden,true);assert.equal(nodes['update-install'].hidden,true);await nodes['update-check'].click();assert.equal(lastPost.url,'/api/update/check');
 Object.assign(state.update,{mode:'manual',available:true,version:'0.1.11',phase:'available',notes:'Melhorias'});await ctx.refresh();assert.equal(nodes['update-download'].hidden,false);assert.equal(nodes['update-install'].hidden,true);
 lastPost=null;await nodes['update-download'].click();assert.equal(lastPost,null);assert.equal(nodes['update-destination'].hidden,false);assert.equal(nodes['update-usb0'].disabled,false);await nodes['update-internal'].click();assert.equal(lastPost.url,'/api/update/download');assert.equal(lastPost.options.body,'internal');
 state.updateDestinations=[{id:'internal',available:true},{id:'usb0',available:true},{id:'usb1',available:false}];await ctx.refresh();await nodes['update-download'].click();assert.equal(nodes['update-usb0'].disabled,false);lastPost=null;await nodes['update-usb1'].click();assert.equal(lastPost.options.body,'usb1');await nodes['update-download'].click();await nodes['update-usb0'].click();assert.equal(lastPost.options.body,'usb0');
 await nodes['update-download'].click();await nodes['update-destination-cancel'].click();assert.equal(nodes['update-destination'].hidden,true);
 // Choosing or cancelling cleanup sends nothing; confirmation has an explicit destination.
 lastPost=null;await nodes['update-cleanup'].click();assert.equal(lastPost,null);assert.equal(nodes['update-cleanup-box'].hidden,false);await nodes['update-cleanup-cancel'].click();assert.equal(lastPost,null);assert.equal(nodes['update-cleanup-box'].hidden,true);
 for(const target of ['internal','usb0','usb1']){await nodes['update-cleanup'].click();nodes['update-cleanup-target'].value=target;await nodes['update-cleanup-confirm'].click();assert.equal(lastPost.url,'/api/update/cleanup');assert.equal(lastPost.options.body,target);assert.equal(nodes['update-cleanup-box'].hidden,true);}
 await nodes['update-cleanup'].click();lastPost=null;nodes['update-cleanup-target'].value='../usb0';await nodes['update-cleanup-confirm'].click();assert.equal(lastPost,null);await nodes['update-cleanup-cancel'].click();
 state.update.available=false;state.update.phase='current';state.updateCleanupMessage='Apagados 2 ficheiros antigos.';await ctx.refresh();assert.equal(nodes['update-cleanup'].disabled,false);assert.equal(nodes['update-cleanup-message'].textContent,state.updateCleanupMessage);state.update.available=true;
 Object.assign(state.update,{busy:true,phase:'downloading',done:100,size:200});await ctx.refresh();assert.equal(nodes['update-progress'].value,50);for(const id of ['update-check','update-download','update-install','update-cleanup','update-cleanup-confirm','upload','send-link'])assert.equal(nodes[id].disabled,true,id);
 Object.assign(state.update,{busy:false,ready:true,phase:'ready',done:200,message:'Atualizacao guardada em /data/pkg/h1pNoise-update-38.pkg. Instala manualmente.'});await ctx.refresh();assert.equal(nodes['update-download'].hidden,false);assert.equal(nodes['update-download'].textContent,'Guardar outra cópia');assert.equal(nodes['update-install'].hidden,true);assert.equal(nodes['update-install'].disabled,true);assert.match(nodes['update-message'].textContent,/Instala manualmente/);
 lastPost=null;await nodes['update-install'].click();await nodes['update-confirm-install'].click();assert.equal(lastPost,null);assert.equal(nodes['update-confirm'].hidden,true);
 state.update.mode='runtime';state.update.ready=false;state.update.phase='available';await ctx.refresh();assert.equal(nodes['update-cleanup'].disabled,true);assert.equal(nodes['update-download'].textContent,'Atualizar agora');await nodes['update-download'].click();assert.equal(lastPost.url,'/api/update/download');
 state.update.ready=true;state.update.phase='error';await ctx.refresh();assert.equal(nodes['update-install'].hidden,false);assert.equal(nodes['update-install'].disabled,false);await nodes['update-install'].click();assert.equal(lastPost.url,'/api/update/install');
 state.update.mode='installer-test';state.update.ready=false;state.update.phase='available';await ctx.refresh();assert.equal(nodes['update-download'].textContent,'Descarregar e instalar (teste)');assert.match(nodes['update-footnote'].textContent,/auxiliar/);await nodes['update-download'].click();assert.equal(lastPost.url,'/api/update/download');
 state.update.ready=true;state.update.phase='error';await ctx.refresh();assert.equal(nodes['update-install'].hidden,false);assert.equal(nodes['update-install'].disabled,false);assert.equal(nodes['update-install'].textContent,'Entregar ao Updater (teste)');await nodes['update-install'].click();assert.equal(lastPost.url,'/api/update/install');
 state.update.mode='direct-test';state.update.ready=false;state.update.phase='available';await ctx.refresh();assert.equal(nodes['update-download'].textContent,'Descarregar e instalar sem fechar (teste)');assert.match(nodes['update-footnote'].textContent,/mantendo a app aberta/);assert.match(nodes['update-footnote'].textContent,/Não precisa do Updater/);await nodes['update-download'].click();assert.equal(lastPost.url,'/api/update/download');
 state.update.ready=true;state.update.phase='error';state.update.installSent=true;await ctx.refresh();assert.equal(nodes['update-install'].disabled,true);assert.equal(nodes['update-check'].disabled,true);lastPost=null;await nodes['update-install'].click();assert.equal(lastPost,null);
 state.update.phase='installed';state.update.ready=false;state.update.available=false;await ctx.refresh();assert.equal(nodes['update-label'].textContent,'Nova versão instalada');assert.match(nodes['update-version'].textContent,/Versão em execução/);assert.match(nodes['update-version'].textContent,/Nova versão instalada/);
 state.update.installSent=false;state.update.ready=true;state.update.available=true;state.update.phase='error';await ctx.refresh();assert.equal(nodes['update-install'].textContent,'Instalar sem fechar (teste)');await nodes['update-install'].click();assert.equal(lastPost.url,'/api/update/install');
 state.update.mode='manual';await ctx.refresh();lastPost=null;await nodes['update-install'].click();assert.equal(lastPost,null);assert.equal(nodes['update-install'].hidden,true);
 state.update.task=42;state.update.phase='queued';await ctx.refresh();for(const id of ['update-check','update-install','send-link','upload','reset'])assert.equal(nodes[id].disabled,true,id);
 for(const status of [401,429]){authStatus=status;nodes.code.value='0123';await nodes.connect.click();const before=requests;await ctx.refresh();await ctx.refresh();assert.equal(requests,before,'authentication errors must stop automatic retries');authStatus=0;}
 console.log('UI checks passed, including stopped automatic retries after 401/429.');
})().catch(e=>{console.error(e);process.exitCode=1;});


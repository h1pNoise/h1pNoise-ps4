'use strict';
const $=id=>document.getElementById(id);
let code='',connected=false,polling=false,pending='',selectedFile=null,lastStatus=null,previous=null,toastTimer=0,filesKey='',qrCode='',editingCode=false;
let qrOpen=!matchMedia('(max-width: 680px)').matches;
const phases={idle:'Sem atividade',ready:'Pronto',metadata:'A obter dados do magnet',checking:'A verificar',trackers:'A procurar fontes',waiting:'À espera de fontes',downloading:'A descarregar',paused:'Em pausa',downloaded:'Download concluído',installing:'A instalar',installed:'Instalado',error:'Requer atenção'};
const size=n=>n>=1e9?(n/1e9).toFixed(2)+' GB':n>=1e6?(n/1e6).toFixed(1)+' MB':n>=1e3?(n/1e3).toFixed(1)+' kB':Math.max(0,n)+' B';
const duration=s=>s>=3600?Math.ceil(s/3600)+' h':s>=60?Math.ceil(s/60)+' min':Math.max(1,Math.ceil(s))+' s';
function text(id,s){if($(id).textContent!==s)$(id).textContent=s;}
function toast(s){text('toast',s);$('toast').hidden=false;clearTimeout(toastTimer);toastTimer=setTimeout(()=>{$('toast').hidden=true;},4000);}
function badge(id,type){$(id).className='badge'+(type?' '+type:'');}
function alertText(s){text('alert',s);}
async function api(path,body,post=false){
 const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),post?60000:15000);
 try{
  const r=await fetch('/api/'+path,{method:post?'POST':'GET',headers:{'X-Harbor-Code':code,...(body?{'Content-Type':'application/octet-stream'}:{})},body:body||undefined,signal:controller.signal});
  let d;try{d=await r.json();}catch(_){throw Error('A consola enviou uma resposta inválida. Volta a ligar.');}
  if(!r.ok){const error=Error(d.error||'Não foi possível concluir o pedido.');error.status=r.status;if(r.status===401||r.status===429)code='';throw error;}return d;
 }catch(e){if(e.name==='AbortError')throw Error('A consola demorou a responder. Confirma o estado antes de repetir o pedido.');throw e;}finally{clearTimeout(timer);}
}
function selectMode(mode,focus=false){
 for(const name of ['torrent','magnet','link']){const active=name===mode;$('tab-'+name).setAttribute('aria-selected',String(active));$('tab-'+name).tabIndex=active?0:-1;$('panel-'+name).hidden=!active;}
 if(focus)$('tab-'+mode).focus();
}
for(const name of ['torrent','magnet','link']){
 $('tab-'+name).onclick=()=>selectMode(name);
 $('tab-'+name).onkeydown=e=>{if(['ArrowLeft','ArrowRight','Home','End'].includes(e.key)){e.preventDefault();const modes=['torrent','magnet','link'],index=modes.indexOf(name);selectMode(e.key==='Home'?modes[0]:e.key==='End'?modes[2]:modes[(index+(e.key==='ArrowRight'?1:2))%3],true);}};
}
function controls(){
 const d=lastStatus||{},u=d.update||{},blocked=!connected||d.busy||d.directBusy||u.busy||u.task>=0||!!pending;
 $('upload').disabled=blocked||!selectedFile;$('reset').disabled=blocked||(!d.loaded&&!d.magnetPending);
 $('send-magnet').disabled=blocked||!$('magnet-url').value.trim();
 $('send-link').disabled=blocked||!d.directSupported||!$('pkg-url').value.trim();
 $('start').disabled=$('download').disabled=blocked||!d.loaded;
 $('pause').disabled=!connected||!!pending||!d.busy||d.phase==='installing';
 $('install').disabled=blocked||!d.loaded||d.done!==d.total||d.phase==='installed';
 $('confirm-reset').disabled=blocked||(!d.loaded&&!d.magnetPending);
 $('update-check').disabled=blocked||!u.supported;
 $('update-download').disabled=blocked||!u.supported||!u.available||u.ready;
 $('update-install').disabled=$('update-confirm-install').disabled=blocked||u.mode!=='runtime'||!u.ready;
}
function selectFile(file){
 text('upload-error','');
 if(!file)return;
 selectedFile=null;$('drop').classList.remove('selected');text('selected-name','Escolhe o teu ficheiro .torrent');text('selected-size','Arrasta para aqui ou procura um ficheiro');controls();
 if(!file.name.toLowerCase().endsWith('.torrent')){text('upload-error','Escolhe um ficheiro com a extensão .torrent.');return;}
 if(!file.size||file.size>8388608){text('upload-error','O torrent deve ter entre 1 byte e 8 MB.');return;}
 selectedFile=file;text('selected-name',file.name);text('selected-size',size(file.size)+' · clica para trocar');$('drop').classList.add('selected');controls();
}
$('torrent').onchange=()=>selectFile($('torrent').files[0]);
for(const event of ['dragenter','dragover'])$('drop').addEventListener(event,e=>{e.preventDefault();$('drop').classList.add('dragging');});
for(const event of ['dragleave','drop'])$('drop').addEventListener(event,e=>{e.preventDefault();$('drop').classList.remove('dragging');});
$('drop').addEventListener('drop',e=>{if(e.dataTransfer.files.length!==1){text('upload-error','Escolhe apenas um torrent de cada vez.');return;}selectFile(e.dataTransfer.files[0]);});
$('magnet-url').oninput=()=>{text('magnet-error','');controls();};
$('pkg-url').oninput=()=>{text('link-error','');controls();};
function qrVisibility(){
 $('qrbox').hidden=!connected||!qrOpen||qrCode!==code;
 $('qrbox').classList.toggle('expanded',qrOpen);
 $('toggle-qr').setAttribute('aria-expanded',String(qrOpen));text('toggle-qr',qrOpen?'Fechar QR':'Ver QR');
}
async function showQR(){
 try{
  const d=await api('qr'),scale=6,border=4,canvas=$('qr'),ctx=canvas.getContext('2d');
  if(!Number.isInteger(d.size)||d.size<21||d.size>37||typeof d.modules!=='string'||d.modules.length!==d.size*d.size||/[^01]/.test(d.modules))throw Error('Não foi possível gerar o QR.');
  canvas.width=canvas.height=(d.size+border*2)*scale;ctx.fillStyle='#ffffff';ctx.fillRect(0,0,canvas.width,canvas.height);ctx.fillStyle='#000000';
  for(let y=0;y<d.size;y++)for(let x=0;x<d.size;x++)if(d.modules[y*d.size+x]==='1')ctx.fillRect((x+border)*scale,(y+border)*scale,scale,scale);
  qrCode=code;text('connection-address',location.origin);qrVisibility();
 }catch(e){text('pair-status',e.message);toast(e.message);}
}
function render(d){
 lastStatus=d;connected=true;text('connection',d.directSupported?'PS4 ligada':'Modo de teste');badge('connection-badge','online');
 $('pair-form').hidden=!editingCode;
 $('pairing').classList.toggle('connected',!editingCode);$('connection-bottom').hidden=false;
 const hasTransfer=!!d.loaded||!!d.magnetPending;
 $('empty').hidden=hasTransfer;$('torrent-progress').hidden=!hasTransfer;
 text('phase-label',hasTransfer?(phases[d.phase]||'A aguardar'):'Sem atividade');
 badge('phase-badge',d.phase==='error'?'error':d.phase==='paused'||d.phase==='waiting'?'warn':d.busy?'online busy':hasTransfer?'online':'');
 text('name',d.name||'À espera de um torrent');
 const installing=d.phase==='installing',total=installing?d.installTotal:d.total,done=installing?d.installDone:d.done,pct=total?Math.min(100,100*done/total):0;
 text('percent',d.loaded?pct.toFixed(1)+'%':'—');$('progress').value=pct;
 text('bytes',d.magnetPending?'A obter os dados do torrent…':!d.loaded?'Nenhum torrent carregado':installing&&!total?'A preparar a instalação…':size(done)+' / '+size(total));
 const now=performance.now();let rate=0;
 if(d.phase==='downloading'&&previous&&previous.phase===d.phase&&previous.name===d.name&&d.done>=previous.done&&now>previous.time)rate=(d.done-previous.done)*1000/(now-previous.time);
 previous={time:now,done:d.done,phase:d.phase,name:d.name};
 text('speed',rate>0?size(rate)+'/s':'—');text('peers',hasTransfer?String(d.peers):'—');text('eta',rate>0&&d.total>d.done?duration((d.total-d.done)/rate):'—');
 text('message',d.message||'');$('message').classList.toggle('error',d.phase==='error');
 const count=(d.files||[]).length;text('file-count',d.magnetPending?'Os ficheiros aparecem após obter os dados':count+' '+(count===1?'ficheiro PKG':'ficheiros PKG'));text('files-summary','Ficheiros incluídos ('+count+')');
 $('files').parentElement.hidden=!!d.magnetPending;
 const key=JSON.stringify(d.files||[]);
 if(filesKey!==key){filesKey=key;$('files').replaceChildren();for(const f of d.files||[]){const li=document.createElement('li'),name=document.createElement('b'),bytes=document.createElement('span');name.textContent=f.name;bytes.textContent=size(f.size);li.append(name,bytes);$('files').append(li);}}
 const complete=d.loaded&&d.total>0&&d.done===d.total;
 $('start').hidden=$('download').hidden=d.busy||complete||!!d.magnetPending;$('pause').hidden=!d.busy||installing;
 $('install').hidden=!complete||d.busy||d.phase==='installed';
 text('start',d.phase==='paused'?'Retomar e instalar':'Descarregar e instalar');text('download',d.phase==='paused'?'Só retomar':'Só descarregar');
 if(d.magnetPending)text('pause','Cancelar procura');else text('pause','Pausar');
 const unknown=d.free===null||d.free===undefined;text('space-value',unknown?'Indisponível':size(d.free));$('space-value').classList.toggle('unknown',unknown);
 text('space',unknown?(d.allowUnknownSpace?'Medição indisponível. Podes descarregar; confirma o espaço no dispositivo.':'Não foi possível medir o espaço disponível.'):'A instalação precisa de espaço adicional ao download.');
 text('storage-path','Destino: '+(d.storagePath||'/data/pkg'));
 text('mobile-space',(unknown?'Espaço livre: medição indisponível.':'Espaço livre: '+size(d.free)+'.')+' Destino: '+(d.storagePath||'/data/pkg'));
 const dp=d.directPhase||'idle';$('link-result').dataset.state=dp;
 text('link-heading',!d.directSupported?'Disponível na PS4 real':dp==='checking'?'A verificar o link':dp==='queued'?'Enviado para a PS4':dp==='error'?'Requer atenção':'Pronto para receber');
 badge('link-badge',dp==='error'?'error':dp==='checking'?'online busy':dp==='queued'?'online':'');
 text('link-status',!d.directSupported?'O envio para as Transferências requer uma PS4 real. No PC e no shadPS4 podes testar a página e os torrents.':d.directMessage||'Envia o link e acompanha o pedido em Notificações → Transferências na PS4.');
 const u=d.update||{};
 const updateLabels={idle:'Por verificar',checking:'A procurar',current:'Atualizada','channel-old':'Canal desatualizado',available:'Nova versão',downloading:'A descarregar',ready:'Pronta para instalar',installing:'A preparar instalação',queued:'Enviada para a PS4',error:'Requer atenção'};
 text('update-label',u.supported?(updateLabels[u.phase]||'Por verificar'):'PS4 real');
 badge('update-badge',u.phase==='error'||u.phase==='channel-old'?'warn':u.available?'online':'');
 text('update-version','Versão instalada: '+(u.current||'0.1.10')+(u.available?' · Disponível: '+u.version:''));
 text('update-message',u.message||'As atualizações da aplicação requerem uma PS4 real.');
 text('update-notes',u.notes||'');$('update-notes').hidden=!u.available||!u.notes;
 $('update-download').hidden=!u.available||u.ready||u.busy||u.task>=0;
 $('update-install').hidden=u.mode!=='runtime'||!u.ready||u.busy; $('update-confirm').hidden=true;
 text('update-download',u.mode==='runtime'?'Atualizar agora':'Descarregar atualização');
 text('update-install','Reiniciar com a nova versão');
 text('update-footnote',u.mode==='runtime'?'Também podes carregar X no comando da PS4. A app descarrega, verifica e reinicia.':'A instalação desta versão é manual.');
 $('update-progress').hidden=$('update-bytes').hidden=!u.size||(!u.ready&&u.phase!=='downloading');
 $('update-progress').value=u.size?Math.min(100,100*u.done/u.size):0;text('update-bytes',size(u.done||0)+' / '+size(u.size||0));
 controls();
}
async function refresh(){
 if(!code||polling)return;polling=true;const requestCode=code;
 try{const d=await api('status');if(code===requestCode)render(d);}
 catch(e){connected=false;previous=null;text('connection','Sem ligação');badge('connection-badge','warn');$('pairing').classList.remove('connected');$('pair-form').hidden=false;$('qrbox').hidden=true;qrCode='';text('pair-status',e.message);controls();}
 finally{polling=false;}
}
$('connect').onclick=async()=>{
 const entered=$('code').value.trim().toLowerCase();
 if(!/^[0-9]{4}$/.test(entered)){text('pair-status','Introduz os 4 números que aparecem na televisão.');return;}
 if(polling)return;code=entered;connected=false;previous=null;editingCode=false;$('connect').disabled=true;controls();text('pair-status','A ligar à consola…');
 await refresh();
 if(connected){$('pair-form').hidden=true;text('pair-status','Ligação estabelecida.');toast('Ligado ao dispositivo.');await showQR();}
 $('connect').disabled=false;
};
$('code').onkeydown=e=>{if(e.key==='Enter')$('connect').click();};
$('change-code').onclick=()=>{editingCode=!editingCode;$('pair-form').hidden=!editingCode;$('pairing').classList.toggle('connected',!editingCode&&connected);if(editingCode)$('code').focus();};
$('toggle-qr').onclick=async()=>{qrOpen=!qrOpen;if(qrCode!==code)await showQR();qrVisibility();};
$('copy-address').onclick=async()=>{
 try{
  if(navigator.clipboard&&window.isSecureContext)await navigator.clipboard.writeText(location.origin);
  else{const field=document.createElement('textarea');field.value=location.origin;field.style.position='fixed';field.style.opacity='0';document.body.append(field);field.select();const ok=document.execCommand('copy');field.remove();if(!ok)throw Error();}
  toast('Endereço copiado.');
 }catch(_){toast('Abre o endereço apresentado por baixo do QR.');}
};
async function action(path,body,errorId='alert'){
 if(pending)return false;pending=path;controls();text(errorId,'');
 try{await api(path,body,true);return true;}catch(e){text(errorId,e.message);return false;}
 finally{pending='';await refresh();controls();}
}
$('upload').onclick=async()=>{
 if(!selectedFile||!connected||$('upload').disabled)return;
 if(await action('torrent',selectedFile,'upload-error')){toast('Torrent recebido. Pronto para começar.');$('transfer').scrollIntoView({behavior:matchMedia('(prefers-reduced-motion: reduce)').matches?'auto':'smooth',block:'nearest'});}
};
$('send-magnet').onclick=async()=>{
 if($('send-magnet').disabled||!connected)return;
 const url=$('magnet-url').value.trim();
 if(!url.startsWith('magnet:?')){text('magnet-error','Cola uma ligação que comece por magnet:?');return;}
 if(new TextEncoder().encode(url).length>=32768){text('magnet-error','O magnet é demasiado longo.');return;}
 if(await action('magnet',url,'magnet-error')){toast('Magnet recebido. A procurar os dados do torrent.');$('transfer').scrollIntoView({behavior:matchMedia('(prefers-reduced-motion: reduce)').matches?'auto':'smooth',block:'nearest'});}
};
$('send-link').onclick=async()=>{
 if($('send-link').disabled||!connected)return;
 const url=$('pkg-url').value.trim();let parsed;try{parsed=new URL(url);}catch(_){}
 if(!parsed||!['http:','https:'].includes(parsed.protocol)){text('link-error','Cola um link direto HTTP ou HTTPS para o PKG.');return;}
 if(await action('pkg-url',url,'link-error'))toast('Link recebido. A consola está a verificá-lo.');
};
for(const [id,path] of [['start','download-install'],['download','download'],['pause','pause'],['install','install']])$(id).onclick=async()=>{if(!$(id).disabled)await action(path);};
$('reset').onclick=()=>{$('reset-confirm').hidden=false;$('confirm-reset').focus();};
$('cancel-reset').onclick=()=>{$('reset-confirm').hidden=true;$('reset').focus();};
$('confirm-reset').onclick=async()=>{
 if($('confirm-reset').disabled)return;
 if(await action('reset')){$('reset-confirm').hidden=true;selectedFile=null;$('torrent').value='';$('magnet-url').value='';text('magnet-error','');$('drop').classList.remove('selected');text('selected-name','Escolhe o teu ficheiro .torrent');text('selected-size','Arrasta para aqui ou procura um ficheiro');controls();toast('Pronto para outra transferência.');}
};
$('update-check').onclick=async()=>{if(!$('update-check').disabled)await action('update/check',null,'update-error');};
$('update-download').onclick=async()=>{if(!$('update-download').disabled)await action('update/download',null,'update-error');};
$('update-install').onclick=async()=>{if(!$('update-install').disabled)await action('update/install',null,'update-error');};
$('update-cancel').onclick=()=>{$('update-confirm').hidden=true;};
$('update-confirm-install').onclick=()=>{};
const pairingCode=new URLSearchParams(location.hash.slice(1)).get('code');
if(pairingCode&&/^[0-9]{4}$/.test(pairingCode)){$('code').value=pairingCode;history.replaceState(null,'',location.pathname+location.search);$('connect').onclick();}
setInterval(refresh,2500);



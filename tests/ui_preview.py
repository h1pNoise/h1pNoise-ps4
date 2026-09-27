"""Loopback-only interactive UI preview. Never downloads or installs anything."""
from pathlib import Path
from http.server import ThreadingHTTPServer,BaseHTTPRequestHandler
import json,time,threading,urllib.parse,cv2
ROOT=Path(__file__).resolve().parents[1]
PIN='0123456789abcdef'
STATE={};LOCK=threading.Lock()
def scenario(name):
 STATE.clear();STATE.update(name='',loaded=False,busy=False,directSupported=True,directBusy=False,directTask=-1,directPhase='idle',directMessage='',phase='idle',total=0,done=0,installTotal=0,installDone=0,peers=0,message='',free=186400000000,storagePath='/data/pkg',files=[],version='0.1.9 preview',last=time.monotonic())
 STATE['version']='0.1.10 preview'
 STATE['update']=dict(supported=True,busy=False,available=False,ready=False,task=-1,current='0.1.10',phase='current',message='Demonstração: tens a versão mais recente.',notes='',version='',done=0,size=0)
 if name=='update':STATE['update'].update(available=True,phase='available',version='0.1.11',size=6619136,message='Uma nova versão está disponível. Dados simulados.',notes='Exemplo de uma futura atualização com melhorias na aplicação.')
 if name in ['download','paused','error','complete']:
  STATE.update(name='Demo Homebrew Collection',loaded=True,busy=name=='download',phase={'download':'downloading','paused':'paused','error':'error','complete':'downloaded'}[name],total=2590000000,done=1050000000,peers=12,files=[dict(name='Demo Homebrew.pkg',size=2590000000)],message='Demonstração visual. Nenhum ficheiro está a ser descarregado.')
  if name=='complete':STATE['done']=STATE['total']
  if name=='error':STATE.update(message='Exemplo de erro: não foi possível contactar as fontes. Tenta novamente.',free=None)
 if name=='link':STATE.update(directPhase='queued',directTask=42,directMessage='Exemplo: pedido enviado para as Transferências da PS4. Esta pré-visualização não criou uma transferência real.')
scenario('idle')
BAR='''<div style="padding:10px 16px;background:#26392f;border-bottom:1px solid #456051;color:#d8f8e8;font:12px system-ui;display:flex;align-items:center;gap:14px;flex-wrap:wrap"><strong>PRÉ-VISUALIZAÇÃO · sem downloads reais</strong><label>Estado <select id="demo-scene" style="background:#14221a;color:white;border:1px solid #496453;border-radius:5px;padding:5px"><option value="idle">Sem torrent</option><option value="download">A descarregar</option><option value="paused">Em pausa</option><option value="error">Erro</option><option value="complete">Concluído</option><option value="link">Link enviado</option></select></label><a href="/mobile" style="color:inherit">Ver no telemóvel</a><a href="/screen" style="color:inherit">Ecrã da PS4</a></div>'''
DEMO="""<script>document.getElementById('demo-scene').onchange=async function(){await fetch('/demo',{method:'POST',body:this.value});await refresh();if(this.value==='link')selectMode('link');else selectMode('torrent');};document.getElementById('code').value='0123456789abcdef';document.getElementById('connect').click();</script>"""
class Handler(BaseHTTPRequestHandler):
 def log_message(self,*args):pass
 def send(self,status,kind,data):
  if isinstance(data,str):data=data.encode()
  self.send_response(status);self.send_header('Content-Type',kind);self.send_header('Content-Length',str(len(data)));self.send_header('Cache-Control','no-store');self.end_headers();self.wfile.write(data)
 def data(self,obj,status=200):self.send(status,'application/json',json.dumps(obj,ensure_ascii=False))
 def do_GET(self):
  path=urllib.parse.urlsplit(self.path).path
  if path in ['/','/app']:
   html=(ROOT/'web/index.html').read_text(encoding='utf-8').replace('<body>','<body>'+BAR).replace('</body>',DEMO+'</body>');self.send(200,'text/html; charset=utf-8',html)
  elif path in ['/app.css','/app.js']:self.send(200,'text/css' if path.endswith('css') else 'text/javascript',(ROOT/'web'/path[1:]).read_bytes())
  elif path=='/icon.png':self.send(200,'image/png',(ROOT/'assets/icon0.png').read_bytes())
  elif path=='/mobile':self.send(200,'text/html; charset=utf-8','<html style="background:#080b0d;color:white;font:14px system-ui"><p style="text-align:center">Pré-visualização a 390 px · <a href="/" style="color:#9debcf">Voltar</a></p><iframe title="Interface móvel" src="/app" style="display:block;width:390px;height:850px;max-width:100%;margin:auto;border:1px solid #47534d;border-radius:20px"></iframe></html>')
  elif path=='/screen':self.send(200,'text/html; charset=utf-8','<html style="background:#080b0d;color:white;font:14px system-ui"><p>Ecrã renderizado pelo código da PS4 · <a href="/" style="color:#9debcf">Voltar à página interativa</a></p><img style="width:100%;max-width:1280px" src="/screen.png"></html>')
  elif path=='/screen.png':
   p=ROOT/'build/ui-native-download.png'
   if p.exists():self.send(200,'image/png',p.read_bytes())
   else:self.send(404,'text/plain','A preparar a imagem.')
  elif path=='/api/status':
   with LOCK:
    if STATE['phase']=='downloading':
     now=time.monotonic();STATE['done']=min(STATE['total'],STATE['done']+int((now-STATE['last'])*12000000));STATE['last']=now
     if STATE['done']==STATE['total']:STATE.update(phase='downloaded',busy=False)
    result={k:v for k,v in STATE.items() if k!='last'}
   self.data(result)
  elif path=='/api/qr':
   matrix=cv2.QRCodeEncoder_create().encode('http://127.0.0.1:8899/#code='+PIN)[2:-2,2:-2]
   self.data(dict(size=matrix.shape[0],modules=''.join('1' if x==0 else '0' for x in matrix.flatten())))
  else:self.send(404,'text/plain','Not found')
 def do_POST(self):
  length=int(self.headers.get('Content-Length','0'))
  if length>8388608:return self.data({'error':'Ficheiro demasiado grande.'},413)
  body=self.rfile.read(length)
  with LOCK:
   if self.path=='/demo':scenario(body.decode())
   elif self.headers.get('X-Harbor-Code')!=PIN:return self.data({'error':'Código de demonstração inválido.'},401)
   elif self.path=='/api/update/check':scenario('update')
   elif self.path=='/api/update/download':STATE['update'].update(ready=True,phase='ready',done=STATE['update']['size'],message='Exemplo: ficheiro descarregado e verificado. Nenhum download real foi feito.')
   elif self.path=='/api/update/install':STATE['update'].update(task=43,phase='queued',message='Exemplo: pedido de atualização enviado. Nenhuma instalação real foi feita.')
   elif self.path=='/api/torrent':scenario('paused');STATE.update(phase='ready',done=0,message='Torrent de exemplo recebido. Os dados são simulados.')
   elif self.path in ['/api/download','/api/download-install']:STATE.update(phase='downloading',busy=True,last=time.monotonic())
   elif self.path=='/api/pause':STATE.update(phase='paused',busy=False,message='Transferência de exemplo em pausa.')
   elif self.path=='/api/reset':scenario('idle')
   elif self.path=='/api/install':STATE.update(phase='installed',busy=False,message='Exemplo de instalação concluída. Nenhum PKG foi instalado.')
   elif self.path=='/api/pkg-url':STATE.update(directPhase='queued',directTask=42,directMessage='Link recebido na demonstração. Nenhum download ou instalação foi iniciado.')
   else:return self.data({'error':'Pedido desconhecido.'},404)
  self.data({'ok':True})
if __name__=='__main__':ThreadingHTTPServer(('127.0.0.1',8899),Handler).serve_forever()

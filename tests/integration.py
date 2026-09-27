"""Local synthetic tracker + BitTorrent peer. Never contacts public trackers."""
import hashlib, http.client, http.server, json, os, socket, socketserver, struct
import subprocess, threading, time, tempfile, pathlib, unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]
def enc(x):
    if isinstance(x,int): return b'i'+str(x).encode()+b'e'
    if isinstance(x,str): x=x.encode()
    if isinstance(x,bytes): return str(len(x)).encode()+b':'+x
    if isinstance(x,list): return b'l'+b''.join(map(enc,x))+b'e'
    return b'd'+b''.join(enc(k)+enc(v) for k,v in sorted(x.items()))+b'e'
def exact(s,n):
    result=b''
    while len(result)<n:
        q=s.recv(n-len(result))
        if not q: raise EOFError()
        result+=q
    return result
def wire(s,p):s.sendall(struct.pack('!I',len(p))+p)

class Seed(socketserver.BaseRequestHandler):
    def handle(self):
        s=self.request;s.settimeout(12)
        try:
            h=exact(s,68)
            if h[28:48]!=self.server.ih:return
            s.sendall(h[:48]+b'-TEST00-localfixture')
            count=len(self.server.hashes);bits=bytearray((count+7)//8)
            for i in range(count):bits[i//8]|=1<<(7-i%8)
            wire(s,b'\x05'+bits);wire(s,b'\x01')
            while True:
                n=struct.unpack('!I',exact(s,4))[0];p=exact(s,n)
                if p and p[0]==6:
                    piece,offset,size=struct.unpack('!III',p[1:]);start=piece*self.server.pl+offset
                    if self.server.delay:time.sleep(self.server.delay)
                    data=self.server.data[start:start+size]
                    if self.server.corrupt:data=bytes([data[0]^255])+data[1:]
                    wire(s,b'\x07'+struct.pack('!II',piece,offset)+data)
                    self.server.requests.append((piece,offset,size))
        except (EOFError,OSError):pass
class TCP(socketserver.ThreadingTCPServer):allow_reuse_address=True;daemon_threads=True
class UDPTracker(socketserver.BaseRequestHandler):
    def handle(self):
        p,s=self.request
        if len(p)==16:s.sendto(struct.pack('!IIQ',0,struct.unpack('!I',p[12:16])[0],123456),self.client_address)
        elif len(p)==98:s.sendto(struct.pack('!IIIII',1,struct.unpack('!I',p[12:16])[0],60,0,1)+socket.inet_aton('127.0.0.1')+struct.pack('!H',self.server.seedport),self.client_address)
class Tracker(http.server.BaseHTTPRequestHandler):
    def log_message(self,*a):pass
    def do_GET(self):
        data=enc({b'interval':60,b'peers':socket.inet_aton('127.0.0.1')+struct.pack('!H',self.server.seedport)})
        self.send_response(200)
        if self.server.chunked:
            self.send_header('Transfer-Encoding','chunked');self.end_headers()
            for start in range(0,len(data),7):
                chunk=data[start:start+7];self.wfile.write(('%x\r\n'%len(chunk)).encode()+chunk+b'\r\n')
            self.wfile.write(b'0\r\n\r\n')
        else:self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)

class Integration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory(dir=ROOT/'build');cls.data_dir=pathlib.Path(cls.tmp.name)
        cls.seed=TCP(('127.0.0.1',0),Seed);cls.seed.pl=65536;cls.seed.delay=0;cls.seed.requests=[];cls.seed.corrupt=False
        cls.seed.data=bytes((i*37+5)%256 for i in range(900123))
        cls.seed.hashes=[hashlib.sha1(cls.seed.data[i:i+cls.seed.pl]).digest() for i in range(0,len(cls.seed.data),cls.seed.pl)]
        cls.tracker=http.server.ThreadingHTTPServer(('127.0.0.1',0),Tracker);cls.tracker.seedport=cls.seed.server_address[1];cls.tracker.chunked=False
        cls.udp=socketserver.UDPServer(('127.0.0.1',0),UDPTracker);cls.udp.seedport=cls.seed.server_address[1]
        for server in [cls.seed,cls.tracker,cls.udp]:threading.Thread(target=server.serve_forever,daemon=True).start()
        cls.info={b'files':[{b'length':123457,b'path':[b'base.pkg']},{b'length':len(cls.seed.data)-123457,b'path':[b'patch.pkg']}],b'name':b'Local synthetic test',b'piece length':cls.seed.pl,b'pieces':b''.join(cls.seed.hashes)}
        cls.seed.ih=hashlib.sha1(enc(cls.info)).digest();cls.ih=cls.seed.ih.hex()
        cls.torrent=enc({b'announce':f'http://127.0.0.1:{cls.tracker.server_port}/announce'.encode(),b'info':cls.info})
        sock=socket.socket();sock.bind(('127.0.0.1',0));cls.port=sock.getsockname()[1];sock.close()
        cls.launch()
    @classmethod
    def launch(cls):
        cls.proc=subprocess.Popen([str(ROOT/'build/harbor-host.exe'),str(cls.data_dir),str(cls.port)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
        line=cls.proc.stdout.readline();assert 'CODE' in line,line;cls.code=line.split()[-1]
        for _ in range(30):
            try:
                s=socket.create_connection(('127.0.0.1',cls.port),.2);s.close();break
            except OSError:time.sleep(.1)
    @classmethod
    def shutdown(cls):
        cls.proc.terminate();cls.proc.wait(timeout=10);cls.proc.stdout.close();cls.proc.stderr.close()
    @classmethod
    def tearDownClass(cls):
        cls.shutdown()
        for server in [cls.seed,cls.tracker,cls.udp]:server.shutdown();server.server_close()
        cls.tmp.cleanup()
    def api(self,path,body=None,method='GET',extra=None,auth=True):
        c=http.client.HTTPConnection('127.0.0.1',self.port,timeout=10);headers={'X-Harbor-Code':self.code} if auth else {}
        headers.update(extra or {});c.request(method,path,body=body,headers=headers);r=c.getresponse();data=r.read();code=r.status;c.close()
        try:data=json.loads(data)
        except ValueError:pass
        return code,data
    def command(self,p):
        status,data=self.api('/api/'+p,method='POST');self.assertEqual(status,200,data)
    def wait(self,phase,timeout=15):
        until=time.monotonic()+timeout
        while time.monotonic()<until:
            status,s=self.api('/api/status');self.assertEqual(status,200)
            if s['phase']==phase and not s['busy']:return s
            time.sleep(.1)
        self.fail(s)
    def test_01_auth_and_http(self):
        self.assertEqual(self.api('/api/status',auth=False)[0],401)
        self.assertEqual(self.api('/api/status',extra={'Host':'evil.example'})[0],403)
        self.assertEqual(self.api('/api/pause',method='POST',extra={'Origin':'https://evil.example'})[0],403)
        self.assertEqual(self.api('/')[0],200)
        self.assertEqual(self.api('/api/install',method='POST')[0],400)
    def test_02_reject_malformed(self):
        for value in [b'',b'de',b'd4:infodee',b'9999999999999:x',b'li01ee',b'l'*30+b'e'*30]:
            self.assertEqual(self.api('/api/torrent',value,'POST')[0],400)
        info=dict(self.info);info[b'files']=[{b'length':len(self.seed.data),b'path':[b'..',b'base.pkg']}]
        self.assertEqual(self.api('/api/torrent',enc({b'announce':b'http://127.0.0.1/a',b'info':info}),'POST')[0],400)

    def test_storage_status(self):
        status,data=self.api('/api/status')
        self.assertEqual(status,200)
        self.assertIsInstance(data['free'],int)
        self.assertGreater(data['free'],0)
        self.assertEqual(data['storagePath'],str(self.data_dir))

    def test_storage_unavailable(self):
        # A missing parent makes mkdir fail; the host must report unknown, not zero.
        probe=socket.socket();probe.bind(('127.0.0.1',0));port=probe.getsockname()[1];probe.close()
        missing=self.data_dir/'not-created'/'storage'
        proc=subprocess.Popen([str(ROOT/'build/harbor-host.exe'),str(missing),str(port)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
        try:
            line=proc.stdout.readline();self.assertIn('CODE',line);pin=line.split()[-1]
            for attempt in range(30):
                try:
                    conn=http.client.HTTPConnection('127.0.0.1',port,timeout=2)
                    conn.request('GET','/api/status',headers={'X-Harbor-Code':pin})
                    response=conn.getresponse();data=json.loads(response.read());conn.close();break
                except OSError:
                    if attempt==29:raise
                    time.sleep(.1)
            self.assertEqual(response.status,200)
            self.assertIsNone(data['free'])
        finally:
            proc.terminate();proc.wait(timeout=10);proc.stdout.close();proc.stderr.close()

    def test_pairing_qr(self):
        self.assertEqual(self.api('/api/qr',auth=False)[0],401)
        status,data=self.api('/api/qr')
        self.assertEqual(status,200,data)
        self.assertIn(data['size'],[21,25,29,33,37])
        self.assertEqual(len(data['modules']),data['size']**2)
        self.assertLessEqual(set(data['modules']),{'0','1'})
        # Independent decoder checks the URL and session token encoded by the C library.
        import cv2, numpy as np
        matrix=np.array([0 if x=='1' else 255 for x in data['modules']],dtype=np.uint8).reshape(data['size'],data['size'])
        matrix=np.repeat(np.repeat(np.pad(matrix,4,constant_values=255),7,axis=0),7,axis=1)
        decoded,points,_=cv2.QRCodeDetector().detectAndDecode(matrix)
        self.assertIsNotNone(points)
        self.assertTrue(decoded.startswith('http://'),decoded)
        self.assertTrue(decoded.endswith(f':{self.port}/#code={self.code}'),decoded)
        self.assertEqual(self.api('/')[0],200)
        info=dict(self.info);info[b'pieces']=b'x'*20
        self.assertEqual(self.api('/api/torrent',enc({b'announce':b'http://127.0.0.1/a',b'info':info}),'POST')[0],400)
    def test_03_download_multifile_and_integrity(self):
        self.assertEqual(self.api('/api/torrent',self.torrent,'POST')[0],200)
        self.command('download');s=self.wait('downloaded');self.assertEqual(s['done'],len(self.seed.data))
        folder=self.data_dir/self.ih
        self.assertEqual((folder/'file00.pkg').read_bytes(),self.seed.data[:123457])
        self.assertEqual((folder/'file01.pkg').read_bytes(),self.seed.data[123457:])
        self.assertGreater(len(self.seed.requests),len(self.seed.hashes))
    def test_04_resume_and_chunked_tracker(self):
        self.seed.requests.clear();self.tracker.chunked=True
        folder=self.data_dir/self.ih
        with (folder/'file01.pkg').open('r+b') as f:f.seek(180000);f.write(b'corrupt!')
        self.command('download');self.wait('downloaded')
        requested={piece for piece,_,_ in self.seed.requests}
        self.assertEqual(requested,{(123457+180000)//65536})
        self.assertEqual((folder/'file01.pkg').read_bytes(),self.seed.data[123457:])
    def test_05_reopen_and_verify(self):
        self.shutdown();self.launch()
        s=self.api('/api/status')[1];self.assertTrue(s['loaded']);self.assertEqual(s['done'],0)
        self.seed.requests.clear();self.command('download');self.wait('downloaded');self.assertEqual(self.seed.requests,[])
    def test_06_invalid_pkg_not_installed(self):
        self.command('install');s=self.wait('error');self.assertIn('cabecalho',s['message'])
    def test_07_pause(self):
        folder=self.data_dir/self.ih
        (folder/'file00.pkg').write_bytes(b'');(folder/'file01.pkg').write_bytes(b'')
        self.seed.delay=.03;self.command('download');time.sleep(.2);self.command('pause');self.wait('paused');self.seed.delay=0
        self.command('download');self.wait('downloaded')
    def test_08_udp_tracker(self):
        torrent=enc({b'announce':f'udp://127.0.0.1:{self.udp.server_address[1]}/announce'.encode(),b'info':self.info})
        self.assertEqual(self.api('/api/torrent',torrent,'POST')[0],200)
        (self.data_dir/self.ih/'file00.pkg').write_bytes(b'')
        self.command('download');self.wait('downloaded')
        self.assertEqual((self.data_dir/self.ih/'file00.pkg').read_bytes(),self.seed.data[:123457])
    def test_09_bad_peer_cannot_commit_piece(self):
        (self.data_dir/self.ih/'file00.pkg').write_bytes(b'');(self.data_dir/self.ih/'file01.pkg').write_bytes(b'')
        self.seed.corrupt=True;self.command('download')
        until=time.monotonic()+10
        while time.monotonic()<until:
            s=self.api('/api/status')[1]
            if s['phase']=='waiting':break
            time.sleep(.1)
        self.assertEqual(s['done'],0);self.command('pause');self.wait('paused');self.seed.corrupt=False
        self.command('download');self.wait('downloaded')
    def test_10_large_metadata_uses_64_bits(self):
        total=52_264_632_320;pl=4_194_304;pieces=(total+pl-1)//pl
        info={b'length':total,b'name':b'large.pkg',b'piece length':pl,b'pieces':b'x'*(pieces*20)}
        torrent=enc({b'announce':b'http://127.0.0.1/announce',b'info':info})
        self.assertEqual(self.api('/api/torrent',torrent,'POST')[0],200)
        s=self.api('/api/status')[1];self.assertEqual(s['total'],total);self.assertEqual(s['files'][0]['size'],total)

if __name__=='__main__':unittest.main(verbosity=2)

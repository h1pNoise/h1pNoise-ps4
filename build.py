"""Build the current PS4 PKG or Windows test host. No proprietary Sony SDK required."""
from pathlib import Path
import argparse, os, subprocess, shutil, re

ROOT = Path(__file__).resolve().parent
VERSION = dict(re.findall(r'^#define\s+(APP_\w+)\s+"([^"]+)"', (ROOT / 'src/version.h').read_text(), re.M))

def run(args, cwd=None):
    subprocess.run([str(a) for a in args], cwd=cwd or ROOT, check=True)

def embed():
    page = (ROOT / 'web/index.html').read_text(encoding='utf-8')
    page = page.replace('id="app-version">h1pNoise', 'id="app-version">VERSÃO '+VERSION['APP_VERSION'])
    page = page.replace('id="footer-version">h1pNoise', 'id="footer-version">h1pNoise '+VERSION['APP_VERSION'])
    page = page.replace('<link rel="stylesheet" href="/app.css">', '<style>'+ (ROOT/'web/app.css').read_text(encoding='utf-8')+'</style>')
    page = page.replace('<script src="/app.js"></script>', '<script>'+ (ROOT/'web/app.js').read_text(encoding='utf-8')+'</script>')
    raw = page.encode('utf-8')
    lines = ['static const char web_html[] =']
    for i in range(0,len(raw),80):
        lines.append('"'+''.join('\\%03o'%b for b in raw[i:i+80])+'"')
    icon=(ROOT/'assets/icon0.png').read_bytes()
    lines.append(';\nstatic const unsigned char web_icon[] = {')
    for i in range(0,len(icon),32):lines.append(','.join(str(b) for b in icon[i:i+32])+',')
    (ROOT / 'src/web.h').write_text('\n'.join(lines)+'};\n')

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--host',action='store_true')
    p.add_argument('--shadps4',action='store_true',help='Emulator-only socket ABI compatibility; PKG installation disabled')
    p.add_argument('--sdk',default=os.getenv('OO_PS4_TOOLCHAIN'))
    p.add_argument('--llvm',default=os.getenv('LLVM_BIN'))
    p.add_argument('--zig',default=os.getenv('ZIG_EXE','zig'))
    a=p.parse_args();embed()
    if a.host and a.shadps4:p.error('--host and --shadps4 are separate targets')
    build=ROOT/('build-shadps4' if a.shadps4 else 'build');build.mkdir(exist_ok=True)
    sources=[ROOT/'src'/x for x in ['core.c','magnet.c','appearance.c','platform.c','storage.c','pkg_validation.c','remote_pkg.c','engine.c','server.c','main.c','pairing.c','vendor/qrcodegen.c','updater.c','update_destination.c','update_http.c','update_platform.c','update_manifest.c','update_file.c','vendor/monocypher.c','vendor/monocypher-ed25519.c']]
    if a.host:
        os.environ.setdefault('ZIG_GLOBAL_CACHE_DIR',str(build/'zig-global'))
        os.environ.setdefault('ZIG_LOCAL_CACHE_DIR',str(build/'zig-local'))
        compiler=a.zig if Path(a.zig).exists() else os.getenv('CLANG_EXE','clang')
        command=[compiler]+(['cc'] if Path(compiler).stem=='zig' else [])
        run([*command,'-O1','-g','-Wall','-Wextra','-Wno-misleading-indentation','-o',build/'harbor-host.exe',*sources,'-lws2_32','-lbcrypt'])
        return
    if not a.sdk or not a.llvm: p.error('Set --sdk and --llvm (or OO_PS4_TOOLCHAIN and LLVM_BIN).')
    sdk=Path(a.sdk).resolve();llvm=Path(a.llvm).resolve()
    os.environ['OO_PS4_TOOLCHAIN']=str(sdk)
    sources.extend([ROOT/'src/ps4.c',ROOT/'src/ps4_remote.c',ROOT/'src/ps4_storage.c',ROOT/'src/display.c']);objects=[]
    if not a.shadps4:
        sources.extend([ROOT/'src/ps4_user.c',ROOT/'src/ps4_bgft.c',ROOT/'src/installer_access.c',ROOT/'src/vendor/libjbc/jailbreak.c',ROOT/'src/vendor/libjbc/kernelrw.c'])
    for source in sources:
        obj=build/(source.stem+'.o');objects.append(obj)
        defines=['-D__ORBIS__']+(['-DHARBOR_SHADPS4'] if a.shadps4 else [])
        run([llvm/'clang.exe','--target=x86_64-pc-freebsd12-elf',*defines,'-O2','-fPIC','-funwind-tables','-Wall','-Wextra','-Wno-misleading-indentation','-isysroot',sdk,'-isystem',sdk/'include','-c',source,'-o',obj])
    elf=build/'harbor.elf'
    run([llvm/'ld.lld.exe','-m','elf_x86_64','-pie','--script',sdk/'link.x','--eh-frame-hdr','-L'+str(sdk/'lib'),*objects,sdk/'lib/crt1.o','-lc','-lkernel','-lSceNet','-lSceNetCtl','-lSceSysmodule','-lSceRandom','-lSceVideoOut','-lSceBgft','-lSceAppInstUtil','-lSceSystemService','-lSceHttp','-lSceSsl','-lSceUserService','-o',elf])
    pkg=build/'package';(pkg/'sce_sys/about').mkdir(parents=True,exist_ok=True);(pkg/'sce_module').mkdir(exist_ok=True)
    tools=sdk/'bin/windows'
    run([tools/'create-fself.exe','-in',elf,'-out',build/'harbor.oelf','--eboot',pkg/'eboot.bin','--paid','0x3800000000000011'])
    for name in ['libc.prx','libSceFios2.prx']:
        shutil.copy2(sdk/'samples/hello_world/sce_module'/name,pkg/'sce_module'/name)
    shutil.copy2(sdk/'samples/hello_world/sce_sys/about/right.sprx',pkg/'sce_sys/about/right.sprx')
    shutil.copy2(ROOT/'assets/icon0.png',pkg/'sce_sys/icon0.png')
    os.environ['DOTNET_ROLL_FORWARD']='Major'
    tool=tools/'PkgTool.Core.exe';sfo=pkg/'sce_sys/param.sfo'
    if sfo.exists():sfo.unlink()
    run([tool,'sfo_new',sfo])
    content=VERSION['APP_CONTENT_ID']
    title='h1pNoise'
    fields={'APP_TYPE':(1,4),'APP_VER':(VERSION['APP_SFO_VERSION'],8),'ATTRIBUTE':(0,4),'CATEGORY':('gd',4),'CONTENT_ID':(content,48),'DOWNLOAD_DATA_SIZE':(0,4),'SYSTEM_VER':(0,4),'TITLE':(title,128),'TITLE_ID':(VERSION['APP_TITLE_ID'],12),'VERSION':(VERSION['APP_SFO_VERSION'],8)}
    for key,(value,size) in fields.items():run([tool,'sfo_setentry',sfo,key,'--type','Integer' if isinstance(value,int) else 'Utf8','--maxsize',size,'--value',value])
    files='eboot.bin sce_sys/about/right.sprx sce_sys/param.sfo sce_sys/icon0.png sce_module/libc.prx sce_module/libSceFios2.prx'
    run([tools/'create-gp4.exe','-out','pkg.gp4','--content-id='+content,'--files',files],cwd=pkg)
    run([tool,'pkg_build','pkg.gp4','.'],cwd=pkg)
    output='h1pNoise-'+VERSION['APP_VERSION']+('-shadPS4-test.pkg' if a.shadps4 else '.pkg')
    artifact=next(pkg.glob('*.pkg'));shutil.copy2(artifact,build/output)
    print('Built',build/output)

if __name__=='__main__':main()

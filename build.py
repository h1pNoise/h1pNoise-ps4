"""Build Windows test host or PS4 FPKG. No proprietary Sony SDK required."""
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
    p.add_argument('--runtime',action='store_true',help='PS4 bootstrap and signed executable updates; never replace the running PKG')
    p.add_argument('--pkg-payload-test',action='store_true',help='GoldHEN payload verifies handoff, closes source app, installs and reopens')
    p.add_argument('--pkg-direct-test',action='store_true',help='Isolated native PKG self-install attempt without closing the app')
    p.add_argument('--pkg-installer-test',action='store_true',help='Isolated experimental PKG handoff to a separately installed helper')
    p.add_argument('--pkg-installer-version',help='Version for an isolated candidate build; requires --pkg-installer-build')
    p.add_argument('--pkg-installer-build',type=int,help='Build/SFO for an isolated candidate build')
    p.add_argument('--sdk',default=os.getenv('OO_PS4_TOOLCHAIN'))
    p.add_argument('--llvm',default=os.getenv('LLVM_BIN'))
    p.add_argument('--zig',default=os.getenv('ZIG_EXE','zig'))
    a=p.parse_args();embed()
    if a.host and a.shadps4:p.error('--host and --shadps4 are separate targets')
    if a.runtime and (a.host or a.shadps4):p.error('--runtime requires a real PS4 target')
    if a.pkg_payload_test and (a.host or a.shadps4 or a.runtime or a.pkg_direct_test or a.pkg_installer_test):p.error('--pkg-payload-test requires an isolated real PS4 build')
    if a.pkg_payload_test and not (a.pkg_installer_version and a.pkg_installer_build):p.error('Choose an explicit payload test version/build')
    if a.pkg_direct_test and (a.host or a.shadps4 or a.runtime or a.pkg_installer_test):p.error('--pkg-direct-test requires a separate real PS4 target')
    if a.pkg_direct_test and not a.pkg_installer_version:p.error('--pkg-direct-test requires an explicit isolated version and build')
    if a.pkg_installer_test and (a.host or a.shadps4 or a.runtime):p.error('--pkg-installer-test requires a separate real PS4 target')
    if (a.pkg_installer_version or a.pkg_installer_build) and (not (a.pkg_installer_test or a.pkg_direct_test or a.pkg_payload_test) or not a.pkg_installer_version or not a.pkg_installer_build):p.error('Candidate version/build require --pkg-installer-test and both values')
    build=ROOT/('build-pkg-install-test' if a.pkg_installer_test else 'build-shadps4' if a.shadps4 else 'build');build.mkdir(exist_ok=True)
    extra=[]
    if a.pkg_installer_test or a.pkg_direct_test or a.pkg_payload_test:
        # Never edit or repackage the published version. The opt-in build has
        # its own newer version and output directory, outside the release feed.
        parts=list(map(int,VERSION['APP_VERSION'].split('.')));parts[-1]+=1
        VERSION['APP_VERSION']='.'.join(map(str,parts))
        number=int(re.search(r'^#define APP_BUILD (\d+)',(ROOT/'src/version.h').read_text(),re.M).group(1))+1
        if a.pkg_installer_version:
            if not re.fullmatch(r'\d+\.\d+\.\d+',a.pkg_installer_version) or a.pkg_installer_build<number or tuple(map(int,a.pkg_installer_version.split('.')))<tuple(parts):p.error('Candidate must be newer than the published source')
            VERSION['APP_VERSION']=a.pkg_installer_version;number=a.pkg_installer_build
        if number>99:p.error('Choose the next SFO version explicitly before build 100')
        build=ROOT/(('build-pkg-payload-test-' if a.pkg_payload_test else 'build-pkg-direct-test-' if a.pkg_direct_test else 'build-pkg-install-test-')+VERSION['APP_VERSION']);build.mkdir(exist_ok=True)
        VERSION['APP_SFO_VERSION']='00.%02d'%number
        config=build/'installer-test-version.h'
        config.write_text('#define H1PNOISE_VERSION_H\n#define APP_BUILD '+str(number)+'\n'+''.join('#define '+k+' "'+v+'"\n' for k,v in VERSION.items()))
        extra=['-DHARBOR_PKG_PAYLOAD_TEST' if a.pkg_payload_test else '-DHARBOR_PKG_DIRECT_TEST' if a.pkg_direct_test else '-DHARBOR_PKG_INSTALLER_TEST','-include',config]
    sources=[ROOT/'src'/x for x in ['core.c','magnet.c','platform.c','storage.c','pkg_validation.c','remote_pkg.c','engine.c','server.c','main.c','pairing.c','vendor/qrcodegen.c','updater.c','update_destination.c','update_http.c','update_platform.c','update_manifest.c','update_file.c','vendor/monocypher.c','vendor/monocypher-ed25519.c']]
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
    if a.runtime:sources.append(ROOT/'src/runtime_update.c')
    if a.pkg_payload_test:
        payload_out=build/'payload'
        run([os.sys.executable,ROOT/'tools/build_update_payload.py','--sdk',sdk,'--llvm',llvm,'--out',payload_out])
        blob=(payload_out/'h1pNoise-update-payload.bin').read_bytes()
        embedded=build/'payload_data.c'
        embedded.write_text('#include <stddef.h>\nconst unsigned char update_payload[]={'+','.join(map(str,blob))+'};\nconst size_t update_payload_size=sizeof(update_payload);\n')
        sources.extend([ROOT/'src/pkg_update_payload_handoff.c',embedded])
    if a.pkg_installer_test:sources.append(ROOT/'src/pkg_update_handoff.c')
    if a.pkg_direct_test:sources.append(ROOT/'src/pkg_update_direct.c')
    for source in sources:
        obj=build/(source.stem+'.o');objects.append(obj)
        defines=['-D__ORBIS__']+(['-DHARBOR_SHADPS4'] if a.shadps4 else [])+(['-DHARBOR_RUNTIME_UPDATES'] if a.runtime else [])
        run([llvm/'clang.exe','--target=x86_64-pc-freebsd12-elf',*defines,*extra,'-O2','-fPIC','-funwind-tables','-Wall','-Wextra','-Wno-misleading-indentation','-isysroot',sdk,'-isystem',sdk/'include','-c',source,'-o',obj])
    elf=build/'harbor.elf'
    run([llvm/'ld.lld.exe','-m','elf_x86_64','-pie','--script',sdk/'link.x','--eh-frame-hdr','-L'+str(sdk/'lib'),*objects,sdk/'lib/crt1.o','-lc','-lkernel','-lSceNet','-lSceNetCtl','-lSceSysmodule','-lSceRandom','-lSceVideoOut','-lSceBgft','-lSceAppInstUtil','-lSceSystemService','-lSceHttp','-lSceSsl','-lSceUserService',*(['-lScePad'] if a.runtime else []),'-o',elf])
    pkg=build/'package';(pkg/'sce_sys/about').mkdir(parents=True,exist_ok=True);(pkg/'sce_module').mkdir(exist_ok=True)
    tools=sdk/'bin/windows'
    run([tools/'create-fself.exe','-in',elf,'-out',build/'harbor.oelf','--eboot',pkg/('h1pNoise.self' if a.runtime else 'eboot.bin'),'--paid','0x3800000000000011'])
    if a.runtime:
        bootstrap=[]
        for name in ['bootstrap.c','runtime_update.c','update_manifest.c','vendor/monocypher.c','vendor/monocypher-ed25519.c']:
            source=ROOT/'src'/name;obj=build/('boot-'+source.stem+'.o');bootstrap.append(obj)
            run([llvm/'clang.exe','--target=x86_64-pc-freebsd12-elf','-D__ORBIS__','-DHARBOR_RUNTIME_UPDATES','-O2','-fPIC','-funwind-tables','-Wall','-Wextra','-Wno-misleading-indentation','-isysroot',sdk,'-isystem',sdk/'include','-c',source,'-o',obj])
        bootelf=build/'bootstrap.elf'
        run([llvm/'ld.lld.exe','-m','elf_x86_64','-pie','--script',sdk/'link.x','--eh-frame-hdr','-L'+str(sdk/'lib'),*bootstrap,sdk/'lib/crt1.o','-lc','-lkernel','-lSceSystemService','-o',bootelf])
        run([tools/'create-fself.exe','-in',bootelf,'-out',build/'bootstrap.oelf','--eboot',pkg/'eboot.bin','--paid','0x3800000000000011'])
        shutil.copy2(pkg/'h1pNoise.self',build/('h1pNoise-'+VERSION['APP_VERSION']+'.self'))
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
    if a.runtime:files+=' h1pNoise.self'
    run([tools/'create-gp4.exe','-out','pkg.gp4','--content-id='+content,'--files',files],cwd=pkg)
    run([tool,'pkg_build','pkg.gp4','.'],cwd=pkg)
    output='h1pNoise-'+VERSION['APP_VERSION']+('-payload-test.pkg' if a.pkg_payload_test else '-direct-test.pkg' if a.pkg_direct_test else '-install-test.pkg' if a.pkg_installer_test else '-shadPS4-test.pkg' if a.shadps4 else '-experimental.pkg' if int(VERSION['APP_VERSION'].split('.')[0])==0 else '.pkg')
    artifact=next(pkg.glob('*.pkg'));shutil.copy2(artifact,build/output)
    print('Built',build/output)
    if a.pkg_installer_test:run([os.sys.executable,ROOT/'tools/build_pkg_updater.py','--sdk',sdk,'--llvm',llvm,'--out',build/'updater-helper'])

if __name__=='__main__':main()

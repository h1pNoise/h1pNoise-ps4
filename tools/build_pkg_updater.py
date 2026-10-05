"""Build the opt-in independent installer. No publishing or signing keys."""
from pathlib import Path
import argparse,subprocess,os,shutil
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser()
for key in ['sdk','llvm','out']:p.add_argument('--'+key,required=True,type=Path)
a=p.parse_args();sdk=a.sdk.resolve();llvm=a.llvm.resolve();out=a.out.resolve();out.mkdir(parents=True,exist_ok=True)
def run(args,cwd=root):subprocess.run(list(map(str,args)),cwd=cwd,check=True)
objects=[]
for name in ['pkg_update_helper.c','pkg_update.c','update_file.c','update_manifest.c','ps4_user.c','installer_access.c','vendor/libjbc/jailbreak.c','vendor/libjbc/kernelrw.c','vendor/monocypher.c','vendor/monocypher-ed25519.c']:
 obj=out/(Path(name).stem+'.o');objects.append(obj)
 run([llvm/'clang.exe','--target=x86_64-pc-freebsd12-elf','-D__ORBIS__','-O2','-fPIC','-funwind-tables','-Wall','-Wextra','-Wno-misleading-indentation','-isysroot',sdk,'-isystem',sdk/'include','-c',root/'src'/name,'-o',obj])
elf=out/'updater.elf'
run([llvm/'ld.lld.exe','-m','elf_x86_64','-pie','--script',sdk/'link.x','--eh-frame-hdr','-L'+str(sdk/'lib'),*objects,sdk/'lib/crt1.o','-lc','-lkernel','-lSceAppInstUtil','-lSceSystemService','-lSceUserService','-lSceSysmodule','-o',elf])
pkg=out/'package';(pkg/'sce_sys/about').mkdir(parents=True,exist_ok=True);(pkg/'sce_module').mkdir(exist_ok=True)
tools=sdk/'bin/windows';os.environ['OO_PS4_TOOLCHAIN']=str(sdk);os.environ['DOTNET_ROLL_FORWARD']='Major'
run([tools/'create-fself.exe','-in',elf,'-out',out/'updater.oelf','--eboot',pkg/'eboot.bin','--paid','0x3800000000000011'])
for name in ['libc.prx','libSceFios2.prx']:shutil.copy2(sdk/'samples/hello_world/sce_module'/name,pkg/'sce_module'/name)
shutil.copy2(sdk/'samples/hello_world/sce_sys/about/right.sprx',pkg/'sce_sys/about/right.sprx')
shutil.copy2(root/'assets/icon0.png',pkg/'sce_sys/icon0.png')
sfo=pkg/'sce_sys/param.sfo';sfo.unlink(missing_ok=True);tool=tools/'PkgTool.Core.exe'
run([tool,'sfo_new',sfo]);cid='IV0000-HBRU00001_00-H1PNOISEUPDATER0'
fields={'APP_TYPE':(1,4),'APP_VER':('00.01',8),'ATTRIBUTE':(0,4),'CATEGORY':('gd',4),'CONTENT_ID':(cid,48),'DOWNLOAD_DATA_SIZE':(0,4),'SYSTEM_VER':(0,4),'TITLE':('h1pNoise Updater - TESTE',128),'TITLE_ID':('HBRU00001',12),'VERSION':('00.01',8)}
for key,(value,size) in fields.items():run([tool,'sfo_setentry',sfo,key,'--type','Integer' if isinstance(value,int) else 'Utf8','--maxsize',size,'--value',value])
files='eboot.bin sce_sys/about/right.sprx sce_sys/param.sfo sce_sys/icon0.png sce_module/libc.prx sce_module/libSceFios2.prx'
run([tools/'create-gp4.exe','-out','pkg.gp4','--content-id='+cid,'--files',files],cwd=pkg)
run([tool,'pkg_build','pkg.gp4','.'],cwd=pkg)
artifact=next(pkg.glob('*.pkg'));shutil.copy2(artifact,out/'h1pNoise-Updater-0.1.0-test.pkg')
print('Built',out/'h1pNoise-Updater-0.1.0-test.pkg')

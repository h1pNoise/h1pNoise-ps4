"""Build a freestanding, position-independent GoldHEN payload on Windows.

No fixed firmware addresses. Entry and dynamic module loading follow the
public syscall interface used by sleirsgoevy's payloads and DPI.
"""
from pathlib import Path
import argparse, subprocess, struct
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser()
for key in ('sdk','llvm','out'):p.add_argument('--'+key,type=Path,required=True)
a=p.parse_args();sdk=a.sdk.resolve();llvm=a.llvm.resolve();out=a.out.resolve();out.mkdir(parents=True,exist_ok=True)
def run(args):subprocess.run(list(map(str,args)),check=True)
sources=['payload_runtime.c','pkg_update_helper.c','pkg_update.c','update_file.c','update_manifest.c','vendor/libjbc/jailbreak.c','vendor/libjbc/kernelrw.c','vendor/monocypher.c','vendor/monocypher-ed25519.c']
objects=[]
for name in sources:
 obj=out/(Path(name).stem+'.o');objects.append(obj)
 run([llvm/'clang.exe','--target=x86_64-pc-freebsd12-elf','-D__ORBIS__','-DHARBOR_PKG_PAYLOAD_TEST','-O2','-fPIE','-fvisibility=hidden','-ffreestanding','-fno-stack-protector','-fno-builtin','-ffunction-sections','-fdata-sections','-Wall','-Wextra','-Wno-misleading-indentation','-isysroot',sdk,'-isystem',sdk/'include','-c',root/'src'/name,'-o',obj])
script=out/'payload.ld'
script.write_text('ENTRY(_start)\nSECTIONS { . = 0; .text : { KEEP(*(.text.entry)) *(.text*) } .rodata : { *(.rodata*) } .data : { *(.data*) } .bss : { __bss_start = .; *(.bss*) *(COMMON) __bss_end = .; } /DISCARD/ : { *(.eh_frame*) *(.comment*) *(.note*) } }\n')
# Resolve only this explicit API set. Assembly tail calls preserve varargs and
# the caller's ABI; a missing symbol aborts before acknowledging the handoff.
groups={
 'libSceLibcInternal.sprx':['fread','fwrite','fflush','fclose','ferror','fileno','fprintf','snprintf','fseeko','ftello','rewind','malloc','free','strcmp','strncmp','strcpy','memcmp','memchr'],
 'libkernel.sprx':['sceKernelLoadStartModule','sceKernelSendNotificationRequest','usleep'],
 'libSceSysmodule.sprx':['sceSysmoduleLoadModuleInternal'],
 'libSceAppInstUtil.sprx':['sceAppInstUtilInitialize','sceAppInstUtilAppInstallPkg'],
 'libSceUserService.sprx':['sceUserServiceInitialize','sceUserServiceGetForegroundUser'],
 'libSceSystemService.sprx':['sceSystemServiceLaunchApp'],
}
decl=['#include <stddef.h>','extern int payload_module(const char*),payload_resolve(int,const char*,void**);','extern void *payload_fopen_ptr;']
body=['int payload_bind_symbols(void){ int h;']
for module,names in groups.items():
 body.append(f'h=payload_module("/system/common/lib/{module}");if(h<0)return -1;')
 if module=='libSceLibcInternal.sprx':body.append('if(payload_resolve(h,"fopen",&payload_fopen_ptr))return -1;')
 for name in names:
  decl.append(f'void *p_{name};')
  decl.append(f'asm(".global {name}\\n{name}:\\njmp *p_{name}(%rip)\\n");')
  body.append(f'if(payload_resolve(h,"{name}",&p_{name}))return -1;')
body.append('return 0;}')
bindings=out/'bindings.c';bindings.write_text('\n'.join(decl+body))
obj=out/'bindings.o';objects.append(obj)
run([llvm/'clang.exe','--target=x86_64-pc-freebsd12-elf','-fPIE','-fvisibility=hidden','-ffreestanding','-fno-stack-protector','-O2','-c',bindings,'-o',obj])
elf=out/'h1pNoise-update-payload.elf';binary=out/'h1pNoise-update-payload.bin'
run([llvm/'ld.lld.exe','-m','elf_x86_64','-static','--gc-sections','--script',script,*objects,'-o',elf])
run([llvm/'llvm-objcopy.exe','-O','binary',elf,binary])
# BSS must be included in the transferred mapping, then zeroed by _start.
symbols=subprocess.check_output([str(llvm/'llvm-nm.exe'),'-n',str(elf)],text=True)
end=next(int(line.split()[0],16) for line in symbols.splitlines() if line.endswith(' __bss_end'))
data=binary.read_bytes();assert len(data)<=end<1024*1024
binary.write_bytes(data+b'\0'*(end-len(data)))
undefined=subprocess.check_output([str(llvm/'llvm-nm.exe'),'--undefined-only',str(elf)],text=True)
assert not undefined.strip(),undefined
assert next(line.split()[0] for line in symbols.splitlines() if line.endswith(' _start'))=='0000000000000000'
header=elf.read_bytes();shoff=struct.unpack_from('<Q',header,40)[0];shsize,shnum=struct.unpack_from('<HH',header,58)
assert not any(struct.unpack_from('<I',header,shoff+i*shsize+4)[0] in (4,9) and struct.unpack_from('<Q',header,shoff+i*shsize+32)[0] for i in range(shnum)), 'Relocations in raw payload'
print('Payload built:',binary,'bytes',end,'(entry 0; no imports or relocations)')

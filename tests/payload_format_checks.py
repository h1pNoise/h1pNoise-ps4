"""Check the actual native BIN entry, including the previously rejected build."""
from pathlib import Path
import sys,struct
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from payload_format import validate_raw_payload

data=Path(sys.argv[1]).read_bytes()
entry=validate_raw_payload(data)
assert entry==16 and data[entry:entry+3]==bytes.fromhex('488d3d')
# LLVM can emit multi-byte alignment NOPs. They are skipped by the JMP.
# Check both RIP-relative BSS addresses in the real startup instead.
assert data[entry+7:entry+10]==bytes.fromhex('488d0d')
bss_start=entry+7+struct.unpack_from('<i',data,entry+3)[0]
bss_end=entry+14+struct.unpack_from('<i',data,entry+10)[0]
assert entry+32<=bss_start<=bss_end==len(data)
bad=[b'',data[:4],data[:31],b'\x48'+data[1:],b'\xeb'+data[1:],b'\xe9'+struct.pack('<i',-1)+data[5:],b'\xe9'+struct.pack('<i',len(data))+data[5:]]
for case in bad:
 try:validate_raw_payload(case)
 except ValueError:pass
 else:raise AssertionError('Invalid payload accepted')
if len(sys.argv)>2:
 old=Path(sys.argv[2]).read_bytes()
 try:validate_raw_payload(old)
 except ValueError:pass
 else:raise AssertionError('The BIN rejected on PS4 passed the format check')
print(f'{len(bad)+1+(len(sys.argv)>2)} payload format checks passed; real E9 JMP targets startup at byte {entry}, old rejected BIN fails validation.')

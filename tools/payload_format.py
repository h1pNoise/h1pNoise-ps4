"""Validate our raw BIN entry convention (JMP rel32), not arbitrary ELF files."""
import struct

def validate_raw_payload(data):
 if not 32 <= len(data) <= 1024*1024:
  raise ValueError('Raw payload size out of range')
 if data[0] != 0xe9:
  raise ValueError('Raw BIN must start with JMP rel32 (E9)')
 entry=5+struct.unpack_from('<i',data,1)[0]
 if not 5 <= entry < len(data):
  raise ValueError('Raw BIN entry jumps outside its executable image')
 return entry

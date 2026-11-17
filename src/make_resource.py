"""Emit the documented Windows PiPL layout without the legacy PiPLtool executable."""
import struct
from pathlib import Path
u32=lambda v:struct.pack('<I',v)
def pascal(s):
    b=s.encode('ascii');return bytes([len(b)])+b
def prop(key,data):
    data+=b'\0'*(-len(data)%4)
    return b'MIB8'+key.encode('ascii')[::-1]+u32(0)+u32(len(data))+data
properties=[
    prop('kind',b'TKFe'),prop('name',pascal('SmileMorph Precomp')),prop('catg',pascal('SmileMorph Precomp')),
    prop('8664',b'EffectMain\0'),prop('ePVR',struct.pack('<HH',2,0)),
    prop('eSVR',struct.pack('<HH',13,29)),prop('eVER',u32((3 << 15) | 2)),
    # eINF has a 2-byte payload plus 2-byte alignment, unlike the string lengths.
    b'MIB8FNIe'+u32(0)+u32(2)+b'\0'*4,
    prop('eGLO',u32(0x02000006)),prop('eGL2',u32(0)),
    prop('eMNA',pascal('SmileMorph Precomp Native')),prop('aeFL',u32(8))]
out=Path(__file__).resolve().parents[1]/'build';out.mkdir(exist_ok=True)
(out/'SmileMorphPrecomp.pipl').write_bytes(struct.pack('<HII',1,0,len(properties))+b''.join(properties))
(out/'SmileMorphPrecomp.rc').write_text('16000 "PiPL" "SmileMorphPrecomp.pipl"\n',encoding='ascii')

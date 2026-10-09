# SPDX-License-Identifier: GPL-3.0-only
"""Run from any directory; existing sampleio is the independent oracle."""
import sys, json, struct, hashlib
from pathlib import Path
root = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(root / 'tools'))
import sampleio as sio
cases = [('silence', 'SILENCE', [([0]*32,60,None,None)]),
 ('impulse', 'IMPULSE', [([32767]+[0]*30,60,None,None)]),
 ('extremes', 'EXTREME', [([-32768,32767]*20,60,None,None)]),
 ('odd', 'ODD', [([1,-2,30000],60,None,None)]),
 ('zones', 'caféß長kit!', [([1,2,3],72,None,None),([-32768,32767,0,99,5],48,None,None)]),
 ('sixteen', 'DRUMKIT', [([n*100,-n*100,0],note,note,note) for n,note in enumerate([36,35,38,39,42,46,44,37,40,43,48,49,51,70,63,56])]),
 ('capacity', 'EXACT', [([0]*162816,60,None,None)]),
 ('oddCapacity', 'EXACTODD', [([0]*81407,48,None,None),([0]*81407,72,None,None)])]
out=[]
def decode(data,n):
    pred=idx=0
    result=[]
    for pos in range(n):
        code=(data[pos//2] >> (4*(pos&1))) & 15
        step=sio.IMA_STEP[idx]
        vd=(step>>3)+(step if code&4 else 0)+(step>>1 if code&2 else 0)+(step>>2 if code&1 else 0)
        pred=max(-32768,min(32767,pred+(-vd if code&8 else vd)))
        idx=max(0,min(88,idx+sio.IMA_IDX[code&7]))
        result.append(pred)
    return result
for label,name,zones in cases:
    hdr,data=sio.user_slot(name,zones)
    out.append(dict(label=label,name=name,zones=[dict(pcm=s,root=r,lo=lo,hi=hi,decoded=decode(sio.ima_encode(s,0)[0],len(s))) for s,r,lo,hi in zones],header=hdr.hex(),data=data.hex()))
# user_slot has no loop API; patch its header using ima_encode's independently captured state.
s=[-32768,32767,1000,-1000,0,55,100,0,32767]
hdr,data=sio.user_slot('LOOP',[(s,60,0,127)])
data,state=sio.ima_encode(s,3)
hdr=bytearray(hdr)
struct.pack_into('<II',hdr,40,3,7)
struct.pack_into('<hB',hdr,54,*state)
hdr[59]=1
out.append(dict(label='loop',name='LOOP',zones=[dict(pcm=s,root=60,lo=0,hi=127,ls=3,le=8,decoded=decode(data,len(s)))],header=hdr.hex(),data=data.hex()))
try:
    sio.user_slot('OVER',[([0]*162817,60,None,None)])
    raise AssertionError('Python oracle accepted overflow')
except ValueError:
    pass
dest=Path(__file__).parent/'Fixtures'
dest.mkdir(exist_ok=True)
(dest/'golden.json').write_text(json.dumps(out,separators=(',',':')),encoding='utf-8')
(dest/'provenance.json').write_text(json.dumps(dict(python=sys.version,source='tools/sampleio.py',sha256=hashlib.sha256((root/'tools/sampleio.py').read_bytes()).hexdigest()),indent=2))

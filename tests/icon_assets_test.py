"""Compare generated icon pixels against the PNG, then prepare C renderer oracles."""
from pathlib import Path
import json
import random
import re
import sys
import tempfile
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from gen_icons import main as generate

def array(text,name):
    body=re.search(r'\b'+name+r'\[[^=]+ = \{(.*?)\n\};',text,re.S).group(1)
    return [[int(v,0) for v in re.findall(r'0x[0-9a-f]+|\d+',row)]
            for row in re.findall(r'\{([^{}]+)\}',body)]

def oracle(assets):
    meta=json.loads((assets/'icons.json').read_text());size=meta['cell'];cols=meta['cols']
    image=Image.open(assets/'icons.png').convert('RGBA');lum=image.convert('L');alpha=image.getchannel('A')
    return [[(lum.getpixel(((n%cols)*size+x,(n//cols)*size+y))*
              alpha.getpixel(((n%cols)*size+x,(n//cols)*size+y))//255+42)//85
             for y in range(size) for x in range(size)] for n in range(len(meta['names']))]

def verify(header,expected,size):
    text=header.read_text()
    packed='#define ICON_ROWS_PACKED 1' in text
    data=array(text,'ICON_ROWS' if packed else 'ICON_DATA')
    ids=array(text,'ICON_ROW_ID') if packed else None
    for n,glyph in enumerate(expected):
        for k,value in enumerate(glyph):
            y,x=divmod(k,size)
            raw=data[ids[n][y]][x//4] if packed else data[n][k//4]
            shift=6-2*((x if packed else k)%4)
            assert (raw>>shift)&3==value,(n,x,y,value)
    return text,packed

def main():
    out=ROOT/'build/host/icon-assets';out.mkdir(parents=True,exist_ok=True)
    header=out/'felucca_icons.h';generate(header,ROOT/'assets')
    expected=oracle(ROOT/'assets');text,packed=verify(header,expected,12);assert packed
    names=json.loads((ROOT/'assets/icons.json').read_text())['names']
    subset=[names.index(n) for n in ('tempo','tape')]
    compact=out/'felucca_header_icons.h';generate(compact,ROOT/'assets','tempo,tape')
    verify(compact,[expected[n] for n in subset],12)
    for invalid in ('missing','tempo,tempo'):
        try: generate(compact,ROOT/'assets',invalid)
        except ValueError: pass
        else: raise AssertionError('Invalid subset accepted')
    (out/'icon_oracle.h').write_text('#define ICON_ORACLE_N '+str(len(expected))+'\n'+
        f'#define ICON_ORACLE_TEMPO {subset[0]}\n#define ICON_ORACLE_TAPE {subset[1]}\n'+
        'static const uint8_t ICON_ORACLE[][144]={\n'+
        ''.join('{'+','.join(map(str,glyph))+'},\n' for glyph in expected)+'};\n')
    legacy=out/'legacy';legacy.mkdir(exist_ok=True)
    prefix=text.split('#define ICON_ROWS_PACKED')[0]
    raw=[[sum(glyph[k+j]<<(6-2*j) for j in range(4)) for k in range(0,144,4)] for glyph in expected]
    (legacy/'felucca_icons.h').write_text(prefix+'static const uint8_t ICON_DATA[][36]={\n'+
        ''.join('{'+','.join(map(str,glyph))+'},\n' for glyph in raw)+'};\n')
    with tempfile.TemporaryDirectory(dir=out) as temporary:
        assets=Path(temporary);header=assets/'output.h'
        # Non-byte-aligned rows exercise packed row padding without losing pixels.
        (assets/'icons.json').write_text(json.dumps({'cell':5,'cols':4,'names':['a','b','c','d']}))
        Image.new('L',(20,5),255).save(assets/'icons.png');generate(header,assets)
        _,packed=verify(header,oracle(assets),5);assert packed
        # More than 256 distinct rows must fall back to the original two-bit format.
        rng=random.Random(1341);image=Image.new('L',(96,36))
        image.putdata([rng.randrange(4)*85 for _ in range(96*36)]);image.save(assets/'icons.png')
        (assets/'icons.json').write_text(json.dumps({'cell':12,'cols':8,'names':[f'icon{i}' for i in range(24)]}))
        generate(header,assets);_,packed=verify(header,oracle(assets),12);assert not packed
        (assets/'icons.png').unlink();generate(header,assets)
        assert '#define FELUCCA_ICONS_N 0' in header.read_text()
    print('icon assets: full/header atlas pixels, subset validation, row padding, overflow fallback and missing artwork PASS')

if __name__=='__main__':main()

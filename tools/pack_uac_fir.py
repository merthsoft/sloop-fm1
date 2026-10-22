# SPDX-License-Identifier: GPL-3.0-only
"""Losslessly pack UAC FIR second differences; expand once before audio starts."""
from pathlib import Path
import re

def pack_uac_fir(gen):
    gen=Path(gen)
    s=(gen.parent.parent/'firmware/src/uac_fir.h').read_text()
    body=re.search(r'static const int16_t UAC_FIR\[80\]\[12\]\s*=\s*\{(.*?)\};',s,re.S)[1]
    values=list(map(int,re.findall(r'-?\d+',body)))
    assert len(values)==960
    blob=bytearray()
    for c in range(12):
        col=values[c::12];diff=[col[i]-2*col[i-1]+col[i-2] for i in range(2,80)]
        lo=min(diff);bits=(max(diff)-lo).bit_length() or 1
        assert -128<=lo<=127 and bits<=8
        for v in col[:2]:blob.extend((v&65535).to_bytes(2,'little'))
        blob.extend((lo&255,bits));acc=used=0
        for v in diff:
            acc|=(v-lo)<<used;used+=bits
            while used>=8:blob.append(acc&255);acc>>=8;used-=8
        if used:blob.append(acc&255)
    data=',\n'.join('    '+','.join(map(str,blob[i:i+24])) for i in range(0,len(blob),24))
    code='''/* Generated lossless boot-only FIR values; no audio-path decoder. */
static int16_t UAC_FIR[80][12];
static const uint8_t BOOT_UAC_FIR[] = {
%s
};
static void uac_fir_init(void)
{
    const uint8_t *p=BOOT_UAC_FIR;
    for(uint32_t c=0;c<12;c++) {
        int32_t a=(int16_t)(p[0]|(uint32_t)p[1]<<8);
        int32_t b=(int16_t)(p[2]|(uint32_t)p[3]<<8);
        int32_t lo=(int8_t)p[4];uint32_t bits=p[5],acc=0,used=0;
        p+=6;UAC_FIR[0][c]=(int16_t)a;UAC_FIR[1][c]=(int16_t)b;
        for(uint32_t r=2;r<80;r++) {
            if(used<bits){acc|=(uint32_t)*p++<<used;used+=8;}
            int32_t v=2*b-a+lo+(int32_t)(acc&((1u<<bits)-1u));
            acc>>=bits;used-=bits;UAC_FIR[r][c]=(int16_t)v;a=b;b=v;
        }
    }
}
'''%data
    (gen/'boot_uac_fir.h').write_text(code)
    (gen/'uac_fir_oracle.bin').write_bytes(b''.join((v&65535).to_bytes(2,'little') for v in values))
    print(f'UAC FIR: 1920 RAM bytes, {len(blob)} packed bytes')
    return 1920

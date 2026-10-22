# SPDX-License-Identifier: GPL-3.0-only
"""Pack target-compiled factory presets from an ELF32 relocatable object.
No linked addresses, host compiler, or installed firmware is needed.
"""
from pathlib import Path
import struct,re,json
import lz4blk

def read_presets(path):
    data=Path(path).read_bytes()
    if data[:7]!=b'\x7fELF\x01\x01\x01':raise ValueError('expected little-endian ELF32')
    h=struct.unpack_from('<16sHHIIIIIHHHHHH',data)
    if h[1]!=1 or h[11]!=40:raise ValueError('expected relocatable ELF32')
    sections=[struct.unpack_from('<IIIIIIIIII',data,h[6]+i*40) for i in range(h[12])]
    def content(s):
        out=data[s[4]:s[4]+s[5]]
        if len(out)!=s[5]:raise ValueError('truncated section')
        return out
    def string(buf,at):
        if at<0 or at>=len(buf):raise ValueError('string offset')
        return buf[at:buf.index(0,at)].decode('ascii')
    entries=[]
    for si,sy in enumerate(sections):
        if sy[1]!=2:continue
        symdata=content(sy);strings=content(sections[sy[6]])
        symbols=[struct.unpack_from('<IIIBBH',symdata,i) for i in range(0,len(symdata),16)]
        reloc={}
        for rel in sections:
            if rel[1]!=4 or rel[6]!=si:continue
            rb=content(rel)
            for i in range(0,len(rb),12):
                off,info,addend=struct.unpack_from('<IIi',rb,i)
                reloc[rel[7],off]=(info&255,symbols[info>>8],addend)
        for sym in symbols:
            name=string(strings,sym[0])
            if not re.fullmatch(r'\w+_PRESETS|SMP_PRESET_TABLE',name):continue
            if not sym[2] or sym[2]%36:raise ValueError('preset record size')
            raw=content(sections[sym[5]])[sym[1]:sym[1]+sym[2]]
            rows=[];names=[]
            for off in range(0,len(raw),36):
                rtype,target,addend=reloc[sym[5],sym[1]+off]
                if rtype!=8 or any((sym[5],sym[1]+off+j) in reloc for j in range(4,36)):raise ValueError('unexpected record relocation')
                names.append(string(content(sections[target[5]]),target[1]+addend))
                rows.append(list(raw[off+4:off+36]))
            entries.append(dict(name=name,count=len(rows),fields=rows,names=names))
    if not entries:raise ValueError('no factory records')
    return entries

def pack(path,gen):
    gen=Path(gen);e=read_presets(path);stream=bytearray();names=[]
    for table in e:
        for row,name in zip(table['fields'],table['names']):
            mask=sum(1<<j for j,v in enumerate(row) if v)
            stream+=struct.pack('<I',mask)+bytes(v for v in row if v);names.append(name)
    blob=lz4blk.compress(bytes(stream));assert lz4blk.decompress(blob)==stream
    sizes=['/* Generated from the raw target object. */','#pragma once']
    sizes += ['#define BP_N_'+t['name']+' '+str(t['count']) for t in e]
    (gen/'boot_presets_sizes.h').write_text('\n'.join(sizes)+'\n')
    code=['/* Generated from the raw target object. */','static const uint8_t BP_DATA[]={'+','.join(map(str,blob))+'};','static const char BP_NAMES[]=']
    code += [json.dumps(name)+' "\\0"' for name in names];code[-1]+=';'
    code+=['static preset_t *const BP_ARRAYS[]={'+','.join(t['name'] for t in e)+'};','static const uint8_t BP_COUNT[]={'+','.join(str(t['count']) for t in e)+'};']
    code+=['''static int boot_presets_init(void *scratch, uint32_t capacity) {
_Static_assert(sizeof(preset_t)==sizeof(((preset_t *)0)->name)+32,"preset payload ABI");
if(capacity < %d) return 0;
uint8_t *work=scratch;
const boot_table_t table={work,BP_DATA,sizeof BP_DATA,%d,1,0,1,0};
if(!boot_table_unpack(&table)) return 0;
const uint8_t *src=work,*end=work+%d;const char *name=BP_NAMES;
for(uint32_t a=0;a<sizeof BP_COUNT;a++) for(uint32_t i=0;i<BP_COUNT[a];i++) {
 if(end-src<4) return 0;
 preset_t *out=&BP_ARRAYS[a][i];uint32_t mask=boot_load(src,4);src+=4;
 uint8_t *dst=(uint8_t *)out+sizeof out->name;
 for(uint32_t j=0;j<32;j++) {
   if((mask>>j)&1u) { if(src==end)return 0;dst[j]=*src++; } else dst[j]=0;
 }
 if(name>=BP_NAMES+sizeof BP_NAMES) return 0;
 out->name=name;while(*name++) {if(name==BP_NAMES+sizeof BP_NAMES)return 0;}
}
return src==end;
}'''%(len(stream),len(stream),len(stream))]
    (gen/'boot_presets.h').write_text('\n'.join(code)+'\n')
    (gen/'boot_presets_oracle.json').write_text(json.dumps(e,indent=2))
    return e,len(stream),len(blob)

if __name__=='__main__':
    import sys
    e,raw,packed=pack(sys.argv[1],sys.argv[2]);print(len(e),sum(t['count'] for t in e),raw,packed)

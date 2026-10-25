# SPDX-License-Identifier: GPL-3.0-only
"""Pack target-compiled factory presets from an ELF32 relocatable object.
No linked addresses, host compiler, or installed firmware is needed.
"""
from pathlib import Path
import struct,re,json
import lz4blk

def read_presets(path, zones=None):
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
            if name=='SMP_ZONES' and zones is not None:
                if not sym[2] or sym[2]%28:raise ValueError('sample zone ABI')
                if any(k==sym[5] and sym[1]<=off<sym[1]+sym[2] for k,off in reloc):
                    raise ValueError('sample zone relocation')
                raw=content(sections[sym[5]])[sym[1]:sym[1]+sym[2]]
                if len(raw)!=sym[2]:raise ValueError('truncated sample zones')
                zones.append(raw)
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

def pack_names(names):
    """Six-bit uppercase/punctuation alphabet; preserve other ASCII via LZ4."""
    raw=('\0'.join(names)+'\0\0').encode('ascii')
    if not all(v==0 or 32<=v<=63 or 65<=v<=90 for v in raw):
        return raw,lz4blk.compress_best(raw),0
    packed=bytearray(4);acc=used=0
    for v in raw:
        acc|=(v&63)<<used;used+=6
        while used>=8:packed.append(acc&255);acc>>=8;used-=8
    if used:packed.append(acc&255)
    return raw,bytes(packed),6

def pack(path,gen):
    gen=Path(gen);zones=[];e=read_presets(path,zones);stream=bytearray();names=[]
    # Bit widths are per table/field; each column starts on a byte boundary.
    # Preserve all 32 target payload bytes, including signed fields and padding.
    for table in e:
        names.extend(table['names'])
        for j in range(32):
            vs=[row[j] for row in table['fields']];width=max(vs).bit_length()
            stream.append(width)
            acc=used=0
            for v in vs:
                acc|=v<<used;used+=width
                while used>=8:stream.append(acc&255);acc>>=8;used-=8
            if used:stream.append(acc&255)
    blob=lz4blk.compress_best(bytes(stream));assert lz4blk.decompress(blob)==stream
    sizes=['/* Generated from the raw target object. */','#pragma once']
    sizes += ['#define BP_N_'+t['name']+' '+str(t['count']) for t in e]
    (gen/'boot_presets_sizes.h').write_text('\n'.join(sizes)+'\n')
    names_raw,names_blob,name_bits=pack_names(names)
    code=['/* Generated from the raw target object. */','static const uint8_t BP_DATA[]={'+','.join(map(str,blob))+'};',
          'static char BP_NAMES['+str(len(names_raw))+'];',
          'static const uint8_t BP_NAME_DATA[]={'+','.join(map(str,names_blob))+'};']
    code+=['static preset_t *const BP_ARRAYS[]={'+','.join(t['name'] for t in e)+'};','static const uint8_t BP_COUNT[]={'+','.join(str(t['count']) for t in e)+'};']
    code+=['''static int boot_presets_init(void *scratch, uint32_t capacity) {
_Static_assert(sizeof(preset_t)==sizeof(((preset_t *)0)->name)+32,"preset payload ABI");
if(capacity < %d) return 0;
const boot_table_t nt={BP_NAMES,BP_NAME_DATA,sizeof BP_NAME_DATA,sizeof BP_NAMES,1,0,1,%d};
if(!boot_table_unpack(&nt))return 0;
%s
uint8_t *work=scratch;
const boot_table_t table={work,BP_DATA,sizeof BP_DATA,%d,1,0,1,0};
if(!boot_table_unpack(&table)) return 0;
const uint8_t *src=work,*end=work+%d;const char *name=BP_NAMES;
for(uint32_t a=0;a<sizeof BP_COUNT;a++) {
 for(uint32_t j=0;j<32;j++) {
  if(src==end) return 0;
  uint32_t width=*src++,acc=0,used=0;if(width>8) return 0;
  for(uint32_t i=0;i<BP_COUNT[a];i++) {
   if(used<width){if(src==end)return 0;acc|=(uint32_t)*src++<<used;used+=8;}
   ((uint8_t *)&BP_ARRAYS[a][i]+sizeof BP_ARRAYS[a][i].name)[j]=(uint8_t)(acc&((1u<<width)-1u));
   acc>>=width;used-=width;
  }
 }
 for(uint32_t i=0;i<BP_COUNT[a];i++) {
  if(name>=BP_NAMES+sizeof BP_NAMES)return 0;
  BP_ARRAYS[a][i].name=name;while(*name++){if(name==BP_NAMES+sizeof BP_NAMES)return 0;}
 }
}
return src==end;
}'''%(len(stream),name_bits,'for(uint32_t i=0;i<sizeof BP_NAMES;i++)if(BP_NAMES[i]>0 && BP_NAMES[i]<32)BP_NAMES[i]|=64;' if name_bits else '',len(stream),len(stream))]
    (gen/'boot_presets.h').write_text('\n'.join(code)+'\n')
    (gen/'boot_presets_oracle.json').write_text(json.dumps(e,indent=2))
    if len(zones)!=1:raise ValueError('expected one sample zone table')
    zone=zones[0];packed=lz4blk.compress_best(zone)
    if lz4blk.decompress(packed)!=zone:raise ValueError('zone round trip')
    (gen/'boot_zones_oracle.bin').write_bytes(zone)
    (gen/'boot_zones.h').write_text(
        '/* Generated from raw target-compiled sample-zone metadata. */\n'
        'static const uint8_t BOOT_ZONES[]={'+','.join(map(str,packed))+'};\n'
        'static int boot_zones_init(void) {\n'
        '_Static_assert(sizeof SMP_ZONES=='+str(len(zone))+',"zone ABI");\n'
        'const boot_table_t t={SMP_ZONES,BOOT_ZONES,sizeof BOOT_ZONES,sizeof SMP_ZONES,1,0,1,0};\n'
        'return boot_table_unpack(&t);\n}\n',encoding='utf-8',newline='\n')
    return e,len(stream),len(blob)

if __name__=='__main__':
    import sys
    e,raw,packed=pack(sys.argv[1],sys.argv[2]);print(len(e),sum(t['count'] for t in e),raw,packed)

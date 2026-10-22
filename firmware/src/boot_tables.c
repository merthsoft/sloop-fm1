/* SPDX-License-Identifier: GPL-3.0-only */
/* Runs only at cold startup, before the audio interrupt and second core start. */
typedef struct {
    void *dst;
    const uint8_t *src;
    uint16_t packed, count;
    uint8_t unit, order, width, bits; /* bits: direct packed residuals, 0 = LZ4 */
} boot_table_t;
static uint32_t boot_load(const uint8_t *p,uint32_t unit)
{
    uint32_t v=0;
    for(uint32_t j=0;j<unit;j++) v|=(uint32_t)p[j]<<(8u*j);
    return v;
}
static void boot_store(uint8_t *p,uint32_t unit,uint32_t v)
{
    for(uint32_t j=0;j<unit;j++) p[j]=(uint8_t)(v>>(8u*j));
}
static int boot_table_unpack(const boot_table_t *t)
{
    uint8_t *out=t->dst;
    const uint8_t *src=t->src,*end=src+t->packed;
    uint32_t prefix=t->order*t->unit;
    if(t->order>4u || t->count<t->order || (t->unit!=1u && t->unit!=2u && t->unit!=4u) ||
       (t->width!=1u && t->width!=t->unit)) return 0;
    if(t->bits) {
        uint32_t n=t->count-t->order,bit=0;
        if(t->bits>31u || (uint32_t)t->packed!=prefix+4u+(n*t->bits+7u)/8u) return 0;
        for(uint32_t i=0;i<prefix;i++)out[i]=*src++;
        uint32_t base=boot_load(src,4);src+=4;
        for(uint32_t i=t->order;i<t->count;i++) {
            uint32_t v=0;
            for(uint32_t b=0;b<t->bits;b++,bit++)v|=(uint32_t)((src[bit>>3]>>(bit&7u))&1u)<<b;
            boot_store(out+i*t->unit,t->unit,base+v);
        }
        goto integrate;
    }
    uint32_t expected=prefix+(t->count-t->order)*t->width,at=0;
    while(src<end) {
        uint32_t token=*src++,n=token>>4,b;
        if(n==15u) do {if(src==end)return 0;b=*src++;n+=b;} while(b==255u);
        if(n>expected-at || (uint32_t)(end-src)<n)return 0;
        while(n--)out[at++]=*src++;
        if(src==end)break;
        if(end-src<2)return 0;
        uint32_t offset=src[0]|(uint32_t)src[1]<<8;src+=2;
        if(!offset || offset>at)return 0;
        n=token&15u;
        if(n==15u) do {if(src==end)return 0;b=*src++;n+=b;} while(b==255u);
        n+=4u;
        if(n>expected-at)return 0;
        while(n--) {out[at]=out[at-offset];at++;}
    }
    if(at!=expected)return 0;
    /* Expand narrow signed residuals backwards, preserving unread compressed residuals. */
    for(uint32_t i=t->count;i>t->order;) {
        --i;
        uint32_t v=boot_load(out+prefix+(i-t->order)*t->width,t->width);
        if(t->width==1u && t->unit>1u)v=(uint32_t)(int32_t)(int8_t)v;
        boot_store(out+i*t->unit,t->unit,v);
    }
integrate:
    for(uint32_t i=t->order;i<t->count && t->order;i++) {
        uint32_t v=boot_load(out+i*t->unit,t->unit);
        uint32_t a=boot_load(out+(i-1u)*t->unit,t->unit);
        if(t->order==1u)v+=a;
        else {
            uint32_t b=boot_load(out+(i-2u)*t->unit,t->unit);
            if(t->order==2u)v+=2u*a-b;
            else {
                uint32_t c=boot_load(out+(i-3u)*t->unit,t->unit);
                if(t->order==3u)v+=3u*a-3u*b+c;
                else v+=4u*a-6u*b+4u*c-boot_load(out+(i-4u)*t->unit,t->unit);
            }
        }
        boot_store(out+i*t->unit,t->unit,v);
    }
    return 1;
}

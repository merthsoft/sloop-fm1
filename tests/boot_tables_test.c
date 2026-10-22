/* SPDX-License-Identifier: GPL-3.0-only */
#define FELUCCA_BOOT_TABLES 1
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "felucca_tables.h"
typedef struct {               /* proportional, see tools/gen_font.py */
    uint8_t h;
    uint8_t pad;               /* bitmap starts this many pixels left of the pen */
    uint8_t first, last;
    uint8_t sh;                /* 1: the bitmaps are another font's, drawn at 2x (L = S at 2x, no copy) */
    const uint8_t *adv;        /* advance per glyph */
    const uint8_t *bw;         /* bitmap width per glyph (starts FONT_PAD left of the pen) */
    const uint16_t *off;       /* byte offset of each glyph */
    const uint8_t *data;
    uint8_t bits;              /* SLOOP 2.6: 1 = 1 bit a pixel (a pixel font: glyph rows one after the other, high
                                * bit first, each glyph from a whole byte); 0 = 2 pixels a byte, alpha 0..15 */
} felucca_font_t;
#include "felucca_font.h"
#define DS_LANES 16
 typedef struct {const char *name,*style;uint8_t crush,base[DS_LANES],level[DS_LANES];} ds_factory_kit_t;
#include "felucca_drumkits.h"
#include "uac_fir.h"
#include "felucca_fm6.h"
#include "boot_tables_init.h"
int main(int argc,char **argv)
{
    assert(argc==2);
    FILE *f=fopen(argv[1],"rb");assert(f);
    assert(boot_tables_init());
    unsigned total=0;
    for(unsigned i=0;i<sizeof BOOT_TABLES/sizeof BOOT_TABLES[0];i++) {
        const boot_table_t *t=&BOOT_TABLES[i]; const uint8_t *p=t->dst;
        for(unsigned j=0;j<t->count*t->unit;j++) {int b=fgetc(f);assert(b!=EOF && (uint8_t)b==p[j]);total++;}
    }
    assert(fgetc(f)==EOF);fclose(f);
    FILE *fir=fopen("build/gen/uac_fir_oracle.bin","rb");assert(fir);
    const uint8_t *fir_bytes=(const uint8_t *)UAC_FIR;
    for(unsigned i=0;i<sizeof UAC_FIR;i++){int b=fgetc(fir);assert(b!=EOF && (uint8_t)b==fir_bytes[i]);}
    assert(fgetc(fir)==EOF);fclose(fir);
    uint8_t out[4]={0,0,0xaa,0xbb},bad_offset[]={0,0,0},too_long[]={0x50,1,2,3,4,5};
    boot_table_t bad={out,bad_offset,3,1,1,0,1};assert(!boot_table_unpack(&bad));
    bad.src=too_long;bad.packed=6;assert(!boot_table_unpack(&bad));assert(out[2]==0xaa && out[3]==0xbb);
    uint8_t bitstream[5]={0};
    boot_table_t bits_bad={out,bitstream,5,1,1,0,1,32};assert(!boot_table_unpack(&bits_bad));
    bits_bad.bits=2;bits_bad.packed=4;assert(!boot_table_unpack(&bits_bad));
    bits_bad.packed=5;bits_bad.count=8;assert(!boot_table_unpack(&bits_bad));
    assert(out[2]==0xaa && out[3]==0xbb);
    printf("boot tables: %u restored bytes match the original tables; malformed input bounds PASS\n",total);
    return 0;
}

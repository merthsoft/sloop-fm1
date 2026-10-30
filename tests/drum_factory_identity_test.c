/* SPDX-License-Identifier: GPL-3.0-only */
/* Compare restored factory lookup with frozen originals plus the three
 * explicitly reviewed snare edits. All other records stay byte-identical. */
#undef NDEBUG
#define FELUCCA_BOOT_TABLES 1
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>

int main(int argc,char **argv)
{
    _Static_assert(sizeof(dsnd_t)==22,"factory fixture sound layout");
    host_tracks_init();
    assert(DS_NKITS==32);
    FILE *f=fopen(argc>1?argv[1]:"tests/fixtures/drum-factory-sounds.bin","rb");
    assert(f);
    for(uint32_t kit=0;kit<DS_NKITS;kit++)for(uint32_t lane=0;lane<DS_LANES;lane++) {
        dsnd_t sound,expected;
        ds_factory_sound(kit+2u*DS_NKITS,lane+2u*DS_LANES,&sound);
        assert(fread(&expected,1,sizeof expected,f)==sizeof expected);
        if(lane==1u) {
            if(kit==0u){expected.decay=68;expected.t2lev=76;expected.click=12;expected.ndec=74;}
            if(kit==2u){expected.decay=59;expected.t2lev=44;expected.click=14;expected.ndec=68;expected.level=136;}
            if(kit==4u){expected.decay=54;expected.click=8;expected.ndec=62;}
        }
        assert(!memcmp(&sound,&expected,sizeof sound));
    }
    assert(fgetc(f)==EOF);assert(!fclose(f));
    puts("factory drums: 509 unchanged records + 3 reviewed snares, actual restored lookup and modulo indices PASS");
    return 0;
}

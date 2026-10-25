/* SPDX-License-Identifier: GPL-3.0-only */
/* Compare declaration defaults/raw zones with the actual startup metadata restoration. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
int main(int argc,char **argv)
{
    assert(argc==2);host_tracks_init();
    assert(sizeof(smp_zone_t)==28);
    FILE *out=fopen(argv[1],"wb");assert(out);
    assert(fwrite(live_mod,sizeof live_mod,1,out)==1);
    assert(fwrite(&dust,sizeof dust,1,out)==1);
    assert(fwrite(SMP_ZONES,sizeof SMP_ZONES,1,out)==1);
    assert(!fclose(out));
    /* Reinitializing tracks must not reset active performance or the dust RNG. */
    live_mod[0].depth=101;dust.rnd=123456;host_tracks_init();
    assert(live_mod[0].depth==101 && dust.rnd==123456);
    printf("startup state: %zu state bytes and %zu target zone bytes, startup defaults preserved PASS\n",sizeof live_mod+sizeof dust,sizeof SMP_ZONES);
    return 0;
}

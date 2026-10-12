/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
int main(void)
{
    host_tracks_init();track_t *t=&trk[0];
    static const int legacy[4][3]={{0,0,0},{0,12,0},{0,4,7},{0,3,7}};
    for(int ar=0;ar<4;ar++){t->p[P_E6]=ar;for(int tick=0;tick<24;tick++)assert(lofi_arp_offset(t,60,tick)==legacy[ar][tick%3]);}
    assert(NELEM(N_RARP)==CH_COUNT+1);
    for(int scale=0;scale<NSCALES;scale++)for(int root=0;root<12;root++)for(int note=0;note<128;note++)for(int type=1;type<CH_COUNT;type++) {
        if(type==CH_MAJOR||type==CH_MINOR)continue; /* Covered by the unchanged legacy modes above. */
        t->p[P_SCALE]=scale;t->p[P_ROOT]=root;t->p[P_CHORD]=type;
        t->p[P_E6]=type+(type>=CH_DOM7?1:3);uint8_t chord[4];uint32_t count=chord_notes(t,note,chord);
        for(int tick=0;tick<8;tick++)assert(lofi_arp_offset(t,note,tick)==(count?(int)chord[tick%count]-note:0));
    }
    t->p[P_SCALE]=1;t->p[P_ROOT]=0;t->p[P_QUANT]=0;t->p[P_E6]=CH_MAJ7+1;
    assert(lofi_arp_offset(t,60,0)==0&&lofi_arp_offset(t,60,1)==4&&lofi_arp_offset(t,60,2)==7&&lofi_arp_offset(t,60,3)==11);
    t->p[P_E6]=CH_HALFDIM+1;assert(lofi_arp_offset(t,60,1)==3&&lofi_arp_offset(t,60,2)==6&&lofi_arp_offset(t,60,3)==10);
    puts("LOFI arp: legacy pitches unchanged; every SEL chord shape/scale/root and MIDI range boundary PASS");return 0;
}

/* SPDX-License-Identifier: GPL-3.0-only */
/* Distinguish master-bus processing from note allocation. Host rendering has no
 * hardware deadline; this cannot prove that the device avoids CPU shedding. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>

int main(void)
{
    static const uint8_t chord[4][3]={{65,69,72},{65,69,72},{60,64,67},{60,64,67}};
    int32_t out[CTL*2];
    host_tracks_init();voice_expand=4;
    track_t *t=&trk[0];host_preset(t,ENGI_GRAIN,2); /* FLUTE DUST */
    t->p[P_VOICE]=V_POLY;
    for(uint32_t type=0;type<NELEM(N_LOFI);type++) {
        song.g[G_LTYPE]=(int16_t)type;
        song.g[G_VWOW]=80;song.g[G_VTONE]=64;song.g[G_VCRACKLE]=40;
        for(uint32_t c=0;c<4;c++) {
            for(uint32_t i=0;i<3;i++)trk_note_on(t,chord[c][i],100);
            uint32_t kills=voice_kills;
            for(uint32_t b=0;b<300;b++) {
                if(!(b%64u))drum_on((b%128u)?38:36,100);
                mix_block(out,CTL);
                for(uint32_t i=0;i<3;i++) {
                    uint32_t found=0;
                    for(uint32_t v=0;v<NVOICE;v++)
                        found|=t->v[v].active && t->v[v].gate && t->v[v].note==chord[c][i];
                    assert(found);
                }
                assert(voice_kills==kills);
            }
            for(uint32_t i=0;i<3;i++)trk_note_off(t,chord[c][i]);
        }
    }
    puts("LOFI voice isolation: all master types preserve Flute Dust 4411 chord gates with drums; no allocation kills (hardware deadlines not simulated) PASS");
    return 0;
}

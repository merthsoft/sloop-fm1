/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>

static int gain(int setting, uint32_t t)
{
    song.g[G_DUCK] = setting;
    drums.kick = 0;
    duck.t = t;
    duck_block(0);
    return duck.g1;
}

int main(void)
{
    const uint32_t len = BEAT_U / 2u;
    song.g[G_DREC] = 1;
    for (uint32_t t = 0; t <= len; t += len / 256u) {
        int previous = 32767;
        for (int setting = 0; setting <= 127; setting++) {
            int g = gain(setting, t);
            assert(g >= 0 && g <= previous);
            previous = g;
        }
        assert(gain(0, t) == 32767);
        if (t < len) {
            int x = 32767 - (int)((t << 10) / (len >> 5));
            int old = 32767 - mulq15(127 * 258, mulq15(x, x));
            assert(abs(gain(64, t) - old) <= 258);
        }
    }
    assert(gain(127, len / 4) == 0);
    assert(gain(64, len / 4) > 14000);
    assert(gain(127, len) == 32767);
    gain(127, len);
    drums.kick = 1;
    duck_block(123);
    assert(!drums.kick && duck.t == 123 && duck.g0 == 32767 && duck.g1 == 0);
    duck_block(123);
    assert(duck.g0 == 0 && duck.g1 == 0);
    for (int rate = 0; rate < 6; rate++) {
        song.g[G_DREC] = rate;
        uint32_t duration = BEAT_U / DIV_DEN[rate];
        for (int shape = 0; shape < 3; shape++) {
            song.g[G_DSHAPE] = shape;
            for (int setting = 0; setting <= 127; setting++) {
                int previous = 0;
                for (uint32_t i = 0; i <= 256; i++) {
                    int g = gain(setting, duration * i / 256u);
                    assert(g >= previous && g <= 32767);
                    previous = g;
                }
                assert(previous == 32767);
            }
        }
    }
    song.g[G_DREC] = 1;
    int mid[3];
    for (int shape=0;shape<3;shape++) { song.g[G_DSHAPE]=shape;mid[shape]=gain(64,len/2); }
    assert(mid[0]>mid[1] && mid[1]>mid[2]);
    /* Every GM note follows its hardware lane, in sampled/factory/user SYN kits.
     * Click/count-in notes never trigger even in ANY mode. */
    static const uint32_t kits[]={0,DRUM_SAMPLED,DRUM_SYN};
    for(uint32_t kit=0;kit<3;kit++) {
        TDRUM->p[P_E0]=kits[kit];
        for(int trigger=0;trigger<4;trigger++) {
            song.g[G_DTRIG]=trigger;
            for(uint32_t note=0;note<128;note++) {
                memset(&drums,0,sizeof drums);
                uint32_t lane=lane_of_note(note);
                int kick=lane<2, snare=lane==2 || lane==8;
                int expected=!DRUM_CLICK(note) && (trigger==3 || (trigger==0?kick:trigger==1?snare:kick||snare));
                drum_on(note,100);
                assert(!!drums.kick==expected);
            }
        }
    }
    host_tracks_init();
    for(uint32_t part=0;part<NPART;part++) {
        track_t *t=&trk[part];host_preset(t,0,0);
        t->p[P_ATK]=0;t->p[P_SUS]=127;t->p[P_DLY]=64;t->p[P_REV]=64;
        trk_note_on(t,60,100);
        for(uint32_t b=0;b<16;b++) {
            mixacc_t m={0};int32_t scratch[CTL];
            duck.g0=duck.g1=0;duck_bypass[part]=b&1u;
            mix_part_run(t,CTL,&m,scratch);
            int64_t energy=0;
            for(uint32_t j=0;j<CTL;j++)energy+=llabs(m.l[j])+llabs(m.r[j])+llabs(m.sd[j])+llabs(m.sr[j]);
            assert((b&1u)?energy>0:energy==0);
        }
        /* Bypassing DUCK must never bypass track mute. */
        t->p[P_MUTE]=1;
        for(uint32_t b=0;b<200;b++){mixacc_t m={0};int32_t scratch[CTL];mix_part_run(t,CTL,&m,scratch);}
        mixacc_t m={0};int32_t scratch[CTL];mix_part_run(t,CTL,&m,scratch);
        for(uint32_t j=0;j<CTL;j++)assert(!m.l[j]&&!m.r[j]&&!m.sd[j]&&!m.sr[j]);
    }
    puts("DUCK: curves, triggers, independent track bypass, sends and mute PASS");
    return 0;
}

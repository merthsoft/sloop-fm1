/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
int main(int argc, char **argv)
{
    panel=PANEL_DEFAULT;host_tracks_init();palette_set(4);
    outdir=argc>1?argv[1]:"build/host";
    vis_style=9;vis_update();
    for(uint32_t t=0;t<NPART;t++) {
        trk[t].p[P_MUTE]=0;song.solo=0;
        for(uint32_t n=0;n<128;n++) { trk_note_on(&trk[t],n,100);trk_note_off(&trk[t],n); }
        /* A stolen/finished voice is gone before the next display frame. */
        memset(trk[t].v,0,sizeof trk[t].v);
    }
    pads_tick();pads_tick();vis_update(); /* visualizer draws every other UI frame */
    for(uint32_t t=0;t<NPART;t++) for(uint32_t w=0;w<4;w++)
        assert(vis_notes[vis_notes_head][t][w]==0xFFFFFFFFu);
    pads_tick();vis_update();
    for(uint32_t t=0;t<NPART;t++) for(uint32_t w=0;w<4;w++)
        assert(!vis_notes[vis_notes_head][t][w]);
    /* Coincident pitches retain all track colors, including the newest column. */
    memset(&vis_frame.trails,0,sizeof vis_frame.trails);
    for(uint32_t t=0;t<NPART;t++) vis_notes[vis_notes_head][t][60>>5]=1u<<(60&31);
    vis_drum_notes[vis_notes_head]=1u;
    cv_begin(240,120,C_BLACK);cv_oy=-120;vis_trails();
    for(uint32_t t=0;t<NPART;t++) assert(swap16(cv_px[6*240+235+t])==TE_COL[t]);
    assert(swap16(cv_px[6*240+239])==C_WHITE);
    assert(swap16(cv_px[103*240+235])==C_WHITE);
    assert(swap16(cv_px[104*240+235])==C_BLACK);
    cv_blit(0,120);cv_oy=0;ppm("note-trails-overlap");
    /* Held voices remain a continuous trail; muted parts show no queued attacks. */
    trk[0].v[0].active=1;trk[0].v[0].gate=1;trk[0].v[0].note=127;
    trk[1].p[P_MUTE]=1;vis_note_hits[1][0]=1;
    vis_update();assert(vis_notes[vis_notes_head][0][3]&(1u<<31));
    assert(!vis_notes[vis_notes_head][1][0]);
    /* Released voices and echo energy do not turn tails into note events. */
    trk[0].v[0].gate=0;punch.cur=PX_ECHO;punch.g=32767;
    vis_update();assert(!vis_notes[vis_notes_head][0][3]);
    /* A held source chord follows the actual full-wet gate cut/reopen phase. */
    trk[0].v[0].gate=1;punch.cur=PX_GATE;punch.gp=0;pad_lit[0]=6;
    vis_update();assert(vis_notes[vis_notes_head][0][3]&(1u<<31));assert(vis_drum_notes[vis_notes_head]&1);
    punch.gp=beat_samples()/8u+64u;vis_note_hits[0][0]=1;
    vis_update();for(uint32_t t=0;t<NPART;t++)for(uint32_t w=0;w<4;w++)assert(!vis_notes[vis_notes_head][t][w]);
    assert(!vis_drum_notes[vis_notes_head]);
    punch.gp=0;vis_update();assert(vis_notes[vis_notes_head][0][3]&(1u<<31));
    assert(!vis_notes[vis_notes_head][0][0]); /* Closed-window attacks were consumed, not delayed into reopen. */
    punch.gp=beat_samples()/8u+64u;punch.g=16000;vis_update();
    assert(vis_notes[vis_notes_head][0][3]&(1u<<31)); /* Dry source remains during effect crossfade. */
    punch.cur=-1;punch.g=0;
    /* Held chords must form uninterrupted lines, rather than short dashes. */
    memset(&vis_frame.trails,0,sizeof vis_frame.trails);vis_notes_head=47;
    for(uint32_t i=0;i<48;i++) for(uint32_t t=0;t<NPART;t++)
        for(uint32_t k=0;k<3;k++) {
            uint32_t n=48+t*12+k*4;
            vis_notes[i][t][n>>5]|=1u<<(n&31);
        }
    for(uint32_t pass=0;pass<2;pass++) {
        cv_begin(240,120,C_BLACK);cv_oy=-(int32_t)(pass*120);vis_trails();
        for(uint32_t t=0;t<NPART;t++) for(uint32_t k=0;k<3;k++) {
            uint32_t n=48+t*12+k*4,y=216-n*3/2;
            if(y>=pass*120&&y<(pass+1)*120) for(uint32_t x=t;x<239;x++)
                assert(swap16(cv_px[(y-pass*120)*240+x])==TE_COL[t]);
        }
    }
    cv_oy=0;
    /* Fresh gallery rendering from the actual production drawing routine. */
    {
        const uint8_t notes[]={48,50,51,53,55,56,58,60};
        memset(&vis_frame.trails,0,sizeof vis_frame.trails);
        for(uint32_t i=0;i<48;i++) {
            for(uint32_t t=0;t<NPART;t++) {
                uint32_t n=notes[(i/4+t*2)%8]+t*12;
                vis_notes[i][t][n>>5]|=1u<<(n&31);
            }
            vis_drum_notes[i]=(i%8==0?1u:0u)|(i%8==4?2u:0u)|(i%2==0?4u:0u);
        }
        for(uint32_t pass=0;pass<2;pass++) {
            cv_begin(240,120,C_BLACK);cv_oy=-(int32_t)(pass*120);vis_trails();cv_blit(0,pass*120);
        }
        cv_oy=0;ppm("note-trails-gallery");
    }
    puts("note trails: all 128 pitches/all synth tracks, between-frame attacks, overlap colors, sustain and mute PASS");
    return 0;
}

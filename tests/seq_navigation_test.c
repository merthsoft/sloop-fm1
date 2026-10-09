/* SPDX-License-Identifier: GPL-3.0-only */
/* Physical controls through production ui_input, with audio running each frame. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static void expect_page(const char *title)
{
    assert(!ui.home && !strcmp(cur_page()->title,title));
}
static void select_turn(int32_t amount)
{ encs[panel.enc[EN_SELECT]]=amount;frame(); }
int main(void)
{
    panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(4);
    song.sel=1;go_home();frame();
    tap(B_SEQ);expect_page("STEP");
    select_turn(1);expect_page("PATTERN");
    select_turn(1);expect_page("SEQUENCES");assert(sequence_browser);
    select_turn(1);expect_page("SEQUENCES"); /* shaping subpage */
    select_turn(1);expect_page("SONG");assert(!sequence_browser);
    select_turn(-1);expect_page("SEQUENCES");
    select_turn(-1);expect_page("PATTERN");
    select_turn(-1);expect_page("STEP");
    go_home();frame();
    static const char *const order[]={"STEP","PATTERN","SEQUENCES","SONG","STEP"};
    for(uint32_t i=0;i<5;i++) {tap(B_SEQ);expect_page(order[i]);assert(song.sel==1);}
    select_turn(9);expect_page("SONG");tap(B_ENV);expect_page("ENV");
    tap(B_ENV);expect_page("ENV DEST");
    tap(B_SEQ);select_turn(9);expect_page("SONG");tap(B_LFO);expect_page("LFO");
    /* A sequence-derived chord sustains through fifteen ties while the actual
     * ARP MODE knob edits only track2, never the neighboring synths. */
    for(uint32_t i=0;i<NTRK;i++) steps_clear(&trk[i]);
    track_t *t=&trk[1];
    t->step[0].time=ST_NOTE;t->step[0].n=4;
    t->step[0].note[0]=60;t->step[0].note[1]=64;t->step[0].note[2]=67;t->step[0].note[3]=69;
    for(uint32_t i=1;i<16;i++)t->step[i].time=ST_TIE;
    t->p[P_SLEN]=16;t->p[P_AORDER]=AORDER_SEQ_NOTE;t->p[P_AOCT]=1;t->p[P_AMODE]=ARP_UP;
    trk[0].p[P_AMODE]=ARP_DOWN;trk[2].p[P_AMODE]=ARP_UPDOWN;
    seq_start();frame();assert(song.playing&&seq_harmony[1].n==4);
    tap(B_ARP);expect_page("ARP");frames(12); /* release debounce/encoder quiet */
    encs[panel.enc[EN_K1]]=-100;frame();assert(t->p[P_AMODE]==ARP_OFF);
    for(uint32_t mode=1;mode<ARP_COUNT;mode++) {
        frames(12);encs[panel.enc[EN_K1]]=1;frame();
        assert(t->p[P_AMODE]==(int16_t)mode);
        assert(trk[0].p[P_AMODE]==ARP_DOWN&&trk[2].p[P_AMODE]==ARP_UPDOWN);
        assert(song.sel==1&&song.playing);frame();
        assert(seq_harmony[1].n==4);
    }
    seq_stop();
    puts("native UI: SEQ button/SELECT reach STEP, PATTERN, SEQUENCES, SONG; ENV/LFO escape; all ARP modes edit selected SNOTE track while playing PASS");
    return 0;
}

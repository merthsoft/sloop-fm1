/* SPDX-License-Identifier: GPL-3.0-only */
/* Real UI edits with the audio/MIDI scheduler running, not a copied event model. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static unsigned sounding[16][128], on_count[16][128], off_count[16][128];
static void midi_drain(void)
{
    while(mo_r!=mo_w) {
        uint32_t packet=midi_out_q[mo_r++%MOUT_Q],status=(packet>>8)&255u,note=(packet>>16)&127u,ch=status&15u;
        if((status&240u)==0x90u) {assert(!sounding[ch][note]);sounding[ch][note]=1;on_count[ch][note]++;}
        if((status&240u)==0x80u) {assert(sounding[ch][note]);sounding[ch][note]=0;off_count[ch][note]++;}
    }
}
static void pump(void){int32_t out[CTL*2];mix_block(out,CTL);midi_drain();}
static void until_step(unsigned step)
{
    unsigned timeout=FS*4/CTL;
    while(TSEL->seq_idx!=step&&timeout--)pump();
    assert(timeout&&song.playing);
}
static void begin(unsigned ratchet)
{
    seq_stop();midi_drain();host_tracks_init();song.rec=rec_wait=ft_on=transport_req=0;
    fm1_in.buttons=fm1_in.notes=0;go_home();song.sel=0;ui.menu=ui.confirm=0;undo.valid=0;
    song.g[G_MIDI]=1;usb.config=1;arrangement_enabled=0;
    memset(sounding,0,sizeof sounding);memset(on_count,0,sizeof on_count);memset(off_count,0,sizeof off_count);
    mo_r=mo_w=0;memset(mo_set,0,sizeof mo_set);
    for(unsigned i=0;i<2;i++) {
        track_t *t=&trk[i];t->p[P_SLEN]=8;t->p[P_SDIV]=2;t->p[P_SGATE]=128;
        t->p[P_VOICE]=V_POLY;t->p[P_AMODE]=t->p[P_STRUM]=t->p[P_CHORD]=0;
        t->p[P_TRANS]=0;
    }
    TSEL->step[0]=(step_t){.time=ST_NOTE,.n=4,.note={60,64,67,71},.vel=90,.rat=ratchet?255:0};
    if(!ratchet)assert(!note_extend(TSEL,0,3,0,1));
    trk[1].step[0]=(step_t){.time=ST_NOTE,.n=1,.note={48},.vel=90};
    assert(!note_extend(&trk[1],0,6,0,1));
    for(unsigned p=0;p<NPAGES;p++)if(!strcmp(PAGES[p].title,"NOTES"))ui.page=p;
    ui.home=0;ui.cursor=0;page_entered();roll_select((piano_selection_t){0,64});
    seq_start();pump();assert(TSEL->seq_n==4&&trk[1].seq_n==1);
}
static void finish(void)
{
    seq_stop();midi_drain();
    for(unsigned ch=0;ch<16;ch++)for(unsigned n=0;n<128;n++) {
        assert(!sounding[ch][n]);assert(on_count[ch][n]==off_count[ch][n]);
    }
}
static void live_pitch_and_length(void)
{
    begin(0);step_edit(1,1);assert(ui.roll_pitch==65&&note_chain_len(TSEL,0,65)==4);
    /* Edit is not an immediate reattack or all-notes-off. */
    assert(sounding[0][64]&&!sounding[0][65]&&sounding[1][48]);
    until_step(1);assert(!sounding[0][64]&&sounding[0][65]);
    assert(on_count[0][60]==1&&on_count[0][67]==1&&on_count[0][71]==1&&on_count[1][48]==1);
    step_edit(2,-2);assert(note_chain_len(TSEL,0,65)==2);
    until_step(2);assert(!sounding[0][65]&&sounding[0][60]&&sounding[1][48]);
    until_step(0);assert(sounding[0][65]&&on_count[0][65]==2);
    finish();
}
static void live_ratchet_pitch(void)
{
    begin(1);step_edit(1,1);assert(TSEL->step[0].note[1]==65&&TSEL->seq_skip);
    until_step(1);assert(!on_count[0][65]);
    /* The next attack reads the edited pitch; it is tracked and released. */
    until_step(0);assert(on_count[0][65]==1);until_step(1);assert(on_count[0][65]==4);
    finish();
}
static void live_actions(void)
{
    begin(0);actions_open(0,0,0);assert(actions_ready());
    note_action.row=4;note_action.dest=4;actions_apply();
    assert(note_chain_len(TSEL,4,64)==4&&note_slot(&TSEL->step[0],64)==TSEL->step[0].n);
    assert(song.playing&&sounding[0][64]&&sounding[1][48]);
    until_step(1);assert(!sounding[0][64]&&sounding[0][60]&&on_count[1][48]==1);
    until_step(4);assert(sounding[0][64]);
    note_action.row=6;actions_apply();assert(note_slot(&TSEL->step[4],64)==TSEL->step[4].n);
    until_step(5);assert(!sounding[0][64]&&on_count[1][48]==1);finish();
}
static void live_move_brush(void)
{
    begin(0);ui.cursor=0;ui.tie_sess=(undo_sess+=4u)|3u;step_move(2);
    assert(song.playing&&TSEL->step[0].time==ST_REST&&TSEL->step[2].n==4);
    assert(undo_swap(0)&&song.playing&&TSEL->step[0].n==4&&on_count[1][48]==1);
    assert(undo_swap(1)&&song.playing&&TSEL->step[0].time==ST_REST&&TSEL->step[2].n==4);
    until_step(1);assert(!TSEL->seq_n&&on_count[1][48]==1);
    until_step(2);assert(TSEL->seq_n==4);finish();
}
static void recording_ownership(void)
{
    begin(0);step_t before[NSTEP];memcpy(before,TSEL->step,sizeof before);
    song.rec=1;step_edit(1,1);assert(!memcmp(before,TSEL->step,sizeof before)&&!strcmp(ui.msg,"STOP RECORDING"));
    step_move(1);assert(!memcmp(before,TSEL->step,sizeof before));
    song.rec=0;rec_wait=1;step_edit(2,1);assert(!memcmp(before,TSEL->step,sizeof before));
    rec_wait=0;finish();
}
int main(void)
{
    panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(4);outdir="build/host";
    live_pitch_and_length();live_ratchet_pitch();live_actions();live_move_brush();recording_ownership();
    puts("live note editor: pitch/length/move/delete, next-loop playback, ratchet capacity, other chord/track isolation, balanced MIDI and recording ownership PASS");
}

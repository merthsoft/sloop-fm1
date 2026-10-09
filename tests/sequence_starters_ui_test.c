/* SPDX-License-Identifier: GPL-3.0-only */
#undef NDEBUG
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"

static int same_pattern(const track_t *a,const track_t *b)
{
    return !memcmp(a->step,b->step,sizeof a->step)&&!memcmp(a->micro,b->micro,sizeof a->micro)&&
        !memcmp(a->fill,b->fill,sizeof a->fill)&&!memcmp(a->lock,b->lock,sizeof a->lock)&&
        a->p[P_SLEN]==b->p[P_SLEN]&&a->p[P_SDIV]==b->p[P_SDIV]&&
        a->p[P_SSWING]==b->p[P_SSWING]&&a->seq_active==b->seq_active;
}
int main(int argc,char **argv)
{
    static track_t before,after,other,latch_before;
    uint32_t i,g,scale,mode,j;
    panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(4);
    outdir=argc>1?argv[1]:"build/host";song.sel=1;
    for(g=0;g<NSEQUENCE_STARTERS;g++)for(scale=0;scale<NSCALES;scale++)
        for(mode=0;mode<3;mode++)for(i=0;i<NSTEP;i++) {
            step_t s=sequence_starter_voiced(g,i,11,scale,3,mode,0,1);
            assert(s.n<=4);
            for(j=0;j<s.n;j++) {
                if(s.note[j]>127||(j&&s.note[j]<=s.note[j-1]))
                    fprintf(stderr,"starter %u scale %u mode %u step %u: notes %u %u %u %u\n",g,scale,mode,i,s.note[0],s.note[1],s.note[2],s.note[3]);
                assert(s.note[j]<=127&&(!j||s.note[j]>s.note[j-1]));
            }
        }
    trk[1].step[3].n=1;trk[1].step[3].note[0]=64;trk[1].step[3].time=ST_NOTE;
    trk[1].micro[3]=-13;step_fill_set(&trk[1],3,FC_FILL);
    assert(lock_set(&trk[1],3,P_PAN,21));
    trk[1].p[P_SLEN]=11;trk[1].p[P_SDIV]=3;trk[1].p[P_SSWING]=17;
    trk[1].seq_active=0;before=trk[1];other=trk[0];
    for(i=0;i<NPAGES;i++)if(PAGES[i].scope==SC_STARTER)break;
    assert(i<NPAGES);ui.home=0;ui.page=(uint8_t)i;page_entered();
    assert(sequence_browser&&sequence_preview_tick);
    assert(sequence_scale==1&&trk[1].p[P_SCALE]==0);
    sequence_sel=0;sequence_root=0;sequence_scale=1;sequence_mode=0;
    sequence_shape=(rhythm_shape_t){0,0,0,255,0};
    sequence_screen_draw();ppm("live-sequence-starters");
    sequence_screen_input(BT(B_OCTUP),0);
    assert(sequence_confirm&&same_pattern(&before,&trk[1]));
    fm1_in.buttons=BT(B_OCTUP);fm1_ms=100;
    sequence_confirm=0;sequence_screen_input(BT(B_OCTUP),0);
    fm1_ms=799;sequence_screen_input(0,0);assert(same_pattern(&before,&trk[1]));
    fm1_ms=800;sequence_screen_input(0,0);fm1_in.buttons=0;
    assert(!sequence_confirm&&trk[1].p[P_SLEN]==64&&groove_undo_matches());
    after=trk[1];assert(!memcmp(&other,&trk[0],sizeof other));
    puts("sequence UI: hold threshold and track isolation PASS");fflush(stdout);
    assert(undo_swap(0)&&same_pattern(&before,&trk[1]));
    assert(undo_swap(1)&&same_pattern(&after,&trk[1]));
    puts("sequence UI: complete metadata undo/redo PASS");fflush(stdout);
    trk_note_on(&trk[1],60,100);
    trk[1].nheld=1;trk[1].held[0]=60;latch_before=trk[1];
    sequence_screen_input(BT(B_OCTDN),0);
    assert(!sequence_preview.active&&!strcmp(ui.msg,"RELEASE FIRST"));
    assert(!memcmp(&latch_before,&trk[1],sizeof latch_before));
    trk[1].nheld=0;chord_latch_n[1]=1;chord_latch_notes[1][0]=60;latch_before=trk[1];
    sequence_screen_input(BT(B_OCTDN),0);
    assert(!sequence_preview.active&&chord_latch_n[1]==1&&chord_latch_notes[1][0]==60);
    assert(!memcmp(&latch_before,&trk[1],sizeof latch_before));
    chord_latch_n[1]=0;arp_chord_latch_n[1]=1;arp_chord_latch_notes[1][0]=60;
    sequence_screen_input(BT(B_OCTDN),0);
    assert(!sequence_preview.active&&arp_chord_latch_n[1]==1&&arp_chord_latch_notes[1][0]==60);
    assert(!memcmp(&latch_before,&trk[1],sizeof latch_before));
    arp_chord_latch_n[1]=0;trk_note_off(&trk[1],60);
    puts("sequence UI: existing live chord/ARP latches preserve sounding voices PASS");fflush(stdout);
    sequence_screen_input(BT(B_OCTDN),0);assert(sequence_preview.active);
    assert(same_pattern(&after,&trk[1]));
    sequence_preview_block(0);assert(sequence_preview.n==3);
    seq_stop();assert(!sequence_preview.active&&sequence_preview.n==0);
    puts("sequence UI: preview and STOP release PASS");fflush(stdout);
    sequence_screen_input(BT(B_OCTDN),0);assert(sequence_preview.active);
    go_home();assert(!sequence_browser&&!sequence_preview.active&&!sequence_preview_tick);
    song.sel=1;ui.home=0;ui.page=(uint8_t)i;page_entered();
    sequence_screen_input(BT(B_OCTUP),0);assert(sequence_confirm);
    song.playing=1;fm1_in.buttons=BT(B_OCTUP);fm1_ms+=1000;
    sequence_screen_input(0,0);assert(same_pattern(&after,&trk[1]));
    song.playing=0;fm1_in.buttons=0;
    sequence_page=1;sequence_screen_draw();ppm("live-sequence-rhythm");
    encs[panel.enc[EN_SELECT]]=1;sequence_screen_input(0,0);
    assert(sequence_browser&&sequence_page==2);
    encs[panel.enc[EN_SELECT]]=1;sequence_screen_input(0,0);
    assert(!sequence_browser&&cur_page()->scope==SC_SONG);
    ui.page=(uint8_t)i;page_entered();assert(sequence_browser);
    encs[panel.enc[EN_SELECT]]=-1;sequence_screen_input(0,0);
    assert(!sequence_browser&&cur_page()->scope==SC_RECORD);
    ui.page=(uint8_t)i;page_entered();
    tap(B_OCTDN);assert(sequence_preview.active);
    puts("sequence UI: physical listen PASS");fflush(stdout);
    tap(B_PLAY);assert(song.playing&&!sequence_preview.active&&!sequence_browser);
    tap(B_PLAY);assert(!song.playing);
    ui.home=0;ui.page=(uint8_t)i;page_entered();
    tap(B_SEQ);assert(!sequence_browser&&cur_page()->scope==SC_SONG);
    ui.home=0;ui.page=(uint8_t)i;page_entered();
    encs[panel.enc[EN_SELECT]]=1;frame();assert(sequence_browser&&sequence_page==1);
    encs[panel.enc[EN_SELECT]]=1;frame();assert(sequence_browser&&sequence_page==2);
    int8_t physical_octave=song.octave;
    encs[panel.enc[EN_K1]]=1;frame();assert(sequence_octave==physical_octave+1);
    assert(song.octave==physical_octave);
    sequence_screen_draw();ppm("live-sequence-pitch");
    encs[panel.enc[EN_SELECT]]=1;frame();assert(!sequence_browser&&cur_page()->scope==SC_SONG);
    ui.home=0;ui.page=(uint8_t)i;page_entered();
    sequence_screen_close();assert(!sequence_preview_end);
    puts("sequence UI: native entry, 700ms hold, full undo/redo, track isolation, non-destructive preview, STOP/home ownership PASS");
    return 0;
}

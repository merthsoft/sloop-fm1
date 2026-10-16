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
    /* Musical oracle independent of the scale-degree mapper: enumerate pitches
     * over several octaves, including saturation at the MIDI bounds. */
    for (scale=0;scale<NSCALES;scale++) {
        uint8_t tones[12]; uint32_t n=0, degree, root;
        for (i=0;i<12;i++) if (SCALE_MASK[scale] & (1u<<i)) tones[n++]=(uint8_t)i;
        for (root=0;root<12;root++) for (int32_t octave=-6;octave<=6;octave++)
            for (degree=0;degree<32;degree++) {
                int32_t expected=60+(int32_t)root+12*(octave+(int32_t)(degree/n))+tones[degree%n];
                expected=expected<0?0:expected>127?127:expected;
                assert(sequence_degree_note(degree,root,scale,octave)==(uint32_t)expected);
            }
    }
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
    assert(sequence_browser&&sequence_page==3);
    encs[panel.enc[EN_SELECT]]=1;sequence_screen_input(0,0);
    assert(!sequence_browser&&cur_page()->scope==SC_SONG);
    ui.page=(uint8_t)i;page_entered();assert(sequence_browser);
    encs[panel.enc[EN_SELECT]]=-1;sequence_screen_input(0,0);
    assert(!sequence_browser&&cur_page()->scope==SC_RECORD);
    ui.page=(uint8_t)i;page_entered();
    tap(B_OCTDN);assert(sequence_preview.active);
    sequence_preview_block(0);
    assert(sequence_preview.n);
    {
        /* SELECT changes the controls in view, not audition ownership or phase. */
        uint8_t saved[sizeof sequence_preview];
        memcpy(saved,&sequence_preview,sizeof saved);
        for(uint32_t page=1;page<=3;page++) {
            encs[panel.enc[EN_SELECT]]=1;sequence_screen_input(0,0);
            assert(sequence_page==page&&!memcmp(saved,&sequence_preview,sizeof saved));
        }
        for(int page=2;page>=0;page--) {
            encs[panel.enc[EN_SELECT]]=-1;sequence_screen_input(0,0);
            assert(sequence_page==page&&!memcmp(saved,&sequence_preview,sizeof saved));
        }
        encs[panel.enc[EN_K2]]=1;sequence_screen_input(0,0);
        assert(!sequence_preview.active&&!sequence_preview.n);
    }
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
    int8_t browser_octave=sequence_octave;
    encs[panel.enc[EN_K1]]=1;frame();assert(sequence_octave==browser_octave+1);
    assert(song.octave==physical_octave);
    sequence_screen_draw();ppm("live-sequence-pitch");
    encs[panel.enc[EN_SELECT]]=1;frame();assert(sequence_browser&&sequence_page==3);
    encs[panel.enc[EN_SELECT]]=1;frame();assert(!sequence_browser&&cur_page()->scope==SC_SONG);
    ui.home=0;ui.page=(uint8_t)i;page_entered();
    sequence_screen_close();assert(!sequence_preview_end);
    sequence_sel=0;sequence_root=2;sequence_scale=2;sequence_octave=1;
    sequence_mode=STARTER_BASS;sequence_vlead=1;
    sequence_shape=(rhythm_shape_t){3,-2,7,255,1};
    other=trk[1];
    song.sel=0;song.octave=-2;
    trk[0].p[P_ROOT]=9;trk[0].p[P_SCALE]=1;trk[0].p[P_VLEAD]=0;
    sequence_screen_open();
    assert(sequence_root==2&&sequence_scale==2&&sequence_octave==1&&
           sequence_mode==STARTER_BASS&&sequence_vlead==1&&sequence_sel==0);
    assert(sequence_shape.rotate==3&&sequence_shape.offset==-2&&
           sequence_shape.feel==7&&sequence_shape.part==255&&sequence_shape.sync==1);
    sequence_screen_close();sequence_screen_open();
    assert(sequence_root==2&&sequence_scale==2&&sequence_octave==1);
    sequence_shape=(rhythm_shape_t){0,0,0,255,0};
    assert(sequence_starter_apply());
    assert(trk[0].step[0].n==1&&trk[0].step[0].note[0]==62&&
           trk[0].step[16].note[0]==69); /* D minor bass, rather than track 0's A major */
    assert(trk[0].p[P_ROOT]==9&&trk[0].p[P_SCALE]==1&&song.octave==-2);
    assert(!memcmp(&other,&trk[1],sizeof other));
    sequence_screen_close();
    puts("sequence UI: cross-track key, scale, octave and browser preferences persist; apply uses retained key PASS");
    sequence_screen_open();sequence_rhythm=0;sequence_shape=(rhythm_shape_t){0,0,0,255,0};
    sequence_mode=STARTER_CHORD;
    encs[panel.enc[EN_PRESET]]=3;sequence_screen_input(0,0);
    assert(sequence_rhythm==3&&!sequence_preview.active);
    ui.msg_t=0;sequence_screen_draw();ppm("live-sequence-rhythm-choice");
    assert(sequence_starter_apply());
    for(i=0;i<NSTEP;i++) assert((trk[0].step[i].time==ST_NOTE)==(i%4==0));
    sequence_screen_input(1u<<panel.btn[B_OCTDN],0);
    assert(sequence_preview.active&&sequence_preview.rhythm==3);
    sequence_preview_cancel();sequence_screen_close();song.sel=1;sequence_screen_open();
    assert(sequence_rhythm==3);
    sequence_screen_close();sequence_rhythm=0;
    puts("sequence UI: PRESET selects independent rhythm, preview/apply agree and choice persists across tracks PASS");
    /* Overlay tied layers: existing C holds under a new E attack, then releases
     * independently. No stale per-note dynamics leak into appended slots. */
    {
        step_t overlay[NSTEP] = {0};
        track_t *t=&trk[1];
        t->p[P_SLEN]=4;
        for(i=0;i<NSTEP;i++) { t->step[i]=(step_t){.time=ST_REST}; overlay[i].time=ST_REST; }
        t->step[0]=(step_t){.n=1,.note={60},.vel=80,.lvl=0xFC,.rat=0xFC};
        t->step[1].time=ST_TIE; t->step[2].time=ST_TIE;t->step[2].flags=SF_SLIDE;
        overlay[1]=(step_t){.n=1,.note={64},.vel=100,.lvl=LV_SOFT};
        overlay[2].time=ST_TIE; overlay[3].time=ST_TIE;
        assert(sequence_starter_add(t,overlay)==1);
        assert(overlay[0].n==1&&overlay[0].note[0]==60);
        assert(overlay[1].n==2&&overlay[1].note[0]==60&&overlay[1].note[1]==64);
        assert(overlay[1].vel==80&&((overlay[1].lvl>>2)&3)==LV_SOFT&&((overlay[1].rat>>2)&3)==0);
        assert(overlay[2].time==ST_TIE&&(overlay[2].flags&SF_SLIDE));
        assert(overlay[3].time==ST_NOTE&&overlay[3].n==1&&overlay[3].note[0]==64);
        /* Full chord plus duplicate is accepted; a fifth distinct pitch fails. */
        t->step[0]=(step_t){.n=4,.note={60,64,67,71}};
        overlay[0]=(step_t){.n=1,.note={64}};
        assert(sequence_starter_add(t,overlay)==1&&overlay[0].n==4);
        overlay[0]=(step_t){.n=1,.note={65}};
        assert(sequence_starter_add(t,overlay)==-1);
        song.sel=1; song.playing=song.rec=rec_wait=ft_on=transport_req=0;
        sequence_sel=0;sequence_mode=STARTER_CHORD;sequence_root=0;
        sequence_scale=1;sequence_octave=0;sequence_rhythm=0;sequence_vlead=0;
        t->step[0]=(step_t){.n=4,.note={48,52,55,59}};before=*t;
        assert(sequence_starter_apply_mode(1)==-1&&same_pattern(&before,t));
        t->step[0]=(step_t){.n=1,.note={48}};
        t->step[1]=(step_t){.time=ST_REST};
        t->step[2]=(step_t){.n=4,.note={49,52,55,59}};
        before=*t;
        uint32_t undo_before=undo.sess;
        assert(sequence_starter_apply_mode(1)==-1&&same_pattern(&before,t)&&undo.sess==undo_before);
        for(i=0;i<NSTEP;i++) t->step[i]=(step_t){.time=ST_REST};
        t->step[0]=(step_t){.n=1,.note={48},.vel=77};
        t->p[P_SDIV]=3;t->p[P_SSWING]=19;t->micro[0]=7;
        t->lock[0]=(plock_t){.step=0,.param=P_LEVEL,.val=60};
        t->fill[0]=1;before=*t;other=trk[0];
        assert(sequence_starter_apply_mode(1)==1);
        assert(t->step[0].n==4&&t->step[0].note[0]==48&&t->step[0].vel==77);
        assert(t->p[P_SLEN]==4&&t->p[P_SDIV]==3&&t->p[P_SSWING]==19&&t->micro[0]==7);
        assert(!memcmp(t->lock,before.lock,sizeof t->lock)&&!memcmp(t->fill,before.fill,sizeof t->fill));
        assert(!memcmp(&trk[0],&other,sizeof other));
        after=*t;assert(undo_swap(0));assert(same_pattern(t,&before));
        assert(undo_swap(1));assert(same_pattern(t,&after));
        sequence_screen_open();encs[panel.enc[EN_ALGO]]=1;sequence_screen_input(0,0);
        assert(sequence_add==1);encs[panel.enc[EN_ALGO]]=-1;sequence_screen_input(0,0);
        assert(sequence_add==0);sequence_screen_close();
        /* Both native Apply gestures must use ADD, rather than the replace wrapper. */
        assert(undo_swap(0));before=*t;sequence_screen_open();sequence_add=1;
        sequence_screen_input(BT(B_OCTUP),0);assert(sequence_confirm&&same_pattern(&before,t));
        sequence_screen_input(BT(B_OCTUP),0);assert(t->step[0].n==4&&t->step[0].note[0]==48);
        assert(t->p[P_SLEN]==4&&t->p[P_SDIV]==3&&t->micro[0]==7);
        assert(undo_swap(0)&&same_pattern(&before,t));
        fm1_ms=100;fm1_in.buttons=BT(B_OCTUP);sequence_screen_input(BT(B_OCTUP),0);
        fm1_ms=802;sequence_screen_input(0,0);
        assert(!sequence_confirm&&t->step[0].n==4&&t->step[0].note[0]==48);
        assert(undo_swap(0)&&same_pattern(&before,t));fm1_in.buttons=0;sequence_screen_close();sequence_add=0;
        /* Independent set oracle: randomized attacks/rests/ties, circular starts,
         * duplicate pitches and overflow over the entire pattern. */
        for(uint32_t run=0;run<1000;run++) {
            uint8_t expected[16][128]={{0}},old_notes[128]={0},new_notes[128]={0};
            uint32_t seed=run+1,len=1+run%16,overflow=0;
            t->p[P_SLEN]=(int16_t)len;
            for(i=0;i<len;i++) {
                step_t *parts[]={&t->step[i],&overlay[i]};
                for(uint32_t p=0;p<2;p++) {
                    seed=seed*1664525u+1013904223u;
                    *parts[p]=(step_t){.time=(uint8_t)(seed%3)};
                    if(parts[p]->time==ST_NOTE) {
                        parts[p]->n=1+(uint8_t)((seed>>8)%4);
                        for(j=0;j<parts[p]->n;j++) parts[p]->note[j]=(uint8_t)(48+((seed>>(j*3))%12));
                    }
                }
            }
            /* Scan two cycles: seed tie chains at step zero from the preceding loop. */
            for(i=0;i<len*2;i++) {
                uint32_t at=i%len,count=0;const step_t *parts[]={&t->step[at],&overlay[at]};
                uint8_t *sets[]={old_notes,new_notes};
                for(uint32_t p=0;p<2;p++) if(parts[p]->time!=ST_TIE) {
                    memset(sets[p],0,128);
                    if(parts[p]->time==ST_NOTE) for(j=0;j<parts[p]->n;j++) sets[p][parts[p]->note[j]]=1;
                }
                if(i>=len) for(j=0;j<128;j++) {expected[at][j]=old_notes[j]|new_notes[j];count+=expected[at][j];}
                if(count>4) overflow=1;
            }
            before=*t;int result=sequence_starter_add(t,overlay);
            assert((result<0)==overflow&&same_pattern(&before,t));
            if(result>0) {
                memset(new_notes,0,128);
                for(i=0;i<len;i++) {
                    if(overlay[i].time!=ST_TIE) {
                        memset(new_notes,0,128);
                        if(overlay[i].time==ST_NOTE) for(j=0;j<overlay[i].n;j++) new_notes[overlay[i].note[j]]=1;
                    }
                    assert(!memcmp(new_notes,expected[i],128));
                }
            }
        }
    }
    puts("sequence UI: additive ties/slide flags, duplicates, atomic overflow, metadata, undo/redo, both Apply gestures and 1000 independent union cases PASS");
    puts("sequence UI: native entry, 700ms hold, full undo/redo, track isolation, non-destructive preview, STOP/home ownership PASS");
    /* User recipe: actual knobs, variable length, preview/Apply, undo and guards. */
    song.sel=0;song.playing=song.rec=transport_req=rec_wait=ft_on=0;
    fm1_in.notes=fm1_in.buttons=0;sequence_add=0;sequence_rhythm=0;sequence_mode=STARTER_CHORD;
    sequence_shape=(rhythm_shape_t){0,0,0,255,0};sequence_root=0;sequence_scale=1;
    sequence_octave=0;sequence_vlead=0;sequence_sel=0;
    sequence_screen_open();sequence_page=3;
    const uint8_t recipe[]={4,0,1,3};
    for(uint32_t at=0;at<4;at++) {
        encs[panel.enc[EN_K2]]=(int32_t)at-sequence_custom_slot;sequence_screen_input(0,0);
        encs[panel.enc[EN_K3]]=(int32_t)recipe[at]-sequence_custom_degree[at];
        if(!encs[panel.enc[EN_K3]]) encs[panel.enc[EN_K1]]=1; /* ensure CUSTOM selection */
        sequence_screen_input(0,0);
    }
    encs[panel.enc[EN_K1]]=4-sequence_custom_count;sequence_screen_input(0,0);
    assert(sequence_sel==NSEQUENCE_STARTERS && !memcmp(recipe,sequence_custom_degree,4));
    ui.msg_t=0;sequence_screen_draw();ppm("live-custom-progression");
    for(uint32_t count=1;count<=8;count++) {
        encs[panel.enc[EN_K1]]=(int32_t)count-sequence_custom_count;sequence_screen_input(0,0);
        encs[panel.enc[EN_K4]]=100;sequence_screen_input(0,0);
        uint32_t len=sequence_length(sequence_sel),span=sequence_span(sequence_sel);
        assert(len==count*span && len<=NSTEP && sequence_custom_slot<count);
        before=trk[0];assert(sequence_starter_apply());
        assert(trk[0].p[P_SLEN]==(int32_t)len);
        for(uint32_t at=0;at<count;at++) {
            assert(trk[0].step[at*span].n==3);
            assert(trk[0].step[at*span].note[0]==sequence_degree_note(sequence_custom_degree[at],0,1,0));
        }
        after=trk[0];sequence_screen_input(BT(B_OCTDN),0);assert(sequence_preview.active);
        for(uint32_t at=0;at<=len;at++) {
            sequence_preview.first=at==0;sequence_preview.phase=div_units(2);
            sequence_preview_block(0);assert(sequence_preview.step==at%len);
            assert(sequence_preview.n==3);
            assert(!memcmp(sequence_preview.notes,after.step[(at%len)/span*span].note,3));
        }
        uint32_t phase=sequence_preview.phase;encs[panel.enc[EN_K2]]=1;sequence_screen_input(0,0);
        assert(sequence_preview.active && sequence_preview.phase==phase);
        encs[panel.enc[EN_K3]]=1;sequence_screen_input(0,0);
        assert(!sequence_preview.active);
        assert(undo_swap(0)&&same_pattern(&before,&trk[0]));
    }
    sequence_screen_close();
    puts("custom progressions: 5-1-2-4 knobs, 1..8 chords, span bounds, preview wrap/parity, undo PASS");
    /* The groove visualizer follows the same odd-step swing at 0,50,100. */
    for(uint32_t k=0;k<NTRK;k++) {steps_clear(&trk[k]);trk[k].p[P_SLEN]=16;trk[k].p[P_SDIV]=2;trk[k].p[P_SSWING]=0;}
    clk_beat=clk_pos=0;trk[0].step[1]=(step_t){.note={60},.n=1,.vel=100};
    for(uint32_t sw=0;sw<=100;sw+=50) {
        song.g[G_SWING]=sw;cv_begin(240,124,C_BLACK);cv_oy=0;vis_groove();
        uint32_t x=42+sw*28/200;
        assert(cv_px[65*240+x]==swap16(mix565(TE_DIM[0],TE_COL[0],200)));
    }
    song.g[G_SWING]=0;
    puts("swing visualizer: exact odd-step displacement at 0/50/100 PASS");
    return 0;
}

/* SPDX-License-Identifier: GPL-3.0-only */
/* A small native browser; caller owns the entry gesture. */
static uint8_t sequence_browser, sequence_page, sequence_confirm, sequence_initialized;
static uint32_t sequence_hold_t0;
static const char *const STARTER_MODE_NAMES[] = {"CHORD", "BASS", "ARP NOTES"};
static void sequence_preview_cancel(void)
{ fm1_irq_off(); sequence_preview_stop(); fm1_irq_on(); }

static void sequence_screen_close(void)
{
    sequence_preview_cancel();
    sequence_preview_tick = 0; sequence_preview_end = 0;
    sequence_browser = sequence_confirm = 0;
    sequence_hold_t0 = 0; ui.force = 1;
}
static void sequence_screen_open(void)
{
    if (song.sel >= NPART) return;
    /* Seed once from the first track, then keep the workstation's choices when
     * building matching chord, bass and arp parts on other tracks. */
    if (!sequence_initialized) {
        sequence_root = (uint8_t)clamp(trk[song.sel].p[P_ROOT],0,11);
        sequence_scale = (uint8_t)clamp(trk[song.sel].p[P_SCALE],0,NSCALES-1);
        if (!sequence_scale) sequence_scale = 1; /* useful default progression */
        sequence_octave = (int8_t)clamp(song.octave,-3,3);
        sequence_vlead = (uint8_t)!!trk[song.sel].p[P_VLEAD];
        sequence_initialized = 1;
    }
    sequence_browser = 1; sequence_page = sequence_confirm = 0;
    sequence_preview_tick = sequence_preview_block; sequence_preview_end = sequence_preview_stop;
    sequence_hold_t0 = 0; ui.force = 1;
}
/* Keep these cold screens out of the main UI handlers (measured target size). */
static void __attribute__((noinline)) sequence_screen_draw(void)
{
    static uint32_t head;
    const sequence_starter_t *p = &SEQUENCE_STARTERS[sequence_sel];
    char b[16]; uint32_t k;
    te_header(sequence_page == 2 ? "seq pitch" : sequence_page ? "seq rhythm" : "sequences",TE_COL[song.sel % NPART],&head);
    cv_begin(240,120,C_BLACK);
    cv_text(4,4,&FONT_S,p->name,C_WHITE);
    cv_text(4,24,&FONT_S,"RHY",TE_G4);
    cv_text(32,24,&FONT_S,SEQUENCE_RHYTHMS[sequence_rhythm].name,C_WHITE);
    cv_text(164,24,&FONT_S,"PRESET",TE_G3);
    for (k=0;k<4;k++) {
        fmt_int(b,p->degree[k]+1);
        cv_text(20+56*(int32_t)k,48,&FONT_L,b,TE_COL[song.sel % NPART]);
    }
    cv_text(4,92,&FONT_S,N_NOTE[sequence_root],C_WHITE);
    cv_text(40,92,&FONT_S,N_SCALE[sequence_scale],C_WHITE);
    cv_text(112,92,&FONT_S,STARTER_MODE_NAMES[sequence_mode],C_WHITE);
    cv_blit(0,40);
    cv_begin(240,80,C_BLACK);
    cv_text(4,0,&FONT_S,sequence_confirm ? "OCT+ AGAIN / HOLD" : "OCT+ APPLY OCT- LISTEN",C_WHITE);
    cv_text(4,20,&FONT_S,sequence_page == 2 ? "1 OCTAVE  2 ROOT" : sequence_page ? "1 ROTATE  2 OFFSET" : "1 STARTER 2 ROOT",TE_G3);
    cv_text(4,40,&FONT_S,sequence_page == 2 ? "3 SCALE   4 VLEAD" : sequence_page == 1 ? "3 SYNCOPATE 4 FEEL" : "3 SCALE   4 MODE",TE_G3);
    if (sequence_page == 1) {
        fmt_int(b,sequence_shape.rotate); cv_text(4,60,&FONT_S,b,TE_G4);
        fmt_int(b,sequence_shape.offset); cv_text(52,60,&FONT_S,b,TE_G4);
        fmt_int(b,sequence_shape.sync); cv_text(100,60,&FONT_S,b,TE_G4);
        fmt_int(b,sequence_shape.feel); cv_text(148,60,&FONT_S,b,TE_G4);
    } else {
        fmt_int(b,sequence_octave + 4); cv_text(4,60,&FONT_S,"OCT",TE_G4);
        cv_text(36,60,&FONT_S,b,C_WHITE);
        cv_text(60,60,&FONT_S,sequence_page == 2 ?
            (sequence_vlead?"VLEAD ON":"VLEAD OFF") : "TURN SELECT",TE_G3);
    }
    if (sequence_preview.active) cv_text(188,60,&FONT_S,"PLAY",C_WHITE);
    if (ui.msg_t) { cv_rect(0,0,240,20,C_WHITE); cv_text(4,2,&FONT_S,ui.msg,C_BLACK); }
    cv_blit(0,160);
}
static void __attribute__((noinline)) sequence_screen_input(uint32_t pressed,uint32_t home)
{
    uint32_t k; int32_t s; int changed=0;
    if (song.sel >= NPART || home == 1u) { sequence_screen_close(); return; }
    if ((s=panel_enc(EN_SELECT))) {
        int32_t next=(int32_t)sequence_page+s;
        if(next<0) { page_walk(-1); return; }
        if(next>2) { page_walk(1); return; }
        /* Browsing controls must not restart or release the running audition. */
        sequence_page=(uint8_t)next;
        sequence_confirm=0; sequence_hold_t0=0; ui.force=1;
    }
    for(k=0;k<4;k++) if ((s=panel_enc(EN_K1+k))) {
        if (sequence_page == 2 && k == 0u) {
            sequence_octave=(int8_t)clamp(sequence_octave+s,-3,3);
        } else if (sequence_page == 2 && k == 3u) {
            sequence_vlead=(uint8_t)clamp(sequence_vlead+s,0,1);
        } else if (sequence_page != 1) {
            uint8_t *v = k==0 ? &sequence_sel : k==1 ? &sequence_root : k==2 ? &sequence_scale : &sequence_mode;
            int max = k==0 ? NSEQUENCE_STARTERS-1 : k==1 ? 11 : k==2 ? NSCALES-1 : 2;
            *v=(uint8_t)clamp((int32_t)*v+s,0,max);
        } else if(k==0) sequence_shape.rotate=(int8_t)clamp(sequence_shape.rotate+s,-16,16);
        else if(k==1) sequence_shape.offset=(int8_t)clamp(sequence_shape.offset+s,-8,8);
        else if(k==2) sequence_shape.sync=(uint8_t)clamp(sequence_shape.sync+s,0,2);
        else sequence_shape.feel=(int8_t)clamp(sequence_shape.feel+s,MICRO_MIN,MICRO_MAX);
        changed=1;
    }
    if ((s=panel_enc(EN_PRESET))) {
        sequence_rhythm=(uint8_t)clamp((int32_t)sequence_rhythm+s,0,NSEQUENCE_RHYTHMS-1);
        changed=1;
    }
    if(changed) {
        sequence_preview_cancel(); sequence_confirm=0; sequence_hold_t0=0; ui.force=1;
    }
    panel_enc(EN_ALGO);
    if (pressed & (1u << panel.btn[B_OCTDN])) {
        if(sequence_preview.active) sequence_preview_cancel();
        else if(trk[song.sel].nheld || chord_latch_n[song.sel] || arp_chord_latch_n[song.sel])
            ui_message("RELEASE FIRST");
        else if(song.playing || transport_req || song.rec || rec_wait || ft_on || fm1_in.notes)
            ui_message("STOP FIRST");
        else {
            fm1_irq_off();
            sequence_preview.track=song.sel; sequence_preview.starter=sequence_sel;
            sequence_preview.root=sequence_root; sequence_preview.scale=sequence_scale;
            sequence_preview.mode=sequence_mode; sequence_preview.octave=sequence_octave;
            sequence_preview.vlead=sequence_vlead;
            sequence_preview.rhythm=sequence_rhythm;
            sequence_preview.shape=sequence_shape; sequence_preview.step=0;
            sequence_preview.phase=0; sequence_preview.first=1; sequence_preview.active=1;
            fm1_irq_on();
        }
        sequence_confirm=0; sequence_hold_t0=0; ui.force=1;
    }
    if (pressed & (1u << panel.btn[B_OCTUP])) {
        sequence_preview_cancel();
        if(song.playing || transport_req || song.rec || rec_wait || ft_on) ui_message("STOP FIRST");
        else if(!sequence_confirm && sequence_starter_has_content(&trk[song.sel])) {
            sequence_confirm=1; sequence_hold_t0=fm1_ms|1u; ui.force=1;
        } else if(sequence_starter_apply()) {
            sequence_confirm=0; sequence_hold_t0=0; ui_message("SEQUENCE APPLIED");
        }
    }
    if (!(fm1_in.buttons & (1u << panel.btn[B_OCTUP]))) sequence_hold_t0=0;
    if(sequence_hold_t0 && fm1_ms-(sequence_hold_t0&~1u)>=700u) {
        sequence_hold_t0=0;
        if(sequence_starter_apply()) { sequence_confirm=0; ui_message("SEQUENCE APPLIED"); }
        else ui_message("STOP FIRST");
    }
    if(pressed & (1u << panel.btn[B_PLAY])) sequence_screen_close();
}

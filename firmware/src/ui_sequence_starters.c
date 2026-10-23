/* SPDX-License-Identifier: GPL-3.0-only */
/* A small native browser; caller owns the entry gesture. */
static uint8_t sequence_browser, sequence_page, sequence_confirm, sequence_initialized;
static uint8_t sequence_add, sequence_custom_slot;
static uint32_t sequence_hold_t0;
/* Read the locked browser draft; literal pitches never pass through kb_map twice. */
static uint32_t progression_chord(uint32_t key,uint8_t *out)
{
    int32_t pad=punch_key(key);
    if(pad<0)return 0;
    uint32_t slot=(uint32_t)pad;
    uint32_t count=sequence_sel==NSEQUENCE_STARTERS?sequence_custom_count:4u;
    if(slot>=2u*count)return 0;
    int32_t octave=sequence_octave+(int32_t)(slot/count);
    uint32_t degree=sequence_degree(sequence_sel,slot%count);
    uint32_t n=sequence_sel!=NSEQUENCE_STARTERS && (SEQUENCE_STARTERS[sequence_sel].degrees&4096u)?4u:3u;
    uint32_t actual=0;
    for(uint32_t i=0;i<n;i++) {
        uint8_t note=(uint8_t)sequence_degree_note(degree+2u*i,sequence_root,sequence_scale,0);
        if(!actual || note>out[actual-1])out[actual++]=note;
    }
    track_t *t=&trk[progression.part];
    /* History stays at the reference register: octave pads and manual octave changes
     * cannot be cancelled by nearest-voice search or compound explicit inversions. */
    if(sequence_vlead && vl_n[progression.part])
        harmony_voice_lead(out,actual,vl_prev[progression.part],vl_n[progression.part],0);
    memcpy(vl_prev[progression.part],out,actual);vl_n[progression.part]=(uint8_t)actual;
    uint32_t kept=0;
    for(uint32_t i=0;i<actual;i++) {
        uint8_t note=(uint8_t)clamp((int32_t)out[i]+12*octave,0,127);
        if(!kept || note>out[kept-1u]) out[kept++]=note;
    }
    actual=kept;
    harmony_shape(out,actual,t->chord_inv,t->chord_spread);
    return actual;
}

static const char *const STARTER_MODE_NAMES[] = {"CHORD", "BASS", "ARP NOTES"};
UI_SMALL static void sequence_preview_cancel(void)
{ fm1_irq_off(); sequence_preview_stop(); fm1_irq_on(); }

UI_SMALL static void sequence_screen_close(void)
{
    sequence_preview_cancel();
    sequence_preview_tick = 0; sequence_preview_end = 0;
    sequence_browser = sequence_confirm = 0;
    sequence_hold_t0 = 0; ui.force = 1;
}
UI_SMALL static void sequence_screen_open(void)
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
    sequence_browser = 1; sequence_page = progression.active?4:0;sequence_confirm = 0;
    sequence_preview_tick = sequence_preview_block; sequence_preview_end = sequence_preview_stop;
    sequence_hold_t0 = 0; ui.force = 1;
}
/* Keep these cold screens out of the main UI handlers (measured target size). */
UI_SMALL static void sequence_screen_draw(void)
{
    static uint32_t cards;
    const char *lab[4]={"START","ROOT","SCALE","MODE"};
    char v[4][12];const char *val[4]={v[0],v[1],v[2],v[3]};
    int32_t ratio[4]={-1,-1,-1,-1};
    char b[32];uint32_t k;
    if(ui.force) draw_frame();
    draw_head();
    fmt_int(v[0],sequence_sel+1);str_cpy(v[1],N_NOTE[sequence_root],12);
    str_cpy(v[2],N_SCALE[sequence_scale],12);str_cpy(v[3],sequence_mode==STARTER_ARP?"ARP":STARTER_MODE_NAMES[sequence_mode],12);
    if(sequence_page==1) {
        lab[0]="ROTATE";lab[1]="OFFSET";lab[2]="SYNCOP";lab[3]="FEEL";
        fmt_int(v[0],sequence_shape.rotate);fmt_int(v[1],sequence_shape.offset);
        fmt_int(v[2],sequence_shape.sync);fmt_int(v[3],sequence_shape.feel);
    } else if(sequence_page==2) {
        lab[0]="OCTAVE";lab[3]="VLEAD";
        fmt_int(v[0],sequence_octave+4);str_cpy(v[3],sequence_vlead?"ON":"OFF",12);
    } else if(sequence_page==3) {
        lab[0]="COUNT";lab[1]="SLOT";lab[2]="DEGREE";lab[3]="SPAN";
        fmt_int(v[0],sequence_custom_count);fmt_int(v[1],sequence_custom_slot+1);
        fmt_int(v[2],sequence_custom_degree[sequence_custom_slot]+1);fmt_int(v[3],sequence_custom_span);
    } else if(sequence_page==4) {
        lab[0]="LIVE";lab[1]="OCTAVE";lab[2]="HOLD";lab[3]="SLOT";
        str_cpy(v[0],progression.active?"ON":"OFF",12);fmt_int(v[1],sequence_octave+4);
        str_cpy(v[2],TSEL->p[P_AHOLD]?"ON":"OFF",12);
        str_cpy(v[3],"-",12);
        if(progression.active && progression.key<27u) {
            uint32_t slot=(uint32_t)punch_key(progression.key);
            fmt_int(v[3],slot%(sequence_sel==NSEQUENCE_STARTERS?sequence_custom_count:4u)+1u);
        }
    }

    te_dials(lab,val,ratio,sequence_page+song.sel*13u,&cards);
    cv_begin(240,124,C_BLACK);
    cv_text(4,0,&FONT_S,sequence_sel==NSEQUENCE_STARTERS?"CUSTOM":SEQUENCE_STARTERS[sequence_sel].name,C_WHITE);
    for(k=0;k<(sequence_sel==NSEQUENCE_STARTERS?sequence_custom_count:4u);k++) {
        fmt_int(b,sequence_degree(sequence_sel,k)+1);
        int32_t x=8+28*(int32_t)k;
        cv_rect(x,26,24,28,TE_G1);cv_text(x+4,23,&FONT_L,b,TE_COL[song.sel]);
        if((sequence_page==3&&k==sequence_custom_slot) || (sequence_page==4 && progression.active && k+1u==(uint32_t)(v[3][0]-'0'))) cv_rect(x,55,24,2,C_WHITE);
    }
    cv_text(4,62,&FONT_S,sequence_page==4?(TSEL->p[P_AMODE]?"RHYTHM: ARP / NOW":"OUTPUT: CHORD / NOW"):sequence_add?"ALGO: MERGE":"ALGO: REPLACE",TE_G4);
    cv_text(4,82,&FONT_S,sequence_page==4?N_NOTE[sequence_root]:"RHY:",TE_G4);
    cv_text(40,82,&FONT_S,sequence_page==4?N_SCALE[sequence_scale]:SEQUENCE_RHYTHMS[sequence_rhythm].name,C_WHITE);
    cv_text(4,102,&FONT_S,sequence_page==4?(song.rec?"REC: PLAYED NOTES":"WHITE KEYS: CHORD SLOTS"):sequence_preview.active?"PREVIEW PLAYING":"PRESETS: RHYTHM",TE_COL[song.sel]);
    cv_blit(0,74);
    cv_begin(240,38,C_BLACK);
    str_cpy(b,sequence_page==4?"PERFORM ":sequence_page==3?"CUSTOM ":"SEQUENCES ",sizeof b);
    fmt_int(b+str_len(b),sequence_page+1);str_cpy(b+str_len(b),"/5",4);
    cv_text(4,0,&FONT_S,b,C_WHITE);
    cv_text(4,20,&FONT_S,sequence_page==4?"WHITE: SLOTS  PLAY / REC":sequence_confirm?"OCT+ AGAIN / HOLD":sequence_preview.active?"OCT- STOP / OCT+ APPLY":"OCT- LISTEN / OCT+ APPLY",TE_G4);
    cv_blit(0,202);
}
UI_SMALL static void sequence_screen_input(uint32_t pressed,uint32_t home)
{
    uint32_t k; int32_t s; int changed=0, apply=0;
    if (song.sel >= NPART || home == 1u) { sequence_screen_close(); return; }
    if ((s=panel_enc(EN_SELECT))) {
        int32_t next=(int32_t)sequence_page+s;
        if(next<0) { page_walk(-1); return; }
        if(next>4) { page_walk(1); return; }
        /* Browsing controls must not restart or release the running audition. */
        sequence_page=(uint8_t)next;
        sequence_confirm=0; sequence_hold_t0=0; ui.force=1;
    }
    for(k=0;k<4;k++) if ((s=panel_enc(EN_K1+k))) {
        if(sequence_page==4) {
            if(k==0) {
                fm1_irq_off();
                progression_build=progression_chord;
                int ok=1;
                if(s<0) progression_stop();
                else if(!progression.active) ok=!(arrangement_enabled || chain_n) && progression_arm();
                fm1_irq_on();
                if(!ok) ui_message((arrangement_enabled || chain_n)?"LOOP MODE":"FINISH TAKE");
            } else if(k==1) sequence_octave=(int8_t)clamp(sequence_octave+s,-3,3);
            else if(k==2) TSEL->p[P_AHOLD]=(int16_t)clamp(TSEL->p[P_AHOLD]+s,0,1);
            ui.force=1;continue;
        }
        if (sequence_page == 3) {
            if(k==1) {
                sequence_custom_slot=(uint8_t)clamp(sequence_custom_slot+s,0,sequence_custom_count-1);
                ui.force=1;continue; /* browsing a position does not change the audition */
            }
            sequence_preview_cancel();sequence_sel=NSEQUENCE_STARTERS;
            uint8_t *v=k==0?&sequence_custom_count:k==2?&sequence_custom_degree[sequence_custom_slot]:&sequence_custom_span;
            *v=(uint8_t)clamp(*v+s,k==0?1:0,k==0?8:k==2?6:sequence_custom_count>4?1:2);
            if(sequence_custom_count>4 && sequence_custom_span>1) sequence_custom_span=1;
            if(sequence_custom_slot>=sequence_custom_count) sequence_custom_slot=sequence_custom_count-1;
        } else if (sequence_page == 2 && k == 0u) {
            sequence_octave=(int8_t)clamp(sequence_octave+s,-3,3);
        } else if (sequence_page == 2 && k == 3u) {
            sequence_vlead=(uint8_t)clamp(sequence_vlead+s,0,1);
        } else if (sequence_page != 1) {
            uint8_t *v = k==0 ? &sequence_sel : k==1 ? &sequence_root : k==2 ? &sequence_scale : &sequence_mode;
            int max = k==0 ? NSEQUENCE_STARTERS : k==1 ? 11 : k==2 ? NSCALES-1 : 2;
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
    if ((s=panel_enc(EN_ALGO))) {
        sequence_add=(uint8_t)clamp(sequence_add+s,0,1);
        sequence_confirm=0; sequence_hold_t0=0; ui.force=1;
    }
    if(progression.active || sequence_page==4) {
        if(pressed & (1u<<panel.btn[B_PLAY])) transport_req=song.playing?2:1;
        return;
    }
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
        } else apply=1;
    }
    if (!(fm1_in.buttons & (1u << panel.btn[B_OCTUP]))) sequence_hold_t0=0;
    if(sequence_hold_t0 && fm1_ms-(sequence_hold_t0&~1u)>=700u) {
        apply=1;
    }
    if (apply) {
        int result=sequence_starter_apply_mode(sequence_add);
        sequence_confirm=0;sequence_hold_t0=0;
        ui_message(result<0 ? "CHORD FULL" : result ? "SEQUENCE APPLIED" : "STOP FIRST");
    }
    if(pressed & (1u << panel.btn[B_PLAY])) sequence_screen_close();
}

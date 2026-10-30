/* SPDX-License-Identifier: GPL-3.0-only */
/* Compact 240 x 240 song-order editor. All changes and flash saves require
 * stopped transport. The four existing project slots are sections A..D. */
static uint8_t song_cursor, song_store_armed, song_load_armed;
static uint32_t song_store_deadline;
UI_SMALL static int on_song_page(void) { return !ui.home && cur_page()->scope == SC_SONG; }
UI_SMALL static void song_sane(void)                         /* a bad chain (blank / damaged settings): defaults */
{
    uint32_t i, ok = arrangement.count >= 1u && arrangement.count <= ARR_STEPS;
    for (i = 0; ok && i < arrangement.count; i++)
        ok = arrangement.entry[i].scene < ARR_SCENES && arrangement.entry[i].bars >= 1u && arrangement.entry[i].bars <= 64u;
    if (!ok)
        arr_defaults(&arrangement);
    if (song_cursor >= arrangement.count)
        song_cursor = (uint8_t)(arrangement.count - 1u);
}

/* SONG: the chain of sections (A..D in the track colours), bars of each, where the song is */
UI_SMALL static void song_screen_draw(void)
{
    song_sane();
    uint32_t i,start=song_cursor>3u?song_cursor-3u:0u;
    static uint32_t previous;
    uint32_t sig=song_cursor+17u*arrangement_enabled+37u*song.playing+
        71u*arrangement_clock.index+127u*arrangement_clock.bar+257u*arrangement.count;
    char b[24];
    draw_head();
    for(i=0;i<arrangement.count;i++)
        sig=sig*31u+arrangement.entry[i].scene+7u*arrangement.entry[i].bars;
    for(i=0;i<ARR_SCENES;i++) sig=sig*3u+(uint32_t)project_used(i)+TE_COL[i];
    if(!ui.force&&sig==previous) return;
    previous=sig;
    if(ui.force) draw_frame();
    cv_band(44);
    const arr_entry_t *entry=&arrangement.entry[song_cursor];
    for(i=0;i<4;i++) {
        static const char *const label[]={"ENTRY","SECT","BARS","LENGTH"};
        uint32_t v=i==0?song_cursor+1u:i==1?entry->scene:i==2?entry->bars:arrangement.count;
        int32_t ratio=i==0?(int32_t)v*1000/arrangement.count:i==1?(int32_t)v*1000/3:
            i==2?(int32_t)v*1000/64:(int32_t)v*1000/ARR_STEPS;
        if(i==1) {b[0]=(char)('A'+v);b[1]=0;} else fmt_int(b,(int32_t)v);
        ui_card(2+(int32_t)i*60,0,label[i],b,ratio,TE_COL[i],0);
    }
    cv_band_blit(26);
    cv_band(124);
    cv_text_c_white(4,0,arrangement_enabled?"SONG ORDER":"LOOP / SONG ORDER");
    for(i=0;i<4;i++) {
        uint32_t pos=start+i;
        if(pos>=arrangement.count) continue;
        const arr_entry_t *e=&arrangement.entry[pos];
        uint32_t selected=pos==song_cursor,used=project_used(e->scene);
        uint16_t sc=TE_COL[e->scene&3u];
        int32_t y=22+(int32_t)i*24,w=e->bars*110/64+4;
        cv_rect(2,y,236,23,selected?TE_G1:C_BLACK);
        if(selected) cv_rect_c_white(2,y,2,23);
        fmt_int(b,(int32_t)pos+1);cv_text_s(8,y+3,b,selected?C_WHITE:TE_G3);
        cv_rect(30,y+2,22,19,used?sc:TE_G2);
        b[0]=(char)('A'+e->scene);b[1]=0;
        cv_text_s(37,y+3,b,used?C_BLACK:sc);
        cv_rect(60,y+9,w,6,used?TE_DIM[e->scene&3u]:TE_G2);
        fmt_int(b,e->bars);str_cpy(b+str_len(b),e->bars==1?" BAR":" BARS",7);
        cv_text_s(180,y+3,used?b:"EMPTY",selected?C_WHITE:TE_G3);
        if(arrangement_clock.running&&arrangement_clock.index==pos)
            cv_rect(60,y+17,(int32_t)(arrangement_clock.bar+1u)*w/(e->bars?e->bars:1),2,sc);
    }
    cv_band_blit(74);
    cv_band(38);
    cv_text_te_g4(4,0,"REC STORE / SAVE CHAIN");
    cv_text_te_g3(4,20,"OCT- MODE / OCT+ X2 LOAD");
    cv_band_blit(202);
}

UI_SMALL static void song_screen_input(uint32_t pressed, uint32_t home)
{
    uint32_t k, b;
    int32_t steps;
    song_sane();
    if (home == 1u) { go_home(); return; }       /* BT_TAP */
    for (k = 0; k < NB; k++) {
        if (!((pressed >> panel.btn[k]) & 1u)) continue;
        b = k;
        if ((b == B_PLAY || b == B_REC) && ft_owns_press())
            continue;                            /* (they close a free take: seq.c) */
        if (b == B_PLAY) {
            if (!song.playing && arrangement_enabled && !arr_valid(&arrangement, arrangement_ready())) {
                ui_message("EMPTY SECTION: REC");
            } else transport_req = song.playing ? 2 : 1;
        } else if (b == B_SEQ) {
            open_family(FAM_SEQ);
        } else if (b == B_SAVE || b == B_REC || b == B_OCTDN || b == B_OCTUP) {
            uint32_t scene = arrangement.entry[song_cursor].scene;
            if (song.playing || transport_req) { ui_message("STOP FIRST"); continue; }
            if (b == B_SAVE) {
                arrangement_save();
                song_store_armed = 0;
            } else if (b == B_OCTDN) {
                arrangement_enabled ^= 1u;
                ui_message(arrangement_enabled ? "SONG MODE" : "LOOP MODE");
            } else if (b == B_OCTUP) {                   /* load: a second press within 3 s (unsaved work goes) */
                if (song_load_armed && (int32_t)(fm1_ms - song_store_deadline) <= 0) {
                    song_load_armed = 0;
                    project_load(scene);
                } else {
                    song_load_armed = 1;
                    song_store_armed = 0;
                    song_store_deadline = fm1_ms + 3000u;
                    ui_message("OCT+ AGAIN: LOAD");
                }
            } else if (project_used(scene) &&
                       (!song_store_armed || (int32_t)(fm1_ms - song_store_deadline) > 0)) {
                song_store_armed = 1;
                song_store_deadline = fm1_ms + 3000u;
                ui_message("REC AGAIN: REPLACE");
            } else {
                project_save(scene);
                settings_save();
                song_store_armed = 0;
            }
        }
    }
    for (k = 0; k < 4u; k++) {
        steps = panel_enc(EN_K1 + k);
        if (!steps) continue;
        song_store_armed = 0;
        song_load_armed = 0;
        if (k == 0) {
            song_cursor = (uint8_t)clamp(song_cursor + steps, 0, arrangement.count - 1);
            continue;
        }
        if (song.playing || transport_req) { ui_message("STOP FIRST"); continue; }
        if (k == 1) arrangement.entry[song_cursor].scene = (uint8_t)clamp(arrangement.entry[song_cursor].scene + steps, 0, 3);
        if (k == 2) arrangement.entry[song_cursor].bars = (uint8_t)clamp(arrangement.entry[song_cursor].bars + steps, 1, 64);
        if (k == 3) {
            arrangement.count = (uint8_t)clamp(arrangement.count + steps, 1, ARR_STEPS);
            if (song_cursor >= arrangement.count) song_cursor = arrangement.count - 1;
        }
    }
    /* Unused encoders have no hidden action on this screen. */
    panel_enc(EN_SELECT); panel_enc(EN_ALGO); panel_enc(EN_PRESET);
}

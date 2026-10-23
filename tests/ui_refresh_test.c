/* SPDX-License-Identifier: GPL-3.0-only */
/* Real renderer: dirty-region correctness, routing/state preservation, and captures. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"

static uint16_t incremental[240*240];
static void regular(void)
{
    if(ui.force) draw_frame();
    draw_head();draw_columns();draw_graph();draw_foot();ui.force=0;
}
static void same_as_full(void (*draw)(void),const char *context)
{
    memcpy(incremental,screen,sizeof screen);
    ui.force=1;draw();ui.force=0;
    if(memcmp(incremental,screen,sizeof screen)) {
        fprintf(stderr,"dirty redraw differs from full redraw: %s\n",context);
        abort();
    }
}
static void modulation(void) {layer_screen_draw();ui.force=0;}
static void tracks(void) {studio_tracks_draw();ui.force=0;}
static void song_layout(void) {song_screen_draw();ui.force=0;}
static void record_layout(void) {rec_screen_draw();ui.force=0;}
static void help_hints(void)
{
    assert(strstr(page_hold_hint(&PAGES[page_first(FAM_LFO)]),"VIBRATO"));
    assert(strstr(page_hold_hint(&PAGES[page_first(FAM_ENV)]),"TREMOLO"));
    for(uint32_t p=0;p<NPAGES;p++) assert(text_w(&FONT_S,page_hold_hint(&PAGES[p]))<=232);
}
/* Search complete glyph ink in a bounded area: clipping or an overlapping control
 * cannot pass simply because the first few letters were drawn. */
static int text_found(const char *s,uint16_t color,uint32_t top,uint32_t bottom,const felucca_font_t *font)
{
    uint16_t glyph[240*32];
    int w=text_w(font,s),h=font->h;if(w>232)return 0;assert(h<=32);
    cv_oy=0;cv_begin(w,h,C_BLACK);cv_text(0,0,font,s,color);
    memcpy(glyph,cv_px,w*h*sizeof *glyph);
    for(uint32_t y=top;y+h<=bottom;y++) for(int x=0;x+w<=240;x++) {
        int match=1;
        for(int j=0;j<h&&match;j++) for(int i=0;i<w;i++)
            if(glyph[j*w+i]!=swap16(C_BLACK)&&glyph[j*w+i]!=screen[(y+j)*240+x+i]) {match=0;break;}
        if(match) return 1;
    }
    return 0;
}
static void text_visible(const char *s,uint16_t color,uint32_t top,uint32_t bottom)
{
    if(text_found(s,color,top,bottom,&FONT_S)||text_found(s,color,top,bottom,&FONT_L))return;
    fprintf(stderr,"Incomplete text: %s\n",s);abort();
}
static void polish_checks(void)
{
    ui.msg_t=0;ui.menu=0;ui.force=1;draw_head();
    /* Tape icon and track badge have a real gutter, even with USB visible. */
    for(int y=0;y<H_HEAD;y++) for(int x=168;x<172;x++) assert(screen[y*240+x]==swap16(C_BLACK));
    for(uint32_t sec=0;sec<MS_COUNT;sec++) {
        ui.menu=1;ui.menu_sel=MS_FIRST[sec];ui.force=1;draw_menu();
        for(uint32_t t=0;t<MS_COUNT;t++) text_visible(MS_TAB[t],t==sec?C_WHITE:C_DIM,21,47);
    }
    ui.menu_sel=MI_COLOR;ui.force=1;draw_menu();ppm("polish-settings");
    /* The longest custom palette name and every swatch must survive together. */
    theme_t old=theme;uint8_t valid=theme_valid,on=theme_on;
    memcpy(theme.name,"ABCDEFGH",8);theme_valid=theme_on=1;
    ui.menu_sel=MI_COLOR;ui.force=1;draw_menu();
    text_visible("ABCDEFGH",C_WHITE,47,97);
    for(uint32_t k=0;k<5;k++) assert(screen[80*240+170+k*14]==swap16(pal[k]));
    ppm("polish-settings-long");theme=old;theme_valid=valid;theme_on=on;
    ui.menu=2;ui.force=1;draw_menu();
    text_visible(FELUCCA_VERSION " MERTHSOFT",C_GRAY,0,H_HEAD);
    text_visible("GITHUB.COM/MERTHSOFT/",C_AMB,21,240);
    text_visible("SLOOP-FM1",C_AMB,21,240);
    text_visible("VOICE: REF. KLATTSCH (MIT)",TE_G4,21,240);
    ppm("polish-about");ui.menu=0;
    /* Long summary values and units cannot collide with the neighbouring cell. */
    cv_begin(240,H_GRAPH,C_BLACK);cv_oy=0;
    for(uint32_t k=0;k<4;k++) draw_column(k,"VALUE","SMOOTH","",TE_COL[k],-1);
    cv_begin(240,H_GRAPH,C_BLACK);graph_big();cv_blit(0,Y_GRAPH);
    text_visible("SMOOTH",C_HI,Y_GRAPH,Y_GRAPH+H_GRAPH);
    for(uint32_t k=0;k<4;k++) draw_column(k,"VALUE","100.0","KHZ",TE_COL[k],-1);
    cv_begin(240,H_GRAPH,C_BLACK);graph_big();cv_blit(0,Y_GRAPH);
    text_visible("100.0",C_HI,Y_GRAPH,Y_GRAPH+H_GRAPH);
    text_visible("KHZ",C_GRAY,Y_GRAPH,Y_GRAPH+H_GRAPH);
    /* Modulation extrema have margin on both sides instead of clipping at +127. */
    for(uint32_t p=0;p<NPAGES;p++) if(PAGES[p].graph==GR_DEST) {
        int16_t saved[4];
        for(uint32_t k=0;k<4;k++) {saved[k]=TSEL->p[PAGES[p].id[k]];TSEL->p[PAGES[p].id[k]]=TP[PAGES[p].id[k]].max;}
        ui.page=p;ui.force=1;regular();
        for(int y=Y_GRAPH;y<Y_GRAPH+H_GRAPH;y++) for(int x=232;x<240;x++) assert(screen[y*240+x]==swap16(C_BLACK));
        for(uint32_t k=0;k<4;k++) TSEL->p[PAGES[p].id[k]]=saved[k];
    }
    punch_clear();punch.req=punch.cur=-1;ui.layer=LY_FX;ui.force=1;layer_screen_draw();
    text_visible("DOUBLE",TE_G3,74,224);text_visible("LOFI",TE_G3,74,224);
    ppm("polish-fx");ui.layer=LY_PLAY;
    for(uint32_t p=0;p<NPAGES;p++)if(PAGES[p].scope==SC_GLOBAL&&PAGES[p].id[0]==G_LTYPE)ui.page=p;
    ui.home=0;ui.hot_t=0;
    static const char *const lofi_controls[5][3]={{"WOW","DARK","DAMAGE"},{"WOW","DARK","DAMAGE"},{"FLUTTR","DARK","DRIVE"},{"WARBLE","DARK","DAMAGE"},{"RATE","LATCH","MIX"}};
    for(uint32_t mode=0;mode<5;mode++) {
        song.g[G_LTYPE]=mode;regular();same_as_full(regular,"LOFI mode and label change");
        for(uint32_t k=0;k<3;k++)text_visible(lofi_controls[mode][k],TE_G4,Y_LABEL,Y_GRAPH);
        char name[24];snprintf(name,sizeof name,"lofi-%u",mode);ppm(name);
    }
    song.g[G_LTYPE]=0;
    ui.force=1;tracks();ppm("polish-header");
}
int main(int argc,char **argv)
{
    outdir=argc>1?argv[1]:"build/neon-ui";
    panel=PANEL_DEFAULT;layers_init();host_tracks_init();
    settings.palette=NPALETTES-1u;palette_set(settings.palette);
    assert(!strcmp(PALETTES[settings.palette].name,"NEON"));
    song.sel=1;
    for(uint32_t i=0;i<NPART;i++) {
        set_engine_of(&trk[i],0);apply_preset_to(&trk[i],0);trk[i].engine=0;
        trk[i].p[P_SLEN]=64;
        for(uint32_t j=0;j<64;j+=4) {
            trk[i].step[j]=(step_t){0};trk[i].step[j].time=ST_NOTE;
            trk[i].step[j].n=1;trk[i].step[j].note[0]=48+i*7+j/4;
        }
    }
    TDRUM->p[P_SLEN]=64;
    /* All bits set used to look like an invalid synth note count to the hash. */
    memset(TDRUM->dstep,255,sizeof TDRUM->dstep);
    ui.home=0;ui.page=page_first(FAM_TRK);ui.force=1;tracks();
    ppm("neon-home");same_as_full(tracks,"HOME");
    /* Engine text is present in the selected track color, without name overlap. */
    char name[16];trk_short_name(song.sel,name);
    int32_t x=12+text_w(&FONT_S,name),colored=0;
    const char *eng=ENGINES[TSEL->eng_req%NENGINES]->name;
    for(int32_t y=202;y<218;y++) for(int32_t px=x;px<x+text_w(&FONT_S,eng);px++)
        colored+=screen[y*240+px]==swap16(TE_COL[song.sel]);
    assert(colored);
    for(uint32_t e=0;e<NENGINES;e++) {
        const engine_t *en=ENGINES[e];
        assert(12+12*text_w(&FONT_S,"W")+text_w(&FONT_S,en->name)<=240);
    }
    set_engine_of(TSEL,ENGI_FM6);apply_preset_to(TSEL,0);TSEL->engine=ENGI_FM6;
    tracks();same_as_full(tracks,"HOME engine changes");
    set_engine_of(TSEL,0);apply_preset_to(TSEL,0);TSEL->engine=0;
    tracks();same_as_full(tracks,"HOME engine returns");

    /* A stopped drum-kit change must update the actual sound label immediately,
     * including external edits which do not force a full-screen redraw. */
    uint32_t selected=song.sel;int16_t kit=TDRUM->p[P_E0];
    song.sel=TRK_DRUM;tracks();
    for(uint32_t k=0;k<DRUM_KITS;k++) {
        TDRUM->p[P_E0]=(int16_t)k;tracks();
        text_visible(DRUM_KIT_NAMES[k],C_WHITE,202,220);
        text_visible("DRUMS",TE_COL[TRK_DRUM],202,220);
        same_as_full(tracks,"HOME drum kit change");
    }
    ppm("neon-home-drums");
    TDRUM->p[P_E0]=kit;song.sel=selected;tracks();

    /* Pulse invalidates HOME even if no sequencer step advances (long notes).
     * Only the sun changes; track lanes and footer retain identical pixels. */
    song.playing=1;clk_pos=0;tracks();
    assert(home_sun_light()==16);ppm("neon-sun-bright");
    memcpy(incremental,screen,sizeof screen);
    clk_pos=BEAT_U*3u/4u;tracks();
    assert(home_sun_light()<16);
    assert(memcmp(incremental+74*240,screen+74*240,44*240*sizeof *screen));
    assert(!memcmp(incremental+118*240,screen+118*240,(240-118)*240*sizeof *screen));
    ppm("neon-sun-dim");same_as_full(tracks,"sun pulse with stationary notes");
    uint32_t light=16;
    for(uint32_t tick=0;tick<16;tick++) {
        clk_pos=tick*(BEAT_U/16u);assert(home_sun_light()<=light);
        light=home_sun_light();
    }
    clk_pos=0;tracks();assert(home_sun_light()==16);
    same_as_full(tracks,"sun beat resets");
    song.playing=0;clk_pos=BEAT_U/2u;tracks();assert(home_sun_light()==16);
    same_as_full(tracks,"sun stops");
    TDRUM->dstep[5].on[0]^=1;tracks();same_as_full(tracks,"drum edit invalidates HOME");
    trk[1].p[P_MUTE]=1;tracks();same_as_full(tracks,"mute invalidates HOME");
    trk[1].p[P_MUTE]=0;

    /* Every visible standard page in every existing palette still draws safely. */
    for(uint32_t pal=0;pal<NPALETTES;pal++) {
        palette_set(pal);
        for(uint32_t p=0;p<NPAGES;p++) {
            if(PAGES[p].scope==SC_SONG||PAGES[p].scope==SC_DRUM||PAGES[p].scope==SC_STARTER) continue;
            ui.page=p;ui.force=1;regular();regular();same_as_full(regular,"standard page");
        }
    }
    palette_set(NPALETTES-1u);
    /* Every model has a complete visible label; cached edits match a full draw. */
    for(uint32_t p=0;p<NPAGES;p++) if(!strcmp(PAGES[p].title,"REVERB"))ui.page=p;
    ui.hot_t=0;ui.force=1;
    for(int model=0;model<5;model++) {
        char capture[32];song.g[G_RMODEL]=model;regular();
        snprintf(capture,sizeof capture,"neon-reverb-%d",model);ppm(capture);
        text_visible(N_RMODEL[model],VAL(0),26,224);
        same_as_full(regular,"reverb type change");
    }
    song.g[G_RMODEL]=0;
    ui.page=page_first(FAM_ENV);TSEL->p[P_ATK]=35;TSEL->p[P_DEC]=64;
    TSEL->p[P_SUS]=85;TSEL->p[P_REL]=68;ui.hot_col=1;ui.hot_t=30;
    ui.force=1;regular();ppm("neon-envelope");
    ui.hot_t=0;regular();same_as_full(regular,"focus expires");
    ui.page=page_first(FAM_LFO);ui.force=1;regular();
    uint32_t sig=ui.graph_sig;TSEL->p[P_LRATE]=90;regular();
    assert(sig!=ui.graph_sig);same_as_full(regular,"LFO rate");
    TSEL->p[P_LSYNC]=6;regular();sig=ui.graph_sig;
    song.g[G_BPM]+=20;regular();assert(sig!=ui.graph_sig);
    same_as_full(regular,"synced LFO tempo");ppm("neon-lfo");

    for(uint32_t p=0;p<NPAGES;p++) if(PAGES[p].graph==GR_DELAY) {
        ui.page=p;ui.hot_t=0;ui.force=1;regular();
        same_as_full(regular,"delay caption redraw");ppm("neon-delay");
        /* The entire caption must be present somewhere within the graph strip.
         * Compare with independently rendered full glyphs, without assuming its y. */
        static uint16_t caption[240*32];assert(FONT_S.h<=32);
        cv_oy=0;cv_begin(240,FONT_S.h,C_BLACK);
        cv_text(8,0,&FONT_S,"DRY   TEMPO-SYNCED ECHOES",C_GRAY);
        memcpy(caption,cv_px,240*FONT_S.h*sizeof *caption);
        int found=0;
        for(uint32_t y=Y_GRAPH;y+FONT_S.h<=Y_GRAPH+H_GRAPH;y++)
            found|=!memcmp(caption,screen+y*240,240*FONT_S.h*sizeof *caption);
        assert(found);
    }
    help_hints();

    /* Rendering must not rewrite the track sound, sequence or held scale LEDs. */
    int16_t params[P_COUNT];step_t pattern[NSTEP];
    memcpy(params,TSEL->p,sizeof params);memcpy(pattern,TSEL->step,sizeof pattern);
    lights_scale=1;
    for(uint32_t layer=LY_VIB;layer<=LY_PAN;layer++) {
        ui.layer=layer;live_mod_t *m=&live_mod[layer-LY_VIB];
        m->part=song.sel;m->on=1;m->depth=85;m->rate=60;
        m->phase=0x40000000u;m->value=16384;m->wave=0;m->sync=0;
        if(layer==LY_WAH) {m->sync=WAH_ENV;m->wave=20;}
        fm1_ms+=50;ui.force=1;modulation();
        const char *name=layer==LY_VIB?"neon-vibrato":layer==LY_TREM?"neon-tremolo":layer==LY_WAH?"neon-wah":"neon-pan";
        ppm(name);same_as_full(modulation,name);
        ly_lock=layer;fm1_ms+=50;modulation();same_as_full(modulation,"lock badge");
        ly_lock=LY_PLAY;
        assert(!memcmp(params,TSEL->p,sizeof params));
        assert(!memcmp(pattern,TSEL->step,sizeof pattern)&&lights_scale==1);
    }
    ui.layer=LY_PLAY;ui.force=1;regular();same_as_full(regular,"return from popup");
    sequence_screen_open();ui.force=1;sequence_screen_draw();ppm("neon-sequences");
    sequence_page=1;sequence_screen_draw();same_as_full(sequence_screen_draw,"library subpage");
    sequence_screen_close();song.sel=TRK_DRUM;drum_page=3;
    ui.force=1;groove_screen_draw();ppm("neon-groove");
    drum_page=4;groove_screen_draw();same_as_full(groove_screen_draw,"groove rhythm subpage");
    song.sel=1;arr_defaults(&arrangement);song_cursor=0;
    for(uint32_t p=0;p<NPAGES;p++) if(PAGES[p].scope==SC_SONG) ui.page=p;
    for(uint32_t pal=0;pal<NPALETTES;pal++) {
        palette_set(pal);ui.force=1;song_layout();
        assert(screen[99*240+31]==swap16(TE_COL[0]));
        same_as_full(song_layout,"song palette and card layout");
    }
    palette_set(NPALETTES-1u);ui.force=1;song_layout();ppm("neon-song");
    arrangement.count=ARR_STEPS;song_cursor=ARR_STEPS-1;
    for(uint32_t k=0;k<ARR_STEPS;k++) arrangement.entry[k]=(arr_entry_t){k%4,64};
    song_layout();same_as_full(song_layout,"song last entry and longest bars");
    ppm("neon-song-long");
    song.playing=0;rec_wait=1;rec_tempo=1;rec_count=0;ci_on=0;ft_on=0;
    ui.force=1;record_layout();same_as_full(record_layout,"record ready");ppm("neon-record");
    rec_count=1;ci_on=1;ci_beat=0;
    record_layout();same_as_full(record_layout,"count-in transition");ppm("neon-count-in");
    ci_beat=3;record_layout();same_as_full(record_layout,"count-in last beat");
    ci_on=0;rec_wait=0;ft_on=1;ft_trk=1;ft_t=FS*3u/CTL;
    record_layout();same_as_full(record_layout,"free take transition");ppm("neon-free-take");
    fm1_ms+=250;record_layout();same_as_full(record_layout,"free take timer blink");
    ft_on=0;rec_wait=1;record_layout();same_as_full(record_layout,"free take back to ready");
    rec_wait=0;ci_on=0;rec_count=0;ft_t=0;arr_defaults(&arrangement);
    song.sel=1;ui.page=page_first(FAM_ENV);ui.menu=1;ui.menu_sel=MI_HELP;
    mi_set(MI_HELP,0);assert(ui.menu==3);draw_menu();ppm("neon-help");
    menu_input(1u<<panel.btn[B_OCTDN]);assert(ui.menu==1);
    polish_checks();
    puts("UI refresh: dirty/full parity, palettes, drum union, modulation state, record/song layout, help PASS");
    return 0;
}

/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Felucca UI drawing: status bar (top), columns + gauges, graphs, focus readout,
 * footer (steps + engine / preset / page). */
static void draw_menu(void);
static uint32_t str_hash(uint32_t h, const char *s);

/* --------------------------------------------------------- drawing --- */
UI_SMALL static int is_eng_name(const char *s)                 /* one of the ENGINES[]->name strings */
{
    uint32_t e;
    for (e = 0; e < NENGINES; e++)
        if (s == ENGINES[e]->name)
            return 1;
    return 0;
}

/* Fit a short card label/value without cutting a glyph. */
UI_SMALL static void fit(char *d, const char *src, const felucca_font_t *f, int32_t maxw)
{
    str_cpy(d, src, 8);           /* engine names are kept whole */
    while (d[0] && text_w(f, d) > maxw)
        d[str_len(d) - 1u] = 0;
}

UI_SMALL static int32_t batt_level(void)                         /* ADC ch3 thresholds */
{
    return song.batt_raw >= 591 ? 3 : song.batt_raw >= 561 ? 2 : song.batt_raw >= 531 ? 1 : 0;
}
/* charging (a USB host powers us; there is no charger status line): bars fill 1, 2, 3
 * every 0.6 s, so the header is redrawn twice a second at most; else the level */
UI_SMALL static int32_t batt_shown(void)
{
    if (usb.config && !usb.suspended)
        return 1 + (int32_t)((fm1_ms / 600u) % 3u);
    return batt_level();
}

/* top bar: transport, BPM, octave | USB, battery, CPU; messages replace it */
UI_SMALL static void draw_head(void)
{
    char b[16];
    uint32_t i;
    int32_t x;
    uint32_t rec = ((song.rec >> song.sel) & 1u) || rec_wait || ft_on ? 2u : song.rec != 0u;   /* 2 the selected track armed, 1 another */
    uint32_t sig = (uint32_t)song.playing * 3u + rec * 5u + (uint32_t)(song.octave + 8) * 11u + song.sel * 13131u +
                   (ui.msg_t ? str_hash(7u, ui.msg) : 0u) + (uint32_t)song.g[G_BPM] * 101u + (ui.bpm_t != 0) * 31u +
                   (uint32_t)batt_shown() * 7777u + (usb.config && !usb.suspended) * 99991u + mlu.on * 7919u;
    if (!ui.force && sig == ui.head_sig)
        return;
    ui.head_sig = sig;
    cv_begin(240, H_HEAD, C_BLACK);
    if (ui.msg_t) {
        cv_text(4, 1, &FONT_S, ui.msg, C_HI);
        cv_blit(0, Y_HEAD);
        return;
    }
    if (song.playing) {                               /* > play, square stop */
        for (i = 0; i < 5u; i++)
            cv_rect(4 + (int32_t)i * 2, 4 + (int32_t)i, 2, 10 - 2 * (int32_t)i, C_WHITE);
    } else {
        cv_rect(4, 5, 8, 8, C_HI);
    }
    if (rec)                                          /* recording armed: white = this track, gray = another */
        te_disc(21, 9, 4, rec == 2u ? TE_RED : C_GRAY);
    fmt_int(b, song.g[G_BPM]);
    x = 32;
    if (FELUCCA_ICONS) {                              /* metronome, then the BPM */
        cv_icon(x, 2, ICON_TEMPO, C_GRAY);
        x += 14;
    }
    x = cv_text(x, 1, &FONT_S, b, ui.bpm_t ? C_WHITE : C_HI);   /* white while SELECT turns it */
    if (mlu.on) {                                     /* MIDI LEARN on (2.6, GLO + F#3): in the octave's place */
        cv_text(x + 12, 1, &FONT_S, "LEARN", C_AMB);
    } else if (song.octave) {
        str_cpy(b, song.octave > 0 ? "+" : "", 4);
        fmt_int(b + str_len(b), song.octave);
        cv_text(x + 12, 1, &FONT_S, "OCT", C_GRAY);
        cv_text(x + 40, 1, &FONT_S, b, C_HI);
    }
    if (FELUCCA_ICONS) {                              /* the selected track: tape + number */
        cv_icon(156, 2, ICON_TAPE, C_GRAY);
        b[0] = (char)('1' + song.sel);
        b[1] = 0;
        cv_rect(172, 2, 12, 16, TE_COL[song.sel & 3u]);
        cv_text(174, 1, &FONT_S, b, C_BLACK);
    } else {
        b[0] = 'T';
        b[1] = (char)('1' + song.sel);
        b[2] = 0;
        cv_text(158, 1, &FONT_S, b, C_HI);
    }
    {   /* battery, 3 bars; USB when a host is there */
        int32_t lvl = batt_shown(), k;
        int32_t bx = 236 - 19;                          /* right edge (the CPU figure is in the console) */
        cv_rect(bx, 4, 17, 1, C_GRAY);
        cv_rect(bx, 12, 17, 1, C_GRAY);
        cv_rect(bx, 4, 1, 9, C_GRAY);
        cv_rect(bx + 16, 4, 1, 9, C_GRAY);
        cv_rect(bx + 17, 6, 2, 5, C_GRAY);
        for (k = 0; k < lvl; k++)
            cv_rect(bx + 2 + k * 5, 6, 3, 5, lvl == 1 && batt_level() <= 1 ? C_WHITE : C_HI);
        if (usb.config && !usb.suspended)
            cv_text(bx - 28, 1, &FONT_S, "USB", C_DIM);
    }
    cv_blit(0, Y_HEAD);
}
/* full redraw: the strips (head, columns, graph, foot) cover the rest, so only
 * the space between them is cleared, then the rules are drawn */
UI_SMALL static void draw_frame(void)
{
    uint32_t i;
    lcd_fill(0, H_HEAD, 240, Y_LABEL - H_HEAD, C_BLACK);
    lcd_fill(0, Y_SEP_END, 240, Y_GRAPH - Y_SEP_END, C_BLACK);
    lcd_fill(0, Y_GRAPH + H_GRAPH, 240, Y_FOOT - Y_GRAPH - H_GRAPH, C_BLACK);
    for (i = 0; i < 4u; i++)                            /* left inset of each column */
        lcd_fill(i * 60u, Y_LABEL, 4, Y_SEP_END - Y_LABEL, C_BLACK);
    lcd_fill(0, H_HEAD, 240, 1, C_LINE);
    lcd_fill(0, Y_FOOT - 2, 240, 1, C_AMB);

}

/* one column: [icon] LABEL / value unit / gauge, redrawn only when it changed.
 * ratio: 0..1000 for the gauge, -1 = no gauge. icon: ICON_* (icons.c), ICON_AUTO = by label */
#define LABEL_X (FELUCCA_ICONS ? ICON_CELL + ICON_GAP : 0)
UI_SMALL static void draw_column(uint32_t c, const char *label, const char *val, const char *unit, uint16_t vc,
                        int32_t ratio, uint32_t icon)
{
    char l[8], v[8], u[8], key[32], shown[16];
    /* Labels take the card width; retain icon in the cache for compatibility. */
    fit(l, label, &FONT_S, 48);
    fit(v, val, &FONT_S, is_eng_name(val) ? 56 : 48);   /* engine names whole */
    str_cpy(u, unit, sizeof u);
    str_cpy(key, l, 8);                                 /* cache key: texts + colour + gauge */
    str_cpy(key + str_len(key), "|", 2);
    str_cpy(key + str_len(key), v, 8);
    str_cpy(key + str_len(key), "|", 2);
    str_cpy(key + str_len(key), u, 8);
    {
        uint32_t n = str_len(key);
        key[n] = (char)('A' + (vc == C_WHITE) + (vc == C_DIM) * 2);
        key[n + 1] = (char)(' ' + (ratio < 0 ? 0 : 1 + ratio / 20));
        key[n + 2] = (char)(icon == ICON_NONE ? '~' : '!' + icon % 90u);   /* same label, other icon */
        key[n + 3] = 0;
    }
    if (c < 4u) {                                       /* (the big values: their own fit) */
        str_cpy(ui.big_l[c], l, 8);
        str_cpy(ui.big_v[c], val, sizeof ui.big_v[c]);
        str_cpy(ui.big_u[c], unit, 8);
        ui.big_c[c] = vc;
    }
    if (c == ui.hot_col) {
        str_cpy(ui.focus_l, l, 8);
        str_cpy(ui.focus_v, v, 8);
        str_cpy(ui.focus_u, u, 8);
    }
    if (!ui.force && str_eq(key, ui.col[c]))
        return;
    str_cpy(ui.col[c], key, sizeof ui.col[c]);
    cv_begin(56, Y_SEP_END - Y_LABEL, C_BLACK);
    str_cpy(shown,v,sizeof shown);
    if(text_w(&FONT_S,shown)+text_w(&FONT_S,unit)<=48)
        str_cpy(shown+str_len(shown),unit,sizeof shown-str_len(shown));
    ui_card(0,0,l,shown,ratio,vc,ui.hot_t&&ui.hot_col==c);
    cv_blit(c * 60u + 2u, Y_LABEL);
}

/* Matches voice.c: attack is linear, decay and release are exponential
 * (env += (target - env) * k each tick, ~99 % after the set time). Time
 * axis is the parameter value (the times themselves are exponential). */
UI_SMALL static void graph_adsr(const track_t *t, uint16_t c)
{
    int32_t a = 4 + t->p[P_ATK] * 50 / 127, d = 6 + t->p[P_DEC] * 50 / 127, r = 6 + t->p[P_REL] * 60 / 127;
    int32_t top = 8, bot = 90, sus = t->p[P_SUS] * 1000 / 127;          /* 0..1000 */
    int32_t x0 = 6, x1 = x0 + a, x3 = 232 - r, i, px, py;
    int32_t e = 32768;                                                  /* exp(-4.6 u), Q15 */
#define EGY(lvl) (bot - (lvl) * (bot - top) / 1000)
    cv_line(x0, bot, x1, top, ui.hot_t&&ui.hot_col==0?C_WHITE:c);                                       /* attack: linear */
    px = x1;
    py = top;
    for (i = 1; i <= d; i++) {                                          /* decay: exponential to SUS */
        int32_t lvl;
        e = (e * (32768 - 150733 / d)) >> 15;           /* k^d = exp(-4.6) */
        lvl = sus + ((1000 - sus) * e >> 15);
        cv_line(px, py, x1 + i, EGY(lvl), ui.hot_t&&ui.hot_col==1?C_WHITE:c);
        px = x1 + i;
        py = EGY(lvl);
    }
    cv_line(px, py, x3, EGY(sus), ui.hot_t&&ui.hot_col==2?C_WHITE:c);                                    /* sustain */
    px = x3;
    py = EGY(sus);
    e = 32768;
    for (i = 1; i <= r; i++) {                                          /* release: exponential to 0 */
        e = (e * (32768 - 150733 / r)) >> 15;
        cv_line(px, py, x3 + i, EGY(sus * e >> 15), ui.hot_t&&ui.hot_col==3?C_WHITE:c);
        px = x3 + i;
        py = EGY(sus * e >> 15);
    }
    cv_line(0, bot + 1, 239, bot + 1, C_LINE);
#undef EGY
}

UI_SMALL static void graph_lfo(const track_t *t, uint16_t c)
{
    int32_t x, py = 50;
    uint32_t ph = (uint32_t)t->p[P_LPHASE] << 25;
    uint64_t span = (uint64_t)(t->p[P_LSYNC] ? lfo_sync_inc(t->p) : LFO_INC[t->p[P_LRATE]&127]) * FS * 2u / CTL;
    cv_text(4,0,&FONT_S,"2s GUIDE",C_GRAY);
    for (x = 0; x < 240; x++) {                      /* fixed-time schematic, never updates audio state */
        uint64_t phase=(uint64_t)ph+span*(uint32_t)x/240u;
        int32_t w=lfo_wave((track_t *)t,(uint32_t)phase),y;
        if(t->p[P_LWAVE]==4) w=(int32_t)((((uint32_t)(phase>>32)+1u)*2654435761u)>>16)-32768;
        if(t->p[P_LPOL]) w=(w+32768)/2;
        if(t->p[P_LFADE]) {
            uint32_t step=ENV_LIN[t->p[P_LFADE]&127]>>9;
            uint64_t fade=(uint64_t)(step?step:1)*(uint32_t)x*FS*2u/(240u*CTL);
            w=w*(int32_t)(fade>32767?32767:fade)/32768;
        }
        y=50-w*38/32768;
        if (x)
            cv_line(x - 1, py, x, y, c);
        py = y;
    }
    cv_line(0, 50, 239, 50, C_LINE);
}

UI_SMALL static void graph_steps(const track_t *t, uint16_t c)
{
    uint32_t i, len = (uint32_t)t->p[P_SLEN];
    for (i = 0; i < NSTEP; i++) {                    /* 4 rows of 16 thin bars */
        int32_t x = 6 + (int32_t)(i % 16u) * 14 + (int32_t)(i % 16u) / 4 * 4, y = 6 + (int32_t)(i / 16u) * 24;
        const step_t *st = &t->step[i];
        if (i >= len)
            continue;
        if (step_on(st))
            cv_rect(x, y, 1, 14, c);
        else if (st->time == ST_TIE)
            cv_rect(x, y + 6, 1, 8, C_DIM);
        else
            cv_rect(x, y + 13, 1, 1, C_DIM);
        if (st->flags & SF_ACCENT)
            cv_rect(x - 1, y - 2, 3, 1, c);
        if ((song.playing && i == t->seq_idx) || i == ui.cursor)
            cv_rect(x - 1, y + 16, 3, 3, C_WHITE);
    }
}
/* STEP/NOTES viewport, keeping independent chains joined through mixed attacks. */
UI_SMALL static void graph_roll(const track_t *t, uint16_t c)
{
    uint32_t i,j,len=trk_len(t),span=roll_span(),base=ui.cursor/span*span;
    int32_t lo=127,hi=0,x0=22,w=216,top=20,bot=118,rh;
    piano_selection_t selected=roll_selection();
    int slot=notes_page()?piano_selection_slot(t,selected):-1;
    int32_t focus=slot>=0?selected.pitch:last_note;
    for(i=0;i<len;i++) {
        const step_t *st=&t->step[i];
        if(!step_on(st))continue;
        for(j=0;j<st->n;j++){if(st->note[j]<lo)lo=st->note[j];if(st->note[j]>hi)hi=st->note[j];}
    }
    if(lo>hi)lo=hi=focus;
    if(hi-lo<11){lo=clamp((lo+hi)/2-6,0,115);hi=lo+11;}
    if(hi-lo>17){lo=clamp(focus-8,lo,hi-17);hi=lo+17;}
    lo=clamp(lo-1,0,127);hi=clamp(hi+1,0,127);rh=(bot-top)/(hi-lo+1);
    char label[16];
    fmt_int(label,(int32_t)base+1);str_cpy(label+str_len(label),"-",2);
    fmt_int(label+str_len(label),(int32_t)(base+span<len?base+span:len));
    cv_text(2,0,&FONT_S,label,C_GRAY);
    str_cpy(label,"ZOOM ",sizeof label);fmt_int(label+5,(int32_t)(8u<<(ui.roll_zoom+1)));
    cv_text(238-text_w(&FONT_S,label),0,&FONT_S,label,C_DIM);
    for(int32_t pitch=lo;pitch<=hi;pitch++) {
        int32_t y=bot-(pitch-lo+1)*rh;
        if((0x54au>>(pitch%12))&1u)cv_rect(x0,y,w,rh,TE_G1);
        if(!(pitch%12)) {
            note_name(label,pitch);cv_text(0,y+rh/2-6,&FONT_S,label,C_GRAY);
            cv_rect(x0,y+rh-1,w,1,C_DIM);
        }
    }
    for(i=0;i<=span;i++) {
        int32_t x=x0+(int32_t)i*w/(int32_t)span;
        if(!(i%4u))cv_rect(x,top,1,bot-top,i%16u?C_DIM:C_GRAY);
        else if(span<=32u)cv_rect(x,bot+1,1,2,C_DIM);
    }
    for(i=0;i<span && base+i<len;i++) {
        step_t effective;note_effective(t,base+i,&effective);
        int32_t x=x0+(int32_t)i*w/(int32_t)span,next=x0+(int32_t)(i+1)*w/(int32_t)span;
        for(j=0;j<effective.n;j++) {
            int32_t pitch=effective.note[j];if(pitch<lo||pitch>hi)continue;
            int32_t y=bot-(pitch-lo+1)*rh+1;
            uint32_t tied=effective.flags&SF_TIE(j),force=(effective.lvl>>(2*j))&3u;
            int32_t left=tied?(i?x-1:x0):x+1;
            cv_rect(left,y,next-left-1>0?next-left-1:1,rh-1,
                    effective.flags&SF_ACCENT?C_WHITE:force==LV_GHOST?TE_DIM[song.sel&3u]:c);
            if(j==0u && t->step[base+i].time==ST_NOTE && (effective.flags&SF_SLIDE))
                cv_line(next-2,y,next+1,y+2,c);
        }
    }
    if(slot>=0 && selected.pitch>=lo && selected.pitch<=hi) {
        uint32_t end=selected.step+note_chain_len(t,selected.step,selected.pitch);
        int32_t xa=x0+((int32_t)selected.step-(int32_t)base)*w/(int32_t)span;
        int32_t xb=x0+((int32_t)end-(int32_t)base)*w/(int32_t)span-1;
        xa=clamp(xa,x0,x0+w-1);xb=clamp(xb,xa,x0+w-1);
        int32_t y=bot-((int32_t)selected.pitch-lo+1)*rh;
        cv_rect(xa,y,xb-xa+1,1,C_WHITE);cv_rect(xa,y+rh,xb-xa+1,1,C_WHITE);
        cv_rect(xa,y,1,rh+1,C_WHITE);cv_rect(xb,y,1,rh+1,C_WHITE);
    }
    int32_t cx=x0+((int32_t)ui.cursor-(int32_t)base)*w/(int32_t)span;
    cv_rect(cx,bot+2,2,2,C_WHITE);
    if(song.playing && t->seq_idx>=base && t->seq_idx<base+span)
        cv_rect(x0+((int32_t)t->seq_idx-(int32_t)base)*w/(int32_t)span,top,1,bot-top,C_HI);
}
/* Physical black-key names stay fixed when ROOT / keyboard octave changes. */
UI_SMALL static void graph_scale(const track_t *t, uint16_t c)
{
    if(!t->p[P_CHORD] || t->p[P_QUANT]==3) {
        uint32_t root=(uint32_t)t->p[P_ROOT],mask=scale_mask(t);
        mask=(mask<<root)|(mask>>(12u-root));
        for(uint32_t i=0;i<12;i++) {
            int32_t x=6+(int32_t)i*19, y=((0x54au>>i)&1u)?10:40;
            uint16_t col=(mask>>i)&1u?(i==root?C_WHITE:c):C_DIM;
            cv_rect(x,y,16,1,col);cv_rect(x+7,y,1,40,col);
        }
        return;
    }
    static const char *const guide[]={"F#3 FLIP3 G#3 +7","A#3 SUS4  C#4 +9","D#4 INV+  F#4 SUS2","G#4 +13   A#4 MAJ7","C#5 MIN7  D#5 INV-","F#5 OPEN  WHEN LATCHED: TAP"};
    for(uint32_t i=0;i<6;i++)
        cv_text(8,(int32_t)i*16,&FONT_S,guide[i],c);
}

UI_SMALL static void graph_fx(const track_t *t, uint16_t c)
{
    uint32_t i;
    for (i = 0; i < 4u; i++) {
        int32_t value=t->p[P_DIST+i];
        if(is_drum(t))value=i==0?song.g[G_DRLVL]:i==1?t->p[P_PAN]+64:i==2?song.g[G_DRDLY]:song.g[G_DRREV];
        int32_t h = value * 80 / 127, x = (int32_t)i * 60 + 28;
        cv_rect(x, 10, 1, 80, C_LINE);
        cv_rect(x, 90 - h, 1, h, c);
        cv_rect(x - 3, 90 - h, 7, 1, c);
    }
}
/* SLICER page: the pattern's 16 steps, a 'x' step a full bar; a '.' step: GATE a bar as high as
 * it stays open (DEPTH), STUT hatched (it repeats the last 'x'); the step playing underlined.
 * Grey when the SLICER is OFF. */
UI_SMALL static void graph_slicer(const track_t *t, uint16_t c)
{
    uint32_t i, pat = sl_pattern(t), mode = (uint32_t)t->p[P_SLCR], cur = sl[t - trk].idx;
    int32_t open = 70 - t->p[P_SLDEPTH] * 70 / 127;      /* px a closed GATE step keeps */
    uint16_t col = mode == SL_OFF ? C_DIM : c;
    for (i = 0; i < 16u; i++) {
        int32_t x = 4 + (int32_t)i * 14 + (int32_t)(i / 4u) * 2, y;
        if ((pat >> i) & 1u) {
            cv_rect(x, 10, 11, 70, col);
        } else if (mode == SL_STUT) {
            for (y = 10; y < 80; y += 4)
                cv_rect(x, y, 11, 1, col);
        } else {
            cv_rect(x, 79, 11, 1, C_LINE);
            if (open)
                cv_rect(x, 80 - open, 11, open, C_DIM);
        }
        if (mode != SL_OFF && i == cur)
            cv_rect(x, 85, 11, 3, C_WHITE);
    }
}
/* Hash raw fixed-size cells: drums share this union and are not synth notes. */
UI_SMALL static uint32_t steps_hash(const track_t *t)
{
    const uint8_t *bytes=(const uint8_t *)t->step;
    uint32_t h=2166136261u;
    for(uint32_t i=0;i<sizeof t->step;i++) h=(h^bytes[i])*16777619u;
    return h;
}

UI_SMALL static uint32_t str_hash(uint32_t h, const char *s)
{
    while (*s)
        h = (h ^ (uint8_t)*s++) * 16777619u;
    return h;
}


/* a page drawn with its values large (graph_big) */
UI_SMALL static int big_page(const page_t *pg)            /* (2.6: the INSERT pages too: their middle stayed empty) */
{
    return (pg->graph == GR_NONE || pg->graph == GR_INS) && pg->scope != SC_SONG && pg->scope != SC_DRUM;
}
UI_SMALL static uint32_t graph_signature(void)
{
    const page_t *pg = cur_page();
    const track_t *t = TSEL;
    uint32_t h = 2166136261u ^ (ui.hot_t ? ui.hot_col+1u : 0u), i;
    for(i=0;i<G_COUNT;i++) h=(h^(uint32_t)song.g[i])*16777619u;
    if (ui.hot_t && settings.zoom)
        h = str_hash(str_hash(str_hash(h ^ 0x5555u, ui.focus_v), ui.focus_l), ui.focus_u);
    if (!ui.home && big_page(pg))                    /* the big values: as the columns show them */
        for (i = 0; i < 4u; i++)
            h = str_hash(str_hash(str_hash(h ^ ui.big_c[i] * 31u, ui.big_v[i]), ui.big_l[i]), ui.big_u[i]);
    if (ui.home)
        return h ^ (ui.frame / 3u);                  /* activity lanes: bounded UI cadence */
    h ^= (uint32_t)pg->graph * 131u + TSEL->eng_req + song.sel * 7777u + pg->id[0]*104729u;
    for (i = 0; i < P_COUNT; i++)
        h = (h ^ (uint32_t)t->p[i]) * 16777619u;
    h ^= (uint32_t)TSEL->preset * 7u + (uint32_t)song.g[G_SLOT] * 13u + TSEL->user * 257u + up_gen * 7919u + ui.uslot * 104729u;
    {   /* (2.7) the favourites: the PRESETS list and its FAV column */
        uint32_t q;
        for (q = 0; q < FAV_N; q++) h = h * 31u + fav[q];
    }
    if (pg->graph == GR_SLCR && t->p[P_SLCR])        /* the SLICER's step playing */
        h ^= (sl[song.sel].idx + 1u) * 2654435761u;
    if (pg->graph == GR_SLOTS)                       /* (a checksum over each slot) */
        for (i = 0; i < 4u; i++)
            h ^= (uint32_t)project_used(i) << (20u + i);
    if (pg->graph == GR_STEPS || pg->graph == GR_ROLL) {
        uint32_t ph = song.playing ? t->seq_idx : 0xFFFFu;
        if (pg->graph == GR_ROLL) {
            uint32_t span=roll_span();
            if (ph/span!=ui.cursor/span) ph=0xFFFFu;
            h ^= ((uint32_t)ui.roll_zoom+1u)*104729u;
            h ^= (uint32_t)roll_selection().pitch*2654435761u;
        }
        h ^= steps_hash(t) + ph * 31u + ui.cursor * 7919u;
    }
    return h;
}
/* preset browser: the list by kind (ui.c BANK), the current one in white */
UI_SMALL static void graph_browse(void)
{
    uint32_t total, cur = preset_pos(&total), e, k;
    int32_t row;
    if (!total)
        return;
    for (row = -3; row <= 3; row++) {
        int32_t y = 4 + (row + 3) * 17;
        char tag[5], nm[13];
        int sel = row == 0;
        uint32_t n = (cur + total * 4u + (uint32_t)row) % total;
        e = preset_at(n, &k);
        if (e == NENGINES) {                             /* user preset: "U07" and its name */
            up_slot_label(tag, k);
            up_name(k, nm);
        } else {                                         /* its kind: BASS, KEYS, PAD... */
            str_cpy(tag, preset_kind(n), sizeof tag);
            str_cpy(nm, ENGINES[e]->presets[k].name, sizeof nm);
        }
        if (sel)
            cv_rect(4, y + 6, 3, 3, C_WHITE);
        cv_text(14, y, &FONT_S, tag, sel ? C_GRAY : C_DIM);
        cv_text(54, y, &FONT_S, nm, sel ? C_WHITE : C_GRAY);
    }
}

/* user preset slots around the selected one: "U07  NAME" / EMPTY, the selected one in white */
UI_SMALL static void graph_user(void)
{
    int32_t row, first = clamp((int32_t)ui.uslot - 3, 0, UP_SLOTS - 7);
    for (row = 0; row < 7; row++) {
        uint32_t k = (uint32_t)(first + row);
        int32_t y = 4 + row * 17;
        char tag[4], nm[13];
        int sel = k == ui.uslot, used = up_used(k);
        up_slot_label(tag, k);
        if (used)
            up_name(k, nm);
        else
            str_cpy(nm, "EMPTY", sizeof nm);
        if (sel)
            cv_rect(4, y + 6, 3, 3, C_WHITE);
        cv_text(14, y, &FONT_S, tag, sel ? C_WHITE : C_GRAY);
        cv_text(54, y, &FONT_S, nm, used ? (sel ? C_WHITE : C_HI) : C_DIM);
    }
}

/* the sections A..D (the four slots): used / empty, the selected one in white */
UI_SMALL static void graph_slots(void)
{
    uint32_t i;
    for (i = 0; i < 4u; i++) {
        int32_t y = 8 + (int32_t)i * 26;
        char b[4];
        int sel = (int32_t)i + 1 == song.g[G_SLOT];
        b[0] = (char)('A' + i);
        b[1] = 0;
        if (sel)
            cv_rect(4, y + 6, 3, 3, C_WHITE);
        cv_text(14, y, &FONT_S, b, sel ? C_WHITE : C_GRAY);
        cv_text(40, y, &FONT_S, project_used(i) ? "USED" : "EMPTY", project_used(i) ? (sel ? C_WHITE : C_HI) : C_DIM);
    }
}

/* Shared track identity and level helpers for the overview and layers. */
UI_SMALL static uint32_t trk_level(uint32_t c)                /* LEVEL 0..127 (the drum track: GLO > DRUMS LEVEL) */
{
    return (uint32_t)(c == TRK_DRUM ? song.g[G_DRLVL] : trk[c].p[P_LEVEL]) & 127u;
}

UI_SMALL static void trk_short_name(uint32_t c, char *b)      /* the track's sound, b holds 13 */
{
    const track_t *t = &trk[c];
    const engine_t *e = ENGINES[t->eng_req % NENGINES];
    if (c == TRK_DRUM)
        str_cpy(b, DRUM_KIT_NAMES[drum_kit()], 13);
    else if (user_of(t) < UP_SLOTS)
        up_name(user_of(t), b);
    else if (e->npresets)
        str_cpy(b, e->presets[t->preset % e->npresets].name, 13);
    else
        str_cpy(b, e->name, 13);
}

#include "scratch.h"
#define ui_scope_snap (main_scratch.snap)
_Static_assert(sizeof main_scratch.snap == SCOPE_N * sizeof(int16_t), "scope scratch size");
/* The visualizers retain their shared snapshot; HOME uses pattern lanes. */
/* Four-track HOME. Pattern state, not delay repeats, drives the lanes. */
UI_SMALL static void graph_home(void)
{
    uint32_t r,i;
    uint16_t sun[4];
    uint32_t light=home_sun_light();
    for(r=0;r<4;r++) sun[r]=rgb565_scale(TE_COL[r],light,16u);
    /* Striped round sun and wireframe skyline; no bitmap or framebuffer. */
    for(int32_t y=0;y<44;y++) {
        int32_t w=22,dy=y-21;
        if(y>20 ? y%5>1 : y%5==4) continue;
        while(w*w+dy*dy>22*22) w--;
        cv_rect(120-w,y,2*w+1,1,sun[y<12?2:y<27?1:3]);
    }
    static const uint8_t ridge[][2]={{0,38},{18,16},{38,32},{51,24},{77,40},{94,44}};
    for(uint32_t side=0;side<2;side++) for(uint32_t p=1;p<NELEM(ridge);p++) {
        int32_t a=side?240-ridge[p-1][0]:ridge[p-1][0];
        int32_t b=side?240-ridge[p][0]:ridge[p][0];
        cv_line(a,ridge[p-1][1],b,ridge[p][1],C_DIM);
        cv_line(a,ridge[p-1][1],b,44,C_LINE);
    }
    cv_line(0,44,239,44,C_AMB);
    for(int32_t x=-180;x<=420;x+=60) cv_line(120,44,x,123,C_LINE);
    for(r=0;r<NTRK;r++) {
        const track_t *t=&trk[r];
        int32_t y=48+(int32_t)r*19;
        char n[2]={(char)('1'+r),0};
        int mute=!trk_level(r) || t->p[P_MUTE] || (song.solo && !(song.solo&(1u<<r)));
        cv_rect(2,y,236,18,TE_G1);
        cv_rect(2,y,2,18,r==song.sel?C_WHITE:TE_DIM[r]);
        cv_text(9,y+1,&FONT_S,n,TE_COL[r]);
        for(i=0;i<16;i++) {
            uint32_t st=(t->seq_idx/16u)*16u+i;
            int on=trk_step_on(t,st);
            int32_t x=32+(int32_t)i*11;
            if(st>=(uint32_t)t->p[P_SLEN]) continue;
            cv_rect(x,y+7,9,on?8:2,mute?TE_G2:on?TE_COL[r]:TE_DIM[r]);
            if(song.playing&&st==t->seq_idx) cv_rect(x,y+1,2,16,C_WHITE);
        }
        if((song.rec>>r)&1u) cv_text(218,y+1,&FONT_S,"R",TE_RED);
        else if(mute) cv_text(218,y+1,&FONT_S,"M",C_WHITE);
    }
}

/* SLOOP 2.4: a page without a graph (EDIT, VOICE, the DEST pages, GLOBAL, MASTER, SYSTEM...): its values
 * large in the empty space, 2 x 2 as the knobs (KNOB 1 2 / KNOB 3 4), the one turned in white; one value
 * alone (FILTER) across the width. The columns (draw_columns, drawn first) fill ui.big_* */
UI_SMALL static void graph_big(void)
{
    uint32_t c, n = 0;
    for (c = 0; c < 4u; c++)
        n += ui.big_l[c][0] != 0;
    if (n > 1u) {
        cv_rect(119, 6, 1, 112, C_LINE);
        cv_rect(6, 62, 228, 1, C_LINE);
    }
    for (c = 0; c < 4u; c++) {
        int32_t x0 = c & 1u ? 126 : 6, y0 = c & 2u ? 64 : 2, x;
        if (!ui.big_l[c][0])
            continue;
        /* Keep the complete value and its unit in their half of the screen. */
        int32_t uw = ui.big_u[c][0] ? text_w(&FONT_S, ui.big_u[c]) + 4 : 0;
        const felucca_font_t *f = text_w(&FONT_L, ui.big_v[c]) + uw <= 108 ? &FONT_L : &FONT_S;
        if (n == 1u) {
            x0 = (240 - text_w(f, ui.big_v[c]) - uw) / 2;
            y0 = 30;
        }
        cv_text(x0, y0 + 2, &FONT_S, ui.big_l[c], TE_COL[c & 3u]);
        x = cv_text(x0, y0 + 20 + FONT_L.h - f->h, f, ui.big_v[c], ui.big_c[c] == C_DIM ? C_DIM : ui.big_c[c] == C_WHITE ? C_WHITE : C_HI);
        if (ui.big_u[c][0])
            cv_text(x + 4, y0 + 34, &FONT_S, ui.big_u[c], C_GRAY);
    }
}

/* Parameter schematics, not sampled audio or measured response curves. */
UI_SMALL static void graph_guide(const page_t *pg, const track_t *t, uint16_t c)
{
    int32_t i,v;
    cv_text(4,0,&FONT_S,"GUIDE",C_GRAY);
    if(pg->graph==GR_DEST) {
        for(i=0;i<4;i++) {
            uint32_t id=pg->id[i];
            if(id==P_ESYNC) continue;
            v=t->p[id];
            cv_text(4,20+i*19,&FONT_S,TP[id].label,TE_COL[i]);
            /* Signed amounts share a zero marker; both extrema remain visible. */
            int32_t limit = v < 0 ? -TP[id].min : TP[id].max;
            int32_t w = limit ? v * 88 / limit : 0;
            cv_line(140,20+i*19,140,35+i*19,C_GRAY);
            cv_rect(w<0?140+w:140,24+i*19,w<0?-w:w,7,TE_COL[i]);
        }
    } else if(pg->graph==GR_FILTER) {
        v=t->p[P_TFLT];
        cv_text(8,74,&FONT_S,v<0?"LOW PASS":v>0?"HIGH PASS":"OFF / DRY",C_WHITE);
        int32_t x=120+v*100/64;
        cv_line(8,v>0?65:25,x,25,c);
        cv_line(x,25,232,v<0?65:25,c);
        cv_line(x,20,x,68,C_WHITE);
    } else {
        /* Leave a complete caption inside the graph, above the footer. */
        int32_t base=76,h=60;
        int32_t gap=clamp((int32_t)((uint64_t)dly_samples(song.g[G_DTIME])*200u*song.g[G_BPM]/(FS*60u*8u)),8,200);
        for(i=0;i<24 && 12+i*gap<236;i++) {
            cv_rect(12+i*gap,base-h,3,h,i?c:C_WHITE);
            h=h*song.g[G_DFDBK]/128;
        }
        cv_text(8,base+4,&FONT_S,"DRY   TEMPO-SYNCED ECHOES",C_GRAY);
    }
}
UI_SMALL static void draw_graph(void)
{
    const page_t *pg = cur_page();
    const track_t *t = TSEL;
    uint16_t c = TE_COL[song.sel & 3u];               /* LIVE: the curves in the track's colour */
    uint32_t sig, top, drum_note = !ui.home && is_drum(t) && !page_for_drum(pg);
    if (!ui.home && pg->graph == GR_TRK) {
        studio_tracks_draw();
        ui.graph_top = 1;                            /* the next graph draws its top rows again */
        return;
    }
    sig = graph_signature();
    if (!ui.force && sig == ui.graph_sig)
        return;
    ui.graph_sig = sig;
    cv_begin(240, H_GRAPH, C_BLACK);
    cv_oy = ui.home ? 0 : G_OY;
    if (ui.home) {
        graph_home();
    } else if (drum_note) {                          /* a page the drum track has no use for */
        static const char *const L[2] = {"DRUM TRACK", "SEQ  TRACKS  GLO DRUMS"};
        cv_text((240 - text_w(&FONT_S, L[0])) / 2, 26, &FONT_S, L[0], C_HI);
        cv_text((240 - text_w(&FONT_S, L[1])) / 2, 52, &FONT_S, L[1], C_DIM);
    } else {
        switch (pg->graph) {
        case GR_DEST: case GR_FILTER: case GR_DELAY:
            cv_oy=0;graph_guide(pg,t,c); break;
        case GR_ADSR:
            graph_adsr(t, c);
            break;
        case GR_LFO:
            graph_lfo(t, c);
            break;
        case GR_STEPS:
            graph_steps(t, c);
            break;
        case GR_ROLL:
            cv_oy=0;
            graph_roll(t, c);
            break;
        case GR_SCALE:
            graph_scale(t, c);
            break;
        case GR_FX:
            graph_fx(t, c);
            break;
        case GR_SLCR:
            graph_slicer(t, c);
            break;
        case GR_BROWSE:
            cv_oy = 0;
            graph_browse();
            break;
        case GR_SLOTS:
            cv_oy = 0;
            graph_slots();
            break;
        case GR_USER:
            cv_oy = 0;
            graph_user();
            break;
        default:
            if (big_page(pg)) {
                cv_oy = 0;
                graph_big();
            }
            break;
        }
    }
    top = ui.home || (!drum_note && (pg->graph == GR_BROWSE || pg->graph == GR_SLOTS || pg->graph == GR_USER ||
                                     big_page(pg) || pg->graph==GR_ROLL || pg->graph==GR_DEST || pg->graph==GR_FILTER || pg->graph==GR_DELAY));   /* these draw from the top */
    cv_oy = 0;
    if (ui.hot_t && settings.zoom && pg->graph != GR_ROLL && !(top && big_page(pg) && !drum_note)) {   /* (the big values show it already) */                 /* focus (menu ZOOM): the touched value, large and white */
        int32_t x;
        top = 1;
        cv_rect(0, 0, 150, 50, C_BLACK);
        cv_text(4, 0, &FONT_S, ui.focus_l, C_GRAY);
        x = cv_text(4, 16, &FONT_L, ui.focus_v, C_WHITE);
        cv_text(x + 4, 30, &FONT_S, ui.focus_u, C_DIM);
    }
    /* graphs keep out of the top G_OY rows: skip them unless something is (or was) there */
    cv_blit_from(0, Y_GRAPH, top || ui.graph_top || ui.force ? 0u : G_OY);
    ui.graph_top = (uint8_t)top;
}

/* the page shown: its place in its family and how many (*n), as SELECT walks them (2.7: the screens of their own, SONG
 * and DRUMS, are not counted: SEQ is STEP 1/2, PATTERN 2/2; the page shown always counts) */
UI_SMALL static uint32_t page_pos(uint32_t *n)
{
    const page_t *pg = cur_page();
    uint32_t i, k = 0;
    *n = 0;
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].fam == pg->fam && (i == ui.page || (page_shown(&PAGES[i]) && PAGES[i].scope != SC_SONG &&
                                                         PAGES[i].scope != SC_DRUM))) {
            (*n)++;
            if (i == ui.page)
                k = *n;
        }
    return k;
}

/* Shared by the footer and help, so hold labels describe the actual gesture. */
UI_SMALL static const char *page_hold_hint(const page_t *pg)
{
    static const char *const hints[FAM_COUNT]={
        "PRESETS: SOUND / ALGO: TRACK", "HOLD ENV: TREMOLO", "HOLD LFO: VIBRATO",
        "HOLD FX: PUNCH", "HOLD SEL: KEY/LATCH", "HOLD EDIT: ERASE",
        "HOLD GLO: MIX/FILL", "HOLD SAVE: SONG/NEW", "HOLD ARP: ROLL/LATCH",
        "HOLD SEQ: STEP KEYS", "PRESETS: SOUND / ALGO: TRACK"};
    return pg->scope==SC_STEP?"PRESETS: ZOOM / OCT: TOOLS":hints[pg->fam];
}
UI_SMALL static const char *page_title(const page_t *pg)
{
    const char *name=pg->scope==SC_ENGINE?ENGINES[TSEL->eng_req%NENGINES]->page_title[pg->id[0]!=P_E0]:0;
    return name?name:pg->title;
}
UI_SMALL static void draw_foot(void)
{
    const page_t *pg=cur_page();
    char title[32],hint[32],scope[8];
    uint32_t n,k,sig;
    if(ui.home) {
        str_cpy(title,"HOME",sizeof title);
        trk_short_name(song.sel,hint);
    } else {
        str_cpy(title,page_title(pg),sizeof title);
        k=page_pos(&n);
        if(n>1) { str_cpy(title+str_len(title)," ",2);fmt_int(title+str_len(title),k);
            str_cpy(title+str_len(title),"/",2);fmt_int(title+str_len(title),n); }
        str_cpy(hint,page_hold_hint(pg),sizeof hint);
    }
    str_cpy(scope,pg->scope==SC_GLOBAL&&!ui.home?"GLOBAL":"T1",sizeof scope);
    if(scope[0]=='T') scope[1]=(char)('1'+song.sel);
    while(text_w(&FONT_S,title)>228-text_w(&FONT_S,scope)-8) title[str_len(title)-1u]=0;
    sig=str_hash(str_hash(str_hash(7,title),hint),scope);
    if(!ui.force&&sig==ui.foot_sig) return;
    ui.foot_sig=sig;
    cv_begin(240,H_FOOT,C_BLACK);
    cv_text(4,0,&FONT_S,title,C_WHITE);
    cv_text(236-text_w(&FONT_S,scope),0,&FONT_S,scope,TE_COL[song.sel]);
    cv_text(4,20,&FONT_S,hint,TE_G4);
    cv_blit(0,Y_FOOT);
}
/* Share ordinary cards while keeping special-page overrides off HOME. */
UI_SMALL static void draw_parameter_column(uint32_t c,const param_desc_t *d,int32_t value,int home)
{
    char val[12];const char *unit;
    if (!home && cur_page()->id[c] == G_INFO && cur_page()->scope == SC_GLOBAL) {
        fmt_int(val, (int32_t)(song.cpu_q8 * 100u / 256u));
        unit = "%";
    } else {
        param_format(d, value, val, &unit);
        /* Generic enum formatting retains five-character legacy displays;
         * these six-letter type names fit the current cards in full. */
        if (d == &GP[G_RMODEL])
            str_cpy(val,N_RMODEL[clamp(value,0,5)],sizeof val);
    }
    if (!home && cur_page()->scope == SC_RECORD && c == 0u && value) {
        uint32_t snap_units = div_units(value == REC_SNAP_EIGHTH ? 1u : 0u);
        uint32_t track_units = div_units(trk_div(TSEL));
        if (snap_units < track_units || snap_units % track_units) unit = "DIV LIMIT";
    }
    const char *label = d->label;
    if(!home && is_drum(TSEL) && cur_page()->graph==GR_FX && !c)label="LEVEL";
    if(!home && cur_page()->scope==SC_GLOBAL && cur_page()->id[0]==G_DTYPE && (song.g[G_DTYPE]==1 || song.g[G_DTYPE]==3) && c==2)label="TONE";
    if (!home && cur_page()->scope==SC_GLOBAL && cur_page()->id[0]==G_LTYPE && c) {
        static const char *const labels[5][3] = {
            {"WOW","DARK","DAMAGE"}, {"FLUTTR","DARK","DRIVE"},
            {"WARBLE","DARK","DAMAGE"}, {"RATE","LATCH","MIX"},
            {"DRIFT","BAND","STATIC"}
        };
        label=labels[song.g[G_LTYPE]?song.g[G_LTYPE]-1:0][c-1];
    }
    draw_column(c, !home && cur_page()->fam == FAM_SCL && cur_page()->id[c] == P_AHOLD ? "LATCH" : label, val, unit, VAL(c), !home && d->fmt == F_ENUM && d->max < 2 ? -1 : RATIO(d, value),
                param_icon(d, value));
}
UI_SMALL static void draw_columns(void)
{
    uint32_t c;
    char val[12];
    const char *unit;
    if (ui.home) {
        for (c = 0; c < 4u; c++) {
            int16_t *vp;
            const param_desc_t *d = home_param(c, &vp);
            draw_parameter_column(c,d,*vp,1);
        }
        return;
    }
    if (is_drum(TSEL) && !page_for_drum(cur_page())) {   /* "DRUM TRACK" (the graph says so) */
        for (c = 0; c < 4u; c++)
            draw_column(c, "", "", "", C_HI, -1, ICON_AUTO);
        return;
    }
    if (cur_page()->scope == SC_TRK) {                 /* TRACK LEVEL LEN PAN of the selected track */
        const track_t *t = TSEL;
        uint32_t lvl = trk_level(song.sel);
        fmt_int(val, (int32_t)song.sel + 1);
        draw_column(0, "TRACK", val, "/4", VAL(0u), (int32_t)song.sel * 1000 / (NTRK - 1), ICON_AUTO);
        if (!lvl || t->p[P_MUTE]) {                    /* (MUTE: a turn of KNOB 2 unmutes, tracks_edit) */
            str_cpy(val, "MUTE", 12);
            unit = "";
        } else if (is_drum(t)) {
            fmt_int(val, (int32_t)lvl);
            unit = "";
        } else {
            param_format(&TP[P_LEVEL], (int32_t)lvl, val, &unit);
        }
        draw_column(1, "LEVEL", val, unit, lvl && !t->p[P_MUTE] ? VAL(1u) : C_DIM, (int32_t)lvl * 1000 / 127, ICON_AUTO);
        param_format(&TP[P_SLEN], t->p[P_SLEN], val, &unit);
        draw_column(2, "LEN", val, unit, VAL(2u), RATIO(&TP[P_SLEN], t->p[P_SLEN]), ICON_AUTO);
        param_format(&TP[P_PAN], t->p[P_PAN], val, &unit);
        draw_column(3, "PAN", val, unit, VAL(3u), RATIO(&TP[P_PAN], t->p[P_PAN]), param_icon(&TP[P_PAN], t->p[P_PAN]));
        return;
    }
    if (cur_page()->graph == GR_BROWSE) {
        uint32_t total, cur = preset_pos(&total);
        char u[8];
        fmt_int(val, (int32_t)cur + 1);
        str_cpy(u, "/", 8);
        fmt_int(u + 1, (int32_t)total);
        draw_column(0, "No.", val, u, VAL(0u), -1, ICON_NONE);
        draw_column(1, "ENG", ENGINES[TSEL->eng_req]->name, "", VAL(1u), -1, engine_icon(ENGINES[TSEL->eng_req]->name));
        draw_column(2, "", "", "", C_HI, -1, ICON_AUTO);
        draw_column(3, "FAV", fav_has(fav_key()) ? "ON" : "--", "", VAL(3u), -1, ICON_NONE);   /* (2.7) KNOB 4 */
        return;
    }
    if (cur_page()->graph == GR_USER) {                  /* SLOT, then three GO buttons */
        int used = up_used(ui.uslot);
        up_slot_label(val, ui.uslot);
        draw_column(0, "SLOT", val, "", VAL(0u), (int32_t)ui.uslot * 1000 / (int32_t)(UP_SLOTS - 1u), ICON_AUTO);
        draw_column(1, "LOAD", "--", "", used ? C_HI : C_DIM, -1, ICON_AUTO);
        draw_column(2, "ERASE", "--", "", used ? C_HI : C_DIM, -1, ICON_AUTO);
        draw_column(3, "SAVE", "--", "", C_HI, -1, ICON_AUTO);
        return;
    }
    if (notes_page() && !(fm1_in.buttons&((1u<<panel.btn[B_OCTUP])|(1u<<panel.btn[B_OCTDN])))) {
        piano_selection_t selected=roll_selection();
        int slot=piano_selection_slot(TSEL,selected);
        char step[8],unit[8],pitch[8]="--",length[8]="--";
        static const char *const levels[]={"NORM","GHOST","SOFT","HARD"};
        fmt_int(step,(int32_t)ui.cursor+1);str_cpy(unit,"/",sizeof unit);fmt_int(unit+1,trk_len(TSEL));
        if(slot>=0) {scale_note_name(pitch,TSEL,selected.pitch);fmt_int(length,note_chain_len(TSEL,selected.step,selected.pitch));}
        draw_column(0,"EVENT",step,unit,VAL(0u),-1,ICON_AUTO);
        draw_column(1,"PITCH",pitch,"",slot>=0?VAL(1u):C_DIM,-1,ICON_AUTO);
        draw_column(2,"LEN",length,"STEP",slot>=0?VAL(2u):C_DIM,-1,ICON_AUTO);
        draw_column(3,"FORCE",slot>=0?levels[(TSEL->step[ui.cursor].lvl>>(2u*slot))&3u]:"--",
                    TSEL->step[ui.cursor].flags&SF_ACCENT?"STEP ACC":"",slot>=0?VAL(3u):C_DIM,-1,ICON_AUTO);
        return;
    }
    if (cur_page()->scope == SC_STEP) {
        static const char *const TIME_N[3] = {"NOTE", "TIE", "REST"};
        step_t effective;
        note_effective(TSEL,ui.tie_start<trk_len(TSEL)?ui.tie_start:ui.cursor,&effective);
        const step_t *st = &effective;
        char u[8];
        if (st->n) {
            note_name(val, st->note[0]);
            u[0] = 0;
            if (st->n > 1) {
                str_cpy(u, "+", 8);
                fmt_int(u + 1, st->n - 1);
            }
        } else {
            str_cpy(val, "--", 12);
            u[0] = 0;
        }
        if (ui.tie_start<trk_len(TSEL) && st->n) str_cpy(u,"SRC",8);
        if (ui.tie_voice && ui.tie_voice<=st->n) {
            note_name(val,st->note[ui.tie_voice-1]);str_cpy(u,"SEL",8);
        }
        {
            static const char *const FLAG_N[4] = {"-", "ACC", "SLD", "A+S"};
            char sn[8], sl[8];
            fmt_int(sn, (int32_t)ui.cursor + 1);
            str_cpy(sl, "/", 8);
            fmt_int(sl + 1, TSEL->p[P_SLEN]);
            draw_column(0, "STEP", sn, sl, VAL(0u), -1, ICON_AUTO);
            draw_column(1, "NOTE", val, u, step_on(st) ? VAL(1u) : C_DIM, -1, ICON_AUTO);
            if (step_on(st) || st->time==ST_TIE) {
                char ln[8];
                uint32_t voice,first=note_edit_target(&voice);
                fmt_int(ln, (int32_t)note_voice_len(TSEL,first,voice));
                draw_column(2, "LEN", ln, "STEP", VAL(2u), -1, ICON_AUTO);
            } else {
                draw_column(2, "TIME", TIME_N[st->time % 3u], "", VAL(2u), -1, ICON_AUTO);
            }
            if (fm1_in.buttons&(1u<<panel.btn[B_OCTUP])) {
                step_t src;note_effective(TSEL,ui.tie_start,&src);
                str_cpy(val,"ALL",12);
                if(ui.tie_voice && ui.tie_voice<=src.n) note_name(val,src.note[ui.tie_voice-1]);
                draw_column(3,"VOICE",val,"",VAL(3u),-1,ICON_AUTO);
            } else {
                st=&TSEL->step[ui.cursor];
                draw_column(3, "FLAG", FLAG_N[(st->flags & SF_ACCENT ? 1u : 0u) | (st->flags & SF_SLIDE ? 2u : 0u)],
                        st->flags&SF_TIES?"TIE":"",VAL(3u), -1, ICON_AUTO);
            }
        }
        return;
    }
    for (c = 0; c < 4u; c++) {
        int16_t *vp;
        const param_desc_t *d = page_desc(cur_page(), c, &vp);
        if (cur_page()->graph == GR_SLOTS && c == 1u) {   /* 2.6: ERASE, lit while the slot holds a project */
            draw_column(1, "ERASE", "--", "", project_used((uint32_t)song.g[G_SLOT] - 1u) ? C_HI : C_DIM, -1, ICON_AUTO);
            continue;
        }
        if (!d || !d->label || d->label[0] == '-') {
            draw_column(c, "", "", "", C_HI, -1, ICON_AUTO);
            continue;
        }
        draw_parameter_column(c,d,*vp,0);
    }
}


/* the UI's timers, once a frame whatever the screen (a message, the BPM highlight, an armed GO, the
 * white value of the knob just turned) */
UI_SMALL static void ui_timers(void)
{
    if (ui.msg_t)
        ui.msg_t--;
    if (ui.bpm_t)
        ui.bpm_t--;
    if (ui.arm_t && !--ui.arm_t)
        ui.arm = 0;
    if (ui.hot_t)
        ui.hot_t--;
}

UI_SMALL static void vis_draw(void);                     /* ui_vis.c */
static int vis_shown(void);
static uint8_t vis_on, vis_shown_last, vis_name_t;   /* the visualiser (ui_vis.c): on, drawn last frame, its name's time */
UI_SMALL static void ui_draw(void)
{
    ui.frame++;
    /* STEP modifier help follows the physical hold; warnings retain their normal timeout. */
    int step_help = !memcmp(ui.msg, "HOLD 1 ", 7);
    uint32_t oct = fm1_in.buttons & ((1u << panel.btn[B_OCTUP]) | (1u << panel.btn[B_OCTDN]));
    if (oct && !ui.home && !ui.menu && ui.layer == LY_PLAY && !ui.hold_kind &&
        cur_page()->scope == SC_STEP && !is_drum(TSEL) && !song.rec && !rec_wait && !ft_on) {
        if (!ui.msg_t || step_help) {
            ui_message(oct & (1u << panel.btn[B_OCTUP]) ? "HOLD 1 TIE 4 VOICE" : "HOLD 1 REST 2 OCT 3 MOVE");
            ui.msg_t = 2;
        }
    } else if (step_help) ui.msg_t = 0;
    if (groove_preview.active && (!on_drum_page() || drum_page < 2 || song.sel != TRK_DRUM || ui.menu))
        groove_preview.active = 0;
    pads_tick();
    if (!vis_shown()) memset(vis_note_hits, 0, sizeof vis_note_hits);
    if (rec_go) {                                       /* the take started: say so */
        rec_go = 0;
        ui_message("RECORDING");
    }
    if (ft_bars) {                                      /* a free take closed: the loop it made */
        char m[24];
        uint32_t n = ft_bars;
        ft_bars = 0;
        if (n == 0xFFu) {
            ui_message("TAKE DROPPED");
        } else {
            str_cpy(m, n == 1u ? "LOOP 1 BAR " : n == 2u ? "LOOP 2 BARS " : "LOOP 4 BARS ", sizeof m);
            fmt_int(m + str_len(m), song.g[G_BPM]);
            str_cpy(m + str_len(m), " BPM", sizeof m - str_len(m));
            ui_message(m);
        }
    }
    if (er_flash) {                                     /* EDIT + key took something out */
        er_flash = 0;
        if (!ui.msg_t)
            ui_message("ERASED");
    }
    if (!ui.menu && (ui.layer != LY_PLAY || ui.hold_kind)) {   /* a layer held / a hold to confirm */
        if (ui.hold_kind)
            hold_screen_draw();
        else
            layer_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (layer_shown) {                                  /* back to the page */
        layer_shown = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
    if ((rec_wait || ft_on) && !ui.menu && !on_song_page()) {   /* armed / free take */
        rec_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (rec_shown) {                                    /* back to the page */
        rec_shown = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
#if FELUCCA_ARRANGER
    if (!ui.menu && on_song_page()) {
        song_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
#endif
    if (ui.menu) {
        draw_menu();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (vis_shown()) {                                  /* the visualiser over the TRACKS screen */
        vis_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (vis_shown_last) {                               /* back from it */
        vis_shown_last = 0;
        vis_on = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
    if(actions_on()) {actions_draw();ui_timers();ui.force=0;return;}
    if (!ui.home && cur_page()->scope == SC_TRK) {
        studio_tracks_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (on_drum_page()) {
        drum_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (sequence_browser) {
        sequence_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    cursor_fix();
    if (ui.force)
        draw_frame();
    felucca_dbg.stage = 3;
    draw_head();
    felucca_dbg.stage = 4;
    draw_columns();
    felucca_dbg.stage = 5;
    draw_graph();
    ui_timers();
    felucca_dbg.stage = 6;
    draw_foot();
    ui.force = 0;
}

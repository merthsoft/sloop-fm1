/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* SLOOP menu (HOME held), in sections as the pages are (2.4): SCREEN (COLOR, ZOOM, KNOB ACCEL (2.7)), LIGHTS (LIGHTS, KEYS, NOTES),
 * AUDIO (LOWCUT, USB AUDIO, USB SERIAL (FELUCCA_CDC builds only), USB RATE (2.7 beta)), SYSTEM (2ND CORE (dualcore), HARDWARE CALIBRATION, LEARN CLEAR (2.6), ABOUT).
 * SELECT goes to the section before / after (stopping at the ends), KNOB 1..4 set the section's settings in order (the knob's colour marks
 * its row), PRESETS moves the cursor; OCT+ steps the cursor's setting round or opens it (CALIBRATION, ABOUT),
 * OCT- closes (from ABOUT: back to the section). */
/* ------------------------------------------------------------ menu --- */
/* (2.7: USB SERIAL only in a build with the serial console, FELUCCA_CDC=1: the FM-1's build has none) */
enum { MI_COLOR, MI_ZOOM, MI_ACCEL, MI_HELP, MI_LIGHTS, MI_KEYS, MI_NOTES, MI_SCALELEVEL, MI_LOWCUT, MI_USB,
#if FELUCCA_CDC
       MI_SERIAL,
#endif
       MI_RATE, MI_BUDGET, MI_RESO, MI_FADE, MI_MIDISCALE, MI_SWING, MI_ADVANCE, MI_PREVIEW, MI_CPU1, MI_PANEL, MI_LCLEAR, MI_ABOUT, MI_COUNT };
static const char *const MI_NAME[MI_COUNT] = {"COLOR", "ZOOM", "KNOB ACCEL", "PAGE HELP", "LIGHTS", "KEYS", "NOTES", "SCALE LEVEL", "SPEAKER LOWCUT", "USB AUDIO",
#if FELUCCA_CDC
                                              "USB SERIAL",
#endif
                                              "USB RATE", "VOICE BUDGET", "FILTER RESO", "PUNCH FADE", "MIDI SCALE", "SWING GRID", "STEP ADVANCE", "DRUM PREVIEW", "2ND CORE", "HARDWARE CALIBRATION", "MIDI LEARN CLEAR", "ABOUT"};
enum { MS_SCREEN, MS_LIGHTS, MS_AUDIO, MS_FX, MS_MIDI, MS_TIMING, MS_SEQ, MS_SYSTEM, MS_COUNT };
static const char *const MS_NAME[MS_COUNT] = {"SCREEN", "LIGHTS", "AUDIO", "FX", "MIDI", "TIMING", "SEQ", "SYSTEM"};
static const uint8_t MS_FIRST[MS_COUNT + 1] = {MI_COLOR, MI_LIGHTS, MI_LOWCUT, MI_RESO, MI_MIDISCALE, MI_SWING, MI_ADVANCE, MI_CPU1, MI_COUNT};

UI_SMALL static int mi_cpu1_have(void)                      /* the second core is running (cpu1.c; the host tests have no cpu1.c) */
{
#if FELUCCA_CPU1
    return cpu1_state == CPU1_OK;
#else
    return 0;
#endif
}
static const char *const LIGHTS_NAME[LIGHTS_N] = {"OFF", "LOW", "MID", "HIGH"};   /* every button lit, the labels readable */
static const char *const KEYS_NAME[KEYS_N] = {"OFF", "C KEYS", "WHITE KEYS", "ALL KEYS"};      /* keys lit too, at the LIGHTS level */
static const char *const MS_TAB[MS_COUNT] = {"SCR", "LGT", "AUD", "FX", "MIDI", "TIME", "SEQ", "SYS"};
#define MI_Y0 26                                   /* the first row, under the section tabs */
#define MI_DY 50                                   /* a row: the label, the value in large type */

UI_SMALL static uint32_t mi_sec(uint32_t i)                 /* the section of item i */
{
    uint32_t k = 0;
    while (k + 1u < MS_COUNT && i >= MS_FIRST[k + 1u])
        k++;
    return k;
}
UI_SMALL static uint32_t mi_row(uint32_t i) { return i - MS_FIRST[mi_sec(i)]; }   /* its row: KNOB 1 + row sets it */

/* item i's value as shown, and its colour */
UI_SMALL static const char *mi_value(uint32_t i, uint16_t *c)
{
    *c = C_HI;
    switch (i) {
    case MI_COLOR:                                    /* (2.7) your theme, by its name */
        if (theme_shown()) {
            static char b[9];
            uint32_t k;
            for (k = 0; k < 8u && theme.name[k]; k++)
                b[k] = theme.name[k];
            b[k] = 0;
            return k ? b : "MY THEME";
        }
        return PALETTES[settings.palette % NPALETTES].name;
    case MI_ZOOM: return settings.zoom ? "ON" : "OFF";
    case MI_ACCEL: return knob_accel_off ? "OFF" : "ON";   /* (2.7) the knobs speed up with a fast turn */
    case MI_SCALELEVEL: return scale_light_level?LIGHTS_NAME[scale_light_level]:"DIM";
    case MI_LIGHTS: return LIGHTS_NAME[lights_lvl % LIGHTS_N];
    case MI_KEYS:
        *c = lights_lvl ? C_HI : C_DIM;               /* (needs LIGHTS) */
        return KEYS_NAME[lights_keys % KEYS_N];
    case MI_SWING: return swing_eighth?"1/8":"STEP";
    case MI_BUDGET: {
        static const char names[][8] = {"8 / 12", "10 / 15", "12 / 18", "14 / 21", "16 / 24"};
        return names[voice_expand];
    }
    case MI_ADVANCE: return step_adv_off ? "OFF" : "ON";
    case MI_PREVIEW: return drum_preview ? "ON" : "OFF";
    case MI_RESO:
    case MI_FADE: {
        static char b[4];
        fmt_int(b, i == MI_RESO ? flt_reso : fxfade);
        return b;
    }
    case MI_MIDISCALE: return midi_scale ? "KEYBOARD" : "OFF";
    case MI_NOTES: return lights_notes ? "ON" : "OFF";
    case MI_LOWCUT: return settings.lowcut ? "ON" : "OFF";
    case MI_USB: return usb_full ? "FULL" : "MASTER";
#if FELUCCA_CDC
    case MI_SERIAL: return usb_serial ? "ON" : "OFF";
#endif
    case MI_RATE: {                                   /* (2.7 beta) the rates the USB audio input offers (usb.c) */
        static const char *const R[3] = {"AUTO", "44.1 KHZ", "48 KHZ"};
        return R[usb_rate % 3u];
    }
    case MI_CPU1:                                     /* the render on both cores (fx.c); live, the sound is the same */
        *c = mi_cpu1_have() ? C_HI : C_DIM;
        return cpu1_off ? "OFF" : "ON";
    case MI_LCLEAR: {                                 /* (2.6) the CCs MIDI LEARN holds; OCT+ clears them */
        static char b[8];
        uint32_t n = ml_count();
        if (!n) {
            *c = C_DIM;
            return "NONE";
        }
        fmt_int(b, (int32_t)n);
        str_cpy(b + str_len(b), " CC", 4);
        return b;
    }
    default:
        *c = C_DIM;
        return "";                                    /* (an action: OCT+ opens it) */
    }
}

/* Help uses the active page descriptors, not a second control state machine. */
UI_SMALL static void draw_page_help(void)
{
    const page_t *pg=cur_page();
    const char *labels[4];
    for(uint32_t k=0;k<4;k++) {
        int16_t *v;const param_desc_t *d=ui.home?home_param(k,&v):page_desc(pg,k,&v);
        labels[k]=d?d->label:"--";
        if(pg->fam==FAM_SCL&&pg->id[k]==P_AHOLD) labels[k]="LATCH";
    }
    if(pg->graph==GR_BROWSE) {
        labels[0]="SOUND";labels[1]="KIND";labels[2]="KEEP / UNDO";labels[3]="FAV";
    } else if(pg->scope==SC_STEP) {
        labels[0]="STEP";labels[1]="NOTE";labels[2]="LENGTH";labels[3]=ui.tie_voice?"VOICE":"FLAG";
    } else if(pg->scope==SC_TRK) {
        labels[0]="SWING";labels[1]="LEVEL";labels[2]="STEPS";labels[3]="PAN";
    } else if(pg->scope==SC_SONG) {
        labels[0]="ENTRY";labels[1]="SECT";labels[2]="BARS";labels[3]="LENGTH";
    }
    for(uint32_t pass=0;pass<2;pass++) {
        cv_band(120);cv_oy=-(int32_t)pass*120;
        cv_text_c_white(4,2,"PAGE HELP");
        cv_text_s(4,22,ui.home?"HOME":page_title(pg),TE_COL[song.sel]);
        for(uint32_t k=0;k<4;k++) {
            char n[4]={'K',(char)('1'+k),':',0};
            cv_text_s(4,46+(int32_t)k*20,n,TE_COL[k]);
            cv_text_te_g4(36,46+(int32_t)k*20,labels[k]);
        }
        if(pg->scope==SC_EVOLVE) {
            cv_text_te_g4(4,132,"DRONE: HOLD / MOVE");
            cv_text_te_g4(4,152,"RANGE: SCALE STEPS");
            cv_text_te_g4(4,172,"ONE NOTE PER CYCLE");
            cv_text_te_g4(4,192,"RUNTIME / NOT SAVED");
        } else {
        cv_text_te_g4(4,132,"SELECT: PAGE");
        cv_text_te_g4(4,152,"PRESETS");
        cv_text_te_g4(148,152,"ALGORITHM");
        cv_text_te_g4(4,172,"SOUND");
        cv_text_te_g4(148,172,"TRACK");
        cv_text_te_g4(4,192,page_hold_hint(pg));
        }
        cv_text_c_white(4,222,"OCT-: BACK");
        cv_oy=0;cv_band_blit(pass*120);
    }
}
#if FELUCCA_AUDIO_DIAG
/* A snapshot on opening ABOUT, not a continuously redrawn page that changes load.
 * OCT+ refreshes; counters last until power-off. See audio.c for trial policy. */
UI_SMALL static void draw_audio_diag(void)
{
    static const char *const labels[] = {"STEALS", "SHEDS", "RESTORE", "MAX US", "LATE", "C1MASK"};
    uint32_t v[6] = {voice_kills, shed_count, drone_restores, audio_wall_max,
        felucca_dbg.late, cpu1_last_mask};
    cv_band(H_HEAD);
    cv_text_c_white(4,1,"DIAG " FELUCCA_VERSION);cv_band_blit(Y_HEAD);
    for(uint32_t pass=0;pass<2;pass++) {
        cv_band(pass?95u:124u);cv_oy=pass?-124:0;
        for(uint32_t i=0;i<NELEM(labels);i++) {
            char b[40];fmt_int(b,(int32_t)v[i]);
            cv_text_c_dim(4,(int32_t)i*18,labels[i]);
            cv_text_c_white(236-text_w(&FONT_S,b),(int32_t)i*18,b);
        }
        cv_oy=0;cv_band_blit(H_HEAD+1+pass*124u);
    }
}
#endif
UI_SMALL static void draw_menu(void)
{
    uint32_t i, pass, sec = mi_sec(ui.menu_sel % MI_COUNT);
    if(ui.menu==3) { if(ui.force) draw_page_help();return; }
#if FELUCCA_AUDIO_DIAG
    if(ui.menu==2) {if(ui.force)draw_audio_diag();return;}
#endif
    uint32_t sig = ui.menu * 7u + ui.menu_sel * 131u + settings.palette * 1009u + settings.lowcut * 7919u +
                   settings.zoom * 104729u + lights_lvl * 1299709u + lights_keys * 15485863u + ml_count() * 2654435u +
                   lights_notes * 32452843u + usb_full * 49979687u + usb_serial * 86028121u + usb_rate * 15486719u + theme_shown() * 32452867u + theme.crc + cpu1_off * 122949829u +
                   ((uint32_t)scale_light_level << 5) + knob_accel_off * 179424673u + midi_scale * 961748941u + swing_eighth * 1234567u +
                   step_adv_off * 193u + drum_preview * 65537u + flt_reso * 8191u + fxfade * 257u + voice_expand;
    if (!ui.force && sig == ui.menu_sig)
        return;
    ui.menu_sig = sig;
    if (ui.force)                                   /* head + rule + two bands cover rows 0..229 */
        lcd_fill(0, H_HEAD + 1 + 124 + 95, 240, 240 - (H_HEAD + 1 + 124 + 95), C_BLACK);
    cv_band(H_HEAD);
    cv_text_c_hi(4, 1, ui.menu == 2 ? "ABOUT" : "MENU");
    if (ui.menu == 2)
        cv_text_c_gray(236 - text_w(&FONT_S, FELUCCA_VERSION " MERTHSOFT"), 1, FELUCCA_VERSION " MERTHSOFT");
    if (ui.menu != 2) {                             /* the section and its number, as a page's "ENV DEST 2/2" */
        char t[16];
        str_cpy(t, MS_NAME[sec], sizeof t);
        str_cpy(t + str_len(t), " ", 2);
        fmt_int(t + str_len(t), (int32_t)sec + 1);
        str_cpy(t + str_len(t), "/", 2);
        fmt_int(t + str_len(t), MS_COUNT);
        cv_text_c_gray(236 - text_w(&FONT_S, t), 1, t);
    }
    cv_band_blit(Y_HEAD);
    lcd_fill(0, H_HEAD, 240, 1, C_LINE);
    for (pass = 0; pass < 2u; pass++) {             /* the canvas holds 124 rows: draw in two bands */
        cv_band(pass ? 95u : 124u);
        cv_oy = pass ? -124 : 0;
        if (ui.menu == 2) {
#if !FELUCCA_AUDIO_DIAG
            /* Same credits/positions; one draw path avoids repeated call setup. */
            static const char *const credits[]={MERTHSLOOP_NAME,"BASED ON SLOOP / FELUCCA",
                "LEO KUROSHITA","H\xDCGELTON INSTRUMENTS","HUGELTON.COM","GPL-3.0, NO WARRANTY",
                "GITHUB.COM/MERTHSOFT/","SLOOP-FM1","FONT: TERMINUS (OFL)",
                "SAMPLES: VERSILIAN (CC0)","+ SONIC PI (CC0)","PHASE: CRISPYZEBRA (GPL)","VOICE: REF. KLATTSCH (MIT)"};
            for(uint32_t line=0;line<NELEM(credits);line++) {
                uint16_t color=!line?C_WHITE:line>=8?TE_G4:(210u>>line)&1u?C_AMB:C_HI;
                int32_t y=line?8+(int32_t)line*16:4;
                int32_t x=cv_text_s(4,y,credits[line],color);
                if(line==2)cv_text_c_amb(x+8,y,"@KUROGEDELIC");
            }
#if FELUCCA_CPU1
            char sv[12];cpu1_supplies(sv);
            cv_text_c_gray(236-text_w(&FONT_S,sv),72,sv);
            cv_text_c_amb(236-text_w(&FONT_S,cpu1_label()),88,cpu1_label());
#endif
#endif
        } else {
            int32_t x = 4, total = 0;
            for (i = 0; i < MS_COUNT; i++) total += text_w(&FONT_S, MS_TAB[i]);
            int32_t gap = (232 - total) / (MS_COUNT - 1);
            for (i = 0; i < MS_COUNT; i++) {        /* the sections as tabs: this one lit */
                int32_t w = text_w(&FONT_S, MS_TAB[i]);
                if (i == sec)
                    cv_rect_c_line(x - 2, 3, w + 4, 18);
                cv_text_s(x, 4, MS_TAB[i], i == sec ? C_WHITE : C_DIM);
                x += w + gap;
            }
            for (i = MS_FIRST[sec]; i < MS_FIRST[sec + 1u]; i++) {
                uint32_t r = i - MS_FIRST[sec];
                int32_t y = MI_Y0 + (int32_t)r * MI_DY, xv;
                int cur = i == ui.menu_sel;
                uint16_t vc;
                const char *v = mi_value(i, &vc);
                char k[4] = {'K', (char)('1' + r), 0, 0};
                cv_rect(4, y + 2, 4, MI_DY - 8, cur ? C_WHITE : TE_COL[r & 3u]);   /* the knob's colour */
                xv = cv_text_s(14, y, k, TE_COL[r & 3u]);
                cv_text_s(xv + 6, y, MI_NAME[i], cur ? C_WHITE : C_GRAY);
                if (v[0] && text_w(&FONT_L, v) <= (i == MI_COLOR ? 144 : 222)) {
                    xv = cv_text(14, y + 16, &FONT_L, v, cur && vc == C_HI ? C_WHITE : vc);
                } else if (v[0]) {                      /* (too wide for the large type: WHITE KEYS) */
                    xv = cv_text_s(14, y + 24, v, cur && vc == C_HI ? C_WHITE : vc);
                } else {
                    xv = cv_text_s(14, y + 24, "OCT+ OPENS", cur ? C_WHITE : C_DIM);
                }
                if (i == MI_COLOR) {                    /* the palette's colours */
                    uint32_t q;
                    for (q = 0; q < 5u; q++)
                        cv_rect(166 + (int32_t)q * 14, y + 28, 10, 10, pal[q]);
                }
                if (i == MI_RATE && usb_rate != usb_rate_now())
                    cv_text_c_amb(xv + 8, y + 30, "RESTART");   /* (usb.c: at the next start) */
#if FELUCCA_CDC
                if (i == MI_SERIAL && usb_serial != usb_cdc_now())
                    cv_text_c_amb(xv + 8, y + 30, "RESTART");   /* (usb.c: at the next start) */
#endif
                if (i == MI_CPU1 && !mi_cpu1_have())
                    cv_text_c_amb(xv + 8, y + 30, "NOT RUNNING");   /* (cpu1.c: ABOUT says 1 CORE) */
            }
            if (MS_FIRST[sec + 1u] - MS_FIRST[sec] <= 3u) {   /* (SYSTEM has four rows with 2ND CORE: its fourth
                                                               * row, ABOUT, takes the help's place) */
                cv_text_c_dim(4, MI_Y0 + 3 * MI_DY + 6, "SELECT SECTION  KNOB SETS");
                cv_text_c_dim(4, MI_Y0 + 3 * MI_DY + 22, "OCT+ OK   OCT- CLOSE");   /* (rows .. 218) */
            }
        }
        cv_oy = 0;
        cv_band_blit(H_HEAD + 1 + pass * 124u);
    }
}

UI_SMALL static void enc_drop(void)                             /* knob turns nobody takes */
{
    uint32_t k;
    for (k = 0; k < NE; k++)
        panel_enc(k);
}

UI_SMALL static void menu_close(void)
{
    if (song.playing || transport_req)
        settings_later = 1;                            /* (a flash write stops the audio: once stopped) */
    else
        settings_save();                               /* palette / panel table, if changed */
    ui.menu = 0;
    ui.force = 1;
    go_home();
}

/* Shared byte switches keep identical menu direction/toggle rules in one place. */
UI_SMALL static void mi_toggle(uint8_t *value,int32_t direction)
{*value=(uint8_t)(direction>0?1u:direction<0?0u:!*value);}

UI_SMALL static void mi_step(uint8_t *v,int32_t s,uint32_t n)
{
    if(s>0 && *v+1u<n) (*v)++;
    else if(s<0 && *v) (*v)--;
    else if(!s) *v=(uint8_t)((*v+1u)%n);
}

/* item i: a knob turned s (> 0 right, < 0 left), or 0: OCT+ (steps round, toggles, or opens) */
UI_SMALL static void mi_set(uint32_t i, int32_t s)
{
    switch (i) {
    case MI_COLOR: {                                   /* both ways round; OCT+ the next. 2.7: your theme after MONO
                                                        * (the palette number stays the last built-in one: an older
                                                        * SLOOP shows that) */
        uint32_t n = NPALETTES + (theme_valid ? 1u : 0u), at = theme_shown() ? NPALETTES : settings.palette % NPALETTES;
        at = (at + (s < 0 ? n - 1u : 1u)) % n;
        theme_on = (uint8_t)(at == NPALETTES);
        if (!theme_on)
            settings.palette = at;
        palette_set(settings.palette);
        ui.force = 1;
        break;
    }
    case MI_ACCEL:                                     /* (2.7) right ON, left OFF; OCT+ toggles (ui_input.c accel) */
        mi_toggle(&knob_accel_off,-s);
        break;
    case MI_ZOOM:
    case MI_LOWCUT: {                                  /* right ON, left OFF; OCT+ toggles */
        uint32_t *v = i == MI_LOWCUT ? &settings.lowcut : &settings.zoom;
        *v = s > 0 ? 1u : s < 0 ? 0u : !*v;
        fx_lowcut = (uint8_t)(settings.lowcut != 0);
        break;
    }
    case MI_USB:                                       /* right FULL, left MASTER */
        mi_toggle(&usb_full,s);
        break;
#if FELUCCA_CDC
    case MI_SERIAL:
        mi_toggle(&usb_serial,s);
        break;
#endif
    case MI_RATE:
    case MI_BUDGET: {
        uint8_t *v = i == MI_RATE ? &usb_rate : &voice_expand;
        uint32_t last = i == MI_RATE ? 2u : 4u;
        mi_step(v,s,last+1u);
        break;
    }
    case MI_CPU1:                                      /* right ON, left OFF; at once (the next block renders so) */
        mi_toggle(&cpu1_off,-s);
        cpu1_units = cpu1_off ? 0 : -1;
        if (!cpu1_off && !cpu1_on())                   /* 2.7: core 1 starts only at power-on */
            ui_message("ON AT NEXT POWER-ON");
        break;
    case MI_MIDISCALE:
    case MI_SWING: {
        uint8_t *v = i == MI_SWING ? &swing_eighth : &midi_scale;
        mi_toggle(v,s);
        settings_later = 1;
        break;
    }
    case MI_ADVANCE:
    case MI_PREVIEW:
        mi_toggle(i == MI_ADVANCE ? &step_adv_off : &drum_preview, i == MI_ADVANCE ? -s : s);
        settings_later = 1;
        break;
    case MI_RESO:
    case MI_FADE: {
        uint8_t *v = i == MI_RESO ? &flt_reso : &fxfade;
        uint32_t max = i == MI_RESO ? 10u : 15u;
        *v = s ? (uint8_t)clamp((int32_t)*v + (s > 0 ? 1 : -1), 0, (int32_t)max) : (uint8_t)((*v + 1u) % (max + 1u));
        settings_later = 1;
        break;
    }
    case MI_NOTES:
        mi_toggle(&lights_notes,s);
        break;
    case MI_SCALELEVEL:
    case MI_LIGHTS:
    case MI_KEYS: {                                    /* brighter / dimmer (stops at the ends); OCT+ steps round */
        uint8_t *v = i == MI_LIGHTS ? &lights_lvl : i == MI_KEYS ? &lights_keys : &scale_light_level;
        uint32_t n = i == MI_LIGHTS ? LIGHTS_N : KEYS_N;
        mi_step(v,s,n);
        if (i == MI_KEYS && lights_keys && !lights_lvl)
            lights_lvl = LIGHTS_LOW;                   /* keys lit need a level: the lowest */
        break;
    }
    case MI_LCLEAR:                                    /* (2.6) every learned CC: OCT+ */
        if (!s)
            ui_message(ml_clear_all() ? "LEARN CLEARED" : "NOTHING LEARNED");
        break;
    case MI_PANEL:                                     /* actions: OCT+ only */
        if (!s) {
            panel_setup();
            ui.force = 1;
        }
        break;
    case MI_HELP:
    case MI_ABOUT:
        if (!s) {
            ui.menu = i==MI_HELP ? 3 : 2;
            ui.force = 1;
        }
        break;
    default:
        break;
    }
}

/* menu: SELECT the section, KNOB 1..3 its settings, PRESETS the cursor, OCT+ ok, OCT- close (ABOUT -> the section) */
UI_SMALL static void menu_input(uint32_t pressed)
{
    int32_t s;
    uint32_t k, ok = (pressed >> panel.btn[B_OCTUP]) & 1u, back = (pressed >> panel.btn[B_OCTDN]) & 1u;
    uint32_t sel = ui.menu_sel % MI_COUNT, sec = mi_sec(sel), n = MS_FIRST[sec + 1u] - MS_FIRST[sec];
    if (back) {
        if (ui.menu >= 2)
            ui.menu = 1, ui.force = 1;
        else
            menu_close();
        return;
    }
    if (ui.menu != 1) {
#if FELUCCA_AUDIO_DIAG
        if(ui.menu==2 && ok) ui.force=1;
#endif
        enc_drop();
        return;
    }
    if ((s = panel_enc(EN_SELECT)) != 0) {             /* the section before / after (stops at the ends) */
        int32_t to = clamp((int32_t)sec + (s > 0 ? 1 : -1), 0, MS_COUNT - 1);
        if ((uint32_t)to != sec) {
            ui.menu_sel = MS_FIRST[to];
            ui.force = 1;
        }
        enc_drop();
        return;
    }
    if ((s = panel_enc(EN_PRESET)) != 0)               /* the cursor, round the section's rows */
        ui.menu_sel = (uint8_t)(MS_FIRST[sec] + (sel - MS_FIRST[sec] + (s > 0 ? 1u : n - 1u)) % n);
    for (k = 0; k < 4u; k++)
        if ((s = panel_enc(EN_K1 + k)) != 0 && k < n) {
            ui.menu_sel = (uint8_t)(MS_FIRST[sec] + k);   /* (the cursor follows the knob turned) */
            mi_set(MS_FIRST[sec] + k, s);
        }
    if (ok)
        mi_set(ui.menu_sel % MI_COUNT, 0);
    enc_drop();                                        /* swallow the rest while the menu is up */
}

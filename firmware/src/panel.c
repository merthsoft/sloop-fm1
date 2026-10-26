/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Physical panel: which matrix button / encoder carries which printed label.
 * The default table can be overridden by HARDWARE CALIBRATION (hold OCT- and
 * OCT+ while powering on), which asks for each label in turn. The learned
 * table lives in .noinit and, with FELUCCA_FLASH, in flash with the settings
 * (project.c). */
/* (B_SCL: the button printed SEL, the key / scale button between FX and ENV) */
enum { B_FX, B_SCL, B_ENV, B_LFO, B_EDIT, B_GLO, B_HOME, B_SAVE, B_ARP, B_SEQ, B_PLAY, B_REC,
       B_OCTDN, B_OCTUP, NB };
enum { EN_SELECT, EN_ALGO, EN_PRESET, EN_K1, EN_K2, EN_K3, EN_K4, NE };
static const char *const B_NAME[NB] = {"FX", "SEL", "ENV", "LFO", "EDIT", "GLO", "HOME", "SAVE",
                                        "ARP", "SEQ", "PLAY", "REC", "OCT-", "OCT+"};
static const char *const E_NAME[NE] = {"SELECT", "ALGORITHM", "PRESETS", "KNOB 1", "KNOB 2",
                                        "KNOB 3", "KNOB 4"};
#define PANEL_MAGIC 0x50414E35u          /* "PAN5": bump when PANEL_DEFAULT changes */

typedef struct {
    uint32_t magic;
    uint8_t btn[NB];             /* matrix button id (0..13) per label */
    uint8_t enc[NE];             /* matrix encoder (0..6) per role */
    int8_t dir[NE];              /* +1 / -1 so that clockwise is + */
} panel_t;
panel_t panel __attribute__((section(".noinit")));

static const panel_t PANEL_DEFAULT = {
    PANEL_MAGIC,
    {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1},   /* PLAY = 12, REC = 13 */
    {0, 1, 6, 2, 3, 4, 5},                           /* SELECT = enc 0, ALGORITHM = enc 1, PRESETS = enc 6 */
    {1, 1, 1, 1, 1, 1, 1},
};

UI_SMALL static void panel_init(void)                     /* also after a flash load: ids are used as array indexes and shifts */
{
    uint32_t i, ok = panel.magic == PANEL_MAGIC;
    for (i = 0; ok && i < NB; i++)
        ok = panel.btn[i] < 14u;
    for (i = 0; ok && i < NE; i++)
        ok = panel.enc[i] < 7u && (panel.dir[i] == 1 || panel.dir[i] == -1);
    if (!ok)
        panel = PANEL_DEFAULT;
}

UI_SMALL static uint32_t panel_btn_of(uint32_t matrix_id)        /* label of a matrix button, NB if none */
{
    uint32_t b;
    for (b = 0; b < NB; b++)
        if (panel.btn[b] == matrix_id)
            return b;
    return NB;
}

UI_SMALL static void panel_led(uint32_t label, int on) { fm1_led_key(panel.btn[label], on); }

/* steps of a role, + = clockwise */
static uint32_t ui_input_ms;                    /* the last button, key or knob turn (ui_input.c; autosave) */
/* the knobs (roles, bit per EN_*) not to be read again in this UI pass (ui_input: a layer took them, #102 of
 * Felucca): panel_enc returns 0 and leaves their detents for the next pass */
static uint32_t enc_hold;
UI_SMALL static int32_t panel_enc(uint32_t role)
{
    int32_t s;
    if ((enc_hold >> role) & 1u)
        return 0;
    s = fm1_enc_take(panel.enc[role]) * panel.dir[role];
    if (s)
        ui_input_ms = fm1_ms;                      /* a knob turning is not idle either: autosave waits */
    return s;
}

/* user settings that survive a reset */
#define SETTINGS_MAGIC 0x53455433u              /* "SET3" */
struct { uint32_t magic, palette, lowcut, zoom; } settings __attribute__((section(".noinit")));

/* the lights (menu LIGHTS / KEYS / NOTES), for playing in the dark: kept in flash with the settings
 * (project.c persist_t.lights), read at every boot; not in .noinit, so nothing there moves. The same
 * word keeps SYNC and the REC screen's MODE / START (seq.c rec_tempo, rec_count) */
enum { LIGHTS_OFF, LIGHTS_LOW, LIGHTS_MID, LIGHTS_HIGH, LIGHTS_N };
enum { KEYS_OFF, KEYS_C, KEYS_WHITE, KEYS_ALL, KEYS_N };   /* (ALL: SLOOP 2.4; an older SLOOP reads it as OFF) */
static uint8_t lights_lvl, lights_keys;
static uint8_t lights_notes;                   /* 1: on a synth track the sounding notes light their keys */
static uint8_t scale_light_level;              /* 0 DIM (legacy), 1 LOW, 2 MID, 3 HIGH */
static uint8_t lights_scale;                   /* SEL + OCT+: persistent scale guide, fork side record */
static uint8_t lights_sync;                    /* GLO > SYSTEM > SYNC (G_SYNC), kept here: 0 INT, 1 USB, 2 TRS */
static uint8_t lights_mout;                    /* GLO > SYSTEM > MIDI (G_MIDI): 1 = the sequencer goes to MIDI OUT too */
static uint8_t lights_min;                     /* GLO > SYSTEM > IN (G_ROUTE): 0 NOTES, 1 CLOCK only, 2..17 (2.6) CH 1..16 only */
/* 2.6: GLO > MIDI CH1 CH2 CH3, GLO > DRUMS > CH, GLO > MIDI > OTHER: settings of the FM-1 (kept with the MIDI LEARN map
 * after the SYN kits: project.c midi_part), mirrored in song.g (felucca_init) */
#define MIDI_SET_DEF {1, 2, 3, 10, 0, 2, 0, 0, 0, 0}   /* CH1 CH2 CH3, drums, OTHER, (2.6) BEND, (2.7) FX CH, DR MAP, REMOTE,
                                                     * CLK OUT */
static uint8_t midi_set[10] = MIDI_SET_DEF;
static const uint16_t LIGHTS_NS[LIGHTS_N] = {0u, 500u, 1000u, 2000u};   /* the backlight pulse a frame (ns): a lit
                                                * LED ~95 us, the glow (landmarks) 4 us (fm1_input.h) */
static uint8_t usb_serial;                      /* menu USB SERIAL: 1 = the serial console presented (usb.c) */
static uint8_t usb_rate;                        /* menu USB RATE (2.7 beta): 0 AUTO, 1 44.1, 2 48 kHz (usb.c usb_rate_mode, at the next start) */
static uint8_t cpu1_off;                        /* menu 2ND CORE: 1 = OFF, every render unit on core 0 (cpu1.h cpu1_units);
                                                 * 2.7: ON by default (bit 26 set = OFF); core 1 starts at power-on only when ON */
static uint8_t vis_style;                       /* compact visualizer index (ui_vis.c) */
static uint8_t knob_accel_off;                  /* menu KNOB ACCEL (2.7): 1 = OFF, one value a detent however fast (ui_input.c accel) */
enum { VIS_SCOPE, VIS_SPECTRUM, VIS_SPECTROGRAM, VIS_METERS, VIS_TRAILS, VIS_GROOVE, VIS_JOURNEY, VIS_STYLE_COUNT };
/* Stable fork-side IDs survive pruning/reordering; removed IDs fall back to scope. */
static const uint8_t VIS_SAVE_ID[VIS_STYLE_COUNT]={0,1,2,4,9,10,12};
UI_SMALL static uint8_t vis_from_saved(uint32_t id)
{
    for(uint32_t i=0;i<VIS_STYLE_COUNT;i++) if(VIS_SAVE_ID[i]==id) return (uint8_t)i;
    return VIS_SCOPE;
}
static uint8_t vis_upstream_id(void) { return vis_style<=VIS_METERS?VIS_SAVE_ID[vis_style]:0u; }
UI_SMALL static uint32_t fork_settings_word(void)
{
    return (uint32_t)(lights_scale != 0u) | (uint32_t)(midi_scale != 0u) << 1 |
           (uint32_t)(swing_eighth != 0u) << 2 | (uint32_t)scale_light_level << 3 | (uint32_t)voice_expand << 5 | (uint32_t)VIS_SAVE_ID[vis_style % VIS_STYLE_COUNT] << 8;
}
UI_SMALL static void fork_settings_from_word(uint32_t w)
{
    scale_light_level = (w >> 3) & 3u;
    /* Fork-only bits 5..7: old records select the original budget. */
    voice_expand = (uint8_t)clamp((w >> 5) & 7u, 0, 4);
    lights_scale = w & 1u;
    midi_scale = (w >> 1) & 1u;
    swing_eighth = (w >> 2) & 1u;
    vis_style = vis_from_saved((w >> 8) & 255u);
}
UI_SMALL static uint32_t lights_word(void)
{
    return (uint32_t)lights_lvl | (uint32_t)lights_keys << 4 | (uint32_t)(lights_notes != 0u) << 8 |
           (uint32_t)(rec_tempo != 0u) << 9 | (uint32_t)(rec_count != 0u) << 10 | (uint32_t)(usb_full != 0u) << 11 |
           (uint32_t)(lights_sync % 3u) << 12 | (uint32_t)(lights_mout != 0u) << 14 | (uint32_t)(lights_min == 1u) << 15 |
           (uint32_t)(usb_serial != 0u) << 16 | (uint32_t)vis_upstream_id() << 17 |
           (uint32_t)(lights_min >= 2u && lights_min <= 17u ? lights_min - 1u : 0u) << 21 |   /* 2.6: CH n (n 1..16; 0 none) */
           (uint32_t)(cpu1_off != 0u) << 26 |          /* 2ND CORE OFF (2.7: ON by default; 2.6 took 21..25) */
           (uint32_t)(knob_accel_off != 0u) << 27 |    /* KNOB ACCEL OFF (2.7; 0 = ON, as before 2.7) */
           (uint32_t)(usb_rate % 3u) << 28 |           /* USB RATE (2.7 beta; 0 = AUTO, as before) */
           (uint32_t)(theme_on != 0u) << 30 |
           (uint32_t)(rec_quant_off != 0u) << 31;      /* REC screen QUANT OFF (2.7, seq.c; 0 = ON, as before) */           /* COLOR on your theme (2.7; gfx.c; an older SLOOP shows palette) */
}
UI_SMALL static void lights_from_word(uint32_t w)
{
    lights_lvl = (uint8_t)((w & 15u) < LIGHTS_N ? (w & 15u) : LIGHTS_OFF);
    lights_keys = (uint8_t)(((w >> 4) & 15u) < KEYS_N ? ((w >> 4) & 15u) : KEYS_OFF);
    lights_notes = (uint8_t)((w >> 8) & 1u);
    lights_scale = midi_scale = swing_eighth = scale_light_level = voice_expand = 0;
    rec_tempo = (uint8_t)((w >> 9) & 1u);       /* the REC screen: MODE, START (seq.c) */
    rec_count = (uint8_t)((w >> 10) & 1u);
    usb_full = (uint8_t)((w >> 11) & 1u);       /* menu USB AUDIO (fx.c) */
    lights_sync = (uint8_t)(((w >> 12) & 3u) % 3u);
    lights_mout = (uint8_t)((w >> 14) & 1u);    /* GLO > SYSTEM > MIDI (seq.c) */
    lights_min = (uint8_t)((w >> 15) & 1u);     /* GLO > SYSTEM > IN (seq.c) */
    if (((w >> 21) & 31u) >= 1u && ((w >> 21) & 31u) <= 16u)
        lights_min = (uint8_t)(((w >> 21) & 31u) + 1u);
    usb_serial = (uint8_t)((w >> 16) & 1u);
    { uint32_t v = (w >> 17) & 15u;
      vis_style = v<=4u?vis_from_saved(v):VIS_SCOPE; }
    cpu1_off = (uint8_t)((w >> 26) & 1u);       /* menu 2ND CORE (ui_menu.c); 2.7: bit 26 set = OFF, so any older settings
                                                 * word (2.6 and before: 0) reads as ON, the two-core SLOOP (an early 2.7 beta
                                                 * set it for ON: that FM-1 reads OFF once) */
    cpu1_units = cpu1_off ? 0 : -1;
    knob_accel_off = (uint8_t)((w >> 27) & 1u);  /* menu KNOB ACCEL (2.7): an older word reads ON */
    usb_rate = (uint8_t)(((w >> 28) & 3u) % 3u); /* menu USB RATE (2.7 beta): an older word reads AUTO */
    theme_on = (uint8_t)((w >> 30) & 1u);
    rec_quant_off = (uint8_t)((w >> 31) & 1u);  /* REC screen QUANT (2.7): an older word reads ON */        /* menu COLOR on your theme (2.7, gfx.c): shown once it is there */
}

static void settings_save(void);              /* project.c: flash copy (FELUCCA_FLASH) */
/* Importing canonical settings (including a phone's song-order-only write)
 * retains device-local fork preferences. An explicitly changed upstream visualizer wins. */
UI_SMALL static void settings_restore_word(uint32_t w)
{
    uint32_t previous = lights_word(), forked = fork_settings_word();
    lights_from_word(w);
    if ((w ^ previous) & (15u << 17))
        forked = (forked & 31u) | (uint32_t)VIS_SAVE_ID[vis_style] << 8;
    fork_settings_from_word(forked);
}
static uint8_t settings_later;                 /* changed while playing: saved once stopped (project.c) */

UI_SMALL static void settings_init(void)
{
    if (settings.magic != SETTINGS_MAGIC || settings.palette >= NPALETTES) {
        settings.magic = SETTINGS_MAGIC;
        settings.palette = NPALETTES - 1u;       /* NEON; existing saved palettes keep their IDs */
        settings.lowcut = 0;
        settings.zoom = 0;                     /* large readout of the touched value: off */
    }
    palette_set(settings.palette);
    fx_lowcut = (uint8_t)(settings.lowcut != 0);
}

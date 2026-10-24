/* SPDX-License-Identifier: GPL-3.0-only */
/* Production editor command dispatch and sequencer, with inert platform hooks.
 * Flash writes and unrelated editor musical commands fail if invoked. */
#undef NDEBUG
#define FELUCCA_ARRANGER 1
#define FELUCCA_FLASH 1
#include <stdint.h>
static uint8_t nor[0x100000];
#define SMP_USER_XIP(k) (nor+SMP_USER_OFF(k))
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t *p) {}
#include "../firmware/src/gfx.c"
static int32_t fm1_enc_take(uint32_t e) {return 0;}
static void fm1_led_key(uint32_t id,int on) {}
#include "../firmware/src/panel.c"
#include "../firmware/src/ui.c"
#undef clamp
static uint8_t flash_ok=1;
static int st_read(uint32_t off,void *dst,uint32_t n) {assert(off+n<=sizeof nor);memcpy(dst,nor+off,n);return 0;}
static int st_erase(uint32_t off) {assert(0);return -1;}
static int st_prog(uint32_t off,const void *src,uint32_t n) {assert(0);return -1;}
#define ST_XIP_PTR(off) (nor+(off))
#include "../firmware/src/storage.c"
#define FL_FAR(fn) fn
static uint32_t irq_save(void) {return 0;}
static void irq_restore(uint32_t f) {}
static uint32_t fl_jedec_ram(void) {return 0x856014u;}
static void fl_plain_window_init(void) {}
#define fm1_xip_ptr(off) (nor+(off))
static uint8_t groove_confirm;
#include "../firmware/src/upreset.c"
#include "../firmware/src/project.c"
static void ota_wire_send(const uint8_t *p,uint32_t n) { assert(n >= 6 && p[0] == 0xf0 && p[n-1] == 0xf7); }
static void fm1_wdt_feed(void) {}
static void fl_inval(uint32_t off,uint32_t n) {}
static int fl_erase4k_quiet(uint32_t off,uint32_t *took) {return st_erase(off);}
static int fl_write(uint32_t off,const uint8_t *p,uint32_t n) {return st_prog(off,p,n);}
/* Unused editor commands need their platform declarations at compile time. */
static uint32_t so_w,so_r;
#define SXQ 2048u
static uint8_t groove_cursor,drum_cursor;
static uint32_t fm1_icfg(void) {return 0;}
static void fm1_icfg_set(uint32_t irq) {}
static void layer_key(uint32_t b,uint32_t k,uint32_t down) { assert(0); }
static uint32_t key_of_white(uint32_t k) { assert(0);return 0; }
static void rec_toggle(void) { assert(0); }
static void pattern_rotate(track_t *t,int32_t delta) { assert(0); }
static void pattern_length(track_t *t,int32_t delta) { assert(0); }
static void pattern_transpose(track_t *t,int32_t delta) { assert(0); }
static int ota_frame_get(const uint8_t **p,uint32_t *n) {return 0;}
static void ota_frame_done(void) {}
static void sequence_screen_close(void) {assert(0);}
#include "../firmware/src/editor.c"
/* Only STEP_SET / TRACK_STEP are exercised; unrelated editor musical hooks
 * above fail if called. The handler and step decoder are production source. */
static int active[16][128];
static void drain_notes(void) {
    while(mo_r!=mo_w){uint32_t p=midi_out_q[mo_r++%MOUT_Q],s=(p>>8)&240u,c=(p>>8)&15u,n=(p>>16)&127u;
        if(s==144u){assert(!active[c][n]);active[c][n]++;}if(s==128u){assert(active[c][n]);active[c][n]--;}}
}
static void pump_notes(void){int32_t out[CTL*2];mix_block(out,CTL);drain_notes();}
static void until(unsigned idx){unsigned timeout=FS*4/CTL;while(trk[0].seq_idx!=idx){pump_notes();assert(timeout--);}}
int main(void) {
    memset(nor,255,sizeof nor);host_tracks_init();panel=PANEL_DEFAULT;usb.config=1;song.g[G_MIDI]=1;
    song.sel=song.octave=0;fm1_in.notes=fm1_in.buttons=0;
    for(unsigned command=0;command<2;command++)for(unsigned slot=0;slot<4;slot++) {
        seq_stop();drain_notes();host_tracks_init();song.g[G_MIDI]=1;
        track_t *t=&trk[0];t->p[P_VOICE]=V_POLY;t->p[P_AMODE]=t->p[P_CHORD]=t->p[P_STRUM]=0;
        t->p[P_SLEN]=4;t->p[P_SGATE]=128;t->p[P_SDIV]=2;
        t->step[0]=(step_t){.note={60,64,67,71},.n=4,.vel=90,.rat=255};
        t->step[1]=(step_t){.note={60,64,67,71},.n=4,.flags=SF_TIE(0)|SF_TIE(1)|SF_TIE(2)|SF_TIE(3)};
        trk[1].p[P_AMODE]=0;trk[1].p[P_SLEN]=4;trk[1].p[P_SGATE]=128;
        trk[1].step[0]=(step_t){.note={48},.n=1,.vel=90};trk[1].step[1].time=trk[1].step[2].time=ST_TIE;
        seq_start();pump_notes();assert(t->seq_n==4&&active[1][48]);
        uint8_t f[]={ED_HDR0,ED_HDR1,ED_HDR2,ED_STEP_SET,0,4,60,64,67,71,ST_NOTE,0,90,0,2,127};
        uint8_t frame[18];uint32_t n=sizeof f;
        if(command){memcpy(frame,f,4);frame[3]=ED_TRACK_STEP;frame[4]=0;memcpy(frame+5,f+4,sizeof f-4);n++;}
        else memcpy(frame,f,sizeof f);
        frame[(command?7:6)+slot]=72;
        ed_handle(frame,n);assert(t->step[0].note[slot]==72);drain_notes();
        until(1);assert(!active[0][72]&&active[1][48]);
        until(2);
        for(unsigned pitch=0;pitch<128;pitch++)assert(!active[0][pitch]); /* before STOP */
        until(0);assert(active[0][72]);until(2);assert(!active[0][72]);
    }
    /* Old seven-bit writers preserve bit 7; v16 writers explicitly set/clear it.
     * All four level/ratchet high bits retain their original fields. */
    seq_stop();drain_notes();
    track_t *leg=&trk[0];
    uint8_t bytes[]={1,60,0,0,0,ST_NOTE,SF_ACCENT,100,127,15,127};
    ed_step_in(leg,0,bytes,sizeof bytes);
    assert(leg->step[0].flags==(SF_ACCENT|SF_LEG) && leg->step[0].lvl==255 && leg->step[0].rat==255);
    bytes[9]=3;ed_step_in(leg,0,bytes,sizeof bytes);assert(leg->step[0].flags&SF_LEG);
    bytes[9]=11;ed_step_in(leg,0,bytes,sizeof bytes);assert(!(leg->step[0].flags&SF_LEG));
    bytes[9]=15;ed_step_in(leg,0,bytes,sizeof bytes);
    project_t captured;proj_capture(&captured);assert(proj_ok(&captured));
    leg->step[0].flags=0;proj_apply(&captured,1);assert(leg->step[0].flags&SF_LEG);
    seq_stop();drain_notes();for(unsigned c=0;c<16;c++)for(unsigned p=0;p<128;p++)assert(!active[c][p]);
    puts("phone sequencing: production STEP_SET/TRACK_STEP dispatch, four slots, ratchets, REST, next loop and other track PASS");return 0;
}


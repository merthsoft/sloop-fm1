/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual project/settings/storage and editor boundaries on a fault-injected NOR.
 * Keep PROJ_HOST undefined: production restore and retry code must be compiled. */
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
static unsigned erases, programs, headers;
enum { FAULT_NONE, FAULT_SIDE_DROP, FAULT_SIDE_CORRUPT, FAULT_SIDE_PROG, FAULT_SIDE_READ };
static int fault;
static int st_read(uint32_t off,void *dst,uint32_t n) {
    assert(off+n<=sizeof nor);
    if(fault==FAULT_SIDE_READ && (off&4095u)==64u)return -1;
    memcpy(dst,nor+off,n);return 0;
}
static int st_erase(uint32_t off) {assert(!(off&4095u));erases++;memset(nor+off,255,4096);return 0;}
static int st_prog(uint32_t off,const void *src,uint32_t n) {
    assert(off+n<=sizeof nor && (off&255u)+n<=256u);programs++;
    if(!(off&4095u))headers++;
    if((off&4095u)==64u) {
        if(fault==FAULT_SIDE_PROG)return -9;
        if(fault==FAULT_SIDE_DROP)return 0;
    }
    for(uint32_t i=0;i<n;i++)nor[off+i]&=((const uint8_t *)src)[i];
    if((off&4095u)==64u && fault==FAULT_SIDE_CORRUPT)nor[off+4]&=0xfdu; /* extra zero bit */
    return 0;
}
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
static void ota_wire_send(const uint8_t *p,uint32_t n) {}
static void fm1_wdt_feed(void) {}
static void fl_inval(uint32_t off,uint32_t n) {}
static int fl_erase4k_quiet(uint32_t off,uint32_t *took) {return st_erase(off);}
static int fl_write(uint32_t off,const uint8_t *p,uint32_t n) {
    while(n) {uint32_t k=256u-(off&255u);if(k>n)k=n;
        int rc=st_prog(off,p,k);if(rc)return rc;off+=k;p+=k;n-=k;}return 0;
}
/* Unused editor commands need their platform declarations at compile time. */
static uint32_t so_w,so_r;
#define SXQ 2048u
static uint8_t groove_cursor,drum_cursor;
static uint32_t fm1_icfg(void) {return 0;}
static void fm1_icfg_set(uint32_t irq) {}
static void layer_key(uint32_t b,uint32_t k,uint32_t down);
static uint32_t key_of_white(uint32_t k);
static void rec_toggle(void);
static void pattern_rotate(track_t *t,int32_t delta);
static void pattern_length(track_t *t,int32_t delta);
static void pattern_transpose(track_t *t,int32_t delta);
static int ota_frame_get(const uint8_t **p,uint32_t *n);
static void ota_frame_done(void);
static void layer_key(uint32_t b,uint32_t k,uint32_t down) {assert(0);}
static uint32_t key_of_white(uint32_t k) {assert(0);return 0;}
static void rec_toggle(void) {assert(0);}
static void pattern_rotate(track_t *t,int32_t d) {assert(0);}
static void pattern_length(track_t *t,int32_t d) {assert(0);}
static void pattern_transpose(track_t *t,int32_t d) {assert(0);}
static int ota_frame_get(const uint8_t **p,uint32_t *n) {return 0;}
static void ota_frame_done(void) {}
static void sequence_screen_close(void) {assert(0);}
#include "../firmware/src/editor.c"
static void setup(void) {
    memset(nor,255,sizeof nor);host_tracks_init();panel=PANEL_DEFAULT;
    memset(&settings,0,sizeof settings);memset(proj_slot,0,sizeof proj_slot);
    memset(&theme,0,sizeof theme);theme_valid=theme_on=0;
    lights_from_word(0);fork_settings_from_word(0);
    arr_defaults(&arrangement);settings_init();fault=0;erases=programs=headers=0;
    settings_later=fav_dirty=sec_dirty=song_dirty=0;memset(&persist_saved,0,sizeof persist_saved);
    memset(midi_saved,0,sizeof midi_saved);fork_settings_saved=0;
    for(unsigned i=0;i<FAV_N;i++)fav[i]=FAV_NONE;
}
static void u32_7(uint8_t *p,uint32_t v) {for(unsigned i=0;i<5;i++)p[i]=(uint8_t)((v>>(7*i))&127u);}
/* Use the real backup protocol handler, including transfer staging and CRC. */
static uint32_t restore_settings(const persist_t *p) {
    uint8_t begin[12]={0,1}, data[7+sizeof(*p)*2]={1,1}, commit[2]={2,1};
    u32_7(begin+2,sizeof *p);u32_7(begin+7,st_crc32(p,sizeof *p));
    ed_n=0;assert(ed_backup(ED_BK_PUT,begin,sizeof begin));assert(ed_n==3 && !ed_out[2]);
    ed_n=0;ed_pack7((const uint8_t *)p,sizeof *p);uint32_t packed=ed_n;
    memcpy(data+7,ed_out,packed);
    ed_n=0;assert(ed_backup(ED_BK_PUT,data,7+packed));assert(ed_n==3 && !ed_out[2]);
    ed_n=0;assert(ed_backup(ED_BK_PUT,commit,sizeof commit));assert(ed_n==3);
    return ed_out[2];
}
static void transport_test(void) {
    setup();persist_t before,desired,after;persist_fill(&before);desired=before;desired.palette=1;
    for(unsigned changed=0;changed<2;changed++) {
        desired.arrangement.entry[0].bars=(uint8_t)(before.arrangement.entry[0].bars+changed);
        for(unsigned state=0;state<3;state++) {
            song.playing=state==0;transport_req=state==0?0:(uint8_t)state;
            assert(restore_settings(&desired)==3);
            persist_fill(&after);assert(!memcmp(&before,&after,sizeof before));
            assert(!erases && !programs);
        }
    }
    song.playing=transport_req=0;
    lights_scale=1;swing_eighth=1;scale_light_level=2;
    uint32_t forked=fork_settings_word();
    assert(!restore_settings(&desired) && erases && settings.palette==1);
    assert(fork_settings_word()==forked);
    memset(&settings,0,sizeof settings);panel.magic=0;fork_settings_from_word(0);
    persist_boot();settings_init();panel_init();
    assert(settings.palette==1 && !memcmp(&arrangement,&desired.arrangement,sizeof arrangement));
    assert(fork_settings_word()==forked && !memcmp(&panel,&desired.panel,sizeof panel));
    puts("settings restore: real begin/data/commit rejects all active transport states; stopped cold reload PASS");
}
static void side_retry_test(void) {
    for(int injected=FAULT_SIDE_DROP;injected<=FAULT_SIDE_READ;injected++) {
        setup();settings_save();assert(!settings_later);
        persist_t old_payload;side_rec_t old_side;
        assert(st_load(OBJ_SETTINGS,&old_payload,sizeof old_payload)==sizeof old_payload);
        assert(!st_side_load(OBJ_SETTINGS,&old_side,sizeof old_side));
        uint32_t old_fork=fork_settings_saved,old_headers=headers;
        fav[0]=2;fav_dirty=1;lights_scale=1;swing_eighth=1;scale_light_level=2;
        theme.magic=THEME_MAGIC;memcpy(theme.name,"RETRY",5);theme.c[4]=theme.hi=0xffff;
        theme.crc=st_crc32(&theme,__builtin_offsetof(theme_t,crc));assert(theme_check(&theme));
        theme_valid=theme_on=1;
        uint32_t expected_fork=fork_settings_word();theme_t expected_theme=theme;
        fault=injected;settings_save();
        assert(settings_later && fav_dirty && fork_settings_saved==old_fork && headers==old_headers);
        fault=FAULT_NONE;
        persist_t loaded;side_rec_t side;
        assert(st_load(OBJ_SETTINGS,&loaded,sizeof loaded)==sizeof loaded && !memcmp(&old_payload,&loaded,sizeof loaded));
        assert(!st_side_load(OBJ_SETTINGS,&side,sizeof side) && !memcmp(&old_side,&side,sizeof side));
        /* Exercise the main-loop deferred retry, not a test replacement. */
        fm1_ms+=6000;ui_input_ms=0;sections_flush();
        assert(!settings_later && !fav_dirty && headers==old_headers+1 && fork_settings_saved==expected_fork);
        memset(&settings,0,sizeof settings);memset(&theme,0,sizeof theme);theme_valid=theme_on=0;
        for(unsigned i=0;i<FAV_N;i++)fav[i]=FAV_NONE;
        fork_settings_from_word(0);persist_boot();settings_init();panel_init();
        assert(fav[0]==2 && fav[1]==FAV_NONE && theme_valid && theme_on);
        assert(!memcmp(&theme,&expected_theme,sizeof theme) && fork_settings_word()==expected_fork);
    }
    /* Older/no-side objects remain legal, including when the side path is faulty. */
    setup();fault=FAULT_SIDE_READ;
    assert(!st_save(OBJ_PROJECT0,"legacy",6));char b[6];
    assert(st_load(OBJ_PROJECT0,b,sizeof b)==6 && !memcmp(b,"legacy",6));
    puts("side record: dropped/corrupt/error/read faults preserve old commit; deferred retry and cold reload PASS");
}
static void upstream_settings_test(void) {
    setup();
    uint8_t mp[MIDI_PART], round[MIDI_PART];
    midi_part_fill(mp); assert(midi_part_default(mp));
    mp[2*ML_N+3] = 0xea; /* ch10 + preview + advance-off + OTHER */
    mp[2*ML_N+4] = 0xf9; /* bend9 + fade15 */
    mp[2*ML_N+6] = 0x2b; /* map1 + clock1 + resonance10 */
    midi_part_read(mp,sizeof mp);
    assert(drum_preview && step_adv_off && fxfade==15 && flt_reso==10);
    assert(song.g[G_DRCH]==10 && song.g[G_CHOTH]==1 && song.g[G_BEND]==9);
    assert(song.g[G_DRMAP]==1 && song.g[G_CLKOUT]==1);
    midi_part_fill(round); assert(!memcmp(mp,round,sizeof mp) && !midi_part_default(round));
    lights_scale=1;scale_light_level=3;settings.palette=1;settings_save();
    drum_preview=step_adv_off=fxfade=flt_reso=0;lights_scale=0;
    persist_boot();dsu_boot();
    assert(drum_preview && step_adv_off && fxfade==15 && flt_reso==10);
    assert(lights_scale && scale_light_level==3 && song.g[G_BEND]==9 && song.g[G_CLKOUT]==1);
    persist_t backup;persist_fill(&backup);backup.palette=2;
    assert(!restore_settings(&backup)); /* backup restore retains suffix preferences */
    drum_preview=step_adv_off=fxfade=flt_reso=0;persist_boot();dsu_boot();
    assert(drum_preview && step_adv_off && fxfade==15 && flt_reso==10);
    mp[2*ML_N+6]=0xff; midi_part_read(mp,sizeof mp);
    assert(flt_reso==10 && song.g[G_DRMAP]==1 && song.g[G_CLKOUT]==1);
    mp[2*ML_N+4]=0xf0; midi_part_read(mp,sizeof mp); assert(song.g[G_BEND]==2 && fxfade==15);
    /* Old suffix values restore defaults without misreading new high bits. */
    mp[2*ML_N+3]=10; mp[2*ML_N+4]=2;mp[2*ML_N+6]=0;
    midi_part_read(mp,sizeof mp);
    assert(!drum_preview && !step_adv_off && !fxfade && !flt_reso);
    puts("upstream suffix: packed bits, old defaults, invalid clamp, restart, backup retention PASS");
}
static unsigned packed(const void *p,unsigned n,uint8_t *dst){ed_n=0;ed_pack7(p,n);unsigned z=ed_n;memcpy(dst,ed_out,z);return z;}
static void request(unsigned cmd,const uint8_t *a,unsigned n){uint8_t f[640]={ED_HDR0,ED_HDR1,ED_HDR2};f[3]=cmd;memcpy(f+4,a,n);memset(main_scratch.st,0xa5,sizeof main_scratch.st);ed_handle(f,n+4);}
static void sample_scratch_test(void){
 setup();
 for(unsigned slot=0;slot<4;slot++)for(unsigned zones=1;zones<=16;zones+=15){
  uint8_t data[256],a[620]={0};smp_user_hdr_t h={0};
  for(unsigned i=0;i<sizeof data;i++)data[i]=(uint8_t)(i*37+slot+zones);
  a[0]=slot;request(ED_SMP_BEGIN,a,1);assert(ed_smp_open[slot] && !usr_nz[slot]);
  a[0]=slot;a[1]=0;a[2]=4;a[3]=0;unsigned np=packed(data,sizeof data,a+4);
  request(ED_SMP_WRITE,a,4+np);assert(!memcmp(nor+SMP_USER_OFF(slot)+512,data,256));
  h.magic=SMP_USER_MAGIC;h.version=1;h.nz=zones;h.data_len=256;h.crc=st_crc32(data,sizeof data);memcpy(h.name,"AUDIT",5);
  for(unsigned i=0;i<zones;i++){h.zone[i].n=512;h.zone[i].le=511;h.zone[i].rate=65536;h.zone[i].root16=960;h.zone[i].hi=127;}
  a[0]=slot;h.nz=17;np=packed(&h,sizeof h,a+1);request(ED_SMP_END,a,np+1);assert(ed_smp_open[slot] && !usr_nz[slot]);
  h.nz=zones;np=packed(&h,sizeof h,a+1);request(ED_SMP_END,a,np+1);
  assert(!ed_smp_open[slot] && usr_nz[slot]==zones && !memcmp(nor+SMP_USER_OFF(slot),&h,sizeof h));
  memset(main_scratch.st,0x5a,sizeof main_scratch.st);smp_user_scan(slot);assert(usr_nz[slot]==zones);
 }
 persist_t p;persist_fill(&p);assert(!restore_settings(&p));
 puts("sample scratch: 8 uploads, all four slots, 1/16 zones, malformed header refusal, scratch poisoning, header/data bytes and backup restore PASS");
}

int main(int argc,char **argv) {
    if(argc==1 || !strcmp(argv[1],"samples"))sample_scratch_test();
    if(argc==1 || !strcmp(argv[1],"upstream"))upstream_settings_test();
    if(argc==1 || !strcmp(argv[1],"transport"))transport_test();
    if(argc==1 || !strcmp(argv[1],"side"))side_retry_test();
    return 0;
}

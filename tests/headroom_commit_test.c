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
enum { FAULT_NONE, FAULT_SIDE_DROP, FAULT_SIDE_CORRUPT, FAULT_SIDE_PROG, FAULT_SIDE_READ, FAULT_TAIL_DROP, FAULT_TAIL_PROG, FAULT_TAIL_CORRUPT };
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
    int tail=((off&~4095u)==0xFC000u || (off&~4095u)==0xFD000u) && (off&4095u)>=2048u;
    if(tail && fault==FAULT_TAIL_PROG)return -9;
    if(tail && fault==FAULT_TAIL_DROP)return 0;
    if((off&4095u)==64u) {
        if(fault==FAULT_SIDE_PROG)return -9;
        if(fault==FAULT_SIDE_DROP)return 0;
    }
    for(uint32_t i=0;i<n;i++)nor[off+i]&=((const uint8_t *)src)[i];
    if(tail && fault==FAULT_TAIL_CORRUPT)nor[off]=0;
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

int original_commit_test(int argc,char **argv) {
    if(argc==1 || !strcmp(argv[1],"samples"))sample_scratch_test();
    if(argc==1 || !strcmp(argv[1],"upstream"))upstream_settings_test();
    if(argc==1 || !strcmp(argv[1],"transport"))transport_test();
    if(argc==1 || !strcmp(argv[1],"side"))side_retry_test();
    return 0;
}

static void notify_test(void) {
 setup();
 for(unsigned cmd=0;cmd<4;cmd++)for(unsigned selected=0;selected<NTRK;selected++)for(unsigned track=0;track<NTRK;track++) {
  uint8_t a[14]={track,7,2,64,64,1,0,0,0,0,0,0,0,0};unsigned which,n;
  song.sel=selected;ui.force=0;ed_w.st[7]=0x12345678;
  if(cmd==0){which=ED_MICRO_SET;n=3;a[2]=90;}
  else if(cmd==1){which=ED_FILL_SET;n=3;}
  else if(cmd==2){which=ED_LOCK_SET;n=5;a[2]=P_LEVEL;}
  else {which=ED_DRUM_STEP;n=14;a[0]=7;a[1]=3;track=TRK_DRUM;}
  request(which,a,n);assert(ui.force);
  assert(ed_w.st[7]==(selected==track?ed_step_sig(&trk[track],7):0x12345678));
  if(cmd==3)break;
 }
 puts("editor notifications: actual MICRO/FILL/LOCK/DRUM requests preserve selected/nonselected WATCH shadow PASS");
}
static void syn_commit_test(void) {
 setup();song.playing=1;assert(ed_dsyn_store()==3 && !erases);song.playing=0;
 flash_ok=0;assert(ed_dsyn_store()==4 && !erases);flash_ok=1;
 ed_bk_put=1;assert(ed_dsyn_store()==4 && !erases);ed_bk_put=0;
 persist_t before=persist_saved;fault=FAULT_SIDE_PROG;assert(ed_dsyn_store()==4 && !memcmp(&before,&persist_saved,sizeof before));
 fault=FAULT_NONE;assert(!ed_dsyn_store());persist_t after;persist_fill(&after);assert(!memcmp(&after,&persist_saved,sizeof after));
 puts("SYN STORE: busy/flash/staging errors, injected write failure, unchanged saved cache and successful retry PASS");
}
static void browser_autosave_test(void) {
 setup();song.sel=0;browse_saved.active=0;browse_category=0;
 song.playing=transport_req=rec_wait=ft_on=ui.menu=0;flash_ok=1;
 fm1_ms=100000;ui_input_ms=autosave_ms=autosave_checked=0;
 browse_step(1);assert(browse_saved.active);
 unsigned writes=programs;autosave_tick();
 assert(!autosave_checked && programs==writes);
 browse_end(1);autosave_tick();assert(autosave_checked==fm1_ms);
 browse_step(1);assert(browse_saved.active);
 proj_capture(&autosave_buf);proj_apply(&autosave_buf,1);
 assert(!browse_saved.active);
 puts("browser persistence: pending audition blocks actual autosave; KEEP resumes; project adoption invalidates original patch PASS");
}
#ifndef SNAPSHOT_PERSISTENCE_FIXTURE
UI_SMALL static uint32_t ed_step_sig_reference(const track_t *t, uint32_t i)   /* all 10 bytes (a drum step's lanes too), v7: + its nudge and locks, v8: + its condition */
{
    const uint8_t *b = (const uint8_t *)&t->step[i % NSTEP];
    uint32_t k, h = 0x811C9DC5u;
    for (k = 0; k < sizeof t->step[0]; k++)
        h = (h ^ b[k]) * 16777619u;
    h = (h ^ (uint8_t)t->micro[i % NSTEP]) * 16777619u;
    h = (h ^ (step_fill(t, i) + 1u)) * 16777619u;        /* v8: its fill condition */
    for (k = 0; k < NLOCK; k++)
        if (t->lock[k].step == i)
            h = (h ^ (uint32_t)(t->lock[k].param | (uint16_t)t->lock[k].val << 8)) * 16777619u;
    return h;
}
static void audit_hash_oracle(void) {
    track_t t;uint32_t seed=0x71821345u;
    for(uint32_t trial=0;trial<10000;trial++) {
        for(uint32_t j=0;j<sizeof t;j++){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;((uint8_t *)&t)[j]=(uint8_t)seed;}
        for(uint32_t i=0;i<NSTEP;i++)assert(ed_step_sig(&t,i)==ed_step_sig_reference(&t,i));
    }
    puts("editor hash: 640000 reference comparisons passed");
}
static void duck_persistence_test(void) {
 setup();flash_ok=1;
 song.g[G_DUCK]=97;song.g[G_DTRIG]=2;song.g[G_DREC]=4;song.g[G_DSHAPE]=2;
 duck_bypass[1]=1;song.g[G_DTYPE]=3;
 proj_capture(&autosave_buf);
 assert(!st_save(OBJ_AUTOSAVE,&autosave_buf,sizeof autosave_buf));
 host_tracks_init();memset(&autosave_buf,0,sizeof autosave_buf);
 assert(st_load(OBJ_AUTOSAVE,&autosave_buf,sizeof autosave_buf)==sizeof autosave_buf);
 proj_apply(&autosave_buf,1);
 assert(song.g[G_DUCK]==97 && song.g[G_DTRIG]==2 && song.g[G_DREC]==4 && song.g[G_DSHAPE]==2);
 assert(!duck_bypass[0] && duck_bypass[1] && !duck_bypass[2] && song.g[G_DTYPE]==3);
 unsigned writes=programs;
 for(unsigned cmd=87;cmd<=89;cmd++) {
  uint8_t op=0;request(cmd,&op,1);
  assert(ed_n==8 && ed_out[4]==cmd && ed_out[5]==0 && ed_out[6]==2 && ed_out[7]==0xF7);
 }
 assert(programs==writes && song.g[G_DUCK]==97);
 puts("DUCK: NOR save/cold-state reload; retired companion commands explicitly unavailable without writes PASS");
}
static void song_save_filter_test(void) {
 setup();flash_ok=1;song.playing=transport_req=0;autosave_hash=0;
 song.g[G_FILT]=-40;proj_capture(&autosave_buf);assert(!autosave_commit(&autosave_buf));
 song.g[G_FILT]=0;arrangement_save();arrangement_save();
 /* No idle tick or shutdown flush: power goes off immediately after SAVE. */
 host_tracks_init();song.g[G_FILT]=-40;autosave_hash=0;autosave_resume();
 assert(!song.g[G_FILT]);
 /* Other current globals and mix values belong to the same working save. */
 song.g[G_FILT]=23;song.g[G_SWING]=61;trk[0].p[P_LEVEL]=79;
 arrangement.entry[0].bars=7;arrangement_save();
 host_tracks_init();autosave_hash=0;autosave_resume();
 assert(song.g[G_FILT]==23 && song.g[G_SWING]==61 && trk[0].p[P_LEVEL]==79);
 persist_boot();assert(arrangement.entry[0].bars==7);
 assert(!proj_slot[0].magic); /* SONG SAVE never silently overwrites a section. */
 fault=FAULT_SIDE_PROG;arrangement.entry[0].bars=8;arrangement_save();assert(strcmp(ui.msg,"SONG SAVED"));
 fault=FAULT_NONE;arrangement_save();assert(!strcmp(ui.msg,"SONG SAVED"));
 persist_boot();assert(arrangement.entry[0].bars==8);
 /* Background autosave must also record zero once actually idle and quiet. */
 song.g[G_FILT]=-40;proj_capture(&autosave_buf);assert(!autosave_commit(&autosave_buf));
 song.g[G_FILT]=0;song.playing=transport_req=rec_wait=ft_on=ui.menu=0;browse_saved.active=0;
 for(uint32_t p=0;p<NTRK;p++)memset(trk[p].v,0,sizeof trk[p].v);
 memset(&drums,0,sizeof drums);fm1_ms+=30000u;ui_input_ms=fm1_ms-30000u;autosave_checked=0;
 autosave_tick();
 host_tracks_init();song.g[G_FILT]=-40;autosave_hash=0;autosave_resume();assert(!song.g[G_FILT]);
 song.playing=1;unsigned writes=programs;arrangement_save();assert(programs==writes);song.playing=0;
 puts("SONG SAVE: cleared filter, repeated saves, immediate restart, working mix and arrangement order PASS");
}
int main(int argc,char **argv){audit_hash_oracle();original_commit_test(argc,argv);notify_test();syn_commit_test();browser_autosave_test();duck_persistence_test();song_save_filter_test();return 0;}
#endif

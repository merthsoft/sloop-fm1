/* SPDX-License-Identifier: GPL-3.0-only */
/* Real section queue, explicit save, project codec and NOR A/B storage.
 * Device-settings persistence is an observable external commit seam. */
#define FELUCCA_ARRANGER 1
#define FELUCCA_FLASH 1
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG /* Assertions must run under optimized host compilers too. */
#include <assert.h>
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i) {return i<NPART?i:0;}
#include "../firmware/src/project.c"
static uint8_t nor[0x100000];
static int fail_write;
static uint32_t fail_sector;
static int st_read(uint32_t off,void *p,uint32_t n) {memcpy(p,nor+off,n);return 0;}
static int st_erase(uint32_t off) {if(fail_write || (fail_sector && off==fail_sector))return -1;memset(nor+off,255,4096);return 0;}
static int st_prog(uint32_t off,const void *p,uint32_t n) {
    if(fail_write)return -1;assert((off&255)+n<=256);
    for(uint32_t i=0;i<n;i++)nor[off+i]&=((const uint8_t *)p)[i];return 0;
}
#include "../firmware/src/storage.c"
static uint8_t sec_dirty,song_dirty,fav_dirty,settings_later;
static int flash_ok=1,setting_commits,preference,persisted_preference,fail_settings;
static char message[64];
static void ui_message(const char *s) {snprintf(message,sizeof message,"%s",s);}
static void ui_say(const char *s,const char *suffix) {snprintf(message,sizeof message,"%s%s",s,suffix);}
static void settings_save(void) {
    setting_commits++;settings_later=fail_settings;
    if(!fail_settings)persisted_preference=preference;
}
static const char *const SEC_NAME[]={"A","B","C","D"};
static project_t autosave_buf;
static uint32_t autosave_hash,autosave_ms;
#include "../firmware/src/project_save.c"
static project_t expected,loaded;
static void cold_load_d(void) {
    memset(proj_slot,0,sizeof proj_slot);host_tracks_init();
    assert(st_load(OBJ_PROJECT0+3,&loaded,sizeof loaded)==sizeof loaded);
    assert(proj_ok(&loaded));proj_apply(&loaded,1);
    assert(trk[0].p[P_LEVEL]==31 && trk[1].p[P_LEVEL]==96 && trk[2].p[P_LEVEL]==17);
    assert(song.g[G_DRLVL]==55 && song.g[G_SWING]==83);
    assert(trk[0].p[P_ROOT]==7 && trk[0].p[P_SCALE]==8);
    assert(song.g[G_RMODEL]==4 && song.g[G_RTIME]==73);
}
static void cold_resume(void) {
    host_tracks_init();memset(proj_slot,0,sizeof proj_slot);
    assert(st_load(OBJ_AUTOSAVE,&loaded,sizeof loaded)==sizeof loaded);
    assert(proj_ok(&loaded));proj_apply(&loaded,1);
    assert(trk[0].p[P_ROOT]==7 && trk[0].p[P_SCALE]==8);
    assert(song.g[G_RMODEL]==4 && song.g[G_RTIME]==73);
    assert(autosave_hash==loaded.sum);
}
static void setup(void) {
    host_tracks_init();memset(nor,255,sizeof nor);sec_dirty=song_dirty=0;
    song.playing=transport_req=0;arrangement_clock.running=0;fail_sector=0;setting_commits=0;fail_write=fail_settings=0;
    trk[0].p[P_LEVEL]=31;trk[1].p[P_LEVEL]=96;trk[2].p[P_LEVEL]=17;
    trk[0].p[P_ROOT]=1;trk[0].p[P_SCALE]=2; /* Old startup state: C# minor. */
    proj_capture(&loaded);assert(!st_save(OBJ_AUTOSAVE,&loaded,sizeof loaded));
    autosave_hash=loaded.sum;
    trk[0].p[P_ROOT]=7;trk[0].p[P_SCALE]=8; /* G Phrygian */
    song.g[G_RMODEL]=4;song.g[G_RTIME]=73;
    song.g[G_DRLVL]=55;song.g[G_SWING]=83;preference=1;persisted_preference=0;
}
int main(void) {
    setup();section_store(3);
    assert(!strcmp(message,"SAVED SECTION D") && !sec_dirty);
    assert(setting_commits==1 && persisted_preference==1);cold_load_d();cold_resume();
    setup();song.playing=1;section_store(3);
    assert(!strcmp(message,"QUEUED: STOP TO SAVE") && sec_dirty==8 && !setting_commits);
    assert(st_load(OBJ_PROJECT0+3,&loaded,sizeof loaded)<0);
    expected=proj_slot[3];trk[0].p[P_LEVEL]=120;song.g[G_SWING]=0;
    song.playing=0;sections_write();assert(!sec_dirty && persisted_preference==1);
    assert(st_load(OBJ_PROJECT0+3,&loaded,sizeof loaded)==sizeof loaded);
    assert(!memcmp(&expected,&loaded,sizeof loaded));cold_load_d();cold_resume();
    assert(trk[0].p[P_LEVEL]==120 && song.g[G_SWING]==0);
    setup();fail_write=1;section_store(3);
    assert(sec_dirty==8 && strstr(message,"RETRYING") && !setting_commits);
    fail_write=0;sections_write();assert(!sec_dirty && persisted_preference==1);cold_load_d();cold_resume();
    setup();fail_settings=1;section_store(3);
    assert(settings_later && !persisted_preference && strstr(message,"SETTINGS"));
    fail_settings=0;sections_write();assert(!settings_later && persisted_preference==1);
    setup();fail_sector=st_sector(OBJ_AUTOSAVE,1);section_store(3);
    assert(sec_dirty==8 && strstr(message,"RETRYING"));
    assert(st_load(OBJ_PROJECT0+3,&loaded,sizeof loaded)==sizeof loaded);
    assert(pt_get(&loaded.t[0],P_ROOT)==7 && pt_get(&loaded.t[0],P_SCALE)==8);
    assert(st_load(OBJ_AUTOSAVE,&loaded,sizeof loaded)==sizeof loaded);
    assert(pt_get(&loaded.t[0],P_ROOT)==1 && pt_get(&loaded.t[0],P_SCALE)==2);
    fail_sector=0;sections_write();assert(!sec_dirty);cold_resume();
    setup();song.playing=1;section_store(3);song.playing=0;
    fail_sector=st_sector(OBJ_AUTOSAVE,1);sections_write();assert(sec_dirty==8);
    fail_sector=0;sections_write();assert(!sec_dirty);cold_resume();
    /* No change to upstream object IDs/codec: a power-on read sees the explicit
     * save immediately, without waiting for background autosave's 20s gap. */
    setup();section_store(3);cold_resume();
    puts("section save: stopped durability, live queue snapshot, levels/swing/key cold reload, startup snapshot, settings commit and failure retries PASS");
    return 0;
}

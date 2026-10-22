/* SPDX-License-Identifier: GPL-3.0-only */
#undef NDEBUG
#define FELUCCA_ARRANGER 1
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i) {return i<NPART?i:0;}
#include "../firmware/src/project.c"
static struct {uint8_t force;} ui;
static uint8_t sync_reload;
#include "../firmware/src/arranger_scene.c"
static void reset(uint32_t bpm) {
    memset(trk,0,sizeof trk);host_tracks_init();for(uint32_t k=0;k<NTRK;k++)steps_clear(&trk[k]);
    memset(&drums,0,sizeof drums);drum_factory_set=-2;song.g[G_BPM]=bpm;
    song.rec=0;rec_wait=0;fm1_in.notes=fm1_in.buttons=kb_prev=0;
}
#define UP_HOST 1
#include "../firmware/src/upreset.c"

static void packet(uint32_t src, uint32_t status, uint32_t a, uint32_t b)
{
    midi_in_q[mi_w++ % MQ] = (src << 4) | 11u | (status << 8) | (a << 16) | (b << 24);
    events_block(CTL);
}
static void all_expression(uint32_t v)
{ for (uint32_t k=0;k<NTRK;k++) midi_cc(&trk[k],11,v); }
static void full_expression(void)
{ for (uint32_t k=0;k<NTRK;k++) assert(ex_off[k]==0); }
static double synth_energy(uint32_t expression)
{
    reset(120);all_expression(127);midi_cc(&trk[0],11,expression);
    host_preset(&trk[0],0,0);trk[0].p[P_AMODE]=0;trk[0].p[P_CHOR]=trk[0].p[P_DLY]=trk[0].p[P_REV]=0;trk_note_on(&trk[0],60,110);
    double sum=0;mixacc_t mx;int32_t scratch[CTL];
    for(int b=0;b<150;b++) {memset(&mx,0,sizeof mx);mix_part(&trk[0],CTL,&mx,scratch);if(b>30)for(int i=0;i<CTL;i++)sum+=(double)mx.l[i]*mx.l[i];}
    return sum;
}
static double drum_energy(uint32_t expression, uint32_t kit)
{
    reset(120);all_expression(127);TDRUM->p[P_E0]=(int16_t)kit;
    midi_cc(TDRUM,11,expression);drum_on(36,110);
    mixacc_t mx;double sum=0;
    for(int b=0;b<200;b++) {
        memset(&mx,0,sizeof mx);mix_drums(CTL,&mx);
        for(int i=0;i<CTL;i++)sum+=(double)mx.l[i]*mx.l[i];
    }
    return sum;
}
int main(void)
{
    /* Enable after applying the documented unowned drums.c hook. Exercise
     * both raw samples (kit zero) and synthesized kits, dry and send gain. */
    if(getenv("MIDI_REQUIRE_DRUM_HOOK")) {
        for(uint32_t kit=0;kit<DRUM_KITS;kit++) {
            double full=drum_energy(127,kit),half=drum_energy(64,kit),off=drum_energy(0,kit);
            if(kit>=DRUM_USR && kit<DRUM_SYN) assert(full==0 && half==0 && off==0); /* empty user sample slots */
            else assert(full>0 && half>0 && half<full*.15 && off==0);
        }
    }
    double full=synth_energy(127),half=synth_energy(64),silent=synth_energy(0);
    assert(full>0 && half>0 && half<full*.15 && silent==0);
    reset(120);all_expression(127);
    int level=trk[0].p[P_LEVEL],drlevel=song.g[G_DRLVL];
    packet(0,0xb0,11,64);assert(ex_off[0]==63 && ex_off[1]==0);
    packet(1,0xb9,11,0);assert(ex_off[TRK_DRUM]==127);
    assert(trk[0].p[P_LEVEL]==level && song.g[G_DRLVL]==drlevel);
    midi_cc(&trk[0],1,99);packet(0,0xb0,121,0);
    assert(ex_off[0]==0 && mod_midi[0]==0 && ex_off[TRK_DRUM]==127);
    packet(1,0xb9,121,0);full_expression();
    packet(0,0xb0,0,1);packet(1,0xc0,4,0);assert(pc_bank[0]==0 && pc_req[0]==5);
    packet(0,0xc0,4,0);assert(pc_bank[0]==1);
    packet(0,0xb1,0,8);packet(0,0xc1,9,0);assert(pc_bank[1]==8 && pc_bank[0]==1);
    packet(0,0xb0,0,7);assert(pc_bank[0]==1); /* queued snapshot */
    song.g[G_ROUTE]=3;song.sel=2;
    packet(0,0xb0,0,9);packet(0,0xc1,7,0);assert(pc_bank[2]==8 && pc_req[2]==8);
    song.g[G_ROUTE]=1;packet(0,0xb1,0,3);song.g[G_ROUTE]=0;
    packet(0,0xc1,0,0);assert(pc_bank[1]==8);
    memset(up_bank,0,sizeof up_bank);
    up_rec_t *r=up_rec(5);r->used=UP_USED;r->ver=UP_VER;r->np=P_COUNT;r->engine=0;r->name[0]='X';
    assert(up_used(5));
    assert(up_pc_index(0,127,153)==127 && up_pc_index(1,24,153)==152);
    assert(up_pc_index(1,25,153)==153 && up_pc_index(1,26,153)==-1);
    assert(up_pc_index(8,5,153)==153 && up_pc_index(8,0,153)==-1);
    assert(up_pc_index(8,32,153)==-1 && up_pc_index(127,127,153)==-1);
    assert(up_pc_index(128,0,153)==-1 && up_pc_index(0,128,153)==-1);
    static project_t saved,before,after;
    reset(120);proj_capture(&before);all_expression(0);proj_capture(&after);
    assert(!memcmp(&before,&after,sizeof before));saved=before;
    proj_apply(&saved,1);full_expression();all_expression(1);proj_apply(&saved,0);full_expression();
    proj_slot[0]=saved;all_expression(0);arrangement_apply(0);full_expression();
    song_backup();all_expression(0);song_restore();full_expression();
    /* Pending PC cannot overwrite adopted sounds; CC0 channel latches survive. */
    packet(0,0xb0,0,1);packet(0,0xc0,4,0);proj_apply(&saved,1);
    assert(!pc_req[0]&&!pc_bank[0]&&ch_bank[0][0]==1);
    puts("MIDI expression audio/routing/reset/load/scene and source-isolated bank mapping: ok");
    return 0;
}

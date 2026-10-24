/* SPDX-License-Identifier: GPL-3.0-only */
/* Production pc_poll/preset_go_to/up_load_to with memory-backed XIP records.
 * Exercises actual record loading; NOR programming remains outside this test. */
#define UI_PAGES_HARNESS_ONLY 1
#define UI_REAL_UPRESET 1
#include "ui_pages_test.c"
#include "../firmware/src/upreset.c"
static void *up_stage(void) { static up_bank_t staged;return &staged; }
static up_bank_t user_banks[2];
static void pc(uint32_t track,uint32_t bank,uint32_t program)
{ pc_bank[track]=(uint8_t)bank;pc_req[track]=(uint8_t)(program+1);pc_poll();assert(!pc_req[track]); }
static void factory(uint32_t track,uint32_t bank,uint32_t program)
{
    uint32_t idx=bank*128u+program,k,e=base_at(idx,&k);
    assert(e<NENGINES);pc(track,bank,program);
    assert(trk[track].eng_req==e && trk[track].preset==k);
}
int main(void)
{
    panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(4);
    bank_resolve();song.sel=0;
    trk[1].p[P_LEVEL]=53;trk[1].p[P_PAN]=-17;
    trk[1].p[P_REV]=33;trk[1].p[P_DLY]=12;trk[1].p[P_CHOR]=15;
    trk[1].p[P_AMODE]=ARP_PULSE;trk[1].p[P_AHOLD]=1;
    factory(1,0,127);factory(1,1,NBANK-129);
    assert(trk[1].p[P_LEVEL]==53 && trk[1].p[P_PAN]==-17);
    assert(trk[1].p[P_REV]==33 && trk[1].p[P_DLY]==12 && trk[1].p[P_CHOR]==15);
    assert(trk[1].p[P_AMODE]==ARP_PULSE && trk[1].p[P_AHOLD]==1);
    uint32_t e=trk[1].eng_req,p=trk[1].preset;
    pc(1,127,127);assert(trk[1].eng_req==e && trk[1].preset==p);
    pc(1,8,31);assert(trk[1].eng_req==e && trk[1].preset==p);
    pc(TRK_DRUM,127,2);assert(TDRUM->p[P_E0]==2);
    pc(TRK_DRUM,8,127);assert(TDRUM->p[P_E0]==2);
    /* Only the bank captured with PC is consumed, not the newer CC0 latch. */
    ch_bank[0][1]=127;factory(1,0,0);
    for(unsigned b=0;b<2;b++)up_bank_xip[b]=&user_banks[b];
    up_rec_t *rec=&user_banks[0].r[5];
    rec->used=UP_USED;rec->ver=UP_VER;rec->engine=0;rec->np=P_COUNT;
    memcpy(rec->name,"BANK USER 5",12);
    for(unsigned i=0;i<P_COUNT;i++)rec->p[i]=track_desc(&trk[0],i)->def;
    rec->p[P_E0]=2;rec->p[P_VOICE]=V_UNISON;rec->p[P_REV]=0;
    trk[1].p[P_VOICE]=V_LEGATO;pc(1,8,5);
    assert(trk[1].user==6&&trk[1].eng_req==0&&trk[1].p[P_E0]==2);
    assert(trk[1].p[P_VOICE]==V_LEGATO&&trk[1].p[P_REV]==33&&trk[1].p[P_LEVEL]==53);
    trk[1].p[P_E0]=0;pc(1,1,NBANK-128); /* first occupied user in linear list */
    assert(trk[1].user==6&&trk[1].p[P_E0]==2);
    /* Factory-owned arps follow the new sound; player edits survive it,
     * including an engine switch before the audio engine catches up. */
    track_t *t=&trk[0];t->user=0;t->p[P_AMODE]=0;t->eng_req=3;
    unsigned chip=0;while(strcmp(ENGINES[3]->presets[chip].name,"8BIT ARP"))chip++;
    apply_preset_to(t,chip);assert(t->p[P_AMODE] && arp_is_sounds(t));
    apply_preset_to(t,0);assert(!t->p[P_AMODE]);
    apply_preset_to(t,chip);t->p[P_ARATE]++;int rate=t->p[P_ARATE];
    apply_preset_to(t,0);assert(t->p[P_AMODE] && t->p[P_ARATE]==rate);
    t->p[P_AMODE]=0;apply_preset_to(t,chip);set_engine_of(t,0);assert(!t->p[P_AMODE]);
    t->p[P_AMODE]=ARP_PULSE;set_engine_of(t,3);assert(t->p[P_AMODE]==ARP_PULSE);
    /* A user preset has the same ownership policy as a factory sound. */
    t->p[P_AMODE]=0;rec->p[P_AMODE]=ARP_PULSE;assert(!up_load_to(t,5));
    assert(arp_is_sounds(t));apply_preset_to(t,0);assert(!t->p[P_AMODE]);
    t->p[P_AMODE]=0;up_load_to(t,5);t->p[P_ARATE]++;
    rate=t->p[P_ARATE];apply_preset_to(t,0);assert(t->p[P_AMODE]==ARP_PULSE && t->p[P_ARATE]==rate);
    puts("MIDI production UI PC seam: factory/user records, invalid inputs, drum bank-ignore and preservation PASS");
    return 0;
}

/* SPDX-License-Identifier: GPL-3.0-only */
#undef NDEBUG
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
#include "../firmware/src/note_chain_actions.c"
static track_t saved;
static step_t model[64];
static unsigned active[128],ons,offs;
static void drain(void)
{
    while(mo_r!=mo_w) {
        uint32_t p=midi_out_q[mo_r++%MOUT_Q],status=(p>>8)&0xf0,n=(p>>16)&127;
        if(status==0x90){assert(!active[n]);active[n]=1;ons++;}
        if(status==0x80){assert(active[n]);active[n]=0;offs++;}
    }
}
static void reset(track_t *t,unsigned len)
{
    steps_clear(t);t->p[P_SLEN]=len;t->p[P_VOICE]=V_POLY;
    for(unsigned k=0;k<NLOCK;k++)t->lock[k].step=LOCK_FREE;
}
static void reject(track_t *t,unsigned at,unsigned pitch,unsigned dest,unsigned move,int code)
{
    saved=*t;
    int r=note_chain_place(t,at,pitch,dest,move,0);
    if(r!=code)fprintf(stderr,"reject %u %u -> %u move%u got%d expected%d\n",at,pitch,dest,move,r,code);
    assert(r==code);assert(!memcmp(t,&saved,sizeof saved));
    assert(note_chain_place(t,at,pitch,dest,move,1)==code);assert(!memcmp(t,&saved,sizeof saved));
}
static void verify(track_t *t,unsigned at,unsigned pitch,unsigned dest,unsigned target,unsigned move,unsigned placing)
{
    saved=*t;unsigned len=trk_len(t),count=note_chain_len(t,at,pitch);
    for(unsigned i=0;i<len;i++)note_effective(t,i,&model[i]);
    int r=placing?note_chain_place(t,at,pitch,dest,move,0):note_chain_pitch(t,at,pitch,target,0);
    assert(!memcmp(t,&saved,sizeof saved));
    int w=placing?note_chain_place(t,at,pitch,dest,move,1):note_chain_pitch(t,at,pitch,target,1);
    assert(r==w);
    if(r){assert(!memcmp(t,&saved,sizeof saved));return;}
    for(unsigned i=0;i<len;i++) {
        step_t got;note_effective(t,i,&got);
        for(unsigned n=0;n<128;n++) {
            unsigned k=note_slot(&model[i],n),exists=k<model[i].n;
            unsigned incoming=exists&&!!(model[i].flags&SF_TIE(k));
            if(move&&i>=at&&i-at<count&&n==pitch)exists=0;
            if(i>=dest&&i-dest<count&&n==target){exists=1;incoming=i!=dest;}
            unsigned g=note_slot(&got,n);assert((g<got.n)==exists);
            if(exists)assert(!!(got.flags&SF_TIE(g))==incoming);
            if(exists&&n!=target) {
                assert(((got.lvl>>(2*g))&3)==((model[i].lvl>>(2*k))&3));
                assert(((got.rat>>(2*g))&3)==((model[i].rat>>(2*k))&3));
            }
        }
    }
    assert(!memcmp(t->lock,saved.lock,sizeof t->lock));
    assert(!memcmp(t->micro,saved.micro,sizeof t->micro));
}
int main(void)
{
    host_tracks_init();track_t *t=&trk[0];
    reset(t,8);
    t->step[0]=(step_t){.note={60,64,67,71},.n=4,.vel=91,.lvl=0xe4,.rat=0x1b};
    t->step[1].time=ST_TIE;t->step[2].time=ST_TIE;
    t->step[3]=(step_t){.note={60},.n=1,.vel=75}; /* deliberate reattack */
    verify(t,0,64,0,65,1,0);
    assert(t->step[0].lvl==0xe4&&t->step[0].rat==0x1b);
    verify(t,0,65,1,65,1,1); /* forwards overlapping move */
    verify(t,1,65,0,65,1,1); /* backwards overlap */
    reject(t,0,65,1,0,NOTE_CHAIN_COLLISION);
    reject(t,0,65,7,1,NOTE_CHAIN_RANGE);
    saved=*t;assert(note_chain_pitch(t,0,65,128,1)==NOTE_CHAIN_RANGE);assert(!memcmp(t,&saved,sizeof saved));
    t->lock[0].step=0;reject(t,0,65,4,1,NOTE_CHAIN_LOCKS);t->lock[0].step=LOCK_FREE;
    t->step[4]=(step_t){.note={1,2,3,4},.n=4};reject(t,0,65,4,0,NOTE_CHAIN_FULL);
    t->step[4]=(step_t){.note={1},.n=1,.vel=23,.flags=SF_ACCENT,.lvl=3,.rat=2};
    t->p[P_VOICE]=V_MONO;reject(t,0,65,4,0,NOTE_CHAIN_POLY);t->p[P_VOICE]=V_POLY;
    verify(t,0,65,4,65,0,1);assert(t->step[4].vel==23&&(t->step[4].flags&SF_ACCENT));
    assert((t->step[4].lvl&3)==3&&(t->step[4].rat&3)==2);
    assert(((t->step[4].lvl>>2)&3)==1&&((t->step[4].rat>>2)&3)==2);
    /* Every slot, extreme MIDI pitches, every short length and randomized
     * valid mixed independent/legacy patterns, compared with a cell model. */
    int32_t rng=0x123459;
    for(unsigned round=0;round<1200;round++) {
        unsigned len=1+noise32(&rng)%64;reset(t,len);
        for(unsigned i=0;i<len;i++) {
            step_t *s=&t->step[i];
            if(i&&noise32(&rng)%4==0){s->time=ST_TIE;continue;}
            s->n=1+noise32(&rng)%4;s->time=ST_NOTE;s->vel=20+noise32(&rng)%100;
            s->lvl=noise32(&rng);s->rat=noise32(&rng);
            for(unsigned k=0;k<s->n;k++) {
                s->note[k]=k==3?127:k*32;
                if(i&&noise32(&rng)%2) {
                    step_t prev;note_effective(t,i-1,&prev);
                    if(note_slot(&prev,s->note[k])<prev.n)s->flags|=SF_TIE(k);
                }
            }
        }
        unsigned at=noise32(&rng)%len;step_t *s=&t->step[at];
        if(s->time!=ST_NOTE)continue;
        unsigned k=noise32(&rng)%s->n;if(s->flags&SF_TIE(k))continue;
        unsigned pitch=s->note[k],placing=noise32(&rng)%2,dest=placing?noise32(&rng)%len:at;
        verify(t,at,pitch,dest,placing?pitch:noise32(&rng)%128,placing?noise32(&rng)%2:1,placing);
    }
    reset(t,8);t->p[P_AMODE]=t->p[P_CHORD]=t->p[P_STRUM]=0;
    t->p[P_SDIV]=2;t->p[P_SGATE]=16;
    t->step[0]=(step_t){.note={60,64},.n=2,.vel=90};
    t->step[1].time=t->step[2].time=ST_TIE;
    assert(!note_chain_pitch(t,0,64,67,1));
    assert(!note_chain_place(t,0,67,1,1,1));
    assert(!note_chain_place(t,1,67,4,0,1));
    song.g[G_MIDI]=1;song.g[G_BPM]=120;usb.config=1;
    memset(mo_set,0,sizeof mo_set);mo_r=mo_w=0;
    seq_start();
    for(unsigned i=0;i<FS*6/CTL;i++){int32_t out[CTL*2];mix_block(out,CTL);drain();}
    seq_stop();drain();assert(ons>=3&&ons==offs);
    for(unsigned n=0;n<128;n++)assert(!active[n]);
    puts("note chain actions: atomic rejection, event model, overlap, expression, 1200 randomized patterns, balanced sequenced MIDI PASS");
}

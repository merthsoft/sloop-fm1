/* SPDX-License-Identifier: GPL-3.0-only */
/* Focused engine ownership/pitch contracts, through production hostsim includes.
 * Standalone: optimization-cc -O1 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal
 *   tests/engine_pitch_ownership_test.c -lm -pthread -o PRIVATE_OUT
 * --fingerprints emits stable direct-engine renders for before/after comparison;
 * --fingerprints-fine adds common fine pitch (intentional affected changes).
 * Never updates the project's golden files. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
static unsigned failures, checks;
#define EXPECT(c) do { checks++; if (!(c)) { if (failures++ < 24) fprintf(stderr,"engine contract line %d: %s\n",__LINE__,#c); } } while (0)
static track_t *fresh(unsigned e)
{
    memset(trk,0,sizeof trk); memset(eng_arena,0,sizeof eng_arena);
    memset(eng_arena_own,0,sizeof eng_arena_own); memset(phys_lay,0,sizeof phys_lay);
    memset(fm6_eff,0,sizeof fm6_eff); memset(live_mod,0,sizeof live_mod);
    memset(mod_midi,0,sizeof mod_midi); rng_state=0x1234567; vage=0;
    formant_nz=0x2545F491; song.playing=0; song.sel=0;
    host_tracks_init(); host_preset(&trk[0],e,0);
    trk[0].p[P_AMODE]=0; return &trk[0];
}
static vmod_t modulation(int fine)
{
    vmod_t m={0}; m.pitch16=960; m.fine=fine;
    m.inc=pitch_inc(m.pitch16)+(uint32_t)((int32_t)(pitch_inc(m.pitch16)>>12)*fine);
    m.shape=64<<8; m.amp0=m.amp1=16000; return m;
}
static void render_tick(track_t *t)
{ int32_t out[CTL]; track_render(t,out,CTL); }
static unsigned gates(track_t *t)
{ unsigned n=0; for(unsigned i=0;i<NVOICE;i++) n+=!!t->v[i].gate; return n; }
static void midi_event(unsigned st,unsigned note,unsigned vel)
{
    midi_in_q[mi_w++%MQ]=(st>>4)|(st<<8)|(note<<16)|(vel<<24);
    events_block(CTL);
}
static void note_zero(void)
{
    const unsigned modes[]={V_MONO,V_LEGATO,V_UNISON};
    /* Each priority, both arrival orders and both release orders, including
     * intermediate ownership before all-off can hide a lost fallback. */
    for(unsigned mode=0;mode<3;mode++) for(unsigned priority=0;priority<3;priority++)
    for(unsigned order=0;order<2;order++) for(unsigned offorder=0;offorder<2;offorder++) {
        track_t *t=fresh(0); t->p[P_VOICE]=modes[mode]; t->p[P_PRIO]=priority;
        unsigned a=order?60:0,b=order?0:60;
        trk_note_on(t,a,100); render_tick(t);
        unsigned age=t->v[0].age;
        t->v[0].env=1<<23; t->v[0].penv=1234;
        trk_note_on(t,b,100);
        unsigned want=priority==1?0:priority==2?60:b;
        EXPECT(t->v[0].gate && t->v[0].note==want && t->nmono==2);
        if(modes[mode]!=V_MONO) {
            EXPECT(t->v[0].age==age);
            EXPECT(t->v[0].env==(1<<23) && t->v[0].penv==1234);
        }
        unsigned off=offorder?60:0,left=offorder?0:60;
        trk_note_off(t,off);
        EXPECT(t->nmono==1 && t->v[0].gate && t->v[0].note==left);
        trk_note_off(t,left); EXPECT(!gates(t) && !t->nmono);
    }
    /* All mode transitions retain per-note release obligations. In particular,
     * POLY->MONO must not confuse zeroed mono metadata with ownership of note0. */
    const unsigned allmodes[]={V_POLY,V_MONO,V_LEGATO,V_UNISON};
    for(unsigned old=0;old<4;old++) for(unsigned now=0;now<4;now++) {
        track_t *t=fresh(0); t->p[P_VOICE]=allmodes[old];
        trk_note_on(t,60,100); trk_note_on(t,0,100);
        t->p[P_VOICE]=allmodes[now]; trk_note_off(t,0); trk_note_off(t,60);
        EXPECT(!gates(t) && !t->nmono);
        trk_note_on(t,0,100); EXPECT(t->v[0].gate || gates(t));
        trk_note_off(t,0); EXPECT(!gates(t));
    }
    for(unsigned now=1;now<4;now++) {
        track_t *t=fresh(0); t->p[P_VOICE]=V_POLY;
        trk_note_on(t,60,100); trk_note_on(t,0,100);
        t->p[P_VOICE]=allmodes[now]; trk_note_off(t,0);
        EXPECT(t->v[0].gate && t->v[0].note==60);
        trk_note_off(t,60); EXPECT(!gates(t));
    }
    for(unsigned mode=0;mode<3;mode++) for(unsigned time_mode=0;time_mode<2;time_mode++) {
        track_t *t=fresh(0); t->p[P_VOICE]=modes[mode];
        t->p[P_GLIDE]=80; t->p[P_GLMODE]=time_mode;
        trk_note_on(t,0,100); render_tick(t); trk_note_on(t,60,100);
        EXPECT(t->v[0].pitch_cur==0); /* the sounding zero pitch is a valid glide origin */
        render_tick(t);
        int q=t->v[0].pitch_cur*256+t->v[0].pitch_frac;
        EXPECT(q>0 && q<60*4096);
        trk_note_off(t,60); trk_note_off(t,0); EXPECT(!gates(t));
    }
    /* Real USB queue -> routing -> allocator, without direct note-on calls. */
    for(unsigned mode=0;mode<3;mode++) {
        track_t *t=fresh(0); t->p[P_VOICE]=modes[mode];
        song.g[G_ROUTE]=2; song.g[G_FXCH]=0; midi_scale=0;
        mi_w=mi_r=0; memset(midi_sel_on,0,sizeof midi_sel_on);
        midi_event(0x90,0,100); midi_event(0x90,60,100); midi_event(0x80,60,0);
        EXPECT(t->v[0].gate && t->v[0].note==0);
        midi_event(0x80,0,0); EXPECT(!gates(t));
    }
}
/* Frequency ratio oracle, rather than a copy of each engine's integer formula.
 * The tolerance allows phase-table/coefficient truncation, but rejects an
 * omitted +/-100/4096 common factor (2.44%). */
static void ratio(uint32_t base,uint32_t adjusted,int fine)
{
    double expected=1.0+fine/4096.0;
    EXPECT(base && fabs((double)adjusted/base-expected)<0.0006);
}
static void oscillators(void)
{
    for(int fine=-100;fine<=100;fine+=200) {
        for(unsigned e=0;e<=2;e+=2) for(unsigned det=0;det<2;det++) {
            track_t *t=fresh(e); t->p[P_E0]=0;
            if(e==0) { t->p[P_E1]=det?17:0; t->p[P_E2]=64; }
            else { t->p[P_E4]=det?17:0; t->p[P_E5]=1; }
            voice_t a={0},b={0}; a.note=b.note=60;
            vmod_t m=modulation(0),f=modulation(fine); int32_t x=0,y=0;
            ENGINES[e]->render(t,&a,&x,1,&m); ENGINES[e]->render(t,&b,&y,1,&f);
            ratio(a.ph[0],b.ph[0],fine); ratio(a.ph[1],b.ph[1],fine);
            if(!det) EXPECT(b.ph[0]==b.ph[1]);
        }
        track_t *t=fresh(3); t->p[P_E4]=16; t->p[P_E5]=7; t->p[P_E6]=2;
        voice_t a={0},b={0}; a.note=b.note=60; lofi_note_on(t,&a); lofi_note_on(t,&b);
        vmod_t m=modulation(0),f=modulation(fine);
        for(unsigned block=0;block<64;block++) {
            uint32_t ph=a.ph[0],pf=b.ph[0]; int32_t x=0,y=0;
            lofi_render(t,&a,&x,1,&m); lofi_render(t,&b,&y,1,&f);
            ratio(a.ph[0]-ph,b.ph[0]-pf,fine);
        }
        for(unsigned mode=NZ_LFSR;mode<=NZ_META;mode++) for(unsigned offset=0;offset<2;offset++) {
            t=fresh(11); t->p[P_E0]=mode; t->p[P_E7]=offset?17:0; t->p[P_E6]=0;
            voice_t a={0},b={0}; a.note=b.note=60; noise_note_on(t,&a); noise_note_on(t,&b);
            int32_t x=0,y=0; noise_render(t,&a,&x,1,&m); noise_render(t,&b,&y,1,&f);
            ratio(a.ph[0],b.ph[0],fine);
        }
        for(unsigned mode=NZ_LFSR;mode<=NZ_META;mode++) {
            t=fresh(11); t->p[P_E0]=mode; t->p[P_E7]=0; t->p[P_E6]=127;
            voice_t a={0},b={0}; a.note=b.note=60; noise_note_on(t,&a); noise_note_on(t,&b);
            /* A reachable nonzero drift state forces META's alternate path. */
            a.ph[2]=b.ph[2]=400000;
            a.s[3]=b.s[3]=(int32_t)((20000u<<16)|100u);
            int32_t x=0,y=0; noise_render(t,&a,&x,1,&m); noise_render(t,&b,&y,1,&f);
            ratio(a.ph[0],b.ph[0],fine);
        }
        t=fresh(11); t->p[P_E0]=NZ_META; t->p[P_E7]=127; t->p[P_E6]=0;
        voice_t v={0}; v.note=127; noise_note_on(t,&v);
        f.pitch16=2047; f.inc=pitch_inc(2047)+(uint32_t)((int32_t)(pitch_inc(2047)>>12)*fine); int32_t x=0;
        noise_render(t,&v,&x,1,&f); EXPECT(v.ph[0]==0xFFFFFFFFu);
    }
}
static uint32_t sample_advance(track_t *t,int fine,int cap)
{
    voice_t v={0}; v.note=60; v.gate=v.active=1; sample_note_on(t,&v);
    if(cap) {
        usr_zone[0][0]=SMP_ZONES[0]; usr_zone[0][0].rate=4u<<16;
        usr_zone[0][0].root16=-1536; v.s[4]=0x8000;
    }
    vmod_t m=modulation(fine); int32_t out[CTL]={0}; sample_render(t,&v,out,CTL,&m);
    EXPECT(v.ph[0]>0 && !v.s[6]);
    return (v.ph[0]-1u)*65536u+v.ph[1]; /* subtract interpolation priming sample */
}
static void uploaded_rate_bounds(void)
{
    track_t *t=fresh(4); const int pitches[]={-1536,0,576};
    for(int d=-1536;d<=576;d++) EXPECT((pow2_q16(d)>>8)<=2048u);
    /* Playback uses rate>>8. Every possible quotient, including rate<256 and
     * the maximum supported 4*FS, is represented. Compare actual decoder
     * cursor movement with a wide arithmetic oracle, including both caps. */
    for(unsigned bucket=0;bucket<=1024;bucket++) for(unsigned p=0;p<3;p++)
    for(int fine=-100;fine<=100;fine+=100) {
        unsigned rate=bucket?bucket*256u:1u;
        usr_zone[0][0]=SMP_ZONES[0]; usr_zone[0][0].rate=rate;
        usr_zone[0][0].root16=960-pitches[p];
        voice_t v={0}; v.note=60; v.gate=v.active=1; v.s[4]=0x8000;
        vmod_t m=modulation(fine); int32_t out=0;
        sample_render(t,&v,&out,1,&m);
        int64_t nominal=(pow2_q16(pitches[p])>>8)*(rate>>8);
        int64_t expected=nominal+((nominal*fine)>>12),cap=(rate>>8)*2048u;
        if(expected>cap) expected=cap;
        EXPECT(v.ph[0] && (v.ph[0]-1u)*65536u+v.ph[1]==(uint32_t)expected);
        if(!fine) EXPECT(expected==nominal); /* cap must preserve all old rates */
    }
}
static void samples_and_grains(void)
{
    track_t *t=fresh(4); uint32_t base=sample_advance(t,0,0);
    for(int fine=-100;fine<=100;fine+=200) ratio(base,sample_advance(t,fine,0),fine);
    EXPECT(sample_advance(t,100,1)==32u*CTL*65536u);
    EXPECT(sample_advance(t,-100,1)<32u*CTL*65536u);
    /* Slow source rates retain fine resolution: shifting step before multiplying
     * would turn this into a dead zone. */
    usr_zone[0][0]=SMP_ZONES[0]; usr_zone[0][0].rate=256; usr_zone[0][0].root16=960;
    uint32_t adv[3];
    for(unsigned i=0;i<3;i++) {
        voice_t v={0}; v.note=60; v.gate=v.active=1; v.s[4]=0x8000;
        vmod_t m=modulation(i==0?-100:i==1?0:100); int32_t out[CTL]={0};
        sample_render(t,&v,out,CTL,&m); adv[i]=(v.ph[0]-1u)*65536u+v.ph[1];
    }
    EXPECT(adv[0]<adv[1] && adv[1]<adv[2]);
    t=fresh(8); t->p[P_E0]=0; t->p[P_E2]=24; t->p[P_E5]=t->p[P_E6]=0;
    t->v[0].note=60; t->v[0].active=t->v[0].gate=1;
    grain_note_on(t,&t->v[0]);
    for(unsigned b=0;b<1024;b++) grain_block(t);
    gr_part_t *P=(gr_part_t*)eng_arena_of(t,ENGI_GRAIN), saved=*P;
    uint32_t steps[3];
    for(unsigned i=0;i<3;i++) {
        *P=saved; vmod_t m=modulation(i==0?-100:i==1?0:100);
        gr_spawn(P,t,0,(uint32_t)t->v[0].s[0],1000,&m);
        EXPECT(P->g[0].owner==1); steps[i]=P->g[0].step;
    }
    ratio(steps[1],steps[0],-100); ratio(steps[1],steps[2],100);
    for(int fine=-100;fine<=100;fine+=200) {
        *P=saved; vmod_t m=modulation(fine); m.pitch16=2047;
        m.inc=pitch_inc(2047)+(uint32_t)((int32_t)(pitch_inc(2047)>>12)*fine); t->p[P_E4]=48;
        for(unsigned i=0;i<GR_NG;i++) gr_spawn(P,t,0,(uint32_t)t->v[0].s[0],1000,&m);
        uint32_t load=0;
        for(unsigned i=0;i<GR_NG;i++) if(P->g[i].owner) {
            EXPECT(P->g[i].step<=GR_STEP_MAX && P->g[i].step>=256u);
            load+=P->g[i].step<<P->g[i].rev;
        }
        EXPECT(load<=GR_LOAD_MAX && load>0);
    }
    /* Production voice/control boundary: all SAMPLE unison voices start with
     * the same cursor, but common fine factors must give different advances. */
    for(unsigned e=3;e<=4;e++) {
        t=fresh(e); t->p[P_VOICE]=V_UNISON; t->p[P_DETUNE]=127;
        if(e==3) t->p[P_E4]=t->p[P_E5]=t->p[P_E6]=0;
        trk_note_on(t,60,100); uint32_t initial[NVOICE];
        for(unsigned i=0;i<NVOICE;i++) initial[i]=t->v[i].ph[0];
        render_tick(t); unsigned different=0;
        for(unsigned i=1;i<NVOICE;i++) {
            uint32_t a=e==3?t->v[0].ph[0]-initial[0]:t->v[0].ph[0]*65536u+t->v[0].ph[1];
            uint32_t b=e==3?t->v[i].ph[0]-initial[i]:t->v[i].ph[0]*65536u+t->v[i].ph[1];
            different+=a!=b;
        }
        EXPECT(different && gates(t)==NVOICE);
        trk_note_off(t,60); EXPECT(!gates(t));
    }
}
static uint64_t hash_samples(uint64_t h,const int32_t *b,unsigned n)
{ for(unsigned i=0;i<n;i++){ h^=(uint32_t)b[i]; h*=0x100000001B3ull; } return h; }
static void fingerprints(int nonzero)
{
    /* Fine=0 direct renders for every engine/preset, plus all TRIO wave sets,
     * filter modes, intervals, ring and sync with both fine signs. All m.inc
     * fixtures obey the production caller's m.fine contract. */
    for(unsigned e=0;e<12;e++) for(unsigned pi=0;pi<ENGINES[e]->npresets;pi++) {
        track_t *t=fresh(e); host_preset(t,e,pi); t->p[P_VOICE]=V_POLY;
        voice_t *v=&t->v[0]; v->note=60; v->vel=100; v->active=v->gate=1;
        ENGINES[e]->note_on(t,v); uint64_t h=0xCBF29CE484222325ull;
        vmod_t m=modulation(nonzero?100:0); m.envq15=16000;
        for(unsigned b=0;b<512;b++) {
            int32_t out[CTL]={0}; if(ENGINES[e]->block) ENGINES[e]->block(t);
            ENGINES[e]->render(t,v,out,CTL,&m); h=hash_samples(h,out,CTL);
        }
        printf("preset %u %u %s %016llx\n",e,pi,ENGINES[e]->presets[pi].name,(unsigned long long)h);
    }
    for(unsigned wave=0;wave<16;wave++) for(unsigned mode=0;mode<4;mode++)
    for(int fine=-100;fine<=100;fine+=100) {
        track_t *t=fresh(6); t->p[P_E0]=wave; t->p[P_E4]=mode;
        t->p[P_E1]=-12; t->p[P_E2]=7; t->p[P_E3]=17;
        voice_t *v=&t->v[0]; v->note=60; v->vel=100; v->active=v->gate=1;
        trio_note_on(t,v); vmod_t m=modulation(fine); uint64_t h=0xCBF29CE484222325ull;
        for(unsigned b=0;b<64;b++) {int32_t out[CTL]={0}; trio_render(t,v,out,CTL,&m); h=hash_samples(h,out,CTL);}
        printf("trio %u %u %d %016llx\n",wave,mode,fine,(unsigned long long)h);
    }
}
int main(int argc,char **argv)
{
    if(argc>1) { fingerprints(!strcmp(argv[1],"--fingerprints-fine")); return 0; }
    note_zero(); printf("note zero: %u failures after %u checks\n",failures,checks);
    oscillators(); printf("oscillators: %u cumulative failures after %u checks\n",failures,checks);
    samples_and_grains(); uploaded_rate_bounds();
    printf("engine pitch/ownership: %u failures in %u checks\n",failures,checks);
    return failures!=0;
}

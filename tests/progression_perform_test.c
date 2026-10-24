/* SPDX-License-Identifier: GPL-3.0-only */
/* Real browser, physical key edges, USB/TRS decoder, arp and recorder together. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"

typedef struct { uint8_t part, note, vel; uint16_t step; } attack_t;
static attack_t attacks[4096];
static uint32_t nattacks, noffs, sounding[16][128], capture_step;
static void drain_output(void)
{
    while(mo_r!=mo_w) {
        uint32_t p=midi_out_q[mo_r++%MOUT_Q], st=(p>>8)&240u, ch=(p>>8)&15u;
        uint32_t note=(p>>16)&127u, vel=(p>>24)&127u;
        if(st==0x90u && vel) {
            if(sounding[ch][note])fprintf(stderr,"duplicate on: ch=%u note=%u buttons=%x key=%u live=%u queue=%u/%u\n",ch,note,fm1_in.buttons,progression.key,progression.active,mo_r,mo_w);
            assert(!sounding[ch][note]); sounding[ch][note]=1;
            assert(nattacks<4096u);
            attacks[nattacks++]=(attack_t){(uint8_t)ch,(uint8_t)note,(uint8_t)vel,(uint16_t)capture_step};
        } else if(st==0x80u || (st==0x90u&&!vel)) {
            assert(sounding[ch][note]); sounding[ch][note]=0;noffs++;
        }
    }
}
static void advance(uint32_t count)
{
    while(count--) { uint32_t later;capture_step=rec_target(&trk[0],&later)%trk_len(&trk[0]);events_block(CTL);drain_output(); }
}
static void keys(uint32_t mask) {fm1_in.notes=mask;advance(1);}
static void knob(uint32_t k,int32_t delta) {encs[panel.enc[EN_K1+k]]=delta;frame();drain_output();advance(1);}
static void midi(uint32_t cable,uint32_t status,uint32_t a,uint32_t b)
{midi_in_q[mi_w++%MQ]=(cable<<4)|(status>>4)|status<<8|a<<16|b<<24;advance(1);}
static void held(const uint8_t *want,uint32_t n)
{if(TSEL->nheld!=n||memcmp(TSEL->held,want,n))fprintf(stderr,"held: key=%u active=%u count=%u actual %u %u %u wanted %u %u %u buttons=%x\n",progression.key,progression.active,TSEL->nheld,TSEL->held[0],TSEL->held[1],TSEL->held[2],want[0],want[1],want[2],fm1_in.buttons);assert(TSEL->nheld==n);assert(!memcmp(TSEL->held,want,n));}
static void stopped(void)
{
    seq_stop();drain_output();
    assert(!progression.active&&!trk[0].nheld&&!trk[0].arp_n);
    for(uint32_t ch=0;ch<16;ch++)for(uint32_t n=0;n<128;n++)assert(!sounding[ch][n]);
    assert(noffs==nattacks);
}
static void reset_case(void)
{
    seq_stop();song.rec=rec_wait=ci_on=ft_on=0;transport_req=0;clk_beat=clk_pos=0;
    fm1_in.notes=fm1_in.buttons=kb_prev=notes_seen=edges_btn=0;ly_lock=LY_PLAY;
    panic_req=(1u<<NTRK)-1u;events_block(CTL);
    host_tracks_init();layers_init();panel=PANEL_DEFAULT;song.sel=0;song.seq_mode=0;song.octave=0;
    song.g[G_MIDI]=1;song.g[G_BPM]=120;song.g[G_CLKOUT]=0;song.g[G_SYNC]=0;
    usb.config=1;usb.detached=0;
    go_home();
    for(uint32_t p=0;p<NPAGES;p++)if(PAGES[p].scope==SC_STARTER)ui.page=(uint8_t)p;
    ui.home=0;page_entered();assert(sequence_browser&&sequence_page==0);
    encs[panel.enc[EN_SELECT]]=4;frame();assert(sequence_page==4&&cur_page()->scope==SC_STARTER);
    sequence_sel=NSEQUENCE_STARTERS;sequence_custom_count=4;
    sequence_custom_degree[0]=4;sequence_custom_degree[1]=0;sequence_custom_degree[2]=1;sequence_custom_degree[3]=3;
    sequence_root=0;sequence_scale=2;sequence_octave=0;sequence_vlead=0;
    TSEL->p[P_AMODE]=ARP_UP;TSEL->p[P_AHOLD]=0;TSEL->p[P_AOCT]=1;TSEL->p[P_APROB]=127;
    TSEL->p[P_TRANS]=0;TSEL->p[P_VOICE]=V_POLY;TSEL->p[P_SLEN]=16;
    TSEL->p[P_ARATE]=2;TSEL->p[P_ASWING]=0;TSEL->p[P_AORDER]=AORDER_NOTE;
    trk[1].p[P_AMODE]=ARP_UP;trk[1].p[P_AHOLD]=0;
    TSEL->chord_inv=TSEL->chord_spread=0;
    rec_count=rec_tempo=rec_quant_off=0;rec_snap[0]=REC_SNAP_TRACK;
    progression_build=progression_chord;
    advance(1);fm1_ms+=LY_QUIET_MS;frames(2);memset(mo_set,0,sizeof mo_set);mo_r=mo_w=0;
    memset(sounding,0,sizeof sounding);nattacks=noffs=0;
}
static void arm(void){knob(0,1);if(!progression.active||!sequence_browser||sequence_page!=4)fprintf(stderr,"arm: active=%u browser=%u page=%u held=%u phys=%u sus=%u ext=%u rec=%u wait=%u play=%u panic=%u lock=%u quiet=%u\n",progression.active,sequence_browser,sequence_page,TSEL->nheld,TSEL->arp_phys,sus_on[0],progression_external_busy(0),song.rec,rec_wait,song.playing,panic_req,ly_lock,ly_quiet());assert(progression.active&&sequence_browser&&sequence_page==4);}

static void modifier_cases(void)
{
    static const uint8_t black[11]={1,3,5,8,10,13,15,17,20,22,25};
    static const uint8_t expect[11][4]={
        {60,63,67},{60,64,67,71},{60,65,67},{60,64,67,74},{64,67,72},
        {60,62,67},{60,64,67,81},{60,64,67,71},{60,64,67,70},{55,60,64},{60,67,76}
    };
    static const uint8_t count[11]={3,4,3,4,3,3,4,4,4,3,3};
    static const uint8_t white[16]={0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26};
    for(unsigned arp=0;arp<2;arp++) {
        reset_case();TSEL->p[P_AMODE]=arp?ARP_UP:0;
        sequence_scale=1;sequence_custom_count=3;
        sequence_custom_degree[0]=0;sequence_custom_degree[1]=3;sequence_custom_degree[2]=4;
        arm();
        for(unsigned pad=0;pad<16;pad++) {
            static const uint8_t roots[3]={60,65,67}, thirds[3]={64,69,71}, fifths[3]={67,72,74};
            uint8_t want[3]={(uint8_t)clamp(roots[pad%3]+12*(pad/3),0,127),
                (uint8_t)clamp(thirds[pad%3]+12*(pad/3),0,127),(uint8_t)clamp(fifths[pad%3]+12*(pad/3),0,127)};
            keys(1u<<white[pad]);held(want,3);keys(0);
        }
        keys(1);
        for(unsigned m=0;m<11;m++) {
            uint32_t attacks_before=nattacks;
            keys(1|(1u<<black[m]));held(expect[m],count[m]);
            /* C and G stay held when the third changes: no blanket reattack. */
            if(!arp&&m==0)assert(nattacks==attacks_before+1);
            keys(1);held((uint8_t[]){60,64,67},3);
        }
        keys(1|(1u<<2));held((uint8_t[]){65,69,72},3);
        keys(1u<<2);held((uint8_t[]){65,69,72},3);keys(0);
        knob(2,1);keys(1);keys(0);
        for(unsigned m=0;m<11;m++) {
            keys(1u<<black[m]);held(expect[m],count[m]);keys(0);held(expect[m],count[m]);
            keys(1u<<black[m]);held((uint8_t[]){60,64,67},3);keys(0);
        }
        /* A later browser edit changes the next pad, not the sounding slot
         * when its modifier changes. CHROM on SEL does not disable LIVE mods. */
        TSEL->p[P_QUANT]=3;TSEL->p[P_CHORD]=0;
        sequence_octave=2;sequence_root=7;sequence_scale=2;
        keys(1u<<1);held((uint8_t[]){60,63,67},3);keys(0);
        keys(1u<<1);held((uint8_t[]){60,64,67},3);keys(0);
        /* Quality alternatives toggle exclusively, as ordinary chord mode. */
        keys(1u<<5);keys(0);keys(1u<<13);held((uint8_t[]){60,62,67},3);keys(0);
        assert(!(chord_latch_mods[0]&CM_SUS4) && (chord_latch_mods[0]&CM_SUS2));
        keys(1u<<13);keys(0);keys(1u<<17);keys(0);keys(1u<<20);
        held((uint8_t[]){60,64,67,70},4);keys(0);
        assert(!(chord_latch_mods[0]&CM_MAJ7) && (chord_latch_mods[0]&CM_MIN7));
        knob(2,-1);assert(!TSEL->nheld&&!TSEL->arp_n);stopped();
        assert(!chord_latch_mods[0]);
    }
    puts("perform: all16 pads with3-slot repeats,11 momentary/toggle modifiers, snapshot and direct/arp ownership PASS");
    /* Pool-only checks miss shortened live gates. Hold actual generated roots
     * (including UP8's extra tonic), then alter only the third mid-gate. */
    static const uint8_t modes[4]={ARP_UP,ARP_UP8,ARP_REPEAT,ARP_PULSE};
    for(unsigned mode=0;mode<4;mode++) {
        reset_case();sequence_scale=1;sequence_custom_degree[0]=0;
        TSEL->p[P_AMODE]=modes[mode];TSEL->p[P_AOCT]=2;TSEL->p[P_AGATE]=127;
        arm();keys(1);
        unsigned target=mode==1?84:mode==2?67:60,wait=0;
        while(!(TSEL->arp_n && TSEL->arp_notes[0]==target) && wait++<10000)advance(1);
        assert(wait<10000);
        uint32_t before=nattacks,off_before=noffs,gate=TSEL->arp_off,index=TSEL->arp_idx;
        keys(1|(1u<<1));
        assert(sounding[0][target] && TSEL->arp_n && TSEL->arp_off<gate && TSEL->arp_idx==index);
        assert(nattacks==before && noffs==off_before+(mode==3?2u:0u)); /* PULSE loses E in both octaves. */
        keys(1);assert(sounding[0][target] && nattacks==before);
        advance(500);keys(0);stopped();
    }
    /* All progression lengths still fill the keyboard at clipped registers,
     * sparse scales and voice-led modifier changes. */
    reset_case();TSEL->p[P_AMODE]=0;arm();sequence_root=11;sequence_scale=5;sequence_vlead=1;
    for(unsigned count=1;count<=8;count++)for(unsigned extreme=0;extreme<2;extreme++) {
        sequence_custom_count=count;sequence_octave=extreme?3:-3;
        for(unsigned slot=0;slot<count;slot++)sequence_custom_degree[slot]=(slot*3)%7;
        for(unsigned pad=0;pad<16;pad++) {
            keys(1u<<white[pad]);
            for(unsigned m=0;m<11;m++) {
                keys((1u<<white[pad])|(1u<<black[m]));
                assert(TSEL->nheld && TSEL->nheld<=4);
                for(unsigned i=0;i<TSEL->nheld;i++) {
                    assert(TSEL->held[i]<128);
                    if(i)assert(TSEL->held[i]>TSEL->held[i-1]);
                }
                keys(1u<<white[pad]);
            }
            keys(0);
            assert(noffs==nattacks);nattacks=noffs=0;
        }
    }
    stopped();
    puts("perform: unchanged generated gates, UP8 expansion and count1..8 sparse/extreme/VLEAD bounds PASS");
}

int main(int argc,char **argv)
{
    outdir=argc>1?argv[1]:"build/host";
    settings.palette=NPALETTES-1u;palette_set(settings.palette);
    modifier_cases();
    reset_case();arm();
    keys(1);held((uint8_t[]){67,70,74},3);
    uint32_t phase=TSEL->arp_pos;
    progression_press(2);assert(TSEL->arp_pos==phase&&!TSEL->arp_new);
    keys(1|(1u<<2));held((uint8_t[]){60,63,67},3);
    keys(1u<<2);held((uint8_t[]){60,63,67},3); /* superseded pad up cannot release new harmony */
    keys(0);assert(!TSEL->nheld&&!TSEL->arp_n);
    keys(1u<<4);held((uint8_t[]){62,65,68},3);keys(0);
    keys(1u<<6);held((uint8_t[]){65,68,72},3);keys(0);
    keys(1u<<7);held((uint8_t[]){79,82,86},3);keys(0);
    keys(1u<<1);assert(!TSEL->nheld);keys(0); /* modifiers alone do not start a chord */
    knob(2,1);keys(1);keys(0);held((uint8_t[]){67,70,74},3);
    uint8_t old[4];memcpy(old,TSEL->held,3);
    knob(1,1);held(old,3);keys(1);held((uint8_t[]){79,82,86},3);keys(0);
    uint32_t previous=nattacks;keys(1);advance(800);assert(nattacks>previous);keys(0);
    knob(2,-1);assert(!TSEL->nheld&&!TSEL->arp_n);stopped();
    puts("perform: custom 5-1-2-4, stale release, HOLD, retrigger, next-chord octave and modifier-only silence PASS");

    reset_case();arm();sequence_vlead=1;
    keys(1);uint8_t lower[4];memcpy(lower,TSEL->held,3);keys(0);
    keys(1u<<7);for(uint32_t i=0;i<3;i++)assert(TSEL->held[i]==lower[i]+12);keys(0);stopped();
    reset_case();arm();TSEL->chord_inv=1;TSEL->chord_spread=1;keys(1);
    held((uint8_t[]){70,79,86},3);stopped();
    reset_case();sequence_sel=4;arm();keys(1);held((uint8_t[]){60,63,67,70},4);stopped();
    puts("perform: explicit octave pad anchor and shared inversion/spread PASS");

    reset_case();
    midi(0,0x90,91,117);assert(TSEL->nheld&&progression_external_busy(0));assert(progression_arm());
    midi(0,0x80,91,0);assert(!progression_external_busy(0)&&!TSEL->nheld);
    progression_stop();midi(0,0xb0,64,127);assert(progression_arm());midi(0,0xb0,64,0);keys(1);
    for(uint32_t cable=0;cable<2;cable++) {midi(cable,0x90,91,127);held((uint8_t[]){67,70,74},3);assert(!progression_external_busy(0));midi(cable,0x80,91,0);}
    midi(0,0x91,83,113);assert(trk[1].nheld==1&&trk[1].held[0]==83);midi(0,0x81,83,0);assert(!trk[1].nheld);
    keys(0);knob(0,-1);assert(!progression.active);
    midi(0,0x90,91,117);assert(TSEL->nheld==1&&TSEL->held[0]==91);midi(0,0x80,91,0);assert(!TSEL->nheld);
    stopped();puts("perform: actual USB/TRS note/pedal ownership, other-track routing and post-disarm MIDI PASS");

    reset_case();TSEL->step[0]=(step_t){{91},1,ST_NOTE,90,0,0,0};
    step_t before[NSTEP];memcpy(before,TSEL->step,sizeof before);
    arm();keys(1);tap(B_PLAY);drain_output();assert(song.playing&&progression.active&&sequence_browser);
    advance(1300);assert(!TSEL->seq_n&&!memcmp(before,TSEL->step,sizeof before));
    TSEL->p[P_AORDER]=AORDER_SEQ_NOTE;advance(400);assert(!TSEL->seq_n&&!seq_harmony[0].n&&TSEL->nheld==3);
    for(uint32_t i=0;i<nattacks;i++)assert(attacks[i].note!=91);
    uint32_t into,slen,abs=trk_grid(TSEL,&into,&slen);progression_stop();drain_output();
    assert(TSEL->seq_abs==abs);seq_tick(TSEL,1);assert(TSEL->seq_abs==abs&&!TSEL->seq_n);
    stopped();puts("perform: PLAY retains harmony, stored/SNOTE sources suppressed, immutable pattern and natural-grid resume PASS");

    reset_case();arm();knob(2,1);keys(1);keys(0);
    for(uint32_t b=0;b<5;b++) {
        static const uint8_t buttons[]={B_ENV,B_LFO,B_FX,B_ARP,B_SCL};
        uint32_t saved_key=progression.key;uint8_t source[4];memcpy(source,TSEL->held,3);
        press(buttons[b]);drain_output();
        for(uint32_t f=0;f<50;f++){frame();drain_output();}
        keys(1u<<2);frame();drain_output();
        if(buttons[b]==B_FX) assert(punch.keybit); /* effects stay playable over the live arp */
        keys(0);
        assert(progression.active&&sequence_browser&&progression.key==saved_key);held(source,3);
        release(buttons[b]);drain_output();
    }
    ui.force=1;frame();drain_output();ppm("progression-perform-live");stopped();
    puts("perform: ENV/LFO/FX/ARP/SEL holds suspend pads without replacing live harmony PASS");

    /* ARP OFF is a sustained direct chord, not a stalled generator. */
    reset_case();TSEL->p[P_AMODE]=0;arm();keys(1);
    assert(progression.direct&&TSEL->arp_n==3&&nattacks==3);
    advance(500);assert(nattacks==3&&!noffs&&TSEL->arp_n==3);
    keys(0);assert(!TSEL->arp_n&&!TSEL->nheld&&noffs==3);
    knob(2,1);keys(1);keys(0);uint32_t direct_attacks=nattacks;
    knob(1,1);advance(100);assert(TSEL->arp_n==3&&nattacks==direct_attacks);
    keys(1u<<2);held((uint8_t[]){72,75,79},3);keys(0);stopped();
    reset_case();TSEL->p[P_AMODE]=0;arm();tap(B_REC);drain_output();keys(1);
    assert(song.playing&&song.rec==1&&TSEL->rh_n==3&&TSEL->step[0].n==3);
    advance(1000);assert(nattacks==3&&TSEL->rh_n==3);
    keys(1|(1u<<1));assert(TSEL->rh_n==3);held((uint8_t[]){67,71,74},3);
    uint32_t ignored,changed_step=rec_target(TSEL,&ignored)%trk_len(TSEL);step_t changed;
    note_effective(TSEL,changed_step,&changed);assert(changed.n==3);
    assert(note_slot(&changed,70)==changed.n);
    assert(changed.flags&SF_CONT(note_slot(&changed,67)));
    assert(changed.flags&SF_CONT(note_slot(&changed,74)));
    assert(!(changed.flags&SF_CONT(note_slot(&changed,71))));
    advance(400);note_effective(TSEL,changed_step+1,&changed);
    assert(changed.n==3&&note_slot(&changed,70)==changed.n);
    keys(1);assert(TSEL->rh_n==3);held((uint8_t[]){67,70,74},3);
    keys(0);assert(!TSEL->rh_n&&!TSEL->arp_n);uint32_t ties=0;
    for(uint32_t i=1;i<16;i++)ties+=TSEL->step[i].time==ST_TIE;
    assert(ties);stopped();
    /* Recorded NOTE/CONT fields, rather than only runtime held[], must drive
     * the same modified harmony during ordinary stored-pattern playback. */
    seq_start();drain_output();uint32_t playback_wait=0;
    while(TSEL->seq_abs%trk_len(TSEL)!=changed_step && playback_wait++<5000)advance(1);
    assert(playback_wait<5000 && TSEL->seq_n==3);
    assert(sounding[0][67]&&sounding[0][71]&&sounding[0][74]&&!sounding[0][70]);
    while(TSEL->seq_abs%trk_len(TSEL)!=changed_step+1 && playback_wait++<5000)advance(1);
    assert(playback_wait<5000 && sounding[0][71]&&!sounding[0][70]);stopped();
    reset_case();TSEL->p[P_AMODE]=0;arm();tap(B_REC);drain_output();keys(1);
    uint32_t near_wait=0;
    while(!((rec_target(TSEL,&ignored),ignored)) && near_wait++<500)advance(1);
    assert(near_wait<500);keys(1|(1u<<1));keys(1);
    uint32_t sk;for(sk=0;sk<TSEL->rskip_n && TSEL->rskip[sk]!=70;sk++);
    if(sk==TSEL->rskip_n)fprintf(stderr,"later modifier missing skip70: abs%u n%u [%u %u %u %u]\n",TSEL->rskip_abs,TSEL->rskip_n,TSEL->rskip[0],TSEL->rskip[1],TSEL->rskip[2],TSEL->rskip[3]);
    assert(sk<TSEL->rskip_n);
    const step_t *future=&TSEL->step[TSEL->rskip_abs%trk_len(TSEL)];
    assert(future->n==4 && TSEL->rskip_n==4);
    for(unsigned j=0;j<future->n;j++) {
        for(sk=0;sk<TSEL->rskip_n && TSEL->rskip[sk]!=future->note[j];sk++);
        assert(sk<TSEL->rskip_n);
    }
    uint32_t boundary_attacks=nattacks;advance(100);
    assert(nattacks==boundary_attacks);keys(0);stopped();
    reset_case();TSEL->p[P_AMODE]=0;TSEL->p[P_SLEN]=4;
    arm();tap(B_REC);drain_output();keys(1);
    uint32_t wrap_wait=0;
    while(rec_target(TSEL,&ignored)<4u && wrap_wait++<1500)advance(1);
    assert(wrap_wait<1500);uint32_t wrap_before=nattacks;
    keys(1|(1u<<1));held((uint8_t[]){67,71,74},3);
    assert(nattacks==wrap_before+1 && TSEL->rh_n==3);
    for(unsigned s=0;s<4;s++)assert(TSEL->step[s].n<=4);
    keys(1);held((uint8_t[]){67,70,74},3);keys(0);stopped();
    reset_case();TSEL->p[P_AMODE]=0;arm();keys(1);TSEL->p[P_AMODE]=ARP_UP;advance(1);
    assert(progression.active&&!progression.direct&&TSEL->nheld==3);advance(300);
    TSEL->p[P_AMODE]=0;advance(1);assert(progression.active&&progression.direct&&TSEL->arp_n==3);stopped();
    puts("perform: ARP OFF sustained chords, HOLD, next-chord octave, recorded ties and mode-change cleanup PASS");

    /* Generated events, not source chords, must become the stored notes once. */
    reset_case();arm();rec_count=0;tap(B_REC);drain_output();assert(rec_wait&&progression.active);
    keys(1);assert(song.playing&&song.rec==1&&!ft_on&&!rec_wait);
    advance(1000);song.rec=0;
    uint32_t stored=0;for(uint32_t s=0;s<16;s++) {
        uint32_t expected=0;
        for(uint32_t e=0;e<nattacks;e++)if(attacks[e].step==s) {
            expected++;uint32_t k;for(k=0;k<TSEL->step[s].n;k++)if(TSEL->step[s].note[k]==attacks[e].note)break;
            assert(k<TSEL->step[s].n);assert(step_vel(&TSEL->step[s],k)==attacks[e].vel);
        }
        if(TSEL->step[s].n!=expected)fprintf(stderr,"record step %u stored=%u expected=%u attacks=%u clk=%u/%u\n",s,TSEL->step[s].n,expected,nattacks,clk_beat,clk_pos);
        assert(TSEL->step[s].n==expected);stored+=expected;
    }
    assert(stored==nattacks&&stored>2);stopped();
    puts("perform: REC note-start uses tempo recording; emitted attack pitches/velocities equal stored notes exactly PASS");

    /* Expanded PULSE can sound more notes than the existing four-note recorder stores. */
    reset_case();sequence_sel=4;TSEL->p[P_AMODE]=ARP_PULSE;TSEL->p[P_AOCT]=4;
    step_t outside[NSTEP-16];memcpy(outside,TSEL->step+16,sizeof outside);
    arm();tap(B_REC);drain_output();keys(1);advance(400);song.rec=0;
    stored=0;for(uint32_t s=0;s<16;s++){assert(TSEL->step[s].n<=4);stored+=TSEL->step[s].n;}
    assert(stored&&nattacks>stored&&!memcmp(outside,TSEL->step+16,sizeof outside));stopped();
    puts("perform: four-octave PULSE respects existing four-note recording capacity without adjacent-step corruption PASS");

    reset_case();arm();rec_tempo=0;rec_count=1;tap(B_REC);drain_output();tap(B_PLAY);drain_output();
    assert(ci_on&&!song.playing&&rec_wait&&progression.active&&!rec_tempo);
    keys(1);uint32_t waited=0;while(!song.playing&&waited++<6000)advance(1);
    assert(song.playing&&song.rec==1&&progression.active&&!ci_on&&!rec_tempo);assert(nattacks);
    stopped();puts("perform: real REC/PLAY count-in retains source and begins generated recording PASS");

    reset_case();arm();keys(1);song.sel=1;advance(1);assert(!progression.active&&!trk[0].nheld&&!trk[0].arp_n);stopped();
    reset_case();arm();keys(1);panic_req=1;advance(1);assert(!progression.active&&!TSEL->nheld);stopped();
    reset_case();arm();keys(1);tap(B_HOME);drain_output();assert(progression.active&&!sequence_browser&&TSEL->nheld==3);stopped();
    reset_case();arm();keys(1);press(B_EDIT);drain_output();assert(progression.active);release(B_EDIT);drain_output();stopped();
    puts("perform: track/panic/STOP cleanup and HOME/EDIT continuity leave balanced generated MIDI PASS");
    /* Take ownership mid-groove; draft editing and page navigation stay musical. */
    reset_case();memset(TSEL->step,0,sizeof TSEL->step);tap(B_PLAY);drain_output();advance(200);assert(song.playing);
    sequence_screen_open();sequence_page=4;
    uint32_t beat_before=clk_beat,pos_before=clk_pos;
    arm();assert(song.playing&&!TSEL->nheld&&!pen_n);
    assert(clk_beat>beat_before || clk_pos>=pos_before);
    keys(0);knob(2,1);keys(1);keys(0);held((uint8_t[]){67,70,74},3);
    encs[panel.enc[EN_SELECT]]=-4;frame();drain_output();assert(sequence_page==0);
    knob(1,2);held((uint8_t[]){67,70,74},3);assert(progression.active&&song.playing);
    keys(1u<<2);held((uint8_t[]){62,65,69},3);keys(0);
    tap(B_ARP);drain_output();assert(progression.active&&!sequence_browser&&song.playing);
    advance(300);stopped();
    puts("perform: running takeover, held-key retirement, draft snapshot and ARP page continuity PASS");

    reset_case();TSEL->p[P_AMODE]=0;keys(1);assert(kb_kind[0]==KS_NOTE&&kb_n[0]);
    arm();assert(!TSEL->nheld&&!TSEL->arp_n);keys(0);keys(1);
    held((uint8_t[]){67,70,74},3);keys(0);stopped();
    reset_case();rec_tempo=1;tap(B_REC);drain_output();assert(rec_wait&&!ft_on);
    sequence_screen_open();sequence_page=4;arm();keys(1);
    assert(song.playing&&song.rec&&progression.active);stopped();
    puts("perform: held physical-source takeover and arming an existing tempo take PASS");

    reset_case();arm();uint32_t fuzz=0x51a7e123u;
    for(uint32_t edge=0;edge<300;edge++) {
        fuzz=fuzz*1664525u+1013904223u;
        if(!(edge%11u))knob(2,(fuzz&1u)?1:-1);
        else if(!(edge%7u))knob(1,(fuzz&1u)?1:-1);
        else keys((1u<<((fuzz>>8)%27u))&((fuzz&3u)?~0u:0u));
        advance(17);assert(progression.active&&TSEL->nheld<=4&&TSEL->arp_n<=64);
    }
    stopped();puts("perform: 300 deterministic pad/HOLD/octave transitions retain bounded ownership and balanced MIDI PASS");
    return 0;
}

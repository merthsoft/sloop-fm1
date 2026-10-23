/* SPDX-License-Identifier: GPL-3.0-only */
/* Physical keys, retained harmony, generated MIDI and bounded harmonic output. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"

static const uint8_t mod_key[]={1,3,5,8,10,13,15,17,20,22,25};
static uint8_t sounding[16][128];
static uint32_t ons,offs;
static void drain(void)
{
    while(mo_r!=mo_w) {
        uint32_t p=midi_out_q[mo_r++%MOUT_Q],st=(p>>8)&240u,ch=(p>>8)&15u,n=(p>>16)&127u,v=p>>24;
        /* KEYS + SEQ MIDI can overlap one raw key and its generated arp pitch. */
        if(st==0x90u && v){sounding[ch][n]++;ons++;}
        else if(st==0x80u || (st==0x90u&&!v)){assert(sounding[ch][n]);sounding[ch][n]--;offs++;}
    }
}
static void keys(uint32_t mask){fm1_in.notes=mask;events_block(CTL);drain();}
static void tick(uint32_t count){while(count--){events_block(CTL);drain();}}
static void finish(void)
{
    keys(0);seq_stop();drain();
    assert(ons==offs);
    for(uint32_t ch=0;ch<16;ch++)for(uint32_t n=0;n<128;n++)assert(!sounding[ch][n]);
}
static void begin(int arp,int latch)
{
    seq_stop();drain();fm1_in.notes=fm1_in.buttons=kb_prev=notes_seen=edges_btn=0;
    panic_req=(1u<<NTRK)-1u;events_block(CTL);host_tracks_init();layers_init();panel=PANEL_DEFAULT;
    song.sel=0;song.solo=song.rec=song.playing=song.seq_mode=rec_wait=ft_on=0;song.octave=0;
    song.g[G_MIDI]=1;song.g[G_BPM]=120;usb.config=1;usb.detached=0;
    go_home();ly_lock=LY_PLAY;
    TSEL->p[P_VOICE]=V_POLY;TSEL->p[P_ROOT]=0;TSEL->p[P_SCALE]=1;TSEL->p[P_QUANT]=2;
    TSEL->p[P_CHORD]=CH_TRIAD;TSEL->p[P_VLEAD]=TSEL->p[P_TRANS]=0;
    TSEL->p[P_AMODE]=arp?ARP_UP:0;TSEL->p[P_AHOLD]=latch;
    TSEL->p[P_AOCT]=1;TSEL->p[P_APROB]=127;TSEL->p[P_ARATE]=2;
    TSEL->chord_inv=TSEL->chord_spread=0;
    events_block(CTL);mo_r=mo_w=0;memset(mo_set,0,sizeof mo_set);
    memset(sounding,0,sizeof sounding);ons=offs=0;
}
static void expect(uint32_t mods,const uint8_t *want,uint32_t count)
{
    uint8_t c[4];uint32_t n=chord_play_notes(TSEL,60,mods,c);
    if(n!=count || memcmp(c,want,count)) {
        fprintf(stderr,"mods=%x:",mods);for(uint32_t i=0;i<n;i++)fprintf(stderr," %u",c[i]);fputc('\n',stderr);
    }
    assert(n==count&&!memcmp(c,want,count));
}
static void expect_owned(uint32_t mods,int arp,int latch)
{
    uint8_t want[4],actual[16];uint32_t n=chord_play_notes(TSEL,60,mods,want),count;
    if(arp){count=TSEL->nheld;memcpy(actual,TSEL->held,count);}
    else if(latch){count=chord_latch_n[0];memcpy(actual,chord_latch_notes[0],count);}
    else {count=kb_n[7];memcpy(actual,kb_nt[7],count);}
    harmony_sort_notes(actual,count);
    assert(count==n&&!memcmp(actual,want,n));
}
static void physical(void)
{
    for(uint32_t arp=0;arp<2;arp++)for(uint32_t latch=0;latch<2;latch++) {
        begin(arp,latch);keys(1u<<7);if(latch)keys(0);
        uint32_t root=latch?0u:1u<<7;
        for(uint32_t i=0;i<11;i++) {
            keys(root|(1u<<mod_key[i]));expect_owned(1u<<i,arp,latch);
            keys(root);expect_owned(latch?1u<<i:0u,arp,latch);tick(80);
            if(latch){keys(1u<<mod_key[i]);keys(0);expect_owned(0,arp,latch);}
            assert(TSEL->arp_phys==(arp&&!latch?3u:0u));
        }
        finish();
    }
    puts("modifiers: eleven physical keys, held/latched direct and arp sources, balanced MIDI PASS");
}
static void latch_groups(void)
{
    begin(0,1);keys(1u<<7);keys(0);
    keys(1u<<5);keys(0);assert(chord_latch_mods[0]==CM_SUS4);
    keys(1u<<13);keys(0);assert(chord_latch_mods[0]==CM_SUS2);expect_owned(CM_SUS2,0,1);
    keys(1u<<17);keys(0);assert(chord_latch_mods[0]==(CM_SUS2|CM_MAJ7));
    keys(1u<<20);keys(0);assert(chord_latch_mods[0]==(CM_SUS2|CM_MIN7));expect_owned(CM_SUS2|CM_MIN7,0,1);
    keys(1u<<20);keys(0);assert(chord_latch_mods[0]==CM_SUS2);
    keys(1u<<13);keys(0);assert(!chord_latch_mods[0]);
    finish();
    begin(0,0);keys((1u<<7)|(1u<<13));expect_owned(CM_SUS2,0,0);
    song.sel=1;assert(!chord_mods(1));song.sel=0;finish();
    begin(0,0);TSEL->p[P_QUANT]=3;
    for(uint32_t i=0;i<11;i++) {
        keys(1u<<mod_key[i]);assert(kb_kind[mod_key[i]]==KS_NOTE&&!chord_mods(0));keys(0);
    }
    finish();puts("modifiers: exclusive latch qualities, track isolation and CHROM roots PASS");
}
static void bounds(void)
{
    begin(0,0);
    const uint8_t root[]={0,24,60,100,120,127};
    for(uint32_t type=0;type<CH_COUNT;type++)for(uint32_t scale=0;scale<NSCALES;scale++) {
        TSEL->p[P_CHORD]=type;TSEL->p[P_SCALE]=scale;
        for(uint32_t r=0;r<sizeof root;r++)for(uint32_t mods=0;mods<2048;mods++) {
            uint8_t c[6];memset(c,213,sizeof c);vl_n[0]=0;
            uint32_t n=chord_play_notes(TSEL,root[r],mods,c+1);assert(n>=1&&n<=4);
            assert(c[0]==213&&c[5]==213); /* scratch beyond the returned count may hold deduplicated base tones */
            for(uint32_t i=0;i<n;i++){assert(c[i+1]<128);if(i)assert(c[i]<c[i+1]);}
        }
    }
    finish();puts("modifiers: all 2048 combinations across every shape/scale, six MIDI registers, capacity and uniqueness PASS");
}
int main(void)
{
    begin(0,0);
    for(uint32_t k=0;k<27;k++) {
        uint32_t expected=0;for(uint32_t i=0;i<11;i++)if(k==mod_key[i])expected=1u<<i;
        assert(chord_mod_of_key(k)==expected);
    }
    expect(CM_SUS2,(uint8_t[]){60,62,67},3);
    expect(CM_THIRTEEN,(uint8_t[]){60,64,67,81},4);
    expect(CM_MAJ7,(uint8_t[]){60,64,67,71},4);
    expect(CM_MIN7,(uint8_t[]){60,64,67,70},4);
    expect(CM_DOWN,(uint8_t[]){55,60,64},3);
    expect(CM_OPEN,(uint8_t[]){60,67,76},3);
    expect(CM_SUS2|CM_SUS4,(uint8_t[]){60,65,67},3);
    expect(CM_MAJ7|CM_MIN7,(uint8_t[]){60,64,67,70},4);
    expect(CM_NINE|CM_THIRTEEN,(uint8_t[]){60,64,74,81},4);
    expect(CM_SEVEN|CM_NINE|CM_THIRTEEN,(uint8_t[]){60,64,71,81},4);
    expect(CM_INV|CM_DOWN,(uint8_t[]){60,64,67},3);
    TSEL->p[P_CHORD]=CH_SIXTH;
    expect(CM_SEVEN|CM_NINE|CM_THIRTEEN,(uint8_t[]){60,64,71,81},4);
    expect(CM_MIN7|CM_NINE,(uint8_t[]){60,64,70,74},4);
    TSEL->p[P_CHORD]=CH_TRIAD;TSEL->p[P_SCALE]=2;expect(CM_THIRTEEN,(uint8_t[]){60,63,67,80},4);
    TSEL->p[P_CHORD]=CH_MAJOR;expect(CM_THIRTEEN,(uint8_t[]){60,64,67,81},4);
    TSEL->p[P_CHORD]=CH_MIN7;expect(CM_MAJ7,(uint8_t[]){60,63,67,71},4);
    TSEL->p[P_CHORD]=CH_TRIAD;TSEL->p[P_SCALE]=1;TSEL->p[P_VLEAD]=1;
    expect(0,(uint8_t[]){60,64,67},3);
    for(uint32_t i=0;i<8;i++){expect(CM_INV,(uint8_t[]){64,67,72},3);expect(CM_DOWN,(uint8_t[]){55,60,64},3);}
    finish();physical();latch_groups();bounds();return 0;
}

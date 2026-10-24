/* Sequencing review regressions: real physical events and MIDI ownership before STOP. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static int balance[16][128];
static void drain_balance(void)
{
    while(mo_r!=mo_w) {
        uint32_t p=midi_out_q[mo_r++%MOUT_Q],st=(p>>8)&240u,ch=(p>>8)&15u,n=(p>>16)&127u;
        if(st==0x90u)balance[ch][n]++;
        if(st==0x80u){assert(balance[ch][n]>0);balance[ch][n]--;}
    }
}
static void keys_owned(uint32_t bits){fm1_in.notes=bits;events_block(CTL);drain_balance();}
static void silence(void){for(unsigned c=0;c<16;c++)for(unsigned n=0;n<128;n++)assert(!balance[c][n]);}
static void start_case(int arp,int hold,int chord)
{
    host_tracks_init();layers_init();panel=PANEL_DEFAULT;go_home();
    song.sel=song.octave=0;song.playing=song.rec=rec_wait=ft_on=transport_req=0;
    song.g[G_MIDI]=0;usb.config=1;usb.detached=0;
    fm1_in.notes=fm1_in.buttons=kb_prev=0;ly_lock=LY_PLAY;
    trk[0].p[P_VOICE]=V_POLY;trk[0].p[P_AMODE]=arp?ARP_UP:0;
    trk[0].p[P_AHOLD]=hold;trk[0].p[P_CHORD]=chord?CH_TRIAD:0;
    trk[0].p[P_SCALE]=1;trk[0].p[P_QUANT]=2;trk[0].p[P_AOCT]=1;trk[0].p[P_VLEAD]=0;
    trk[0].p[P_TRANS]=0;trk[0].chord_inv=trk[0].chord_spread=0;
    events_block(CTL);drain_balance();silence();
}
static int has_pitch(unsigned n){for(unsigned i=0;i<trk[0].nheld;i++)if(trk[0].held[i]==n)return 1;return 0;}
static void revoice_release(int arp,int hold,int root_first,int generated)
{
    static const unsigned mods[]={1,3,5,8,10,13,15,17,20,22,25};
    for(unsigned m=0;m<11;m++) {
        start_case(arp,hold,1);song.g[G_MIDI]=generated;
        keys_owned(1u<<7);keys_owned((1u<<7)|(1u<<mods[m]));
        keys_owned(root_first?1u<<mods[m]:1u<<7);keys_owned(0);
        if(arp&&!generated)silence(); /* direct HOLD and generated gates still own their output */
        trk[0].p[P_AHOLD]=0;events_block(CTL);drain_balance();silence();seq_stop();
    }
}
static void channel_release(unsigned type,unsigned off)
{
    start_case(type==1, type==2||type==3, type==2);if(type==4)song.sel=TRK_DRUM;
    keys_owned(1u<<7);if(type==2||type==3)keys_owned(0);
    song.g[type==4?G_DRCH:G_CH1]=off?0:2;events_block(CTL);drain_balance();
    silence(); /* routing panic retires raw notes as well as generated ones */
    keys_owned(0);silence(); /* every release on the original channel, before STOP */
}
static void overlap(unsigned reverse,unsigned hold,unsigned other)
{
    start_case(1,hold,1);keys_owned(1u<<7);keys_owned((1u<<7)|(1u<<other));
    assert(has_pitch(60)&&has_pitch(64)&&has_pitch(67)&&has_pitch(71));
    keys_owned(1u<<(reverse?7:other));
    assert(has_pitch(67));
    if(!hold){assert(trk[0].nheld==3);assert(has_pitch(reverse?60:71));}
    /* Surviving shared tone must follow its surviving key's stored octave and scale. */
    unsigned survivor=reverse?7:other;
    song.octave=2;trk[0].p[P_ROOT]=2;trk[0].p[P_SCALE]=2;
    if(!hold)for(unsigned i=0;i<trk[0].nheld;i++)assert((held_key[0][i]&31u)==survivor+1u);
    uint8_t pool[64];unsigned n=arp_list(&trk[0],pool),found=0;
    unsigned expected=kb_pitch(&trk[0],survivor,kb_oct[survivor])+((int)67-kb_root[survivor]);
    for(unsigned i=0;i<n;i++)found|=pool[i]==expected;
    assert(found);
    keys_owned(0);silence();if(!hold)assert(!trk[0].nheld);
    trk[0].p[P_AHOLD]=0;events_block(CTL);drain_balance();assert(!trk[0].nheld);silence();
}
static void union_owned(void)
{
    uint8_t want[128]={0};unsigned count=0;
    for(unsigned key=0;key<27;key++)if(kb_kind[key]==KS_NOTE&&kb_trk[key]==0)
        for(unsigned i=0;i<kb_n[key];i++)want[kb_nt[key][i]]=1;
    for(unsigned n=0;n<128;n++){count+=want[n];assert(has_pitch(n)==want[n]);}
    assert(trk[0].nheld==count);
}
static void overlap_modifiers(unsigned reverse)
{
    start_case(1,0,1);keys_owned(1u<<7);keys_owned((1u<<7)|(1u<<14));union_owned();
    keys_owned((1u<<7)|(1u<<14)|(1u<<5));union_owned(); /* two chords revoice together */
    keys_owned((1u<<(reverse?7:14))|(1u<<5));union_owned();
    keys_owned(1u<<(reverse?7:14));union_owned();keys_owned(0);silence();assert(!trk[0].nheld);
}
static void transitions(void)
{
    for(unsigned arp=0;arp<2;arp++)for(unsigned hold=0;hold<2;hold++) {
        start_case(arp,hold,1);keys_owned(1u<<7);keys_owned((1u<<7)|(1u<<5));
        trk[0].p[P_AMODE]=arp?0:ARP_UP;events_block(CTL);drain_balance();
        keys_owned(1u<<5);keys_owned(0);
        trk[0].p[P_AHOLD]=0;events_block(CTL);drain_balance();silence();seq_stop();

        start_case(arp,hold,1);keys_owned(1u<<7);
        apply_preset_to(&trk[0],0);events_block(CTL);drain_balance();keys_owned(0);
        trk[0].p[P_AHOLD]=0;events_block(CTL);drain_balance();silence();seq_stop();

        start_case(arp,hold,1);keys_owned(1u<<7);if(hold)keys_owned(0);
        progression_build=progression_chord;assert(progression_arm());drain_balance();silence();
        keys_owned(0);silence();
        song.g[G_MIDI]=1;progression_press(0);drain_balance();
        trk[0].p[P_AMODE]=arp?0:ARP_UP;events_block(CTL);drain_balance();
        progression_stop();drain_balance();silence();seq_stop();
    }
}
int main(int argc,char **argv)
{
    unsigned which=argc>1?(unsigned)atoi(argv[1]):0;
    if(!which||which==2){for(unsigned a=0;a<2;a++)for(unsigned h=0;h<2;h++)for(unsigned r=0;r<2;r++)for(unsigned g=0;g<2;g++)revoice_release(a,h,r,g);}
    if(!which||which==3){for(unsigned t=0;t<5;t++)for(unsigned off=0;off<2;off++)channel_release(t,off);}
    if(!which||which==4){for(unsigned r=0;r<2;r++)for(unsigned h=0;h<2;h++)for(unsigned k=11;k<=14;k+=3)overlap(r,h,k);for(unsigned r=0;r<2;r++)overlap_modifiers(r);}
    if(!which||which==5)transitions();
    puts("sequencing ownership regression PASS");return 0;
}

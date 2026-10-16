/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static void clock_at(uint32_t units)
{clk_beat=units/BEAT_U;clk_pos=units%BEAT_U;}
static void grids(void)
{
    track_t *t=&trk[0];uint32_t into,len,expected_into,expected_len;
    /* Zero settings bit retains every pre-existing track division and amount. */
    swing_eighth=0;
    for(uint32_t div=0;div<NDIV_STEP;div++)for(int amount=-20;amount<=140;amount+=20) {
        t->p[P_SDIV]=div;t->p[P_SSWING]=amount;song.g[G_SWING]=0;
        for(uint32_t tick=0;tick<11*BEAT_U;tick+=BEAT_U/31) {
            clock_at(tick);
            uint32_t expected=grid_at(div,swings(div)?swing_units(amount,div_units(div)):0,&expected_into,&expected_len);
            assert(trk_grid(t,&into,&len)==expected&&into==expected_into&&len==expected_len);
        }
    }
    swing_eighth=1;
    const uint32_t amounts[]={0,1,17,50,99,100};
    for(uint32_t source=0;source<2;source++)for(uint32_t a=0;a<NELEM(amounts);a++)for(uint32_t div=1;div<=3;div++) {
        uint32_t den=DIV_DEN[div],sub=den/2,u=BEAT_U/den,delta=amounts[a]*u/200;
        t->p[P_SDIV]=div;t->p[P_SSWING]=source?amounts[a]:0;song.g[G_SWING]=source?0:amounts[a];
        for(uint32_t beat=0;beat<17;beat++)for(uint32_t k=0;k<den;k++) {
            uint32_t start=beat*BEAT_U+(k<sub?k*(u+delta):sub*(u+delta)+(k-sub)*(u-delta));
            uint32_t duration=k<sub?u+delta:u-delta,absolute=beat*den+k;
            clock_at(start);assert(trk_grid(t,&into,&len)==absolute&&into==0&&len==duration&&len);
            clock_at(start+duration-1);assert(trk_grid(t,&into,&len)==absolute&&into==duration-1);
            if(start) {clock_at(start-1);assert(trk_grid(t,&into,&len)==absolute-1);}
            assert(swing_step_units(div,amounts[a],absolute)==duration);
        }
    }
    for(uint32_t div=0;div<NDIV_STEP;div++)if(!div||div>=4) {
        t->p[P_SDIV]=div;t->p[P_SSWING]=100;song.g[G_SWING]=100;
        for(uint32_t tick=0;tick<5*BEAT_U;tick+=BEAT_U/37) {
            clock_at(tick);uint32_t expected=grid_at(div,0,&expected_into,&expected_len);
            assert(trk_grid(t,&into,&len)==expected&&into==expected_into&&len==expected_len);
        }
    }
    t->p[P_SDIV]=2;t->p[P_SLEN]=14;t->p[P_SSWING]=0;song.g[G_SWING]=100;
    for(uint32_t n=0;n<100;n++) {
        uint32_t sub=n%4,u=BEAT_U/4,start=n/4*BEAT_U+(sub<2?sub*(u+u/2):3*u+(sub-2)*(u/2));
        clock_at(start);assert(trk_grid(t,&into,&len)==n);
    }
    swing_eighth=0;song.g[G_SWING]=0;
    puts("swing grid: legacy equivalence, 1/8/16/32 boundaries, positive subdivisions, clamps and odd loop phase PASS");
}
static void rom(void)
{
    uint32_t h=2166136261u;
    assert(NDRUM_GROOVES==24);
    for(uint32_t g=0;g<NDRUM_GROOVES;g++) {
        const drum_groove_t *p=&DRUM_GROOVES[g];
        for(uint32_t lane=0;lane<7;lane++) {
            uint64_t mask=drum_groove_mask(p,lane);
            for(uint32_t b=0;b<8;b++)h=(h^((mask>>(8*b))&255u))*16777619u;
        }
        h=(h^p->len)*16777619u;h=(h^p->div)*16777619u;
    }
    assert(h==0x6849f44au); /* Frozen original, full-width ROM masks and metadata. */
    puts("groove ROM: all seven masks, lengths and divisions match the original 24 definitions PASS");
}
typedef struct {uint32_t block,note;} event_t;
static event_t straight[512],step_swung[512],eighth[512],preview[512];
static uint32_t render(uint32_t g,uint32_t mode,uint32_t amount,uint32_t audition,int32_t feel,event_t *events)
{
    seq_stop();host_tracks_init();
    for(uint32_t t=0;t<NTRK;t++)steps_clear(&trk[t]);
    memset(&drums,0,sizeof drums);drums.set=-2;
    song.g[G_BPM]=120;song.g[G_CLOCK]=0;song.g[G_SWING]=amount;swing_eighth=mode;
    song.sel=TRK_DRUM;song.rec=transport_req=rec_wait=ft_on=0;
    fm1_in.notes=fm1_in.buttons=0;panic_req=0;
    rhythm_shape_t shape={0,0,(int8_t)feel,255,0};
    assert(drum_groove_apply_shaped(g,&shape));
    if(audition) {
        groove_preview_id=g;groove_preview_read=drum_groove_preview_step;
        groove_shape=shape;groove_preview.feel=(int8_t)feel;
        groove_preview.step=0;groove_preview.first=1;groove_preview.phase=-feel*(int32_t)swing_step_units(DRUM_GROOVES[g].div,amount,0)/64;
        groove_preview.len=DRUM_GROOVES[g].len;groove_preview.div=DRUM_GROOVES[g].div;
        groove_preview.active=1;
    } else seq_start();
    uint32_t count=0;int32_t out[CTL*2];
    for(uint32_t block=0;block<6*FS/CTL;block++) {
        uint32_t age=drums.age;mix_block(out,CTL);
        for(uint32_t next=age+1;next<=drums.age;next++)for(uint32_t lane=0;lane<NDRUM;lane++)
            if(drums.v[lane].age==next) {
                assert(count<512);events[count++]=(event_t){block,drums.v[lane].note};
            }
    }
    seq_stop();groove_preview.active=0;return count;
}
static void playback(void)
{
    uint32_t n=render(3,0,0,0,0,straight);
    assert(render(3,0,100,0,0,step_swung)==n && !memcmp(straight,step_swung,n*sizeof *straight));
    assert(render(3,1,100,0,0,eighth)==n);
    uint32_t moved=0,hat=0;
    for(uint32_t i=0;i<n;i++) {
        assert(straight[i].note==eighth[i].note);moved+=straight[i].block!=eighth[i].block;
        if(eighth[i].note==LANE_NOTE[4]) {
            uint32_t expected=(hat/2)*FS/2+(hat&1u?3*FS/8:0);
            int32_t error=(int32_t)(eighth[i].block*CTL)-(int32_t)expected;
            assert(error>=-(int32_t)CTL && error<=(int32_t)CTL);hat++;
        }
    }
    assert(moved && hat>=16);
    for(int32_t feel=-16;feel<=16;feel+=16) for(uint32_t g=3;g<=23;g+=20) { /* HALF TIME and 14-step SEVEN SKIP */
        n=render(g,1,100,0,feel,eighth);assert(render(g,1,100,1,feel,preview)==n);
        for(uint32_t i=0;i<n;i++) {
            assert(eighth[i].note==preview[i].note);
            int32_t delta=(int32_t)eighth[i].block-(int32_t)preview[i].block;
            if(delta < -1 || delta > 1) fprintf(stderr,"preview mismatch g=%u feel=%d i=%u note=%u apply=%u preview=%u delta=%d\n",g,feel,i,eighth[i].note,eighth[i].block,preview[i].block,delta);
            assert(delta>=-1&&delta<=1);
        }
    }
    swing_eighth=0;song.g[G_SWING]=0;
    puts("HALF TIME: eighth hats move to 0/375/500/875ms; STEP compatibility and 14-step preview/Apply parity PASS");
}
static void display_and_settings(void)
{
    seq_stop();swing_eighth=0;uint32_t original=lights_word();
    ui.menu=1;ui.menu_sel=MI_SWING;encs[panel.enc[EN_K1]]=1;frame();
    assert(swing_eighth && settings_later);
    uint32_t saved=lights_word();assert((saved^original)==(1u<<24));
    swing_eighth=0;lights_from_word(saved);assert(swing_eighth);
    lights_from_word(saved&~(1u<<24));assert(!swing_eighth);
    encs[panel.enc[EN_K1]]=1;frame();ui.force=1;frame();outdir="build/host";ppm("swing-grid-eighth");
    ui.menu=0;studio_open(SC_TRK);song.g[G_SWING]=0;
    encs[panel.enc[EN_K1]]=100;frame();assert(song.g[G_SWING]==100);
    for(uint32_t t=0;t<NTRK;t++) {steps_clear(&trk[t]);trk[t].p[P_SLEN]=16;trk[t].p[P_SDIV]=2;trk[t].p[P_SSWING]=0;}
    for(uint32_t i=0;i<NSTEP;i++)TDRUM->dstep[i]=drum_groove_step(3,i);
    clock_at(0);
    for(uint32_t mode=0;mode<2;mode++)for(uint32_t amount=0;amount<=100;amount+=50) {
        swing_eighth=mode;song.g[G_SWING]=amount;cv_begin(240,124,C_BLACK);cv_oy=-144;vis_groove();
        /* Step 3's eighth-note hat: unchanged in STEP, offset by 28px at max eighth swing. */
        uint32_t x=70+(mode?amount*28/100:0);
        assert(cv_px[65*240+x]==swap16(mix565(TE_DIM[3],TE_COL[3],200)));
    }
    cv_oy=0;ui.menu=0;swing_eighth=1;song.g[G_SWING]=100;
    vis_style=10;vis_name_t=30;vis_on=1;ui.msg_t=0;ui.force=1;vis_draw();ppm("swing-halftime-eighth");
    vis_on=0;swing_eighth=0;song.g[G_SWING]=0;
    puts("swing controls: physical menu knob, old settings default, flag round-trip, home amount and HALF TIME visualizer PASS");
}
int main(void)
{
    panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(4);
    rom();grids();playback();display_and_settings();return 0;
}

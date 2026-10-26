/* SPDX-License-Identifier: GPL-3.0-only */
/* Run the production audio ISR with controlled hardware-clock/DMA readings.
 * Render, allocation, fade and recovery are real; times are injected, not benchmarks. */
#ifndef FELUCCA_AUDIO_DIAG
#define FELUCCA_AUDIO_DIAG 1
#endif
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>
#define FM1_TICKS_PER_US 24u
#define FM1_AUDIO_HALF 0x80u
static uint32_t elapsed_us,nested_us,clock_reads,half_reads,simulate_late;
static uint32_t fm1_ticks(void);
static uint8_t fm1_audio_pending(void) {return FM1_AUDIO_HALF;}
static void fm1_audio_ack_aux(uint8_t p) {(void)p;}
static uint32_t fm1_audio_free_half(void) {return half_reads++?simulate_late:0;}
static void fm1_audio_ack_half(void) {}
static void fm1_audio_init(int32_t *b,uint32_t n,void (*isr)(void),uint32_t p) {(void)b;(void)n;(void)isr;(void)p;}
#include "../firmware/src/audio.c"
void isr_alnk0(void) {}
static uint32_t fm1_ticks(void) {
    if(!clock_reads++)return 0;
    t5_nested_ticks=nested_us*FM1_TICKS_PER_US;
    return elapsed_us*FM1_TICKS_PER_US;
}
static void half(uint32_t us,uint32_t nested,uint32_t late) {
    elapsed_us=us;nested_us=nested;simulate_late=late;clock_reads=half_reads=0;
    fm1_alnk0_irq();assert(!felucca_dbg.in_audio);
}
static void reset(void) {
    seq_stop();memset(trk,0,sizeof trk);host_tracks_init();
    voice_expand=4;voice_kills=shed_count=shed_req=shed_over=0;
    memset(&felucca_dbg,0,sizeof felucca_dbg);
#if FELUCCA_AUDIO_DIAG
    audio_wall_max=0;
#endif
    host_preset(&trk[2],3,0);trk[2].p[P_VOICE]=V_POLY;trk[2].p[P_AMODE]=0;
    for(uint32_t n=60;n<=67;n+=3)trk_note_on(&trk[2],n,100);
}
int main(void) {
    const uint32_t deadline=HALF_FRAMES*1000000u/FS;
    reset();
    for(uint32_t i=0;i<200;i++)half(5373,0,0);
#if FELUCCA_AUDIO_DIAG
    assert(!shed_count&&!voice_kills&&!shed_req&&audio_wall_max==5373&&!felucca_dbg.late);
    assert(voices_busy()==3);
#else
    assert(!shed_count&&!voice_kills&&!shed_req&&!felucca_dbg.late&&voices_busy()==3); /* normal builds retain the successful policy too. */
#endif /* Reported device peak must not thin the drone. */
    reset();uint32_t edge=deadline*AUDIO_SHED_PERCENT/100u;
    half(edge,0,0);half(edge,0,0);assert(!shed_req);
    half(edge+1,0,0);assert(!shed_req);half(edge+1,0,0);assert(shed_req&&!shed_count);
    half(1000,0,0);assert(shed_count==1&&voice_kills==1&&!shed_req);
    assert(trk[2].v[0].gate); /* existing lowest-note protection remains */
    reset();half(deadline+1000,0,1);half(1000,0,0);half(1000,0,0);
    assert(!shed_count&&felucca_dbg.late==1); /* isolated spike: no deliberate note cutoff */
    reset();half(5600,3100,0);half(5600,3100,0);assert(shed_req);
    assert(felucca_dbg.last_us==2500);
#if FELUCCA_AUDIO_DIAG
    assert(audio_wall_max==5600);
#endif
    half(1000,0,0);assert(shed_count==1); /* wall-time guard includes nested USB/timer work */
    reset();half(deadline+1,0,1);half(deadline+1,0,1);half(deadline+1,0,1);
    assert(shed_count==1&&shed_req&&felucca_dbg.late==3); /* sustained real overload remains protected */
    half(1000,0,0);assert(shed_count==2&&!shed_req);half(1000,0,0);assert(shed_count==2);
    puts("audio ISR: reported 5373us peak, normal/diagnostic threshold boundary, consecutive/isolated spikes, nested time, real shedding and DMA diagnostics PASS");
}

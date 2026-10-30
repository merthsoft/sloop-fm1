/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual ISR/owner recovery under controlled full-wall times; no host timing claim. */
#define FELUCCA_AUDIO_DIAG 1
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>
#define FM1_TICKS_PER_US 24u
#define FM1_AUDIO_HALF 0x80u
static uint32_t elapsed_us,clock_reads,half_reads,late;
static uint32_t fm1_ticks(void);
static uint8_t fm1_audio_pending(void){return FM1_AUDIO_HALF;}
static void fm1_audio_ack_aux(uint8_t p){(void)p;}
static uint32_t fm1_audio_free_half(void){return half_reads++?late:0;}
static void fm1_audio_ack_half(void){}
static void fm1_audio_init(int32_t*b,uint32_t n,void(*f)(void),uint32_t p){(void)b;(void)n;(void)f;(void)p;}
#include "../firmware/src/audio.c"
void isr_alnk0(void){}
static uint32_t fm1_ticks(void){return clock_reads++?elapsed_us*FM1_TICKS_PER_US:0;}
static void half(uint32_t us){elapsed_us=us;clock_reads=half_reads=0;fm1_alnk0_irq();}
static void safe(uint32_t halves){for(uint32_t i=0;i<halves;i++)half(4000);}
static uint32_t held(void){uint32_t n=0;for(uint32_t i=0;i<NVOICE;i++)n+=trk[1].v[i].active&&trk[1].v[i].gate&&trk[1].v[i].stage!=4;return n;}
static void setup(uint32_t engine){
 seq_stop();memset(trk,0,sizeof trk);host_tracks_init();
 memset(kb_kind,0,sizeof kb_kind);fm1_in.buttons=0;ly_lock=LY_PLAY;kb_grid=0;song.sel=1;
 memset(midi_scale_held,0,sizeof midi_scale_held);memset(drone_lost,0,sizeof drone_lost);
 memset(drift_p,0,sizeof drift_p);memset(evolve_p,0,sizeof evolve_p);
 cpu_lost_mask=cpu_recover_safe=cpu_recover_backoff=voice_recover_wait=voice_kills=shed_count=shed_req=shed_over=0;
 late=0;voice_expand=4;song.playing=song.rec=0;cpu1_units=-1;
 track_t*t=&trk[1];host_preset(t,engine,engine==8?2:0);t->p[P_VOICE]=V_POLY;
 t->p[P_ATK]=t->p[P_DEC]=t->p[P_REL]=0;t->p[P_SUS]=127;t->p[P_AMODE]=t->p[P_CHORD]=0;
 song.g[G_LTYPE]=1;song.g[G_VWOW]=song.g[G_VTONE]=song.g[G_VCRACKLE]=64;
}
static void midi_chord(void){for(uint32_t j=0;j<3;j++)midi_scale_input(&trk[1],60+j*3,60+j*3,67+j,1);}
static void shed(void){half(5600);half(5600);assert(shed_req);half(4000);assert(shed_count==1&&held()==2);}
int main(void){
 setup(5);midi_chord();shed();safe(50);assert(held()==2);safe(60);assert(held()==3&&shed_count==1&&cpu_recover_backoff==1);
 setup(5);key_down(0);key_down(3);key_down(7);assert(held()==3);shed();safe(110);assert(held()==3);key_up(0);key_up(3);key_up(7);safe(110);assert(!held());
 /* Continued insufficient margin cannot re-admit, even below shed threshold. */
 setup(5);midi_chord();shed();for(uint32_t i=0;i<500;i++)half(5000);assert(held()==2);safe(110);assert(held()==3);
 /* Renewed overload backs off the next readmission, without disabling shedding. */
 half(5600);half(5600);half(4000);assert(held()==2&&shed_count==2);safe(110);assert(held()==2);safe(90);assert(held()==3);
 setup(5);midi_chord();for(uint32_t i=0;i<200;i++)half(5600);assert(held()==1);uint32_t kills=shed_count;for(uint32_t i=0;i<100;i++)half(5600);assert(held()==1&&shed_count==kills);
 setup(5);midi_chord();shed();late=1;safe(250);assert(held()==2&&!cpu_recover_safe);late=0;safe(110);assert(held()==3);
 setup(5);midi_chord();shed();trk[1].p[P_SUS]=0;safe(1);trk[1].p[P_SUS]=127;safe(250);assert(held()==2&&!cpu_lost_mask);
 /* A real MIDI release during cooldown cancels the original owner. */
 setup(5);midi_chord();shed();midi_scale_input(&trk[1],63,63,0,0);safe(250);assert(held()==2);
 /* A new note-on supersedes the pending generation/velocity. */
 setup(5);midi_chord();shed();midi_scale_input(&trk[1],63,63,111,1);safe(250);assert(held()==3);for(uint32_t i=0;i<NVOICE;i++)if(trk[1].v[i].gate&&trk[1].v[i].note==63)assert(trk[1].v[i].vel==111);
 /* Unowned direct host attacks are deliberately never resurrected. */
 setup(5);for(uint32_t j=0;j<3;j++)trk_note_on(&trk[1],60+j*3,100);shed();safe(250);assert(held()==2);
 /* STOP/alloff and queued engine changes cancel all pending pitches. */
 setup(5);midi_chord();shed();trk_all_off(&trk[1]);safe(250);assert(!held());
 setup(5);midi_chord();shed();trk[1].eng_req=0;safe(250);assert(!held()&&trk[1].engine==0);
 /* GRAIN sample-generation and source changes cancel; natural SAMPLE never queues. */
 setup(8);midi_chord();shed();smp_user_gen++;safe(250);assert(held()==2);
 setup(8);midi_chord();shed();trk[1].p[P_E0]++;safe(1);trk[1].p[P_E0]--;safe(250);assert(held()==2);
 setup(4);midi_chord();half(5600);half(5600);half(4000);assert(!cpu_lost_mask);safe(250);assert(!cpu_lost_mask);
 /* Ordinary budget/local-cap stealing is not a CPU recovery request. */
 setup(5);midi_chord();trk_note_on(&trk[1],70,100);trk_note_on(&trk[1],74,100);assert(!cpu_lost_mask);safe(250);assert(!cpu_lost_mask);
 setup(5);midi_chord();voice_expand=0;for(uint32_t p=0;p<3;p+=2){host_preset(&trk[p],0,0);trk[p].p[P_VOICE]=V_POLY;for(uint32_t n=0;n<4;n++)trk_note_on(&trk[p],40+p*12+n*3,100);}assert(voice_kills&&!cpu_lost_mask);safe(250);assert(!cpu_lost_mask);
 /* Real sequencer owner is recoverable, but a gate/step release cancels it. */
 setup(5);step_t st={0};st.time=ST_NOTE;st.n=3;st.vel=89;st.flags=SF_SLIDE;st.note[0]=60;st.note[1]=63;st.note[2]=66;seq_step(&trk[1],&st,BEAT_U,0);shed();safe(110);assert(held()==3);
 setup(5);seq_step(&trk[1],&st,BEAT_U,0);shed();seq_release(&trk[1]);safe(250);assert(!held());
 setup(5);trk[1].p[P_AMODE]=1;trk[1].nheld=1;trk[1].held[0]=63;for(uint32_t j=0;j<3;j++)generated_emit(&trk[1],60+j*3,100,0);shed_voice();assert(held()==2);arp_release(&trk[1]);cpu_recover_safe=FS;drone_recover(FS);drone_recover(CTL);assert(!held()&&!cpu_lost_mask);
 puts("CPU-shed recovery: real MIDI/seq owners, actual ISR margin/cooldown/backoff, release/newattack/alloff/engine/source cancellation, no budget recovery PASS");
}

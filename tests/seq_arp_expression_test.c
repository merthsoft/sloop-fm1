/* SPDX-License-Identifier: GPL-3.0-only */
#undef NDEBUG
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
static unsigned preview_stops;
static void preview_stop(void) { preview_stops++; }

static void reset(void)
{
    seq_stop(); host_tracks_init();
    memset(seq_harmony, 0, sizeof seq_harmony);
    song.playing = song.rec = 0; song.g[G_MIDI] = 1; usb.config = 1;
    trk[0].p[P_AMODE] = ARP_UP; trk[0].p[P_AORDER] = AORDER_SEQ_NOTE;
    trk[0].p[P_AOCT] = 2; events_block(0);
    mo_r = mo_w = 0;
}
static unsigned velocity_at(unsigned pitch)
{
    uint8_t notes[64], velocity[64];
    unsigned n = arp_expression_list(&trk[0], notes, velocity);
    for (unsigned i=0;i<n;i++) if(notes[i]==pitch) return velocity[i];
    return 0;
}
int main(void)
{
    step_t s = {.n=3,.note={60,64,67},.vel=80,.lvl= (1u<<2)|(3u<<4)};
    step_t tie = {.time=ST_TIE}, rest = {.time=ST_REST};
    reset(); seq_step(&trk[0], &s, BEAT_U/4, 0);
    for(unsigned i=0;i<3;i++) {
        assert(velocity_at(s.note[i])==step_vel(&s,i));
        assert(velocity_at(s.note[i]+12)==step_vel(&s,i));
    }
    input_on(&trk[0],60,111); assert(velocity_at(60)==111);
    input_off(&trk[0],60); assert(velocity_at(60)==80);
    input_on(&trk[0],60,25); assert(velocity_at(60)==80);
    seq_step(&trk[0],&rest,BEAT_U/4,0); assert(velocity_at(60)==25);
    input_on(&trk[0],60,52); assert(velocity_at(60)==52);
    input_on(&trk[0],65,71); input_off(&trk[0],60); assert(velocity_at(65)==71);
    reset(); input_on(&trk[0],65,71);
    trk[0].p[P_AHOLD]=1; input_off(&trk[0],65); assert(velocity_at(65)==71);
    input_on(&trk[0],62,39); assert(!velocity_at(65)&&velocity_at(62)==39);
    s.flags=SF_ACCENT; seq_step(&trk[0],&s,BEAT_U/4,0);
    seq_step(&trk[0],&tie,BEAT_U/4,0); assert(velocity_at(64)==step_vel(&s,1));
    /* Every selector carries the velocity of the selected expanded tone. */
    for(unsigned mode=ARP_UP;mode<=ARP_WALK;mode++) {
        trk[0].p[P_AMODE]=mode; trk[0].arp_idx=0xffffffffu;
        for(unsigned hit=0;hit<70;hit++) {
            uint32_t vel=0, note=arp_next_velocity(&trk[0],&vel);
            assert(vel==velocity_at(note));
        }
    }
    /* Production pulse audio and generated MIDI use the same expression. */
    reset(); trk[0].p[P_AMODE]=ARP_PULSE; seq_step(&trk[0],&s,BEAT_U/4,0);
    arp_tick(&trk[0],0); unsigned hits=0;
    while(mo_r!=mo_w) {
        uint32_t packet=midi_out_q[mo_r++%MOUT_Q];
        if(((packet>>8)&0xf0)==0x90) {
            unsigned note=(packet>>16)&127, vel=(packet>>24)&127;
            assert(vel==velocity_at(note)); hits++;
        }
    }
    assert(hits==6);
    for(unsigned i=0;i<NVOICE;i++)
        if(trk[0].v[i].gate)
            assert(trk[0].v[i].vel==velocity_at(trk[0].v[i].note));
    /* Expanded overlaps, including nonadjacent PLAY-order duplicates, use max. */
    reset(); trk[0].p[P_AMODE]=ARP_PULSE; trk[0].p[P_AORDER]=AORDER_SEQ_PLAY;
    input_on(&trk[0],72,23); input_on(&trk[0],60,113);
    arp_tick(&trk[0],0); hits=0;
    while(mo_r!=mo_w) {
        uint32_t packet=midi_out_q[mo_r++%MOUT_Q];
        if(((packet>>8)&0xf0)==0x90) {
            unsigned note=(packet>>16)&127;
            if(note==72) { assert(((packet>>24)&127)==113); hits++; }
        }
    }
    assert(hits==1);
    reset(); trk[0].p[P_AMODE]=ARP_PULSE; trk[0].p[P_AOCT]=4;
    input_on(&trk[0],126,19); input_on(&trk[0],127,107);
    arp_tick(&trk[0],0); hits=0;
    while(mo_r!=mo_w) {
        uint32_t packet=midi_out_q[mo_r++%MOUT_Q];
        if(((packet>>8)&0xf0)==0x90 && ((packet>>16)&127)==127) {
            assert(((packet>>24)&127)==107); hits++;
        }
    }
    assert(hits==1);
    /* A full live pool still merges a stronger existing sequence pitch. */
    reset(); for(unsigned i=0;i<16;i++) input_on(&trk[0],50+i,20);
    s=(step_t){.n=2,.note={50,90},.vel=99}; seq_step(&trk[0],&s,BEAT_U/4,0);
    assert(velocity_at(50)==99 && !velocity_at(90));
    reset(); s=(step_t){.n=2,.note={60,60},.lvl=1};
    seq_step(&trk[0],&s,BEAT_U/4,0); assert(velocity_at(60)==96);
    reset();
    midi_in_q[mi_w++%MQ]=0x09u | (0x90u<<8) | (60u<<16) | (43u<<24);
    midi_in_q[mi_w++%MQ]=0x19u | (0x90u<<8) | (67u<<16) | (109u<<24);
    events_block(0); assert(velocity_at(60)==43 && velocity_at(67)==109);
    reset(); arp_add(&trk[0],60); assert(velocity_at(60)==100);
    sequence_preview_end=preview_stop; input_on(&trk[0],61,37);
    assert(preview_stops==1 && velocity_at(61)==37); sequence_preview_end=0;
    s=(step_t){.n=3,.note={60,64,67},.vel=80,.lvl=(1u<<2)|(3u<<4),.flags=SF_ACCENT};
    /* Route toggles retain the sounding snapshot, even if source storage edits. */
    reset(); trk[0].p[P_AOCT]=1; trk[0].step[0]=s;
    seq_start(); events_block(0); trk[0].step[0].vel=7;
    trk[0].p[P_AORDER]=AORDER_NOTE; events_block(0);
    assert(trk[0].seq_n==3 && seq_direct_vel[0][1]==step_vel(&s,1));
    trk[0].p[P_AORDER]=AORDER_SEQ_PLAY; events_block(0);
    assert(velocity_at(64)==step_vel(&s,1));
    /* Recording retains the existing velocity-to-level quantization. */
    reset(); song.playing=1; song.rec=1; trk[0].seq_idx=0;
    arp_emit(&trk[0],60,33);
    assert(trk[0].step[0].n==1 && (trk[0].step[0].lvl&3)==vel_lvl(33));
    puts("arp expression: PASS"); return 0;
}

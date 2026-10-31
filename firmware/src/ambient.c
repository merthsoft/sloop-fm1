/* SPDX-License-Identifier: GPL-3.0-only */
/* Independent motion, prepared once by each part mixer before rendering. */
/* Shared timing for free-running motion and one-shot transitions. */
static __attribute__((minsize,noinline)) uint32_t motion_phase(uint32_t rate)
{
    uint32_t period=(EVOLVE_UNITS[rate-8u]/8u)*(BEAT_U/12u);
    uint32_t clock=(clk_beat&63u)*BEAT_U+clk_pos;
    return (uint32_t)(((uint64_t)(clock%period)<<32)/period);
}
static uint32_t motion_step(uint32_t rate){return 3116529u/EVOLVE_SECONDS[rate];}
#include "snapshot.c"
static struct {uint32_t phase,last;uint8_t playing;} snapshot_run[NPART];
static struct { uint32_t phase; int32_t rng; int16_t from,to,value,depth; uint8_t target,state,shape; } drift[NPART];
static __attribute__((noinline)) void drift_tick(track_t *t)
{
    uint32_t part=(uint32_t)(t-trk);
    if(part>=NPART)return;
    int16_t *p=drift_p[part];
    uint32_t shape=p[0]?(uint32_t)p[0]-1u:drift[part].shape;
    int32_t target=p[0] && drift[part].target==p[3] && drift[part].shape==shape ? clamp(p[2],0,127)*128 : 0;
    drift[part].depth+=(int16_t)clamp(target-drift[part].depth,-32,32);
    if(!drift[part].depth){drift[part].target=(uint8_t)p[3];drift[part].shape=(uint8_t)shape;}
    if(!p[0] && !drift[part].depth){drift[part].value=0;drift[part].state=255;return;}
    if(!drift[part].rng) {
        drift[part].rng=(int32_t)(0x3156494eu+part);
        drift[part].to=(int16_t)(noise32(&drift[part].rng)>>16);
    }
    uint32_t old=drift[part].phase,rate=(uint32_t)clamp(p[1],0,NELEM(N_EVOLVE_CYCLE)-1);
    uint32_t state=rate | (song.playing?32u:0u) | (p[0]?64u:0u);
    if(rate>=NELEM(EVOLVE_SECONDS)) {
        uint32_t phase=song.playing?motion_phase(rate):old;
        if(drift[part].state!=state) {
            int32_t current=drift[part].from+(((int32_t)drift[part].to-drift[part].from)*(int32_t)(old>>17)>>15);
            drift[part].from=drift[part].to=(int16_t)current;
            /* Join an in-progress cycle without jumping; begin at its next boundary. */
            if(song.playing && !phase)drift[part].to=(int16_t)(noise32(&drift[part].rng)>>16);
        } else if(song.playing && phase<old) {
            drift[part].from=drift[part].to;
            drift[part].to=(int16_t)(noise32(&drift[part].rng)>>16);
        }
        drift[part].phase=phase;
    } else {
        drift[part].phase+=motion_step(rate);
        if(drift[part].phase<old) {
            drift[part].from=drift[part].to;
            drift[part].to=(int16_t)(noise32(&drift[part].rng)>>16);
        }
    }
    drift[part].state=(uint8_t)state;
    int32_t value=drift[part].from+(((int32_t)drift[part].to-drift[part].from)*(int32_t)(drift[part].phase>>17)>>15);
    if(drift[part].shape)value=modulation_wave(drift[part].shape-1u,drift[part].phase,0);
    drift[part].value=(int16_t)((value*drift[part].depth)>>15);
}
static int32_t drift_mod(const track_t *t,uint32_t target)
{
    uint32_t part=(uint32_t)(t-trk);
    static const uint8_t masks[]={1,2,4,3,5,6,7,0};
    return part<NPART && drift[part].target<NELEM(masks) && (masks[drift[part].target]&(1u<<target)) ? drift[part].value : 0;
}

/* Cross-source/patch audio crossfades need separate voices. Automatic parameter
 * morphing therefore requires matching source identities at both endpoints. */
static __attribute__((minsize,noinline)) int snapshot_ready(const track_t *t,uint32_t part)
{
    if(part>=NPART || snapshot_p[part][3]!=3)return 0;
    if(snapshot_p[part][4]>1)return 1;
    uint32_t id=t->engine==ENGI_FM6?P_E7:P_E0;
    if((1u<<t->engine)&((1u<<4)|(1u<<ENGI_GRAIN)|(1u<<ENGI_FM6)|(1u<<ENGI_PHYS)))
        return snapshot_values[part][0][id]==snapshot_values[part][1][id] &&
               t->p[id]==snapshot_values[part][0][id];
    return 1;
}
static __attribute__((minsize,noinline)) int32_t snapshot_position(uint32_t part)
{
    return clamp(snapshot_p[part][0]+(drift[part].target==DRIFT_MORPH?(drift[part].value>>8):0),0,127);
}

/* Apply once per control block, including silent parts. Completion commits the
 * endpoint as the ordinary track sound; there is no retained audio overlay. */
static __attribute__((minsize,noinline)) void snapshot_tick(track_t *t)
{
    uint32_t part=(uint32_t)(t-trk);int16_t *p=snapshot_p[part];
    if(!p[5])return;
    if(t->xf_on || t->engine!=t->eng_req){snapshot_run[part].playing=0;return;}
    if(!snapshot_ready(t,part)){p[5]=0;return;}
    uint32_t rate=(uint32_t)p[6],old=snapshot_run[part].phase,step;
    if(rate<8)step=motion_step(rate);
    else {
        uint32_t phase=motion_phase(rate);
        step=song.playing && snapshot_run[part].playing?phase-snapshot_run[part].last:0;
        snapshot_run[part].last=phase;snapshot_run[part].playing=(uint8_t)song.playing;
    }
    uint32_t phase=old+step;
    /* Reserve the exact endpoint for completion, even on a 160-second run. */
    int32_t position=phase<old?127:(int32_t)(((phase>>17)*127u)>>15);
    snapshot_run[part].phase=phase;
    if(p[5]==2)position=127-position;
    if(position!=p[0])snapshot_apply(t,position,0);
    p[0]=(int16_t)position;
    if(phase<old)p[5]=0;
}

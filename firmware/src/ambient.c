/* SPDX-License-Identifier: GPL-3.0-only */
/* Independent motion, prepared once by each part mixer before rendering. */
#include "snapshot.c"
static struct { uint32_t phase; int32_t rng; int16_t from,to,value,depth; uint8_t target,state; } drift[NPART];
static __attribute__((noinline)) void drift_tick(track_t *t)
{
    uint32_t part=(uint32_t)(t-trk);
    if(part>=NPART)return;
    int16_t *p=drift_p[part];
    int32_t target=p[0] && drift[part].target==p[3] ? clamp(p[2],0,127)*128 : 0;
    drift[part].depth+=(int16_t)clamp(target-drift[part].depth,-32,32);
    if(!drift[part].depth && drift[part].target!=p[3])drift[part].target=(uint8_t)p[3];
    if(!p[0] && !drift[part].depth){drift[part].value=0;drift[part].state=255;return;}
    if(!drift[part].rng) {
        drift[part].rng=(int32_t)(0x3156494eu+part);
        drift[part].to=(int16_t)(noise32(&drift[part].rng)>>16);
    }
    uint32_t old=drift[part].phase,rate=(uint32_t)clamp(p[1],0,NELEM(N_EVOLVE_CYCLE)-1);
    uint32_t state=rate | (song.playing?32u:0u) | (p[0]?64u:0u);
    if(rate>=NELEM(EVOLVE_SECONDS)) {
        uint32_t period=(EVOLVE_UNITS[rate-8u]/8u)*(BEAT_U/12u);
        uint32_t clock=(clk_beat&63u)*BEAT_U+clk_pos;
        uint32_t phase=song.playing?(uint32_t)(((uint64_t)(clock%period)<<32)/period):old;
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
        drift[part].phase+=3116529u/EVOLVE_SECONDS[rate];
        if(drift[part].phase<old) {
            drift[part].from=drift[part].to;
            drift[part].to=(int16_t)(noise32(&drift[part].rng)>>16);
        }
    }
    drift[part].state=(uint8_t)state;
    int32_t value=drift[part].from+(((int32_t)drift[part].to-drift[part].from)*(int32_t)(drift[part].phase>>17)>>15);
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

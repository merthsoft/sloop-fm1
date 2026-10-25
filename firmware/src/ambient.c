/* SPDX-License-Identifier: GPL-3.0-only */
/* Independent, bounded runtime motion. Called by each track's render owner. */
static struct { uint32_t phase; int32_t rng; int16_t from,to,value,depth; uint8_t target; } drift[NPART];
static void drift_tick(track_t *t)
{
    uint32_t part=(uint32_t)(t-trk);
    if(part>=NPART)return;
    int16_t *p=drift_p[part];
    int32_t target=(p[0]&1) && drift[part].target==p[3] ? clamp(p[2],0,127)*128 : 0;
    drift[part].depth+=(int16_t)clamp(target-drift[part].depth,-32,32);
    if(!drift[part].depth && drift[part].target!=p[3])drift[part].target=(uint8_t)p[3];
    if(!(p[0]&1) && !drift[part].depth){drift[part].value=0;return;}
    if(!drift[part].rng) {
        drift[part].rng=(int32_t)(0x3156494eu+part);
        drift[part].to=(int16_t)(noise32(&drift[part].rng)>>16);
    }
    uint32_t old=drift[part].phase;
    drift[part].phase+=623305u>>(uint32_t)clamp(p[1],0,5);
    if(drift[part].phase<old) {
        drift[part].from=drift[part].to;
        drift[part].to=(int16_t)(noise32(&drift[part].rng)>>16);
    }
    int32_t value=drift[part].from+(((int32_t)drift[part].to-drift[part].from)*(int32_t)(drift[part].phase>>17)>>15);
    drift[part].value=(int16_t)((value*drift[part].depth)>>15);
}
static int32_t drift_mod(const track_t *t,uint32_t target)
{
    uint32_t part=(uint32_t)(t-trk);
    return part<NPART && drift[part].target==target ? drift[part].value : 0;
}

/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared manual/render-time A/B parameter interpolation. */
static __attribute__((minsize,noinline)) int snapshot_param(uint32_t id)
{
    return id<=P_LD_AMP || (id>=P_DIST && id<=P_REV) ||
           id==P_GLIDE || id==P_PAN || id==P_DETUNE || id==P_TFLT || id>=P_ITYPE;
}

/* Locks keep their instantaneous sound, with the morphed value as their new
 * base. Capturing under a lock likewise captures the underlying sound. */
static __attribute__((minsize,noinline)) int16_t *snapshot_base(track_t *t,uint32_t id)
{
    for(uint32_t k=0;k<t->lk_n && k<NLOCK;k++)
        if(t->lk_param[k]==id && t->p[id]==t->lk_set[k])return &t->lk_base[k];
    return &t->p[id];
}

static __attribute__((minsize,noinline)) void snapshot_apply(track_t *t,int32_t position,int overlay)
{
    uint32_t part=(uint32_t)(t-trk);
    for(uint32_t id=0;id<P_COUNT;id++)if(snapshot_param(id)) {
        int32_t a=snapshot_values[part][0][id],b=snapshot_values[part][1][id];
        /* Most captured controls are identical. Avoid lock/descriptor lookup and
         * interpolation when the temporary overlay already has that value. */
        if(overlay && a==b && t->p[id]==a)continue;
        int16_t *base=snapshot_base(t,id);
        if(overlay && base!=&t->p[id])continue; /* Active locks override modulation. */
        if(a==b)*base=(int16_t)a;
        else {
            const param_desc_t *d=id>=P_E0?track_desc(t,id):&TP[id];
            *base=(int16_t)(d->names?(position<64?a:b):a+(b-a)*position/127);
        }
    }
}

/* The normal dispatcher can predict LFO wraps if a morph leaves both timing
 * controls at the values it is about to render (including an active lock). */
static __attribute__((minsize,noinline)) int snapshot_lfo_stable(uint32_t part,const int16_t *p)
{
    return snapshot_values[part][0][P_LRATE]==p[P_LRATE] &&
           snapshot_values[part][1][P_LRATE]==p[P_LRATE] &&
           snapshot_values[part][0][P_LSYNC]==p[P_LSYNC] &&
           snapshot_values[part][1][P_LSYNC]==p[P_LSYNC];
}

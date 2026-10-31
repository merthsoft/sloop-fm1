/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared manual/render-time A/B parameter interpolation. */
static __attribute__((minsize,noinline)) int snapshot_param(uint32_t id)
{
    return id<=P_LD_AMP || (id>=P_DIST && id<=P_REV) ||
           id==P_GLIDE || id==P_PAN || id==P_DETUNE || id==P_TFLT || id>=P_ITYPE;
}

/* Scope is orthogonal to the stored capture: never discard unselected values. */
static __attribute__((minsize,noinline)) int snapshot_selected(const track_t *t,uint32_t id)
{
    uint32_t scope=(uint32_t)snapshot_p[t-trk][4];
    if(!scope)return 1;
    if(scope==1)return id>=P_E0 || id==P_GLIDE || id==P_DETUNE;
    if(scope==2)return id==P_TFLT || id==P_FTYPE ||
        (t->engine==0 && (id==P_E4 || id==P_E5 || id==P_E7)) ||
        (t->engine==3 && id==P_E7);
    if(scope==3)return (id>=P_ATK && id<=P_ED_SHP) || id==P_ESYNC;
    if(scope==4)return (id>=P_LRATE && id<=P_LD_AMP) || (id>=P_LSYNC && id<=P_LPOL);
    return id==P_LEVEL || id==P_ED_FX || id==P_PAN ||
        (id>=P_DIST && id<=P_REV) || (id>=P_ITYPE && id<=P_IMIX);
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
    for(uint32_t id=0;id<P_COUNT;id++)if(snapshot_param(id) && snapshot_selected(t,id)) {
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
    if(snapshot_p[part][4] && snapshot_p[part][4]!=4)return 1;
    return snapshot_values[part][0][P_LRATE]==p[P_LRATE] &&
           snapshot_values[part][1][P_LRATE]==p[P_LRATE] &&
           snapshot_values[part][0][P_LSYNC]==p[P_LSYNC] &&
           snapshot_values[part][1][P_LSYNC]==p[P_LSYNC];
}

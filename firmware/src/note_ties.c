/* SPDX-License-Identifier: GPL-3.0-only */
/* Bounded native note-chain edits; no pattern-sized staging buffer. */
static uint32_t note_slot(const step_t *s, uint32_t pitch)
{
    uint32_t i;
    for (i=0;i<s->n && s->note[i]!=pitch;i++) {}
    return i;
}
static void note_effective(const track_t *t, uint32_t i, step_t *s)
{
    if (i>=NSTEP) {memset(s,0,sizeof *s);s->time=ST_REST;return;}
    uint32_t at=i;
    while (at && t->step[at].time==ST_TIE) at--;
    *s=t->step[at];
    if (s->time!=ST_NOTE) {s->n=0;s->flags&=(uint8_t)~SF_TIES;}
    if (at!=i) { s->flags=(uint8_t)((s->flags&3u)|(((1u<<s->n)-1u)<<2));s->rat=0; }
}
static uint32_t note_chain_len(const track_t *t, uint32_t i, uint32_t pitch)
{
    uint32_t n=1;
    for (;i+n<trk_len(t);n++) {
        const step_t *s=&t->step[i+n];
        if (s->time==ST_TIE) continue;
        uint32_t k=note_slot(s,pitch);
        if (s->time!=ST_NOTE || k==s->n || !(s->flags&SF_TIE(k))) break;
    }
    return n;
}
static uint32_t note_origin(const track_t *t,uint32_t i,uint32_t pitch)
{
    while (i) {
        const step_t *s=&t->step[i];uint32_t k=note_slot(s,pitch);
        if (s->time!=ST_TIE && (s->time!=ST_NOTE || k==s->n || !(s->flags&SF_TIE(k)))) break;
        step_t prev;note_effective(t,i-1,&prev);
        if (note_slot(&prev,pitch)==prev.n) break;
        i--;
    }
    return i;
}
static void note_remove_slot(step_t *s,uint32_t k)
{
    for (;k+1<s->n;k++) {
        s->note[k]=s->note[k+1];
        s->lvl=(uint8_t)((s->lvl&~(3u<<(2*k)))|(((s->lvl>>(2*k+2))&3u)<<(2*k)));
        s->rat=(uint8_t)((s->rat&~(3u<<(2*k)))|(((s->rat>>(2*k+2))&3u)<<(2*k)));
        s->flags=(uint8_t)((s->flags&~SF_TIE(k))|((s->flags&SF_TIE(k+1))>>1));
    }
    s->n--;s->note[s->n]=0;s->flags&=(uint8_t)~SF_TIE(s->n);
    s->lvl&=(uint8_t)((1u<<(2*s->n))-1u);s->rat&=(uint8_t)((1u<<(2*s->n))-1u);
    if (!s->n) s->time=ST_REST;
}
static void note_cut(track_t *t,uint32_t first,uint32_t count,uint32_t pitch)
{
    for (uint32_t i=first;i<first+count;i++) {
        step_t s;note_effective(t,i,&s);
        uint32_t k=note_slot(&s,pitch);
        if (k<s.n) note_remove_slot(&s,k);
        t->step[i]=s;
        if (!s.n) t->step[i].time=ST_REST;
    }
}
static void note_erase(track_t *t,uint32_t at,uint32_t pitch)
{
    step_t src;note_effective(t,at,&src);
    for (uint32_t k=0;k<src.n;k++) if (pitch>127 || src.note[k]==pitch)
        note_cut(t,at,note_chain_len(t,at,src.note[k]),src.note[k]);
}
/* Return -1 at a deliberate reattack, -2 at capacity, -3 without a source.
 * Call first with write=0: a whole scrub increment is validated before mutation. */
static int note_extend(track_t *t,uint32_t first,uint32_t end,uint32_t voice,uint32_t write)
{
    step_t src;note_effective(t,first,&src);
    if (!src.n || voice>src.n) return -3;
    for (uint32_t at=first+1;at<=end;at++) {
        step_t dst;note_effective(t,at,&dst);
        for (uint32_t j=0;j<src.n;j++) if (!voice || j+1==voice) {
            uint32_t k=note_slot(&dst,src.note[j]);
            if (k<dst.n) {
                if (!(dst.flags&SF_TIE(k))) return -1;
            } else {
                if (dst.n==4) return -2;
                if (!dst.n) dst.vel=(src.flags&SF_ACCENT)?127:src.vel;
                dst.note[k]=src.note[j];dst.n++;dst.flags|=(uint8_t)SF_TIE(k);
                dst.lvl=(uint8_t)((dst.lvl&~(3u<<(2*k)))|(((src.lvl>>(2*j))&3u)<<(2*k)));
                dst.rat&=(uint8_t)~(3u<<(2*k));
            }
        }
        if (dst.n>1 && t->p[P_VOICE]!=V_POLY && t->step[at].time==ST_NOTE && t->step[at].n) return -4;
        if (write && t->step[at].time!=ST_TIE) {
            /* Keep ordinary whole-chord brush runs compact and compatible. */
            step_t prev;note_effective(t,at-1,&prev);
            uint32_t same=prev.n==dst.n;
            for (uint32_t j=0;j<dst.n;j++)
                if (!(dst.flags&SF_TIE(j)) || note_slot(&prev,dst.note[j])==prev.n) same=0;
            if (same) t->step[at].time=ST_TIE;
            else {dst.time=ST_NOTE;t->step[at]=dst;}
        }
    }
    return 0;
}
static int note_pattern_linked(const track_t *t)
{
    for (uint32_t i=0;i<trk_len(t);i++)
        if (t->step[i].time==ST_NOTE && (t->step[i].flags&SF_TIES)) return 1;
    return 0;
}

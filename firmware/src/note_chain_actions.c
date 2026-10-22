/* SPDX-License-Identifier: GPL-3.0-only */
/* Include after note_ties.c. Negative results never change any track bytes. */
enum { NOTE_CHAIN_SOURCE=-3, NOTE_CHAIN_COLLISION=-1, NOTE_CHAIN_FULL=-2,
       NOTE_CHAIN_POLY=-4, NOTE_CHAIN_RANGE=-5, NOTE_CHAIN_LOCKS=-6 };

UI_SMALL static int note_chain_action(track_t *t, uint32_t onset, uint32_t pitch,
                                    uint32_t dest, uint32_t target,
                                    uint32_t remove, uint32_t placing, uint32_t write)
{
    uint32_t len=trk_len(t);
    if (onset>=len || dest>=len || pitch>127 || target>127) return NOTE_CHAIN_RANGE;
    step_t src=t->step[onset];
    uint32_t slot=note_slot(&src,pitch);
    if (src.time!=ST_NOTE || slot==src.n || (src.flags&SF_TIE(slot))) return NOTE_CHAIN_SOURCE;
    uint32_t count=note_chain_len(t,onset,pitch);
    if (count>len-dest) return NOTE_CHAIN_RANGE;
    if (placing) for (uint32_t k=0;k<NLOCK;k++)
        if (t->lock[k].step==onset) return NOTE_CHAIN_LOCKS;
    if (remove && dest==onset && target==pitch) return 0;
    /* Both public entry points always validate before starting the write pass.
     * Streaming the ORIGINAL effective predecessor prevents a changed legacy
     * tie from contaminating the next cell, including outside the edited span. */
    for (uint32_t pass=0;pass<=!!write;pass++) {
        step_t prev={0}; uint32_t changed=0;
        for (uint32_t at=0;at<len;at++) {
            step_t raw=t->step[at], s=raw;
            if (raw.time==ST_TIE) {
                s=prev;s.flags=(uint8_t)((s.flags&3u)|(((1u<<s.n)-1u)*SF_CONT0));s.rat=0;
            } else if (s.time!=ST_NOTE) {s.n=0;s.flags&=(uint8_t)~SF_TIES;}
            prev=s;
            uint32_t cut=remove && at>=onset && at-onset<count;
            uint32_t add=at>=dest && at-dest<count;
            if (!placing && cut) {
                if (note_slot(&s,target)<s.n) return NOTE_CHAIN_COLLISION;
                s.note[note_slot(&s,pitch)]=(uint8_t)target;
                add=0;
            } else if (cut) {
                uint32_t k=note_slot(&s,pitch);
                if (k<s.n) note_remove_slot(&s,k);
            }
            if (add) {
                uint32_t k=note_slot(&s,target);
                if (k<s.n) return NOTE_CHAIN_COLLISION;
                if (s.n==4) return NOTE_CHAIN_FULL;
                if (s.n && t->p[P_VOICE]!=V_POLY) return NOTE_CHAIN_POLY;
                s.note[k]=(uint8_t)target;s.n++;
                s.flags=(uint8_t)((s.flags&~SF_TIE(k))|(at!=dest?SF_TIE(k):0));
                s.lvl=(uint8_t)((s.lvl&~(3u<<(2*k)))|(((src.lvl>>(2*slot))&3u)<<(2*k)));
                s.rat=(uint8_t)((s.rat&~(3u<<(2*k)))|(at==dest?((src.rat>>(2*slot))&3u)<<(2*k):0));
                if (!k) {s.vel=src.vel;s.flags=(uint8_t)((s.flags&~SF_ACCENT)|(src.flags&SF_ACCENT));}
                s.time=ST_NOTE;
            }
            if (raw.time!=ST_TIE) changed=0;
            if (cut || add || changed) {
                changed=1;
                if (s.n) s.time=ST_NOTE; else s.time=ST_REST;
                if (pass) t->step[at]=s;
            }
        }
    }
    return 0;
}

UI_SMALL static int note_chain_pitch(track_t *t,uint32_t onset,uint32_t pitch,uint32_t target,uint32_t write)
{
    return note_chain_action(t,onset,pitch,onset,target,1,0,write);
}
UI_SMALL static int note_chain_place(track_t *t,uint32_t onset,uint32_t pitch,uint32_t dest,uint32_t move,uint32_t write)
{
    return note_chain_action(t,onset,pitch,dest,pitch,!!move,1,write);
}

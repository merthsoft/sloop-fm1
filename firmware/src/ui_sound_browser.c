/* SPDX-License-Identifier: GPL-3.0-only */
/* A single reversible audition owns only patch parameters, never voices/patterns.
 * Leaving the browser cancels; KEEP establishes a new audition baseline. */
static struct {
    int16_t p[P_COUNT], ab[2][P_COUNT], morph[SNAPSHOT_N];
    uint32_t morph_phase;
    uint8_t patch[FP_SIZE+1u], fm_slot, fm_pending;
    uint8_t active, part, eng, preset, user;
    int8_t fav;
} browse_saved;
enum { BROWSE_USER=NELEM(BANK_KIND)+2, BROWSE_ENGINE=BROWSE_USER+1,
       BROWSE_LAST=BROWSE_ENGINE+NENGINES-1 };
static uint8_t browse_category; /* ALL, FAV, kinds, USER, factory engine filters */
UI_SMALL static const char *browse_label(void)
{
    return !browse_category?"ALL":browse_category==1?"FAV":
        browse_category<BROWSE_USER?BANK_KIND[browse_category-2]:
        browse_category==BROWSE_USER?"USER":ENGINES[browse_category-BROWSE_ENGINE]->name;
}
UI_SMALL static int browse_match(uint32_t n,uint32_t nf,const uint32_t *favlist)
{
    uint32_t base=n<nf?favlist[n]:n-nf;
    return !browse_category || (browse_category==1?n<nf:
        n>=nf && (browse_category>=BROWSE_ENGINE?
            base<NBANK && BANK[base].e==browse_category-BROWSE_ENGINE:
            browse_category==(base<NBANK?BANK[base].kind+2u:BROWSE_USER)));
}
/* Filter only this device view; favourites and MIDI Program Change identities stay stable. */
UI_SMALL static uint32_t browse_list(uint32_t rank,uint32_t *position,uint32_t *count)
{
    uint32_t b[FAV_N],nf=fav_list(b),total,cur=preset_pos(&total),out=0;
    *count=*position=0;
    for(uint32_t n=0;n<total;n++)if(browse_match(n,nf,b)) {
        if(n==cur)*position=*count;
        if((*count)++==rank)out=n;
    }
    return out;
}
UI_SMALL static void browse_end(int keep)
{
    if(!keep)browse_category=0;
    if(!browse_saved.active)return;
    track_t *t=&trk[browse_saved.part];
    browse_saved.active=0;
    if(!keep) {
        fm1_irq_off();
        locks_restore(t);sound_change(t);
        t->eng_req=browse_saved.eng;t->preset=browse_saved.preset;t->user=browse_saved.user;
        memcpy(t->p,browse_saved.p,sizeof t->p);
        memcpy(snapshot_values[browse_saved.part],browse_saved.ab,sizeof browse_saved.ab);
        memcpy(snapshot_p[browse_saved.part],browse_saved.morph,sizeof browse_saved.morph);
        snapshot_run[browse_saved.part].phase=browse_saved.morph_phase;
        snapshot_run[browse_saved.part].playing=0;
        fm1_irq_on();
        fm6_set_patch(browse_saved.part,browse_saved.patch);
        fm6_slot[browse_saved.part]=browse_saved.fm_slot;
        fm6_pend=(uint8_t)((fm6_pend&~(1u<<browse_saved.part))|browse_saved.fm_pending);
        fav_at=browse_saved.fav;
        sync_reload=1;
    }
    ui.force=1;
}
UI_SMALL static void browse_step(int32_t steps)
{
    uint32_t cur,total;
    browse_list(0,&cur,&total);
    if(!total){ui_message("NO SOUNDS");return;}
    uint32_t n=browse_list((uint32_t)(((int32_t)cur+steps%(int32_t)total+(int32_t)total)%(int32_t)total),&cur,&total);
    if(!browse_saved.active) {
        track_t *t=TSEL;
        fm1_irq_off();
        for(uint32_t id=0;id<P_COUNT;id++)browse_saved.p[id]=*snapshot_base(t,id);
        memcpy(browse_saved.ab,snapshot_values[song.sel],sizeof browse_saved.ab);
        memcpy(browse_saved.morph,snapshot_p[song.sel],sizeof browse_saved.morph);
        browse_saved.morph_phase=snapshot_run[song.sel].phase;
        memcpy(browse_saved.patch,fm6_patch[song.sel],sizeof browse_saved.patch);
        browse_saved.fm_slot=fm6_slot[song.sel];browse_saved.fm_pending=(uint8_t)(fm6_pend&(1u<<song.sel));
        browse_saved.eng=t->eng_req;browse_saved.preset=t->preset;browse_saved.user=t->user;
        browse_saved.part=song.sel;browse_saved.fav=fav_at;browse_saved.active=1;
        fm1_irq_on();
    }
    preset_go(n);
}


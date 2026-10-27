/* SPDX-License-Identifier: GPL-3.0-only */
/* A/B controls share the live track parameters. No audio-rate morph runner. */
UI_SMALL static int snapshot_param(uint32_t id)
{
    return id<=P_LD_AMP || (id>=P_DIST && id<=P_REV) ||
           id==P_GLIDE || id==P_PAN || id==P_DETUNE || id==P_TFLT || id>=P_ITYPE;
}

/* Locks keep their instantaneous sound, with the morphed value as their new
 * base. Capturing under a lock likewise captures the underlying sound. */
UI_SMALL static int16_t *snapshot_base(track_t *t,uint32_t id)
{
    for(uint32_t k=0;k<t->lk_n && k<NLOCK;k++)
        if(t->lk_param[k]==id && t->p[id]==t->lk_set[k])return &t->lk_base[k];
    return &t->p[id];
}

UI_SMALL static void snapshot_edit(uint32_t slot,int32_t steps)
{
    uint32_t part=song.sel;
    if(part>=NPART || slot==3u)return;
    track_t *t=&trk[part];int16_t *p=snapshot_p[part];
    if(slot) {
        if(steps<=0)return;
        if(ui.arm!=0xD0u+slot) {
            ui.arm=(uint8_t)(0xD0u+slot);ui.arm_t=90;
            ui_say("AGAIN: ",slot==1u?"STORE A":"STORE B");return;
        }
        ui.arm=0;
        fm1_irq_off();
        for(uint32_t id=0;id<P_COUNT;id++)
            snapshot_values[part][slot-1u][id]=*snapshot_base(t,id);
        p[3]|=(int16_t)(1u<<(slot-1u));p[0]=slot==1u?0:127;
        fm1_irq_on();
        ui_message(slot==1u?"A STORED":"B STORED");
    } else {
        if(p[3]!=3){ui_message("STORE A AND B");return;}
        int32_t position=clamp(p[0]+accel(EN_K1,steps,127),0,127);
        fm1_irq_off();
        for(uint32_t id=0;id<P_COUNT;id++)if(snapshot_param(id)) {
            /* Common times remain numeric even when ENV SYNC displays names.
             * Engine selectors precede their dependent fields in this loop. */
            const param_desc_t *d=id>=P_E0?track_desc(t,id):&TP[id];
            int32_t a=snapshot_values[part][0][id],b=snapshot_values[part][1][id];
            *snapshot_base(t,id)=(int16_t)(d->names?(position<64?a:b):a+(b-a)*position/127);
        }
        p[0]=(int16_t)position;
        fm1_irq_on();
        sync_reload=1;
    }
    ui.force=1;
}

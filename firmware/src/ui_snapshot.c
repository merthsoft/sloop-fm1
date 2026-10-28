/* SPDX-License-Identifier: GPL-3.0-only */
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
        snapshot_apply(t,position,0);
        p[0]=(int16_t)position;
        fm1_irq_on();
        sync_reload=1;
    }
    ui.force=1;
}

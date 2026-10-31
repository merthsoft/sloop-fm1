/* SPDX-License-Identifier: GPL-3.0-only */
UI_SMALL static void snapshot_edit(uint32_t slot,int32_t steps)
{
    uint32_t part=song.sel;
    if(part>=NPART || slot==3u || slot>=SNAPSHOT_N)return;
    track_t *t=&trk[part];int16_t *p=snapshot_p[part];
    if(slot>=4u) {
        int32_t value=clamp(p[slot]+steps,SNAPSHOT_DESC[slot].min,SNAPSHOT_DESC[slot].max);
        fm1_irq_off();
        if(slot==5u && value) {
            if(!snapshot_ready(t,part)) {
                fm1_irq_on();ui_message(p[3]!=3?"STORE A AND B":"MATCH SOURCES");return;
            }
            memset(&snapshot_run[part],0,sizeof snapshot_run[part]);
            p[0]=value==1?0:127;snapshot_apply(t,p[0],0);
            if(drift_p[part][3]==DRIFT_MORPH) {
                drift_p[part][0]=0;drift[part].depth=drift[part].value=0;
            }
        }
        p[slot]=(int16_t)value;
        if(p[6]>=8) snapshot_run[part].last=motion_phase((uint32_t)p[6]);
        snapshot_run[part].playing=(uint8_t)song.playing;
        fm1_irq_on();sync_reload=1;
    } else if(slot) {
        if(steps<=0)return;
        if(ui.arm!=0xD0u+slot) {
            ui.arm=(uint8_t)(0xD0u+slot);ui.arm_t=90;
            ui_say("AGAIN: ",slot==1u?"STORE A":"STORE B");return;
        }
        ui.arm=0;
        fm1_irq_off();
        for(uint32_t id=0;id<P_COUNT;id++)
            snapshot_values[part][slot-1u][id]=*snapshot_base(t,id);
        p[5]=0;p[3]|=(int16_t)(1u<<(slot-1u));p[0]=slot==1u?0:127;
        fm1_irq_on();
        ui_message(slot==1u?"A STORED":"B STORED");
    } else {
        if(p[3]!=3){ui_message("STORE A AND B");return;}
        int32_t position=clamp(p[0]+accel(cur_page()->scope==SC_MORPH && cur_page()->id[0]==5?EN_K3:EN_K1,steps,127),0,127);
        fm1_irq_off();
        p[5]=0;snapshot_apply(t,position,0);
        p[0]=(int16_t)position;
        fm1_irq_on();
        sync_reload=1;
    }
    ui.force=1;
}

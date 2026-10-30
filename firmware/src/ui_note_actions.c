/* SPDX-License-Identifier: GPL-3.0-only */
/* Small native action sheets; musical edits use the same preflight as tests. */
#include "note_chain_actions.c"
#include "drum_hit_actions.c"
static const char *const NOTE_ACTIONS[]={"SLIDE STEP","NUDGE STEP","FILL STEP","ACCENT STEP","MOVE NOTE","COPY NOTE","DELETE NOTE"};
static const char *const HIT_ACTIONS[]={"RATCHET HIT","NUDGE STEP","FILL STEP","MOVE HIT","COPY HIT","DELETE HIT"};
UI_SMALL static int actions_on(void)
{return !ui.home && (cur_page()->scope==SC_ACTION || (on_drum_page()&&drum_page==5));}
UI_SMALL static int actions_ready(void)
{
    if(note_action.epoch!=project_revision || note_action.track!=song.sel || note_action.at>=trk_len(TSEL))return 0;
    if(note_action.hit)return is_drum(TSEL)&&dstep_has(&TSEL->dstep[note_action.at],note_action.lane);
    return !is_drum(TSEL)&&ui.cursor==note_action.at&&piano_selection_slot(TSEL,roll_selection())>=0;
}
UI_SMALL static uint32_t actions_shared(void){return note_action.hit?3u:4u;}
UI_SMALL static int32_t actions_value(uint32_t row)
{
    const step_t *s=&TSEL->step[note_action.at];
    if(!row)return note_action.hit?(int32_t)dstep_rat(&TSEL->dstep[note_action.at],note_action.lane):!!(s->flags&SF_SLIDE);
    if(row==1)return TSEL->micro[note_action.at];
    if(row==2)return step_fill(TSEL,note_action.at);
    return !!(s->flags&SF_ACCENT);
}
UI_SMALL static void actions_draw(void)
{
    static uint32_t last;
    uint32_t sig=steps_hash(TSEL)+song.sel*977u+note_action.row*7919u+note_action.dest*104729u+
        note_action.hit*31u+note_action.lane*131u+note_action.at*257u+project_revision*65537u;
    for(unsigned i=0;i<NSTEP;i++)sig=sig*31u+(uint8_t)TSEL->micro[i]+step_fill(TSEL,i);
    draw_head();
    if(!ui.force&&last==sig)return;
    last=sig;uint32_t ready=actions_ready(),n=note_action.hit?6u:7u;char label[32],value[16];
    cv_oy=0;cv_band(44);
    if(ready) {
        if(note_action.hit)str_cpy(label,LANE_SHORT[note_action.lane],sizeof label);
        else scale_note_name(label,TSEL,ui.roll_pitch);
        str_cpy(label+str_len(label),"  STEP ",8);fmt_int(label+str_len(label),note_action.at+1);
    } else str_cpy(label,note_action.hit?"SELECT A HIT":"SELECT A NOTE",sizeof label);
    cv_text_c_white(4,0,label);cv_text_c_gray(4,22,"K1: ROW  K2: VALUE");cv_band_blit(Y_LABEL);
    cv_band(H_GRAPH);
    for(uint32_t row=0;row<n;row++) {
        int32_t y=(int32_t)row*17;uint16_t color=row==note_action.row?C_WHITE:C_GRAY;
        if(row==note_action.row){cv_rect_te_g1(0,y,240,17);cv_rect(0,y,2,17,TE_COL[song.sel]);}
        cv_text_s(5,y,note_action.hit?HIT_ACTIONS[row]:NOTE_ACTIONS[row],color);
        str_cpy(value,"--",sizeof value);
        if(ready) {
            if(row<actions_shared()) {
                int32_t v=actions_value(row);
                if(row==2) {static const char *const fill[]={"NORM","ONLY","NOT","NORM"};str_cpy(value,fill[v&3],sizeof value);}
                else if(row==0&&note_action.hit){str_cpy(value,"X",sizeof value);fmt_int(value+1,v+1);}
                else if(row==1)fmt_int(value,v);
                else str_cpy(value,v?"ON":"OFF",sizeof value);
            } else if(row<actions_shared()+2u)fmt_int(value,note_action.dest+1);
            else str_cpy(value,"GO",sizeof value);
        }
        cv_text_s(235-text_w(&FONT_S,value),y,value,color);
    }
    cv_band_blit(Y_GRAPH);cv_band(H_FOOT);cv_rect(0,0,240,1,TE_COL[song.sel]);
    cv_text_s(4,2,note_action.hit?"HIT ACTIONS":"NOTE ACTIONS",TE_COL[song.sel]);
    cv_text_c_gray(4,20,"OCT+: APPLY  OCT-: BACK");cv_band_blit(Y_FOOT);
}
UI_SMALL static void actions_leave(void)
{
    enc_drop(); /* no pending turn leaks into the destination page */
    kb_grid=0;
    if(note_action.hit){drum_page=0;ui.force=1;return;}
    for(uint32_t p=0;p<NPAGES;p++)if(PAGES[p].scope==SC_STEP&&PAGES[p].id[0]==4u){ui.page=(uint8_t)p;break;}
    page_entered();ui.force=1;
}
UI_SMALL static int actions_editable(void)
{
    if(song.rec||rec_wait||ft_on){ui_message("STOP RECORDING");return 0;}
    return 1;
}
UI_SMALL static void actions_edit(uint32_t slot,int32_t delta)
{
    uint32_t shared=actions_shared(),at=note_action.at;
    if(!slot){note_action.row=(uint8_t)clamp((int32_t)note_action.row+clamp(delta,-7,7),0,shared+2u);return;}
    if(slot!=1)return;
    if(note_action.row>=shared) {note_action.dest=(uint8_t)clamp((int32_t)note_action.dest+clamp(delta,-NSTEP,NSTEP),0,trk_len(TSEL)-1);return;}
    if(!actions_ready()){ui_message("EMPTY SOURCE");return;}
    if(!actions_editable())return;
    uint32_t row=note_action.row;int32_t old=actions_value(row),max=row==1?MICRO_MAX:row==2?2:!row&&note_action.hit?3:1;
    int32_t value=clamp(old+clamp(delta,-64,64),row==1?MICRO_MIN:0,max);
    if(value==old)return;
    undo_mark(TSEL,(undo_sess+=4u)|3u);fm1_irq_off();
    if(row==1)TSEL->micro[at]=(int8_t)value;
    else if(row==2)step_fill_set(TSEL,at,(uint32_t)value);
    else if(!row&&note_action.hit)dstep_set(&TSEL->dstep[at],note_action.lane,dstep_lvl(&TSEL->dstep[at],note_action.lane),value);
    else {uint32_t flag=row?SF_ACCENT:SF_SLIDE;TSEL->step[at].flags=(uint8_t)((TSEL->step[at].flags&~flag)|(value?flag:0));}
    seq_edit_changed(TSEL);fm1_irq_on();sync_reload=1;
}
UI_SMALL static void actions_apply(void)
{
    if(!actions_ready()){ui_message("EMPTY SOURCE");return;}
    if(!actions_editable())return;
    uint32_t shared=actions_shared(),row=note_action.row,at=note_action.at,dest=note_action.dest,move=row==shared;
    if(row<shared){ui_message("TURN KNOB 2");return;}
    int rc;
    if(note_action.hit)rc=row==shared+2u?drum_hit_delete(TSEL,note_action.lane,at,0):drum_hit_place(TSEL,note_action.lane,at,dest,move,0);
    else rc=row==shared+2u?0:note_chain_place(TSEL,at,ui.roll_pitch,dest,move,0);
    if(rc<0){ui_message(note_action.hit?(rc==-3?"HIT ALREADY HERE":"EMPTY SOURCE"):rc==-5?"END OF PATTERN":rc==-6?"STEP HAS LOCKS":rc==-2?"CHORD FULL":rc==-4?"SET VOICE TO POLY":"NOTE ALREADY HERE");return;}
    if(row==shared&&dest==at)return;
    undo_mark(TSEL,(undo_sess+=4u)|3u);fm1_irq_off();
    if(note_action.hit){if(row==shared+2u)drum_hit_delete(TSEL,note_action.lane,at,1);else drum_hit_place(TSEL,note_action.lane,at,dest,move,1);}
    else if(row==shared+2u)note_erase(TSEL,at,ui.roll_pitch);
    else note_chain_place(TSEL,at,ui.roll_pitch,dest,move,1);
    seq_edit_changed(TSEL);fm1_irq_on();
    if(row==shared){note_action.at=(uint8_t)dest;if(note_action.hit)drum_cursor=(uint8_t)dest;else roll_select((piano_selection_t){dest,ui.roll_pitch});}
    if(row==shared+2u&&!note_action.hit)ui.roll_pitch=0;
    sync_reload=1;ui.force=1;ui_message(row==shared?"NOTE MOVED":row==shared+1u?"NOTE COPIED":"NOTE DELETED");
}
UI_SMALL static int actions_input(uint32_t pressed,uint32_t home)
{
    if(!actions_on())return 0;
    if(song.rec||rec_wait||ft_on||(pressed&(1u<<panel.btn[B_PLAY]))){actions_leave();return 0;}
    if(home==BT_TAP){go_home();kb_grid=0;return 1;}
    if(pressed&(1u<<panel.btn[B_OCTDN])){actions_leave();return 1;}
    if(pressed&(1u<<panel.btn[B_OCTUP]))actions_apply();
    int32_t s;
    if((s=panel_enc(EN_SELECT))){
        if(note_action.hit){drum_page=(uint8_t)clamp(5+clamp(s,-5,5),0,5);kb_grid=drum_page==5?2:0;ui.force=1;}
        else page_walk(clamp(s,-(int32_t)NPAGES,(int32_t)NPAGES));
        if(!actions_on()){enc_drop();return 1;}
    }
    if((s=panel_enc(EN_ALGO))){uint32_t track=song.sel;track_select(clamp((int32_t)song.sel+(s>0?1:-1),0,3));if(track!=song.sel)actions_leave();}
    panel_enc(EN_PRESET);
    for(uint32_t k=0;k<4;k++)if((s=panel_enc(EN_K1+k)))actions_edit(k,s);
    return 1;
}

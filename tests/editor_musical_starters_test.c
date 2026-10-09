/* SPDX-License-Identifier: GPL-3.0-only */
#undef NDEBUG
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static uint8_t reply[600];static uint32_t reply_n;
static void ed_b(uint32_t v){assert(reply_n<sizeof reply);reply[reply_n++]=v&127u;}
static void ed_str(const char *s,uint32_t max){uint32_t i;for(i=0;s[i]&&i<max;i++)ed_b(s[i]);ed_b(0);}
static uint32_t fm1_icfg(void){return 1;}
static void fm1_icfg_set(uint32_t v){(void)v;}
#include "../firmware/src/editor_musical_starters.c"
static void command(uint8_t *a,uint32_t n,uint32_t status){reply_n=0;assert(ed_musical_starters_handle(76,a,n));assert(reply[0]==(n?a[0]:127)&&reply[1]==status);}
int main(void){
    static track_t before,other;uint32_t i;uint8_t query[]={0},list[]={1};
    uint8_t a[]={2,1,23,11,2,3,0,1,16,8,0,32,0,1,4},lease[]={4,1,4,1};
    panel=PANEL_DEFAULT;host_tracks_init();song.sel=1;usb.config=1;
    before=trk[1];other=trk[0];command(query,1,0);assert(reply_n==8&&reply[3]==24&&reply[4]==7);
    command(list,1,0);assert(reply_n<600&&reply[2]==24);
    {uint32_t at=3;for(i=0;i<24;i++){assert(reply[at++]==i);assert(!strcmp((char*)&reply[at],SEQUENCE_STARTERS[i].name));at+=strlen((char*)&reply[at])+1;}assert(at==reply_n);}
    query[0]=5;command(query,1,0);assert(reply[2]==NSCALES);
    assert(!memcmp(&before,&trk[1],sizeof before));command(0,0,1);command(a,12,1);
    a[1]=0;command(a,13,4);a[1]=1;
    song.playing=1;command(a,13,2);song.playing=0;transport_req=2;command(a,13,2);transport_req=0;
    trk[1].micro[3]=11;before=trk[1];command(a,13,3);assert(!memcmp(&before,&trk[1],sizeof before));
    a[0]=3;
    midi_sel_on[0][60]=1;command(a,15,2);midi_sel_on[0][60]=0;
    midi_sel_on[9][36]=4;command(a,15,2);midi_sel_on[9][36]=0;
    command(a,15,0);ed_starter_tick(1);assert(sequence_preview.active&&ed_starter_token==513);
    assert(!memcmp(before.step,trk[1].step,sizeof before.step)&&!memcmp(before.micro,trk[1].micro,sizeof before.micro));command(lease,4,0);fm1_ms+=1499;ed_starter_service();assert(sequence_preview.active);
    fm1_ms++;ed_starter_service();assert(!sequence_preview.active&&!ed_starter_token);command(lease,4,4);
    command(a,15,0);usb.resets++;ed_starter_service();assert(!sequence_preview.active);
    command(a,15,0);usb.config=0;ed_starter_tick(1);assert(!sequence_preview.active);usb.config=1;
    command(a,15,0);fm1_in.notes=1;ed_starter_tick(1);assert(!sequence_preview.active);fm1_in.notes=0;
    trk[1].nheld=1;command(a,15,2);trk[1].nheld=0;
    command(a,15,0);input_on(&trk[1],60,100);assert(!sequence_preview.active);input_off(&trk[1],60);
    command(a,15,0);midi_in_q[mi_w++%MQ]=9u|(0x99u<<8)|(36u<<16)|(100u<<24);events_block(1);assert(!sequence_preview.active);midi_in_q[mi_w++%MQ]=8u|(0x89u<<8)|(36u<<16);events_block(1);
    command(a,15,0);song.sel=0;ed_starter_tick(1);assert(!sequence_preview.active);song.sel=1;
    midi_sel_on[0][60]=1;midi_sel_on[1][61]=2;panic_req=1;events_block(1);
    assert(!midi_sel_on[0][60]&&midi_sel_on[1][61]);midi_sel_on[1][61]=0;
    a[0]=2;a[12]=1;command(a,13,0);assert(!memcmp(&other,&trk[0],sizeof other));
    for(i=0;i<NSTEP;i++){step_t s=sequence_starter_voiced(23,i,11,2,0,0,&sequence_shape,1);assert(!memcmp(&s,&trk[1].step[i],sizeof s));}
    assert(undo_swap(0));assert(!memcmp(before.step,trk[1].step,sizeof before.step)&&!memcmp(before.micro,trk[1].micro,sizeof before.micro));
    printf("editor musical starters: PASS\n");return 0;
}

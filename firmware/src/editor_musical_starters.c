/* SPDX-License-Identifier: GPL-3.0-only */
/* Additive command 76; all musical material comes from the native ROM bank. */
#define ED_MUSICAL_STARTERS 76u
#define ED_STARTER_LEASE 1500u
static uint32_t ed_starter_until, ed_starter_resets;
static uint16_t ed_starter_token;
static int ed_starter_busy(void)
{
    uint32_t k;
    if (song.playing || transport_req || song.rec || rec_wait || ft_on || panic_req || fm1_in.notes) return 1;
    for(k=0;k<NPART;k++) if(trk[k].nheld || chord_latch_n[k] || arp_chord_latch_n[k]) return 1;
    return 0;
}
/* Scan only on START/APPLY; never on the audio tick. Reuses routing ownership RAM. */
static int ed_starter_midi_held(void)
{
    uint32_t ch,note;
    for(ch=0;ch<16;ch++)for(note=0;note<128;note++)if(midi_sel_on[ch][note])return 1;
    return 0;
}
static void ed_starter_end(void)
{
    sequence_preview_stop(); ed_starter_token=0;
}
static void ed_starter_tick(uint32_t n)
{
    if(ed_starter_token && (!usb.config || usb.resets!=ed_starter_resets ||
       (int32_t)(fm1_ms-ed_starter_until)>=0 || ed_starter_busy() || song.sel!=sequence_preview.track)) ed_starter_end();
    sequence_preview_block(n);
}
static void ed_starter_service(void)
{
    uint32_t irq=fm1_icfg(); fm1_irq_off();
    if(ed_starter_token && (!usb.config || usb.resets!=ed_starter_resets ||
       (int32_t)(fm1_ms-ed_starter_until)>=0 || !sequence_preview.active)) ed_starter_end();
    fm1_icfg_set(irq);
}
static int ed_musical_starters_handle(uint32_t cmd,const uint8_t *a,uint32_t n)
{
    uint32_t op=n?a[0]:127u, status=0,k,token=0,irq;
    if(cmd!=ED_MUSICAL_STARTERS) return 0;
    ed_starter_service();
    if(op==0 && n==1) { ed_b(0);ed_b(0);ed_b(1);ed_b(NSEQUENCE_STARTERS);ed_b(7);ed_b(NSCALES);ed_b(ED_STARTER_LEASE);ed_b(ED_STARTER_LEASE>>7);return 1; }
    if((op==1 || op==5) && n==1) {
        ed_b(op);ed_b(0);ed_b(op==1?NSEQUENCE_STARTERS:NSCALES);
        for(k=0;k<(op==1?NSEQUENCE_STARTERS:NSCALES);k++){ed_b(k);ed_str(op==1?SEQUENCE_STARTERS[k].name:N_SCALE[k],20);}return 1;
    }
    if(op<2 || op>4 || (op==2?n!=13:op==3?n!=15:n!=4)) status=1;
    for(k=0;k<n;k++) if(a[k]>127) status=1;
    if(!status && op!=4 && (a[1]>=NPART || a[2]>=NSEQUENCE_STARTERS || a[3]>11 || a[4]>=NSCALES || a[5]>6 || a[6]>2 || a[7]>1 || a[8]>32 || a[9]>16 || a[10]>2 || a[11]>63 || a[12]>1)) status=1;
    if(!status && op==3) {token=a[13]|(uint32_t)a[14]<<7;if(!token)status=1;}
    if(!status && op==4) {token=a[1]|(uint32_t)a[2]<<7;if(!token || a[3]>1)status=1;}
    irq=fm1_icfg();fm1_irq_off();
    if(!status && op==4) {
        if(token!=ed_starter_token) status=4;
        else if(a[3]) ed_starter_until=fm1_ms+ED_STARTER_LEASE;
        else ed_starter_end();
    } else if(!status) {
        if(song.sel!=a[1]) status=4;
        else if(ed_starter_busy() || ed_starter_midi_held() || sequence_browser || (op==3 && ed_starter_token && token!=ed_starter_token)) status=2;
        else if(op==2 && !a[12] && sequence_starter_has_content(&trk[a[1]])) status=3;
        else if(op==3) {
            sequence_preview_stop();sequence_preview.track=a[1];sequence_preview.starter=a[2];
            sequence_preview.root=a[3];sequence_preview.scale=a[4];sequence_preview.octave=(int8_t)a[5]-3;
            sequence_preview.mode=a[6];sequence_preview.vlead=a[7];
            sequence_preview.rhythm=0; /* Existing phone protocol requests original rhythm. */
            sequence_preview.shape=(rhythm_shape_t){(int8_t)a[8]-16,(int8_t)a[9]-8,(int8_t)a[11]-32,255,a[10]};
            sequence_preview.step=0;sequence_preview.phase=0;sequence_preview.first=1;sequence_preview.active=1;
            ed_starter_token=(uint16_t)token;ed_starter_resets=usb.resets;ed_starter_until=fm1_ms+ED_STARTER_LEASE;
            sequence_preview_tick=ed_starter_tick;sequence_preview_end=ed_starter_end;
        } else {
            /* Reuse native apply while preserving native browser preferences. */
            uint8_t id=sequence_sel,root=sequence_root,scale=sequence_scale,mode=sequence_mode,lead=sequence_vlead;
            int8_t oct=sequence_octave;rhythm_shape_t shape=sequence_shape;uint8_t rhythm=sequence_rhythm;
            ed_starter_end();sequence_sel=a[2];sequence_root=a[3];sequence_scale=a[4];sequence_octave=(int8_t)a[5]-3;
            sequence_mode=a[6];sequence_vlead=a[7];sequence_shape=(rhythm_shape_t){(int8_t)a[8]-16,(int8_t)a[9]-8,(int8_t)a[11]-32,255,a[10]};
            sequence_rhythm=0;
            if(!sequence_starter_apply())status=2;
            fm1_irq_off(); /* native apply re-enables IRQs; restore request lock before preferences */
            sequence_sel=id;sequence_root=root;sequence_scale=scale;sequence_octave=oct;sequence_mode=mode;sequence_vlead=lead;sequence_shape=shape;
            sequence_rhythm=rhythm;
        }
    }
    fm1_icfg_set(irq);ed_b(op);ed_b(status);return 1;
}

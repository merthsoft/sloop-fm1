/* Host-only USB playback tests; compile with -DT_CDC=0/1/2 -DHALF_FRAMES=128.
 * Reuses the capture descriptor/ring regression with a modeled SIE/DMA HAL. */
#define FM1_USB_HOST_TEST
#include <stdint.h>
#include <string.h>
static uint8_t regs[5][32], global[16], index_reg;
static uint32_t rd_value, tx_count[5];
static void *rx_dma[5], *tx_dma[5];
static uint32_t fm1_usb_sie_on(void) { return 1; }
static void fm1_usb_sie_wr_start(uint32_t r, uint32_t v)
{
    if (r == 14) index_reg = v;
    else if (r < 16) global[r] = v;
    else regs[index_reg][r] = v;
}
static void fm1_usb_sie_rd_start(uint32_t r)
{ rd_value = r == 14 ? index_reg : r < 16 ? global[r] : regs[index_reg][r]; }
static uint32_t fm1_usb_sie_done(void) { return 1; }
static uint32_t fm1_usb_sie_data(void) { return rd_value; }
static void fm1_usb_reset(void) { memset(regs, 0, sizeof regs); memset(global, 0, sizeof global); }
static void fm1_usb_attach(void *p) { rx_dma[0] = p; }
static void fm1_usb_off(void) {}
static void fm1_usb_ep0_buf(void *p) { rx_dma[0] = p; }
static void fm1_usb_ep_txbuf(uint32_t ep, void *p) { tx_dma[ep] = p; }
static void fm1_usb_ep_rxbuf(uint32_t ep, void *p) { rx_dma[ep] = p; }
static void fm1_usb_ep0_send(void *p, uint32_t n) { tx_dma[0] = p; tx_count[0] = n; }
static void fm1_usb_ep_send(uint32_t ep, void *p, uint32_t n) { tx_dma[ep] = p; tx_count[ep] = n; }
static void fm1_usb_ep4_txbuf(void *p) { tx_dma[4] = p; }
static void fm1_usb_ep4_rxbuf(void *p) { rx_dma[4] = p; }
static void fm1_usb_ep4_send(void *p, uint32_t n) { tx_dma[4] = p; tx_count[4] = n; }
static void fm1_usb_rx_sync(void) {}
static void fm1_usb_ep_enable(uint32_t eps) { (void)eps; }
static uint32_t fm1_usb_sof_take(void) { return 1; }
#ifdef USB_PLAYBACK_LOADER_TEST
#define FELUCCA_UAC 0
#define FELUCCA_CDC 0
#define FELUCCA_OTA 0
#define FELUCCA_LOADER
#define FELUCCA_USB_PID 2
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#include <stdio.h>
static void fm1_delay_ms(uint32_t ms) {(void)ms;}
#include "../firmware/src/usb.c"
static uint8_t control_reply[20]; static uint32_t control_n;
static void ed_b(uint32_t v) { control_reply[control_n++]=(uint8_t)(v&127u); }
#include "../firmware/src/editor_usb_playback.c"
int main(void)
{
    uint8_t query[]={0},state[]={1};
    ed_usb_playback_handle(75,query,1);
    if(control_n!=8||control_reply[3]!=0)return 1;
    control_n=0;ed_usb_playback_handle(75,state,1);
    if(control_n!=2||control_reply[1]!=2)return 1;
    const uint8_t *c,*d;uint16_t n,dn;uint32_t k;
    get_desc(0x200,&c,&n);get_desc(0x100,&d,&dn);
    if(n!=101||c[4]!=2||d[10]!=2||d[12]!=0||d[13]!=3)return 1;
    for(k=9;k<n;k+=c[k])if(c[k+1]==5&&(c[k+2]==4||c[k+2]==0x84))return 1;
    usb.config=1;ep1_config();
    if(rx_dma[1]!=ep1rx||tx_dma[1]!=ep1tx||global[S_INTRRX1E]!=2)return 1;
    puts("loader: unchanged MIDI-only descriptors and configuration PASS");return 0;
}
#else
#define main capture_main
#include "uac_test.c"
#undef main

/* Exercise the actual audio_block capture/mix ordering with stub synthesis. */
#define CTL 32u
#define FS 44100u
#define FM1_AUDIO_HALF 0x80u
#define FM1_TICKS_PER_US 24u
static struct { uint32_t master_q12, cpu_q8; } song = {4096, 0};
static uint8_t usb_full_now, usb_full;
static int32_t usb_out[64], vis_tap[64];
static void mix_block(int32_t *o, uint32_t n) {
    for (uint32_t i=0;i<n*2;i++) { o[i]=1000; usb_out[i]=2000; }
}
static void shed_voice(void) {}
static uint32_t fm1_ticks(void) { return 0; }
static uint8_t fm1_audio_pending(void) { return 0; }
static void fm1_audio_ack_aux(uint8_t p) {(void)p;}
static uint32_t fm1_audio_free_half(void) {return 0;}
static void fm1_audio_ack_half(void) {}
static void fm1_audio_init(int32_t *b,uint32_t n,void (*f)(void),uint32_t p) {(void)b;(void)n;(void)f;(void)p;}
void isr_alnk0(void) {}
#include "../firmware/src/audio.c"

static void setup(uint8_t type, uint8_t request, uint16_t value, uint16_t ix, uint16_t len)
{
    uint8_t s[8] = {type, request, value, value >> 8, ix, ix >> 8, len, len >> 8};
    memcpy(ep0buf, s, 8);
    usb.e0_tx = 0;
    regs[0][S_CSR0] = 1;
    regs[0][S_COUNT0] = 8;
    ep0_service();
}
static int stalled(void) { return regs[0][S_CSR0] == 0x60; }
static void packet(uint32_t n, int16_t l, int16_t r)
{
    uint8_t p[UAC_MAXP]; uint32_t i;
    for (i = 0; i < n; i++) {
        p[4*i] = l; p[4*i+1] = (uint16_t)l >> 8;
        p[4*i+2] = r; p[4*i+3] = (uint16_t)r >> 8;
    }
    uac_play_receive(p, 4*n);
}
static void clear_play(void)
{
    memset(&up, 0, sizeof up); up.step = 65510; up.master = -1; up.fill_lo = UP_N;
    up_control=4096; up_level=up_target=4096; up_remaining=up_clipped=0;
    up_w = up_r = 0; uac_play_stream(1);
    int32_t out[64] = {0}; uac_play_mix(out, 32, 4096, 1); /* acknowledge epoch */
}
static void test_controls(void)
{
    usb.up = 1;
    setup(0, 9, 1, 0, 0);
    check("EP4 DMA directions independent; RX interrupt enabled", rx_dma[4] == ep4rx && tx_dma[4] == ep4tx && (global[S_INTRRX1E] & 0x10));
    setup(1, 11, 1, UAC_PLAY_IF, 0);
    check("SET_INTERFACE playback only; ISO RX configured", !stalled() && up.alt == 1 && uac.alt == 0 && regs[4][S_RXCSR2] == 0x40);
    setup(0x81, 10, 0, UAC_PLAY_IF, 1);
    check("GET_INTERFACE playback alt", tx_count[0] == 1 && ep0buf[0] == 1);
    setup(1, 11, 2, UAC_PLAY_IF, 0); check("reject unsupported playback alternate", stalled());
    setup(1, 11, 1, 0x103, 0); check("reject high interface index byte", stalled());
    setup(0xA2, 0x81, 0x100, 4, 3);
    check("playback GET_CUR 44100", !stalled() && tx_count[0] == 3 && ep0buf[0] == 0x44 && ep0buf[1] == 0xAC);
    setup(0x22, 1, 0x100, 4, 3);
    ep0buf[0] = 0x44; ep0buf[1] = 0xAC; ep0buf[2] = 0;
    regs[0][S_COUNT0] = 3; regs[0][S_CSR0] = 1; ep0_service();
    check("playback SET_CUR accepts supported rate", !stalled());
    setup(0x22, 1, 0x100, 4, 3);
    ep0buf[0] = 0x80; ep0buf[1] = 0xBB; ep0buf[2] = 0;
    regs[0][S_COUNT0] = 3; regs[0][S_CSR0] = 1; ep0_service();
    check("SET_CUR rejects 48000 data stage", stalled());
    setup(0x22, 1, 0x100, 4, 2); check("SET_CUR rejects incorrect length", stalled());
    setup(0, 9, 0, 0, 0); check("unconfigure stops both streams", !up.alt && !uac.alt);
}
static void test_play_descriptors(void)
{
    const uint8_t *c; uint16_t len; uint32_t k, input = 0, output = 0, ep = 0, idle = 0;
    get_desc(0x200, &c, &len);
    for (k = 9; k < len; k += c[k]) {
        const uint8_t *d = c+k;
        if (d[1] == 0x24 && d[2] == 2 && d[0] == 12 && d[3] == 3)
            input = le16(d+4) == 0x101 && d[7] == 2 && le16(d+8) == 3;
        if (d[1] == 0x24 && d[2] == 3 && d[0] == 9 && d[3] == 4)
            output = le16(d+4) == 0x301 && d[7] == 3;
        if (d[1] == 5 && d[2] == 4)
            ep = d[0] == 9 && d[3] == 9 && le16(d+4) == UAC_MAXP && d[6] == 1 && !d[8];
        if (d[1] == 4 && d[2] == UAC_PLAY_IF && !d[3]) idle = !d[4];
    }
    check("playback USB terminal -> speaker; adaptive OUT; idle alt", input && output && ep && idle);
}
static void test_play_ring(void)
{
    int32_t out[64]; uint32_t i, old;
    clear_play();
    for(i=0;i<(UP_PRIME+43u)/44u;i++)packet(44,1000,-1000);
    up_ring[1]=(uint32_t)(uint16_t)3000 | (uint32_t)(uint16_t)-3000<<16;
    up.active=1;up.gain=256;up.phase=32768;
    memset(out,0,sizeof out);uac_play_mix(out,1,4096,1);
    check("fractional stereo interpolation uses adjacent frames",out[0]==1000&&out[1]==-1000);
    clear_play();
    for (i = 0; i < (UP_PRIME+43u)/44u; i++) packet(44, 12000, -12000);
    memset(out, 0, sizeof out); uac_play_mix(out, 32, 4096, 1);
    check("primed playback ramps in stereo at -6dB", out[0] > 0 && out[0] < out[62] && out[1] < 0);
    for (i = 0; i < 12; i++) { packet(44, 12000, -12000); memset(out, 0, sizeof out); uac_play_mix(out, 32, 4096, 1); }
    check("steady return gain and channels", out[62] == 6000 && out[63] == -6000);
    uac_play_stream(0);
    int32_t previous = 6001; int monotonic = 1;
    for (i = 0; i < 8; i++) {
        memset(out, 0, sizeof out); uac_play_mix(out, 32, 4096, 1);
        for (uint32_t j=0; j<32; j++) { if(out[2*j] > previous) monotonic=0; previous=out[2*j]; }
    }
    check("stop ramps monotonically to silence", monotonic && !up.gain && !out[62]);
    old = up_w; uint8_t bad[UAC_MAXP+4] = {0};
    uac_play_receive(bad, 3); uac_play_receive(bad, sizeof bad);
    check("malformed packets discarded atomically", up.malformed == 2 && up_w == old);
    clear_play(); for(i=0;i<30;i++) packet(44, 1000, 1000);
    check("bounded ring rejects whole overrun packets", up_w-up_r <= UP_N && up.overruns > 0);
    for(i=0;i<50;i++) { memset(out,0,sizeof out); uac_play_mix(out,32,4096,1); }
    check("underrun fades held sample to silence", up.underruns == 1 && !up.gain && !out[62]);
    for(i=0;i<(UP_PRIME+43u)/44u;i++) packet(44, 1000, 1000);
    memset(out,0,sizeof out); uac_play_mix(out,32,4096,1);
    check("underrun recovery re-primes and ramps", up.starts == 2 && out[62] > 0 && out[62] < 500);
    for(i=0;i<32;i++) {out[2*i]=30000;out[2*i+1]=-30000;}
    up.last_l=32767;up.last_r=-32768;up.gain=256;up.active=0;up.alt=0;
    uac_play_mix(out,32,4096,1); check("return sum saturates safely",out[0]==32767&&out[1]==-32768);
    clear_play(); for(i=0;i<32;i++) out[i*2]=out[i*2+1]=12345;
    uac_play_mix(out,32,4096,0); check("inactive synth bus is bit-exact",out[0]==12345&&out[63]==12345);
}
static void test_drift(double rate)
{
    clear_play(); int32_t out[64]; uint32_t t, accum=0, midi_count=0, midi_bad=0; double next=0;
    mi_w=mi_r=0;
    memset(&uac,0,sizeof uac);ua_r=ua_w=0;uac_stream(1);uac.flowing=1;usb.config=1;
    for(t=0;t<120000;t++) {
        if(t%250u==0)midi_in_event(0x7F3C9009u);
        accum+=100; packet(accum>=1000?45:44, 8000,-8000); if(accum>=1000)accum-=1000;
        uint32_t capture[UA_MAXF];uac_packet(capture);
        while(next < t+1) {
            while(mi_r!=mi_w) {if(midi_in_q[mi_r++%MQ]!=0x7F3C9009u)midi_bad++;midi_count++;}
            uac_render_start();
            for(uint32_t block=0;block<HALF_FRAMES;block+=32) {
                for(uint32_t j=0;j<64;j++)out[j]=1000;
                uac_tap(out,32);uac_play_mix(out,32,4096,1);
            }
            next+=HALF_FRAMES*1000.0/rate;
        }
    }
    char label[96]; snprintf(label,sizeof label,"120s drift %.1f Hz: bounded, no over/underruns",rate);
    check(label,up.starts==1&&!up.overruns&&!up.underruns&&up.fill_lo>64&&up.fill_hi<UP_PRIME+HALF_FRAMES+128u);
    check("simultaneous capture ring stays bounded without underruns",!uac.underruns&&!uac.overruns&&ua_w-ua_r<UA_N);
    check("duplex playback leaves MIDI events intact",midi_count==480&&!midi_bad);
    printf("  fill=%u..%u final=%u ASRC step=%d\n",up.fill_lo,up.fill_hi,up_w-up_r,up.step);
}

static void test_service_and_capture(void)
{
    int32_t out[64]; uint32_t i;
    clear_play(); usb.config=usb.up=1;usb.suspended=0;uac.alt=0;
    for(i=0;i<(UP_PRIME+43u)/44u;i++) {
        for(uint32_t j=0;j<44;j++) { ep4rx[4*j]=0xE0;ep4rx[4*j+1]=0x2E;ep4rx[4*j+2]=0xE0;ep4rx[4*j+3]=0x2E; }
        regs[4][S_RXCSR1]=1;regs[4][S_RXCOUNT1]=176;regs[4][S_RXCOUNT2]=0;
        uac_service();
    }
    check("nested EP4 service receives with capture idle",up.frames==44u*((UP_PRIME+43u)/44u)&&regs[4][S_RXCSR1]==0);
    for(i=0;i<8;i++){packet(44,12000,12000);memset(out,0,sizeof out);uac_play_mix(out,32,4096,1);}
    uac.feed=1; ua_r=ua_w=0; usb_full_now=0;
    audio_block(out,32);
    check("actual audio_block capture excludes audible return",(int16_t)ua_ring[0]==1000 && out[0]==7000*(1<<OUT_SHIFT));
    usb_full_now=1;ua_r=ua_w=0;audio_block(out,32);
    check("FULL instrument capture also excludes return",(int16_t)ua_ring[0]==2000 && out[0]==7000*(1<<OUT_SHIFT));
    usb.suspended=1; uint32_t w=up_w;
    regs[4][S_RXCSR1]=1;uac_service();
    check("suspended packets discarded and acknowledged",up_w==w&&!regs[4][S_RXCSR1]);
    for(i=0;i<8;i++) audio_block(out,32);
    check("suspend fades return and discards stale ring",!up.gain&&up_r==up_w&&out[62]==1000*(1<<OUT_SHIFT));
    usb.suspended=0;
    regs[4][S_RXCSR1]=5;uac_service();
    check("hardware ISO error discarded and counted",up.hw_errors==1&&up_w==w&&!regs[4][S_RXCSR1]);
    regs[4][S_RXCSR1]=1;regs[4][S_RXCOUNT1]=255;regs[4][S_RXCOUNT2]=3;
    uac_service();
    check("1023-byte malformed DMA packet rejected safely",up.malformed==1&&up_w==w&&!regs[4][S_RXCSR1]);
    global[S_INTRUSB]=4; usb_poll();global[S_INTRUSB]=0;
    check("bus reset stops both alts and removes configuration",!up.alt&&!uac.alt&&!usb.config);
    clear_play();up_w=up_r=UINT32_MAX-128u;
    for(i=0;i<(UP_PRIME+43u)/44u;i++)packet(44,1234,-1234);
    memset(out,0,sizeof out);uac_play_mix(out,32,4096,1);
    check("monotonic ring indices wrap safely",up_w-up_r<UP_N&&up.starts==1&&out[0]>0);
}
/* Compile the actual command handler with minimal wire helpers. */
static uint8_t control_reply[100]; static uint32_t control_n;
static void ed_b(uint32_t v) { control_reply[control_n++]=(uint8_t)(v&127u); }
#include "../firmware/src/editor_usb_playback.c"
static void control(const uint8_t *a,uint32_t n) { control_n=0; ed_usb_playback_handle(75,a,n); }
static void test_editor_controls(void)
{
    clear_play(); uint8_t query[]={0}, bad[]={1,1,32,0}, set[]={1,0,32,1};
    control(query,1);check("negotiated schema and capabilities",control_n==8&&control_reply[2]==1&&control_reply[3]==15);
    control(bad,4);check("malformed gain rejected without mutation",control_n==2&&control_reply[1]==1&&up_control==4096);
    control(NULL,0);check("empty payload rejected",control_n==2&&control_reply[1]==1);
    for(uint32_t i=0;i<20;i++)packet(44,12000,12000);
    int32_t out[512]={0}; uac_play_mix(out,256,4096,1);
    control(set,4);memset(out,0,sizeof out);uac_play_mix(out,256,4096,1);
    check("mute ramps to zero in 256 frames",out[0]>0&&out[510]==0&&up_level==0);
    set[1]=0;set[2]=16;control(set,4);check("gain stored while muted",(up_control&8191)==2048&&up_level==0);
    set[3]=0;control(set,4);for(uint32_t i=0;i<10;i++)packet(44,12000,12000);
    memset(out,0,sizeof out);uac_play_mix(out,256,4096,1);
    check("unmute restores selected gain with ramp",out[0]<out[510]&&out[510]==3000&&up_level==2048);
    set[2]=32;control(set,4);for(uint32_t i=0;i<10;i++)packet(44,30000,30000);
    for(uint32_t i=0;i<512;i++)out[i]=30000;
    uac_play_mix(out,256,4096,1);
    check("gain ramps without exceeding target and clips are counted",up_level==4096&&up_clipped>0&&out[510]==32767);
    uint32_t old_packets=up.packets,old_read=up_r,old_control=up_control;
    uint8_t reset[]={3},diag[]={2};control(reset,1);check("reset returns zero counter baselines",control_n==63&&control_reply[2]==0&&up.packets>0);
    check("reset preserves stream indices gain and lifetime counters",up.packets==old_packets&&up_r==old_read&&up_control==old_control);
    packet(44,1000,1000);control(diag,1);check("snapshot counts since reset",control_n==63&&control_reply[2]==1&&control_reply[7]==44);
}
int main(void)
{
    capture_main(); test_play_descriptors();test_controls();test_play_ring();
    test_drift(44117.6);test_drift(43900);test_drift(44300);
    test_service_and_capture(); test_editor_controls();
    printf("USB playback: %s (%d failures)\n",fails?"FAIL":"PASS",fails);
    return !!fails;
}
#endif

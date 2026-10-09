/* Real OTA-enabled USB frame interception, with a modeled SIE. */
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#define FM1_USB_HOST_TEST
#define FELUCCA_OTA 1
#define FELUCCA_UAC 0
#define FELUCCA_CDC 0
#ifndef T_SCENE
#define T_SCENE 1
#endif
#define FELUCCA_SCENE_TX T_SCENE
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
static uint8_t regs[5][32], global[16], index_reg;
static uint32_t rd_value;
static uint32_t fm1_usb_sie_on(void) { return 1; }
static void fm1_usb_sie_wr_start(uint32_t r,uint32_t v) { if(r==14) index_reg=v; else if(r<16) global[r]=v; else regs[index_reg][r]=v; }
static void fm1_usb_sie_rd_start(uint32_t r) { rd_value=r==14?index_reg:r<16?global[r]:regs[index_reg][r]; }
static uint32_t fm1_usb_sie_done(void) { return 1; }
static uint32_t fm1_usb_sie_data(void) { return rd_value; }
static void fm1_usb_reset(void) { memset(regs,0,sizeof regs); memset(global,0,sizeof global); }
static void fm1_usb_attach(void *p) { (void)p; }
static void fm1_usb_off(void) {}
static void fm1_usb_ep0_buf(void *p) { (void)p; }
static void fm1_usb_ep_txbuf(uint32_t e,void *p) { (void)e; (void)p; }
static void fm1_usb_ep_rxbuf(uint32_t e,void *p) { (void)e; (void)p; }
static void fm1_usb_ep0_send(void *p,uint32_t n) { (void)p; (void)n; }
static void fm1_usb_ep_send(uint32_t e,void *p,uint32_t n) { (void)e; (void)p; (void)n; }
static void fm1_usb_rx_sync(void) {}
static void fm1_usb_ep_enable(uint32_t e) { (void)e; }
static uint32_t fm1_usb_sof_take(void) { return 1; }
static void fm1_delay_ms(uint32_t n) { (void)n; }
#include "../firmware/src/usb.c"
static uint32_t ota_now_ms(void) { return 0; }
static void ota_idle(void) { so_r=so_w; }
int main(void)
{
    const uint8_t *p; uint32_t n,old;
    usb.config=1;
#if T_SCENE
    /* Production has no installed engine hooks: never advertise live apply.
     * A structurally valid Begin must fail without reserving staging state. */
    { uint8_t reply[24], caps[]={SC_CAPS}, begin[18]={SC_BEGIN};
      begin[1]=1; begin[5]=1;
      begin[11]=SC_TX_SCHEMA_SIZE & 127; begin[12]=SC_TX_SCHEMA_SIZE >> 7;
      assert(sc_tx_request(caps,sizeof caps,reply)==24 && reply[23]==0);
      assert(sc_tx_request(begin,sizeof begin,reply)==24 && reply[1]==SC_UNSUPPORTED);
      assert(sc_tx.state==SC_IDLE && sc_tx.token==0 && sc_tx.received==0); }
    /* Full SysEx collection excludes F0/F7 and is consumed before legacy editor. */
    { const uint8_t f[]={0xf0,0x7d,0x46,0x4c,72,0,0xf7}; unsigned i;
      for(i=0;i<sizeof f;i++) sysex_byte(f[i]); }
    assert(sx_ready && sx_frame_len==5);
    assert(!ota_frame_get(&p,&n)); assert(!sx_ready && so_w==10);
    assert((sx_out_q[0]>>8 & 255)==0xf0);
    assert((sx_out_q[1]>>16 & 255)==72);
#else
    /* Production default omits staging RAM and leaves unknown commands to editor. */
    { const uint8_t f[]={0xf0,0x7d,0x46,0x4c,72,0,0xf7}; unsigned i;
      for(i=0;i<sizeof f;i++) sysex_byte(f[i]); }
    assert(ota_frame_get(&p,&n) && n==5 && p[3]==72);
    assert(so_w==0); ota_frame_done();
#endif
    /* Legacy INFO remains available and passes through unchanged. */
    { const uint8_t f[]={0xf0,0x7d,0x46,0x4c,1,0xf7}; unsigned i;
      for(i=0;i<sizeof f;i++) sysex_byte(f[i]); }
    assert(ota_frame_get(&p,&n) && n==4 && p[3]==1); ota_frame_done();
#if T_SCENE
    old=sc_tx.epoch; sc_tx_lost=1; assert(!ota_frame_get(&p,&n)); assert(sc_tx.epoch==old+1);
    old=sc_tx.epoch; usb_detach(); assert(sc_tx.epoch==old+1 && !usb.up);
#else
    (void)old; usb_detach(); assert(!usb.up);
#endif
    puts("scene transaction OTA USB interception/loss/legacy passthrough PASS");
}

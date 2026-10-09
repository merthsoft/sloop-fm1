/* SPDX-License-Identifier: GPL-3.0-only */
/* Included by usb.c; portable standalone host-testable state machine. */
#include "scene_transaction.h"
static sc_tx_state sc_tx = { .epoch = 1 };
static sc_tx_hooks sc_hooks;
static volatile uint8_t sc_tx_lost;
#define SC_TX_PUBLISH() __asm__ volatile("" ::: "memory")
static uint32_t sc_u14(const uint8_t *p) { return p[0] | (uint32_t)p[1] << 7; }
static uint32_t sc_u28(const uint8_t *p) { return sc_u14(p) | sc_u14(p+2) << 14; }
static uint32_t sc_crc(const uint8_t *p, uint32_t n)
{
    uint32_t c=~0u, i, b;
    for(i=0;i<n;i++) { c^=p[i]; for(b=0;b<8;b++) c=(c>>1)^((0u-(c&1u))&0xedb88320u); }
    return ~c;
}
static int sc_tx_schema_valid(const uint8_t *p, uint32_t n)
{
    uint32_t t,i,j,o=4;
    if(n!=SC_TX_SCHEMA_SIZE || p[0]!=1 || p[1]>127 || p[2]>127 ||
       sc_u14(p+1)<20 || sc_u14(p+1)>300 || p[3]!=7) return 0;
    for(t=0;t<3;t++) {
        for(i=0;i<142;i++) if(p[o+i]>127) return 0;
        o+=142;
        if(!p[o] || p[o]>64 || p[o+1]>8) return 0;
        o+=2;
        for(i=0;i<64;i++,o+=11) {
            for(j=0;j<4;j++) if(p[o+j]>127) return 0;
            if(p[o+4]>4 || p[o+5]>2 || p[o+6]>3 || p[o+7]>127 || p[o+10]>63) return 0;
        }
        for(i=0;i<16;i++,o++) for(j=0;j<4;j++) if(((p[o]>>(2*j))&3)==3) return 0;
        for(i=0;i<24;i++,o+=5)
            if((p[o]!=255 && p[o]>=64) || p[o+1]>127 || p[o+2]>127 || p[o+3]>127 || p[o+4]) return 0;
    }
    return o==n;
}
static int sc_enabled(void) { return sc_hooks.revision && sc_hooks.validate && sc_hooks.apply && sc_hooks.enter && sc_hooks.leave && sc_hooks.schedule; }
static void sc_enter(void) { if(sc_enabled()) sc_hooks.enter(); }
static void sc_leave(void) { if(sc_enabled()) sc_hooks.leave(); }
/* USB ISR only raises loss; main loop invokes reset before processing frames.
 * parent must also cancel at stop/seek/restart and expire abandoned preparation. */
static void sc_tx_reset(void)
{
    sc_enter();
    sc_tx.state=SC_CANCELED; sc_tx.reason=SC_OK; sc_tx.token=0; sc_tx.last_token=0;
    sc_tx.received=0;
    sc_tx_lost=0;
    sc_tx.epoch = sc_tx.epoch==0x0fffffffu ? 0 : sc_tx.epoch+1;
    sc_leave(); /* zero epoch permanently disables extension until reboot */
}
/* Called immediately before outgoing sequencer events at an EXACT due tick.
 * Late calls fail closed; revision conflict never enters apply. */
static void sc_tx_boundary(uint32_t tick, uint8_t kind)
{
    if(sc_tx.state!=SC_QUEUED) return;
    SC_TX_PUBLISH();
    if(sc_tx_lost) { sc_tx.state=SC_CANCELED; return; }
    if(tick<sc_tx.boundary) return;
    if(tick!=sc_tx.boundary || kind!=sc_tx.boundary_kind) {
        sc_tx.state=SC_FAILED; sc_tx.reason=SC_TOO_LATE; return;
    }
    if(sc_hooks.revision()!=sc_tx.revision) {
        sc_tx.state=SC_FAILED; sc_tx.reason=SC_CONFLICT; return;
    }
    sc_hooks.apply(sc_tx.payload,sc_tx.length);
    sc_tx.applied_revision=sc_hooks.revision(); SC_TX_PUBLISH(); sc_tx.state=SC_APPLIED;
}
/* Args: op, epoch u28, token u14, operation body. Caps is op only.
 * Replies always fixed 24 bytes: op, rc, state, epoch u28, token u14,
 * received u14, revision u28, boundary u28, kind, max u14, chunk, flags.
 * CRC supplied as 5 base128 digits (last <=15), not truncated u28. */
static uint32_t sc_tx_request(const uint8_t *a, uint32_t n, uint8_t *r)
{
    uint32_t op=n?a[0]:127, rc=SC_INVALID, x=0, i;
    int locked=0;
    if(op==SC_CAPS && n==1) { rc=SC_OK; goto reply; }
    if(n<7 || op<SC_BEGIN || op>SC_STATUS) goto reply;
    for(i=0;i<n;i++) if(a[i]>127) goto reply;
    if(!sc_tx.epoch || sc_u28(a+1)!=sc_tx.epoch) { rc=SC_CONFLICT; goto reply; }
    x=sc_u14(a+5);
    if(!x) goto reply;
    if(op!=SC_BEGIN && x!=sc_tx.token) { rc=SC_CONFLICT; goto reply; }
    if(op==SC_BEGIN) {
        uint32_t len, crc;
        if(n!=18 || a[17]>15) goto reply;
        len=sc_u14(a+11); crc=sc_u28(a+13) | (uint32_t)a[17]<<28;
        if(!sc_enabled()) { rc=SC_UNSUPPORTED; goto reply; }
        if(len!=SC_TX_SCHEMA_SIZE || len>SC_TX_MAX) goto reply;
        if(x==sc_tx.token && sc_tx.state>=SC_RECEIVING && sc_tx.state<=SC_QUEUED) {
            rc=len==sc_tx.length && crc==sc_tx.crc && sc_u28(a+7)==sc_tx.revision ? SC_OK:SC_CONFLICT; goto reply;
        }
        if(sc_tx.state==SC_RECEIVING || sc_tx.state==SC_PREPARED || sc_tx.state==SC_QUEUED) { rc=SC_BUSY; goto reply; }
        if(x<=sc_tx.last_token || sc_hooks.revision()!=sc_u28(a+7)) { rc=SC_CONFLICT; goto reply; }
        sc_tx.token=sc_tx.last_token=(uint16_t)x; sc_tx.length=(uint16_t)len;
        sc_tx.crc=crc; sc_tx.revision=sc_u28(a+7); sc_tx.received=0;
        sc_tx.boundary=0; sc_tx.applied_revision=0; sc_tx.reason=SC_OK; sc_tx.state=SC_RECEIVING; rc=SC_OK;
    } else if(op==SC_DATA) {
        uint8_t bytes[SC_TX_CHUNK]; uint32_t off,k=0,pos=9,j,mask;
        if(n<11 || sc_tx.state!=SC_RECEIVING) goto reply;
        off=sc_u14(a+7);
        while(pos<n) {
            mask=a[pos++];
            if(pos==n) goto reply;
            for(j=0;j<7 && pos<n;j++) { if(k==SC_TX_CHUNK) goto reply; bytes[k++]=a[pos++]|((mask>>j)&1u)<<7; }
            if((mask>>j)!=0) goto reply;
        }
        if(off+k>sc_tx.length) goto reply;
        if(off<sc_tx.received) { rc=off+k<=sc_tx.received && !memcmp(sc_tx.payload+off,bytes,k)?SC_OK:SC_CONFLICT; goto reply; }
        if(off!=sc_tx.received) { rc=SC_MISSING; goto reply; }
        memcpy(sc_tx.payload+off,bytes,k); sc_tx.received+=(uint16_t)k; rc=SC_OK;
    } else if(op==SC_PREPARE) {
        if(n!=7) goto reply;
        if(sc_tx.state==SC_PREPARED || sc_tx.state==SC_QUEUED || sc_tx.state==SC_APPLIED) { rc=SC_OK; goto reply; }
        if(sc_tx.state!=SC_RECEIVING) goto reply;
        if(sc_tx.received!=sc_tx.length) { rc=SC_MISSING; goto reply; }
        if(sc_crc(sc_tx.payload,sc_tx.length)!=sc_tx.crc || !sc_tx_schema_valid(sc_tx.payload,sc_tx.length)) goto reply;
        rc=(uint32_t)sc_hooks.validate(sc_tx.payload,sc_tx.length);
        if(rc>SC_TOO_LATE) rc=SC_INVALID;
        if(rc==SC_OK && sc_hooks.revision()!=sc_tx.revision) rc=SC_CONFLICT;
        if(rc==SC_OK) sc_tx.state=SC_PREPARED;
    } else {
        sc_enter(); locked=1;
        if(op==SC_COMMIT) {
            if(n!=12 || a[11]>2) goto reply;
            if(sc_tx.state==SC_QUEUED || sc_tx.state==SC_APPLIED) {
                rc=sc_u28(a+7)==sc_tx.boundary && a[11]==sc_tx.boundary_kind?SC_OK:SC_CONFLICT; goto reply;
            }
            if(sc_tx.state!=SC_PREPARED) goto reply;
            if(sc_hooks.revision()!=sc_tx.revision) { rc=SC_CONFLICT; goto reply; }
            rc=(uint32_t)sc_hooks.schedule(sc_u28(a+7),a[11]);
            if(rc>SC_TOO_LATE) rc=SC_INVALID;
            if(rc!=SC_OK) goto reply;
            sc_tx.boundary=sc_u28(a+7); sc_tx.boundary_kind=a[11]; SC_TX_PUBLISH(); sc_tx.state=SC_QUEUED; rc=SC_OK;
        } else if(op==SC_CANCEL) {
            if(n!=7) goto reply;
            if(sc_tx.state==SC_APPLIED || sc_tx.state==SC_FAILED) { rc=SC_TOO_LATE; goto reply; }
            sc_tx.state=SC_CANCELED; rc=SC_OK;
        } else if(op==SC_STATUS && n==7) rc=sc_tx.state==SC_FAILED?sc_tx.reason:SC_OK;
    }
reply:
    if(!locked) sc_enter();
    r[0]=(uint8_t)op; r[1]=(uint8_t)rc; r[2]=sc_tx.state;
    for(i=0;i<4;i++) r[3+i]=(uint8_t)((sc_tx.epoch>>(7*i))&127);
    r[7]=sc_tx.token&127; r[8]=sc_tx.token>>7; r[9]=sc_tx.received&127; r[10]=sc_tx.received>>7;
    x=sc_tx.state==SC_APPLIED?sc_tx.applied_revision:sc_tx.revision;
    if(op==SC_CAPS && sc_enabled()) x=sc_hooks.revision();
    for(i=0;i<4;i++) { r[11+i]=(uint8_t)((x>>(7*i))&127); r[15+i]=(uint8_t)((sc_tx.boundary>>(7*i))&127); }
    r[19]=sc_tx.boundary_kind; r[20]=SC_TX_MAX&127; r[21]=SC_TX_MAX>>7; r[22]=SC_TX_CHUNK;
    r[23]=sc_enabled() && sc_tx.epoch ? 7:0;
    sc_leave(); return 24;
}

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../firmware/src/scene_transaction.c"
static uint32_t revision=42, applies, enters, leaves;
static int validation_result, scheduling_result;
static uint32_t rev(void) { return revision; }
static int valid(const uint8_t *p,uint32_t n) { (void)p; (void)n; return validation_result; }
static void apply(const uint8_t *p,uint32_t n) { assert(sc_tx_schema_valid(p,n)); applies++; revision++; }
static void enter(void) { enters++; }
static void leave(void) { leaves++; }
static int schedule(uint32_t tick,uint8_t kind) { (void)tick; (void)kind; return scheduling_result; }
static void put(uint8_t *p,uint32_t x,uint32_t n) { while(n--) { *p++=x&127; x>>=7; } }
static uint8_t data[SC_TX_SCHEMA_SIZE], reply[24], args[128];
static int request(uint32_t op,uint32_t token,uint32_t n)
{
    args[0]=op; put(args+1,sc_tx.epoch,4); put(args+5,token,2);
    assert(sc_tx_request(args,n,reply)==24); assert(enters==leaves); return reply[1];
}
static void payload(void)
{
    uint32_t t,i,o=4;
    memset(data,0,sizeof data); data[0]=1; put(data+1,120,2); data[3]=7;
    for(t=0;t<3;t++) {
        o+=142; data[o]=64; data[o+1]=2; o+=2;
        for(i=0;i<64;i++,o+=11) { data[o+8]=255; data[o+9]=255; data[o+10]=32; }
        o+=16; for(i=0;i<24;i++,o+=5) data[o]=255;
    }
    assert(o==sizeof data); assert(sc_tx_schema_valid(data,sizeof data));
}
static int begin(uint32_t token)
{
    put(args+7,revision,4); put(args+11,sizeof data,2); put(args+13,sc_crc(data,sizeof data),5);
    return request(SC_BEGIN,token,18);
}
static int chunk(uint32_t token,uint32_t off,uint32_t n)
{
    uint32_t i,j,k,pos=9;
    put(args+7,off,2);
    for(i=0;i<n;i+=k) {
        uint32_t m=pos++; k=n-i>7?7:n-i; args[m]=0;
        for(j=0;j<k;j++) { args[m]|=(data[off+i+j]>>7)<<j; args[pos++]=data[off+i+j]&127; }
    }
    return request(SC_DATA,token,pos);
}
static void stage(uint32_t token)
{
    uint32_t off;
    assert(begin(token)==SC_OK);
    assert(request(SC_PREPARE,token,7)==SC_MISSING);
    assert(chunk(token,96,96)==SC_MISSING);
    for(off=0;off<sizeof data;off+=96) assert(chunk(token,off,sizeof data-off>96?96:sizeof data-off)==SC_OK);
    assert(chunk(token,0,96)==SC_OK); /* duplicate chunk */
    assert(request(SC_PREPARE,token,7)==SC_OK); assert(sc_tx.state==SC_PREPARED);
}
static int commit(uint32_t token,uint32_t tick,uint32_t kind)
{ put(args+7,tick,4); args[11]=kind; return request(SC_COMMIT,token,12); }
int main(void)
{
    uint32_t off,i;
    payload();
    assert(request(SC_CAPS,0,1)==SC_OK && reply[23]==0);
    assert(begin(1)==SC_UNSUPPORTED);
    sc_hooks=(sc_tx_hooks){rev,valid,apply,enter,leave,schedule};
    assert(request(SC_CAPS,0,1)==SC_OK && reply[23]==7);
    stage(1); assert(commit(1,100,1)==SC_OK); assert(commit(1,100,1)==SC_OK);
    assert(commit(1,101,1)==SC_CONFLICT); sc_tx_boundary(99,1); assert(applies==0);
    sc_tx_boundary(100,1); assert(applies==1 && sc_tx.state==SC_APPLIED);
    sc_tx_boundary(100,1); assert(applies==1); assert(commit(1,100,1)==SC_OK);
    assert(request(SC_CANCEL,1,7)==SC_TOO_LATE); assert(begin(1)==SC_CONFLICT);
    stage(2); assert(commit(2,200,0)==SC_OK); revision++; sc_tx_boundary(200,0);
    assert(applies==1 && request(SC_STATUS,2,7)==SC_CONFLICT && sc_tx.state==SC_FAILED);
    stage(3); assert(commit(3,300,2)==SC_OK); sc_tx_boundary(301,2);
    assert(request(SC_STATUS,3,7)==SC_TOO_LATE && applies==1);
    stage(4); assert(commit(4,400,0)==SC_OK); assert(request(SC_CANCEL,4,7)==SC_OK);
    sc_tx_boundary(400,0); assert(applies==1); assert(request(SC_CANCEL,4,7)==SC_OK);
    stage(5); assert(commit(5,500,0)==SC_OK); sc_tx_lost=1; sc_tx_boundary(500,0); assert(applies==1);
    sc_tx_reset(); sc_tx_lost=0; assert(sc_tx.token==0 && sc_tx.epoch==2); stage(1);
    assert(request(SC_CANCEL,1,7)==SC_OK);
    assert(begin(2)==SC_OK); assert(chunk(2,0,96)==SC_OK); data[0]^=1;
    assert(chunk(2,0,96)==SC_CONFLICT); data[0]^=1;
    /* altered received contents are caught by the seal CRC */
    for(off=96;off<sizeof data;off+=96) assert(chunk(2,off,sizeof data-off>96?96:sizeof data-off)==SC_OK);
    sc_tx.payload[100]^=1; assert(request(SC_PREPARE,2,7)==SC_INVALID);
    assert(request(SC_CANCEL,2,7)==SC_OK);
    stage(3); scheduling_result=SC_TOO_LATE; assert(commit(3,1,0)==SC_TOO_LATE);
    assert(sc_tx.state==SC_PREPARED); scheduling_result=SC_OK;
    revision++; assert(commit(3,1000,0)==SC_CONFLICT); assert(request(SC_CANCEL,3,7)==SC_OK);
    assert(begin(4)==SC_OK);
    for(off=0;off<sizeof data;off+=96) assert(chunk(4,off,sizeof data-off>96?96:sizeof data-off)==SC_OK);
    validation_result=SC_MISSING; assert(request(SC_PREPARE,4,7)==SC_MISSING);
    validation_result=SC_UNSUPPORTED; assert(request(SC_PREPARE,4,7)==SC_UNSUPPORTED);
    validation_result=SC_OK; revision++; assert(request(SC_PREPARE,4,7)==SC_CONFLICT);
    assert(request(SC_CANCEL,4,7)==SC_OK);
    for(i=0;i<SC_TX_SCHEMA_SIZE;i++) {
        uint8_t saved=data[i]; data[i]=255;
        /* every malformed candidate is bounded, including valid arbitrary packed bytes */
        (void)sc_tx_schema_valid(data,sizeof data); data[i]=saved;
    }
    /* arbitrary packet fuzz never escapes bounded buffers */
    for(i=0;i<20000;i++) {
        uint32_t n=rand()%sizeof args,j; for(j=0;j<n;j++) args[j]=rand()&255;
        sc_tx_request(args,n,reply); assert(sc_tx.received<=SC_TX_MAX);
    }
    printf("scene transaction PASS: lifecycle/faults + 20000 malformed requests; state=%zu hooks=%zu schema=%u max=%u\n",
           sizeof(sc_tx),sizeof(sc_hooks),SC_TX_SCHEMA_SIZE,SC_TX_MAX);
}

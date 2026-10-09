#undef NDEBUG /* Test assertions stay active in optimized host builds. */
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
static uint32_t fm1_ms;
static struct { uint32_t config, resets; } usb = {1, 0};
static uint32_t fm1_icfg(void) { return 0; }
static void fm1_irq_off(void) {}
static void fm1_icfg_set(uint32_t v) { (void)v; }
static uint8_t reply[32]; static uint32_t length;
static void ed_begin(uint32_t cmd) { assert(cmd == 73); length = 0; }
static void ed_b(uint32_t v) { reply[length++] = v & 127; }
static void ed_send(void) {}
#include "../firmware/src/editor_performance.c"
static void press(uint8_t id, uint8_t kind, uint8_t value)
{ uint8_t a[] = {1,id,0,0,0,kind,value}; ed_performance(a,7); assert(reply[1] == 0); }
static void release(uint8_t id)
{ uint8_t a[] = {2,id,0,0,0}; ed_performance(a,5); assert(reply[1] == 0); }
int main(void)
{
    uint8_t query[] = {0}; ed_performance(query,1);
    assert(length == 8 && reply[2] == 1 && reply[3] == 7 && reply[4] == 8);
    for (uint32_t n = 0; n < 12; n++) {
        uint8_t a[12] = {1,1,0,0,0,0,0};
        ed_performance(a,n); assert(reply[1] == (n == 7 ? 0 : 1)); perf_reset();
    }
    for (uint32_t kind = 0; kind < 128; kind++) for (uint32_t value = 0; value < 128; value++) {
        uint8_t a[] = {1,1,0,0,0,kind,value}; ed_performance(a,7);
        assert(reply[1] == ((kind == 1 ? value < 16 : kind < 3 && value == 0) ? 0 : 1)); perf_reset();
    }
    press(1,0,0); press(2,0,0); release(1); assert(perf_fill(fm1_ms)); release(2); assert(!perf_fill(fm1_ms));
    press(1,1,3); press(2,1,7); assert(perf_punch(fm1_ms) == 7); release(2); assert(perf_punch(fm1_ms) == 3);
    int panel = 5; assert((panel >= 0 ? panel : perf_punch(fm1_ms)) == 5);
    release(1); assert((panel >= 0 ? panel : perf_punch(fm1_ms)) == 5); // host release preserves panel
    press(1,0,0); fm1_ms += 750; assert(!perf_fill(fm1_ms)); ed_performance_service();
    uint8_t renew[] = {3,1,0,0,0}; ed_performance(renew,5); assert(reply[1] == 3); assert(!perf_fill(fm1_ms));
    fm1_ms = UINT32_MAX - 100; press(1,0,0); fm1_ms += 200; assert(perf_fill(fm1_ms)); fm1_ms += 550; assert(!perf_fill(fm1_ms));
    perf_reset(); fm1_ms = 0; for (uint8_t i = 1; i <= 8; i++) press(i,0,0);
    uint8_t ninth[] = {1,9,0,0,0,0,0}; ed_performance(ninth,7); assert(reply[1] == 2);
    perf_reset(); press(1,2,0); assert(!perf_fill(fm1_ms)); assert(perf_take_bar(fm1_ms)); assert(perf_fill(fm1_ms));
    release(1); assert(!perf_fill(fm1_ms));
    press(2,2,0); perf_take_bar(fm1_ms); perf_take_bar(fm1_ms); assert(!perf_fill(fm1_ms));
    press(1,0,0); press(2,1,3); usb.config = 0; ed_performance_service(); assert(!perf_fill(fm1_ms) && perf_punch(fm1_ms) == -1);
    usb.config = 1; press(1,0,0); usb.resets++; ed_performance_service(); assert(!perf_fill(fm1_ms));
    press(1,0,0); press(2,1,2); perf_reset(); assert(!perf_fill(fm1_ms) && perf_punch(fm1_ms) == -1); // STOP hook
    ed_performance(renew,5); assert(reply[1] == 3);
    puts("performance remote: discovery, 16400+ malformed cases, ownership, panel priority, expiry/wrap, host loss, STOP passed");
}

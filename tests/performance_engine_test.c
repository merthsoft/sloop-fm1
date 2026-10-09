#undef NDEBUG /* Test assertions stay active in optimized host builds. */
/* Real sequencer and punch DSP regression, using the production editor handler. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
static uint32_t fm1_icfg(void) { return 0; }
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
static void render_punch(void)
{
    for (uint32_t i = 0; i < 8; i++) { int32_t l[CTL] = {0}, r[CTL] = {0}; punch_process(l,r,CTL); }
}
int main(void)
{
    host_tracks_init(); usb.config = 1; fm1_ms = 10;
    song.g[G_BPM] = 120; song.sel = 0; song.playing = 0;
    punch.req = punch.cur = -1; punch.g = 0;
    ly_bit[LY_FX] = 1u << 2; fm1_in.buttons = ly_bit[LY_FX]; fm1_in.notes = 1;
    events_block(CTL); assert(punch.req == 0); // actual physical keyboard ownership
    press(1,1,3); render_punch(); assert(punch.cur == 0);
    release(1); render_punch(); assert(punch.req == 0 && punch.cur == 0);
    press(2,1,5); fm1_in.notes = 0; events_block(CTL); render_punch(); assert(punch.req == -1 && punch.cur == 5);
    release(2); render_punch(); assert(punch.cur == -1);
    fm1_in.buttons = 0;
    fill_held = 1; press(3,0,0); release(3); events_block(CTL); assert(fill_held && fill_now);
    fill_held = 0; events_block(CTL); assert(!fill_now);
    song.playing = 1; clk_beat = 4; clk_pos = 0; fill_last_bar = 0; fill_arm = 1;
    press(4,2,0); events_block(CTL);
    assert(!fill_arm && fill_bar_on && fill_now && perf_fill(fm1_ms));
    release(4); events_block(CTL); assert(!perf_fill(fm1_ms) && fill_bar_on && fill_now);
    clk_beat = 8; clk_pos = 0; events_block(CTL); assert(!fill_bar_on && !fill_now);
    // Host-only bar must stop immediately on release; it must not leak into physical state.
    press(5,2,0); clk_beat = 12; clk_pos = 0; events_block(CTL); assert(fill_now && !fill_bar_on);
    release(5); events_block(CTL); assert(!fill_now);
    // Expiry before the boundary must never start a fill.
    press(6,2,0); fm1_ms += 750; clk_beat = 16; clk_pos = 0; events_block(CTL); assert(!fill_now && !fill_bar_on);
    press(7,0,0); press(8,1,4); render_punch(); assert(punch.cur == 4);
    transport_req = 2; events_block(CTL); render_punch(); assert(!perf_fill(fm1_ms) && !fill_now && punch.cur == -1);
    uint8_t renew[] = {3,7,0,0,0}; ed_performance(renew,5); assert(reply[1] == 3);
    puts("performance engine: real panel keyboard, punch DSP priority/release, simultaneous fill boundary, expiry and STOP passed");
    return 0;
}

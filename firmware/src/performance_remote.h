/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef PERFORMANCE_REMOTE_H
#define PERFORMANCE_REMOTE_H
/* Main-loop writes with interrupts excluded; audio reads bounded aggregates.
 * Panel state is never written here. Tokens are unique per connection gesture. */
#define PERF_OWNERS 8u
#define PERF_LEASE_MS 750u
static volatile struct { uint32_t token, until; uint8_t kind, value; } perf_owner[PERF_OWNERS];
static void perf_reset(void)
{
    uint32_t i;
    for (i = 0; i < PERF_OWNERS; i++) perf_owner[i].token = 0;
}
static uint32_t perf_live(uint32_t i, uint32_t now)
{
    return perf_owner[i].token && (int32_t)(perf_owner[i].until - now) > 0;
}
static uint32_t perf_fill(uint32_t now)
{
    uint32_t i;
    for (i = 0; i < PERF_OWNERS; i++)
        if (perf_live(i, now) && (perf_owner[i].kind == 0u || perf_owner[i].kind == 3u)) return 1;
    return 0;
}
static int32_t perf_punch(uint32_t now)
{
    uint32_t i, token = 0; int32_t fx = -1;
    for (i = 0; i < PERF_OWNERS; i++)
        if (perf_live(i, now) && perf_owner[i].kind == 1u && perf_owner[i].token > token) {
            token = perf_owner[i].token; fx = perf_owner[i].value;
        }
    return fx;
}
static uint32_t perf_take_bar(uint32_t now)
{
    uint32_t i, on = 0;
    for (i = 0; i < PERF_OWNERS; i++)
        if (perf_owner[i].kind == 3u) perf_owner[i].token = 0;
        else if (perf_live(i, now) && perf_owner[i].kind == 2u) { on = 1; perf_owner[i].kind = 3u; }
    return on;
}
#endif

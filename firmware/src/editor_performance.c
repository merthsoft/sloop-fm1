/* SPDX-License-Identifier: GPL-3.0-only */
/* Command 73, schema 1. Included after editor response helpers. */
#include "performance_remote.h"
static uint32_t perf_usb_resets;
static void ed_performance_service(void)
{
    uint32_t i, irq = fm1_icfg();
    fm1_irq_off();
    if (!usb.config || usb.resets != perf_usb_resets) perf_reset();
    perf_usb_resets = usb.resets;
    for (i = 0; i < PERF_OWNERS; i++)
        if (!perf_live(i, fm1_ms)) perf_owner[i].token = 0;
    fm1_icfg_set(irq);
}
static void ed_performance(const uint8_t *a, uint32_t n)
{
    uint32_t op = n ? a[0] : 127u, status = 0, token = 0, i, slot = PERF_OWNERS;
    uint32_t irq;
    ed_performance_service();
    if (op == 0u && n == 1u) {
        ed_begin(73); ed_b(0); ed_b(0); ed_b(1); ed_b(7); ed_b(PERF_OWNERS);
        ed_b(PERF_LEASE_MS & 127u); ed_b(PERF_LEASE_MS >> 7); ed_b(16); ed_send(); return;
    }
    if (op > 4u || op == 0u || n != (op == 1u ? 7u : op == 4u ? 1u : 5u)) status = 1;
    for (i = 0; i < n; i++) if (a[i] > 127u) status = 1;
    if (!status && op != 4u) {
        token = a[1] | (uint32_t)a[2] << 7 | (uint32_t)a[3] << 14 | (uint32_t)a[4] << 21;
        if (!token || (op == 1u && (a[5] > 2u || (a[5] == 1u ? a[6] >= 16u : a[6] != 0u)))) status = 1;
    }
    irq = fm1_icfg(); fm1_irq_off();
    if (!status) {
        if (op == 4u) perf_reset();
        else {
            for (i = 0; i < PERF_OWNERS; i++) if (perf_owner[i].token == token) { slot = i; break; }
            if (op == 2u) { if (slot < PERF_OWNERS) perf_owner[slot].token = 0; }
            else if (op == 3u) {
                if (slot == PERF_OWNERS) status = 3;
                else perf_owner[slot].until = fm1_ms + PERF_LEASE_MS;
            } else {
                if (slot < PERF_OWNERS && (perf_owner[slot].kind != a[5] || perf_owner[slot].value != a[6])) status = 1;
                if (slot == PERF_OWNERS) for (i = 0; i < PERF_OWNERS; i++) if (!perf_owner[i].token) { slot = i; break; }
                if (slot == PERF_OWNERS) status = 2;
                if (!status) {
                    perf_owner[slot].kind = a[5]; perf_owner[slot].value = a[6];
                    perf_owner[slot].until = fm1_ms + PERF_LEASE_MS; perf_owner[slot].token = token;
                }
            }
        }
    }
    fm1_icfg_set(irq);
    ed_begin(73); ed_b(op); ed_b(status);
    for (i = 0; i < 4u; i++) ed_b(token >> (7u * i));
    ed_send();
}

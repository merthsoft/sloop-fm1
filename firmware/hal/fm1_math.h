/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
/* pi32v2 signed clamp for proven lo <= hi. No memory, shared state or calls. */
static inline __attribute__((always_inline)) int32_t fm1_clamp_ordered(int32_t v,int32_t lo,int32_t hi)
{
    /* smax writes v before smin reads hi: early-clobber is required. */
    __asm__("%0 = smax(%0, %1)\n\t%0 = smin(%0, %2)" : "+&r"(v) : "r"(lo), "r"(hi));
    return v;
}

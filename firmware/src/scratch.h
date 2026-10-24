/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Andre Rezemini (the SLOOP ram branch, github.com/andrerezemini/sloop-lab) */
/* One scratch for the main loop's short-lived buffers that are never alive at the same time:
 *   st     the storage payload (storage.c st_buf): filled and consumed inside one st_load / st_save /
 *          st_current call and its caller's next lines (dsu_boot, fm6_bank_scan read it right after);
 *   vis    the visualiser's FFT real/imaginary vectors: consumed within vis_update;
 *   snap   the HOME oscilloscope's (ui_draw.c graph_scope): refilled by every call;
 *   smp    sample upload/backup decode: consumed before its editor request returns;
 *   dec    the decoded OTA frame (ota.c ota_dec): decoded and read inside one ota_take caller.
 * None is touched by the audio interrupt or by TIMER5's poll; none is read after a return to the main
 * loop; a flash access never happens inside a draw, nor a draw inside a flash access. 3,840 B instead
 * of 7,552. Each user asserts its size against its own constant. */
#pragma once
#include <stdint.h>
static union {
    uint8_t st[4096 - 256];                  /* ST_PAYLOAD_MAX (storage.c) */
    int16_t vis[2][512];                     /* VIS_FFT (ui_vis.c): real, imaginary */
    int16_t snap[512];                       /* SCOPE_N (ui_draw.c) */
    uint8_t smp[512]; /* Editor sample upload/backup decode, main loop only. */
    uint8_t dec[640];                        /* OTA_FRAME_MAX (ota.c) */
} main_scratch __attribute__((aligned(4)));

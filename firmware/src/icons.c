/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* The NEON UI draws only tempo and tape header icons. Generate a compact
 * atlas from the original artwork; tests can select the full atlas as an oracle. */
#ifndef FELUCCA_ICON_HEADER
#define FELUCCA_ICON_HEADER "felucca_header_icons.h"
#endif
#include FELUCCA_ICON_HEADER
#ifndef FELUCCA_ICONS
#define FELUCCA_ICONS (FELUCCA_ICONS_N > 0)   /* on when the atlas has icons */
#endif
#if FELUCCA_ICONS && FELUCCA_ICONS_N == 0
#error "FELUCCA_ICONS=1 but felucca_icons.h has no icons (assets/icons.png missing?)"
#endif
#if FELUCCA_ICONS
/* 2-bit icon, ink levels 1..3 = a third .. all of colour c (blended onto black, like cv_text) */
UI_SMALL static void cv_icon(int32_t x, int32_t y, uint32_t id, uint16_t c)
{
    uint16_t ramp[4];
    uint32_t a, i, j;
    const uint8_t *p;
    if (id >= FELUCCA_ICONS_N)
        return;
    for (a = 0; a < 4u; a++)
        ramp[a] = rgb565_over(c, a, 3u);
    for (j = 0; j < ICON_CELL; j++) {
#if ICON_ROWS_PACKED
        p=ICON_ROWS[ICON_ROW_ID[id][j]];
#else
        p=ICON_DATA[id];
#endif
        for (i = 0; i < ICON_CELL; i++) {
            uint32_t k =
#if ICON_ROWS_PACKED
                i;
#else
                j * ICON_CELL + i;
#endif
            uint32_t v = (p[k >> 2] >> (6u - 2u * (k & 3u))) & 3u;
            if (v)
                cv_pset(x + (int32_t)i, y + (int32_t)j, ramp[v]);
        }
    }
}
#else
UI_SMALL static void cv_icon(int32_t x, int32_t y, uint32_t id, uint16_t c) { (void)x; (void)y; (void)id; (void)c; }
#endif

/* SPDX-License-Identifier: GPL-3.0-only */
/* Test the actual renderer against original BDF pixels, including clipping,
 * palette colours, Latin-1, '?' fallback, and scaled large-font metrics.
 * Run once with packed assets and once with an old unmarked four-bit header. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p) {}
#include "gfx.c"
#include "font_reference.h"

static uint16_t expected[64 * 40];

static void check(const felucca_font_t *font, unsigned ch, int scale, int x, int y, uint16_t colour)
{
    char text[] = {(char)ch, 0};
    unsigned code = ch, gx, gy;
    if (scale == 2 && code >= 'a' && code <= 'z') code -= 32;
    if (code < 32 || code > (scale == 2 ? 95 : 255) || (code >= 127 && code < 160)) code = '?';
    unsigned gi = code - 32;
    memset(expected, 0, sizeof expected);
    cv_begin(64, 40, 0);
    cv_oy = 3;
    int end = cv_text(x, y, font, text, colour);
    assert(end == x + REF_ADV[gi] * scale);
    assert(text_w(font, text) == REF_ADV[gi] * scale);
    for (gy = 0; gy < 16u * scale; gy++)
        for (gx = 0; gx < 12u * scale; gx++) {
            int px = x - 2 * scale + (int)gx, py = y + 3 + (int)gy;
            if (REF_PIX[gi][(gy / scale) * 12 + gx / scale] && px >= 0 && px < 64 && py >= 0 && py < 40)
                expected[py * 64 + px] = swap16(colour);
        }
    if (memcmp(expected, cv_px, sizeof expected)) {
        fprintf(stderr, "font rendering mismatch: char %u scale %d x %d y %d colour %u\n", ch, scale, x, y, colour);
        assert(0);
    }
    cv_oy = 0;
}

int main(void)
{
    const uint16_t colours[] = {0xffff, RGB(56, 172, 222), RGB(255, 98, 26), 0};
    const int pos[][2] = {{10, 0}, {-2, -7}, {57, 25}};
    unsigned ch, c, p;
    for (ch = 1; ch <= 255; ch++)
        for (c = 0; c < 4; c++)
            for (p = 0; p < 3; p++) {
                check(&FONT_S, ch, 1, pos[p][0], pos[p][1], colours[c]);
                check(&FONT_L, ch, 2, pos[p][0], pos[p][1], colours[c]);
            }
    printf("font renderer: 6,120 pixel/metric comparisons PASS (format %d)\n", FONT_DATA_BITS);
    return 0;
}

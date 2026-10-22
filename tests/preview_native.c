/* SPDX-License-Identifier: GPL-3.0-only */
/* Same browser adapter on the host, for sample-by-sample WASM comparisons. */
#include <stdio.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
#define __attribute__(x)
#include "../firmware/host/preview.c"
int main(void)
{
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    int32_t config[5 + P_COUNT + G_COUNT];
    if (fread(config,sizeof *config,5,stdin)!=5 || config[4]<0 || config[4]>G_COUNT) return 1;
    if (fread(config+5,sizeof *config,P_COUNT+config[4],stdin)!=(uint32_t)(P_COUNT+config[4])) return 1;
    preview_init();
    preview_render(); /* the adapter obtains its initial output buffer before syncing */
    preview_track(0, config[0], config[1]);
    for (uint32_t i = 0; i < P_COUNT; i++) preview_param(0, i, config[5 + i]);
    /* Omitted appended globals retain preview_init defaults, as on the WASM side. */
    for (uint32_t i = 0; i < (uint32_t)config[4]; i++) preview_global(i, config[5 + P_COUNT + i]);
    if (fread(transfer, FM6_PACKED, 1, stdin) != 1) return 1;
    preview_fm6(0);
    preview_note(0, config[2], 1);
    for (uint32_t i = 0; i < (uint32_t)config[3]; i += CTL) {
        if (i == (uint32_t)config[3] / 2u) preview_note(0, config[2], 0);
        if (fwrite(preview_render(), sizeof output, 1, stdout) != 1) return 1;
    }
    return 0;
}

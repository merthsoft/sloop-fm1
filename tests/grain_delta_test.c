/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>
int main(void)
{
    /* All possible nonnegative 15-bit steps, not only the 89 IMA table values;
     * both signs and the full nibble domain retain the original magnitude. */
    for(int32_t step=0;step<=32767;step++)for(uint32_t code=0;code<16;code++) {
        int32_t expected=(step>>3)+((code&4u)?step:0)+
                         ((code&2u)?step>>1:0)+((code&1u)?step>>2:0);
        assert(ima_delta(step,code)==expected);
    }
    puts("GRAIN IMA magnitude: all 524288 step/nibble combinations match original rounded additions PASS");
    return 0;
}

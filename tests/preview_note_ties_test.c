/* The real browser step adapter must retain all four seven-bit tie flags. */
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#define __attribute__(x)
#include "../firmware/host/preview.c"
int main(void)
{
    preview_init();
    for (unsigned k=0;k<4;k++) {
        step_t s={.note={60,64,67,71},.n=4,.time=ST_NOTE,.vel=100,
            .flags=SF_ACCENT|SF_SLIDE|SF_TIE(k),.lvl=0x39,.rat=0x24};
        memcpy(transfer,&s,sizeof s);transfer[10]=7;transfer[11]=FC_NOFILL;
        preview_step(0,k);
        assert(!memcmp(&s,&trk[0].step[k],sizeof s));
        assert(trk[0].micro[k]==7 && step_fill(&trk[0],k)==FC_NOFILL);
    }
    transfer[6]=255;preview_step(0,0);
    assert(trk[0].step[0].flags==(SF_ACCENT|SF_SLIDE|SF_TIES));
    puts("browser step adapter: all four continuations, dynamics and timing preserved PASS");
    return 0;
}

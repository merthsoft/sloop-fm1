/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>

int main(void)
{
    /* C zero initialization models the target before the common cold hook. */
    assert(!punch.req && !punch.cur && !punch.fg && !punch_filter.cur);
    host_tracks_init();
    assert(punch.req==-1 && punch.cur==-1 && punch.fg==8192 && punch_filter.cur==-1);
    assert(!punch.g && !punch.w && !punch_filter.gain && !punch_gate.gain);
    int32_t l[CTL],r[CTL];
    for (unsigned i=0;i<CTL;i++) { l[i]=12345; r[i]=-6789; }
    punch_process(l,r,CTL);
    for (unsigned i=0;i<CTL;i++) assert(l[i]==12345 && r[i]==-6789);
    punch_hold(0,PX_REV,1); punch_hold(0,PX_LPF,1);
    punch_process(l,r,CTL);
    int cur=punch.cur, filter=punch_filter.cur, gain=punch.g;
    /* Resetting host track defaults is not another hardware boot. */
    host_tracks_init();
    assert(punch.req==PX_REV && punch.cur==cur && punch.g==gain && punch_filter.cur==filter);
    puts("punch cold startup: common defaults, dry first block, no repeat bootstrap PASS");
    return 0;
}

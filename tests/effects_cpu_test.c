/* SPDX-License-Identifier: GPL-3.0-only */
/* Relative host cost + two-core identity; not a physical deadline measurement. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
typedef struct { uint64_t hash,ns; } result_t;
static result_t render(int mode,int cores) {
    int fd[2];assert(!pipe(fd));pid_t child=fork();assert(child>=0);
    if(!child) {
        close(fd[0]);setenv("CORES",cores==2?"2":"1",1);cpu1_host_mode();cpu1_units=-1;
        host_tracks_init();punch_clear();song.playing=1;song.g[G_BPM]=120;song.g[G_SYNC]=0;usb.detached=0;usb.config=1;
        host_preset(&trk[0],ENGI_FM6,0);host_preset(&trk[1],ENGI_GRAIN,0);host_preset(&trk[2],6,3);
        for(int k=0;k<NPART;k++) {
            trk[k].p[P_AMODE]=0;trk[k].p[P_SUS]=127;trk[k].p[P_REV]=127;
            for(int j=0;j<(k==2?2:3);j++)trk_note_on(&trk[k],48+j*7+k,100);
        }
        song.g[G_RMODEL]=mode<8?(mode&1):mode-6;
        if(mode<8&&(mode&4))song.g[G_VWOW]=song.g[G_VTONE]=song.g[G_VCRACKLE]=127;
        if(mode<8&&(mode&2)) {punch_hold(2,PX_REV,1);punch_hold(2,PX_DOUBLE,1);punch_hold(2,PX_TEL,1);punch_hold(2,PX_GATE,1);}
        result_t result={0xcbf29ce484222325ull,0};int32_t out[2*CTL];
        for(int b=0;b<5000;b++) {
            if(!(b%680))trk_note_on(TDRUM,36,110);
            uint64_t before=now_ns();mix_block(out,CTL);uint64_t elapsed=now_ns()-before;
            if(b>=904) {result.ns+=elapsed;for(int i=0;i<2*CTL;i++)result.hash=(result.hash^(uint32_t)out[i])*1099511628211ull;}
        }
        assert(write(fd[1],&result,sizeof result)==sizeof result);_exit(0);
    }
    close(fd[1]);result_t result;assert(read(fd[0],&result,sizeof result)==sizeof result);close(fd[0]);int status;waitpid(child,&status,0);assert(WIFEXITED(status)&&!WEXITSTATUS(status));return result;
}
int main(void) {
    double base=0;
    for(int mode=0;mode<11;mode++) {
        result_t one=render(mode,1),two=render(mode,2);assert(one.hash==two.hash);
        double ns=(double)one.ns/(4096u*CTL);if(!mode)base=ns;
        printf("effects profile %d: %.1f ns/frame, %.3fx baseline; two-core output identical %016llx\n",mode,ns,ns/base,(unsigned long long)one.hash);
    }
    return 0;
}

/* Oracle producer: compile ONLY against upstream 4ec1ac8's source, never this fork. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i) { static const uint8_t e[]={0,1,3};return i<3?e[i]:0; }
#include "../firmware/src/project.c"
int main(int argc,char **argv)
{
    host_tracks_init();
    for(uint32_t k=0;k<NPART;k++) {
        track_t *t=&trk[k];
        t->p[P_AMODE]=(int16_t)(12+k);t->p[P_SLEN]=32;t->p[P_AHOLD]=1;
        t->step[0]=(step_t){.note={48,55,60,64},.n=4,.time=ST_NOTE,.vel=99,.flags=SF_ACCENT};
        t->step[1]=(step_t){.note={48,55,62,65},.n=4,.time=ST_NOTE,.vel=88,.flags=SF_CONT(0)|SF_CONT(1)};
        t->step[2]=t->step[1];t->step[2].flags=SF_CONT_ALL;
        t->micro[1]=-7;step_fill_set(t,2,FC_FILL);lock_set(t,0,P_LEVEL,83);
    }
    song.g[G_SWING]=64;
    drum_tune[0]=-7;drum_dec[0]=3;
    project_t p;proj_capture(&p);
    FILE *f=fopen(argv[1],"wb");if(!f)return 1;fwrite(&p,1,sizeof p,f);fclose(f);return 0;
}

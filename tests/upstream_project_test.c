/* Canonical upstream FUN6 fixture, produced by tests/fixtures/upstream271_project.c. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i) { static const uint8_t e[]={0,1,3};return i<3?e[i]:0; }
#include "../firmware/src/project.c"
static void roundtrip(const char *path, int legato)
{
    project_t wire, imported, saved;
    FILE *f=fopen(path,"rb"); assert(f);
    assert(fread(&wire,1,sizeof wire,f)==sizeof wire && fgetc(f)==EOF);fclose(f);
    host_tracks_init();assert(proj_import(&imported,&wire,sizeof wire));
    assert(!memcmp(&wire,&imported,sizeof wire));
    proj_apply(&imported,1);proj_capture(&saved);
    assert(!memcmp(&wire,&saved,sizeof wire));
    for(uint32_t k=0;k<NPART;k++) {
        assert(trk[k].p[P_AMODE]==(int16_t)(12+k));
        assert(trk[k].step[1].flags==(SF_CONT(0)|SF_CONT(1)));
        assert(trk[k].step[2].flags==SF_CONT_ALL);
        if(legato) { assert(trk[k].step[3].flags==SF_LEG);assert(trk[k].step[4].flags==(SF_LEG|SF_CONT(0))); }
        assert(trk[k].micro[1]==-7 && step_fill(&trk[k],2)==FC_FILL);
    }
    assert(drum_tune[0]==-7 && drum_dec[0]==3);
    puts("upstream 2.7.1: FUN6 load/apply/save byte-identical; arp IDs, four continuations and drum sound settings PASS");
}
int main(void) { roundtrip("tests/fixtures/upstream271.fun6",0);roundtrip("tests/fixtures/upstream28.fun6",1);puts("upstream 2.8 legato and continuations: byte-identical import/apply/save PASS");return 0;}

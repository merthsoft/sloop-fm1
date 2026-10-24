/* SPDX-License-Identifier: GPL-3.0-only */
/* Deterministic legacy wire corpus: freeze the complete converted output of
 * the pre-direct-conversion codec, including count mapping and zeroed padding.
 * Frozen baseline: 9fd9ddc codec, 1000 records per FUN1..6, seven validity/length
 * variants each. LEGACY_DUMP optionally emits accepted records for byte cmp. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i) {static const uint8_t e[]={0,1,3};return i<3?e[i]:0;}
#include "../firmware/src/project.c"
static uint32_t rng_audit=0xB7349123u;
static uint32_t next_audit(void) { rng_audit^=rng_audit<<13; rng_audit^=rng_audit>>17; rng_audit^=rng_audit<<5; return rng_audit; }
int main(void)
{
    static const uint32_t sizes[]={sizeof(project_v1_t),sizeof(project_v2_t),sizeof(project_v3_t),sizeof(project_v4_t),sizeof(project_v5_t),sizeof(project_t)};
    uint8_t *raw = malloc(3840);
    uint32_t digest=0x811c9dc5u,cases=0;
    if (!raw) return 3;
    project_t q;
    FILE *dump = getenv("LEGACY_DUMP") ? fopen(getenv("LEGACY_DUMP"), "wb") : 0;
    for(uint32_t ver=0;ver<6;ver++) for(uint32_t trial=0;trial<1000;trial++) {
        uint32_t n=sizes[ver];
        for(uint32_t k=0;k<960;k++) {
            uint32_t word=next_audit();
            for (uint32_t j=0;j<4;j++) raw[4*k+j]=(uint8_t)(word>>(8*j));
        }
        uint32_t magic=0x46554e31u+ver;
        memcpy(raw,&magic,4);memcpy(raw+4,&n,4);
        uint32_t sum=proj_hash(raw,n-4);memcpy(raw+n-4,&sum,4);
        for(uint32_t variant=0;variant<7;variant++) {
            int len=(int)n;
            if(variant==1)raw[0]^=128;
            if(variant==2)raw[4]^=4;
            if(variant==3)raw[n-4]^=1;
            if(variant==4)len--;
            if(variant==5)len++;
            if(variant==6)len=-1;
            memset(&q,0xA9,sizeof q);
            int ok=proj_import(&q,raw,len);
            if(ok!=(variant==0))return 2;
            if (ok && dump) fwrite(&q, sizeof q, 1, dump);
            digest=(digest^(uint32_t)ok)*16777619u;
            digest=(digest^proj_hash(&q,sizeof q))*16777619u;
            if(variant==1)raw[0]^=128;
            if(variant==2)raw[4]^=4;
            if(variant==3)raw[n-4]^=1;
            cases++;
        }
    }
    printf("%u import/output/rejection cases: %08x\n",cases,digest);
    if (dump) fclose(dump);
    free(raw);
    return digest != 0x8b386c47u;
}

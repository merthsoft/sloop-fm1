/* SPDX-License-Identifier: GPL-3.0-only */
/* Fork-only trailer in the existing settings A/B sectors. No new flash region
 * and no change to the advertised upstream settings/project payloads. */
enum { SNAPSHOT_FIELDS=P_LD_AMP+1+(P_REV-P_DIST+1)+4+(P_COUNT-P_ITYPE) };
typedef struct {
    uint32_t project;
    struct { uint8_t ready,position;int8_t value[2][SNAPSHOT_FIELDS]; } t[NPART];
} snapshot_record_t;
typedef struct {
    uint32_t magic;
    snapshot_record_t section[4],working;
    uint32_t crc;
} snapshot_bank_t;
_Static_assert(NELEM(N_MORPH_SCOPE)*NELEM(N_EVOLVE_CYCLE)<=128,"snapshot motion metadata fits seven spare bits");
#define SNAPSHOT_MAGIC 0x3142464Du /* MFB1 */
static snapshot_record_t snapshot_sections[4] __attribute__((section(".pool")));
static uint32_t snapshot_saved;

UI_SMALL static void snapshot_capture(snapshot_record_t *r,const project_t *p)
{
    memset(r,0,sizeof *r);r->project=p->sum;
    for(uint32_t k=0;k<NPART;k++) {
        /* Six scopes x nineteen times fit seven spare bits. Legacy zero bits
         * decode as ALL / 1S; record size, captures and upstream prefix stay put. */
        uint32_t meta=(uint32_t)snapshot_p[k][4]+NELEM(N_MORPH_SCOPE)*(uint32_t)snapshot_p[k][6];
        r->t[k].ready=(uint8_t)((snapshot_p[k][3]&3)|(meta<<2));
        r->t[k].position=(uint8_t)(snapshot_p[k][0]|((meta&64u)<<1));
        uint32_t j=0;
        for(uint32_t id=0;id<P_COUNT;id++)if(snapshot_param(id)) {
            for(uint32_t side=0;side<2;side++)if((r->t[k].ready&3u)&(1u<<side)) {
                int16_t v=snapshot_values[k][side][id];
                r->t[k].value[side][j]=(int8_t)v;
            }
            j++;
        }
    }
}
UI_SMALL static void snapshot_restore(const snapshot_record_t *r,const project_t *p)
{
    if(r->project!=p->sum)return;
    for(uint32_t k=0;k<NPART;k++) {
        uint32_t meta=(r->t[k].ready>>2)|((r->t[k].position&128u)>>1);
        if(meta>=NELEM(N_MORPH_SCOPE)*NELEM(N_EVOLVE_CYCLE))continue;
        snapshot_p[k][0]=r->t[k].position&127u;snapshot_p[k][3]=r->t[k].ready&3u;
        snapshot_p[k][4]=(int16_t)(meta%NELEM(N_MORPH_SCOPE));
        snapshot_p[k][6]=(int16_t)(meta/NELEM(N_MORPH_SCOPE));snapshot_p[k][5]=0;
        uint32_t j=0;
        for(uint32_t id=0;id<P_COUNT;id++)if(snapshot_param(id)) {
            for(uint32_t side=0;side<2;side++)snapshot_values[k][side][id]=r->t[k].value[side][j];
            j++;
        }
    }
}
static void snapshot_section_apply(const project_t *p)
{
    for(uint32_t s=0;s<4;s++)if(p==&proj_slot[s])snapshot_restore(&snapshot_sections[s],p);
}
#if FELUCCA_FLASH
_Static_assert(sizeof(snapshot_bank_t)<=ST_SECTOR-ST_TAIL_OFF,"snapshot trailer fits settings sector");
_Static_assert(sizeof(snapshot_bank_t)<=sizeof proj_tmp,"snapshot staging fits existing scratch");
UI_SMALL static snapshot_bank_t *snapshot_read(void)
{
    snapshot_bank_t *b=(snapshot_bank_t *)&proj_tmp;st_hdr_t h;
    int copy=st_current(OBJ_SETTINGS,&h);
    if(copy<0 || ST_PAYLOAD_OFF+h.len>ST_TAIL_OFF ||
       st_read(st_sector(OBJ_SETTINGS,(uint32_t)copy)+ST_TAIL_OFF,b,sizeof *b) ||
       b->magic!=SNAPSHOT_MAGIC || b->crc!=st_crc32(b,sizeof *b-4u))return 0;
    return b;
}
UI_SMALL static snapshot_bank_t *snapshot_prepare(void)
{
    snapshot_bank_t *b=(snapshot_bank_t *)&proj_tmp;
    b->magic=SNAPSHOT_MAGIC;memcpy(b->section,snapshot_sections,sizeof snapshot_sections);
    proj_capture(&autosave_buf);snapshot_capture(&b->working,&autosave_buf);
    b->crc=st_crc32(b,sizeof *b-4u);
    return b;
}
#endif
UI_SMALL static int snapshot_pending(void)
{
#if FELUCCA_FLASH
    return snapshot_prepare()->crc!=snapshot_saved;
#else
    return 0;
#endif
}
UI_SMALL static void snapshot_boot(void)
{
    memset(snapshot_sections,0,sizeof snapshot_sections);snapshot_saved=0;
#if FELUCCA_FLASH
    snapshot_bank_t *b=snapshot_read();
    if(b){memcpy(snapshot_sections,b->section,sizeof snapshot_sections);snapshot_saved=b->crc;}
#endif
}
UI_SMALL static void snapshot_resume(const project_t *p)
{
#if FELUCCA_FLASH
    snapshot_bank_t *b=snapshot_read();
    if(b)snapshot_restore(&b->working,p);
#else
    (void)p;
#endif
}

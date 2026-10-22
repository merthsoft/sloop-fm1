/* Actual native controls and card rendering for delay characters. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
#include <assert.h>
int main(int argc,char **argv){
 outdir=argc>1?argv[1]:"build/host";panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(NPALETTES-1u);
 assert(song.g[G_LTYPE]==0);
 go_home();open_family(FAM_FX);
 for(unsigned p=0;p<NPAGES;p++)if(PAGES[p].scope==SC_GLOBAL&&PAGES[p].id[0]==G_DTYPE)ui.page=p;
 ui.home=0;page_entered();frame();
 for(unsigned c=0;c<4;c++){int16_t *vp;assert(page_desc(cur_page(),c,&vp)&&vp);}
 for(unsigned mode=0;mode<3;mode++){
  song.g[G_DTYPE]=mode;ui.force=1;frame();assert(!strcmp(ui.big_v[0],N_DTYPE[mode]));
  char name[24];snprintf(name,sizeof name,"delay-type-%u",mode);ppm(name);
 }
 encs[panel.enc[EN_K1]]=100;frame();assert(song.g[G_DTYPE]==2);
 encs[panel.enc[EN_K1]]=-100;frame();assert(song.g[G_DTYPE]==0);
 song.g[G_DTIME]=1;song.g[G_DFDBK]=30;song.g[G_DMIX]=30;
 encs[panel.enc[EN_K2]]=1;encs[panel.enc[EN_K3]]=1;encs[panel.enc[EN_K4]]=1;frame();
 assert(song.g[G_DTIME]>1&&song.g[G_DFDBK]>30&&song.g[G_DMIX]>30);
 song.g[G_DTYPE]=1;song.g[G_DDRIVE]=70;song.g[G_DCOLOR]=70;frame();
 encs[panel.enc[EN_K2]]=1;encs[panel.enc[EN_K3]]=1;frame();
 assert(song.g[G_DDRIVE]>70&&song.g[G_DCOLOR]>70&&!strcmp(ui.big_l[1],"DRIVE")&&!strcmp(ui.big_l[2],"TONE"));
 song.g[G_DTYPE]=2;song.g[G_DBITS]=8;song.g[G_DHOLD]=2;frame();
 encs[panel.enc[EN_K2]]=1;encs[panel.enc[EN_K3]]=1;frame();
 assert(song.g[G_DBITS]==9&&song.g[G_DHOLD]==3&&!strcmp(ui.big_l[1],"BITS")&&!strcmp(ui.big_l[2],"RATE"));
 for(unsigned p=0;p<NPAGES;p++)if(PAGES[p].scope==SC_GLOBAL&&PAGES[p].id[0]==G_LTYPE)ui.page=p;
 page_entered();frame();encs[panel.enc[EN_K1]]=-100;frame();
 assert(song.g[G_LTYPE]==0&&!strcmp(ui.big_v[0],"OFF"));
 encs[panel.enc[EN_K1]]=1;frame();assert(song.g[G_LTYPE]==1&&!strcmp(ui.big_v[0],"VINYL"));
 puts("delay UI: three types, four active controls, full labels and default LOFI OFF PASS");return 0;
}

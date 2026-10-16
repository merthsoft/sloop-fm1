/* Pixel-for-pixel production icon rendering, including clipping and alpha. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
#include "icon_oracle.h"
int main(void)
{
    const uint16_t colors[]={0xffff,0xf800,0x07e0,0x001f};
    const int pos[][2]={{-3,-4},{0,0},{8,7},{35,27}};
    unsigned comparisons=0;
    assert(FELUCCA_ICONS_N==ICON_ORACLE_N && ICON_CELL==12);
    for(unsigned id=0;id<ICON_ORACLE_N;id++)for(unsigned c=0;c<4;c++)for(unsigned p=0;p<4;p++) {
        uint16_t color=colors[c];cv_oy=0;cv_begin(40,32,0x1082);
        cv_icon(pos[p][0],pos[p][1],id,color);
        for(int y=0;y<32;y++)for(int x=0;x<40;x++) {
            int sx=x-pos[p][0],sy=y-pos[p][1];uint16_t want=0x1082;
            if(sx>=0&&sx<12&&sy>=0&&sy<12) {
                unsigned alpha=ICON_ORACLE[id][sy*12+sx];
                if(alpha)want=(uint16_t)((((color>>11)*alpha/3)<<11)|
                    ((((color>>5)&63)*alpha/3)<<5)|((color&31)*alpha/3));
            }
            assert(cv_px[y*40+x]==swap16(want));comparisons++;
        }
    }
    printf("icon renderer: %u pixel comparisons PASS\n",comparisons);return 0;
}

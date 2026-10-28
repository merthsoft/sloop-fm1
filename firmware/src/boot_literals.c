/* SPDX-License-Identifier: GPL-3.0-only */
/* Restored once, before any UI/storage/audio startup or core-1 launch. */
extern uint8_t boot_literal_main_start[],boot_literal_pool_start[];
extern const uint8_t boot_literal_main_blob[],boot_literal_pool_blob[];
extern const uint8_t boot_literal_pairs[];
extern const uint32_t boot_literal_sizes[7];
/* LZ4 restores compact tokens to the start of their final destination. Expand
 * backwards: every token emits at least one byte, so unread tokens stay intact.
 * The build-time grammar is acyclic; the bounded stack also rejects bad trees. */
static __attribute__((minsize,noinline)) int boot_literal_restore(uint8_t *dst,const uint8_t *src,const uint32_t *s)
{
    if(s[1]>s[2])return 0;
    const boot_table_t t={dst,src,s[0],s[1],1,0,1,0};
    if(!boot_table_unpack(&t))return 0;
    uint32_t at=s[2];
    for(uint32_t i=s[1];i;) {
        uint8_t stack[8];uint32_t n=1;stack[0]=dst[--i];
        while(n) {
            uint32_t v=stack[--n];
            if(v>=128u && v-128u<boot_literal_sizes[6]) {
                const uint8_t *pair=boot_literal_pairs+2u*(v-128u);
                if(pair[0] || pair[1]) {
                    if(n+2u>sizeof stack)return 0;
                    stack[n++]=pair[0];stack[n++]=pair[1];
                    continue;
                }
            }
            if(at<=i)return 0;
            dst[--at]=(uint8_t)v;
        }
    }
    return !at;
}
static int boot_literals_init(void)
{
    return boot_literal_restore(boot_literal_main_start,boot_literal_main_blob,boot_literal_sizes) &&
           boot_literal_restore(boot_literal_pool_start,boot_literal_pool_blob,boot_literal_sizes+3);
}

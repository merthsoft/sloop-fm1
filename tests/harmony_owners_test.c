#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/src/harmony_owners.h"
static harmony_owners s;
static harmony_delta d;
static void put(harmony_owner_id id, const uint8_t *p, unsigned n)
{ assert(harmony_owner_replace(&s, id, p, n, &d)); }
static void trace(unsigned offs, unsigned ons, const uint8_t *off, const uint8_t *on)
{
    assert(d.off_count == offs && d.on_count == ons);
    assert(!offs || !memcmp(d.off, off, offs));
    assert(!ons || !memcmp(d.on, on, ons));
}
int main(void)
{
    harmony_owner_id a = {HARMONY_LOCAL,0,60}, b = {HARMONY_LOCAL,0,65};
    harmony_owner_id usb = {HARMONY_USB,2,60}, trs = {HARMONY_TRS,2,60};
    uint8_t c[] = {60,64,67}, f[] = {60,65,69}, minor[] = {60,63,67};
    uint8_t edges[] = {0,127,0,127}, invalid[] = {128};
    unsigned order, i;
    for (order = 0; order < 2; ++order) {
        harmony_owners_init(&s);
        put(a,c,3); trace(0,3,0,c);
        put(b,f,3); trace(0,2,0,(uint8_t[]){65,69});
        harmony_owner_release(&s, order ? b : a, &d);
        trace(2,0, order ? (uint8_t[]){65,69} : (uint8_t[]){64,67},0);
        assert(s.membership[60] == 1);
        harmony_owner_release(&s, order ? a : b, &d);
        trace(3,0,order ? c : f,0);
        assert(!s.membership[60]);
    }
    put(a,c,3); put(a,minor,3); trace(1,1,(uint8_t[]){64},(uint8_t[]){63});
    assert(s.membership[60] == 1 && s.membership[67] == 1);
    put(usb,minor,3); put(trs,minor,3); put(usb,minor,3); trace(0,0,0,0);
    assert(s.membership[60] == 3); /* repeated USB note-on is replacement */
    harmony_owner_release(&s,usb,&d); trace(0,0,0,0);
    harmony_owner_release(&s,a,&d); trace(0,0,0,0);
    harmony_owner_release(&s,trs,&d); trace(3,0,minor,0);
    put(a,edges,4); trace(0,2,0,(uint8_t[]){0,127});
    assert(!harmony_owner_replace(&s,a,invalid,1,&d));
    assert(s.membership[0] && s.membership[127]);
    harmony_owners_clear(&s);
    for (i = 0; i < HARMONY_OWNER_CAP; ++i) {
        harmony_owner_id id = {HARMONY_USB,0,(uint8_t)i}; put(id,c,3);
    }
    assert(!harmony_owner_replace(&s,b,f,3,&d)); trace(0,0,0,0);
    assert(s.membership[60] == HARMONY_OWNER_CAP);
    /* Existing-owner revoice and releases always work at saturation. */
    put((harmony_owner_id){HARMONY_USB,0,0}, minor,3);
    harmony_owner_release(&s,(harmony_owner_id){HARMONY_USB,0,0},&d);
    trace(1,0,(uint8_t[]){63},0);
    put(b,f,3);
    harmony_owners_clear(&s);
    for (i = 0; i < 128; ++i) assert(!s.membership[i]);
    for (i = 0; i < HARMONY_OWNER_CAP; ++i) assert(!s.owners[i].active);
    harmony_owner_release(&s,b,&d); trace(0,0,0,0);
    printf("harmony owner event traces passed; state=%zu delta=%zu owner=%zu bytes\n",
           sizeof(s),sizeof(d),sizeof(harmony_owner));
    return 0;
}

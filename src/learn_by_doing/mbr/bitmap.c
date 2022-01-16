#include "bitmap.h"
#include "stdint.h"
#include "string.h"
#include "printk.h"
#include "interrupt.h"
#include "debug.h"

void bitmap_init(struct bitmap* btmap) {
    memset(btmap->bits, 0, btmap->btmap_bytes_len);
}

bool bitmap_scan_test(struct bitmap* btmap, uint32_t bit_idx) {
    return btmap->bits[bit_idx/8] & (BITMAP_MASK << (bit_idx % 8));
}

int bitmap_scan(struct bitmap* btmap, uint32_t cnt) {
    uint32_t last_bad_idx = 0;
    uint32_t last_good_idx = 0;
    for(uint32_t i=0; i < btmap->btmap_bytes_len*8; i++) {
        if (bitmap_scan_test(btmap, i)) {
            last_bad_idx = i;
            continue;
        } else {
            last_good_idx = i;
            if((last_good_idx - last_bad_idx) == cnt)
                return last_bad_idx + 1;
        }
    }
    return -1;
}

void bitmap_set(struct bitmap* btmap, uint32_t bit_idx, int8_t value) {
    ASSERT((value == 0) || (value == 1));
    // set value
    if(value)
        btmap->bits[bit_idx/8] |= (BITMAP_MASK << (bit_idx % 8));
    else
        btmap->bits[bit_idx/8] &= ~(BITMAP_MASK << (bit_idx % 8));
}

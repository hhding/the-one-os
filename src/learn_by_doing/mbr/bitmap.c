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
    return btmap->bits[bit_idx/8] & (1 << bit_idx % 8);
}



#ifndef __GAKU_H__
#define __GAKU_H__

#include <stdint.h>

typedef struct GakuData {
    struct {
        uint16_t x;
        uint16_t y;
    } position;
    struct {
        uint16_t w;
        uint16_t h;
    } size;
} GakuData;

#endif

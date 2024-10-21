#ifndef __SSD1306_FONTS_H
#define __SSD1306_FONTS_H

#include "stm32l4xx_hal.h"

typedef struct {
    uint8_t fontWidth; /* Font width in pixels */
    uint8_t fontHeight; /* Font height in pixels */
    const uint16_t *data; /* Pointer to data font data array */
} tm_font_def_ts;

extern tm_font_def_ts TM_Font_7x10;
extern tm_font_def_ts TM_Font_11x18;
extern tm_font_def_ts TM_Font_16x26;

#endif

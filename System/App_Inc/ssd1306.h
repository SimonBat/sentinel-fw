#ifndef __SSD1306_H
#define __SSD1306_H

#include <stdlib.h>
#include <string.h>
#include "stm32l4xx_hal.h"
#include "ssd1306_fonts.h"

/* SSD1306 width in pixels */
#define SSD1306_WIDTH    		128U
/* SSD1306 LCD height in pixels */
#define SSD1306_HEIGHT   		64U
#define SSD1306_ADDRESS    		0x78
/* Absolute value */
#define ABS(x)					((x)>0?(x):-(x))

/* SSD1306 color enumeration */
typedef enum {
	OLED_COLOR_BLACK = 0x00, /* Black color, no pixel */
	OLED_COLOR_WHITE = 0x01 /* Pixel is set. Color depends on LCD */
} ssd1306_color_te;

typedef enum {
	SSD1306_UPDATE_NOT_READY = 0x00,
	SSD1306_UPDATE_READY = 0x01
} ssd1306_state_te;

/* Private SSD1306 structure */
typedef struct {
	int16_t currentX;
	int16_t currentY;
	int16_t offset;
	uint8_t inverted;
	uint8_t initialized;
	uint8_t action;
	uint8_t request;
	uint8_t block;
	uint8_t command;
} ssd1306_ts;

/* Global functions definitions */
void SSD1306_Driver_Init(void);
void SSD1306_Driver_Update(void);
void SSD1306_Toggle_Invert(void);
void SSD1306_Fill(ssd1306_color_te _color);
void SSD1306_Draw_Pixel(int16_t _x, int16_t _y, ssd1306_color_te _color);
void SSD1306_Goto_XY(int16_t _x, int16_t _y, int16_t _offset);
char SSD1306_Draw_Char(char _ch, tm_font_def_ts *_font, ssd1306_color_te _color);
char SSD1306_Draw_String(int16_t _x, int16_t _y, int16_t _offset, char *_str, tm_font_def_ts *_font, ssd1306_color_te _color);
uint8_t SSD1306_Get_HEX_Digit(uint8_t _n);
void SSD1306_Draw_HEX(uint8_t _ch, tm_font_def_ts *_font, ssd1306_color_te _color);
void SSD1306_Draw_Line(int16_t _x0, int16_t _y0, int16_t _x1, int16_t _y1, ssd1306_color_te _color);
void SSD1306_Draw_Rectangle(int16_t _x, int16_t _y, uint8_t _w, uint8_t _h, ssd1306_color_te _color);
void SSD1306_Draw_Filled_Rectangle(int16_t _x, int16_t _y, uint8_t _w, uint8_t _h, ssd1306_color_te _color);
void SSD1306_Draw_Triangle(int16_t _x1, int16_t _y1, int16_t _x2, int16_t _y2, int16_t _x3, int16_t _y3, ssd1306_color_te _color);
void SSD1306_Draw_Filled_Triangle(int16_t _x1, int16_t _y1, int16_t _x2, int16_t _y2, int16_t _x3, int16_t _y3, ssd1306_color_te _color);
void SSD1306_Draw_Circle(int16_t _x0, int16_t _y0, uint8_t _r, ssd1306_color_te _color);
void SSD1306_Draw_Filled_Circle(int16_t _x0, int16_t _y0, uint8_t _r, ssd1306_color_te _color);

#endif

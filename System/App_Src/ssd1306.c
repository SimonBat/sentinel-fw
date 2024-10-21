/**
  ***************************************************************************************************************************************
  * @file     ssd1306.c
  * @owner    SimonasBat
  * @version  v0.0.1
  * @date     2021.09.06
  * @update   2021.09.06
  * @brief    sentinel v1.0
  ***************************************************************************************************************************************
  * @attention
  *
  * (Where to use)
  *
  ***************************************************************************************************************************************
  */

#include "ssd1306.h"
#include "i2c.h"

/* SSD1306 data buffer */
static ssd1306_ts SSD1306;
static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8U];
static const uint8_t SSD1306_INIT_SEQUENCE[] = {
	0xAE, /* display off */
	0x20, /* Set Memory Addressing Mode */
	0x10, /* 00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid */
	0xB0, /* Set Page Start Address for Page Addressing Mode,0-7 */
	0xC8, /* Set COM Output Scan Direction */
	0x00, /* Set low column address */
	0x10, /* Set high column address */
	0x40, /* Set start line address */
	0x81, /* Set contrast control register */
	0xD0, /* Normal 80 */
	0xA1, /* Set segment re-map 0 to 127 */
	0xA6, /* Set normal display */
	0xA8, /* Set multiplex ratio(1 to 64) */
	0x3F,
	0xA4, /* 0xa4,Output follows RAM content;0xa5,Output ignores RAM content */
	0xD3, /* Set display offset */
	0x00, /* Not offset */
	0xD5, /* Set display clock divide ratio/oscillator frequency, normal D5 */
	0xF0, /* Set divide ratio */
	0xD9, /* Set pre-charge period */
	0x55,
	0xDA, /* Set com pins hardware configuration */
	0x12,
	0xDB, /* Set vcomh */
	0x20, /* 0x20,0.77xVcc */
	0x8D, /* Set DC-DC enable */
	0x14,
	0xAF /* Turn on SSD1306 panel */
};

/**
  ***************************************************************************************************************************************
  * @brief  Initializes SSD1306 LCD
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void SSD1306_Driver_Init(void)
{
	uint8_t _cmd;

	for(uint8_t _idx = 0U; _idx < sizeof(SSD1306_INIT_SEQUENCE); _idx++)
	{
		_cmd = SSD1306_INIT_SEQUENCE[_idx];
		I2C_Driver_Write(SSD1306_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, &_cmd, 1U);
	}

	/* Clear screen */
	SSD1306_Fill(OLED_COLOR_BLACK);

	for(uint8_t _block = 0U; _block < 8U; _block++)
	{
		_cmd = 0xB0 + _block;
		I2C_Driver_Write(SSD1306_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, &_cmd, 1U);
		_cmd = 0x00;
		I2C_Driver_Write(SSD1306_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, &_cmd, 1U);
		_cmd = 0x10;
		I2C_Driver_Write(SSD1306_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, &_cmd, 1U);
		I2C_Driver_Write(SSD1306_ADDRESS, 0x40, I2C_MEMADD_SIZE_8BIT, &SSD1306_Buffer[SSD1306_WIDTH * _block], SSD1306_WIDTH);
	}

	/* Set default values */
	SSD1306.currentX = 0;
	SSD1306.currentY = 0;
	/* Initialized OK */
	SSD1306.initialized = 1U;
}

/**
  ***************************************************************************************************************************************
  * @brief  Updates buffer from internal RAM to LCD non blocking task handler
  * @note   This function must be called each time you do some changes to LCD, to update buffer from RAM to LCD
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void SSD1306_Driver_Update(void)
{
	uint8_t _cmd;

	if(SSD1306.initialized)
	{
		for(uint8_t _block = 0U; _block < 8U; _block++)
		{
			_cmd = 0xB0 + _block;
			I2C_Driver_Write(SSD1306_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, &_cmd, 1U);
			_cmd = 0x00;
			I2C_Driver_Write(SSD1306_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, &_cmd, 1U);
			_cmd = 0x10;
			I2C_Driver_Write(SSD1306_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, &_cmd, 1U);
			I2C_Driver_Write(SSD1306_ADDRESS, 0x40, I2C_MEMADD_SIZE_8BIT, &SSD1306_Buffer[SSD1306_WIDTH * _block], SSD1306_WIDTH);
		}
	}
}

/**
  ***************************************************************************************************************************************
  * @brief  Toggles pixels invertion inside internal RAM
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void SSD1306_Toggle_Invert(void)
{
	/* Toggle invert */
	SSD1306.inverted = !SSD1306.inverted;
	/* Do memory toggle */
	for(uint16_t _idx = 0U; _idx < sizeof(SSD1306_Buffer); _idx++){SSD1306_Buffer[_idx] = ~SSD1306_Buffer[_idx];}
}

/**
  ***************************************************************************************************************************************
  * @brief  Fills entire LCD with desired color
  * @param  Color (ssd1306_color_te)
  * @retval None
  ***************************************************************************************************************************************
  */
void SSD1306_Fill(ssd1306_color_te _color)
{
	/* Set memory */
	memset(SSD1306_Buffer, (OLED_COLOR_BLACK == _color) ? 0x00 : 0xFF, sizeof(SSD1306_Buffer));
}

/**
  ***************************************************************************************************************************************
  * @brief  Draws pixel at desired location
  * @param  X (int16_t), y (int16_t), color (ssd1306_color_te)
  * @retval None
  ***************************************************************************************************************************************
  */
void SSD1306_Draw_Pixel(int16_t _x, int16_t _y, ssd1306_color_te _color)
{
	if((_x >= SSD1306_WIDTH) || (_y >= SSD1306_HEIGHT)){return;}

	/* Check if pixels are inverted */
	if(SSD1306.inverted){_color = (ssd1306_color_te)!_color;}
	/* Set color */
	if(OLED_COLOR_WHITE == _color){SSD1306_Buffer[_x + (_y / 8U) * SSD1306_WIDTH] |= 1 << (_y % 8U);}
	else{SSD1306_Buffer[_x + (_y / 8U) * SSD1306_WIDTH] &= ~(1 << (_y % 8U));}
}

/**
  ***************************************************************************************************************************************
  * @brief  Sets cursor pointer to desired location for strings
  * @param  X (int16_t), y (int16_t). offset (int16_t)
  * @retval None
  ***************************************************************************************************************************************
  */
void SSD1306_Goto_XY(int16_t _x, int16_t _y, int16_t _offset)
{
	/* Set write pointers */
	SSD1306.currentX = _x;
	SSD1306.currentY = _y;
	SSD1306.offset = _offset;
}

/**
  ***************************************************************************************************************************************
  * @brief  Puts character to internal RAM
  * @param  Character (char), font (tm_font_def_ts), color (ssd1306_color_te)
  * @retval Character written (char)
  ***************************************************************************************************************************************
  */
char SSD1306_Draw_Char(char _ch, tm_font_def_ts *_font, ssd1306_color_te _color)
{
	/* Go through font */
	for(int32_t _i = 0; _i < _font->fontHeight; _i++)
	{
		int32_t _b = _font->data[(_ch - 32) * _font->fontHeight + _i];

		for(int32_t _j = 0; _j < _font->fontWidth; _j++)
		{
			if(((_b << _j) & 0x8000) && ((SSD1306.currentX + _j) >= SSD1306.offset))
			{SSD1306_Draw_Pixel(SSD1306.currentX + _j, (SSD1306.currentY + _i), (ssd1306_color_te) _color);}
			else if((SSD1306.currentX + _j) >= SSD1306.offset)
			{SSD1306_Draw_Pixel(SSD1306.currentX + _j, (SSD1306.currentY + _i), (ssd1306_color_te)!_color);}
		}
	}

	/* Increase pointer */
	SSD1306.currentX += _font->fontWidth;

	/* Return character written */
	return _ch;
}

/**
  ***************************************************************************************************************************************
  * @brief  Puts string to internal RAM
  * @param  X (int16_t), y (int16_t), offset (int16_t), string (char*), font (tm_font_def_ts*), color (ssd1306_color_te)
  * @retval Zero on success or character value when function failed (char)
  ***************************************************************************************************************************************
  */
char SSD1306_Draw_String(int16_t _x, int16_t _y, int16_t _offset, char *_str, tm_font_def_ts *_font, ssd1306_color_te _color)
{
	SSD1306_Goto_XY(_x,_y,_offset);

	/* Write characters */
	while (*_str)
	{
		/* Write character by character */
		if(SSD1306_Draw_Char(*_str, _font, _color) != *_str){return *_str;}
		/* Increase string pointer */
		_str++;
	}

	/* Everything OK, zero should be returned */
	return *_str;
}

/**
  ***************************************************************************************************************************************
  * @brief  Return hex digit of the byte
  * @param  Byte (uint8_t)
  * @retval HEX digit (uint8_t)
  ***************************************************************************************************************************************
  */
uint8_t SSD1306_Get_HEX_Digit(uint8_t _n)
{
	if(_n < 10U){return _n + '0';}
	else{return (_n - 10U) + 'A';}
}

/**
  ***************************************************************************************************************************************
  * @brief  Print HEX number
  * @param  Hex (uint8_t), font (tm_font_def_ts*), color (ssd1306_color_te)
  * @retval None
  ***************************************************************************************************************************************
  */
void SSD1306_Draw_HEX(uint8_t _ch, tm_font_def_ts *_font, ssd1306_color_te _color)
{
	SSD1306_Draw_Char(SSD1306_Get_HEX_Digit(_ch / 0x10), _font, _color);
	SSD1306_Draw_Char(SSD1306_Get_HEX_Digit(_ch % 0x10), _font, _color);
}

/**
  ***************************************************************************************************************************************
  * @brief  Draws line on LCD
  * @param  X0 (int16_t), y0 (int16_t), x1 (int16_t), y1 (int16_t), color (ssd1306_color_te)
  * @retval None
  ***************************************************************************************************************************************
  */
void SSD1306_Draw_Line(int16_t _x0, int16_t _y0, int16_t _x1, int16_t _y1, ssd1306_color_te _color)
{
	int16_t _dx, _dy, _sx, _sy, _err, _i, _tmp;

	/* Check for overflow */
	if(_x0 >= SSD1306_WIDTH){_x0 = SSD1306_WIDTH - 1;}
	if(_x1 >= SSD1306_WIDTH){_x1 = SSD1306_WIDTH - 1;}
	if(_y0 >= SSD1306_HEIGHT){_y0 = SSD1306_HEIGHT - 1;}
	if(_y1 >= SSD1306_HEIGHT){_y1 = SSD1306_HEIGHT - 1;}

	_dx = (_x0 < _x1) ? (_x1 - _x0) : (_x0 - _x1);
	_dy = (_y0 < _y1) ? (_y1 - _y0) : (_y0 - _y1);
	_sx = (_x0 < _x1) ? 1 : -1;
	_sy = (_y0 < _y1) ? 1 : -1;
	_err = ((_dx > _dy) ? _dx : -_dy) / 2;

	if(0 == _dx)
	{
		if(_y1 < _y0)
		{
			_tmp = _y1;
			_y1 = _y0;
			_y0 = _tmp;
		}

		if(_x1 < _x0){_x0 = _x1;}

		/* Vertical line */
		for(_i = _y0; _i <= _y1; _i++){SSD1306_Draw_Pixel(_x0, _i, _color);}

		/* Return from function */
		return;
	}

	if(0 == _dy)
	{
		if(_y1 < _y0){_y0 = _y1;}

		if(_x1 < _x0)
		{
			_tmp = _x1;
			_x1 = _x0;
			_x0 = _tmp;
		}

		/* Horizontal line */
		for(_i = _x0; _i <= _x1; _i++){SSD1306_Draw_Pixel(_i, _y0, _color);}

		/* Return from function */
		return;
	}

	while(1U)
	{
		SSD1306_Draw_Pixel(_x0, _y0, _color);
		if (_x0 == _x1 && _y0 == _y1){break;}
		int16_t _e2 = _err;

		if(_e2 > -_dx)
		{
			_err -= _dy;
			_x0 += _sx;
		}

		if(_e2 < _dy)
		{
			_err += _dx;
			_y0 += _sy;
		}
	}
}

/**
  ***************************************************************************************************************************************
  * @brief  Draws rectangle on LCD
  * @param  X (int16_t), y (int16_t), w (uint8_t), h (uint8_t), color (ssd1306_color_te)
  * @retval None
  ***************************************************************************************************************************************
  */
void SSD1306_Draw_Rectangle(int16_t _x, int16_t _y, uint8_t _w, uint8_t _h, ssd1306_color_te _color)
{
	/* Check input parameters */
	if((_x >= SSD1306_WIDTH) || (_y >= SSD1306_HEIGHT)){return;}

	/* Check width and height */
	if ((_x + _w) >= SSD1306_WIDTH){_w = SSD1306_WIDTH - _x;}
	if ((_y + _h) >= SSD1306_HEIGHT){_h = SSD1306_HEIGHT - _y;}

	/* Draw 4 lines */
	SSD1306_Draw_Line(_x, _y, _x + _w, _y, _color); /* Top line */
	SSD1306_Draw_Line(_x, _y + _h, _x + _w, _y + _h, _color); /* Bottom line */
	SSD1306_Draw_Line(_x, _y, _x, _y + _h, _color); /* Left line */
	SSD1306_Draw_Line(_x + _w, _y, _x + _w, _y + _h, _color); /* Right line */
}

/**
  ***************************************************************************************************************************************
  * @brief  Draws filled rectangle on LCD
  * @param  X (int16_t), y (int16_t), w (uint8_t), h (uint8_t), color (ssd1306_color_te)
  * @retval None
  ***************************************************************************************************************************************
  */
void SSD1306_Draw_Filled_Rectangle(int16_t _x, int16_t _y, uint8_t _w, uint8_t _h, ssd1306_color_te _color)
{
	/* Check input parameters */
	if((_x >= SSD1306_WIDTH) || (_y >= SSD1306_HEIGHT)){return;}

	/* Check width and height */
	if((_x + _w) >= SSD1306_WIDTH){_w = SSD1306_WIDTH - _x;}
	if((_y + _h) >= SSD1306_HEIGHT){_h = SSD1306_HEIGHT - _y;}

	/* Draw lines */
	for(uint8_t _i = 0U; _i <= _h; _i++){SSD1306_Draw_Line(_x, _y + _i, _x + _w, _y + _i, _color);}
}

/**
  ***************************************************************************************************************************************
  * @brief  Draws triangle on LCD
  * @param  X1 (int16_t), y1 (int16_t), x2 (int16_t), y2 (int16_t), x3 (int16_t), y3 (int16_t), color (ssd1306_color_te)
  * @retval None
  ***************************************************************************************************************************************
  */
void SSD1306_Draw_Triangle(int16_t _x1, int16_t _y1, int16_t _x2, int16_t _y2, int16_t _x3, int16_t _y3, ssd1306_color_te _color)
{
	/* Draw lines */
	SSD1306_Draw_Line(_x1, _y1, _x2, _y2, _color);
	SSD1306_Draw_Line(_x2, _y2, _x3, _y3, _color);
	SSD1306_Draw_Line(_x3, _y3, _x1, _y1, _color);
}

/**
  ***************************************************************************************************************************************
  * @brief  Draws filled triangle on LCD
  * @param  X1 (int16_t), y1 (int16_t), x2 (int16_t), y2 (int16_t), x3 (int16_t), y3 (int16_t), color (ssd1306_color_te)
  * @retval None
  ***************************************************************************************************************************************
  */
void SSD1306_Draw_Filled_Triangle(int16_t _x1, int16_t _y1, int16_t _x2, int16_t _y2, int16_t _x3, int16_t _y3, ssd1306_color_te _color)
{
	int16_t _deltax, _deltay, _x, _y, _xinc1, _xinc2, _yinc1, _yinc2, _den, _num, _numadd, _numpixels;

	_deltax = ABS(_x2 - _x1);
	_deltay = ABS(_y2 - _y1);
	_x = _x1;
	_y = _y1;

	if(_x2 >= _x1)
	{
		_xinc1 = 1;
		_xinc2 = 1;
	}
	else
	{
		_xinc1 = -1;
		_xinc2 = -1;
	}

	if(_y2 >= _y1)
	{
		_yinc1 = 1;
		_yinc2 = 1;
	}
	else
	{
		_yinc1 = -1;
		_yinc2 = -1;
	}

	if(_deltax >= _deltay)
	{
		_xinc1 = 0;
		_yinc2 = 0;
		_den = _deltax;
		_num = _deltax / 2;
		_numadd = _deltay;
		_numpixels = _deltax;
	}
	else
	{
		_xinc2 = 0;
		_yinc1 = 0;
		_den = _deltay;
		_num = _deltay / 2;
		_numadd = _deltax;
		_numpixels = _deltay;
	}

	for(int16_t _curpixel = 0; _curpixel <= _numpixels; _curpixel++)
	{
		SSD1306_Draw_Line(_x, _y, _x3, _y3, _color);
		_num += _numadd;

		if(_num >= _den)
		{
			_num -= _den;
			_x += _xinc1;
			_y += _yinc1;
		}
		_x += _xinc2;
		_y += _yinc2;
	}
}

/**
  ***************************************************************************************************************************************
  * @brief  Draws circle to STM buffer
  * @param  X0 (int16_t), y0 (int16_t), r (uint8_t), color (ssd1306_color_te)
  * @retval None
  ***************************************************************************************************************************************
  */
void SSD1306_Draw_Circle(int16_t _x0, int16_t _y0, uint8_t _r, ssd1306_color_te _color)
{
	int16_t _f = 1 - _r;
	int16_t _ddfx = 1;
	int16_t _ddfy = -2 * _r;
	int16_t _x = 0;
	int16_t _y = _r;

	SSD1306_Draw_Pixel(_x0, _y0 + _r, _color);
	SSD1306_Draw_Pixel(_x0, _y0 - _r, _color);
	SSD1306_Draw_Pixel(_x0 + _r, _y0, _color);
	SSD1306_Draw_Pixel(_x0 - _r, _y0, _color);

	while(_x < _y)
	{
		if(_f >= 0)
		{
			_y--;
			_ddfy += 2;
			_f += _ddfy;
		}

		_x++;
		_ddfx += 2;
		_f += _ddfx;

		SSD1306_Draw_Pixel(_x0 + _x, _y0 + _y, _color);
		SSD1306_Draw_Pixel(_x0 - _x, _y0 + _y, _color);
		SSD1306_Draw_Pixel(_x0 + _x, _y0 - _y, _color);
		SSD1306_Draw_Pixel(_x0 - _x, _y0 - _y, _color);
		SSD1306_Draw_Pixel(_x0 + _y, _y0 + _x, _color);
		SSD1306_Draw_Pixel(_x0 - _y, _y0 + _x, _color);
		SSD1306_Draw_Pixel(_x0 + _y, _y0 - _x, _color);
		SSD1306_Draw_Pixel(_x0 - _y, _y0 - _x, _color);
	}
}

/**
  ***************************************************************************************************************************************
  * @brief  Draws filled circle to STM buffer
  * @param  X0 (int16_t), y0 (int16_t), r (uint8_t), color (ssd1306_color_te)
  * @retval None
  ***************************************************************************************************************************************
  */
void SSD1306_Draw_Filled_Circle(int16_t _x0, int16_t _y0, uint8_t _r, ssd1306_color_te _color)
{
	int16_t _f = 1 - _r;
	int16_t _ddfx = 1;
	int16_t _ddfy = -2 * _r;
	int16_t _x = 0;
	int16_t _y = _r;

	SSD1306_Draw_Pixel(_x0, _y0 + _r, _color);
	SSD1306_Draw_Pixel(_x0, _y0 - _r, _color);
	SSD1306_Draw_Pixel(_x0 + _r, _y0, _color);
	SSD1306_Draw_Pixel(_x0 - _r, _y0, _color);
	SSD1306_Draw_Line(_x0 - _r, _y0, _x0 + _r, _y0, _color);

	while(_x < _y)
	{
		if(_f >= 0)
		{
			_y--;
			_ddfy += 2;
			_f += _ddfy;
		}

		_x++;
		_ddfx += 2;
		_f += _ddfx;

		SSD1306_Draw_Line(_x0 - _x, _y0 + _y, _x0 + _x, _y0 + _y, _color);
		SSD1306_Draw_Line(_x0 + _x, _y0 - _y, _x0 - _x, _y0 - _y, _color);
		SSD1306_Draw_Line(_x0 + _y, _y0 + _x, _x0 - _y, _y0 + _x, _color);
		SSD1306_Draw_Line(_x0 + _y, _y0 - _x, _x0 - _y, _y0 - _x, _color);
	}
}

/**
 ***************************************************************************************************************************************
 * @file     display.c
 * @owner    SimonBat
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

#include "display.h"
#include "string.h"
#include "stdio.h"
#include "ssd1306.h"
#include "ff_profile.h"

static void DISPLAY_Prepare_1_Context(display_ts* _display);
static void DISPLAY_Prepare_2_Context(display_ts* _display);
static void DISPLAY_Prepare_3_Context(display_ts* _display);
static void DISPLAY_Prepare_4_Context(display_ts* _display);
static void DISPLAY_Battery_Status(uint8_t _x, uint8_t _y);
static void DISPLAY_Selection_Mark(uint8_t _x, uint8_t _y);

typedef void (*f_display)(display_ts* _display);
f_display DISPLAY_CONTEXTS[DISPLAY_MAX_CONTEXTS]={
	DISPLAY_Prepare_1_Context,
	DISPLAY_Prepare_2_Context,
	DISPLAY_Prepare_3_Context,
	DISPLAY_Prepare_4_Context
};

/**
  ***************************************************************************************************************************************
  * @brief Prepare display context
  * @param Display handle (display_ts*)
  * @retval None
  ***************************************************************************************************************************************
  */
void DISPLAY_Prepare_Context(display_ts* _display)
{
	if(_display->context>=DISPLAY_MAX_CONTEXTS){return;}

	/* Call prepared display context function */
	if(!_display->updateTmo)
	{
		_display->updateTmo=DISPLAY_UPDATE_TMO;
		DISPLAY_CONTEXTS[_display->context](_display);
	}
}

/**
  ***************************************************************************************************************************************
  * @brief Prepare BMS Data 1 context
  * @param Display handle (display_ts*)
  * @retval None
  ***************************************************************************************************************************************
  */
static void DISPLAY_Prepare_1_Context(display_ts* _display)
{
	uint8_t _x;

	SSD1306_Fill(OLED_COLOR_BLACK);
	_x=(128-(10*7))/2;
	SSD1306_Draw_String(_x, 20, 0, "ENTER PIN:", &TM_Font_7x10, OLED_COLOR_WHITE);
	_x=(128-(_display->passwordNbr*11))/2;

	switch(_display->passwordNbr)
	{
		case(1): SSD1306_Draw_String(_x, 35, 0, "*", &TM_Font_11x18, OLED_COLOR_WHITE); break;
		case(2): SSD1306_Draw_String(_x, 35, 0, "**", &TM_Font_11x18, OLED_COLOR_WHITE); break;
		case(3): SSD1306_Draw_String(_x, 35, 0, "***", &TM_Font_11x18, OLED_COLOR_WHITE); break;
		case(4): SSD1306_Draw_String(_x, 35, 0, "****", &TM_Font_11x18, OLED_COLOR_WHITE); break;
		case(5): SSD1306_Draw_String(_x, 35, 0, "*****", &TM_Font_11x18, OLED_COLOR_WHITE); break;
	}

	SSD1306_Driver_Update();
}

/**
  ***************************************************************************************************************************************
  * @brief Prepare BMS Data 2 context
  * @param Display handle (display_ts*)
  * @retval None
  ***************************************************************************************************************************************
  */
static void DISPLAY_Prepare_2_Context(display_ts* _display)
{
	char _numBuff[16];

	SSD1306_Fill(OLED_COLOR_BLACK);
	DISPLAY_Battery_Status(79,0);
	sprintf(_numBuff,"  0/%d",FF_PROFILE_Get_Data_Number());
	SSD1306_Draw_String(0,0,0,_numBuff, &TM_Font_7x10, OLED_COLOR_WHITE);
	SSD1306_Draw_Line(0,14,127,14,OLED_COLOR_WHITE);
	SSD1306_Draw_String(10, 30,0, "OPEN", &TM_Font_7x10, OLED_COLOR_WHITE);
	SSD1306_Draw_String(10, 40,0, "EDIT", &TM_Font_7x10, OLED_COLOR_WHITE);
	SSD1306_Draw_Line(0,63,127,63,OLED_COLOR_WHITE);
	DISPLAY_Selection_Mark(0,_display->verticalListIdx*10+30);
	SSD1306_Driver_Update();
}

/**
  ***************************************************************************************************************************************
  * @brief Prepare BMS Data 3 context
  * @param Display handle (display_ts*)
  * @retval None
  ***************************************************************************************************************************************
  */
static void DISPLAY_Prepare_3_Context(display_ts* _display)
{
	uint8_t _idx;
	uint8_t* _buff;
	int16_t _xLength;
	int16_t _xOffset;
	char _numBuff[16];
	profile_data_ts* _data=FF_PROFILE_Get_Data(_display->horizontalListIdx);

	SSD1306_Fill(OLED_COLOR_BLACK);
	DISPLAY_Battery_Status(79,0);

	if(_display->btFlag){SSD1306_Draw_String(58,0,0,">>", &TM_Font_7x10, OLED_COLOR_WHITE);}
	else{SSD1306_Draw_String(58,0,0,"  ", &TM_Font_7x10, OLED_COLOR_WHITE);}

	if((_display->horizontalListIdx+1)<10){sprintf(_numBuff,"  %d/%d",(_display->horizontalListIdx+1),FF_PROFILE_Get_Data_Number());}
	else if(((_display->horizontalListIdx+1)>=10)&&((_display->horizontalListIdx+1)<100))
	{sprintf(_numBuff," %d/%d",(_display->horizontalListIdx+1),FF_PROFILE_Get_Data_Number());}
	else if((_display->horizontalListIdx+1)>=100)
	{sprintf(_numBuff,"%d/%d",(_display->horizontalListIdx+1),FF_PROFILE_Get_Data_Number());}

	SSD1306_Draw_String(0,0,0,_numBuff, &TM_Font_7x10, OLED_COLOR_WHITE);
	SSD1306_Draw_Line(0,14,127,14,OLED_COLOR_WHITE);

	if(_display->verticalListIdx==0){SSD1306_Draw_String(_display->xScroll, 20,10,(char*)_data->url, &TM_Font_7x10, OLED_COLOR_WHITE);}
	else{SSD1306_Draw_String(10, 20,10,"NAME", &TM_Font_7x10, OLED_COLOR_WHITE);}

	for(_idx=0;_idx<_data->dataNbr;_idx++)
	{
		if(_display->verticalListIdx==(_idx+1)){SSD1306_Draw_String(_display->xScroll,(_idx*10+30),10,(char*)_data->dataBuffer[_idx], &TM_Font_7x10, OLED_COLOR_WHITE);}
		else
		{
			if(_data->dataNameCode[_idx]==0){SSD1306_Draw_String(10,(_idx*10+30),10,"EMAIL", &TM_Font_7x10, OLED_COLOR_WHITE);}
			else if(_data->dataNameCode[_idx]==1){SSD1306_Draw_String(10,(_idx*10+30),10,"USER", &TM_Font_7x10, OLED_COLOR_WHITE);}
			else if(_data->dataNameCode[_idx]==2){SSD1306_Draw_String(10,(_idx*10+30),10,"PASSWORD", &TM_Font_7x10, OLED_COLOR_WHITE);}
		}
	}

	SSD1306_Draw_Line(0,63,127,63,OLED_COLOR_WHITE);
	DISPLAY_Selection_Mark(0,_display->verticalListIdx*10+20);
	SSD1306_Driver_Update();

	_idx=0;
	if(_display->verticalListIdx==0){_buff=_data->url;}
	else{_buff=_data->dataBuffer[_display->verticalListIdx-1];}

	while(*_buff)
	{
		_idx++;
		_buff++;
	}
	_xLength=_idx*7+3;

	if(_xLength>118)
	{
		_xOffset=118-_xLength+10;
		if(_xOffset>0){_xOffset=0;}

		if((_display->xScroll>_xOffset)&&(!_display->xDirection)){_display->xScroll--;}
		else if((_display->xScroll<13)&&(_display->xDirection)){_display->xScroll++;}

		if(_display->xScroll<=_xOffset){_display->xDirection=1;}
		else if(_display->xScroll>=13){_display->xDirection=0;}
	}
	else{_display->xScroll=10;}
}

/**
  ***************************************************************************************************************************************
  * @brief Prepare BMS Data 4 context
  * @param Display handle (display_ts*)
  * @retval None
  ***************************************************************************************************************************************
  */
static void DISPLAY_Prepare_4_Context(display_ts* _display)
{
	char _numBuff[16];

	SSD1306_Fill(OLED_COLOR_BLACK);
	DISPLAY_Battery_Status(79,0);
	sprintf(_numBuff,"  0/%d",FF_PROFILE_Get_Data_Number());
	SSD1306_Draw_String(0,0,0,_numBuff, &TM_Font_7x10, OLED_COLOR_WHITE);
	SSD1306_Draw_Line(0,14,127,14,OLED_COLOR_WHITE);
	SSD1306_Draw_String(14, 20,0, "PLUG USB CABLE", &TM_Font_7x10, OLED_COLOR_WHITE);
	SSD1306_Draw_String(38, 30,0, "TO EDIT", &TM_Font_7x10, OLED_COLOR_WHITE);
	SSD1306_Draw_String(3, 50,0, "OFF/ON TO RESTART", &TM_Font_7x10, OLED_COLOR_WHITE);
	SSD1306_Draw_Line(0,63,127,63,OLED_COLOR_WHITE);
	SSD1306_Driver_Update();
}

/**
  ***************************************************************************************************************************************
  * @brief  Display battery status
  * @param  Coordinates x(uint8_t), y(uint8_t)
  * @retval None
  ***************************************************************************************************************************************
  */
static void DISPLAY_Battery_Status(uint8_t _x, uint8_t _y)
{
	char _numBuff[5];
	uint8_t _xTmp;
	uint8_t _soc=BAT_Get_SOC();
	uint8_t _idx;
	uint8_t _idxMax=18*_soc/100;

	if(_soc<10){_xTmp=_x+14;}
	else if((_soc>=10)&&(_soc<100)){_xTmp=_x+7;}
	else{_xTmp=_x;}

	sprintf(&_numBuff[0],"%d%%",_soc);
	SSD1306_Draw_String(_xTmp, _y,0, &_numBuff[0], &TM_Font_7x10, OLED_COLOR_WHITE);
	SSD1306_Draw_Line(_x+30,_y,_x+30,_y+8,OLED_COLOR_WHITE);
	SSD1306_Draw_Line(_x+46,_y,_x+46,_y+1,OLED_COLOR_WHITE);
	SSD1306_Draw_Line(_x+46,_y+7,_x+46,_y+8,OLED_COLOR_WHITE);
	SSD1306_Draw_Line(_x+48,_y+2,_x+48,_y+6,OLED_COLOR_WHITE);
	SSD1306_Draw_Line(_x+30,_y,_x+46,_y,OLED_COLOR_WHITE);
	SSD1306_Draw_Line(_x+30,_y+8,_x+46,_y+8,OLED_COLOR_WHITE);
	SSD1306_Draw_Line(_x+46,_y+2,_x+48,_y+2,OLED_COLOR_WHITE);
	SSD1306_Draw_Line(_x+46,_y+6,_x+48,_y+6,OLED_COLOR_WHITE);

	for(_idx=1;_idx<=_idxMax;_idx++)
	{
		if(_x+30+_idx<_x+46){SSD1306_Draw_Line(_x+30+_idx,_y,_x+30+_idx,_y+8,OLED_COLOR_WHITE);}
		else{SSD1306_Draw_Line(_x+30+_idx,_y+2,_x+30+_idx,_y+6,OLED_COLOR_WHITE);}
	}
}

/**
  ***************************************************************************************************************************************
  * @brief  Display selection mark
  * @param  Coordinates x(uint8_t), y(uint8_t)
  * @retval None
  ***************************************************************************************************************************************
  */
static void DISPLAY_Selection_Mark(uint8_t _x, uint8_t _y)
{
	SSD1306_Draw_Filled_Triangle(_x,_y,_x,_y+6,_x+7,_y+3,OLED_COLOR_WHITE);
}

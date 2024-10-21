/**
  ***************************************************************************************************************************************
  * @file     system.c
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

#include "system.h"
#include "bsp.h"
#include "tsl_msp.h"
#include "usb_device.h"
#include "i2c.h"
#include "ssd1306.h"
#include "bt_hogp.h"
#include "led.h"
#include "bat.h"
#include "ff_profile.h"
#include "display.h"

static system_ts SYSTEM;
static const uint8_t SYSTEM_PASSWORD[SYSTEM_PASSWORD_NBR]={SYSTEM_BUTTON_UP,SYSTEM_BUTTON_LEFT,SYSTEM_BUTTON_RIGHT,SYSTEM_BUTTON_DOWN,SYSTEM_BUTTON_OK};

static void SYSTEM_Start_Scheduler(system_ts* _system);
static int SYSTEM_SWO_Write(int Length, char *Buffer);
static void SYSTEM_Scan_Buttons(system_ts* _system);

/**
  ***************************************************************************************************************************************
  * @brief  The application entry point
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
int main(void)
{
	uint8_t _idx;
	uint8_t _addPinFlag=0;
	uint8_t _ledHandlerParam=0;

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();
	/* Configure the system clock */
	BSP_System_Clock_Config();
	/* Initialize all configured peripherals */
	BSP_System_GPIO_Init();
	LED_Init();
	I2C_Driver_Init();
	TSL_Driver_Init();
	FF_PROFILE_Init();
	LED_On();
	HAL_Delay(20);
	SSD1306_Driver_Init();
	BAT_Init();
	HAL_Delay(80);
	LED_Off();

	SYSTEM.offTmo=SYSTEM_OFF_TMO;
	SYSTEM.passwordTmo=250;
	while((SYSTEM.passwordNbr<=SYSTEM_PASSWORD_NBR)&&(SYSTEM.passwordTmo))
	{
		SYSTEM_Scan_Buttons(&SYSTEM);

		if(SYSTEM.passwordNbr<SYSTEM_PASSWORD_NBR){SYSTEM.passwordTmo=250;}

		if((!SYSTEM.button[SYSTEM_BUTTON_UP].statusFlag)&&(!SYSTEM.button[SYSTEM_BUTTON_OK].statusFlag)&&(!SYSTEM.button[SYSTEM_BUTTON_DOWN].statusFlag)&& \
		   (!SYSTEM.button[SYSTEM_BUTTON_LEFT].statusFlag)&&(!SYSTEM.button[SYSTEM_BUTTON_RIGHT].statusFlag))
		{
			_addPinFlag=1;
		}
		else if(((SYSTEM.button[SYSTEM_BUTTON_UP].statusFlag)||(SYSTEM.button[SYSTEM_BUTTON_OK].statusFlag)||(SYSTEM.button[SYSTEM_BUTTON_DOWN].statusFlag)|| \
				 (SYSTEM.button[SYSTEM_BUTTON_LEFT].statusFlag)||(SYSTEM.button[SYSTEM_BUTTON_RIGHT].statusFlag))&&(_addPinFlag))
		{
			_addPinFlag=0;
			SYSTEM.offTmo=SYSTEM_OFF_TMO;

			for(_idx=0;_idx<SYSTEM_PASSWORD_NBR;_idx++)
			{
				if(SYSTEM.button[_idx].statusFlag)
				{
					if(SYSTEM.passwordNbr<SYSTEM_PASSWORD_NBR)
					{
						SYSTEM.password[SYSTEM.passwordNbr]=_idx;
						SYSTEM.passwordNbr++;
					}
					SYSTEM.display.passwordNbr=SYSTEM.passwordNbr;
					break;
				}
			}
		}

		BAT_Handler();
		DISPLAY_Prepare_Context(&SYSTEM.display);

		if(!SYSTEM.ledHandlerTmo)
		{
			SYSTEM.ledHandlerTmo=LED_HANDLER_TMO;
			LED_Handler(&_ledHandlerParam);
		}

		if(!SYSTEM.offTmo){BSP_System_off();}
	}

	SYSTEM.offTmo=SYSTEM_OFF_TMO;

	/* Check password */
	for(_idx=0;_idx<SYSTEM_PASSWORD_NBR;_idx++)
	{
		if(SYSTEM.password[_idx]!=SYSTEM_PASSWORD[_idx])
		{
			FF_PROFILE_Check_Error_Log(0);
			/* Password incorrect - system power off */
			BSP_System_off();
		}
	}

	FF_PROFILE_Check_Error_Log(1);
	SYSTEM.display.context=1;

	while(!SYSTEM.button[SYSTEM_BUTTON_OK].statusFlag)
	{
		SYSTEM_Scan_Buttons(&SYSTEM);

		if((SYSTEM.button[SYSTEM_BUTTON_UP].statusFlag)&&(SYSTEM.display.verticalListIdx>0)&&(!SYSTEM.horizontalListIdxTmo))
		{
			SYSTEM.offTmo=SYSTEM_OFF_TMO;
			SYSTEM.horizontalListIdxTmo=SYSTEM_KEY_TMO;
			SYSTEM.display.verticalListIdx--;
		}
		else if((SYSTEM.button[SYSTEM_BUTTON_DOWN].statusFlag)&&(SYSTEM.display.verticalListIdx<1)&&(!SYSTEM.horizontalListIdxTmo))
		{
			SYSTEM.offTmo=SYSTEM_OFF_TMO;
			SYSTEM.horizontalListIdxTmo=SYSTEM_KEY_TMO;
			SYSTEM.display.verticalListIdx++;
		}

		if((!SYSTEM.button[SYSTEM_BUTTON_UP].statusFlag)&&(!SYSTEM.button[SYSTEM_BUTTON_DOWN].statusFlag)){SYSTEM.horizontalListIdxTmo=0;}

		BAT_Handler();
		DISPLAY_Prepare_Context(&SYSTEM.display);

		if(!SYSTEM.ledHandlerTmo)
		{
			SYSTEM.ledHandlerTmo=LED_HANDLER_TMO;
			LED_Handler(&_ledHandlerParam);
		}

		if(!SYSTEM.offTmo){BSP_System_off();}
	}

	SYSTEM.offTmo=SYSTEM_OFF_TMO;

	if(SYSTEM.display.verticalListIdx==1)
	{
		/* Edit mode */
		SYSTEM.display.verticalListIdx=0;
		SYSTEM.display.context=3;
		USB_Device_Init();

		while(1)
		{
			SYSTEM_Scan_Buttons(&SYSTEM);
			BAT_Handler();
			DISPLAY_Prepare_Context(&SYSTEM.display);
			SYSTEM.offTmo=SYSTEM_OFF_TMO;

			if(!SYSTEM.ledHandlerTmo)
			{
				SYSTEM.ledHandlerTmo=LED_HANDLER_TMO;
				LED_Handler(&_ledHandlerParam);
			}
		}
	}
	else
	{
		/* Open mode */
		SYSTEM.offTmo=SYSTEM_OFF_TMO;
		SYSTEM.display.verticalListIdx=0;
		SYSTEM.display.horizontalListIdx=0;
		SYSTEM.display.context=2;
		SYSTEM.display.xScroll=10;
		SYSTEM.display.xDirection=0;

		SYSTEM_Start_Scheduler(&SYSTEM);
	}
}

/**
  ***************************************************************************************************************************************
  * @brief System start scheduler
  * @param None
  * @retval None
  ***************************************************************************************************************************************
  */
static void SYSTEM_Start_Scheduler(system_ts* _system)
{
	profile_data_ts* _profileData;
	BTPS_Initialization_t BTPS_Initialization;
	HCI_DriverInformation_t HCI_DriverInformation;

	/* Configure the UART Parameters */
	_system->offTmo=SYSTEM_OFF_TMO;
	HCI_DRIVER_SET_COMM_INFORMATION(&HCI_DriverInformation, 1, 921600L, cpUART_RTS_CTS);
	HCI_DriverInformation.DriverInformation.COMMDriverInformation.InitializationDelay = 10;
	/* Set up the application callbacks */
	BTPS_Initialization.MessageOutputCallback = SYSTEM_SWO_Write;

	/* Initialize the application */
	if(HOGP_Application_Init(&HCI_DriverInformation, &BTPS_Initialization))
	{
		if(BTPS_AddFunctionToScheduler(LED_Handler, NULL, LED_HANDLER_TMO))
		{
			if(HOGP_Check_Mailbox_Status())
			{
				while(1)
				{
					HOGP_Task_Handler();
					SYSTEM_Scan_Buttons(_system);

					/* Horizontal list control */
					if((_system->button[SYSTEM_BUTTON_LEFT].statusFlag)&&(_system->display.horizontalListIdx>0)&&(!_system->horizontalListIdxTmo))
					{
						_system->horizontalListIdxTmo=SYSTEM_KEY_TMO;
						_system->display.horizontalListIdx--;
						_system->display.verticalListIdx=0;
						_system->display.xScroll=10;
						_system->display.xDirection=0;
						_system->offTmo=SYSTEM_OFF_TMO;
					}
					else if((_system->button[SYSTEM_BUTTON_LEFT].statusFlag)&&(_system->display.horizontalListIdx==0)&&(!_system->horizontalListIdxTmo))
					{
						_system->horizontalListIdxTmo=SYSTEM_KEY_TMO;
						_system->display.horizontalListIdx=(FF_PROFILE_Get_Data_Number()-1);
						_system->display.verticalListIdx=0;
						_system->display.xScroll=10;
						_system->display.xDirection=0;
						_system->offTmo=SYSTEM_OFF_TMO;
					}
					else if((_system->button[SYSTEM_BUTTON_RIGHT].statusFlag)&&(_system->display.horizontalListIdx<(FF_PROFILE_Get_Data_Number()-1))&& \
							(!_system->horizontalListIdxTmo))
					{
						_system->horizontalListIdxTmo=SYSTEM_KEY_TMO;
						_system->display.horizontalListIdx++;
						_system->display.verticalListIdx=0;
						_system->display.xScroll=10;
						_system->display.xDirection=0;
						_system->offTmo=SYSTEM_OFF_TMO;
					}
					else if((_system->button[SYSTEM_BUTTON_RIGHT].statusFlag)&&(_system->display.horizontalListIdx==(FF_PROFILE_Get_Data_Number()-1))&& \
							(!_system->horizontalListIdxTmo))
					{
						_system->horizontalListIdxTmo=SYSTEM_KEY_TMO;
						_system->display.horizontalListIdx=0;
						_system->display.verticalListIdx=0;
						_system->display.xScroll=10;
						_system->display.xDirection=0;
						_system->offTmo=SYSTEM_OFF_TMO;
					}

					_profileData=FF_PROFILE_Get_Data(_system->display.horizontalListIdx);

					/* Vertical list control */
					if((_system->button[SYSTEM_BUTTON_UP].statusFlag)&&(_system->display.verticalListIdx>0)&&(!_system->verticalListIdxTmo))
					{
						_system->verticalListIdxTmo=SYSTEM_KEY_TMO;
						_system->display.verticalListIdx--;
						_system->display.xScroll=10;
						_system->display.xDirection=0;
						_system->offTmo=SYSTEM_OFF_TMO;
					}
					else if((_system->button[SYSTEM_BUTTON_UP].statusFlag)&&(_system->display.verticalListIdx==0)&&(!_system->verticalListIdxTmo))
					{
						_system->verticalListIdxTmo=SYSTEM_KEY_TMO;
						_system->display.verticalListIdx=(_profileData->dataNbr);
						_system->display.xScroll=10;
						_system->display.xDirection=0;
						_system->offTmo=SYSTEM_OFF_TMO;
					}
					else if((_system->button[SYSTEM_BUTTON_DOWN].statusFlag)&&(_system->display.verticalListIdx<(_profileData->dataNbr))&& \
							(!_system->verticalListIdxTmo))
					{
						_system->verticalListIdxTmo=SYSTEM_KEY_TMO;
						_system->display.verticalListIdx++;
						_system->display.xScroll=10;
						_system->display.xDirection=0;
						_system->offTmo=SYSTEM_OFF_TMO;
					}
					else if((_system->button[SYSTEM_BUTTON_DOWN].statusFlag)&&(_system->display.verticalListIdx==(_profileData->dataNbr))&& \
							(!_system->verticalListIdxTmo))
					{
						_system->verticalListIdxTmo=SYSTEM_KEY_TMO;
						_system->display.verticalListIdx=0;
						_system->display.xScroll=10;
						_system->display.xDirection=0;
						_system->offTmo=SYSTEM_OFF_TMO;
					}
					else if((_system->button[SYSTEM_BUTTON_OK].statusFlag)&&(!_system->dataTxTmo))
					{
						_system->dataTxTmo=SYSTEM_KEY_TMO;
						_system->offTmo=SYSTEM_OFF_TMO;
						if(_system->display.verticalListIdx==0){HOGP_Send_Data_Reports(_profileData->url,_profileData->urlSize);}
						else{HOGP_Send_Data_Reports(_profileData->dataBuffer[_system->display.verticalListIdx-1],_profileData->dataSize[_system->display.verticalListIdx-1]);}
					}

					if((!_system->button[SYSTEM_BUTTON_LEFT].statusFlag)&&(!_system->button[SYSTEM_BUTTON_RIGHT].statusFlag)){_system->horizontalListIdxTmo=0;}
					if((!_system->button[SYSTEM_BUTTON_UP].statusFlag)&&(!_system->button[SYSTEM_BUTTON_DOWN].statusFlag)){_system->verticalListIdxTmo=0;}

					BAT_Handler();
					_system->display.btFlag=HOGP_Get_Connection_Status();
					DISPLAY_Prepare_Context(&_system->display);

					if(!_system->batteryLevelTmo)
					{
						_system->batteryLevelTmo=1000;
						HOGP_Update_Battery_Level();
					}

					if(!_system->offTmo){BSP_System_off();}
				}
			}
			else{BSP_Error_Handler();}
		}
		else{BSP_Error_Handler();}
	}
	else{BSP_Error_Handler();}
}

/**
  ***************************************************************************************************************************************
  * @brief SWO write function
  * @param Data length (uint32_t), buffer (char*)
  * @retval Status (int32_t)
  ***************************************************************************************************************************************
  */
static int SYSTEM_SWO_Write(int _length, char *_buffer)
{
	uint32_t _len;

	for(_len=0;_len<_length;_len++){ITM_SendChar(*_buffer++);}

	return 1;
}

/**
  ***************************************************************************************************************************************
  * @brief SWO write function
  * @param Data length (uint32_t), buffer (char*)
  * @retval Status (int32_t)
  ***************************************************************************************************************************************
  */
void SYSTEM_Update_TMO(void)
{
	uint8_t _idx;

	if(SYSTEM.display.updateTmo){SYSTEM.display.updateTmo--;}
	if(SYSTEM.passwordTmo){SYSTEM.passwordTmo--;}
	if(SYSTEM.ledHandlerTmo){SYSTEM.ledHandlerTmo--;}
	if(SYSTEM.horizontalListIdxTmo){SYSTEM.horizontalListIdxTmo--;}
	if(SYSTEM.verticalListIdxTmo){SYSTEM.verticalListIdxTmo--;}
	if(SYSTEM.dataTxTmo){SYSTEM.dataTxTmo--;}
	if(SYSTEM.batteryLevelTmo){SYSTEM.batteryLevelTmo--;}
	if(SYSTEM.offTmo){SYSTEM.offTmo--;}

	for(_idx=0;_idx<SYSTEM_BUTTONS;_idx++)
	{
		if(SYSTEM.button[_idx].onTmo){SYSTEM.button[_idx].onTmo--;}
		if(SYSTEM.button[_idx].offTmo){SYSTEM.button[_idx].offTmo--;}
	}
}

/**
  ***************************************************************************************************************************************
  * @brief Scan system buttons
  * @param System handle (system_ts*)
  * @retval None
  ***************************************************************************************************************************************
  */
static void SYSTEM_Scan_Buttons(system_ts* _system)
{
	uint8_t _idx;
	tsl_user_status_t tsl_status;

	/* Execute STMTouch Driver state machine */
	tsl_status=TSL_User_Handler();

	if(tsl_status!=TSL_USER_STATUS_BUSY)
	{
		for(_idx=0;_idx<SYSTEM_BUTTONS;_idx++)
		{
			if((TSL_TKEYS_B[_idx].p_Data->StateId==TSL_STATEID_DETECT)&&(!_system->button[_idx].onTmo))
			{
				_system->button[_idx].offTmo=SYSTEM_BUTTON_OFF_TMO;
				_system->button[_idx].statusFlag=1;
				LED_Pulse();
			}
			else if((TSL_TKEYS_B[_idx].p_Data->StateId==TSL_STATEID_RELEASE)&&(!_system->button[_idx].offTmo))
			{
				_system->button[_idx].onTmo=SYSTEM_BUTTON_ON_TMO;
				_system->button[_idx].statusFlag=0;
			}
		}
	}
}

/**
  ***************************************************************************************************************************************
  * @file     ff_profile.c
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

#include "ff_profile.h"
#include "string.h"
#include "bsp.h"

/* Global variables */
static profile_ts PROFILE;

/**
  ***************************************************************************************************************************************
  * @brief FAT file system profile initialization
  * @param None
  * @retval None
  ***************************************************************************************************************************************
  */
void FF_PROFILE_Init(void)
{
	FRESULT _res;
	uint8_t _ffBuffer[128];
	uint16_t _urlNbrIdx;
	uint8_t _urlDataNbrIdx;
	__IO uint8_t _idx = 0U;
	__IO uint32_t _bytesRead;

	if(0U == FATFS_LinkDriver(&FF_Driver,PROFILE.ffPath))
	{
		if(FR_OK == f_mount(&PROFILE.ffFs, PROFILE.ffPath, 0U))
		{
			if(FR_OK == f_open(&PROFILE.dataFile, FF_PROFILE_DATA_FNAME, FA_READ))
			{
				/* Find the first '<' symbol */
				do{
					_res = f_read(&PROFILE.dataFile, &_ffBuffer[_idx], 1U, (UINT*)&_bytesRead);
					if((0U == _bytesRead)||(FR_OK != _res)){BSP_Error_Handler();}
					else{_idx++;}
				}while('<' != _ffBuffer[_idx - 1U]);

				/* Find the '>' symbol */
				_idx = 0U;
				do{
					_res = f_read(&PROFILE.dataFile, &_ffBuffer[_idx], 1U, (UINT*)&_bytesRead);
					if((0U == _bytesRead) || (FR_OK != _res)){BSP_Error_Handler();}
					else{_idx++;}
				}while('>' != _ffBuffer[_idx - 1U]);

				if(2U == _idx){PROFILE.dataNbr = _ffBuffer[0] - 48U;}
				else if(3U == _idx){PROFILE.dataNbr = (_ffBuffer[0] - 48U) * 10U + (_ffBuffer[1] - 48U);}
				else if(4U == _idx){PROFILE.dataNbr = (_ffBuffer[0] - 48U) * 100U + ((_ffBuffer[1] - 48U) * 10U) + (_ffBuffer[1] - 48U);}
				else{BSP_Error_Handler();}

				for(_urlNbrIdx = 0U; _urlNbrIdx < PROFILE.dataNbr; _urlNbrIdx++)
				{
					/* Find the '<' symbol */
					do{
						_res = f_read(&PROFILE.dataFile, &_ffBuffer[_idx], 1U, (UINT*)&_bytesRead);
						if((0U == _bytesRead) || (FR_OK != _res)){BSP_Error_Handler();}
						else{_idx++;}
					}while(('<' != _ffBuffer[_idx - 1U]));

					/* Filter the "url:" mark */
					_res = f_read(&PROFILE.dataFile, &_ffBuffer[0], 4U, (UINT*)&_bytesRead);

					if(('u' != _ffBuffer[0]) || ('r' != _ffBuffer[1]) || ('l' != _ffBuffer[2]) || \
					   (':' != _ffBuffer[3]) || (FR_OK != _res) || (4U != _bytesRead)){BSP_Error_Handler();}

					/* Find the '>' symbol */
					_idx = 0U;
					do{
						_res = f_read(&PROFILE.dataFile, &_ffBuffer[_idx], 1U, (UINT*)&_bytesRead);
						if((0U == _bytesRead) || (FR_OK != _res)){BSP_Error_Handler();}
						else{_idx++;}
					}while('>' != _ffBuffer[_idx - 1U]);

					PROFILE.data[_urlNbrIdx].urlSize = _idx - 1U;
					memcpy(&PROFILE.data[_urlNbrIdx].url[0], &_ffBuffer[0], PROFILE.data[_urlNbrIdx].urlSize);

					/* Find the '<' symbol */
					_idx = 0U;
					do{
						_res = f_read(&PROFILE.dataFile, &_ffBuffer[_idx], 1U, (UINT*)&_bytesRead);
						if((0U == _bytesRead) || (FR_OK != _res)){BSP_Error_Handler();}
						else{_idx++;}
					}while('<' != _ffBuffer[_idx - 1U]);

					/* Find the '>' symbol */
					_idx = 0U;
					do{
						_res = f_read(&PROFILE.dataFile, &_ffBuffer[_idx], 1U, (UINT*)&_bytesRead);
						if((0U == _bytesRead) || (FR_OK != _res)){BSP_Error_Handler();}
						else{_idx++;}
					}while('>' != _ffBuffer[_idx - 1U]);

					PROFILE.data[_urlNbrIdx].dataNbr = _ffBuffer[0] - 48U;

					for(_urlDataNbrIdx = 0U; _urlDataNbrIdx < PROFILE.data[_urlNbrIdx].dataNbr; _urlDataNbrIdx++)
					{
						/* Find the '<' symbol */
						_idx = 0U;
						do{
							_res = f_read(&PROFILE.dataFile, &_ffBuffer[_idx], 1U, (UINT*)&_bytesRead);
							if((0U == _bytesRead) || (FR_OK != _res)){BSP_Error_Handler();}
							else{_idx++;}
						}while('<' != _ffBuffer[_idx - 1U]);

						_idx = 0U;
						do{
							_res = f_read(&PROFILE.dataFile, &_ffBuffer[_idx], 1U, (UINT*)&_bytesRead);
							if((0U == _bytesRead) || (FR_OK != _res)){BSP_Error_Handler();}
							else{_idx++;}
						}while(':' != _ffBuffer[_idx - 1U]);

						if((6U == _idx) && ('e' == _ffBuffer[0]) && ('m' == _ffBuffer[1]) && \
						   ('a' == _ffBuffer[2]) && ('i' == _ffBuffer[3]) && ('l' == _ffBuffer[4]) && (':' == _ffBuffer[5]))
						{PROFILE.data[_urlNbrIdx].dataNameCode[_urlDataNbrIdx] = 0U;}
						else if((5U == _idx) && ('u' == _ffBuffer[0]) && ('s' == _ffBuffer[1]) && \
								('e' == _ffBuffer[2]) && ('r' == _ffBuffer[3]) && (':' == _ffBuffer[4]))
						{PROFILE.data[_urlNbrIdx].dataNameCode[_urlDataNbrIdx] = 1U;}
						else if((9U == _idx) && ('p' == _ffBuffer[0]) && ('a' == _ffBuffer[1]) && \
								('s' == _ffBuffer[2]) && ('s' == _ffBuffer[3]) && ('w' == _ffBuffer[4]) && \
								('o' == _ffBuffer[5]) && ('r' == _ffBuffer[6]) && ('d' == _ffBuffer[7]) && (':' == _ffBuffer[8]))
						{PROFILE.data[_urlNbrIdx].dataNameCode[_urlDataNbrIdx] = 2U;}
						else{BSP_Error_Handler();}

						_idx = 0U;
						do{
							_res = f_read(&PROFILE.dataFile, &_ffBuffer[_idx], 1U, (UINT*)&_bytesRead);
							if((0U == _bytesRead) || (FR_OK != _res)){BSP_Error_Handler();}
							else{_idx++;}
						}while('>' != _ffBuffer[_idx - 1U]);

						PROFILE.data[_urlNbrIdx].dataSize[_urlDataNbrIdx] = _idx - 1U;
						memcpy(&PROFILE.data[_urlNbrIdx].dataBuffer[_urlDataNbrIdx][0], &_ffBuffer[0], PROFILE.data[_urlNbrIdx].dataSize[_urlDataNbrIdx]);
					}
				}

				f_close(&PROFILE.dataFile);
			}else{BSP_Error_Handler();}
		}else{BSP_Error_Handler();}
	}else{BSP_Error_Handler();}
}

/**
  ***************************************************************************************************************************************
  * @brief FAT file system profile initialization
  * @param Status (uint8_t)
  * @retval None
  ***************************************************************************************************************************************
  */
void FF_PROFILE_Check_Error_Log(uint8_t _status)
{
	FRESULT _res;
	uint8_t _ffBuffer[128];
	__IO uint8_t _idx = 0U;
	__IO uint32_t _bytesRead;
	uint8_t _errorNbr;

	if(FR_OK == f_open(&PROFILE.errLogFile, FF_PROFILE_ERROR_LOG_FNAME, (FA_READ | FA_WRITE)))
	{
		/* Find the first '<' symbol */
		do{
			_res = f_read(&PROFILE.errLogFile, &_ffBuffer[_idx], 1U, (UINT*)&_bytesRead);
			if((0U == _bytesRead) || (FR_OK != _res)){BSP_Error_Handler();}
			else{_idx++;}
		}while('<' != _ffBuffer[_idx - 1U]);

		/* Find the '>' symbol */
		_idx = 0U;
		do{
			_res = f_read(&PROFILE.errLogFile, &_ffBuffer[_idx], 1U, (UINT*)&_bytesRead);
			if((0U == _bytesRead) || (FR_OK != _res)){BSP_Error_Handler();}
			else{_idx++;}
		}while('>' != _ffBuffer[_idx - 1U]);

		_errorNbr = _ffBuffer[0] - 48U;
		f_close(&PROFILE.errLogFile);

		if(FR_OK == f_open(&PROFILE.errLogFile, FF_PROFILE_ERROR_LOG_FNAME, (FA_READ | FA_WRITE)))
		{
			if((0U == _status) || ((0U != _status) && (_errorNbr >= 3U)))
			{
				if((_errorNbr + 1U) >= 3U)
				{
					f_write(&PROFILE.errLogFile,"<3>", 3U, (void*)&_bytesRead);
					f_close(&PROFILE.errLogFile);
					BSP_Error_Handler();
				}
				else
				{
					_ffBuffer[0] = ((_errorNbr + 1U) + 48U);
					f_write(&PROFILE.errLogFile, "<", 1U, (void*)&_bytesRead);
					f_write(&PROFILE.errLogFile, &_ffBuffer[0], 1U, (void*)&_bytesRead);
					f_write(&PROFILE.errLogFile, ">", 1U, (void*)&_bytesRead);
				}
			}else{f_write(&PROFILE.errLogFile, "<0>", 3U, (void*)&_bytesRead);}

			f_close(&PROFILE.errLogFile);
		}else{BSP_Error_Handler();}
	}else{BSP_Error_Handler();}
}

/**
  ***************************************************************************************************************************************
  * @brief FF profile get data number
  * @param None
  * @retval Data number (uint8_t)
  ***************************************************************************************************************************************
  */
uint8_t FF_PROFILE_Get_Data_Number(void)
{
	return PROFILE.dataNbr;
}

/**
  ***************************************************************************************************************************************
  * @brief FF profile get data
  * @param Data index (uint16_t)
  * @retval Data (profile_data_ts*)
  ***************************************************************************************************************************************
  */
profile_data_ts* FF_PROFILE_Get_Data(uint16_t _dataIdx)
{
	return (profile_data_ts*)&PROFILE.data[_dataIdx];
}

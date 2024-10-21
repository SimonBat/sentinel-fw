#ifndef __FF_PROFILE_H
#define __FF_PROFILE_H

#include "ff.h"
#include "ff_gen_drv.h"
#include "ff_diskio.h"

#define FF_PROFILE_DATA_FNAME			_T("data.txt")
#define FF_PROFILE_ERROR_LOG_FNAME		_T("error.txt")

typedef struct {
	uint8_t url[128];
	uint8_t urlSize;
	uint8_t dataNbr;
	uint8_t dataNameCode[3];
	uint8_t dataSize[3];
	uint8_t dataBuffer[3][64];
} profile_data_ts;

typedef struct {
	char ffPath[4];
	FATFS ffFs;
	FIL dataFile;
	FIL errLogFile;
	uint16_t dataNbr;
	profile_data_ts data[200];
} profile_ts;

/* Global functions definitions */
void FF_PROFILE_Init(void);
void FF_PROFILE_Check_Error_Log(uint8_t _status);
uint8_t FF_PROFILE_Get_Data_Number(void);
profile_data_ts* FF_PROFILE_Get_Data(uint16_t _dataIdx);

#endif

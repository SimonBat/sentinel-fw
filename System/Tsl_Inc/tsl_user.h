#ifndef __TSL_USER_H
#define __TSL_USER_H

#include "tsl.h"
#include "stm32l4xx_hal.h"

/* Select if you use STMStudio software (0=No, 1=Yes) */
#define USE_STMSTUDIO (0)

#if USE_STMSTUDIO > 0
#include "tsl_cs.h"
#define STMSTUDIO_LOCK {TSL_Enter_Lock();}
#define STMSTUDIO_UNLOCK {TSL_Exit_Lock();}
#else
#define STMSTUDIO_LOCK
#define STMSTUDIO_UNLOCK
#endif

typedef enum {
	TSL_USER_STATUS_BUSY, /* The bank acquisition is on-going */
	TSL_USER_STATUS_OK_NO_ECS, /* The bank acquisition is ok, no time for ECS */
	TSL_USER_STATUS_OK_ECS_ON, /* The bank acquisition is ok, ECS finished */
	TSL_USER_STATUS_OK_ECS_OFF  /* The bank acquisition is ok, ECS not executed */
} tsl_user_status_t;

/* Channel IOs definition */
#define CHANNEL_0_IO_MSK    	(TSC_GROUP1_IO2)
#define CHANNEL_0_GRP_MSK   	(TSC_GROUP1)
#define CHANNEL_0_SRC       	(TSC_GROUP1_IDX) /* Index in source register (TSC->IOGXCR[]) */
#define CHANNEL_0_DEST      	(0) /* Index in destination result array */

#define CHANNEL_1_IO_MSK    	(TSC_GROUP1_IO3)
#define CHANNEL_1_GRP_MSK   	(TSC_GROUP1)
#define CHANNEL_1_SRC       	(TSC_GROUP1_IDX)
#define CHANNEL_1_DEST      	(1)

#define CHANNEL_2_IO_MSK    	(TSC_GROUP1_IO4)
#define CHANNEL_2_GRP_MSK   	(TSC_GROUP1)
#define CHANNEL_2_SRC       	(TSC_GROUP1_IDX)
#define CHANNEL_2_DEST      	(2)

#define CHANNEL_3_IO_MSK    	(TSC_GROUP2_IO2)
#define CHANNEL_3_GRP_MSK   	(TSC_GROUP2)
#define CHANNEL_3_SRC       	(TSC_GROUP2_IDX)
#define CHANNEL_3_DEST      	(3)

#define CHANNEL_4_IO_MSK    	(TSC_GROUP2_IO3)
#define CHANNEL_4_GRP_MSK   	(TSC_GROUP2)
#define CHANNEL_4_SRC       	(TSC_GROUP2_IDX)
#define CHANNEL_4_DEST      	(4)

/* Shield IOs definition */
#define SHIELD_IO_MSK      		(0)

/* TOUCHKEYS_B bank(s) definition*/
#define BANK_0_NBCHANNELS 		(1)
#define BANK_0_MSK_CHANNELS   	(CHANNEL_0_IO_MSK)
#define BANK_0_MSK_GROUPS     	(CHANNEL_0_GRP_MSK)

#define BANK_1_NBCHANNELS 		(1)
#define BANK_1_MSK_CHANNELS  	(CHANNEL_1_IO_MSK)
#define BANK_1_MSK_GROUPS     	(CHANNEL_1_GRP_MSK)

#define BANK_2_NBCHANNELS 		(2)
#define BANK_2_MSK_CHANNELS   	(CHANNEL_2_IO_MSK | CHANNEL_3_IO_MSK)
#define BANK_2_MSK_GROUPS     	(CHANNEL_2_GRP_MSK | CHANNEL_3_GRP_MSK)

#define BANK_3_NBCHANNELS 		(1)
#define BANK_3_MSK_CHANNELS   	(CHANNEL_4_IO_MSK)
#define BANK_3_MSK_GROUPS     	(CHANNEL_4_GRP_MSK)

/* User Parameters */
extern CONST TSL_Bank_T TSL_BANKS[];
extern CONST TSL_TouchKeyB_T TSL_TKEYS_B[];
extern CONST TSL_Object_T TSL_OBJECTS[];
extern TSL_ObjectGroup_T TSL_OBJ_GROUP;

void TSL_User_Init(void);
tsl_user_status_t TSL_User_Handler(void);
void TSL_User_Set_Threshold(void);

#endif

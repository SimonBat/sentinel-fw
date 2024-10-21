/**
  ***************************************************************************************************************************************
  * @file     tsl_user.c
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

#include "tsl_user.h"

/* Source and Configuration (ROM) */
CONST TSL_ChannelSrc_T MyChannels_Src[TSLPRM_TOTAL_CHANNELS] = {
    { CHANNEL_0_SRC, CHANNEL_0_IO_MSK, CHANNEL_0_GRP_MSK },
    { CHANNEL_1_SRC, CHANNEL_1_IO_MSK, CHANNEL_1_GRP_MSK },
    { CHANNEL_2_SRC, CHANNEL_2_IO_MSK, CHANNEL_2_GRP_MSK },
    { CHANNEL_3_SRC, CHANNEL_3_IO_MSK, CHANNEL_3_GRP_MSK },
    { CHANNEL_4_SRC, CHANNEL_4_IO_MSK, CHANNEL_4_GRP_MSK },
};

/* Destination (ROM) */
CONST TSL_ChannelDest_T MyChannels_Dest[TSLPRM_TOTAL_CHANNELS] = {
    { CHANNEL_0_DEST },
    { CHANNEL_1_DEST },
    { CHANNEL_2_DEST },
    { CHANNEL_3_DEST },
    { CHANNEL_4_DEST },
};

/* Data (RAM) */
TSL_ChannelData_T MyChannels_Data[TSLPRM_TOTAL_CHANNELS];

/* List (ROM) */
CONST TSL_Bank_T TSL_BANKS[TSLPRM_TOTAL_BANKS] = {
	/* TOUCHKEYS_B bank(s) definition*/
	{&MyChannels_Src[0], &MyChannels_Dest[0], MyChannels_Data, BANK_0_NBCHANNELS, BANK_0_MSK_CHANNELS, BANK_0_MSK_GROUPS},
	{&MyChannels_Src[1], &MyChannels_Dest[1], MyChannels_Data, BANK_1_NBCHANNELS, BANK_1_MSK_CHANNELS, BANK_1_MSK_GROUPS},
	{&MyChannels_Src[2], &MyChannels_Dest[2], MyChannels_Data, BANK_2_NBCHANNELS, BANK_2_MSK_CHANNELS, BANK_2_MSK_GROUPS},
	{&MyChannels_Src[4], &MyChannels_Dest[4], MyChannels_Data, BANK_3_NBCHANNELS, BANK_3_MSK_CHANNELS, BANK_3_MSK_GROUPS},
};

/* Data (RAM) */
TSL_TouchKeyData_T MyTKeys_Data[TSLPRM_TOTAL_TKEYS];
/* Parameters (RAM) */
TSL_TouchKeyParam_T MyTKeys_Param[TSLPRM_TOTAL_TKEYS];

/* State Machine (ROM) */
void TSL_TKeys_Error_State_Process(void);
void TSL_TKeys_Off_State_Process(void);

CONST TSL_State_T MyTKeys_StateMachine[]={
	/* Calibration states */
	/* 0 */ { TSL_STATEMASK_CALIB,              TSL_tkey_CalibrationStateProcess },
	/* 1 */ { TSL_STATEMASK_DEB_CALIB,          TSL_tkey_DebCalibrationStateProcess },
	/* Release states */
	/* 2 */ { TSL_STATEMASK_RELEASE,            TSL_tkey_ReleaseStateProcess },
	#if TSLPRM_USE_PROX > 0
	/* 3 */ { TSL_STATEMASK_DEB_RELEASE_PROX,   TSL_tkey_DebReleaseProxStateProcess },
	#else
	/* 3 */ { TSL_STATEMASK_DEB_RELEASE_PROX,   0 },
	#endif
	/* 4 */ { TSL_STATEMASK_DEB_RELEASE_DETECT, TSL_tkey_DebReleaseDetectStateProcess },
	/* 5 */ { TSL_STATEMASK_DEB_RELEASE_TOUCH,  TSL_tkey_DebReleaseTouchStateProcess },
	#if TSLPRM_USE_PROX > 0
	/* Proximity states */
	/* 6 */ { TSL_STATEMASK_PROX,               TSL_tkey_ProxStateProcess },
	/* 7 */ { TSL_STATEMASK_DEB_PROX,           TSL_tkey_DebProxStateProcess },
	/* 8 */ { TSL_STATEMASK_DEB_PROX_DETECT,    TSL_tkey_DebProxDetectStateProcess },
	/* 9 */ { TSL_STATEMASK_DEB_PROX_TOUCH,     TSL_tkey_DebProxTouchStateProcess },
	#else
	/* 6 */ { TSL_STATEMASK_PROX,               0 },
	/* 7 */ { TSL_STATEMASK_DEB_PROX,           0 },
	/* 8 */ { TSL_STATEMASK_DEB_PROX_DETECT,    0 },
	/* 9 */ { TSL_STATEMASK_DEB_PROX_TOUCH,     0 },
	#endif
	/* Detect states */
	/* 10 */ { TSL_STATEMASK_DETECT,             TSL_tkey_DetectStateProcess },
	/* 11 */ { TSL_STATEMASK_DEB_DETECT,         TSL_tkey_DebDetectStateProcess },
	/* Touch state */
	/* 12 */ { TSL_STATEMASK_TOUCH,              TSL_tkey_TouchStateProcess },
	/* Error states */
	/* 13 */ { TSL_STATEMASK_ERROR,              TSL_TKeys_Error_State_Process },
	/* 14 */ { TSL_STATEMASK_DEB_ERROR_CALIB,    TSL_tkey_DebErrorStateProcess },
	/* 15 */ { TSL_STATEMASK_DEB_ERROR_RELEASE,  TSL_tkey_DebErrorStateProcess },
	/* 16 */ { TSL_STATEMASK_DEB_ERROR_PROX,     TSL_tkey_DebErrorStateProcess },
	/* 17 */ { TSL_STATEMASK_DEB_ERROR_DETECT,   TSL_tkey_DebErrorStateProcess },
	/* 18 */ { TSL_STATEMASK_DEB_ERROR_TOUCH,    TSL_tkey_DebErrorStateProcess },
	/* Other states */
	/* 19 */ { TSL_STATEMASK_OFF,                TSL_TKeys_Off_State_Process }
};

/* Methods for "extended" type (ROM) */
CONST TSL_TouchKeyMethods_T MyTKeys_Methods = {
	TSL_tkey_Init,
	TSL_tkey_Process
};

/* TouchKeys list (ROM) */
CONST TSL_TouchKeyB_T TSL_TKEYS_B[TSLPRM_TOTAL_TOUCHKEYS_B] = {
	{ &MyTKeys_Data[0], &MyTKeys_Param[0], &MyChannels_Data[CHANNEL_0_DEST] },
	{ &MyTKeys_Data[1], &MyTKeys_Param[1], &MyChannels_Data[CHANNEL_1_DEST] },
	{ &MyTKeys_Data[2], &MyTKeys_Param[2], &MyChannels_Data[CHANNEL_2_DEST] },
	{ &MyTKeys_Data[3], &MyTKeys_Param[3], &MyChannels_Data[CHANNEL_3_DEST] },
	{ &MyTKeys_Data[4], &MyTKeys_Param[4], &MyChannels_Data[CHANNEL_4_DEST] }
};

/* List (ROM) */
CONST TSL_Object_T TSL_OBJECTS[TSLPRM_TOTAL_OBJECTS] = {
	{ TSL_OBJ_TOUCHKEYB, (TSL_TouchKeyB_T *)&TSL_TKEYS_B[0] },
	{ TSL_OBJ_TOUCHKEYB, (TSL_TouchKeyB_T *)&TSL_TKEYS_B[1] },
	{ TSL_OBJ_TOUCHKEYB, (TSL_TouchKeyB_T *)&TSL_TKEYS_B[2] },
	{ TSL_OBJ_TOUCHKEYB, (TSL_TouchKeyB_T *)&TSL_TKEYS_B[3] },
	{ TSL_OBJ_TOUCHKEYB, (TSL_TouchKeyB_T *)&TSL_TKEYS_B[4] }
};

/* Group (RAM) */
TSL_ObjectGroup_T TSL_OBJ_GROUP = {
	&TSL_OBJECTS[0], /* First object */
	TSLPRM_TOTAL_OBJECTS, /* Number of objects */
	0x00, /* State mask reset value */
	TSL_STATE_NOT_CHANGED /* Current state */
};

TSL_Params_T TSL_Params = {
	TSLPRM_ACQ_MIN,
	TSLPRM_ACQ_MAX,
	TSLPRM_CALIB_SAMPLES,
	TSLPRM_DTO,
	#if TSLPRM_TOTAL_TKEYS > 0
	MyTKeys_StateMachine, /* Default state machine for TKeys */
	&MyTKeys_Methods, /* Default methods for TKeys */
	#endif
	#if TSLPRM_TOTAL_LNRTS > 0
	MyLinRots_StateMachine, /* Default state machine for LinRots */
	&MyLinRots_Methods /* Default methods for LinRots */
	#endif
};

__IO TSL_tTick_ms_T ECSLastTick; /* Hold the last time value for ECS */

/**
  ***************************************************************************************************************************************
  * @brief  Initialize the STMTouch Driver
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void TSL_User_Init(void)
{
	TSL_obj_GroupInit(&TSL_OBJ_GROUP); /* Init Objects */
	TSL_Init(TSL_BANKS); /* Init acquisition module */
	TSL_User_Set_Threshold(); /* Init thresholds for each object individually (optional) */
}

/**
  ***************************************************************************************************************************************
  * @brief  Execute STMTouch Driver main State machine
  * @param  None
  * @retval status Return TSL_STATUS_OK if the acquisition is done
  ***************************************************************************************************************************************
  */
tsl_user_status_t TSL_User_Handler(void)
{
	static uint32_t _idx_bank = 0U;
	static uint32_t _config_done = 0U;
	tsl_user_status_t _status = TSL_USER_STATUS_BUSY;

	/* Configure and start bank acquisition */
	if(!_config_done)
	{
		TSL_acq_BankConfig(_idx_bank);
		TSL_acq_BankStartAcq();
		_config_done = 1U;
	}

	/* Check end of acquisition (polling mode) and read result */
	if(TSL_STATUS_OK == TSL_acq_BankWaitEOC())
	{
		STMSTUDIO_LOCK;
		TSL_acq_BankGetResult(_idx_bank, 0, 0);
		STMSTUDIO_UNLOCK;
		_idx_bank++; /* Next bank */
		_config_done = 0U;
	}

	/* Process objects, DxS and ECS, check if all banks have been acquired */
	if(_idx_bank > (TSLPRM_TOTAL_BANKS - 1U))
	{
		/* Reset flags for next banks acquisition */
		_idx_bank = 0U;
		_config_done = 0U;

		/* Process Objects */
		TSL_obj_GroupProcess(&TSL_OBJ_GROUP);
		/* DxS processing (if TSLPRM_USE_DXS option is set) */
		TSL_dxs_FirstObj(&TSL_OBJ_GROUP);

		/* ECS every TSLPRM_ECS_DELAY (in ms) */
		if(TSL_STATUS_OK == TSL_tim_CheckDelay_ms(TSLPRM_ECS_DELAY, &ECSLastTick))
		{
			if(TSL_STATUS_OK == TSL_ecs_Process(&TSL_OBJ_GROUP)){_status = TSL_USER_STATUS_OK_ECS_ON;}
			else{_status = TSL_USER_STATUS_OK_ECS_OFF;}
		}else{_status = TSL_USER_STATUS_OK_NO_ECS;}
	}else{_status = TSL_USER_STATUS_BUSY;}

	return _status;
}

/**
  ***************************************************************************************************************************************
  * @brief  Set thresholds for each object (optional).
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void TSL_User_Set_Threshold(void)
{
	/** Example: Decrease the Detect thresholds for the TKEY 0
	  * MyTKeys_Param[0].DetectInTh -= 10;
	  * MyTKeys_Param[0].DetectOutTh -= 10;
	  */
}

/**
  ***************************************************************************************************************************************
  * @brief  Executed when a sensor is in Error state
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void TSL_TKeys_Error_State_Process(void)
{
	  /* Add here your own processing when a sensor is in Error state */
}

/**
  ***************************************************************************************************************************************
  * @brief  Executed when a sensor is in Off state
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void TSL_TKeys_Off_State_Process(void)
{
	/* Add here your own processing when a sensor is in Off state */
}

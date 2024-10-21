/**
  ***************************************************************************************************************************************
  * @file     tsl_cs.c
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

#include "tsl_cs.h"

#define TARGET_LOCK_ID 	0 /* Do not modify - shared with STMStudio host software */
#define HOST_LOCK_ID   	1 /* Do not modify - shared with STMStudio host software */

typedef struct {
    volatile unsigned char flag[2]; /* Do not modify - shared with STMStudio host software */
    volatile unsigned char turn;    /* Do not modify - shared with STMStudio host software */
} petersons_t;

/* Stm_studio_lock symbol used by the STMStudio host software for synchronization */
petersons_t stm_studio_lock = {{ 0, 0 }, TARGET_LOCK_ID}; /* Do not modify - shared with STMStudio host software */

/**
  ***************************************************************************************************************************************
  * @brief  Enter lock
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void TSL_Enter_Lock(void)
{
    stm_studio_lock.flag[TARGET_LOCK_ID] = 1U;
    stm_studio_lock.turn = HOST_LOCK_ID;
    while (stm_studio_lock.flag[HOST_LOCK_ID] && (HOST_LOCK_ID == stm_studio_lock.turn)){};
}

/**
  ***************************************************************************************************************************************
  * @brief  Exit lock
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void TSL_Exit_Lock(void)
{
    stm_studio_lock.flag[TARGET_LOCK_ID] = 0U;
}

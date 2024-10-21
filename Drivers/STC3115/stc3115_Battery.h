/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
* File Name          : stc3115_Battery.h
* Author             : AMS - IMS application
* Version            : V00
* Date               : 30 July 2014
* Description        : Application/Battery description
********************************************************************************
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.

* THIS SOURCE CODE IS PROTECTED BY A LICENSE.
* FOR MORE INFORMATION PLEASE CAREFULLY READ THE LICENSE AGREEMENT FILE LOCATED
* IN THE ROOT DIRECTORY OF THIS FIRMWARE PACKAGE.
*******************************************************************************/

/* Define to prevent recursive inclusion ------------------------------------ */
#ifndef __Battery_H
#define __Battery_H


/* ******************************************************************************** */
/*        INTERNAL PARAMETERS                                                       */
/*   TO BE ADJUSTED ACCORDING TO BATTERY AND APPLCICATION CHARACTERISTICS           */
/* -------------------------------------------------------------------------------- */

/*Battery parameters define  ------------------------------------------------------ */
#define CAPACITY		100	/* Battery nominal capacity in mAh					*/
#define RINT			200 /* Internal battery impedance in mOhms,0 if unknown	*/

#define OCV_OFFSET_TAB	{0,-54,-60,-31,-27,-40,-25,-16,-22,-42,-93,-98,-111,-127,-127,-127} /* OCVTAB */
	
/*Application parameters define  -------------------------------------------------- */
#define VMODE 			MIXED_MODE	/* Running mode constant, VM_MODE or MIXED_MODE	*/
#define ALM_EN			0			/* Alarm enable constant, set at 1 to enable	*/
#define ALM_SOC			0			/* SOC alarm in % 								*/
#define ALM_VBAT 		3300		/* Voltage alarm in mV							*/
#define RSENSE			50			/* Sense resistor in mOhms 						*/

#define APP_EOC_CURRENT       10   		/* End charge current in mA                 */
#define APP_CUTOFF_VOLTAGE	  3300   	/* Application cut-off voltage in mV      	*/
/* ******************************************************************************** */

#endif

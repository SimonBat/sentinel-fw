/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
* File Name          : stc3115_Driver.c
* Author             : AMS - IMS application
* Version            : V01
* Date               : 10 October 2014
* Description        : STC3115 driver source code
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

/* Includes ------------------------------------------------------------------*/
#include "stc3115_Driver.h" 
#include "stc3115_Battery.h"
#include "i2c.h"

/*
  ===============================================================================
                   ##### STC3115 driver description #####
  ===============================================================================

	This driver is dedicated to STC3115 battery monitoring IC management, it allows
	the stc3115 to be configured, and used without external software.
	
	++ stc3115_Driver.c file contains the functions to initialize and report the 
			battery state based on STC3115 algorithms.
		- Driver contains its own I2C access functions, they have to be linked with 
			I2C driver interface  functions. HW system have to support multi I2C access 
			commands to guaranty the data integrity during the 16 bits register R/W 
			operations.
		- First part of the driver is composed by driver own functions, which should 
			not be accessed by external functions.
		- Then the GasGauge functions are the main interface functions which have to 
			be used by the application to initialize, monitor and stop the STC3115 
			device.
		- Power saving functions can be used to manage the STC3115 operating mode 
			(mixed mode or Voltage mode). These two functions have to be called by 
			external program to switch the STC3115 operating mode after STC3115 
			initialization.
		- Alarm management functions have to be called to set, stop, get, clear and
			change alarm thresholds after STC3115 initialization.
		- The relaxation register can also be updated after initialization thanks to 
			the STC3115_RelxTmrSet.
	
	++ stc3115_Driver.h file defines the STC3115 static numbers, driver API 
			structures and driver API functions.	
		- STC3115_BatteryData_TypeDef is filled by the driver with battery state.
		- STC3115_ConfigData_TypeDef is filled by the driver with battery 
			configuration data from stc3115_Battery.h
		- RAMData is the RAM memory map description, used internally of the driver.
			Have no to be modified by external accesses.
	
	++ stc3115_Battery.h file is used to describe the battery and application 
			configurations.
  ===============================================================================
                    ##### How to use STC3115 driver #####
  ===============================================================================
	++	Initialize STC3115 dedicated hardware externally of the driver (I2C 
			interface, GPIOs...)			
	
	++	Fill STC3115_Battery.h with battery and application parameters
	
	++  Call one time the the GasGauge_Initialization function
	
			Parameters:
				- STC3115_ConfigData_TypeDef parameter is filled with STC3115_Battery.h 
					data by the driver
				- STC3115_BatteryData_TypeDef parameter is filled with STC3115 algorithm 
					results and returns the battery state to application
				
			Function description:
				- Initialize SW structures
				- Check battery history: new battery, swapped battery, already connected 
					battery
				- Initializes STC3115 registers accordingly to battery history
				- Initializes STC3115 RAM memory accordingly to battery history
			
			Returns:
				- (-1) is returned when STC3115 cannot be accessed: I2C bus is not 
					properly configured or STC3115 is not power supplied
				- (0) is returned if initialization succeeds.
			
	++ Call the GasGauge_Task function when battery information is needed: 
			every 1 to 30 seconds (no frequency accuracy needed)
		
			Parameters:
				- STC3115_ConfigData_TypeDef parameter is used in case of STC3115 has to be 
					initialized again
				- STC3115_BatteryData_TypeDef parameter is filled with STC3115 algorithm 
					results and returns the battery state to application
				
			Function description:
				- Check STC3115, battery and application state
				- if battery is well present battery data are updated
				- Available battery state information are returned
			
			Returns:
				- (-1) is returned when STC3115 cannot be accessed (I2C bus is busy or 
					STC3115 is not power supplied) or when BATD/CD pin is in high level state.
				- (0) is returned if only battery SOC,Voltage and OCV data are available
				- (1) is returned if every STC3115_BatteryData_TypeDef data are available.
				
	++ call the GaSGauge_Stop function during the application switch off sequence
		 This will stop the STC3115 and save energy. This will help to recover the
		 battery context during the next GasGauge_Initialization function.
				
	++ GasGauge_Reset function has not to be used by default.
		 This function has to be called only in abnormal use cases.
				- (-1) is returned when STC3115 cannot be stopped
				- (0) is returned if reset operation succeeds
				
*/

/* ---- STC3115 I2C R/W interface --------------------------------------------- */

/* stc3115 RAM registers structure -------------------------------------------- */
static union{
	unsigned char db[RAM_SIZE];  /* last byte holds the CRC 						*/
	struct{
		short int TstWord;     /* 0-1 												*/
		short int HRSOC;       /* 2-3 SOC backup in (1/512%)						*/
		short int CC_cnf;      /* 4-5 current CC_cnf 								*/
		short int VM_cnf;      /* 6-7 current VM_cnf 								*/
		char SOC;              /* 8 SOC (in %) 										*/
		char STC3115_Status;   /* 9  STC3115 working state							*/
		/* bytes ..RAM_SIZE-2 are free, last byte RAM_SIZE-1 is the CRC 			*/
	} reg;
}RAMData;

/*******************************************************************************
* Function Name  : STC3115_ReadByte
* Description    : utility function to read the value stored in one register
* Input          : RegAddress: STC3115 register,
* Return         : 8-bit value, or 0 if error
*******************************************************************************/
static int STC3115_ReadByte(int RegAddress)
{
	int value;
	unsigned char data[2];
	int res;

	res=I2C_Driver_Read(STC3115_SLAVE_ADDRESS,RegAddress,1,data,1);
	
	if(res==0){value=data[0];}
	else{value=-1;}

	return(value);
}

/*******************************************************************************
* Function Name  : STC3115_WriteByte
* Description    : utility function to write a 8-bit value into a register
* Input          : RegAddress: STC3115 register, Value: 8-bit value to write
* Return         : error status (OK, !OK)
*******************************************************************************/
static int STC3115_WriteByte(int RegAddress, unsigned char Value)
{
	int res;
	unsigned char data[2];

	data[0]=Value;
	res=I2C_Driver_Write(STC3115_SLAVE_ADDRESS,RegAddress,1,data,1);
	
	return(res);
}

/*******************************************************************************
* Function Name  : STC3115_ReadWord
* Description    : utility function to read the value stored in one register pair
* Input          : RegAddress: STC3115 register,
* Return         : 16-bit value, or 0 if error
*******************************************************************************/
static int STC3115_ReadWord(int RegAddress)
{
	int value;
	unsigned char data[2];
	int res;

	res=I2C_Driver_Read(STC3115_SLAVE_ADDRESS,RegAddress,1,data,2);
  
	if(res==0)
	{
		/* No error */
		value=data[1];
		value=(value<<8)+data[0];
	}
	else{value=-1;}

	return(value);
}

/*******************************************************************************
* Function Name  : STC3115_WriteWord
* Description    : utility function to write a 16-bit value into a register pair
* Input          : RegAddress: STC3115 register, Value: 16-bit value to write
* Return         : error status (OK, !OK)
*******************************************************************************/
static int STC3115_WriteWord(int RegAddress, int Value)
{
	int res;
	unsigned char data[2];

	data[0]= Value & 0xff;
	data[1]= (Value>>8) & 0xff;
	res=I2C_Driver_Write(STC3115_SLAVE_ADDRESS,RegAddress,1,data,2);
	
	return(res);
}

/*******************************************************************************
* Function Name  : STC3115_ReadWord
* Description    : utility function to read the value stored in one register pair
* Input          : RegAddress: STC3115 register, nbr: number of bytes to read
* Return         : 16-bit value, or 0 if error
*******************************************************************************/
static int STC3115_ReadBytes(unsigned char *data,int RegAddress,int nbr)
{
	int res;

	res=I2C_Driver_Read(STC3115_SLAVE_ADDRESS,RegAddress,1,data,nbr);
  
	return(res);
}

/*******************************************************************************
* Function Name  : STC3115_WriteWord
* Description    : utility function to write a 16-bit value into a register pair
* Input          : RegAddress: STC3115 register, Value: 16-bit value to write, nbr: number of bytes to write
* Return         : error status (OK, !OK)
*******************************************************************************/
static int STC3115_WriteBytes(unsigned char *data,int RegAddress,int nbr)
{
	int res;

	res=I2C_Driver_Write(STC3115_SLAVE_ADDRESS,RegAddress,1,data,nbr);
	
	return(res);
}

/* ---- End of I2C R/W interface --------------------------------------------- */
/* ---- Internal functions --------------------------------------------------- */

/*******************************************************************************
* Function Name  : STC3115_Status
* Description    :  Read the STC3115 status
* Input          : None
* Return         : status word (REG_MODE / REG_CTRL), -1 if error
*******************************************************************************/
static int STC3115_Status(void)
{
	int value;

	/* First, check the presence of the STC3115 by reading first byte of dev. ID */
	if(STC3115_ReadByte(STC3115_REG_ID)!= STC3115_ID){return (-1);}

	/* Read REG_MODE and REG_CTRL */
	value = STC3115_ReadWord(STC3115_REG_MODE);
	value &= 0x7fff;

	return (value);
}

/*******************************************************************************
* Function Name  : STC3115_SetParam
* Description    :  initialize the STC3115 parameters
* Input          : rst: init algo param
* Return         : 0
*******************************************************************************/
static void STC3115_SetParam(STC3115_ConfigData_TypeDef *ConfigData)
{
	int value;
  
	/* Set GG_RUN=0 before changing algo parameters */
	STC3115_WriteByte(STC3115_REG_MODE,STC3115_VMODE);
  
	/* Init OCV curve */
	STC3115_WriteBytes((unsigned char*) ConfigData->OCVOffset, STC3115_REG_OCVTAB, OCVTAB_SIZE);
  
	/* Set alarm level if different from default */
	if(ConfigData->Alm_SOC !=0 ){STC3115_WriteByte(STC3115_REG_ALARM_SOC,ConfigData->Alm_SOC*2);}

	if(ConfigData->Alm_Vbat !=0 )
	{
		value= ((long)(ConfigData->Alm_Vbat << 9) / VoltageFactor); /* LSB=8*2.2mV */
		STC3115_WriteByte(STC3115_REG_ALARM_VOLTAGE, value);
	}
    
	/* Relaxation timer */
	if(ConfigData->Rsense !=0 )
	{
		value= ((long)(ConfigData->RelaxCurrent << 9) / (CurrentFactor / ConfigData->Rsense));   /* LSB=8*5.88uV */
		STC3115_WriteByte(STC3115_REG_CURRENT_THRES,value);
	}
  
	/* Set parameters if different from default, only if a restart is done (battery change) */
	if (RAMData.reg.CC_cnf !=0 ) STC3115_WriteWord(STC3115_REG_CC_CNF,RAMData.reg.CC_cnf);
	if (RAMData.reg.VM_cnf !=0 ) STC3115_WriteWord(STC3115_REG_VM_CNF,RAMData.reg.VM_cnf);

	STC3115_WriteByte(STC3115_REG_CTRL,0x03);  /* Clear PORDET, BATFAIL, free ALM pin, reset conv counter */
	STC3115_WriteByte(STC3115_REG_MODE, STC3115_GG_RUN | (STC3115_VMODE * ConfigData->Vmode) | (STC3115_ALM_ENA * ALM_EN));  /* Set GG_RUN=1, set mode, set alarm enable */
 
	return;
}  

/*******************************************************************************
* Function Name  : STC3115_Startup
* Description    :  initialize and start the STC3115 at application startup
* Input          : None
* Return         : 0 if ok, -1 if error
*******************************************************************************/
static int STC3115_Startup(STC3115_ConfigData_TypeDef *ConfigData)
{
	int res;
	int ocv;
  
	/* Check STC310x status */
	res = STC3115_Status();
	if(res<0){return(res);}

	/* Read OCV */
	ocv=STC3115_ReadWord(STC3115_REG_OCV);
	STC3115_SetParam(ConfigData);  /* set STC3115 parameters  */
	/* Rewrite ocv to start SOC with updated OCV curve */
	STC3115_WriteWord(STC3115_REG_OCV,ocv);
  
	return(0);
}

/*******************************************************************************
* Function Name  : STC3115_Restore
* Description    :  Restore STC3115 state
* Input          : None
* Return         : 
*******************************************************************************/
static int STC3115_Restore(STC3115_ConfigData_TypeDef *ConfigData)
{
	int res;

	/* Check STC310x status */
	res = STC3115_Status();
	if(res<0){return(res);}
 
	STC3115_SetParam(ConfigData);  /* set STC3115 parameters  */
	/* Restore SOC from RAM data */
	STC3115_WriteWord(STC3115_REG_SOC,RAMData.reg.HRSOC);

	return(0);
}

/*******************************************************************************
* Function Name  : STC3115_Powerdown
* Description    :  stop the STC3115 at application power down
* Input          : None
* Return         : error status (OK, !OK)
*******************************************************************************/
static int STC3115_Powerdown(void)
{
	int res;
  
	/* Write 0x01 into the REG_CTRL to release IO0 pin open, */
	STC3115_WriteByte(STC3115_REG_CTRL, 0x01);
	/* Write 0 into the REG_MODE register to put the STC3115 in standby mode */
	res = STC3115_WriteByte(STC3115_REG_MODE, 0);
	if(res!= OK){return (res);}

	return (OK);
}

/*******************************************************************************
* Function Name  : STC3115_conv
* Description    : conversion utility 
*  convert a raw 16-bit value from STC3115 registers into user units (mA, mAh, mV, �C)
*  (optimized routine for efficient operation on 8-bit processors such as STM8)
* Input          : value, factor
* Return         : result = value * factor / 4096
*******************************************************************************/
static int STC3115_conv(short value, unsigned short factor)
{
	int v;
  
	v= ( (long) value * factor ) >> 11;
	v= (v+1)/2;
  
	return (v);
}

/*******************************************************************************
* Function Name  : STC3115_ReadBatteryData
* Description    :  utility function to read the battery data from STC3115
*                  to be called every 5s or so
* Input          : ref to BatteryData structure
* Return         : error status (OK, !OK)
*******************************************************************************/
static int STC3115_ReadBatteryData(STC3115_BatteryData_TypeDef *BatteryData)
{
	unsigned char data[16];
	int res;
	int value;

	/* Read STC3115 registers 0 to 14 */
	res = STC3115_ReadBytes(data, 0, 15);
	
	if(res<0){return(res);}  /* read failed */
	
	/* Fill the battery status data */
	/* SOC */
	value=data[3]; value = (value<<8) + data[2];
	BatteryData->HRSOC = value;     /* Result in 1/512% */
	BatteryData->SOC = (value * 10 + 256) / 512; /* Result in 0.1% */

	/* Conversion counter */
	value=data[5]; value = (value<<8) + data[4];
	BatteryData->ConvCounter = value;

	/* Current */
	value=data[7]; value = (value<<8) + data[6];
	value &= 0x3fff;   /* Mask unused bits */
	if(value>=0x2000){value = value - 0x4000;}  /* Convert to signed value */
	BatteryData->Current = STC3115_conv(value, CurrentFactor/RSENSE);  /* Result in mA */

	/* Voltage */
	value=data[9]; value = (value<<8) + data[8];
	value &= 0x0fff; /* Mask unused bits */
	if(value>=0x0800){value -= 0x1000;}  /* Convert to signed value */
	value = STC3115_conv(value,VoltageFactor);  /* Result in mV */
	BatteryData->Voltage = value;  /* Result in mV */

	/* Temperature */
	value=data[10];
	if(value>=0x80){value -= 0x100;}  /* Convert to signed value */
	BatteryData->Temperature = value*10;  /* Result in 0.1�C */

	/* OCV */
	value=data[14]; value = (value<<8) + data[13];
	value &= 0x3fff; /* Mask unused bits */
	if(value>=0x02000){value -= 0x4000;} /* Convert to signed value */
	value = STC3115_conv(value,VoltageFactor);
	value = (value+2) / 4;  /* Divide by 4 with rounding */
	BatteryData->OCV = value;  /* Result in mV */

	return(OK);
}

/*******************************************************************************
* Function Name  : STC3115_ReadRamData
* Description    : utility function to read the RAM data from STC3115
* Input          : ref to RAM data array
* Return         : error status (OK, !OK)
*******************************************************************************/
static int STC3115_ReadRamData(unsigned char *RamData)
{
	return STC3115_ReadBytes(RamData, STC3115_REG_RAM, RAM_SIZE);
}

/*******************************************************************************
* Function Name  : STC3115_WriteRamData
* Description    : utility function to write the RAM data into STC3115
* Input          : ref to RAM data array
* Return         : error status (OK, !OK)
*******************************************************************************/
static int STC3115_WriteRamData(unsigned char *RamData)
{
	return STC3115_WriteBytes(RamData, STC3115_REG_RAM, RAM_SIZE);
}

/*******************************************************************************
* Function Name  : STC3115_CalcRamCRC8
* Description    : calculate the CRC8
* Input          : data: pointer to byte array, n: number of vytes
* Return         : CRC calue
*******************************************************************************/
static int STC3115_CalcRamCRC8(unsigned char *data, int n)
{
	int crc=0;   /* Initial value */
	int i, j;

	for (i=0;i<n;i++)
	{
		crc ^= data[i];
		for (j=0;j<8;j++)
		{
			crc <<= 1;
			if(crc & 0x100){crc ^= 7;}
		}
	}

	return(crc & 255);
}

/*******************************************************************************
* Function Name  : STC3115_UpdateRamCRC
* Description    : calculate the RAM CRC
* Input          : none
* Return         : CRC value
*******************************************************************************/
static int STC3115_UpdateRamCRC(void)
{
	int res;
  
	res=STC3115_CalcRamCRC8(RAMData.db,RAM_SIZE-1);
	RAMData.db[RAM_SIZE-1] = res; /* Last byte holds the CRC */

	return(res);
}

/*******************************************************************************
* Function Name  : STC3115_InitRamData
* Description    : Init the STC3115 RAM registers with valid test word and CRC
* Input          : STC3115_ConfigData_TypeDef structure
* Return         : none
*******************************************************************************/
static void STC3115_InitRamData(STC3115_ConfigData_TypeDef *ConfigData)
{
	int index;
	/* Set full RAM tab to 0 */
	for (index=0;index<RAM_SIZE;index++)
    RAMData.db[index]=0;
	/* Fill RAM regs */
	RAMData.reg.TstWord=RAM_TSTWORD;  /* Fixed word to check RAM integrity */
	RAMData.reg.CC_cnf = ConfigData->CC_cnf;
	RAMData.reg.VM_cnf = ConfigData->VM_cnf;
	/* Update the crc */
	STC3115_UpdateRamCRC();
}

/* ---- End of Internal functions --------------------------------------------------- */
/* ---- Driver interface functions -------------------------------------------------- */

/*******************************************************************************
* Function Name  : STC3115_Initialization
* Description    : Start the STC3115 battery tracking algorithm system
* Input          : algo parameters in stc3115_Battery.h file
* Return         : 0 is ok, -1 if STC3115 not found or I2C error
* Affect         : Initializes ConfigData and returns battery early status in BatteryData
*******************************************************************************/
int GasGauge_Initialization(STC3115_ConfigData_TypeDef *ConfigData,STC3115_BatteryData_TypeDef *BatteryData)
{
	int res=0;
	int loop;
	int OCVOffset[16] = OCV_OFFSET_TAB;

	/* Fill configuration structure parameters */
	ConfigData->Vmode = VMODE;

	if(RSENSE!=0){ConfigData->Rsense = RSENSE;}
	else{ConfigData->Rsense = 10;} /* Default value to avoid division by 0 */

	ConfigData->CC_cnf = (CAPACITY * ConfigData->Rsense * 250 + 6194) / 12389;

	if(RINT!=0){ConfigData->VM_cnf = (CAPACITY * RINT * 50 + 24444) / 48889;}
	else{ConfigData->VM_cnf = (CAPACITY * 200 * 50 + 24444) / 48889;} /* Default value */

	for(loop=0;loop<16;loop++)
	{
		if(OCVOffset[loop] > 127){OCVOffset[loop] = 127;}
		if(OCVOffset[loop] < -127){OCVOffset[loop] = -127;}
		ConfigData->OCVOffset[loop] = OCVOffset[loop];
	}

	ConfigData->Cnom = CAPACITY;
	ConfigData->RelaxCurrent = CAPACITY / 20;
	ConfigData->Alm_SOC = ALM_SOC;
	ConfigData->Alm_Vbat = ALM_VBAT;

	/* Initialize Gas Gauge system */
	/* Battery presence status init */
	BatteryData->Presence = 1;
	/* Check RAM data validity */
	STC3115_ReadRamData(RAMData.db);

	if((RAMData.reg.TstWord != RAM_TSTWORD)||(STC3115_CalcRamCRC8(RAMData.db,RAM_SIZE)!=0)){STC3115_InitRamData(ConfigData);}

	GasGauge_Reset();
	res=STC3115_Startup(ConfigData);

	RAMData.reg.STC3115_Status = STC3115_INIT;
	STC3115_UpdateRamCRC();
	STC3115_WriteRamData(RAMData.db);

	return(res);
}

/*******************************************************************************
* Function Name  : GasGauge_Reset
* Description    : Reset the Gas Gauge system, this function must not be used frequently
* Input          : None
* Return         : 0 is ok, -1 if I2C error
*******************************************************************************/
int GasGauge_Reset(void)  
{
	int res;

	/* Reset RAM */
	RAMData.reg.TstWord=0;
	RAMData.reg.STC3115_Status = 0;
	res = STC3115_WriteRamData(RAMData.db);
	if(res != OK){return res;}
	/* Reset STC3115 */
	res = STC3115_WriteByte(STC3115_REG_CTRL, STC3115_PORDET);  /* Set soft POR */

	return res;
}

/*******************************************************************************
* Function Name  : GasGauge_Stop
* Description    : Stop the Gas Gauge system and save the context in STC3115 RAM
* Input          : None
* Return         : 0 is ok, -1 if I2C error
*******************************************************************************/
int GasGauge_Stop(void)
{
	int res;
  
	/* Save context in RAM */
	STC3115_ReadRamData(RAMData.db);
	RAMData.reg.STC3115_Status= STC3115_POWERDN;
	/* Update the crc */
	STC3115_UpdateRamCRC();
	STC3115_WriteRamData(RAMData.db);
   
	/* STC3115 Power down */
	res=STC3115_Powerdown();
	if(res!=0){return (-1);}  /* Error */

	return(0);
}

/*******************************************************************************
* Function Name  : GasGauge_Task
* Description    : Periodic Gas Gauge task, to be called periodically (every 1 to 60 seconds)
* Input          : STC315 ConfigData structure filled by GasGauge_Initialization function
* Return         : 1 if data available, 0 if no data, -1 if error
* Affect         : Fill the BatteryData structure with updated battery status
*******************************************************************************/
int GasGauge_Task(STC3115_ConfigData_TypeDef *ConfigData, STC3115_BatteryData_TypeDef *BatteryData)
{
	int res;

	/* ----------------------------- System state verification ---------------------------- */
	/* Read STC3115 status registers */
	res=STC3115_Status();

	if(res<0){return(res);}  /* Return if I2C error or STC3115 not responding */
	BatteryData->status = res;
  
	/* Check STC3115 RAM status (battery has not been changed) */
	STC3115_ReadRamData(RAMData.db);
	if((RAMData.reg.TstWord!= RAM_TSTWORD)||(STC3115_CalcRamCRC8(RAMData.db,RAM_SIZE)!=0))
	{
		/* If RAM non ok, reset it and set init state */
		STC3115_InitRamData(ConfigData);
		RAMData.reg.STC3115_Status = STC3115_INIT;
	}
  
	/* Check battery presence status */
	if((BatteryData->status & (STC3115_BATFAIL<<8)) != 0)
	{
		/*Battery disconnection has been detected			*/
		/* BATD pin level is over 1.61 or Vcc is below 2.7V	*/
		BatteryData->Presence = 0;
		/* HW and SW state machine reset */
		GasGauge_Reset();

		return (-1);
	}
	
	/* Check STC3115 running mode*/
	if((BatteryData->status & STC3115_GG_RUN) == 0)
	{
		if(RAMData.reg.STC3115_Status == STC3115_RUNNING){STC3115_Restore(ConfigData);  /* if RUNNING state, restore STC3115*/}
		else{STC3115_Startup(ConfigData); /* if INIT state, initialize STC3115*/}
		
		RAMData.reg.STC3115_Status = STC3115_INIT;
	}
  
	/* --------------------------------- Read battery data ------------------------------- */
	res=STC3115_ReadBatteryData(BatteryData);
	if(res!=0){return(-1);} /* Abort in case of I2C failure */
  
	/* ------------------------------- Battery data report ------------------------------- */
	/* Check INIT state */
	if (RAMData.reg.STC3115_Status == STC3115_INIT)
	{
		/* INIT state, wait for current & temperature value available: */
		if(BatteryData->ConvCounter>VCOUNT)
		{
			RAMData.reg.STC3115_Status = STC3115_RUNNING;
			/* Battery is connected */
			BatteryData->Presence = 1;
		}
	}

	if(RAMData.reg.STC3115_Status != STC3115_RUNNING) /* Not running : data partially available*/
	{
		BatteryData->ChargeValue = ConfigData->Cnom * BatteryData->SOC / MAX_SOC;
		BatteryData->Current=0;
		BatteryData->Temperature=250;
		BatteryData->RemTime = -1;
	}
	else /*STC3115 running */
	{
		/* ---------- Process SW algorithms -------- */
		/* Early empty compensation */
		if(BatteryData->Voltage<APP_CUTOFF_VOLTAGE){BatteryData->SOC = 0;}
		else if (BatteryData->Voltage<(APP_CUTOFF_VOLTAGE+200)){BatteryData->SOC = BatteryData->SOC * (BatteryData->Voltage - APP_CUTOFF_VOLTAGE) / 200;}

		/* Battery charge value calculation */
		BatteryData->ChargeValue = ConfigData->Cnom * BatteryData->SOC / MAX_SOC;

		if((BatteryData->status & STC3115_VMODE) == 0) /* Mixed mode only*/
		{
		
			/* Lately fully compensation */
			if ((BatteryData->status & STC3115_VMODE) == 0) /* Running in mixed mode */
			{
			
				if(BatteryData->Current > APP_EOC_CURRENT && BatteryData->SOC > 990)
				{
					BatteryData->SOC = 990;
					STC3115_WriteWord(STC3115_REG_SOC,50688);   /* 99% */
				}
			}

			/* Remaining time calculation */
			if(BatteryData->Current < 0)
			{
				BatteryData->RemTime = (BatteryData->ChargeValue * CAPACITY * 6)/(-BatteryData->Current);
				if(BatteryData->RemTime < 0){BatteryData->RemTime = -1; /* means no estimated time available */}
			}
			else{BatteryData->RemTime = -1; /* means no estimated time available */}

		}
		else /* voltage mode only */
		{
			BatteryData->Current=0;
			BatteryData->RemTime=-1;
		}

		/* SOC min/max clamping */
		if(BatteryData->SOC>1000){BatteryData->SOC = MAX_SOC;}
		if(BatteryData->SOC<0){BatteryData->SOC = 0;}
	}
      
	/* save SOC */
	RAMData.reg.HRSOC = BatteryData->HRSOC;
	RAMData.reg.SOC = (BatteryData->SOC+5)/10;
	STC3115_UpdateRamCRC();
	STC3115_WriteRamData(RAMData.db);

	if(RAMData.reg.STC3115_Status==STC3115_RUNNING){return(1);}
	else{return(0);  /* Only SOC, OCV and voltage are valid */}
}

/*******************************************************************************
* Function Name  : STC3115_SetPowerSavingMode
* Description    : Set the power saving mode
* Input          : None
* Return         : error status (OK, !OK)
*******************************************************************************/
int STC3115_SetPowerSavingMode(void)
{
	int res;
  
	/* Read the mode register*/
	res = STC3115_ReadByte(STC3115_REG_MODE);
	/* Set the VMODE bit to 1 */
	res = STC3115_WriteByte(STC3115_REG_MODE, (res | STC3115_VMODE));
	if(res!= OK){return (res);}

	return (OK);
}

/*******************************************************************************
* Function Name  : STC3115_StopPowerSavingMode
* Description    : Stop the power saving mode
* Input          : None
* Return         : error status (OK, !OK)
*******************************************************************************/
int STC3115_StopPowerSavingMode(void)
{
	int res;
  
	/* Read the mode register*/
	res = STC3115_ReadByte(STC3115_REG_MODE);
	/* STC3115 is in power saving mode by default, cannot be set dynamically in mixed mode. */
	/* Change stc3115_Driver.h VMODE parameter, and connect an external sense resistor to STC311 */
	if (VMODE != MIXED_MODE) return (!OK);
  
	/* Set the VMODE bit to 0 */
	res = STC3115_WriteByte(STC3115_REG_MODE, (res & ~STC3115_VMODE));
	if (res!= OK) return (res);

	return (OK);
}

/*******************************************************************************
* Function Name  : STC3115_AlarmSet
* Description    : Set the alarm function
* Input          : None
* Return         : error status (OK, !OK)
*******************************************************************************/
int STC3115_AlarmSet(void)
{
	int res;

	/* Read the mode register */
	res = STC3115_ReadByte(STC3115_REG_MODE);
	/* Set the ALM_ENA bit to 1 */
	res = STC3115_WriteByte(STC3115_REG_MODE, (res | STC3115_ALM_ENA));
	if (res!= OK) return (res);

	return (OK);
}

/*******************************************************************************
* Function Name  : STC3115_AlarmStop
* Description    : Stop the alarm function
* Input          : None
* Return         : error status (OK, !OK)
*******************************************************************************/
int STC3115_AlarmStop(void)
{
	int res;
  
	/* Read the mode register */
	res = STC3115_ReadByte(STC3115_REG_MODE);
	/* Set the ALM_ENA bit to 0 */
	res = STC3115_WriteByte(STC3115_REG_MODE, (res & ~STC3115_ALM_ENA));
	if (res!= OK) return (res);

	return (OK);
}

/*******************************************************************************
* Function Name  : STC3115_AlarmGet
* Description    : Return the ALM status
* Input          : None
* Return         : ALM status 00 : no alarm 
*                             01 : SOC alarm
*                             10 : Voltage alarm
*                             11 : SOC and voltage alarm
*******************************************************************************/
int STC3115_AlarmGet(void)
{
	int res;
  
	/* Read the mode register */
	res = STC3115_ReadByte(STC3115_REG_CTRL);
	if(res== -1){return (res);}
	res = res >> 5;

	return (res);
}

/*******************************************************************************
* Function Name  : STC3115_AlarmClear
* Description    : Clear the alarm signal
* Input          : None
* Return         : error status (OK, !OK)
*******************************************************************************/
int STC3115_AlarmClear(void)
{
	int res;
  
	/* Clear ALM bits */
	res = STC3115_WriteByte(STC3115_REG_CTRL, 0x01);
	if(res!= OK){return (res);}

	return (res);
}

/*******************************************************************************
* Function Name  : STC3115_AlarmSetVoltageThreshold
* Description    : Set the alarm threshold
* Input          : int voltage threshold in mV
* Return         : error status (OK, !OK)
*******************************************************************************/
int STC3115_AlarmSetVoltageThreshold(STC3115_ConfigData_TypeDef *ConfigData, int VoltThresh)
{
	int res;
	int value;
  
	ConfigData->Alm_Vbat = VoltThresh;
	value= ((long)(ConfigData->Alm_Vbat << 9) / VoltageFactor); /* LSB=8*2.2mV */
	res = STC3115_WriteByte(STC3115_REG_ALARM_VOLTAGE, value);

	if(res!= OK){return(res);}

	return (OK);
}

/*******************************************************************************
* Function Name  : STC3115_AlarmSetSOCThreshold
* Description    : Set the alarm threshold
* Input          : int voltage threshold in %
* Return         : error status (OK, !OK)
*******************************************************************************/
int STC3115_AlarmSetSOCThreshold(STC3115_ConfigData_TypeDef *ConfigData, int SOCThresh)
{
	int res;

	ConfigData->Alm_SOC = SOCThresh;
	res = STC3115_WriteByte(STC3115_REG_ALARM_SOC, ConfigData->Alm_SOC*2);

	if(res!= OK){return (res);}
  
	return (OK);
}

/* ---- End of driver interface functions ------------------------------------------ */
/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

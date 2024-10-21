/**
 ***************************************************************************************************************************************
 * @file     bt_hogp.c
 * @owner    SimonBat
 * @version  v0.0.1
 * @date     2020.11.21
 * @update   2020.11.21
 * @brief    sentinel v1.0
 ***************************************************************************************************************************************
 * @attention
 *
 * (Where to use)
 *
 ***************************************************************************************************************************************
 */

#include "bt_hogp.h"
#include "ssd1306.h"
#include "bat.h"
#include "display.h"

/* The following is used as a printf replacement */
#define HOGP_DBG_Display(_x) do{BTPS_OutputMessage _x;} while(0)
#define NUM_SUPPORTED_HCI_VERSIONS (sizeof(HCIVersionStrings)/sizeof(char *) - 1)

static ApplicationStateInfo_t ApplicationStateInfo; /* Container for all of the Application State Information */
static GAPLE_Parameters_t LE_Parameters; /* Holds GAP Parameters like */
static DeviceInfo_t *DeviceInfoList; /* Holds the list head for the */
static Boolean_t SleepEnabledFlag;

/** Discoverability, Connectability Modes.
  * The Encryption Root Key should be generated
  * in such a way as to guarantee 128 bits of
  * entropy.
  */
static BTPSCONST Encryption_Key_t ER = {0x28, 0xBA, 0xE1, 0x35, 0x13, 0xB2, 0x20, 0x45, 0x16, 0xB2, 0x19, 0xD9, 0x80, 0xEE, 0x4A, 0x51};

/** The Identity Root Key should be generated
  * in such a way as to guarantee 128 bits of
  * entropy.
  */
static BTPSCONST Encryption_Key_t IR = {0x41, 0x09, 0xF4, 0x88, 0x09, 0x6B, 0x70, 0xC0, 0x95, 0x23, 0x3C, 0x8C, 0x48, 0xFC, 0xC9, 0xFE};

/** The following keys can be regerenated on the
  * fly using the constant IR and ER keys and
  * are used globally, for all devices.
  */
static Encryption_Key_t DHK;
static Encryption_Key_t IRK;

/** The following string table is used to map HCI Version information
  * to an easily displayable version string.
  */
static BTPSCONST char *HCIVersionStrings[] ={
	"1.0b",
	"1.1",
	"1.2",
	"2.0",
	"2.1",
	"3.0",
	"4.0",
	"4.1",
	"Unknown (greater 4.1)"
};

/** The following table represent the Keyboard Report Descriptor for
  * this HID Keyboard Device.
  */
static Byte_t KeyboardReportDescriptor[]={
	0x05, 0x01,  /* USAGE_PAGE (Generic Desktop)                        */
	0x09, 0x06,  /* USAGE (Keyboard)                                    */
	0xa1, 0x01,  /* COLLECTION (Application)                            */
	0x05, 0x07,  /* USAGE_PAGE (Keyboard)                               */
	0x19, 0xe0,  /* USAGE_MINIMUM (Keyboard LeftControl)                */
	0x29, 0xe7,  /* USAGE_MAXIMUM (Keyboard Right GUI)                  */
	0x15, 0x00,  /* LOGICAL_MINIMUM (0)                                 */
	0x25, 0x01,  /* LOGICAL_MAXIMUM (1)                                 */
	0x75, 0x01,  /* REPORT_SIZE (1)                                     */
	0x95, 0x08,  /* REPORT_COUNT (8)                                    */
	0x81, 0x02,  /* INPUT (Data,Var,Abs)                                */
	0x95, 0x01,  /* REPORT_COUNT (1)                                    */
	0x75, 0x08,  /* REPORT_SIZE (8)                                     */
	0x81, 0x03,  /* INPUT (Cnst,Var,Abs)                                */
	0x95, 0x05,  /* REPORT_COUNT (5)                                    */
	0x75, 0x01,  /* REPORT_SIZE (1)                                     */
	0x05, 0x08,  /* USAGE_PAGE (LEDs)                                   */
	0x19, 0x01,  /* USAGE_MINIMUM (Num Lock)                            */
	0x29, 0x05,  /* USAGE_MAXIMUM (Kana)                                */
	0x91, 0x02,  /* OUTPUT (Data,Var,Abs)                               */
	0x95, 0x01,  /* REPORT_COUNT (1)                                    */
	0x75, 0x03,  /* REPORT_SIZE (3)                                     */
	0x91, 0x03,  /* OUTPUT (Cnst,Var,Abs)                               */
	0x95, 0x06,  /* REPORT_COUNT (6)                                    */
	0x75, 0x08,  /* REPORT_SIZE (8)                                     */
	0x15, 0x00,  /* LOGICAL_MINIMUM (0)                                 */
	0x25, 0x65,  /* LOGICAL_MAXIMUM (101)                               */
	0x05, 0x07,  /* USAGE_PAGE (Keyboard)                               */
	0x19, 0x00,  /* USAGE_MINIMUM (Reserved (no event indicated))       */
	0x29, 0x65,  /* USAGE_MAXIMUM (Keyboard Application)                */
	0x81, 0x00,  /* INPUT (Data,Ary,Abs)                                */
	0xc0         /* END_COLLECTION                                      */
};

static const uint8_t OGP_HID_KEYS[128]={
	0x00, /* NULL */ 	0x00, /* SOH */ 0x00, /* STX */ 0x00, /* ETX */ 0x00, /* EOT */ 0x00, /* ENQ */ 0x00, /* ACK */ 0x00, /* BEL */
	0x00, /* BS */ 		0x2B, /* TAB */ 0x00, /* LF */ 	0x00, /* VT */ 	0x00, /* FF */ 	0x28, /* CR */ 	0x00, /* S0 */ 	0x00, /* SI */
	0x00, /* DLE */ 	0x00, /* DC1 */ 0x00, /* DC2 */ 0x00, /* DC3 */ 0x00, /* DC4 */ 0x00, /* NAK */ 0x00, /* SYN */ 0x00, /* ETB */
	0x00, /* CAN */ 	0x00, /* EM */ 	0x00, /* SUB */ 0x00, /* ESC */ 0x00, /* FS */ 	0x00, /* GS */ 	0x00, /* RS */ 	0x00, /* US */
	0x2C, /* SPACE */ 	0x9E, /* ! */ 	0xB4, /* " */ 	0xA0, /* # */ 	0xA1, /* $ */ 	0xA2, /* % */ 	0xA4, /* & */ 	0x34, /* ' */
	0xA6, /* ( */ 		0xA7, /* ) */ 	0xA5, /* * */ 	0xAE, /* + */ 	0x36, /* , */ 	0x2D, /* - */ 	0x37, /* . */ 	0x38, /* / */
	0x27, /* 0 */ 		0x1E, /* 1 */ 	0x1F, /* 2 */ 	0x20, /* 3 */ 	0x21, /* 4 */ 	0x22, /* 5 */ 	0x23, /* 6 */ 	0x24, /* 7 */
	0x25, /* 8 */ 		0x26, /* 9 */ 	0xB3, /* : */ 	0x33, /* ; */ 	0xB6, /* < */ 	0x2E, /* = */ 	0xB7, /* > */ 	0xB8, /* ? */
	0x9F, /* @ */ 		0x84, /* A */ 	0x85, /* B */ 	0x86, /* C */ 	0x87, /* D */ 	0x88, /* E */ 	0x89, /* F */ 	0x8A, /* G */
	0x8B, /* H */ 		0x8C, /* I */ 	0x8D, /* J */ 	0x8E, /* K */ 	0x8F, /* L */ 	0x90, /* M */ 	0x91, /* N */ 	0x92, /* O */
	0x93, /* P */ 		0x94, /* Q */ 	0x95, /* R */ 	0x96, /* S */ 	0x97, /* T */ 	0x98, /* U */ 	0x99, /* V */ 	0x9A, /* W */
	0x9B, /* X */ 		0x9C, /* Y */ 	0x9D, /* Z */ 	0x2F, /* [ */ 	0x31, /* \ */ 	0x30, /* ] */ 	0xA3, /* ^ */ 	0xAD, /* _ */
	0x35, /* ` */ 		0x04, /* a */ 	0x05, /* b */ 	0x06, /* c */ 	0x07, /* d */ 	0x08, /* e */ 	0x09, /* f */ 	0x0A, /* g */
	0x0B, /* h */ 		0x0C, /* i */ 	0x0D, /* j */ 	0x0E, /* k */ 	0x0F, /* l */ 	0x10, /* m */ 	0x11, /* n */ 	0x12, /* o */
	0x13, /* p */ 		0x14, /* q */ 	0x15, /* r */ 	0x16, /* s */ 	0x17, /* t */ 	0x18, /* u */ 	0x19, /* v */ 	0x1A, /* w */
	0x1B, /* x */ 		0x1C, /* y */ 	0x1D, /* z */ 	0xAF, /* { */ 	0xB1, /* | */ 	0xB0, /* } */ 	0xB5, /* ~ */ 	0x2A  /* DEL */
};

/* Local functions declarations */
static void HOGP_Display_Function_Error(char* Function, int Status);
static int HOGP_Open_Stack(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization);
static int HOGP_Close_Stack(void);
static void HOGP_Free_Device_Info_List(DeviceInfo_t **ListHead);
static void HOGP_BD_ADDR_To_Str(BD_ADDR_t Board_Address, BoardStr_t BoardStr);
static int HOGP_Configure_DIS(void);
static int HOGP_Configure_BAS(void);
static int HOGP_Configure_HIDS(void);
static DeviceInfo_t * HOGP_Search_Device_Info_Entry_By_BD_ADDR(DeviceInfo_t **ListHead, \
															   GAP_LE_Address_Type_t AddressType, BD_ADDR_t BD_ADDR);
static void HOGP_Format_Advertising_Data(unsigned int BluetoothStackID);
static int HOGP_Set_Pairable(void);
static void HOGP_Post_Application_Mailbox(Byte_t MessageID);
static Boolean_t HOGB_Create_New_Device_Info_Entry(DeviceInfo_t **ListHead, \
												   GAP_LE_Address_Type_t ConnectionAddressType, BD_ADDR_t ConnectionBD_ADDR);
static int HOGP_Slave_Security_ReEstablishment(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);
static DeviceInfo_t *HOGP_Delete_Device_Info_Entry(DeviceInfo_t **ListHead, \
												   GAP_LE_Address_Type_t AddressType, BD_ADDR_t BD_ADDR);
static void HOGP_Free_Device_Info_Entry_Memory(DeviceInfo_t *EntryToFree);
static void HOGP_Display_Pairing_Information(GAP_LE_Pairing_Capabilities_t Pairing_Capabilities);
static int HOGP_Slave_Pairing_Request_Response(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);
static void HOGP_Configure_Capabilities(GAP_LE_Pairing_Capabilities_t *Capabilities);
static int HOGP_Encryption_Information_Request_Response(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, Byte_t KeySize, \
														GAP_LE_Authentication_Response_Information_t *GAP_LE_Authentication_Response_Information);
static int HOGP_Disconnect_LE_Device(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);
static void BTPSAPI HOGP_GATT_Connection_Event_Callback(unsigned int BluetoothStackID, \
														GATT_Connection_Event_Data_t *GATT_Connection_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI HOGP_BAS_Event_Callback(unsigned int BluetoothStackID, BAS_Event_Data_t *BAS_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI HOGP_HIDS_Event_Callback(unsigned int BluetoothStackID, HIDS_Event_Data_t *HIDS_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI HOGP_GAP_LE_Event_Callback(unsigned int BluetoothStackID, GAP_LE_Event_Data_t *GAP_LE_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI HOGP_BSC_TimerCallback(unsigned int BluetoothStackID, unsigned int TimerID, unsigned long CallbackParameter);
static void BTPSAPI HOGP_HCI_Sleep_Callback(Boolean_t SleepAllowed, unsigned long CallbackParameter);
static void HOGP_Notify_Battery_Level(ApplicationStateInfo_t *ApplicationStateInfo, Boolean_t Force);
static int HOGP_Start_Advertising(unsigned int BluetoothStackID);
static void HOGP_Notify_Keyboard_Report(ApplicationStateInfo_t *ApplicationStateInfo);

/**
  ***************************************************************************************************************************************
  * @brief Displays a function error
  * @param Pointer to function (char*), Status (int)
  * @retval None
  ***************************************************************************************************************************************
  */
static void HOGP_Display_Function_Error(char* Function, int Status)
{
	HOGP_DBG_Display(("%s Failed: %d.\r\n", Function, Status));
}

/**
  ***************************************************************************************************************************************
  * The following function is used to initialize the application
  * instance.  This function should open the stack and prepare to
  * execute commands based on user input.  The first parameter passed
  * to this function is the HCI Driver Information that will be used
  * when opening the stack and the second parameter is used to pass
  * parameters to BTPS_Init.  This function returns the
  * BluetoothStackID returned from BSC_Initialize on success or a
  * negative error code (of the form APPLICATION_ERROR_XXX).
  ***************************************************************************************************************************************
  */
int HOGP_Application_Init(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization)
{
	int ret_val = HOGP_APPLICATION_ERROR_UNABLE_TO_OPEN_STACK;
	HCI_HCILLConfiguration_t HCILLConfig;
	HCI_Driver_Reconfigure_Data_t DriverReconfigureData;

	/* Next, makes sure that the Driver Information passed appears to be semi-valid. */
	if((HCI_DriverInformation) && (BTPS_Initialization))
	{
		/* Try to Open the stack and check if it was successful. */
		if(!HOGP_Open_Stack(HCI_DriverInformation, BTPS_Initialization))
		{
			/* Now that the device is discoverable attempt to make it pairable */
			ret_val = HOGP_Set_Pairable();
			if(!ret_val)
			{
				/* Create the Application Mailbox */
				if((ApplicationStateInfo.Mailbox = BTPS_CreateMailbox(APPLICATION_MAILBOX_DEPTH, APPLICATION_MAILBOX_SIZE)) != NULL)
				{
					/* Post some messages to the application to kick start the application */
					HOGP_Post_Application_Mailbox(APPLICATION_MAILBOX_MESSAGE_ID_LE_DISCONNECTED);

					/* Register a sleep mode callback if we are using HCILL Mode */
					if((HCI_DriverInformation->DriverInformation.COMMDriverInformation.Protocol == cpHCILL)|| \
					   (HCI_DriverInformation->DriverInformation.COMMDriverInformation.Protocol == cpHCILL_RTS_CTS))
					{
						HCILLConfig.SleepCallbackFunction        = HOGP_HCI_Sleep_Callback;
						HCILLConfig.SleepCallbackParameter       = 0;
						DriverReconfigureData.ReconfigureCommand = HCI_COMM_DRIVER_RECONFIGURE_DATA_COMMAND_CHANGE_HCILL_PARAMETERS;
						DriverReconfigureData.ReconfigureData    = (void *)&HCILLConfig;

						/** Register the sleep mode callback.  Note that if
						  * this function returns greater than 0 then sleep is
						  * currently enabled.
						  */
						ret_val = HCI_Reconfigure_Driver(ApplicationStateInfo.BluetoothStackID, FALSE, &DriverReconfigureData);
						if(ret_val > 0)
						{
							/* Flag that sleep mode is enabled */
							SleepEnabledFlag = TRUE;
						}
					}

					/* Return success to the caller */
					ret_val = (int)ApplicationStateInfo.BluetoothStackID;
				}
				else
				{
					HOGP_DBG_Display(("Failed to create application mailbox.\r\n"));
					ret_val = HOGP_UNABLE_TO_INITIALIZE_STACK;
				}
			}
			else{HOGP_Display_Function_Error("SetPairable", ret_val);}

			/* In some error occurred then close the stack */
			if(ret_val < 0)
			{
				/* Close the Bluetooth Stack */
				HOGP_Close_Stack();
			}
		}
		else
		{
			/* There was an error while attempting to open the Stack */
			HOGP_DBG_Display(("Unable to open the stack.\r\n"));
		}
	}
	else{ret_val = HOGP_APPLICATION_ERROR_INVALID_PARAMETERS;}

	return(ret_val);
}

/**
  ***************************************************************************************************************************************
  * The following function is responsible for opening the SS1
  * Bluetooth Protocol Stack.  This function accepts a pre-populated
  * HCI Driver Information structure that contains the HCI Driver
  * Transport Information.  This function returns zero on successful
  * execution and a negative value on all errors.
  ***************************************************************************************************************************************
  */
static int HOGP_Open_Stack(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization)
{
	int Result;
	int ret_val = 0;
	char BluetoothAddress[16];
	Byte_t Status;
	BD_ADDR_t BD_ADDR;
	unsigned int ServiceID;
	HCI_Version_t HCIVersion;
	L2CA_Link_Connect_Params_t L2CA_Link_Connect_Params;

	/* Next, makes sure that the Driver Information passed appears to be semi-valid */
	if(HCI_DriverInformation)
	{
		HOGP_DBG_Display(("\r\n"));
		/* Initialize BTPSKNRl */
		BTPS_Init((void *)BTPS_Initialization);
		HOGP_DBG_Display(("OpenStack().\r\n"));
		/* Clear the application state information */
		BTPS_MemInitialize(&ApplicationStateInfo, 0, sizeof(ApplicationStateInfo));
		/* Initialize the Stack */
		Result = BSC_Initialize(HCI_DriverInformation, 0);

		/* Next, check the return value of the initialization to see if it was successful */
		if(Result > 0)
		{
			/** The Stack was initialized successfully, inform the user and
			  * set the return value of the initialization function to the
			  * Bluetooth Stack ID.
			  */
			ApplicationStateInfo.BluetoothStackID = Result;
			HOGP_DBG_Display(("Bluetooth Stack ID: %d.\r\n", ApplicationStateInfo.BluetoothStackID));

			/* Initialize the Default Pairing Parameters */
			LE_Parameters.IOCapability   = licNoInputNoOutput;
			LE_Parameters.MITMProtection = FALSE;
			LE_Parameters.OOBDataPresent = FALSE;

			if(!HCI_Version_Supported(ApplicationStateInfo.BluetoothStackID, &HCIVersion))
			{HOGP_DBG_Display(("Device Chipset: %s.\r\n", (HCIVersion <= NUM_SUPPORTED_HCI_VERSIONS)? \
							   HCIVersionStrings[HCIVersion]:HCIVersionStrings[NUM_SUPPORTED_HCI_VERSIONS]));}

			/* Let's output the Bluetooth Device Address so that the user knows what the Device Address is */
			if(!GAP_Query_Local_BD_ADDR(ApplicationStateInfo.BluetoothStackID, &BD_ADDR))
			{
				HOGP_BD_ADDR_To_Str(BD_ADDR, BluetoothAddress);
				HOGP_DBG_Display(("BD_ADDR: %s\r\n", BluetoothAddress));
			}

			/* Go ahead and allow Master/Slave Role Switch */
			L2CA_Link_Connect_Params.L2CA_Link_Connect_Request_Config  = cqAllowRoleSwitch;
			L2CA_Link_Connect_Params.L2CA_Link_Connect_Response_Config = csMaintainCurrentRole;

			L2CA_Set_Link_Connection_Configuration(ApplicationStateInfo.BluetoothStackID, &L2CA_Link_Connect_Params);

			if(HCI_Command_Supported(ApplicationStateInfo.BluetoothStackID, HCI_SUPPORTED_COMMAND_WRITE_DEFAULT_LINK_POLICY_BIT_NUMBER) > 0)
			{HCI_Write_Default_Link_Policy_Settings(ApplicationStateInfo.BluetoothStackID, \
												   (HCI_LINK_POLICY_SETTINGS_ENABLE_MASTER_SLAVE_SWITCH|HCI_LINK_POLICY_SETTINGS_ENABLE_SNIFF_MODE), &Status);}

			/* Regenerate IRK and DHK from the constant Identity Root Key */
			GAP_LE_Diversify_Function(ApplicationStateInfo.BluetoothStackID, (Encryption_Key_t *)(&IR), 1,0, &IRK);
			GAP_LE_Diversify_Function(ApplicationStateInfo.BluetoothStackID, (Encryption_Key_t *)(&IR), 3, 0, &DHK);

			/* Initialize the GATT Service */
			if(!(Result = GATT_Initialize(ApplicationStateInfo.BluetoothStackID, GATT_INITIALIZATION_FLAGS_SUPPORT_LE, HOGP_GATT_Connection_Event_Callback, 0)))
			{
				/* Initialize the GAPS Service */
				Result = GAPS_Initialize_Service(ApplicationStateInfo.BluetoothStackID, &ServiceID);
				if(Result > 0)
				{
					/* Save the Instance ID of the GAP Service */
					ApplicationStateInfo.GAPSInstanceID = (unsigned int)Result;
					/* Set the GAP Device Name and Device Appearance */
					GAPS_Set_Device_Name(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.GAPSInstanceID, LE_DEMO_DEVICE_NAME);
					GAPS_Set_Device_Appearance(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.GAPSInstanceID, GAP_DEVICE_APPEARENCE_VALUE_HID_KEYBOARD);

					/* Atempt to configure the DIS Service Instance */
					Result = HOGP_Configure_DIS();
					if(!Result)
					{
						/* Atempt to configure the BAS Service Instance */
						Result = HOGP_Configure_BAS();
						if(!Result)
						{
							/* Atempt to configure the HID Service Instance */
							Result = HOGP_Configure_HIDS();
							if(!Result)
							{
								/* Format the Advertising Data */
								HOGP_Format_Advertising_Data(ApplicationStateInfo.BluetoothStackID);
								/* Reset the HID Protocol Mode to the default mode (Report Mode) */
								ApplicationStateInfo.HIDProtocolMode = pmReport;
								/* Configure the Battery Level to be at 100% */
								ApplicationStateInfo.BatteryLevel = BAT_Get_SOC();
								/* Return success to the caller */
								ret_val = 0;
							}
							else
							{
								/** The Stack was NOT initialized successfully,
								  * inform the user and set the return value of
								  * the initialization function to an error.
								  */
								HOGP_Display_Function_Error("ConfigureHIDS", Result);
								ret_val = HOGP_UNABLE_TO_INITIALIZE_STACK;
							}
						}
						else
						{
							/** The Stack was NOT initialized successfully,
							  * inform the user and set the return value of the
							  * initialization function to an error.
							  */
							HOGP_Display_Function_Error("ConfigureBAS", Result);
							ret_val = HOGP_UNABLE_TO_INITIALIZE_STACK;
						}
					}
					else
					{
						/** The Stack was NOT initialized successfully, inform
						  * the user and set the return value of the
						  * initialization function to an error.
						  */
						HOGP_Display_Function_Error("ConfigureDIS", Result);
						ret_val = HOGP_UNABLE_TO_INITIALIZE_STACK;
					}
				}
				else
				{
					/** The Stack was NOT initialized successfully, inform the
					  * user and set the return value of the initialization
					  * function to an error.
					  */
					HOGP_Display_Function_Error("GAPS_Initialize_Service", Result);
					ret_val = HOGP_UNABLE_TO_INITIALIZE_STACK;
				}

				/* Shutdown the stack if an error occurred */
				if(ret_val < 0){HOGP_Close_Stack();}
			}
			else
			{
				/** The Stack was NOT initialized successfully, inform the
				  * user and set the return value of the initialization
				  * function to an error.
				  */
				HOGP_Display_Function_Error("GATT_Initialize", Result);
				/* Shutdown the stack */
				HOGP_Close_Stack();
				ret_val = HOGP_UNABLE_TO_INITIALIZE_STACK;
			}
		}
		else
		{
			/** The Stack was NOT initialized successfully, inform the user
			  * and set the return value of the initialization function to
			  * an error.
			  */
			HOGP_Display_Function_Error("BSC_Initialize", Result);
			ApplicationStateInfo.BluetoothStackID = 0;
			ret_val = HOGP_UNABLE_TO_INITIALIZE_STACK;
		}
	}
	else
	{
		/* One or more of the necessary parameters are invalid */
		ret_val = HOGP_INVALID_PARAMETERS_ERROR;
	}

	return(ret_val);
}

/**
  ***************************************************************************************************************************************
  * The following function is responsible for closing the SS1
  * Bluetooth Protocol Stack.  This function requires that the
  * Bluetooth Protocol stack previously have been initialized via the
  * OpenStack() function.  This function returns zero on successful
  * execution and a negative value on all errors.
  ***************************************************************************************************************************************
  */
static int HOGP_Close_Stack(void)
{
	int ret_val = 0;

	/* First check to see if the Stack has been opened */
	if(ApplicationStateInfo.BluetoothStackID)
	{
		/* Cleanup GAP Service Module */
		if(ApplicationStateInfo.GAPSInstanceID)
		{GAPS_Cleanup_Service(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.GAPSInstanceID);}

		/* Cleanup DIS Service Module */
		if(ApplicationStateInfo.DISInstanceID)
		{DIS_Cleanup_Service(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.DISInstanceID);}

		/* Cleanup BAS Service Module */
		if(ApplicationStateInfo.BASInstanceID)
		{BAS_Cleanup_Service(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.BASInstanceID);}

		if(ApplicationStateInfo.SPPServerPortID)
		{
			SPP_Un_Register_SDP_Record(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.SPPServerPortID, ApplicationStateInfo.SPPServerSDPHandle);
			SPP_Close_Server_Port(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.SPPServerPortID);
		}

		/* Free the Device Information List */
		HOGP_Free_Device_Info_List(&DeviceInfoList);
		/* Cleanup GATT Module */
		GATT_Cleanup(ApplicationStateInfo.BluetoothStackID);
		/* Simply close the Stack */
		BSC_Shutdown(ApplicationStateInfo.BluetoothStackID);
		/* Free BTPSKRNL allocated memory */
		BTPS_DeInit();
		HOGP_DBG_Display(("Stack Shutdown.\r\n"));
		/* Flag that the Stack is no longer initialized */
		BTPS_MemInitialize(&ApplicationStateInfo, 0, sizeof(ApplicationStateInfo));
		/* Flag success to the caller */
		ret_val = 0;
	}
	else
	{
		/* A valid Stack ID does not exist, inform to user */
		ret_val = HOGP_UNABLE_TO_INITIALIZE_STACK;
	}

	return(ret_val);
}

/**
  ***************************************************************************************************************************************
  * The following function deletes (and free's all memory) every
  * element of the specified Key Info List. Upon return of this
  * function, the Head Pointer is set to NULL.
  ***************************************************************************************************************************************
  */
static void HOGP_Free_Device_Info_List(DeviceInfo_t **ListHead)
{
	BSC_FreeGenericListEntryList((void **)(ListHead), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoPtr));
}

/**
  ***************************************************************************************************************************************
  * The following function is responsible for converting data of type
  * BD_ADDR to a string.  The first parameter of this function is the
  * BD_ADDR to be converted to a string.  The second parameter of this
  * function is a pointer to the string in which the converted BD_ADDR
  * is to be stored.
  ***************************************************************************************************************************************
  */
static void HOGP_BD_ADDR_To_Str(BD_ADDR_t Board_Address, BoardStr_t BoardStr)
{
	BTPS_SprintF((char *)BoardStr, "0x%02X%02X%02X%02X%02X%02X", Board_Address.BD_ADDR5, \
		Board_Address.BD_ADDR4, \
		Board_Address.BD_ADDR3, \
		Board_Address.BD_ADDR2, \
		Board_Address.BD_ADDR1, \
		Board_Address.BD_ADDR0);
}

/**
  ***************************************************************************************************************************************
  * The following function is a utility function that is used to
  * configure the Device Information Service that is registered by
  * this application. This function returns ZERO if succesful or a
  * negative error code.
  ***************************************************************************************************************************************
  */
static int HOGP_Configure_DIS(void)
{
	int ret_val;
	unsigned int ServiceID;
	DIS_PNP_ID_Data_t PNP_ID;

	/* Initialize the DIS Service */
	ret_val = DIS_Initialize_Service(ApplicationStateInfo.BluetoothStackID, &ServiceID);
	if(ret_val > 0)
	{
		HOGP_DBG_Display(("Device Information Service registered, Service ID = %u.\r\n", ServiceID));
		/* Save the Instance ID of the DIS Service */
		ApplicationStateInfo.DISInstanceID = (unsigned int)ret_val;

		/* Set the DIS Manufacturer Name */
		if(!(ret_val = DIS_Set_Manufacturer_Name(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.DISInstanceID, BTPS_VERSION_PRODUCT_NAME_STRING)))
		{
			/* Set the DIS Software Revision */
			if(!(ret_val = DIS_Set_Software_Revision(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.DISInstanceID, BTPS_VERSION_VERSION_STRING)))
			{
				/* Configure the PNP ID value to use */
				PNP_ID.VendorID_Source = DIS_PNP_ID_VENDOR_SOURCE_BLUETOOTH_SIG;
				PNP_ID.VendorID        = PNP_ID_VENDOR_ID_STONESTREET_ONE;
				PNP_ID.ProductID       = PNP_ID_PRODUCT_ID;
				PNP_ID.ProductVersion  = PNP_ID_PRODUCT_VERSION;

				/* Finally set the PNP ID */
				ret_val = DIS_Set_PNP_ID(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.DISInstanceID, &PNP_ID);
			}
		}
	}
	else{HOGP_DBG_Display(("Error - DIS_Initialize_Service() %d.\r\n", ret_val));}

	return(ret_val);
}

/**
  ***************************************************************************************************************************************
  * The following function is a utility function that is used to
  * configure the Battery Service that is registered by this
  * application.  This function returns ZERO if succesful or a
  * negative error code.
  ***************************************************************************************************************************************
  */
static int HOGP_Configure_BAS(void)
{
	int ret_val;
	unsigned int ServiceID;

	/* Initialize the BAS Service */
	ret_val = BAS_Initialize_Service(ApplicationStateInfo.BluetoothStackID, HOGP_BAS_Event_Callback, 0, &ServiceID);
	if(ret_val > 0)
	{
		HOGP_DBG_Display(("Battery Service registered, Service ID = %u.\r\n", ServiceID));
		/* Save the Instance ID of the BAS Service */
		ApplicationStateInfo.BASInstanceID = (unsigned int)ret_val;
		ret_val = 0;
	}
	else{HOGP_DBG_Display(("Error - BAS_Initialize_Service() %d.\r\n", ret_val));}

	return(ret_val);
}

/**
  ***************************************************************************************************************************************
  * The following function is a utility function that is used to
  * configure the HID Service that is registered by this application.
  * This function returns ZERO if succesful or a negative error code.
  ***************************************************************************************************************************************
  */
static int HOGP_Configure_HIDS(void)
{
	int ret_val;
	unsigned int ServiceID;
	HIDS_HID_Information_Data_t HIDInformation;
	HIDS_Report_Reference_Data_t ReportReferenceData[2];

	/* Configure the HID Information value. */
	HIDInformation.CountryCode = HIDS_HID_LOCALIZATION_BYTE_NO_LOCALIZATION;
	HIDInformation.Flags = HIDS_HID_INFORMATION_FLAGS_NORMALLY_CONNECTABLE;
	HIDInformation.Version = HIDS_HID_VERSION_NUMBER;

	/** Configure the Report Reference structures.  Note that since we
	  * have only 1 report of a type (Input,Output,Feature) we do not need
	  * to have a unique Reference ID and therefore we use a Report ID of
	  * ZERO.
	  */
	ReportReferenceData[0].ReportID   = 0;
	ReportReferenceData[0].ReportType = HIDS_REPORT_REFERENCE_REPORT_TYPE_INPUT_REPORT;
	ReportReferenceData[1].ReportID   = 0;
	ReportReferenceData[1].ReportType = HIDS_REPORT_REFERENCE_REPORT_TYPE_OUTPUT_REPORT;

	/* Initialize the HID Service. */
	ret_val = HIDS_Initialize_Service(ApplicationStateInfo.BluetoothStackID, HIDS_FLAGS_SUPPORT_KEYBOARD, \
									  &HIDInformation, 0, NULL, 0, NULL, (sizeof(ReportReferenceData)/sizeof(HIDS_Report_Reference_Data_t)), \
									  ReportReferenceData, HOGP_HIDS_Event_Callback, 0, &ServiceID);
	if(ret_val > 0)
	{
		HOGP_DBG_Display(("HID Service registered, Service ID = %u.\r\n", ServiceID));
		/* Save the Instance ID of the HID Service */
		ApplicationStateInfo.HIDSInstanceID = (unsigned int)ret_val;
		ret_val = 0;
	}
	else{HOGP_DBG_Display(("Error - HIDS_Initialize_Service() %d.\r\n", ret_val));}

	return(ret_val);
}

/**
  ***************************************************************************************************************************************
  * The following function searches the specified List for the
  * specified Connection BD_ADDR.  This function returns NULL if
  * either the List Head is invalid, the BD_ADDR is invalid, or the
  * Connection BD_ADDR was NOT found.
  ***************************************************************************************************************************************
  */
static DeviceInfo_t * HOGP_Search_Device_Info_Entry_By_BD_ADDR(DeviceInfo_t **ListHead, GAP_LE_Address_Type_t AddressType, BD_ADDR_t BD_ADDR)
{
	DeviceInfo_t *DeviceInfo;

	/* Verify that the input parameters are semi-valid. */
	if((ListHead) && (!COMPARE_NULL_BD_ADDR(BD_ADDR)))
	{
		/* Check to see if this is a resolvable address type.  If so we will search the list based on the IRK. */
		if((AddressType == latRandom) && (GAP_LE_TEST_RESOLVABLE_ADDRESS_BITS(BD_ADDR)))
		{
			/* Walk the list and attempt to resolve this entry to an existing entry with IRK. */
			DeviceInfo = *ListHead;
			while(DeviceInfo)
			{
				/* Check to see if the IRK is valid */
				if(DeviceInfo->Flags & DEVICE_INFO_FLAGS_IRK_VALID)
				{
					/* Attempt to resolve this address with the stored IRK. */
					if(GAP_LE_Resolve_Address(ApplicationStateInfo.BluetoothStackID, &(DeviceInfo->IRK), BD_ADDR))
					{
						/* Address resolved so just exit from the loop. */
						break;
					}
				}

				DeviceInfo = DeviceInfo->NextDeviceInfoPtr;
			}
		}
		else{DeviceInfo = NULL;}

		/* If all else fail we will attempt to search the list by just the BD_ADDR */
		if(DeviceInfo == NULL)
		{DeviceInfo = BSC_SearchGenericListEntry(ekBD_ADDR_t, (void *)(&BD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, BD_ADDR), \
												 BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoPtr), (void **)(ListHead));}
	}
	else{DeviceInfo = NULL;}

	return(DeviceInfo);
}

/**
  ***************************************************************************************************************************************
  * The following function is a utility function that formats the advertising data.
  ***************************************************************************************************************************************
  */
static void HOGP_Format_Advertising_Data(unsigned int BluetoothStackID)
{
	int Result;
	unsigned int Length;
	unsigned int StringLength;
	union{
		Advertising_Data_t AdvertisingData;
		Scan_Response_Data_t ScanResponseData;
	}Advertisement_Data_Buffer;

	/* First, check that valid Bluetooth Stack ID exists. */
	if(BluetoothStackID)
	{
		BTPS_MemInitialize(&(Advertisement_Data_Buffer.AdvertisingData), 0, sizeof(Advertising_Data_t));

		Length = 0;
		/* Set the Flags A/D Field (1 byte type and 1 byte Flags. */
		Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[0]  = 2;
		Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[1]  = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_FLAGS;
		Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[2]  = HCI_LE_ADVERTISING_FLAGS_GENERAL_DISCOVERABLE_MODE_FLAGS_BIT_MASK;
		Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[2] |= HCI_LE_ADVERTISING_FLAGS_BR_EDR_NOT_SUPPORTED_FLAGS_BIT_MASK;
		Length += Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[0] + 1;
		/* Configure the Device Appearance value. */
		Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length]   = 3;
		Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length+1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_APPEARANCE;
		ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length+2]), GAP_DEVICE_APPEARENCE_VALUE_HID_KEYBOARD);
		Length += Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length] + 1;
		/* Configure the services that we say we support. */
		Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length]   = 1 + (UUID_16_SIZE*3);
		Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length+1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_16_BIT_SERVICE_UUID_COMPLETE;
		HIDS_ASSIGN_HIDS_SERVICE_UUID_16(&(Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length+2]));
		BAS_ASSIGN_BAS_SERVICE_UUID_16(&(Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length+4]));
		DIS_ASSIGN_DIS_SERVICE_UUID_16(&(Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length+6]));
		Length += Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length] + 1;

		/* Set the Device Name String. */
		StringLength = BTPS_StringLength(LE_DEMO_DEVICE_NAME);
		if(StringLength < (ADVERTISING_DATA_MAXIMUM_SIZE - Length - 2))
		{Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length+1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_LOCAL_NAME_COMPLETE;}
		else
		{
			Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length+1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_LOCAL_NAME_SHORTENED;
			StringLength = (ADVERTISING_DATA_MAXIMUM_SIZE - Length - 2);
		}

		Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length] = StringLength+1;
		BTPS_MemCopy(&(Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length+2]), LE_DEMO_DEVICE_NAME, StringLength);
		Length += Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length] + 1;
		/* Write thee advertising data to the chip. */
		Result = GAP_LE_Set_Advertising_Data(BluetoothStackID, Length, &(Advertisement_Data_Buffer.AdvertisingData));

		if(!Result){HOGP_DBG_Display(("Advertising Data Configured Successfully.\r\n"));}
		else{HOGP_DBG_Display(("GAP_LE_Set_Advertising_Data(dtAdvertising) returned %d.\r\n", Result));}
	}
}

/**
  ***************************************************************************************************************************************
  * The following function is responsible for placing the local
  * Bluetooth device into Pairable mode.  Once in this mode the device
  * will response to pairing requests from other Bluetooth devices.
  * This function returns zero on successful execution and a negative
  * value on all errors.
  ***************************************************************************************************************************************
  */
static int HOGP_Set_Pairable(void)
{
	int Result;
	int ret_val = 0;

	/* First, check that a valid Bluetooth Stack ID exists. */
	if(ApplicationStateInfo.BluetoothStackID)
	{
		/* Set the LE Pairability Mode. */
		/* Attempt to set the attached device to be pairable. */
		Result = GAP_LE_Set_Pairability_Mode(ApplicationStateInfo.BluetoothStackID, lpmPairableMode);

		/* Next, check the return value of the GAP Set Pairability mode command for successful execution */
		if(!Result)
		{
			/** The device has been set to pairable mode, now register an
			  * Authentication Callback to handle the Authentication events
			  * if required.
			  */
			Result = GAP_LE_Register_Remote_Authentication(ApplicationStateInfo.BluetoothStackID, HOGP_GAP_LE_Event_Callback, (unsigned long)0);

			/* Next, check the return value of the GAP Register Remote Authentication command for successful execution. */
			if(Result)
			{
				/* An error occurred while trying to execute this function. */
				HOGP_Display_Function_Error("GAP_LE_Register_Remote_Authentication", Result);
				ret_val = Result;
			}
		}
		else
		{
			/* An error occurred while trying to make the device pairable. */
			HOGP_Display_Function_Error("GAP_LE_Set_Pairability_Mode", Result);
			ret_val = Result;
		}
	}
	else
	{
		/* No valid Bluetooth Stack ID exists. */
		ret_val = HOGP_INVALID_STACK_ID_ERROR;
	}

	return(ret_val);
}

/**
  ***************************************************************************************************************************************
  * The following function is a utility function that is provided to
  * allow a mechanism of posting into the application mailbox.
  ***************************************************************************************************************************************
  */
static void HOGP_Post_Application_Mailbox(Byte_t MessageID)
{
	/* Post to the application mailbox. */
	BTPS_AddMailbox(ApplicationStateInfo.Mailbox, (void *)&MessageID);
}

/**
  ***************************************************************************************************************************************
  * The following function adds the specified Entry to the specified
  * List.  This function allocates and adds an entry to the list that
  * has the same attributes as parameters to this function.  This
  * function will return FALSE if NO Entry was added.  This can occur
  * if the element passed in was deemed invalid or the actual List
  * Head was invalid.
  * ** NOTE ** This function does not insert duplicate entries into
  *            the list.  An element is considered a duplicate if the
  *            Connection BD_ADDR.  When this occurs, this function
  *            returns NULL.
  ***************************************************************************************************************************************
  */
static Boolean_t HOGB_Create_New_Device_Info_Entry(DeviceInfo_t **ListHead, GAP_LE_Address_Type_t ConnectionAddressType, BD_ADDR_t ConnectionBD_ADDR)
{
	Boolean_t     ret_val = FALSE;
	DeviceInfo_t *DeviceInfoPtr;

	/* Verify that the passed in parameters seem semi-valid. */
	if((ListHead) && (!COMPARE_NULL_BD_ADDR(ConnectionBD_ADDR)))
	{
		/* Allocate the memory for the entry. */
		if((DeviceInfoPtr = BTPS_AllocateMemory(sizeof(DeviceInfo_t))) != NULL)
		{
			/* Initialize the entry. */
			BTPS_MemInitialize(DeviceInfoPtr, 0, sizeof(DeviceInfo_t));
			DeviceInfoPtr->AddressType = ConnectionAddressType;
			DeviceInfoPtr->BD_ADDR     = ConnectionBD_ADDR;

			ret_val = BSC_AddGenericListEntry_Actual(ekBD_ADDR_t, BTPS_STRUCTURE_OFFSET(DeviceInfo_t, BD_ADDR), \
													 BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoPtr), (void **)(ListHead), (void *)(DeviceInfoPtr));
			if(!ret_val)
			{
				/* Failed to add to list so we should free the memory that we allocated for the entry. */
				BTPS_FreeMemory(DeviceInfoPtr);
			}
		}
	}

	return(ret_val);
}

/**
  ***************************************************************************************************************************************
  * The following function provides a mechanism of attempting to
  * re-established security with a previously paired master.
  ***************************************************************************************************************************************
  */
static int HOGP_Slave_Security_ReEstablishment(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR)
{
	int ret_val;
	GAP_LE_Security_Information_t SecurityInformation;

	/* Make sure a Bluetooth Stack is open. */
	if(BluetoothStackID)
	{
		/* Configure the Security Information. */
		SecurityInformation.Local_Device_Is_Master = FALSE;
		SecurityInformation.Security_Information.Slave_Information.Bonding_Type = lbtBonding;
		SecurityInformation.Security_Information.Slave_Information.MITM = LE_Parameters.MITMProtection;
		/* Attempt to pair to the remote device */
		ret_val = GAP_LE_Reestablish_Security(BluetoothStackID, BD_ADDR, &SecurityInformation, HOGP_GAP_LE_Event_Callback, 0);

		if(!ret_val){HOGP_DBG_Display(("GAP_LE_Reestablish_Security sucess.\r\n"));}
		else{HOGP_DBG_Display(("Error - GAP_LE_Reestablish_Security returned %d.\r\n", ret_val));}
	}
	else{ret_val = HOGP_INVALID_STACK_ID_ERROR;}

	return(ret_val);
}

/**
  ***************************************************************************************************************************************
  * The following function searches the specified Key Info List for
  * the specified BD_ADDR and removes it from the List.  This function
  * returns NULL if either the List Head is invalid, the BD_ADDR is
  * invalid, or the specified Entry was NOT present in the list.  The
  * entry returned will have the Next Entry field set to NULL, and
  * the caller is responsible for deleting the memory associated with
  * this entry by calling the FreeKeyEntryMemory() function.
  ***************************************************************************************************************************************
  */
static DeviceInfo_t *HOGP_Delete_Device_Info_Entry(DeviceInfo_t **ListHead, GAP_LE_Address_Type_t AddressType, BD_ADDR_t BD_ADDR)
{
	DeviceInfo_t *LastEntry;
	DeviceInfo_t *DeviceInfo;

	if((ListHead) && (!COMPARE_NULL_BD_ADDR(BD_ADDR)))
	{
		/* Check to see if this is a resolvable address type. If so we will search the list based on the IRK */
		if((AddressType == latRandom) && (GAP_LE_TEST_RESOLVABLE_ADDRESS_BITS(BD_ADDR)))
		{
			/* Now, let's search the list until we find the correct entry. */
			DeviceInfo = *ListHead;
			while((DeviceInfo) && ((!(DeviceInfo->Flags & DEVICE_INFO_FLAGS_IRK_VALID)) || \
				  (!GAP_LE_Resolve_Address(ApplicationStateInfo.BluetoothStackID, &(DeviceInfo->IRK), BD_ADDR))))
			{
				LastEntry  = DeviceInfo;
				DeviceInfo = DeviceInfo->NextDeviceInfoPtr;
			}

			/* Check to see if we found the specified entry. */
			if(DeviceInfo)
			{
				/* OK, now let's remove the entry from the list.  We have to check to see if the entry was the first entry in the list. */
				if(LastEntry)
				{
					/* Entry was NOT the first entry in the list. */
					LastEntry->NextDeviceInfoPtr = DeviceInfo->NextDeviceInfoPtr;
				}
				else{*ListHead = DeviceInfo->NextDeviceInfoPtr;}

				DeviceInfo->NextDeviceInfoPtr = NULL;
			}
		}
		else{DeviceInfo = NULL;}

		/* If all else fail we will attempt to search the list by just the BD_ADDR. */
		if(DeviceInfo == NULL)
		{DeviceInfo = BSC_DeleteGenericListEntry(ekBD_ADDR_t, (void *)(&BD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, BD_ADDR), \
												 BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoPtr), (void **)(ListHead));}
	}
	else{DeviceInfo = NULL;}

	return(DeviceInfo);
}

/**
  ***************************************************************************************************************************************
  * This function frees the specified Key Info Information member memory.
  ***************************************************************************************************************************************
  */
static void HOGP_Free_Device_Info_Entry_Memory(DeviceInfo_t *EntryToFree)
{
	BSC_FreeGenericListEntryMemory((void *)(EntryToFree));
}

/**
  ***************************************************************************************************************************************
  * The following function displays the pairing capabalities that is passed into this function.
  ***************************************************************************************************************************************
  */
static void HOGP_Display_Pairing_Information(GAP_LE_Pairing_Capabilities_t Pairing_Capabilities)
{
	/* Display the IO Capability. */
	switch(Pairing_Capabilities.IO_Capability)
	{
		case licDisplayOnly:HOGP_DBG_Display(("   IO Capability:       lcDisplayOnly.\r\n"));break;
		case licDisplayYesNo:HOGP_DBG_Display(("   IO Capability:       lcDisplayYesNo.\r\n"));break;
		case licKeyboardOnly:HOGP_DBG_Display(("   IO Capability:       lcKeyboardOnly.\r\n"));break;
		case licNoInputNoOutput:HOGP_DBG_Display(("   IO Capability:       lcNoInputNoOutput.\r\n"));break;
		case licKeyboardDisplay:HOGP_DBG_Display(("   IO Capability:       lcKeyboardDisplay.\r\n"));break;
	}

	HOGP_DBG_Display(("   MITM:                %s.\r\n", (Pairing_Capabilities.MITM == TRUE)?"TRUE":"FALSE"));
	HOGP_DBG_Display(("   Bonding Type:        %s.\r\n", (Pairing_Capabilities.Bonding_Type == lbtBonding)?"Bonding":"No Bonding"));
	HOGP_DBG_Display(("   OOB:                 %s.\r\n", (Pairing_Capabilities.OOB_Present == TRUE)?"OOB":"OOB Not Present"));
	HOGP_DBG_Display(("   Encryption Key Size: %d.\r\n", Pairing_Capabilities.Maximum_Encryption_Key_Size));
	HOGP_DBG_Display(("   Sending Keys: \r\n"));
	HOGP_DBG_Display(("      LTK:              %s.\r\n", ((Pairing_Capabilities.Sending_Keys.Encryption_Key == TRUE)?"YES":"NO")));
	HOGP_DBG_Display(("      IRK:              %s.\r\n", ((Pairing_Capabilities.Sending_Keys.Identification_Key == TRUE)?"YES":"NO")));
	HOGP_DBG_Display(("      CSRK:             %s.\r\n", ((Pairing_Capabilities.Sending_Keys.Signing_Key == TRUE)?"YES":"NO")));
	HOGP_DBG_Display(("   Receiving Keys: \r\n"));
	HOGP_DBG_Display(("      LTK:              %s.\r\n", ((Pairing_Capabilities.Receiving_Keys.Encryption_Key == TRUE)?"YES":"NO")));
	HOGP_DBG_Display(("      IRK:              %s.\r\n", ((Pairing_Capabilities.Receiving_Keys.Identification_Key == TRUE)?"YES":"NO")));
	HOGP_DBG_Display(("      CSRK:             %s.\r\n", ((Pairing_Capabilities.Receiving_Keys.Signing_Key == TRUE)?"YES":"NO")));
}

/**
  ***************************************************************************************************************************************
  * The following function provides a mechanism of sending a Slave Pairing Response to a Master's Pairing Request.
  ***************************************************************************************************************************************
  */
static int HOGP_Slave_Pairing_Request_Response(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR)
{
	int ret_val;
	BoardStr_t BoardStr;
	GAP_LE_Authentication_Response_Information_t AuthenticationResponseData;

	/* Make sure a Bluetooth Stack is open. */
	if(BluetoothStackID)
	{
		HOGP_BD_ADDR_To_Str(BD_ADDR, BoardStr);
		HOGP_DBG_Display(("Sending Pairing Response to %s.\r\n", BoardStr));

		/* We must be the slave if we have received a Pairing Request thus we will respond with our capabilities. */
		AuthenticationResponseData.GAP_LE_Authentication_Type = larPairingCapabilities;
		AuthenticationResponseData.Authentication_Data_Length = GAP_LE_PAIRING_CAPABILITIES_SIZE;
		/* Configure the Application Pairing Parameters. */
		HOGP_Configure_Capabilities(&(AuthenticationResponseData.Authentication_Data.Pairing_Capabilities));
		/* Attempt to pair to the remote device. */
		ret_val = GAP_LE_Authentication_Response(BluetoothStackID, BD_ADDR, &AuthenticationResponseData);
		if(ret_val){HOGP_DBG_Display(("Error - GAP_LE_Authentication_Response returned %d.\r\n", ret_val));}
	}
	else{ret_val = HOGP_INVALID_STACK_ID_ERROR;}

	return(ret_val);
}

/**
  ***************************************************************************************************************************************
  * The following function provides a mechanism to configure a
  * Pairing Capabilities structure with the application's pairing
  * parameters.
  ***************************************************************************************************************************************
  */
static void HOGP_Configure_Capabilities(GAP_LE_Pairing_Capabilities_t *Capabilities)
{
	/* Make sure the Capabilities pointer is semi-valid. */
	if(Capabilities)
	{
		/* Configure the Pairing Cabilities structure. */
		Capabilities->Bonding_Type = lbtNoBonding;
		Capabilities->IO_Capability = LE_Parameters.IOCapability;
		Capabilities->MITM = LE_Parameters.MITMProtection;
		Capabilities->OOB_Present = LE_Parameters.OOBDataPresent;

		/** ** NOTE ** This application always requests that we use the
		  *            maximum encryption because this feature is not a
		  *            very good one, if we set less than the maximum we
		  *            will internally in GAP generate a key of the
		  *            maximum size (we have to do it this way) and then
		  *            we will zero out how ever many of the MSBs
		  *            necessary to get the maximum size.  Also as a slave
		  *            we will have to use Non-Volatile Memory (per device
		  *            we are paired to) to store the negotiated Key Size.
		  *            By requesting the maximum (and by not storing the
		  *            negotiated key size if less than the maximum) we
		  *            allow the slave to power cycle and regenerate the
		  *            LTK for each device it is paired to WITHOUT storing
		  *            any information on the individual devices we are
		  *            paired to.
		  */
		Capabilities->Maximum_Encryption_Key_Size = GAP_LE_MAXIMUM_ENCRYPTION_KEY_SIZE;

		/** This application only demostrates using Long Term Key's (LTK)
		  * for encryption of a LE Link (and receiving and IRK for
		  * identifying the remote device if it uses a resolvable random
		  * address), however we could request and send all possible keys
		  * here if we wanted to.
		  */
		Capabilities->Receiving_Keys.Encryption_Key     = FALSE;
		Capabilities->Receiving_Keys.Identification_Key = TRUE;
		Capabilities->Receiving_Keys.Signing_Key        = FALSE;

		Capabilities->Sending_Keys.Encryption_Key       = TRUE;
		Capabilities->Sending_Keys.Identification_Key   = FALSE;
		Capabilities->Sending_Keys.Signing_Key          = FALSE;
	}
}

/**
  ***************************************************************************************************************************************
  * The following function is provided to allow a mechanism of
  * responding to a request for Encryption Information to send to a
  * remote device.
  ***************************************************************************************************************************************
  */
static int HOGP_Encryption_Information_Request_Response(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, Byte_t KeySize, \
														GAP_LE_Authentication_Response_Information_t *GAP_LE_Authentication_Response_Information)
{
	int ret_val;
	Word_t LocalDiv;

	/* Make sure a Bluetooth Stack is open. */
	if(BluetoothStackID)
	{
		/* Make sure the input parameters are semi-valid. */
		if((!COMPARE_NULL_BD_ADDR(BD_ADDR)) && (GAP_LE_Authentication_Response_Information))
		{
			HOGP_DBG_Display(("   Calling GAP_LE_Generate_Long_Term_Key.\r\n"));

			/* Generate a new LTK, EDIV and Rand tuple. */
			ret_val = GAP_LE_Generate_Long_Term_Key(BluetoothStackID, (Encryption_Key_t *)(&DHK), (Encryption_Key_t *)(&ER), &(GAP_LE_Authentication_Response_Information->Authentication_Data.Encryption_Information.LTK), &LocalDiv, &(GAP_LE_Authentication_Response_Information->Authentication_Data.Encryption_Information.EDIV), &(GAP_LE_Authentication_Response_Information->Authentication_Data.Encryption_Information.Rand));
			if(!ret_val)
			{
				HOGP_DBG_Display(("   Encryption Information Request Response.\r\n"));

				/* Response to the request with the LTK, EDIV and Rand values. */
				GAP_LE_Authentication_Response_Information->GAP_LE_Authentication_Type = larEncryptionInformation;
				GAP_LE_Authentication_Response_Information->Authentication_Data_Length = GAP_LE_ENCRYPTION_INFORMATION_DATA_SIZE;
				GAP_LE_Authentication_Response_Information->Authentication_Data.Encryption_Information.Encryption_Key_Size = KeySize;
				ret_val = GAP_LE_Authentication_Response(BluetoothStackID, BD_ADDR, GAP_LE_Authentication_Response_Information);

				if(!ret_val){HOGP_DBG_Display(("   GAP_LE_Authentication_Response (larEncryptionInformation) success.\r\n", ret_val));}
				else{HOGP_DBG_Display(("   Error - SM_Generate_Long_Term_Key returned %d.\r\n", ret_val));}
			}
			else{HOGP_DBG_Display(("   Error - SM_Generate_Long_Term_Key returned %d.\r\n", ret_val));}
		}
		else
		{
			HOGP_DBG_Display(("Invalid Parameters.\r\n"));
			ret_val = HOGP_INVALID_PARAMETERS_ERROR;
		}
	}
	else
	{
		HOGP_DBG_Display(("Stack ID Invalid.\r\n"));
		ret_val = HOGP_INVALID_STACK_ID_ERROR;
	}

	return(ret_val);
}

/**
  ***************************************************************************************************************************************
  * The following function is provided to allow a mechanism to disconnect a currently connected device.
  ***************************************************************************************************************************************
  */
static int HOGP_Disconnect_LE_Device(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR)
{
	int Result;

	/* First, determine if the input parameters appear to be semi-valid. */
	if(BluetoothStackID)
	{
		/* Disconnect the device. */
		Result = GAP_LE_Disconnect(BluetoothStackID, BD_ADDR);

		if(!Result){HOGP_DBG_Display(("Disconnect Request successful.\r\n"));}
		else{HOGP_DBG_Display(("Unable to disconnect device: %d.\r\n", Result));}
	}
	else{Result = -1;}

	return(Result);
}

/**
  ***************************************************************************************************************************************
  * The following function is for an GATT Connection Event Callback.
  * This function is called for GATT Connection Events that occur on
  * the specified Bluetooth Stack.  This function passes to the caller
  * the GATT Connection Event Data that occurred and the GATT
  * Connection Event Callback Parameter that was specified when this
  * Callback was installed.  The caller is free to use the contents of
  * the GATT Client Event Data ONLY in the context of this callback.
  * If the caller requires the Data for a longer period of time, then
  * the callback function MUST copy the data into another Data Buffer.
  * This function is guaranteed NOT to be invoked more than once
  * simultaneously for the specified installed callback (i.e.  this
  * function DOES NOT have be reentrant).  It Needs to be noted
  * however, that if the same Callback is installed more than once,
  * then the callbacks will be called serially.  Because of this, the
  * processing in this function should be as efficient as possible.
  * It should also be noted that this function is called in the Thread
  * Context of a Thread that the User does NOT own.  Therefore,
  * processing in this function should be as efficient as possible
  * (this argument holds anyway because another GATT Event
  * (Server/Client or Connection) will not be processed while this
  * function call is outstanding).
  * * NOTE * This function MUST NOT Block and wait for Events that can
  *          only be satisfied by Receiving a Bluetooth Event
  *          Callback.  A Deadlock WILL occur because NO Bluetooth
  *          Callbacks will be issued while this function is currently
  *          outstanding.
  ***************************************************************************************************************************************
  */
static void BTPSAPI HOGP_GATT_Connection_Event_Callback(unsigned int BluetoothStackID, \
														GATT_Connection_Event_Data_t *GATT_Connection_Event_Data, unsigned long CallbackParameter)
{
	BoardStr_t BoardStr;

	/* Verify that all parameters to this callback are Semi-Valid. */
	if((BluetoothStackID) && (GATT_Connection_Event_Data))
	{
		/* Determine the Connection Event that occurred. */
		switch(GATT_Connection_Event_Data->Event_Data_Type)
		{
			case etGATT_Connection_Device_Connection:
			if(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data)
			{
				HOGP_DBG_Display(("\r\netGATT_Connection_Device_Connection with size %u: \r\n", GATT_Connection_Event_Data->Event_Data_Size));
				HOGP_BD_ADDR_To_Str(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->RemoteDevice, BoardStr);
				HOGP_DBG_Display(("   Connection ID:   %u.\r\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionID));
				HOGP_DBG_Display(("   Connection Type: %s.\r\n", ((GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionType == gctLE)?"LE":"BR/EDR")));
				HOGP_DBG_Display(("   Remote Device:   %s.\r\n", BoardStr));
				HOGP_DBG_Display(("   Connection MTU:  %u.\r\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->MTU));

				ApplicationStateInfo.Flags|= APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED;
				ApplicationStateInfo.LEConnectionInfo.ConnectionID = GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionID;
			}
			break;

			case etGATT_Connection_Device_Connection_Request:
			case etGATT_Connection_Device_Connection_Confirmation:
			case etGATT_Connection_Device_Disconnection:
			case etGATT_Connection_Device_Connection_MTU_Update:
			case etGATT_Connection_Server_Indication:
			case etGATT_Connection_Server_Notification:
			case etGATT_Connection_Service_Database_Update:
			case etGATT_Connection_Service_Changed_Read_Request:
			case etGATT_Connection_Service_Changed_Confirmation:
			case etGATT_Connection_Device_Buffer_Empty:
			case etGATT_Connection_Service_Changed_CCCD_Read_Request:
			case etGATT_Connection_Service_Changed_CCCD_Update:
			break;
		}
	}
	else
	{
		/* There was an error with one or more of the input parameters. */
		HOGP_DBG_Display(("\r\n"));
		HOGP_DBG_Display(("GATT Connection Callback Data: Event_Data = NULL.\r\n"));
	}
}

/**
  ***************************************************************************************************************************************
  * The following represents the a BAS Event Callback.  This function
  * will be called whenever an BAS Event occurs that is associated
  * with the specified Bluetooth Stack ID.  This function passes to
  * the caller the Bluetooth Stack ID, the BAS Event Data that
  * occurred and the BAS Event Callback Parameter that was specified
  * when this Callback was installed.  The caller is free to use the
  * contents of the BAS Event Data ONLY in the context of this
  * callback.  If the caller requires the Data for a longer period of
  * time, then the callback function MUST copy the data into another
  * Data Buffer This function is guaranteed NOT to be invoked more
  * than once simultaneously for the specified installed callback
  * (i.e.  this function DOES NOT have to be re-entrant).It needs to
  * be noted however, that if the same Callback is installed more than
  * once, then the callbacks will be called serially.  Because of
  * this, the processing in this function should be as efficient as
  * possible.  It should also be noted that this function is called in
  * the Thread Context of a Thread that the User does NOT own.
  * Therefore, processing in this function should be as efficient as
  * possible (this argument holds anyway because another BAS Event
  * will not be processed while this function call is outstanding).
  * ** NOTE ** This function MUST NOT Block and wait for events that
  *            can only be satisfied by Receiving BAS Event Packets.
  *            A Deadlock WILL occur because NO BAS Event Callbacks
  *            will be issued while this function is currently
  *            outstanding.
  ***************************************************************************************************************************************
  */
static void BTPSAPI HOGP_BAS_Event_Callback(unsigned int BluetoothStackID, BAS_Event_Data_t *BAS_Event_Data, unsigned long CallbackParameter)
{
	int Result;
	DeviceInfo_t *DeviceInfo;

	/* Verify that all parameters to this callback are Semi-Valid. */
	if((BluetoothStackID) && (BAS_Event_Data))
	{
		/* Search for the Device entry for our current LE connection. */
		if((DeviceInfo = HOGP_Search_Device_Info_Entry_By_BD_ADDR(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType, ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
		{
			/* Determine the Battery Service Event that occurred. */
			switch(BAS_Event_Data->Event_Data_Type)
			{
				case etBAS_Server_Read_Client_Configuration_Request:
				if((BAS_Event_Data->Event_Data.BAS_Read_Client_Configuration_Data) && (BAS_Event_Data->Event_Data.BAS_Read_Client_Configuration_Data->ClientConfigurationType == ctBatteryLevel))
				{
					HOGP_DBG_Display(("Battery Read Battery Client Configuration Request.\r\n"));
					Result = BAS_Read_Client_Configuration_Response(BluetoothStackID, ApplicationStateInfo.BASInstanceID, \
																	BAS_Event_Data->Event_Data.BAS_Read_Client_Configuration_Data->TransactionID, \
																	DeviceInfo->BASServerInformation.Battery_Level_Client_Configuration);
					if(Result){HOGP_DBG_Display(("Error - BAS_Read_Client_Configuration_Response() %d.\r\n", Result));}
				}
				break;

				case etBAS_Server_Client_Configuration_Update:
				if((BAS_Event_Data->Event_Data.BAS_Client_Configuration_Update_Data) && \
				   (BAS_Event_Data->Event_Data.BAS_Client_Configuration_Update_Data->ClientConfigurationType == ctBatteryLevel))
				{
					HOGP_DBG_Display(("Battery Client Configuration Update: %s.\r\n", (BAS_Event_Data->Event_Data.BAS_Client_Configuration_Update_Data->Notify?"ENABLED":"DISABLED")));

					/* Update the stored configuration for this device. */
					if(BAS_Event_Data->Event_Data.BAS_Client_Configuration_Update_Data->Notify)
					{DeviceInfo->BASServerInformation.Battery_Level_Client_Configuration = GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;}
					else{DeviceInfo->BASServerInformation.Battery_Level_Client_Configuration = 0;}
				}
				break;

				case etBAS_Server_Read_Battery_Level_Request:
				if(BAS_Event_Data->Event_Data.BAS_Read_Battery_Level_Data)
				{
					HOGP_DBG_Display(("Battery Read Battery Level Request.\r\n"));
					/* Just respond with the current Battery Level. */
					Result = BAS_Battery_Level_Read_Request_Response(BluetoothStackID, \
																	 BAS_Event_Data->Event_Data.BAS_Read_Battery_Level_Data->TransactionID, (Byte_t)ApplicationStateInfo.BatteryLevel);

					if(Result){HOGP_DBG_Display(("Error - BAS_Battery_Level_Read_Request_Response() %d.\r\n", Result));}
				}
				break;
			}
		}
	}
	else
	{
		/* There was an error with one or more of the input parameters. */
		HOGP_DBG_Display(("\r\n"));
		HOGP_DBG_Display(("Battery Service Callback Data: Event_Data = NULL.\r\n"));
	}
}

/**
  ***************************************************************************************************************************************
  * The following declared type represents the application HID Service
  * Event Callback.  This function will be called whenever an HIDS
  * Event occurs that is associated with the specified Bluetooth Stack
  * ID.  This function passes to the caller the Bluetooth Stack ID,
  * the HIDS Event Data that occurred and the HIDS Event Callback
  * Parameter that was specified when this Callback was installed.
  * The caller is free to use the contents of the HIDS Event Data ONLY
  * in the context of this callback.  If the caller requires the Data
  * for a longer period of time, then the callback function MUST copy
  * the data into another Data Buffer This function is guaranteed NOT
  * to be invoked more than once simultaneously for the specified
  * installed callback (i.e.  this function DOES NOT have be
  * re-entrant).  It needs to be noted however, that if the same
  * Callback is installed more than once, then the callbacks will be
  * called serially.  Because of this, the processing in this function
  * should be as efficient as possible.  It should also be noted that
  * this function is called in the Thread Context of a Thread that the
  * User does NOT own.  Therefore, processing in this function should
  * be as efficient as possible (this argument holds anyway because
  * another HIDS Profile Event will not be processed while this
  * function call is outstanding).
  * ** NOTE ** This function MUST NOT Block and wait for events that
  *            can only be satisfied by receiving HIDS Event Packets.
  *            A Deadlock WILL occur because NO HIDS Event Callbacks
  *            will be issued while this function is currently
  *            outstanding.
  ***************************************************************************************************************************************
  */
static void BTPSAPI HOGP_HIDS_Event_Callback(unsigned int BluetoothStackID, HIDS_Event_Data_t *HIDS_Event_Data, unsigned long CallbackParameter)
{
	int Result;
	Byte_t ErrorCode;
	Word_t Configuration;
	Byte_t *ReportData;
	DeviceInfo_t *DeviceInfo;
	unsigned int ReportDataLength;

	/* Verify that all parameters to this callback are Semi-Valid. */
	if((BluetoothStackID) && (HIDS_Event_Data))
	{
		/* Search for the Device entry for our current LE connection. */
		if((DeviceInfo = HOGP_Search_Device_Info_Entry_By_BD_ADDR(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType, \
																  ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
		{
			/* Determine the HID Service Event that occurred. */
			switch(HIDS_Event_Data->Event_Data_Type)
			{
				case etHIDS_Server_Read_Client_Configuration_Request:
				if(HIDS_Event_Data->Event_Data.HIDS_Read_Client_Configuration_Data)
				{
					HOGP_DBG_Display(("HIDS Read Client Configuration Request: %u.\r\n", HIDS_Event_Data->Event_Data.HIDS_Read_Client_Configuration_Data->ReportType));

					if(HIDS_Event_Data->Event_Data.HIDS_Read_Client_Configuration_Data->ReportType == rtBootKeyboardInputReport)
					{Configuration = DeviceInfo->BootKeyboardInputConfiguration;}
					else{Configuration = DeviceInfo->ReportKeyboardInputConfiguration;}

					/* Respond to the read request. */
					Result = HIDS_Read_Client_Configuration_Response(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.HIDSInstanceID, \
																	 HIDS_Event_Data->Event_Data.HIDS_Read_Client_Configuration_Data->TransactionID, Configuration);
					if(Result){HOGP_DBG_Display(("Error - HIDS_Read_Client_Configuration_Response() %d.\r\n", Result));}
				}
				break;

				case etHIDS_Server_Client_Configuration_Update_Request:
				if(HIDS_Event_Data->Event_Data.HIDS_Client_Configuration_Update_Data)
				{
					HOGP_DBG_Display(("HIDS Client Configuration Update: %u.\r\n", HIDS_Event_Data->Event_Data.HIDS_Client_Configuration_Update_Data->ReportType));

					if(HIDS_Event_Data->Event_Data.HIDS_Client_Configuration_Update_Data->ReportType == rtBootKeyboardInputReport)
					{
						DeviceInfo->BootKeyboardInputConfiguration   = HIDS_Event_Data->Event_Data.HIDS_Client_Configuration_Update_Data->ClientConfiguration;
						HOGP_DBG_Display(("Boot Keyboard Input Report Configuration 0x%04X.\r\n", DeviceInfo->BootKeyboardInputConfiguration));
					}
					else
					{
						DeviceInfo->ReportKeyboardInputConfiguration = HIDS_Event_Data->Event_Data.HIDS_Client_Configuration_Update_Data->ClientConfiguration;
						HOGP_DBG_Display(("Report Keyboard Input Report Configuration 0x%04X.\r\n", DeviceInfo->ReportKeyboardInputConfiguration));
					}
				}
				break;

				case etHIDS_Server_Get_Protocol_Mode_Request:
				if(HIDS_Event_Data->Event_Data.HIDS_Get_Protocol_Mode_Request_Data)
				{
					HOGP_DBG_Display(("HIDS Get Protocol Mode Request.\r\n"));

					/* Note that security is required to read this characteristic. */
					if(ApplicationStateInfo.LEConnectionInfo.Flags & CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED){ErrorCode = 0;}
					else{ErrorCode = ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_ENCRYPTION;}

					/* Respond the Get Protocol Mode request. */
					Result = HIDS_Get_Protocol_Mode_Response(ApplicationStateInfo.BluetoothStackID, \
															 ApplicationStateInfo.HIDSInstanceID, HIDS_Event_Data->Event_Data.HIDS_Get_Protocol_Mode_Request_Data->TransactionID, \
															 ErrorCode, ApplicationStateInfo.HIDProtocolMode);
					if(Result){HOGP_DBG_Display(("Error - HIDS_Get_Protocol_Mode_Response() %d.\r\n", Result));}
				}
				break;

				case etHIDS_Server_Set_Protocol_Mode_Request:
				if(HIDS_Event_Data->Event_Data.HIDS_Set_Protocol_Mode_Request_Data)
				{
					HOGP_DBG_Display(("HIDS Set Protocol Mode Request: %s(%u).\r\n", (HIDS_Event_Data->Event_Data.HIDS_Set_Protocol_Mode_Request_Data->ProtocolMode == pmBoot)?\
									  "Boot":"Report", (unsigned int)HIDS_Event_Data->Event_Data.HIDS_Set_Protocol_Mode_Request_Data->ProtocolMode));

					/* Note that security is required to write this characteristic. */
					if(ApplicationStateInfo.LEConnectionInfo.Flags & CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED)
					{ApplicationStateInfo.HIDProtocolMode = HIDS_Event_Data->Event_Data.HIDS_Set_Protocol_Mode_Request_Data->ProtocolMode;}
				}
				break;

				case etHIDS_Server_Get_Report_Map_Request:
				if(HIDS_Event_Data->Event_Data.HIDS_Get_Report_Map_Data)
				{
					HOGP_DBG_Display(("HIDS Get Report Map Request: Offset = %u.\r\n", HIDS_Event_Data->Event_Data.HIDS_Get_Report_Map_Data->ReportMapOffset));

					/* Note that security is required to read this characteristic. */
					if(ApplicationStateInfo.LEConnectionInfo.Flags & CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED)
					{
						/* Initialize the return value to success. */
						ErrorCode = 0;

						/* Verify that the offset being read is valid. */
						if(HIDS_Event_Data->Event_Data.HIDS_Get_Report_Map_Data->ReportMapOffset < (sizeof(KeyboardReportDescriptor)))
						{
							/* Get a pointer to the report map to return. */
							ReportDataLength = (sizeof(KeyboardReportDescriptor) - HIDS_Event_Data->Event_Data.HIDS_Get_Report_Map_Data->ReportMapOffset);
							ReportData       = &KeyboardReportDescriptor[HIDS_Event_Data->Event_Data.HIDS_Get_Report_Map_Data->ReportMapOffset];
						}
						else{ErrorCode = ATT_PROTOCOL_ERROR_CODE_INVALID_OFFSET;}
					}
					else
					{
						ErrorCode        = ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_ENCRYPTION;
						ReportDataLength = 0;
						ReportData       = NULL;
					}

					/* Respond the Get Report Map request. */
					Result = HIDS_Get_Report_Map_Response(ApplicationStateInfo.BluetoothStackID, \
														  ApplicationStateInfo.HIDSInstanceID, HIDS_Event_Data->Event_Data.HIDS_Get_Report_Map_Data->TransactionID, \
														  ErrorCode, ReportDataLength, ReportData);
					if(Result){HOGP_DBG_Display(("Error - HIDS_Get_Report_Map_Response() %d.\r\n", Result));}
				}
				break;

				case etHIDS_Server_Get_Report_Request:
				if(HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data)
				{
					HOGP_DBG_Display(("HID Get Report Request: %u.\r\n", HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportType));

					/* Note that security is required to read this characteristic. */
					if(ApplicationStateInfo.LEConnectionInfo.Flags & CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED)
					{
						/* Flag that no error has occurred. */
						ErrorCode = 0;
						/* Determine what report the Host is attempting to read. */
						if((HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportType == rtBootKeyboardInputReport) || \
						   ((HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportType == rtReport) && \
							(HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportReferenceData.ReportType == HIDS_REPORT_REFERENCE_REPORT_TYPE_INPUT_REPORT)))
						{
							/** Respond with the Keyboard Input Report. Note
							  * that since our Report Mode Report is
							  * identical to the Boot Mode Report we do not
							  * need to differentiate here.
							  */
							ReportDataLength = HID_KEYBOARD_INPUT_REPORT_SIZE;
							ReportData = ApplicationStateInfo.CurrentInputReport;
						}
						else
						{
							if((HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportType == rtBootKeyboardOutputReport) || \
							   ((HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportType == rtReport) && \
								(HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportReferenceData.ReportType == HIDS_REPORT_REFERENCE_REPORT_TYPE_OUTPUT_REPORT)))
							{
								/** Respond with the Keyboard Output Report.
								  * Note that since our Report Mode Report is
								  * identical to the Boot Mode Report we do
								  * not need to differentiate here.
								  */
								ReportDataLength = sizeof(ApplicationStateInfo.CurrentOutputReport);
								ReportData = &(ApplicationStateInfo.CurrentOutputReport);
							}
							else
							{
								HOGP_DBG_Display(("Unknown Report %u, (ID,Type) = %u, %u.\r\n", HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportType, \
												 HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportReferenceData.ReportID, \
												 HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportReferenceData.ReportType));
								ErrorCode = ATT_PROTOCOL_ERROR_CODE_UNLIKELY_ERROR;
							}
						}
					}
					else
					{
						ErrorCode = ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_ENCRYPTION;
						ReportDataLength = 0;
						ReportData = NULL;
					}

					/* Respond to the Get Report Request. */
					Result = HIDS_Get_Report_Response(ApplicationStateInfo.BluetoothStackID, \
													  ApplicationStateInfo.HIDSInstanceID, HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->TransactionID, \
													  HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportType, \
													  &(HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportReferenceData), \
													  ErrorCode, ReportDataLength, ReportData);
					if(Result){HOGP_DBG_Display(("Error - HIDS_Get_Report_Response() %d.\r\n", Result));}
				}
				break;

				case etHIDS_Server_Set_Report_Request:
				if(HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data)
				{
					HOGP_DBG_Display(("HID Set Report Request: %u.\r\n", HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportType));

					/* Note that security is required to write this characteristic. */
					if(ApplicationStateInfo.LEConnectionInfo.Flags & CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED)
					{
						/* Flag that no error has occurred. */
						ErrorCode = 0;
						/* Determine what report the Host is attempting to write. */
						if((HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportType == rtBootKeyboardOutputReport) || \
						   ((HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportType == rtReport) && \
						    (HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportReferenceData.ReportType == HIDS_REPORT_REFERENCE_REPORT_TYPE_OUTPUT_REPORT)))
						{
							/* Verify that the length is valid. */
							if(HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportLength == (sizeof(ApplicationStateInfo.CurrentOutputReport)))
							{
								/* Set the Output Report Value. */
								ApplicationStateInfo.CurrentOutputReport = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->Report);
								HOGP_DBG_Display(("Current Output Report Value: 0x%02X.\r\n", ApplicationStateInfo.CurrentOutputReport));
							}
							else{ErrorCode = ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH;}
						}
						else
						{
							HOGP_DBG_Display(("Unknown Report %u, (ID,Type) = %u, %u.\r\n", HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportType, \
											 HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportReferenceData.ReportID, \
											 HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportReferenceData.ReportType));
							ErrorCode = ATT_PROTOCOL_ERROR_CODE_UNLIKELY_ERROR;
						}
					}
					else{ErrorCode = ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_ENCRYPTION;}

					/* Respond to the Set Report Request. */
					Result = HIDS_Set_Report_Response(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.HIDSInstanceID, \
													  HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->TransactionID, \
													  HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportType, \
													  &(HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportReferenceData), ErrorCode);
					if(Result){HOGP_DBG_Display(("Error - HIDS_Set_Report_Response() %d.\r\n", Result));}
				}
				break;

				case etHIDS_Server_Control_Point_Command_Indication:
				if(HIDS_Event_Data->Event_Data.HIDS_Control_Point_Command_Data)
				{HOGP_DBG_Display(("HID Control Point Command: %s (%u).\r\n", ((HIDS_Event_Data->Event_Data.HIDS_Control_Point_Command_Data->ControlPointCommand == pcSuspend)? \
																			   "Suspend":"Exit Suspend"), (unsigned int)HIDS_Event_Data->Event_Data.HIDS_Control_Point_Command_Data->ControlPointCommand));}
				break;
			}
		}
		else{HOGP_DBG_Display(("Error Error Error!\r\n"));}
	}
	else
	{
		/* There was an error with one or more of the input parameters. */
		HOGP_DBG_Display(("\r\n"));
		HOGP_DBG_Display(("HIDS Event Callback Data: Event_Data = NULL.\r\n"));
	}
}

/**
  ***************************************************************************************************************************************
  * The following function is for the GAP LE Event Receive Data
  * Callback.  This function will be called whenever a Callback has
  * been registered for the specified GAP LE Action that is associated
  * with the Bluetooth Stack.  This function passes to the caller the
  * GAP LE Event Data of the specified Event and the GAP LE Event
  * Callback Parameter that was specified when this Callback was
  * installed.  The caller is free to use the contents of the GAP LE
  * Event Data ONLY in the context of this callback.  If the caller
  * requires the Data for a longer period of time, then the callback
  * function MUST copy the data into another Data Buffer.  This
  * function is guaranteed NOT to be invoked more than once
  * simultaneously for the specified installed callback (i.e.  this
  * function DOES NOT have be reentrant).  It Needs to be noted
  * however, that if the same Callback is installed more than once,
  * then the callbacks will be called serially.  Because of this, the
  * processing in this function should be as efficient as possible.
  * It should also be noted that this function is called in the Thread
  * Context of a Thread that the User does NOT own.  Therefore,
  * processing in this function should be as efficient as possible
  * (this argument holds anyway because other GAP Events will not be
  * processed while this function call is outstanding).
  * * NOTE * This function MUST NOT Block and wait for Events that can
  *          only be satisfied by Receiving a Bluetooth Event
  *          Callback.  A Deadlock WILL occur because NO Bluetooth
  *          Callbacks will be issued while this function is currently
  *          outstanding.
  ***************************************************************************************************************************************
  */
static void BTPSAPI HOGP_GAP_LE_Event_Callback(unsigned int BluetoothStackID, GAP_LE_Event_Data_t *GAP_LE_Event_Data, unsigned long CallbackParameter)
{
	int Result;
	BoardStr_t BoardStr;
	DeviceInfo_t *DeviceInfo;
	Long_Term_Key_t GeneratedLTK;
	GAP_LE_Authentication_Event_Data_t *Authentication_Event_Data;
	GAP_LE_Authentication_Response_Information_t GAP_LE_Authentication_Response_Information;

	/* Verify that all parameters to this callback are Semi-Valid. */
	if((BluetoothStackID) && (GAP_LE_Event_Data))
	{
		switch(GAP_LE_Event_Data->Event_Data_Type)
		{
			case etLE_Connection_Complete:
			HOGP_DBG_Display(("etLE_Connection_Complete with size %d.\r\n",(int)GAP_LE_Event_Data->Event_Data_Size));

			if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data)
			{
				HOGP_BD_ADDR_To_Str(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address, BoardStr);

				HOGP_DBG_Display(("   Status:       0x%02X.\r\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Status));
				HOGP_DBG_Display(("   Role:         %s.\r\n", (GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Master)?"Master":"Slave"));
				HOGP_DBG_Display(("   Address Type: %s.\r\n", (GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type == latPublic)?"Public":"Random"));
				HOGP_DBG_Display(("   BD_ADDR:      %s.\r\n", BoardStr));

				/* Check to see if the connection was succesfull. */
				if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR)
				{
					/* Save the Connection Information. */
					ApplicationStateInfo.Flags |= APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED;
					ApplicationStateInfo.LEConnectionInfo.Flags = CONNECTION_INFO_FLAGS_CONNECTION_VALID;
					ApplicationStateInfo.LEConnectionInfo.BD_ADDR = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address;
					ApplicationStateInfo.LEConnectionInfo.AddressType = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type;

					/* Inform the main application handler of the LE connection. */
					HOGP_Post_Application_Mailbox(APPLICATION_MAILBOX_MESSAGE_ID_LE_CONNECTED);

					/* Make sure that no entry already exists. */
					if((DeviceInfo = HOGP_Search_Device_Info_Entry_By_BD_ADDR(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type, \
																			  GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address)) == NULL)
					{
						/* No entry exists so create one. */
						if(!HOGB_Create_New_Device_Info_Entry(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type, \
															  GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address))
						{HOGP_DBG_Display(("Failed to add device to Device Info List.\r\n"));}
					}
					else
					{
						/** We have paired with this device previously.
						  * Therefore we will start a timer and if the
						  * Master does not re-establish encryption when the
						  * timer expires we will request that he does so.
						  */
						Result = BSC_StartTimer(BluetoothStackID, \
								(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Current_Connection_Parameters.Connection_Interval * \
								(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Current_Connection_Parameters.Slave_Latency + 8)), HOGP_BSC_TimerCallback, 0);
						if(Result > 0)
						{
							/* Save the Security Timer ID. */
							ApplicationStateInfo.LEConnectionInfo.SecurityTimerID = (unsigned int)Result;
						}
						else{HOGP_DBG_Display(("Error - BSC_StartTimer() returned %d.\r\n", Result));}
					}
				}
				else
				{
					/* Failed to connect to device so start device advertising again. */
					HOGP_Post_Application_Mailbox(APPLICATION_MAILBOX_MESSAGE_ID_LE_DISCONNECTED);
				}
			}
			break;

			case etLE_Disconnection_Complete:
			HOGP_DBG_Display(("etLE_Disconnection_Complete with size %d.\r\n", (int)GAP_LE_Event_Data->Event_Data_Size));

			if(GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data)
			{
				HOGP_DBG_Display(("   Status: 0x%02X.\r\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Status));
				HOGP_DBG_Display(("   Reason: 0x%02X.\r\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Reason));
				HOGP_BD_ADDR_To_Str(GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address, BoardStr);
				HOGP_DBG_Display(("   BD_ADDR: %s.\r\n", BoardStr));

				/* Check to see if the device info is present in the list. */
				if((DeviceInfo = HOGP_Search_Device_Info_Entry_By_BD_ADDR(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address_Type, \
																		  GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address)) != NULL)
				{
					/* Check to see if the link is encrypted. If it isn't we will delete the device structure. */
					if(!(ApplicationStateInfo.LEConnectionInfo.Flags & CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED))
					{
						/* Connection is not encrypted so delete the device structure. */
						DeviceInfo = HOGP_Delete_Device_Info_Entry(&DeviceInfoList, DeviceInfo->AddressType, DeviceInfo->BD_ADDR);
						if(DeviceInfo){HOGP_Free_Device_Info_Entry_Memory(DeviceInfo);}
					}
				}

				/* Inform the event handler of the LE Disconnection. */
				HOGP_Post_Application_Mailbox(APPLICATION_MAILBOX_MESSAGE_ID_LE_DISCONNECTED);
			}
			break;

			case etLE_Encryption_Change:
			HOGP_DBG_Display(("etLE_Encryption_Change with size %d.\r\n",(int)GAP_LE_Event_Data->Event_Data_Size));

			/* Verify that the link is currently encrypted.             */
			if((GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Change_Event_Data->Encryption_Change_Status == HCI_ERROR_CODE_NO_ERROR) && \
			   (GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Change_Event_Data->Encryption_Mode == emEnabled))
			{ApplicationStateInfo.LEConnectionInfo.Flags |= CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED;}
			else{ApplicationStateInfo.LEConnectionInfo.Flags &= ~CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED;}
			break;

			case etLE_Encryption_Refresh_Complete:
			HOGP_DBG_Display(("etLE_Encryption_Refresh_Complete with size %d.\r\n", (int)GAP_LE_Event_Data->Event_Data_Size));

			/* Verify that the link is currently encrypted. */
			if(GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Refresh_Complete_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR)
			{ApplicationStateInfo.LEConnectionInfo.Flags |= CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED;}
			else{ApplicationStateInfo.LEConnectionInfo.Flags &= ~CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED;}
			break;

			case etLE_Authentication:
			HOGP_DBG_Display(("etLE_Authentication with size %d.\r\n", (int)GAP_LE_Event_Data->Event_Data_Size));

			/* Make sure the authentication event data is valid before continueing. */
			if((Authentication_Event_Data = GAP_LE_Event_Data->Event_Data.GAP_LE_Authentication_Event_Data) != NULL)
			{
				HOGP_BD_ADDR_To_Str(Authentication_Event_Data->BD_ADDR, BoardStr);

				switch(Authentication_Event_Data->GAP_LE_Authentication_Event_Type)
				{
					case latLongTermKeyRequest:
					HOGP_DBG_Display(("    latKeyRequest: \r\n"));
					HOGP_DBG_Display(("      BD_ADDR: %s.\r\n", BoardStr));

					/** The other side of a connection is requesting
					  * that we start encryption. Thus we should
					  * regenerate LTK for this connection and send it
					  * to the chip.
					  */
					Result = GAP_LE_Regenerate_Long_Term_Key(BluetoothStackID, (Encryption_Key_t *)(&DHK), (Encryption_Key_t *)(&ER), \
							 	 	 	 	 	 	 	 	 Authentication_Event_Data->Authentication_Event_Data.Long_Term_Key_Request.EDIV, \
															 &(Authentication_Event_Data->Authentication_Event_Data.Long_Term_Key_Request.Rand), &GeneratedLTK);
					if(!Result)
					{
						HOGP_DBG_Display(("      GAP_LE_Regenerate_Long_Term_Key Success.\r\n"));
						/* Respond with the Re-Generated Long Term Key. */
						GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type = larLongTermKey;
						GAP_LE_Authentication_Response_Information.Authentication_Data_Length = GAP_LE_LONG_TERM_KEY_INFORMATION_DATA_SIZE;
						GAP_LE_Authentication_Response_Information.Authentication_Data.Long_Term_Key_Information.Encryption_Key_Size = GAP_LE_MAXIMUM_ENCRYPTION_KEY_SIZE;
						GAP_LE_Authentication_Response_Information.Authentication_Data.Long_Term_Key_Information.Long_Term_Key = GeneratedLTK;
					}
					else
					{
						HOGP_DBG_Display(("      GAP_LE_Regenerate_Long_Term_Key returned %d.\r\n",Result));
						/* Since we failed to generate the requested key we should respond with a negative response. */
						GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type = larLongTermKey;
						GAP_LE_Authentication_Response_Information.Authentication_Data_Length = 0;
					}

					/* Send the Authentication Response. */
					Result = GAP_LE_Authentication_Response(BluetoothStackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Authentication_Response_Information);
					if(!Result)
					{
						/* Master is trying to re-encrypt the Link so therefore we should cancel the Security Timer if it is active. */
						if(ApplicationStateInfo.LEConnectionInfo.SecurityTimerID)
						{
							BSC_StopTimer(BluetoothStackID, ApplicationStateInfo.LEConnectionInfo.SecurityTimerID);
							ApplicationStateInfo.LEConnectionInfo.SecurityTimerID = 0;
						}
					}
					else{HOGP_DBG_Display(("      GAP_LE_Authentication_Response returned %d.\r\n",Result));}
					break;

					case latPairingRequest:
					HOGP_DBG_Display(("Pairing Request: %s.\r\n",BoardStr));
					HOGP_Display_Pairing_Information(Authentication_Event_Data->Authentication_Event_Data.Pairing_Request);

					/** Master is trying to pair with us so therefore we
					  * should cancel the Security Timer if it is
					  * active.
					  */
					if(ApplicationStateInfo.LEConnectionInfo.SecurityTimerID)
					{
						BSC_StopTimer(BluetoothStackID, ApplicationStateInfo.LEConnectionInfo.SecurityTimerID);
						ApplicationStateInfo.LEConnectionInfo.SecurityTimerID = 0;
					}

					/** This is a pairing request. Respond with a
					  * Pairing Response.
					  * * NOTE * This is only sent from Master to Slave.
					  *          Thus we must be the Slave in this
					  *          connection.
					  */

					/* Send the Pairing Response. */
					HOGP_Slave_Pairing_Request_Response(BluetoothStackID, Authentication_Event_Data->BD_ADDR);
					break;

					case latConfirmationRequest:
					HOGP_DBG_Display(("latConfirmationRequest.\r\n"));

					if(Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Request_Type == crtNone)
					{
						HOGP_DBG_Display(("Invoking Just Works.\r\n"));
						/* Just Accept Just Works Pairing. */
						GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type = larConfirmation;

						/** By setting the Authentication_Data_Length to
						  * any NON-ZERO value we are informing the GAP
						  * LE Layer that we are accepting Just Works
						  * Pairing.
						  */
						GAP_LE_Authentication_Response_Information.Authentication_Data_Length = DWORD_SIZE;
						Result = GAP_LE_Authentication_Response(BluetoothStackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Authentication_Response_Information);
						if(Result)
						{HOGP_DBG_Display(("GAP_LE_Authentication_Response returned %d.\r\n",Result));}
					}
					else
					{
						if(Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Request_Type == crtPasskey)
						{
							HOGP_DBG_Display(("Enter 6 digit Passkey on display.\r\n"));
							/* Flag that we are awaiting a Passkey Input. */
							ApplicationStateInfo.LEConnectionInfo.Flags |= CONNECTION_INFO_FLAGS_CONNECTION_AWAITING_PASSKEY;
							ApplicationStateInfo.LEConnectionInfo.PasskeyDigits = 0;
							ApplicationStateInfo.LEConnectionInfo.Passkey = 0;
						}
						else
						{
							if(Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Request_Type == crtDisplay)
							{HOGP_DBG_Display(("Passkey: %06lu.\r\n", Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Display_Passkey));}
						}
					}
					break;

					case latEncryptionInformationRequest:
					HOGP_DBG_Display(("Encryption Information Request %s.\r\n", BoardStr));
					/* Generate new LTK,EDIV and Rand and respond with them. */
					HOGP_Encryption_Information_Request_Response(BluetoothStackID, Authentication_Event_Data->BD_ADDR, Authentication_Event_Data->Authentication_Event_Data.Encryption_Request_Information.Encryption_Key_Size, &GAP_LE_Authentication_Response_Information);
					break;

					case latIdentityInformation:
					HOGP_DBG_Display(("latIdentityInformation.\r\n"));

					/* Search for the Device entry for our current LE connection. */
					if((DeviceInfo = HOGP_Search_Device_Info_Entry_By_BD_ADDR(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType, ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
					{
						/* Store the received IRK and also updated the BD_ADDR that is stored to the "Base" BD_ADDR. */
						DeviceInfo->AddressType = Authentication_Event_Data->Authentication_Event_Data.Identity_Information.Address_Type;
						DeviceInfo->BD_ADDR = Authentication_Event_Data->Authentication_Event_Data.Identity_Information.Address;
						DeviceInfo->IRK = Authentication_Event_Data->Authentication_Event_Data.Identity_Information.IRK;
						DeviceInfo->Flags |= DEVICE_INFO_FLAGS_IRK_VALID;
					}
					break;

					case latSecurityEstablishmentComplete:
					HOGP_DBG_Display(("Security Re-Establishment Complete: %s.\r\n", BoardStr));
					HOGP_DBG_Display(("                            Status: 0x%02X.\r\n", Authentication_Event_Data->Authentication_Event_Data.Security_Establishment_Complete.Status));

					/** Check to see if the Security Re-establishment
					  * was successful (or if it failed since the remote
					  * device attempted to re-pair.
					  */
					if((Authentication_Event_Data->Authentication_Event_Data.Security_Establishment_Complete.Status != GAP_LE_SECURITY_ESTABLISHMENT_STATUS_CODE_NO_ERROR) && \
					   (Authentication_Event_Data->Authentication_Event_Data.Security_Establishment_Complete.Status != GAP_LE_SECURITY_ESTABLISHMENT_STATUS_CODE_DEVICE_TRIED_TO_REPAIR))
					{
						/** Security Re-establishment was not successful
						  * so delete the stored device information and
						  * disconnect the link.
						  */
						HOGP_Disconnect_LE_Device(BluetoothStackID, Authentication_Event_Data->BD_ADDR);

						/* Delete the stored device info structure. */
						if((DeviceInfo = HOGP_Delete_Device_Info_Entry(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType, ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
						{HOGP_Free_Device_Info_Entry_Memory(DeviceInfo);}
					}
					break;

					case latPairingStatus:
					HOGP_DBG_Display(("Pairing Status: %s.\r\n", BoardStr));
					HOGP_DBG_Display(("        Status: 0x%02X.\r\n", Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Status));

					/** Check to see if we have paired successfully with
					  * the device.
					  *                                      */
					if(Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Status == GAP_LE_PAIRING_STATUS_NO_ERROR)
					{
						HOGP_DBG_Display(("        Key Size: %d.\r\n", Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Negotiated_Encryption_Key_Size));

						/** Flag that no matter what we are not waiting
						  * on a Passkey any longer.
						  */
						ApplicationStateInfo.LEConnectionInfo.Flags &= ~APPLICATION_MAILBOX_MESSAGE_ID_PASSKEY_ENTERED;

						/** Search for the Device entry for our current
						  * LE connection.
						  */
						if((DeviceInfo = HOGP_Search_Device_Info_Entry_By_BD_ADDR(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType, ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
						{
							/* Save the encryption key size. */
							DeviceInfo->EncryptionKeySize = Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Negotiated_Encryption_Key_Size;
						}
					}
					else
					{
						/* Disconnect the Link. */
						HOGP_Disconnect_LE_Device(BluetoothStackID, Authentication_Event_Data->BD_ADDR);

						/* Delete the stored device info structure. */
						if((DeviceInfo = HOGP_Delete_Device_Info_Entry(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType, ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
						{HOGP_Free_Device_Info_Entry_Memory(DeviceInfo);}
					}
					break;

					case latSecurityRequest:
					case latIdentityInformationRequest:
					case latSigningInformationRequest:
					case latEncryptionInformation:
					case latSigningInformation:
					break;
				}
			}
			break;

			case etLE_Remote_Features_Result:
			case etLE_Advertising_Report:
			case etLE_Connection_Parameter_Update_Request:
			case etLE_Connection_Parameter_Update_Response:
			case etLE_Connection_Parameter_Updated:
			break;
		}
	}
}

/**
  ***************************************************************************************************************************************
  * The following function represents the Security Timer that is used
  * to ensure that the Master re-establishes security after pairing on
  * subsequent connections.
  ***************************************************************************************************************************************
  */
static void BTPSAPI HOGP_BSC_TimerCallback(unsigned int BluetoothStackID, unsigned int TimerID, unsigned long CallbackParameter)
{
	/* Verify that the input parameters are semi-valid. */
	if((BluetoothStackID) && (TimerID))
	{
		/* Verify that the LE Connection is still active. */
		if((ApplicationStateInfo.Flags & APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED) && (ApplicationStateInfo.LEConnectionInfo.SecurityTimerID == TimerID))
		{
			/* Invalidate the Timer ID that just expired. */
			ApplicationStateInfo.LEConnectionInfo.SecurityTimerID = 0;

			/* If the connection is not currently encrypted then we will send a Slave Security Request. */
			if(!(ApplicationStateInfo.LEConnectionInfo.Flags & CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED))
			{HOGP_Slave_Security_ReEstablishment(BluetoothStackID, ApplicationStateInfo.LEConnectionInfo.BD_ADDR);}
		}
	}
}

/**
  ***************************************************************************************************************************************
  * The following is the HCI Sleep Callback.  This is registered with
  * the stack to note when the Host processor may enter into a sleep
  * mode.
  ***************************************************************************************************************************************
  */
static void BTPSAPI HOGP_HCI_Sleep_Callback(Boolean_t SleepAllowed, unsigned long CallbackParameter)
{
	/* Simply store the state internally. */
	SleepEnabledFlag = SleepAllowed;
}

/**
  ***************************************************************************************************************************************
  * The following function is a utility function that is used to
  * notify the Battery Level to the connected LE device (if the device
  * has registered for notifications).
  ***************************************************************************************************************************************
  */
static void HOGP_Notify_Battery_Level(ApplicationStateInfo_t *ApplicationStateInfo, Boolean_t Force)
{
	int Result;
	DeviceInfo_t *DeviceInfo;

	/* Make sure the input parameters are semi-valid. */
	if((ApplicationStateInfo) && (ApplicationStateInfo->Flags & APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED))
	{
		/* Search for the device info structure for this device. */
		if((DeviceInfo = HOGP_Search_Device_Info_Entry_By_BD_ADDR(&DeviceInfoList, ApplicationStateInfo->LEConnectionInfo.AddressType, \
																  ApplicationStateInfo->LEConnectionInfo.BD_ADDR)) != NULL)
		{
			/* Verify that we can send a notification to this device. */
			if((DeviceInfo->BASServerInformation.Battery_Level_Client_Configuration & GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE) && \
			   ((Force) || ((!Force) && (DeviceInfo->LostNotifiedBatteryLevel != ApplicationStateInfo->BatteryLevel))))
			{
				/* Attempt to send the notification to the device. */
				Result = BAS_Notify_Battery_Level(ApplicationStateInfo->BluetoothStackID, \
						 	 	 	 	 	 	  ApplicationStateInfo->BASInstanceID, ApplicationStateInfo->LEConnectionInfo.ConnectionID, \
												  (Byte_t)ApplicationStateInfo->BatteryLevel);
				if(!Result){DeviceInfo->LostNotifiedBatteryLevel = ApplicationStateInfo->BatteryLevel;}
				else{HOGP_DBG_Display(("Error - BAS_Notify_Battery_Level() %d.\r\n", Result));}
			}
		}
	}

	HOGP_DBG_Display(("Timer Callback.\r\n"));
}

/**
  ***************************************************************************************************************************************
  * The following function is a utility function that starts an advertising process.
  ***************************************************************************************************************************************
  */
static int HOGP_Start_Advertising(unsigned int BluetoothStackID)
{
	int ret_val;
	GAP_LE_Advertising_Parameters_t AdvertisingParameters;
	GAP_LE_Connectability_Parameters_t ConnectabilityParameters;

	/* First, check that valid Bluetooth Stack ID exists. */
	if(BluetoothStackID)
	{
		/* Set up the advertising parameters. */
		AdvertisingParameters.Advertising_Channel_Map = HCI_LE_ADVERTISING_CHANNEL_MAP_DEFAULT;
		AdvertisingParameters.Scan_Request_Filter = fpNoFilter;
		AdvertisingParameters.Connect_Request_Filter = fpNoFilter;
		AdvertisingParameters.Advertising_Interval_Min = 50;
		AdvertisingParameters.Advertising_Interval_Max = 100;

		/** Configure the Connectability Parameters.
		  * * NOTE * Since we do not ever put ourselves to be direct
		  *          connectable then we will set the DirectAddress to all
		  *          0s.
		  */
		ConnectabilityParameters.Connectability_Mode = lcmConnectable;
		ConnectabilityParameters.Own_Address_Type = latPublic;
		ConnectabilityParameters.Direct_Address_Type = latPublic;
		ASSIGN_BD_ADDR(ConnectabilityParameters.Direct_Address, 0, 0, 0, 0, 0, 0);

		/* Now enable advertising. */
		ret_val = GAP_LE_Advertising_Enable(BluetoothStackID, TRUE, &AdvertisingParameters, &ConnectabilityParameters, HOGP_GAP_LE_Event_Callback, 0);
		if(!ret_val){HOGP_DBG_Display(("GAP_LE_Advertising_Enable success.\r\n"));}
		else
		{
			HOGP_DBG_Display(("GAP_LE_Advertising_Enable returned %d.\r\n", ret_val));
			ret_val = HOGP_FUNCTION_ERROR;
		}
	}
	else
	{
		/* No valid Bluetooth Stack ID exists. */
		ret_val = HOGP_INVALID_STACK_ID_ERROR;
	}

	return(ret_val);
}

/**
  ***************************************************************************************************************************************
  * The following function is a utility function that is used to
  * notify the Keyboard Report to the connected LE device (if the
  * device has registered for notifications).
  ***************************************************************************************************************************************
  */
static void HOGP_Notify_Keyboard_Report(ApplicationStateInfo_t *ApplicationStateInfo)
{
	int Result;
	DeviceInfo_t *DeviceInfo;
	HIDS_Report_Reference_Data_t ReportReferenceData;

	/* Make sure the input parameters are semi-valid. */
	if((ApplicationStateInfo) && (ApplicationStateInfo->Flags & APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED))
	{
		/* Search for the device info structure for this device. */
		if((DeviceInfo = HOGP_Search_Device_Info_Entry_By_BD_ADDR(&DeviceInfoList, ApplicationStateInfo->LEConnectionInfo.AddressType, ApplicationStateInfo->LEConnectionInfo.BD_ADDR)) != NULL)
		{
			/** Verify that the connection has registered for the current
			  * notifications based on the operating mode and that the link
			  * is currently encrypted.
			  */
			if(ApplicationStateInfo->LEConnectionInfo.Flags & CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED)
			{
				/* Check to see what characteristic should be notified based on the operating mode. */
				ReportReferenceData.ReportID   = 0;
				ReportReferenceData.ReportType = HIDS_REPORT_REFERENCE_REPORT_TYPE_INPUT_REPORT;

				/* Send the Report Mode Input Report notification. */
				Result = HIDS_Notify_Input_Report(ApplicationStateInfo->BluetoothStackID, ApplicationStateInfo->HIDSInstanceID, ApplicationStateInfo->LEConnectionInfo.ConnectionID, rtReport, &ReportReferenceData, HID_KEYBOARD_INPUT_REPORT_SIZE, ApplicationStateInfo->CurrentInputReport);

				ApplicationStateInfo->CurrentInputReport[0] = 0;
				ApplicationStateInfo->CurrentInputReport[2] = 0;
				/* Send the Report Mode Input Report notification. */
				Result |= HIDS_Notify_Input_Report(ApplicationStateInfo->BluetoothStackID, ApplicationStateInfo->HIDSInstanceID, ApplicationStateInfo->LEConnectionInfo.ConnectionID, rtReport, &ReportReferenceData, HID_KEYBOARD_INPUT_REPORT_SIZE, ApplicationStateInfo->CurrentInputReport);


				/* Check to see if any error occurred. */
				if(Result != HID_KEYBOARD_INPUT_REPORT_SIZE)
					{HOGP_DBG_Display(("Error - HIDS_Notify_Input_Report() returned %d for %s mode.\r\n", Result, (ApplicationStateInfo->HIDProtocolMode == pmBoot)?"Boot":"Keyboard"));}

				HOGP_DBG_Display(("No Error %d \r\n",DeviceInfo->ReportKeyboardInputConfiguration));
			}
			else{HOGP_DBG_Display(("Error %d \r\n",DeviceInfo->ReportKeyboardInputConfiguration));}
		}
	}
}

/**
  ***************************************************************************************************************************************
  * Check the mailbox status
  ***************************************************************************************************************************************
  */
uint8_t HOGP_Check_Mailbox_Status(void)
{
	if(ApplicationStateInfo.Mailbox){return 1;}
	else{return 0;}
}

/**
  ***************************************************************************************************************************************
  * Main HOGP task handler
  ***************************************************************************************************************************************
  */
void HOGP_Task_Handler(void)
{
	int Result;
	Byte_t MessageID;
	GAP_LE_Authentication_Response_Information_t GAP_LE_Authentication_Response_Information;

	/* Process the scheduler. */
	BTPS_ProcessScheduler();

	/* Wait on the application mailbox. */
	if(BTPS_WaitMailbox(ApplicationStateInfo.Mailbox, &MessageID))
	{
		switch(MessageID)
		{
			case APPLICATION_MAILBOX_MESSAGE_ID_LE_CONNECTED:
				/* Notify the battery level if necessary. */
				ApplicationStateInfo.LEConnectionInfo.connectionFlag=1;
				ApplicationStateInfo.BatteryLevel = BAT_Get_SOC();
				HOGP_Notify_Battery_Level(&ApplicationStateInfo, TRUE);
			break;

			case APPLICATION_MAILBOX_MESSAGE_ID_LE_DISCONNECTED:
				/* Reset the HID Protocol Mode to the default mode (Report Mode). */
				ApplicationStateInfo.HIDProtocolMode = pmReport;
				ApplicationStateInfo.LEConnectionInfo.connectionFlag=0;
				/* Start an advertising process. */
				HOGP_Start_Advertising(ApplicationStateInfo.BluetoothStackID);

				/* Clear the LE Connection Information. */
				BTPS_MemInitialize(&(ApplicationStateInfo.LEConnectionInfo), 0, sizeof(ApplicationStateInfo.LEConnectionInfo));

				/* Clear the LE Connection Flag. */
				ApplicationStateInfo.Flags &= ~APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED;
			break;

			case APPLICATION_MAILBOX_MESSAGE_ID_PASSKEY_ENTERED:
				HOGP_DBG_Display(("Responding with Passkey = %06lu.\r\n", ApplicationStateInfo.LEConnectionInfo.Passkey));

				/* Verify that we are still waiting on a passkey. */
				if(ApplicationStateInfo.LEConnectionInfo.Flags & CONNECTION_INFO_FLAGS_CONNECTION_AWAITING_PASSKEY)
				{
					/* Flag that we are no longer waiting on a passkey. */
					ApplicationStateInfo.LEConnectionInfo.Flags &= ~CONNECTION_INFO_FLAGS_CONNECTION_AWAITING_PASSKEY;

					/* Parameters appear to be valid, go ahead and */
					/* populate the response structure. */
					GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type  = larPasskey;
					GAP_LE_Authentication_Response_Information.Authentication_Data_Length  = (Byte_t)(sizeof(DWord_t));
					GAP_LE_Authentication_Response_Information.Authentication_Data.Passkey = (DWord_t)(ApplicationStateInfo.LEConnectionInfo.Passkey);

					/* Submit the Authentication Response. */
					Result = GAP_LE_Authentication_Response(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.LEConnectionInfo.BD_ADDR, &GAP_LE_Authentication_Response_Information);
					if(!Result){HOGP_DBG_Display(("Successfully responded with passkey.\r\n"));}
					else{HOGP_DBG_Display(("Error - GAP_LE_Authentication_Response() %d when responding with passkey.\r\n", Result));}
				}
			break;
		}
	}
}

/**
  ***************************************************************************************************************************************
  * @brief Send data report
  * @param Data (uint8_t*), size (uint8_t)
  * @retval None
  ***************************************************************************************************************************************
  */
void HOGP_Send_Data_Reports(uint8_t* _data, uint8_t _nbr)
{
	uint8_t _idx;

	if(ApplicationStateInfo.LEConnectionInfo.connectionFlag)
	{
		for(_idx=0;_idx<_nbr;_idx++)
		{
			ApplicationStateInfo.CurrentInputReport[0] = ((OGP_HID_KEYS[*_data]&0x80)?0x02:0x00);
			ApplicationStateInfo.CurrentInputReport[2] = (OGP_HID_KEYS[*_data++]&0x7F);
			HOGP_Notify_Keyboard_Report(&ApplicationStateInfo);
		}

		/* TAB */
		ApplicationStateInfo.CurrentInputReport[0] = ((OGP_HID_KEYS[9]&0x80)?0x02:0x00);
		ApplicationStateInfo.CurrentInputReport[2] = (OGP_HID_KEYS[9]&0x7F);
		HOGP_Notify_Keyboard_Report(&ApplicationStateInfo);
	}
}

/**
  ***************************************************************************************************************************************
  * @brief Update battery level
  * @param None
  * @retval None
  ***************************************************************************************************************************************
  */
void HOGP_Update_Battery_Level(void)
{
	if(ApplicationStateInfo.LEConnectionInfo.connectionFlag)
	{
		ApplicationStateInfo.BatteryLevel = BAT_Get_SOC();
		HOGP_Notify_Battery_Level(&ApplicationStateInfo, TRUE);
	}
}

/**
  ***************************************************************************************************************************************
  * @brief Get bluetooth connection state
  * @param None
  * @retval Status (uint8_t)
  ***************************************************************************************************************************************
  */
uint8_t HOGP_Get_Connection_Status(void)
{
	return ApplicationStateInfo.LEConnectionInfo.connectionFlag;
}

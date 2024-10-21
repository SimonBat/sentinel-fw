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
#include "display.h"
#include "bat.h"

/* The following is used as a printf replacement */
#define BT_HOGP_DBG_Display(_x) do{BTPS_OutputMessage _x;} while(0)
#define BT_HOGP_NUM_SUPPORTED_HCI_VERSIONS (sizeof(BT_HOGP_HCIVersionStrings)/sizeof(char *) - 1U)

static bt_hogp_application_state_info_ts BT_HOGP_ApplicationStateInfo; /* Container for all of the Application State Information */
static bt_hogp_GAPLE_parameters_ts BT_HOGP_LE_Parameters; /* Holds GAP Parameters like */
static bt_hogp_device_info_ts *BT_HOGP_DeviceInfoList; /* Holds the list head for the */
static Boolean_t BT_HOGP_SleepEnabledFlag;

/** Discoverability, Connectability Modes.
  * The Encryption Root Key should be generated
  * in such a way as to guarantee 128 bits of
  * entropy.
  */
static BTPSCONST Encryption_Key_t BT_HOGP_ER = {0x28, 0xBA, 0xE1, 0x35, 0x13, 0xB2, 0x20, 0x45, 0x16, 0xB2, 0x19, 0xD9, 0x80, 0xEE, 0x4A, 0x51};

/** The Identity Root Key should be generated
  * in such a way as to guarantee 128 bits of
  * entropy.
  */
static BTPSCONST Encryption_Key_t BT_HOGP_IR = {0x41, 0x09, 0xF4, 0x88, 0x09, 0x6B, 0x70, 0xC0, 0x95, 0x23, 0x3C, 0x8C, 0x48, 0xFC, 0xC9, 0xFE};

/** The following keys can be regerenated on the
  * fly using the constant IR and ER keys and
  * are used globally, for all devices.
  */
static Encryption_Key_t BT_HOGP_DHK;
static Encryption_Key_t BT_HOGP_IRK;

/** The following string table is used to map HCI Version information
  * to an easily displayable version string.
  */
static BTPSCONST char *BT_HOGP_HCIVersionStrings[] = {
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
static Byte_t BT_HOGP_KeyboardReportDescriptor[] = {
	0x05, 0x01, /* USAGE_PAGE (Generic Desktop) */
	0x09, 0x06, /* USAGE (Keyboard) */
	0xa1, 0x01, /* COLLECTION (Application) */
	0x05, 0x07, /* USAGE_PAGE (Keyboard) */
	0x19, 0xe0, /* USAGE_MINIMUM (Keyboard LeftControl) */
	0x29, 0xe7, /* USAGE_MAXIMUM (Keyboard Right GUI) */
	0x15, 0x00, /* LOGICAL_MINIMUM (0) */
	0x25, 0x01, /* LOGICAL_MAXIMUM (1) */
	0x75, 0x01, /* REPORT_SIZE (1) */
	0x95, 0x08, /* REPORT_COUNT (8) */
	0x81, 0x02, /* INPUT (Data,Var,Abs) */
	0x95, 0x01, /* REPORT_COUNT (1) */
	0x75, 0x08, /* REPORT_SIZE (8) */
	0x81, 0x03, /* INPUT (Cnst,Var,Abs) */
	0x95, 0x05, /* REPORT_COUNT (5) */
	0x75, 0x01, /* REPORT_SIZE (1) */
	0x05, 0x08, /* USAGE_PAGE (LEDs) */
	0x19, 0x01, /* USAGE_MINIMUM (Num Lock) */
	0x29, 0x05, /* USAGE_MAXIMUM (Kana) */
	0x91, 0x02, /* OUTPUT (Data,Var,Abs) */
	0x95, 0x01, /* REPORT_COUNT (1) */
	0x75, 0x03, /* REPORT_SIZE (3) */
	0x91, 0x03, /* OUTPUT (Cnst,Var,Abs) */
	0x95, 0x06, /* REPORT_COUNT (6) */
	0x75, 0x08, /* REPORT_SIZE (8) */
	0x15, 0x00, /* LOGICAL_MINIMUM (0) */
	0x25, 0x65, /* LOGICAL_MAXIMUM (101) */
	0x05, 0x07, /* USAGE_PAGE (Keyboard) */
	0x19, 0x00, /* USAGE_MINIMUM (Reserved (no event indicated)) */
	0x29, 0x65, /* USAGE_MAXIMUM (Keyboard Application) */
	0x81, 0x00, /* INPUT (Data,Ary,Abs) */
	0xc0        /* END_COLLECTION */
};

static const uint8_t BT_HOGP_OGP_HID_KEYS[128] = {
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
static void BT_HOGP_Display_Function_Error(char* _function, int _status);
static int BT_HOGP_Open_Stack(HCI_DriverInformation_t *_hciDriverInformation, BTPS_Initialization_t *_btpsInitialization);
static int BT_HOGP_Close_Stack(void);
static void BT_HOGP_Free_Device_Info_List(bt_hogp_device_info_ts **_listHead);
static void BT_HOGP_BD_ADDR_To_Str(BD_ADDR_t _boardAddress, BT_HOGP_BOARD_STR_T _boardStr);
static int BT_HOGP_Configure_DIS(void);
static int BT_HOGP_Configure_BAS(void);
static int BT_HOGP_Configure_HIDS(void);
static bt_hogp_device_info_ts * BT_HOGP_Search_Device_Info_Entry_By_BD_ADDR(bt_hogp_device_info_ts **_listHead, \
																			GAP_LE_Address_Type_t _addressType, BD_ADDR_t _bdADDR);
static void BT_HOGP_Format_Advertising_Data(unsigned int _bluetoothStackID);
static int BT_HOGP_Set_Pairable(void);
static void BT_HOGP_Post_Application_Mailbox(Byte_t _messageID);
static Boolean_t BT_HOGB_Create_New_Device_Info_Entry(bt_hogp_device_info_ts **_listHead, \
													  GAP_LE_Address_Type_t _connectionAddressType, BD_ADDR_t _connectionBdAddr);
static int BT_HOGP_Slave_Security_ReEstablishment(unsigned int _bluetoothStackID, BD_ADDR_t _bdAddr);
static bt_hogp_device_info_ts *BT_HOGP_Delete_Device_Info_Entry(bt_hogp_device_info_ts **_listHead, \
																GAP_LE_Address_Type_t _addressType, BD_ADDR_t _bdAddr);
static void BT_HOGP_Free_Device_Info_Entry_Memory(bt_hogp_device_info_ts *_entryToFree);
static void BT_HOGP_Display_Pairing_Information(GAP_LE_Pairing_Capabilities_t _pairingCapabilities);
static int BT_HOGP_Slave_Pairing_Request_Response(unsigned int _bluetoothStackID, BD_ADDR_t _bdAddr);
static void BT_HOGP_Configure_Capabilities(GAP_LE_Pairing_Capabilities_t *_capabilities);
static int BT_HOGP_Encryption_Information_Request_Response(unsigned int _bluetoothStackID, BD_ADDR_t _bdAddr, Byte_t _keySize, \
														   GAP_LE_Authentication_Response_Information_t *_gapLeAuthenticationResponseInformation);
static int BT_HOGP_Disconnect_LE_Device(unsigned int _bluetoothStackID, BD_ADDR_t _bdAddr);
static void BTPSAPI BT_HOGP_GATT_Connection_Event_Callback(unsigned int _bluetoothStackID, \
														   GATT_Connection_Event_Data_t *_gattConnectionEventData, unsigned long _callbackParameter);
static void BTPSAPI BT_HOGP_BAS_Event_Callback(unsigned int _bluetoothStackID, BAS_Event_Data_t *_basEventData, unsigned long _callbackParameter);
static void BTPSAPI BT_HOGP_HIDS_Event_Callback(unsigned int _bluetoothStackID, HIDS_Event_Data_t *_hidsEventData, unsigned long _callbackParameter);
static void BTPSAPI BT_HOGP_GAP_LE_Event_Callback(unsigned int _bluetoothStackID, GAP_LE_Event_Data_t *_gapLEEventData, unsigned long _callbackParameter);
static void BTPSAPI BT_HOGP_BSC_TimerCallback(unsigned int _bluetoothStackID, unsigned int _timerID, unsigned long _callbackParameter);
static void BTPSAPI BT_HOGP_HCI_Sleep_Callback(Boolean_t _sleepAllowed, unsigned long _callbackParameter);
static void BT_HOGP_Notify_Battery_Level(bt_hogp_application_state_info_ts *_applicationStateInfo, Boolean_t _force);
static int BT_HOGP_Start_Advertising(unsigned int _bluetoothStackID);
static void BT_HOGP_Notify_Keyboard_Report(bt_hogp_application_state_info_ts *_applicationStateInfo);

/**
  ***************************************************************************************************************************************
  * @brief Displays a function error
  * @param Pointer to function (char*), Status (int)
  * @retval None
  ***************************************************************************************************************************************
  */
static void BT_HOGP_Display_Function_Error(char* _function, int _status)
{
	BT_HOGP_DBG_Display(("%s Failed: %d.\r\n", _function, _status));
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
int BT_HOGP_Application_Init(HCI_DriverInformation_t *_hciDriverInformation, BTPS_Initialization_t *_btpsInitialization)
{
	int _retVal = BT_HOGP_APPLICATION_ERROR_UNABLE_TO_OPEN_STACK;
	HCI_HCILLConfiguration_t HCILLConfig;
	HCI_Driver_Reconfigure_Data_t DriverReconfigureData;

	/* Next, makes sure that the Driver Information passed appears to be semi-valid. */
	if((_hciDriverInformation) && (_btpsInitialization))
	{
		/* Try to Open the stack and check if it was successful. */
		if(!BT_HOGP_Open_Stack(_hciDriverInformation, _btpsInitialization))
		{
			/* Now that the device is discoverable attempt to make it pairable */
			_retVal = BT_HOGP_Set_Pairable();

			if(!_retVal)
			{
				/* Create the Application Mailbox */
				if(NULL != (BT_HOGP_ApplicationStateInfo.Mailbox = BTPS_CreateMailbox(BT_HOGP_APPLICATION_MAILBOX_DEPTH, BT_HOGP_APPLICATION_MAILBOX_SIZE)))
				{
					/* Post some messages to the application to kick start the application */
					BT_HOGP_Post_Application_Mailbox(BT_HOGP_APPLICATION_MAILBOX_MESSAGE_ID_LE_DISCONNECTED);

					/* Register a sleep mode callback if we are using HCILL Mode */
					if((cpHCILL == _hciDriverInformation->DriverInformation.COMMDriverInformation.Protocol)|| \
					   (cpHCILL_RTS_CTS == _hciDriverInformation->DriverInformation.COMMDriverInformation.Protocol))
					{
						HCILLConfig.SleepCallbackFunction = BT_HOGP_HCI_Sleep_Callback;
						HCILLConfig.SleepCallbackParameter = 0U;
						DriverReconfigureData.ReconfigureCommand = HCI_COMM_DRIVER_RECONFIGURE_DATA_COMMAND_CHANGE_HCILL_PARAMETERS;
						DriverReconfigureData.ReconfigureData = (void *)&HCILLConfig;

						/** Register the sleep mode callback.  Note that if
						  * this function returns greater than 0 then sleep is
						  * currently enabled.
						  */
						_retVal = HCI_Reconfigure_Driver(BT_HOGP_ApplicationStateInfo.BluetoothStackID, FALSE, &DriverReconfigureData);

						if(_retVal > 0)
						{
							/* Flag that sleep mode is enabled */
							BT_HOGP_SleepEnabledFlag = TRUE;
						}
					}

					/* Return success to the caller */
					_retVal = (int)BT_HOGP_ApplicationStateInfo.BluetoothStackID;
				}
				else
				{
					BT_HOGP_DBG_Display(("Failed to create application mailbox.\r\n"));
					_retVal = BT_HOGP_UNABLE_TO_INITIALIZE_STACK;
				}
			}
			else{BT_HOGP_Display_Function_Error("SetPairable", _retVal);}

			/* In some error occurred then close the stack */
			if(_retVal < 0)
			{
				/* Close the Bluetooth Stack */
				BT_HOGP_Close_Stack();
			}
		}
		else
		{
			/* There was an error while attempting to open the Stack */
			BT_HOGP_DBG_Display(("Unable to open the stack.\r\n"));
		}
	}
	else{_retVal = BT_HOGP_APPLICATION_ERROR_INVALID_PARAMETERS;}

	return (_retVal);
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
static int BT_HOGP_Open_Stack(HCI_DriverInformation_t *_hciDriverInformation, BTPS_Initialization_t *_btpsInitialization)
{
	int _result;
	int _retVal = 0;
	char _bluetoothAddress[16];
	Byte_t _status;
	BD_ADDR_t _bdAddr;
	unsigned int _serviceID;
	HCI_Version_t _hciVersion;
	L2CA_Link_Connect_Params_t _l2caLinkConnectParams;

	/* Next, makes sure that the Driver Information passed appears to be semi-valid */
	if(_hciDriverInformation)
	{
		BT_HOGP_DBG_Display(("\r\n"));
		/* Initialize BTPSKNRl */
		BTPS_Init((void *)_btpsInitialization);
		BT_HOGP_DBG_Display(("OpenStack().\r\n"));
		/* Clear the application state information */
		BTPS_MemInitialize(&BT_HOGP_ApplicationStateInfo, 0U, sizeof(BT_HOGP_ApplicationStateInfo));
		/* Initialize the Stack */
		_result = BSC_Initialize(_hciDriverInformation, 0U);

		/* Next, check the return value of the initialization to see if it was successful */
		if(_result > 0)
		{
			/** The Stack was initialized successfully, inform the user and
			  * set the return value of the initialization function to the
			  * Bluetooth Stack ID.
			  */
			BT_HOGP_ApplicationStateInfo.BluetoothStackID = _result;
			BT_HOGP_DBG_Display(("Bluetooth Stack ID: %d.\r\n", BT_HOGP_ApplicationStateInfo.BluetoothStackID));

			/* Initialize the Default Pairing Parameters */
			BT_HOGP_LE_Parameters.IOCapability = licNoInputNoOutput;
			BT_HOGP_LE_Parameters.MITMProtection = FALSE;
			BT_HOGP_LE_Parameters.OOBDataPresent = FALSE;

			if(!HCI_Version_Supported(BT_HOGP_ApplicationStateInfo.BluetoothStackID, &_hciVersion))
			{BT_HOGP_DBG_Display(("Device Chipset: %s.\r\n", (_hciVersion <= BT_HOGP_NUM_SUPPORTED_HCI_VERSIONS)? \
				BT_HOGP_HCIVersionStrings[_hciVersion]:BT_HOGP_HCIVersionStrings[BT_HOGP_NUM_SUPPORTED_HCI_VERSIONS]));}

			/* Let's output the Bluetooth Device Address so that the user knows what the Device Address is */
			if(!GAP_Query_Local_BD_ADDR(BT_HOGP_ApplicationStateInfo.BluetoothStackID, &_bdAddr))
			{
				BT_HOGP_BD_ADDR_To_Str(_bdAddr, _bluetoothAddress);
				BT_HOGP_DBG_Display(("BD_ADDR: %s\r\n", _bluetoothAddress));
			}

			/* Go ahead and allow Master/Slave Role Switch */
			_l2caLinkConnectParams.L2CA_Link_Connect_Request_Config = cqAllowRoleSwitch;
			_l2caLinkConnectParams.L2CA_Link_Connect_Response_Config = csMaintainCurrentRole;

			L2CA_Set_Link_Connection_Configuration(BT_HOGP_ApplicationStateInfo.BluetoothStackID, &_l2caLinkConnectParams);

			if(HCI_Command_Supported(BT_HOGP_ApplicationStateInfo.BluetoothStackID, HCI_SUPPORTED_COMMAND_WRITE_DEFAULT_LINK_POLICY_BIT_NUMBER) > 0)
			{HCI_Write_Default_Link_Policy_Settings(BT_HOGP_ApplicationStateInfo.BluetoothStackID, \
												   (HCI_LINK_POLICY_SETTINGS_ENABLE_MASTER_SLAVE_SWITCH | HCI_LINK_POLICY_SETTINGS_ENABLE_SNIFF_MODE), \
												   &_status);}

			/* Regenerate IRK and DHK from the constant Identity Root Key */
			GAP_LE_Diversify_Function(BT_HOGP_ApplicationStateInfo.BluetoothStackID, (Encryption_Key_t *)(&BT_HOGP_IR), 1,0, &BT_HOGP_IRK);
			GAP_LE_Diversify_Function(BT_HOGP_ApplicationStateInfo.BluetoothStackID, (Encryption_Key_t *)(&BT_HOGP_IR), 3, 0, &BT_HOGP_DHK);

			/* Initialize the GATT Service */
			if(!(_result = GATT_Initialize(BT_HOGP_ApplicationStateInfo.BluetoothStackID, \
										   GATT_INITIALIZATION_FLAGS_SUPPORT_LE, BT_HOGP_GATT_Connection_Event_Callback, 0)))
			{
				/* Initialize the GAPS Service */
				_result = GAPS_Initialize_Service(BT_HOGP_ApplicationStateInfo.BluetoothStackID, &_serviceID);

				if(_result > 0)
				{
					/* Save the Instance ID of the GAP Service */
					BT_HOGP_ApplicationStateInfo.GAPSInstanceID = (unsigned int)_result;
					/* Set the GAP Device Name and Device Appearance */
					GAPS_Set_Device_Name(BT_HOGP_ApplicationStateInfo.BluetoothStackID, \
										 BT_HOGP_ApplicationStateInfo.GAPSInstanceID, BT_HOGP_LE_DEMO_DEVICE_NAME);
					GAPS_Set_Device_Appearance(BT_HOGP_ApplicationStateInfo.BluetoothStackID, \
											   BT_HOGP_ApplicationStateInfo.GAPSInstanceID, GAP_DEVICE_APPEARENCE_VALUE_HID_KEYBOARD);

					/* Atempt to configure the DIS Service Instance */
					_result = BT_HOGP_Configure_DIS();

					if(!_result)
					{
						/* Atempt to configure the BAS Service Instance */
						_result = BT_HOGP_Configure_BAS();

						if(!_result)
						{
							/* Atempt to configure the HID Service Instance */
							_result = BT_HOGP_Configure_HIDS();

							if(!_result)
							{
								/* Format the Advertising Data */
								BT_HOGP_Format_Advertising_Data(BT_HOGP_ApplicationStateInfo.BluetoothStackID);
								/* Reset the HID Protocol Mode to the default mode (Report Mode) */
								BT_HOGP_ApplicationStateInfo.HIDProtocolMode = pmReport;
								/* Configure the Battery Level to be at 100% */
								BT_HOGP_ApplicationStateInfo.BatteryLevel = BAT_Get_SOC();
								/* Return success to the caller */
								_retVal = 0;
							}
							else
							{
								/** The Stack was NOT initialized successfully,
								  * inform the user and set the return value of
								  * the initialization function to an error.
								  */
								BT_HOGP_Display_Function_Error("ConfigureHIDS", _result);
								_retVal = BT_HOGP_UNABLE_TO_INITIALIZE_STACK;
							}
						}
						else
						{
							/** The Stack was NOT initialized successfully,
							  * inform the user and set the return value of the
							  * initialization function to an error.
							  */
							BT_HOGP_Display_Function_Error("ConfigureBAS", _result);
							_retVal = BT_HOGP_UNABLE_TO_INITIALIZE_STACK;
						}
					}
					else
					{
						/** The Stack was NOT initialized successfully, inform
						  * the user and set the return value of the
						  * initialization function to an error.
						  */
						BT_HOGP_Display_Function_Error("ConfigureDIS", _result);
						_retVal = BT_HOGP_UNABLE_TO_INITIALIZE_STACK;
					}
				}
				else
				{
					/** The Stack was NOT initialized successfully, inform the
					  * user and set the return value of the initialization
					  * function to an error.
					  */
					BT_HOGP_Display_Function_Error("GAPS_Initialize_Service", _result);
					_retVal = BT_HOGP_UNABLE_TO_INITIALIZE_STACK;
				}

				/* Shutdown the stack if an error occurred */
				if(_retVal < 0){BT_HOGP_Close_Stack();}
			}
			else
			{
				/** The Stack was NOT initialized successfully, inform the
				  * user and set the return value of the initialization
				  * function to an error.
				  */
				BT_HOGP_Display_Function_Error("GATT_Initialize", _result);
				/* Shutdown the stack */
				BT_HOGP_Close_Stack();
				_retVal = BT_HOGP_UNABLE_TO_INITIALIZE_STACK;
			}
		}
		else
		{
			/** The Stack was NOT initialized successfully, inform the user
			  * and set the return value of the initialization function to
			  * an error.
			  */
			BT_HOGP_Display_Function_Error("BSC_Initialize", _result);
			BT_HOGP_ApplicationStateInfo.BluetoothStackID = 0U;
			_retVal = BT_HOGP_UNABLE_TO_INITIALIZE_STACK;
		}
	}
	else
	{
		/* One or more of the necessary parameters are invalid */
		_retVal = BT_HOGP_INVALID_PARAMETERS_ERROR;
	}

	return (_retVal);
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
static int BT_HOGP_Close_Stack(void)
{
	int _retVal = 0;

	/* First check to see if the Stack has been opened */
	if(BT_HOGP_ApplicationStateInfo.BluetoothStackID)
	{
		/* Cleanup GAP Service Module */
		if(BT_HOGP_ApplicationStateInfo.GAPSInstanceID)
		{GAPS_Cleanup_Service(BT_HOGP_ApplicationStateInfo.BluetoothStackID, BT_HOGP_ApplicationStateInfo.GAPSInstanceID);}

		/* Cleanup DIS Service Module */
		if(BT_HOGP_ApplicationStateInfo.DISInstanceID)
		{DIS_Cleanup_Service(BT_HOGP_ApplicationStateInfo.BluetoothStackID, BT_HOGP_ApplicationStateInfo.DISInstanceID);}

		/* Cleanup BAS Service Module */
		if(BT_HOGP_ApplicationStateInfo.BASInstanceID)
		{BAS_Cleanup_Service(BT_HOGP_ApplicationStateInfo.BluetoothStackID, BT_HOGP_ApplicationStateInfo.BASInstanceID);}

		if(BT_HOGP_ApplicationStateInfo.SPPServerPortID)
		{
			SPP_Un_Register_SDP_Record(BT_HOGP_ApplicationStateInfo.BluetoothStackID, \
									   BT_HOGP_ApplicationStateInfo.SPPServerPortID, \
									   BT_HOGP_ApplicationStateInfo.SPPServerSDPHandle);
			SPP_Close_Server_Port(BT_HOGP_ApplicationStateInfo.BluetoothStackID, BT_HOGP_ApplicationStateInfo.SPPServerPortID);
		}

		/* Free the Device Information List */
		BT_HOGP_Free_Device_Info_List(&BT_HOGP_DeviceInfoList);
		/* Cleanup GATT Module */
		GATT_Cleanup(BT_HOGP_ApplicationStateInfo.BluetoothStackID);
		/* Simply close the Stack */
		BSC_Shutdown(BT_HOGP_ApplicationStateInfo.BluetoothStackID);
		/* Free BTPSKRNL allocated memory */
		BTPS_DeInit();
		BT_HOGP_DBG_Display(("Stack Shutdown.\r\n"));
		/* Flag that the Stack is no longer initialized */
		BTPS_MemInitialize(&BT_HOGP_ApplicationStateInfo, 0U, sizeof(BT_HOGP_ApplicationStateInfo));
		/* Flag success to the caller */
		_retVal = 0;
	}
	else
	{
		/* A valid Stack ID does not exist, inform to user */
		_retVal = BT_HOGP_UNABLE_TO_INITIALIZE_STACK;
	}

	return _retVal;
}

/**
  ***************************************************************************************************************************************
  * The following function deletes (and free's all memory) every
  * element of the specified Key Info List. Upon return of this
  * function, the Head Pointer is set to NULL.
  ***************************************************************************************************************************************
  */
static void BT_HOGP_Free_Device_Info_List(bt_hogp_device_info_ts **_listHead)
{
	BSC_FreeGenericListEntryList((void **)(_listHead), BTPS_STRUCTURE_OFFSET(bt_hogp_device_info_ts, NextDeviceInfoPtr));
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
static void BT_HOGP_BD_ADDR_To_Str(BD_ADDR_t _boardAddress, BT_HOGP_BOARD_STR_T _boardStr)
{
	BTPS_SprintF((char *)_boardStr, "0x%02X%02X%02X%02X%02X%02X", \
				 _boardAddress.BD_ADDR5, \
				 _boardAddress.BD_ADDR4, \
				 _boardAddress.BD_ADDR3, \
				 _boardAddress.BD_ADDR2, \
				 _boardAddress.BD_ADDR1, \
				 _boardAddress.BD_ADDR0);
}

/**
  ***************************************************************************************************************************************
  * The following function is a utility function that is used to
  * configure the Device Information Service that is registered by
  * this application. This function returns ZERO if succesful or a
  * negative error code.
  ***************************************************************************************************************************************
  */
static int BT_HOGP_Configure_DIS(void)
{
	int _retVal;
	unsigned int _serviceId;
	DIS_PNP_ID_Data_t _pnpId;

	/* Initialize the DIS Service */
	_retVal = DIS_Initialize_Service(BT_HOGP_ApplicationStateInfo.BluetoothStackID, &_serviceId);

	if(_retVal > 0)
	{
		BT_HOGP_DBG_Display(("Device Information Service registered, Service ID = %u.\r\n", _serviceId));
		/* Save the Instance ID of the DIS Service */
		BT_HOGP_ApplicationStateInfo.DISInstanceID = (unsigned int)_retVal;

		/* Set the DIS Manufacturer Name */
		if(!(_retVal = DIS_Set_Manufacturer_Name(BT_HOGP_ApplicationStateInfo.BluetoothStackID, \
												 BT_HOGP_ApplicationStateInfo.DISInstanceID, \
												 BTPS_VERSION_PRODUCT_NAME_STRING)))
		{
			/* Set the DIS Software Revision */
			if(!(_retVal = DIS_Set_Software_Revision(BT_HOGP_ApplicationStateInfo.BluetoothStackID, \
													 BT_HOGP_ApplicationStateInfo.DISInstanceID, \
													 BTPS_VERSION_VERSION_STRING)))
			{
				/* Configure the PNP ID value to use */
				_pnpId.VendorID_Source = DIS_PNP_ID_VENDOR_SOURCE_BLUETOOTH_SIG;
				_pnpId.VendorID = BT_HOGP_PNP_ID_VENDOR_ID_STONESTREET_ONE;
				_pnpId.ProductID = BT_HOGP_PNP_ID_PRODUCT_ID;
				_pnpId.ProductVersion = BT_HOGP_PNP_ID_PRODUCT_VERSION;

				/* Finally set the PNP ID */
				_retVal = DIS_Set_PNP_ID(BT_HOGP_ApplicationStateInfo.BluetoothStackID, \
										 BT_HOGP_ApplicationStateInfo.DISInstanceID, &_pnpId);
			}
		}
	}
	else{BT_HOGP_DBG_Display(("Error - DIS_Initialize_Service() %d.\r\n", _retVal));}

	return(_retVal);
}

/**
  ***************************************************************************************************************************************
  * The following function is a utility function that is used to
  * configure the Battery Service that is registered by this
  * application.  This function returns ZERO if succesful or a
  * negative error code.
  ***************************************************************************************************************************************
  */
static int BT_HOGP_Configure_BAS(void)
{
	int _retVal;
	unsigned int _serviceId;

	/* Initialize the BAS Service */
	_retVal = BAS_Initialize_Service(BT_HOGP_ApplicationStateInfo.BluetoothStackID, BT_HOGP_BAS_Event_Callback, 0U, &_serviceId);

	if(_retVal > 0)
	{
		BT_HOGP_DBG_Display(("Battery Service registered, Service ID = %u.\r\n", _serviceId));
		/* Save the Instance ID of the BAS Service */
		BT_HOGP_ApplicationStateInfo.BASInstanceID = (unsigned int)_retVal;
		_retVal = 0;
	}
	else{BT_HOGP_DBG_Display(("Error - BAS_Initialize_Service() %d.\r\n", _retVal));}

	return _retVal;
}

/**
  ***************************************************************************************************************************************
  * The following function is a utility function that is used to
  * configure the HID Service that is registered by this application.
  * This function returns ZERO if succesful or a negative error code.
  ***************************************************************************************************************************************
  */
static int BT_HOGP_Configure_HIDS(void)
{
	int _retVal;
	unsigned int _serviceId;
	HIDS_HID_Information_Data_t _hidInformation;
	HIDS_Report_Reference_Data_t _reportReferenceData[2];

	/* Configure the HID Information value. */
	_hidInformation.CountryCode = HIDS_HID_LOCALIZATION_BYTE_NO_LOCALIZATION;
	_hidInformation.Flags = HIDS_HID_INFORMATION_FLAGS_NORMALLY_CONNECTABLE;
	_hidInformation.Version = HIDS_HID_VERSION_NUMBER;

	/** Configure the Report Reference structures.  Note that since we
	  * have only 1 report of a type (Input,Output,Feature) we do not need
	  * to have a unique Reference ID and therefore we use a Report ID of
	  * ZERO.
	  */
	_reportReferenceData[0].ReportID = 0;
	_reportReferenceData[0].ReportType = HIDS_REPORT_REFERENCE_REPORT_TYPE_INPUT_REPORT;
	_reportReferenceData[1].ReportID = 0;
	_reportReferenceData[1].ReportType = HIDS_REPORT_REFERENCE_REPORT_TYPE_OUTPUT_REPORT;

	/* Initialize the HID Service. */
	_retVal = HIDS_Initialize_Service(BT_HOGP_ApplicationStateInfo.BluetoothStackID, HIDS_FLAGS_SUPPORT_KEYBOARD, \
									  &_hidInformation, 0, NULL, 0, NULL, (sizeof(_reportReferenceData)/sizeof(HIDS_Report_Reference_Data_t)), \
									  _reportReferenceData, BT_HOGP_HIDS_Event_Callback, 0, &_serviceId);
	if(_retVal > 0)
	{
		BT_HOGP_DBG_Display(("HID Service registered, Service ID = %u.\r\n", _serviceId));
		/* Save the Instance ID of the HID Service */
		BT_HOGP_ApplicationStateInfo.HIDSInstanceID = (unsigned int)_retVal;
		_retVal = 0;
	}
	else{BT_HOGP_DBG_Display(("Error - HIDS_Initialize_Service() %d.\r\n", _retVal));}

	return _retVal;
}

/**
  ***************************************************************************************************************************************
  * The following function searches the specified List for the
  * specified Connection BD_ADDR.  This function returns NULL if
  * either the List Head is invalid, the BD_ADDR is invalid, or the
  * Connection BD_ADDR was NOT found.
  ***************************************************************************************************************************************
  */
static bt_hogp_device_info_ts * BT_HOGP_Search_Device_Info_Entry_By_BD_ADDR(bt_hogp_device_info_ts **_listHead, \
																			GAP_LE_Address_Type_t _addressType, BD_ADDR_t _bdADDR)
{
	bt_hogp_device_info_ts *_deviceInfo;

	/* Verify that the input parameters are semi-valid. */
	if((_listHead) && (!COMPARE_NULL_BD_ADDR(_bdADDR)))
	{
		/* Check to see if this is a resolvable address type.  If so we will search the list based on the IRK. */
		if((latRandom ==_addressType) && (GAP_LE_TEST_RESOLVABLE_ADDRESS_BITS(_bdADDR)))
		{
			/* Walk the list and attempt to resolve this entry to an existing entry with IRK. */
			_deviceInfo = *_listHead;

			while(_deviceInfo)
			{
				/* Check to see if the IRK is valid */
				if(_deviceInfo->Flags & BT_HOGP_DEVICE_INFO_FLAGS_IRK_VALID)
				{
					/* Attempt to resolve this address with the stored IRK. */
					if(GAP_LE_Resolve_Address(BT_HOGP_ApplicationStateInfo.BluetoothStackID, &(_deviceInfo->IRK), _bdADDR))
					{
						/* Address resolved so just exit from the loop. */
						break;
					}
				}

				_deviceInfo = _deviceInfo->NextDeviceInfoPtr;
			}
		}
		else{_deviceInfo = NULL;}

		/* If all else fail we will attempt to search the list by just the BD_ADDR */
		if(NULL == _deviceInfo)
		{_deviceInfo = BSC_SearchGenericListEntry(ekBD_ADDR_t, (void *)(&_bdADDR), BTPS_STRUCTURE_OFFSET(bt_hogp_device_info_ts, BD_ADDR), \
												  BTPS_STRUCTURE_OFFSET(bt_hogp_device_info_ts, NextDeviceInfoPtr), (void **)(_listHead));}
	}else{_deviceInfo = NULL;}

	return _deviceInfo;
}

/**
  ***************************************************************************************************************************************
  * The following function is a utility function that formats the advertising data.
  ***************************************************************************************************************************************
  */
static void BT_HOGP_Format_Advertising_Data(unsigned int _bluetoothStackID)
{
	int _result;
	unsigned int _length;
	unsigned int _stringLength;
	union{
		Advertising_Data_t AdvertisingData;
		Scan_Response_Data_t ScanResponseData;
	}_advertisementDataBuffer;

	/* First, check that valid Bluetooth Stack ID exists. */
	if(_bluetoothStackID)
	{
		BTPS_MemInitialize(&(_advertisementDataBuffer.AdvertisingData), 0U, sizeof(Advertising_Data_t));

		_length = 0U;
		/* Set the Flags A/D Field (1 byte type and 1 byte Flags. */
		_advertisementDataBuffer.AdvertisingData.Advertising_Data[0] = 2;
		_advertisementDataBuffer.AdvertisingData.Advertising_Data[1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_FLAGS;
		_advertisementDataBuffer.AdvertisingData.Advertising_Data[2] = HCI_LE_ADVERTISING_FLAGS_GENERAL_DISCOVERABLE_MODE_FLAGS_BIT_MASK;
		_advertisementDataBuffer.AdvertisingData.Advertising_Data[2] |= HCI_LE_ADVERTISING_FLAGS_BR_EDR_NOT_SUPPORTED_FLAGS_BIT_MASK;
		_length += _advertisementDataBuffer.AdvertisingData.Advertising_Data[0] + 1U;
		/* Configure the Device Appearance value. */
		_advertisementDataBuffer.AdvertisingData.Advertising_Data[_length] = 3;
		_advertisementDataBuffer.AdvertisingData.Advertising_Data[_length + 1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_APPEARANCE;
		ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(_advertisementDataBuffer.AdvertisingData.Advertising_Data[_length + 2U]), \
														 GAP_DEVICE_APPEARENCE_VALUE_HID_KEYBOARD);
		_length += _advertisementDataBuffer.AdvertisingData.Advertising_Data[_length] + 1U;
		/* Configure the services that we say we support. */
		_advertisementDataBuffer.AdvertisingData.Advertising_Data[_length] = 1U + (UUID_16_SIZE * 3U);
		_advertisementDataBuffer.AdvertisingData.Advertising_Data[_length + 1U] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_16_BIT_SERVICE_UUID_COMPLETE;
		HIDS_ASSIGN_HIDS_SERVICE_UUID_16(&(_advertisementDataBuffer.AdvertisingData.Advertising_Data[_length + 2U]));
		BAS_ASSIGN_BAS_SERVICE_UUID_16(&(_advertisementDataBuffer.AdvertisingData.Advertising_Data[_length + 4U]));
		DIS_ASSIGN_DIS_SERVICE_UUID_16(&(_advertisementDataBuffer.AdvertisingData.Advertising_Data[_length + 6U]));
		_length += _advertisementDataBuffer.AdvertisingData.Advertising_Data[_length] + 1U;

		/* Set the Device Name String. */
		_stringLength = BTPS_StringLength(BT_HOGP_LE_DEMO_DEVICE_NAME);
		if(_stringLength < (ADVERTISING_DATA_MAXIMUM_SIZE - _length - 2U))
		{_advertisementDataBuffer.AdvertisingData.Advertising_Data[_length + 1U] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_LOCAL_NAME_COMPLETE;}
		else
		{
			_advertisementDataBuffer.AdvertisingData.Advertising_Data[_length + 1U] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_LOCAL_NAME_SHORTENED;
			_stringLength = (ADVERTISING_DATA_MAXIMUM_SIZE - _length - 2U);
		}

		_advertisementDataBuffer.AdvertisingData.Advertising_Data[_length] = _stringLength + 1U;
		BTPS_MemCopy(&(_advertisementDataBuffer.AdvertisingData.Advertising_Data[_length + 2U]), BT_HOGP_LE_DEMO_DEVICE_NAME, _stringLength);
		_length += _advertisementDataBuffer.AdvertisingData.Advertising_Data[_length] + 1U;
		/* Write thee advertising data to the chip. */
		_result = GAP_LE_Set_Advertising_Data(_bluetoothStackID, _length, &(_advertisementDataBuffer.AdvertisingData));

		if(!_result){BT_HOGP_DBG_Display(("Advertising Data Configured Successfully.\r\n"));}
		else{BT_HOGP_DBG_Display(("GAP_LE_Set_Advertising_Data(dtAdvertising) returned %d.\r\n", _result));}
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
static int BT_HOGP_Set_Pairable(void)
{
	int _result;
	int _retVal = 0;

	/* First, check that a valid Bluetooth Stack ID exists. */
	if(BT_HOGP_ApplicationStateInfo.BluetoothStackID)
	{
		/* Set the LE Pairability Mode. */
		/* Attempt to set the attached device to be pairable. */
		_result = GAP_LE_Set_Pairability_Mode(BT_HOGP_ApplicationStateInfo.BluetoothStackID, lpmPairableMode);

		/* Next, check the return value of the GAP Set Pairability mode command for successful execution */
		if(!_result)
		{
			/** The device has been set to pairable mode, now register an
			  * Authentication Callback to handle the Authentication events
			  * if required.
			  */
			_result = GAP_LE_Register_Remote_Authentication(BT_HOGP_ApplicationStateInfo.BluetoothStackID, \
															BT_HOGP_GAP_LE_Event_Callback, (unsigned long)0);

			/* Next, check the return value of the GAP Register Remote Authentication command for successful execution. */
			if(_result)
			{
				/* An error occurred while trying to execute this function. */
				BT_HOGP_Display_Function_Error("GAP_LE_Register_Remote_Authentication", _result);
				_retVal = _result;
			}
		}
		else
		{
			/* An error occurred while trying to make the device pairable. */
			BT_HOGP_Display_Function_Error("GAP_LE_Set_Pairability_Mode", _result);
			_retVal = _result;
		}
	}
	else
	{
		/* No valid Bluetooth Stack ID exists. */
		_retVal = BT_HOGP_INVALID_STACK_ID_ERROR;
	}

	return _retVal;
}

/**
  ***************************************************************************************************************************************
  * The following function is a utility function that is provided to
  * allow a mechanism of posting into the application mailbox.
  ***************************************************************************************************************************************
  */
static void BT_HOGP_Post_Application_Mailbox(Byte_t _messageID)
{
	/* Post to the application mailbox. */
	BTPS_AddMailbox(BT_HOGP_ApplicationStateInfo.Mailbox, (void *)&_messageID);
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
static Boolean_t BT_HOGB_Create_New_Device_Info_Entry(bt_hogp_device_info_ts **_listHead, \
													  GAP_LE_Address_Type_t _connectionAddressType, BD_ADDR_t _connectionBdAddr)
{
	Boolean_t _retVal = FALSE;
	bt_hogp_device_info_ts *_deviceInfoPtr;

	/* Verify that the passed in parameters seem semi-valid. */
	if((_listHead) && (!COMPARE_NULL_BD_ADDR(_connectionBdAddr)))
	{
		/* Allocate the memory for the entry. */
		if((_deviceInfoPtr = BTPS_AllocateMemory(sizeof(bt_hogp_device_info_ts))) != NULL)
		{
			/* Initialize the entry. */
			BTPS_MemInitialize(_deviceInfoPtr, 0U, sizeof(bt_hogp_device_info_ts));
			_deviceInfoPtr->AddressType = _connectionAddressType;
			_deviceInfoPtr->BD_ADDR     = _connectionBdAddr;

			_retVal = BSC_AddGenericListEntry_Actual(ekBD_ADDR_t, BTPS_STRUCTURE_OFFSET(bt_hogp_device_info_ts, BD_ADDR), \
													 BTPS_STRUCTURE_OFFSET(bt_hogp_device_info_ts, NextDeviceInfoPtr), (void **)(_listHead), (void *)(_deviceInfoPtr));
			if(!_retVal)
			{
				/* Failed to add to list so we should free the memory that we allocated for the entry. */
				BTPS_FreeMemory(_deviceInfoPtr);
			}
		}
	}

	return _retVal;
}

/**
  ***************************************************************************************************************************************
  * The following function provides a mechanism of attempting to
  * re-established security with a previously paired master.
  ***************************************************************************************************************************************
  */
static int BT_HOGP_Slave_Security_ReEstablishment(unsigned int _bluetoothStackID, BD_ADDR_t _bdAddr)
{
	int _retVal;
	GAP_LE_Security_Information_t _securityInformation;

	/* Make sure a Bluetooth Stack is open. */
	if(_bluetoothStackID)
	{
		/* Configure the Security Information. */
		_securityInformation.Local_Device_Is_Master = FALSE;
		_securityInformation.Security_Information.Slave_Information.Bonding_Type = lbtBonding;
		_securityInformation.Security_Information.Slave_Information.MITM = BT_HOGP_LE_Parameters.MITMProtection;
		/* Attempt to pair to the remote device */
		_retVal = GAP_LE_Reestablish_Security(_bluetoothStackID, _bdAddr, &_securityInformation, BT_HOGP_GAP_LE_Event_Callback, 0U);

		if(!_retVal){BT_HOGP_DBG_Display(("GAP_LE_Reestablish_Security sucess.\r\n"));}
		else{BT_HOGP_DBG_Display(("Error - GAP_LE_Reestablish_Security returned %d.\r\n", _retVal));}
	}
	else{_retVal = BT_HOGP_INVALID_STACK_ID_ERROR;}

	return _retVal;
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
static bt_hogp_device_info_ts *BT_HOGP_Delete_Device_Info_Entry(bt_hogp_device_info_ts **_listHead, \
																GAP_LE_Address_Type_t _addressType, BD_ADDR_t _bdAddr)
{
	bt_hogp_device_info_ts *_lastEntry;
	bt_hogp_device_info_ts *_deviceInfo;

	if((_listHead) && (!COMPARE_NULL_BD_ADDR(_bdAddr)))
	{
		/* Check to see if this is a resolvable address type. If so we will search the list based on the IRK */
		if((latRandom == _addressType) && (GAP_LE_TEST_RESOLVABLE_ADDRESS_BITS(_bdAddr)))
		{
			/* Now, let's search the list until we find the correct entry. */
			_deviceInfo = *_listHead;

			while((_deviceInfo) && ((!(_deviceInfo->Flags & BT_HOGP_DEVICE_INFO_FLAGS_IRK_VALID)) || \
				  (!GAP_LE_Resolve_Address(BT_HOGP_ApplicationStateInfo.BluetoothStackID, &(_deviceInfo->IRK), _bdAddr))))
			{
				_lastEntry = _deviceInfo;
				_deviceInfo = _deviceInfo->NextDeviceInfoPtr;
			}

			/* Check to see if we found the specified entry. */
			if(_deviceInfo)
			{
				/* OK, now let's remove the entry from the list.  We have to check to see if the entry was the first entry in the list. */
				if(_lastEntry)
				{
					/* Entry was NOT the first entry in the list. */
					_lastEntry->NextDeviceInfoPtr = _deviceInfo->NextDeviceInfoPtr;
				}
				else{*_listHead = _deviceInfo->NextDeviceInfoPtr;}

				_deviceInfo->NextDeviceInfoPtr = NULL;
			}
		}
		else{_deviceInfo = NULL;}

		/* If all else fail we will attempt to search the list by just the BD_ADDR. */
		if(NULL == _deviceInfo)
		{_deviceInfo = BSC_DeleteGenericListEntry(ekBD_ADDR_t, (void *)(&_bdAddr), BTPS_STRUCTURE_OFFSET(bt_hogp_device_info_ts, BD_ADDR), \
												  BTPS_STRUCTURE_OFFSET(bt_hogp_device_info_ts, NextDeviceInfoPtr), (void **)(_listHead));}
	}
	else{_deviceInfo = NULL;}

	return(_deviceInfo);
}

/**
  ***************************************************************************************************************************************
  * This function frees the specified Key Info Information member memory.
  ***************************************************************************************************************************************
  */
static void BT_HOGP_Free_Device_Info_Entry_Memory(bt_hogp_device_info_ts *_entryToFree)
{
	BSC_FreeGenericListEntryMemory((void *)(_entryToFree));
}

/**
  ***************************************************************************************************************************************
  * The following function displays the pairing capabalities that is passed into this function.
  ***************************************************************************************************************************************
  */
static void BT_HOGP_Display_Pairing_Information(GAP_LE_Pairing_Capabilities_t _pairingCapabilities)
{
	/* Display the IO Capability. */
	switch(_pairingCapabilities.IO_Capability)
	{
		case licDisplayOnly:BT_HOGP_DBG_Display(("   IO Capability:       lcDisplayOnly.\r\n"));break;
		case licDisplayYesNo:BT_HOGP_DBG_Display(("   IO Capability:       lcDisplayYesNo.\r\n"));break;
		case licKeyboardOnly:BT_HOGP_DBG_Display(("   IO Capability:       lcKeyboardOnly.\r\n"));break;
		case licNoInputNoOutput:BT_HOGP_DBG_Display(("   IO Capability:       lcNoInputNoOutput.\r\n"));break;
		case licKeyboardDisplay:BT_HOGP_DBG_Display(("   IO Capability:       lcKeyboardDisplay.\r\n"));break;
	}

	BT_HOGP_DBG_Display(("   MITM:                %s.\r\n", (TRUE == _pairingCapabilities.MITM)?"TRUE":"FALSE"));
	BT_HOGP_DBG_Display(("   Bonding Type:        %s.\r\n", (lbtBonding == _pairingCapabilities.Bonding_Type)?"Bonding":"No Bonding"));
	BT_HOGP_DBG_Display(("   OOB:                 %s.\r\n", (TRUE == _pairingCapabilities.OOB_Present)?"OOB":"OOB Not Present"));
	BT_HOGP_DBG_Display(("   Encryption Key Size: %d.\r\n", _pairingCapabilities.Maximum_Encryption_Key_Size));
	BT_HOGP_DBG_Display(("   Sending Keys: \r\n"));
	BT_HOGP_DBG_Display(("      LTK:              %s.\r\n", ((TRUE == _pairingCapabilities.Sending_Keys.Encryption_Key)?"YES":"NO")));
	BT_HOGP_DBG_Display(("      IRK:              %s.\r\n", ((TRUE == _pairingCapabilities.Sending_Keys.Identification_Key)?"YES":"NO")));
	BT_HOGP_DBG_Display(("      CSRK:             %s.\r\n", ((TRUE == _pairingCapabilities.Sending_Keys.Signing_Key)?"YES":"NO")));
	BT_HOGP_DBG_Display(("   Receiving Keys: \r\n"));
	BT_HOGP_DBG_Display(("      LTK:              %s.\r\n", ((_pairingCapabilities.Receiving_Keys.Encryption_Key == TRUE)?"YES":"NO")));
	BT_HOGP_DBG_Display(("      IRK:              %s.\r\n", ((_pairingCapabilities.Receiving_Keys.Identification_Key == TRUE)?"YES":"NO")));
	BT_HOGP_DBG_Display(("      CSRK:             %s.\r\n", ((_pairingCapabilities.Receiving_Keys.Signing_Key == TRUE)?"YES":"NO")));
}

/**
  ***************************************************************************************************************************************
  * The following function provides a mechanism of sending a Slave Pairing Response to a Master's Pairing Request.
  ***************************************************************************************************************************************
  */
static int BT_HOGP_Slave_Pairing_Request_Response(unsigned int _bluetoothStackID, BD_ADDR_t _bdAddr)
{
	int _retVal;
	BT_HOGP_BOARD_STR_T _boardStr;
	GAP_LE_Authentication_Response_Information_t _authenticationResponseData;

	/* Make sure a Bluetooth Stack is open. */
	if(_bluetoothStackID)
	{
		BT_HOGP_BD_ADDR_To_Str(_bdAddr, _boardStr);
		BT_HOGP_DBG_Display(("Sending Pairing Response to %s.\r\n", _boardStr));

		/* We must be the slave if we have received a Pairing Request thus we will respond with our capabilities. */
		_authenticationResponseData.GAP_LE_Authentication_Type = larPairingCapabilities;
		_authenticationResponseData.Authentication_Data_Length = GAP_LE_PAIRING_CAPABILITIES_SIZE;
		/* Configure the Application Pairing Parameters. */
		BT_HOGP_Configure_Capabilities(&(_authenticationResponseData.Authentication_Data.Pairing_Capabilities));
		/* Attempt to pair to the remote device. */
		_retVal = GAP_LE_Authentication_Response(_bluetoothStackID, _bdAddr, &_authenticationResponseData);
		if(_retVal){BT_HOGP_DBG_Display(("Error - GAP_LE_Authentication_Response returned %d.\r\n", _retVal));}
	}
	else{_retVal = BT_HOGP_INVALID_STACK_ID_ERROR;}

	return _retVal;
}

/**
  ***************************************************************************************************************************************
  * The following function provides a mechanism to configure a
  * Pairing Capabilities structure with the application's pairing
  * parameters.
  ***************************************************************************************************************************************
  */
static void BT_HOGP_Configure_Capabilities(GAP_LE_Pairing_Capabilities_t *_capabilities)
{
	/* Make sure the Capabilities pointer is semi-valid. */
	if(_capabilities)
	{
		/* Configure the Pairing Cabilities structure. */
		_capabilities->Bonding_Type = lbtNoBonding;
		_capabilities->IO_Capability = BT_HOGP_LE_Parameters.IOCapability;
		_capabilities->MITM = BT_HOGP_LE_Parameters.MITMProtection;
		_capabilities->OOB_Present = BT_HOGP_LE_Parameters.OOBDataPresent;

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
		_capabilities->Maximum_Encryption_Key_Size = GAP_LE_MAXIMUM_ENCRYPTION_KEY_SIZE;

		/** This application only demostrates using Long Term Key's (LTK)
		  * for encryption of a LE Link (and receiving and IRK for
		  * identifying the remote device if it uses a resolvable random
		  * address), however we could request and send all possible keys
		  * here if we wanted to.
		  */
		_capabilities->Receiving_Keys.Encryption_Key = FALSE;
		_capabilities->Receiving_Keys.Identification_Key = TRUE;
		_capabilities->Receiving_Keys.Signing_Key = FALSE;

		_capabilities->Sending_Keys.Encryption_Key = TRUE;
		_capabilities->Sending_Keys.Identification_Key = FALSE;
		_capabilities->Sending_Keys.Signing_Key = FALSE;
	}
}

/**
  ***************************************************************************************************************************************
  * The following function is provided to allow a mechanism of
  * responding to a request for Encryption Information to send to a
  * remote device.
  ***************************************************************************************************************************************
  */
static int BT_HOGP_Encryption_Information_Request_Response(unsigned int _bluetoothStackID, BD_ADDR_t _bdAddr, Byte_t _keySize, \
														   GAP_LE_Authentication_Response_Information_t *_gapLeAuthenticationResponseInformation)
{
	int _retVal;
	Word_t _localDiv;

	/* Make sure a Bluetooth Stack is open. */
	if(_bluetoothStackID)
	{
		/* Make sure the input parameters are semi-valid. */
		if((!COMPARE_NULL_BD_ADDR(_bdAddr)) && (_gapLeAuthenticationResponseInformation))
		{
			BT_HOGP_DBG_Display(("   Calling GAP_LE_Generate_Long_Term_Key.\r\n"));

			/* Generate a new LTK, EDIV and Rand tuple. */
			_retVal = GAP_LE_Generate_Long_Term_Key(_bluetoothStackID, (Encryption_Key_t *)(&BT_HOGP_DHK), \
													(Encryption_Key_t *)(&BT_HOGP_ER), \
													&(_gapLeAuthenticationResponseInformation->Authentication_Data.Encryption_Information.LTK), \
													&_localDiv, &(_gapLeAuthenticationResponseInformation->Authentication_Data.Encryption_Information.EDIV), \
													&(_gapLeAuthenticationResponseInformation->Authentication_Data.Encryption_Information.Rand));

			if(!_retVal)
			{
				BT_HOGP_DBG_Display(("   Encryption Information Request Response.\r\n"));

				/* Response to the request with the LTK, EDIV and Rand values. */
				_gapLeAuthenticationResponseInformation->GAP_LE_Authentication_Type = larEncryptionInformation;
				_gapLeAuthenticationResponseInformation->Authentication_Data_Length = GAP_LE_ENCRYPTION_INFORMATION_DATA_SIZE;
				_gapLeAuthenticationResponseInformation->Authentication_Data.Encryption_Information.Encryption_Key_Size = _keySize;
				_retVal = GAP_LE_Authentication_Response(_bluetoothStackID, _bdAddr, _gapLeAuthenticationResponseInformation);

				if(!_retVal){BT_HOGP_DBG_Display(("   GAP_LE_Authentication_Response (larEncryptionInformation) success.\r\n", _retVal));}
				else{BT_HOGP_DBG_Display(("   Error - SM_Generate_Long_Term_Key returned %d.\r\n", _retVal));}
			}
			else{BT_HOGP_DBG_Display(("   Error - SM_Generate_Long_Term_Key returned %d.\r\n", _retVal));}
		}
		else
		{
			BT_HOGP_DBG_Display(("Invalid Parameters.\r\n"));
			_retVal = BT_HOGP_INVALID_PARAMETERS_ERROR;
		}
	}
	else
	{
		BT_HOGP_DBG_Display(("Stack ID Invalid.\r\n"));
		_retVal = BT_HOGP_INVALID_STACK_ID_ERROR;
	}

	return _retVal;
}

/**
  ***************************************************************************************************************************************
  * The following function is provided to allow a mechanism to disconnect a currently connected device.
  ***************************************************************************************************************************************
  */
static int BT_HOGP_Disconnect_LE_Device(unsigned int _bluetoothStackID, BD_ADDR_t _bdAddr)
{
	int _result;

	/* First, determine if the input parameters appear to be semi-valid. */
	if(_bluetoothStackID)
	{
		/* Disconnect the device. */
		_result = GAP_LE_Disconnect(_bluetoothStackID, _bdAddr);

		if(!_result){BT_HOGP_DBG_Display(("Disconnect Request successful.\r\n"));}
		else{BT_HOGP_DBG_Display(("Unable to disconnect device: %d.\r\n", _result));}
	}
	else{_result = -1;}

	return _result;
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
static void BTPSAPI BT_HOGP_GATT_Connection_Event_Callback(unsigned int _bluetoothStackID, \
														   GATT_Connection_Event_Data_t *_gattConnectionEventData, unsigned long _callbackParameter)
{
	BT_HOGP_BOARD_STR_T _boardStr;

	/* Verify that all parameters to this callback are Semi-Valid. */
	if((_bluetoothStackID) && (_gattConnectionEventData))
	{
		/* Determine the Connection Event that occurred. */
		switch(_gattConnectionEventData->Event_Data_Type)
		{
			case etGATT_Connection_Device_Connection:
			if(_gattConnectionEventData->Event_Data.GATT_Device_Connection_Data)
			{
				BT_HOGP_DBG_Display(("\r\netGATT_Connection_Device_Connection with size %u: \r\n", _gattConnectionEventData->Event_Data_Size));
				BT_HOGP_BD_ADDR_To_Str(_gattConnectionEventData->Event_Data.GATT_Device_Connection_Data->RemoteDevice, _boardStr);
				BT_HOGP_DBG_Display(("   Connection ID:   %u.\r\n", _gattConnectionEventData->Event_Data.GATT_Device_Connection_Data->ConnectionID));
				BT_HOGP_DBG_Display(("   Connection Type: %s.\r\n", \
									((gctLE == _gattConnectionEventData->Event_Data.GATT_Device_Connection_Data->ConnectionType)?"LE":"BR/EDR")));
				BT_HOGP_DBG_Display(("   Remote Device:   %s.\r\n", _boardStr));
				BT_HOGP_DBG_Display(("   Connection MTU:  %u.\r\n", _gattConnectionEventData->Event_Data.GATT_Device_Connection_Data->MTU));

				BT_HOGP_ApplicationStateInfo.Flags |= BT_HOGP_APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED;
				BT_HOGP_ApplicationStateInfo.LEConnectionInfo.ConnectionID = _gattConnectionEventData->Event_Data.GATT_Device_Connection_Data->ConnectionID;
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
		BT_HOGP_DBG_Display(("\r\n"));
		BT_HOGP_DBG_Display(("GATT Connection Callback Data: Event_Data = NULL.\r\n"));
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
static void BTPSAPI BT_HOGP_BAS_Event_Callback(unsigned int _bluetoothStackID, BAS_Event_Data_t *_basEventData, unsigned long _callbackParameter)
{
	int _result;
	bt_hogp_device_info_ts *_deviceInfo;

	/* Verify that all parameters to this callback are Semi-Valid. */
	if((_bluetoothStackID) && (_basEventData))
	{
		/* Search for the Device entry for our current LE connection. */
		if(NULL != (_deviceInfo = BT_HOGP_Search_Device_Info_Entry_By_BD_ADDR(&BT_HOGP_DeviceInfoList, \
																	 	 	  BT_HOGP_ApplicationStateInfo.LEConnectionInfo.AddressType, \
																			  BT_HOGP_ApplicationStateInfo.LEConnectionInfo.BD_ADDR)))
		{
			/* Determine the Battery Service Event that occurred. */
			switch(_basEventData->Event_Data_Type)
			{
				case etBAS_Server_Read_Client_Configuration_Request:
				if((_basEventData->Event_Data.BAS_Read_Client_Configuration_Data) && \
				   (ctBatteryLevel == _basEventData->Event_Data.BAS_Read_Client_Configuration_Data->ClientConfigurationType))
				{
					BT_HOGP_DBG_Display(("Battery Read Battery Client Configuration Request.\r\n"));
					_result = BAS_Read_Client_Configuration_Response(_bluetoothStackID, BT_HOGP_ApplicationStateInfo.BASInstanceID, \
							  	  	  	  	  	  	  	  	  	  	 _basEventData->Event_Data.BAS_Read_Client_Configuration_Data->TransactionID, \
																	 _deviceInfo->BASServerInformation.Battery_Level_Client_Configuration);
					if(_result){BT_HOGP_DBG_Display(("Error - BAS_Read_Client_Configuration_Response() %d.\r\n", _result));}
				}
				break;

				case etBAS_Server_Client_Configuration_Update:
				if((_basEventData->Event_Data.BAS_Client_Configuration_Update_Data) && \
				   (ctBatteryLevel == _basEventData->Event_Data.BAS_Client_Configuration_Update_Data->ClientConfigurationType))
				{
					BT_HOGP_DBG_Display(("Battery Client Configuration Update: %s.\r\n", \
										(_basEventData->Event_Data.BAS_Client_Configuration_Update_Data->Notify?"ENABLED":"DISABLED")));

					/* Update the stored configuration for this device. */
					if(_basEventData->Event_Data.BAS_Client_Configuration_Update_Data->Notify)
					{_deviceInfo->BASServerInformation.Battery_Level_Client_Configuration = GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;}
					else{_deviceInfo->BASServerInformation.Battery_Level_Client_Configuration = 0;}
				}
				break;

				case etBAS_Server_Read_Battery_Level_Request:
				if(_basEventData->Event_Data.BAS_Read_Battery_Level_Data)
				{
					BT_HOGP_DBG_Display(("Battery Read Battery Level Request.\r\n"));
					/* Just respond with the current Battery Level. */
					_result = BAS_Battery_Level_Read_Request_Response(_bluetoothStackID, \
							  _basEventData->Event_Data.BAS_Read_Battery_Level_Data->TransactionID, (Byte_t)BT_HOGP_ApplicationStateInfo.BatteryLevel);

					if(_result){BT_HOGP_DBG_Display(("Error - BAS_Battery_Level_Read_Request_Response() %d.\r\n", _result));}
				}
				break;
			}
		}
	}
	else
	{
		/* There was an error with one or more of the input parameters. */
		BT_HOGP_DBG_Display(("\r\n"));
		BT_HOGP_DBG_Display(("Battery Service Callback Data: Event_Data = NULL.\r\n"));
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
static void BTPSAPI BT_HOGP_HIDS_Event_Callback(unsigned int _bluetoothStackID, HIDS_Event_Data_t *_hidsEventData, unsigned long _callbackParameter)
{
	int _result;
	Byte_t _errorCode;
	Word_t _configuration;
	Byte_t *_reportData;
	bt_hogp_device_info_ts *_deviceInfo;
	unsigned int _reportDataLength;

	/* Verify that all parameters to this callback are Semi-Valid. */
	if((_bluetoothStackID) && (_hidsEventData))
	{
		/* Search for the Device entry for our current LE connection. */
		if(NULL != (_deviceInfo = BT_HOGP_Search_Device_Info_Entry_By_BD_ADDR(&BT_HOGP_DeviceInfoList, \
																	  	  	  BT_HOGP_ApplicationStateInfo.LEConnectionInfo.AddressType, \
																			  BT_HOGP_ApplicationStateInfo.LEConnectionInfo.BD_ADDR)))
		{
			/* Determine the HID Service Event that occurred. */
			switch(_hidsEventData->Event_Data_Type)
			{
				case etHIDS_Server_Read_Client_Configuration_Request:
				if(_hidsEventData->Event_Data.HIDS_Read_Client_Configuration_Data)
				{
					BT_HOGP_DBG_Display(("HIDS Read Client Configuration Request: %u.\r\n", _hidsEventData->Event_Data.HIDS_Read_Client_Configuration_Data->ReportType));

					if(rtBootKeyboardInputReport == _hidsEventData->Event_Data.HIDS_Read_Client_Configuration_Data->ReportType)
					{_configuration = _deviceInfo->BootKeyboardInputConfiguration;}
					else{_configuration = _deviceInfo->ReportKeyboardInputConfiguration;}

					/* Respond to the read request. */
					_result = HIDS_Read_Client_Configuration_Response(BT_HOGP_ApplicationStateInfo.BluetoothStackID, \
																	  BT_HOGP_ApplicationStateInfo.HIDSInstanceID, \
																	  _hidsEventData->Event_Data.HIDS_Read_Client_Configuration_Data->TransactionID, \
																	  _configuration);
					if(_result){BT_HOGP_DBG_Display(("Error - HIDS_Read_Client_Configuration_Response() %d.\r\n", _result));}
				}
				break;

				case etHIDS_Server_Client_Configuration_Update_Request:
				if(_hidsEventData->Event_Data.HIDS_Client_Configuration_Update_Data)
				{
					BT_HOGP_DBG_Display(("HIDS Client Configuration Update: %u.\r\n", _hidsEventData->Event_Data.HIDS_Client_Configuration_Update_Data->ReportType));

					if(rtBootKeyboardInputReport == _hidsEventData->Event_Data.HIDS_Client_Configuration_Update_Data->ReportType)
					{
						_deviceInfo->BootKeyboardInputConfiguration = _hidsEventData->Event_Data.HIDS_Client_Configuration_Update_Data->ClientConfiguration;
						BT_HOGP_DBG_Display(("Boot Keyboard Input Report Configuration 0x%04X.\r\n", _deviceInfo->BootKeyboardInputConfiguration));
					}
					else
					{
						_deviceInfo->ReportKeyboardInputConfiguration = _hidsEventData->Event_Data.HIDS_Client_Configuration_Update_Data->ClientConfiguration;
						BT_HOGP_DBG_Display(("Report Keyboard Input Report Configuration 0x%04X.\r\n", _deviceInfo->ReportKeyboardInputConfiguration));
					}
				}
				break;

				case etHIDS_Server_Get_Protocol_Mode_Request:
				if(_hidsEventData->Event_Data.HIDS_Get_Protocol_Mode_Request_Data)
				{
					BT_HOGP_DBG_Display(("HIDS Get Protocol Mode Request.\r\n"));

					/* Note that security is required to read this characteristic. */
					if(BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Flags & BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED){_errorCode = 0;}
					else{_errorCode = ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_ENCRYPTION;}

					/* Respond the Get Protocol Mode request. */
					_result = HIDS_Get_Protocol_Mode_Response(BT_HOGP_ApplicationStateInfo.BluetoothStackID, \
															  BT_HOGP_ApplicationStateInfo.HIDSInstanceID, \
															  _hidsEventData->Event_Data.HIDS_Get_Protocol_Mode_Request_Data->TransactionID, \
															  _errorCode, BT_HOGP_ApplicationStateInfo.HIDProtocolMode);
					if(_result){BT_HOGP_DBG_Display(("Error - HIDS_Get_Protocol_Mode_Response() %d.\r\n", _result));}
				}
				break;

				case etHIDS_Server_Set_Protocol_Mode_Request:
				if(_hidsEventData->Event_Data.HIDS_Set_Protocol_Mode_Request_Data)
				{
					BT_HOGP_DBG_Display(("HIDS Set Protocol Mode Request: %s(%u).\r\n", \
										(pmBoot == _hidsEventData->Event_Data.HIDS_Set_Protocol_Mode_Request_Data->ProtocolMode)?\
									    "Boot":"Report", (unsigned int)_hidsEventData->Event_Data.HIDS_Set_Protocol_Mode_Request_Data->ProtocolMode));

					/* Note that security is required to write this characteristic. */
					if(BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Flags & BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED)
					{BT_HOGP_ApplicationStateInfo.HIDProtocolMode = _hidsEventData->Event_Data.HIDS_Set_Protocol_Mode_Request_Data->ProtocolMode;}
				}
				break;

				case etHIDS_Server_Get_Report_Map_Request:
				if(_hidsEventData->Event_Data.HIDS_Get_Report_Map_Data)
				{
					BT_HOGP_DBG_Display(("HIDS Get Report Map Request: Offset = %u.\r\n", _hidsEventData->Event_Data.HIDS_Get_Report_Map_Data->ReportMapOffset));

					/* Note that security is required to read this characteristic. */
					if(BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Flags & BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED)
					{
						/* Initialize the return value to success. */
						_errorCode = 0;

						/* Verify that the offset being read is valid. */
						if(_hidsEventData->Event_Data.HIDS_Get_Report_Map_Data->ReportMapOffset < (sizeof(BT_HOGP_KeyboardReportDescriptor)))
						{
							/* Get a pointer to the report map to return. */
							_reportDataLength = (sizeof(BT_HOGP_KeyboardReportDescriptor) - _hidsEventData->Event_Data.HIDS_Get_Report_Map_Data->ReportMapOffset);
							_reportData = &BT_HOGP_KeyboardReportDescriptor[_hidsEventData->Event_Data.HIDS_Get_Report_Map_Data->ReportMapOffset];
						}
						else{_errorCode = ATT_PROTOCOL_ERROR_CODE_INVALID_OFFSET;}
					}
					else
					{
						_errorCode = ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_ENCRYPTION;
						_reportDataLength = 0U;
						_reportData = NULL;
					}

					/* Respond the Get Report Map request. */
					_result = HIDS_Get_Report_Map_Response(BT_HOGP_ApplicationStateInfo.BluetoothStackID, \
														   BT_HOGP_ApplicationStateInfo.HIDSInstanceID, \
														   _hidsEventData->Event_Data.HIDS_Get_Report_Map_Data->TransactionID, \
														   _errorCode, _reportDataLength, _reportData);
					if(_result){BT_HOGP_DBG_Display(("Error - HIDS_Get_Report_Map_Response() %d.\r\n", _result));}
				}
				break;

				case etHIDS_Server_Get_Report_Request:
				if(_hidsEventData->Event_Data.HIDS_Get_Report_Request_Data)
				{
					BT_HOGP_DBG_Display(("HID Get Report Request: %u.\r\n", _hidsEventData->Event_Data.HIDS_Get_Report_Request_Data->ReportType));

					/* Note that security is required to read this characteristic. */
					if(BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Flags & BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED)
					{
						/* Flag that no error has occurred. */
						_errorCode = 0;
						/* Determine what report the Host is attempting to read. */
						if((rtBootKeyboardInputReport == _hidsEventData->Event_Data.HIDS_Get_Report_Request_Data->ReportType) || \
						   ((rtReport == _hidsEventData->Event_Data.HIDS_Get_Report_Request_Data->ReportType) && \
							(HIDS_REPORT_REFERENCE_REPORT_TYPE_INPUT_REPORT == _hidsEventData->Event_Data.HIDS_Get_Report_Request_Data->ReportReferenceData.ReportType)))
						{
							/** Respond with the Keyboard Input Report. Note
							  * that since our Report Mode Report is
							  * identical to the Boot Mode Report we do not
							  * need to differentiate here.
							  */
							_reportDataLength = BT_HOGP_HID_KEYBOARD_INPUT_REPORT_SIZE;
							_reportData = BT_HOGP_ApplicationStateInfo.CurrentInputReport;
						}
						else
						{
							if((rtBootKeyboardOutputReport == _hidsEventData->Event_Data.HIDS_Get_Report_Request_Data->ReportType) || \
							   ((rtReport == _hidsEventData->Event_Data.HIDS_Get_Report_Request_Data->ReportType) && \
								(HIDS_REPORT_REFERENCE_REPORT_TYPE_OUTPUT_REPORT == _hidsEventData->Event_Data.HIDS_Get_Report_Request_Data->ReportReferenceData.ReportType)))
							{
								/** Respond with the Keyboard Output Report.
								  * Note that since our Report Mode Report is
								  * identical to the Boot Mode Report we do
								  * not need to differentiate here.
								  */
								_reportDataLength = sizeof(BT_HOGP_ApplicationStateInfo.CurrentOutputReport);
								_reportData = &(BT_HOGP_ApplicationStateInfo.CurrentOutputReport);
							}
							else
							{
								BT_HOGP_DBG_Display(("Unknown Report %u, (ID,Type) = %u, %u.\r\n", _hidsEventData->Event_Data.HIDS_Get_Report_Request_Data->ReportType, \
										_hidsEventData->Event_Data.HIDS_Get_Report_Request_Data->ReportReferenceData.ReportID, \
										_hidsEventData->Event_Data.HIDS_Get_Report_Request_Data->ReportReferenceData.ReportType));
								_errorCode = ATT_PROTOCOL_ERROR_CODE_UNLIKELY_ERROR;
							}
						}
					}
					else
					{
						_errorCode = ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_ENCRYPTION;
						_reportDataLength = 0U;
						_reportData = NULL;
					}

					/* Respond to the Get Report Request. */
					_result = HIDS_Get_Report_Response(BT_HOGP_ApplicationStateInfo.BluetoothStackID, \
							  	  	  	  	  	  	   BT_HOGP_ApplicationStateInfo.HIDSInstanceID, \
													   _hidsEventData->Event_Data.HIDS_Get_Report_Request_Data->TransactionID, \
													   _hidsEventData->Event_Data.HIDS_Get_Report_Request_Data->ReportType, \
													   &(_hidsEventData->Event_Data.HIDS_Get_Report_Request_Data->ReportReferenceData), \
													   _errorCode, _reportDataLength, _reportData);
					if(_result){BT_HOGP_DBG_Display(("Error - HIDS_Get_Report_Response() %d.\r\n", _result));}
				}
				break;

				case etHIDS_Server_Set_Report_Request:
				if(_hidsEventData->Event_Data.HIDS_Set_Report_Request_Data)
				{
					BT_HOGP_DBG_Display(("HID Set Report Request: %u.\r\n", _hidsEventData->Event_Data.HIDS_Set_Report_Request_Data->ReportType));

					/* Note that security is required to write this characteristic. */
					if(BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Flags & BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED)
					{
						/* Flag that no error has occurred. */
						_errorCode = 0;
						/* Determine what report the Host is attempting to write. */
						if((rtBootKeyboardOutputReport == _hidsEventData->Event_Data.HIDS_Set_Report_Request_Data->ReportType) || \
						   ((rtReport == _hidsEventData->Event_Data.HIDS_Set_Report_Request_Data->ReportType) && \
						    (HIDS_REPORT_REFERENCE_REPORT_TYPE_OUTPUT_REPORT == _hidsEventData->Event_Data.HIDS_Set_Report_Request_Data->ReportReferenceData.ReportType)))
						{
							/* Verify that the length is valid. */
							if(_hidsEventData->Event_Data.HIDS_Set_Report_Request_Data->ReportLength == (sizeof(BT_HOGP_ApplicationStateInfo.CurrentOutputReport)))
							{
								/* Set the Output Report Value. */
								BT_HOGP_ApplicationStateInfo.CurrentOutputReport = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(_hidsEventData->Event_Data.HIDS_Set_Report_Request_Data->Report);
								BT_HOGP_DBG_Display(("Current Output Report Value: 0x%02X.\r\n", BT_HOGP_ApplicationStateInfo.CurrentOutputReport));
							}
							else{_errorCode = ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH;}
						}
						else
						{
							BT_HOGP_DBG_Display(("Unknown Report %u, (ID,Type) = %u, %u.\r\n", _hidsEventData->Event_Data.HIDS_Set_Report_Request_Data->ReportType, \
												_hidsEventData->Event_Data.HIDS_Set_Report_Request_Data->ReportReferenceData.ReportID, \
												_hidsEventData->Event_Data.HIDS_Set_Report_Request_Data->ReportReferenceData.ReportType));
							_errorCode = ATT_PROTOCOL_ERROR_CODE_UNLIKELY_ERROR;
						}
					}
					else{_errorCode = ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_ENCRYPTION;}

					/* Respond to the Set Report Request. */
					_result = HIDS_Set_Report_Response(BT_HOGP_ApplicationStateInfo.BluetoothStackID, BT_HOGP_ApplicationStateInfo.HIDSInstanceID, \
													   _hidsEventData->Event_Data.HIDS_Set_Report_Request_Data->TransactionID, \
													   _hidsEventData->Event_Data.HIDS_Set_Report_Request_Data->ReportType, \
													   &(_hidsEventData->Event_Data.HIDS_Set_Report_Request_Data->ReportReferenceData), _errorCode);
					if(_result){BT_HOGP_DBG_Display(("Error - HIDS_Set_Report_Response() %d.\r\n", _result));}
				}
				break;

				case etHIDS_Server_Control_Point_Command_Indication:
				if(_hidsEventData->Event_Data.HIDS_Control_Point_Command_Data)
				{BT_HOGP_DBG_Display(("HID Control Point Command: %s (%u).\r\n", \
									 ((pcSuspend == _hidsEventData->Event_Data.HIDS_Control_Point_Command_Data->ControlPointCommand)? \
									 "Suspend":"Exit Suspend"), (unsigned int)_hidsEventData->Event_Data.HIDS_Control_Point_Command_Data->ControlPointCommand));}
				break;
			}
		}
		else{BT_HOGP_DBG_Display(("Error Error Error!\r\n"));}
	}
	else
	{
		/* There was an error with one or more of the input parameters. */
		BT_HOGP_DBG_Display(("\r\n"));
		BT_HOGP_DBG_Display(("HIDS Event Callback Data: Event_Data = NULL.\r\n"));
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
static void BTPSAPI BT_HOGP_GAP_LE_Event_Callback(unsigned int _bluetoothStackID, GAP_LE_Event_Data_t *_gapLEEventData, unsigned long _callbackParameter)
{
	int _result;
	BT_HOGP_BOARD_STR_T _boardStr;
	bt_hogp_device_info_ts *_deviceInfo;
	Long_Term_Key_t _generatedLTK;
	GAP_LE_Authentication_Event_Data_t *_authenticationEventData;
	GAP_LE_Authentication_Response_Information_t _gapLeAuthenticationResponseInformation;

	/* Verify that all parameters to this callback are Semi-Valid. */
	if((_bluetoothStackID) && (_gapLEEventData))
	{
		switch(_gapLEEventData->Event_Data_Type)
		{
			case etLE_Connection_Complete:
				BT_HOGP_DBG_Display(("etLE_Connection_Complete with size %d.\r\n",(int)_gapLEEventData->Event_Data_Size));

			if(_gapLEEventData->Event_Data.GAP_LE_Connection_Complete_Event_Data)
			{
				BT_HOGP_BD_ADDR_To_Str(_gapLEEventData->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address, _boardStr);

				BT_HOGP_DBG_Display(("   Status:       0x%02X.\r\n", _gapLEEventData->Event_Data.GAP_LE_Connection_Complete_Event_Data->Status));
				BT_HOGP_DBG_Display(("   Role:         %s.\r\n", (_gapLEEventData->Event_Data.GAP_LE_Connection_Complete_Event_Data->Master)?"Master":"Slave"));
				BT_HOGP_DBG_Display(("   Address Type: %s.\r\n", (latPublic == _gapLEEventData->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type)? \
																 "Public":"Random"));
				BT_HOGP_DBG_Display(("   BD_ADDR:      %s.\r\n", _boardStr));

				/* Check to see if the connection was succesfull. */
				if(_gapLEEventData->Event_Data.GAP_LE_Connection_Complete_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR)
				{
					/* Save the Connection Information. */
					BT_HOGP_ApplicationStateInfo.Flags |= BT_HOGP_APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED;
					BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Flags = BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_VALID;
					BT_HOGP_ApplicationStateInfo.LEConnectionInfo.BD_ADDR = _gapLEEventData->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address;
					BT_HOGP_ApplicationStateInfo.LEConnectionInfo.AddressType = _gapLEEventData->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type;

					/* Inform the main application handler of the LE connection. */
					BT_HOGP_Post_Application_Mailbox(BT_HOGP_APPLICATION_MAILBOX_MESSAGE_ID_LE_CONNECTED);

					/* Make sure that no entry already exists. */
					if(NULL == (_deviceInfo = BT_HOGP_Search_Device_Info_Entry_By_BD_ADDR(&BT_HOGP_DeviceInfoList, \
																				 	  	 _gapLEEventData->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type, \
																						 _gapLEEventData->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address)))
					{
						/* No entry exists so create one. */
						if(!BT_HOGB_Create_New_Device_Info_Entry(&BT_HOGP_DeviceInfoList, \
																 _gapLEEventData->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type, \
																 _gapLEEventData->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address))
						{BT_HOGP_DBG_Display(("Failed to add device to Device Info List.\r\n"));}
					}
					else
					{
						/** We have paired with this device previously.
						  * Therefore we will start a timer and if the
						  * Master does not re-establish encryption when the
						  * timer expires we will request that he does so.
						  */
						_result = BSC_StartTimer(_bluetoothStackID, \
								 (_gapLEEventData->Event_Data.GAP_LE_Connection_Complete_Event_Data->Current_Connection_Parameters.Connection_Interval * \
								 (_gapLEEventData->Event_Data.GAP_LE_Connection_Complete_Event_Data->Current_Connection_Parameters.Slave_Latency + 8)), \
								 BT_HOGP_BSC_TimerCallback, 0);

						if(_result > 0)
						{
							/* Save the Security Timer ID. */
							BT_HOGP_ApplicationStateInfo.LEConnectionInfo.SecurityTimerID = (unsigned int)_result;
						}
						else{BT_HOGP_DBG_Display(("Error - BSC_StartTimer() returned %d.\r\n", _result));}
					}
				}
				else
				{
					/* Failed to connect to device so start device advertising again. */
					BT_HOGP_Post_Application_Mailbox(BT_HOGP_APPLICATION_MAILBOX_MESSAGE_ID_LE_DISCONNECTED);
				}
			}
			break;

			case etLE_Disconnection_Complete:
			BT_HOGP_DBG_Display(("etLE_Disconnection_Complete with size %d.\r\n", (int)_gapLEEventData->Event_Data_Size));

			if(_gapLEEventData->Event_Data.GAP_LE_Disconnection_Complete_Event_Data)
			{
				BT_HOGP_DBG_Display(("   Status: 0x%02X.\r\n", _gapLEEventData->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Status));
				BT_HOGP_DBG_Display(("   Reason: 0x%02X.\r\n", _gapLEEventData->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Reason));
				BT_HOGP_BD_ADDR_To_Str(_gapLEEventData->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address, _boardStr);
				BT_HOGP_DBG_Display(("   BD_ADDR: %s.\r\n", _boardStr));

				/* Check to see if the device info is present in the list. */
				if(NULL != (_deviceInfo = BT_HOGP_Search_Device_Info_Entry_By_BD_ADDR(&BT_HOGP_DeviceInfoList, \
																					  _gapLEEventData->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address_Type, \
																					  _gapLEEventData->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address)))
				{
					/* Check to see if the link is encrypted. If it isn't we will delete the device structure. */
					if(!(BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Flags & BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED))
					{
						/* Connection is not encrypted so delete the device structure. */
						_deviceInfo = BT_HOGP_Delete_Device_Info_Entry(&BT_HOGP_DeviceInfoList, _deviceInfo->AddressType, _deviceInfo->BD_ADDR);
						if(_deviceInfo){BT_HOGP_Free_Device_Info_Entry_Memory(_deviceInfo);}
					}
				}

				/* Inform the event handler of the LE Disconnection. */
				BT_HOGP_Post_Application_Mailbox(BT_HOGP_APPLICATION_MAILBOX_MESSAGE_ID_LE_DISCONNECTED);
			}
			break;

			case etLE_Encryption_Change:
			BT_HOGP_DBG_Display(("etLE_Encryption_Change with size %d.\r\n",(int)_gapLEEventData->Event_Data_Size));

			/* Verify that the link is currently encrypted.             */
			if((HCI_ERROR_CODE_NO_ERROR == _gapLEEventData->Event_Data.GAP_LE_Encryption_Change_Event_Data->Encryption_Change_Status) && \
			   (emEnabled == _gapLEEventData->Event_Data.GAP_LE_Encryption_Change_Event_Data->Encryption_Mode))
			{BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Flags |= BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED;}
			else{BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Flags &= ~BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED;}
			break;

			case etLE_Encryption_Refresh_Complete:
			BT_HOGP_DBG_Display(("etLE_Encryption_Refresh_Complete with size %d.\r\n", (int)_gapLEEventData->Event_Data_Size));

			/* Verify that the link is currently encrypted. */
			if(HCI_ERROR_CODE_NO_ERROR == _gapLEEventData->Event_Data.GAP_LE_Encryption_Refresh_Complete_Event_Data->Status)
			{BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Flags |= BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED;}
			else{BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Flags &= ~BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED;}
			break;

			case etLE_Authentication:
			BT_HOGP_DBG_Display(("etLE_Authentication with size %d.\r\n", (int)_gapLEEventData->Event_Data_Size));

			/* Make sure the authentication event data is valid before continueing. */
			if(NULL != (_authenticationEventData = _gapLEEventData->Event_Data.GAP_LE_Authentication_Event_Data))
			{
				BT_HOGP_BD_ADDR_To_Str(_authenticationEventData->BD_ADDR, _boardStr);

				switch(_authenticationEventData->GAP_LE_Authentication_Event_Type)
				{
					case latLongTermKeyRequest:
					BT_HOGP_DBG_Display(("    latKeyRequest: \r\n"));
					BT_HOGP_DBG_Display(("      BD_ADDR: %s.\r\n", _boardStr));

					/** The other side of a connection is requesting
					  * that we start encryption. Thus we should
					  * regenerate LTK for this connection and send it
					  * to the chip.
					  */
					_result = GAP_LE_Regenerate_Long_Term_Key(_bluetoothStackID, (Encryption_Key_t *)(&BT_HOGP_DHK), (Encryption_Key_t *)(&BT_HOGP_ER), \
															  _authenticationEventData->Authentication_Event_Data.Long_Term_Key_Request.EDIV, \
															  &(_authenticationEventData->Authentication_Event_Data.Long_Term_Key_Request.Rand), &_generatedLTK);
					if(!_result)
					{
						BT_HOGP_DBG_Display(("      GAP_LE_Regenerate_Long_Term_Key Success.\r\n"));
						/* Respond with the Re-Generated Long Term Key. */
						_gapLeAuthenticationResponseInformation.GAP_LE_Authentication_Type = larLongTermKey;
						_gapLeAuthenticationResponseInformation.Authentication_Data_Length = GAP_LE_LONG_TERM_KEY_INFORMATION_DATA_SIZE;
						_gapLeAuthenticationResponseInformation.Authentication_Data.Long_Term_Key_Information.Encryption_Key_Size = GAP_LE_MAXIMUM_ENCRYPTION_KEY_SIZE;
						_gapLeAuthenticationResponseInformation.Authentication_Data.Long_Term_Key_Information.Long_Term_Key = _generatedLTK;
					}
					else
					{
						BT_HOGP_DBG_Display(("      GAP_LE_Regenerate_Long_Term_Key returned %d.\r\n", _result));
						/* Since we failed to generate the requested key we should respond with a negative response. */
						_gapLeAuthenticationResponseInformation.GAP_LE_Authentication_Type = larLongTermKey;
						_gapLeAuthenticationResponseInformation.Authentication_Data_Length = 0;
					}

					/* Send the Authentication Response. */
					_result = GAP_LE_Authentication_Response(_bluetoothStackID, _authenticationEventData->BD_ADDR, \
															 &_gapLeAuthenticationResponseInformation);
					if(!_result)
					{
						/* Master is trying to re-encrypt the Link so therefore we should cancel the Security Timer if it is active. */
						if(BT_HOGP_ApplicationStateInfo.LEConnectionInfo.SecurityTimerID)
						{
							BSC_StopTimer(_bluetoothStackID, BT_HOGP_ApplicationStateInfo.LEConnectionInfo.SecurityTimerID);
							BT_HOGP_ApplicationStateInfo.LEConnectionInfo.SecurityTimerID = 0;
						}
					}
					else{BT_HOGP_DBG_Display(("      GAP_LE_Authentication_Response returned %d.\r\n", _result));}
					break;

					case latPairingRequest:
					BT_HOGP_DBG_Display(("Pairing Request: %s.\r\n", _boardStr));
					BT_HOGP_Display_Pairing_Information(_authenticationEventData->Authentication_Event_Data.Pairing_Request);

					/** Master is trying to pair with us so therefore we
					  * should cancel the Security Timer if it is
					  * active.
					  */
					if(BT_HOGP_ApplicationStateInfo.LEConnectionInfo.SecurityTimerID)
					{
						BSC_StopTimer(_bluetoothStackID, BT_HOGP_ApplicationStateInfo.LEConnectionInfo.SecurityTimerID);
						BT_HOGP_ApplicationStateInfo.LEConnectionInfo.SecurityTimerID = 0;
					}

					/** This is a pairing request. Respond with a
					  * Pairing Response.
					  * * NOTE * This is only sent from Master to Slave.
					  *          Thus we must be the Slave in this
					  *          connection.
					  */

					/* Send the Pairing Response. */
					BT_HOGP_Slave_Pairing_Request_Response(_bluetoothStackID, _authenticationEventData->BD_ADDR);
					break;

					case latConfirmationRequest:
					BT_HOGP_DBG_Display(("latConfirmationRequest.\r\n"));

					if(crtNone == _authenticationEventData->Authentication_Event_Data.Confirmation_Request.Request_Type)
					{
						BT_HOGP_DBG_Display(("Invoking Just Works.\r\n"));
						/* Just Accept Just Works Pairing. */
						_gapLeAuthenticationResponseInformation.GAP_LE_Authentication_Type = larConfirmation;

						/** By setting the Authentication_Data_Length to
						  * any NON-ZERO value we are informing the GAP
						  * LE Layer that we are accepting Just Works
						  * Pairing.
						  */
						_gapLeAuthenticationResponseInformation.Authentication_Data_Length = DWORD_SIZE;
						_result = GAP_LE_Authentication_Response(_bluetoothStackID, _authenticationEventData->BD_ADDR, &_gapLeAuthenticationResponseInformation);
						if(_result){BT_HOGP_DBG_Display(("GAP_LE_Authentication_Response returned %d.\r\n",_result));}
					}
					else
					{
						if(crtPasskey == _authenticationEventData->Authentication_Event_Data.Confirmation_Request.Request_Type)
						{
							BT_HOGP_DBG_Display(("Enter 6 digit Passkey on display.\r\n"));
							/* Flag that we are awaiting a Passkey Input. */
							BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Flags |= BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_AWAITING_PASSKEY;
							BT_HOGP_ApplicationStateInfo.LEConnectionInfo.PasskeyDigits = 0;
							BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Passkey = 0;
						}
						else
						{
							if(crtDisplay == _authenticationEventData->Authentication_Event_Data.Confirmation_Request.Request_Type)
							{BT_HOGP_DBG_Display(("Passkey: %06lu.\r\n", _authenticationEventData->Authentication_Event_Data.Confirmation_Request.Display_Passkey));}
						}
					}
					break;

					case latEncryptionInformationRequest:
					BT_HOGP_DBG_Display(("Encryption Information Request %s.\r\n", _boardStr));
					/* Generate new LTK,EDIV and Rand and respond with them. */
					BT_HOGP_Encryption_Information_Request_Response(_bluetoothStackID, \
																	_authenticationEventData->BD_ADDR, \
																	_authenticationEventData->Authentication_Event_Data.Encryption_Request_Information.Encryption_Key_Size, \
																	&_gapLeAuthenticationResponseInformation);
					break;

					case latIdentityInformation:
					BT_HOGP_DBG_Display(("latIdentityInformation.\r\n"));

					/* Search for the Device entry for our current LE connection. */
					if(NULL != (_deviceInfo = BT_HOGP_Search_Device_Info_Entry_By_BD_ADDR(&BT_HOGP_DeviceInfoList, \
																				 	 	  BT_HOGP_ApplicationStateInfo.LEConnectionInfo.AddressType, \
																						  BT_HOGP_ApplicationStateInfo.LEConnectionInfo.BD_ADDR)))
					{
						/* Store the received IRK and also updated the BD_ADDR that is stored to the "Base" BD_ADDR. */
						_deviceInfo->AddressType = _authenticationEventData->Authentication_Event_Data.Identity_Information.Address_Type;
						_deviceInfo->BD_ADDR = _authenticationEventData->Authentication_Event_Data.Identity_Information.Address;
						_deviceInfo->IRK = _authenticationEventData->Authentication_Event_Data.Identity_Information.IRK;
						_deviceInfo->Flags |= BT_HOGP_DEVICE_INFO_FLAGS_IRK_VALID;
					}
					break;

					case latSecurityEstablishmentComplete:
						BT_HOGP_DBG_Display(("Security Re-Establishment Complete: %s.\r\n", _boardStr));
						BT_HOGP_DBG_Display(("                            Status: 0x%02X.\r\n", \
											_authenticationEventData->Authentication_Event_Data.Security_Establishment_Complete.Status));

					/** Check to see if the Security Re-establishment
					  * was successful (or if it failed since the remote
					  * device attempted to re-pair.
					  */
					if((GAP_LE_SECURITY_ESTABLISHMENT_STATUS_CODE_NO_ERROR != _authenticationEventData->Authentication_Event_Data.Security_Establishment_Complete.Status) && \
					   (GAP_LE_SECURITY_ESTABLISHMENT_STATUS_CODE_DEVICE_TRIED_TO_REPAIR != _authenticationEventData->Authentication_Event_Data.Security_Establishment_Complete.Status))
					{
						/** Security Re-establishment was not successful
						  * so delete the stored device information and
						  * disconnect the link.
						  */
						BT_HOGP_Disconnect_LE_Device(_bluetoothStackID, _authenticationEventData->BD_ADDR);

						/* Delete the stored device info structure. */
						if(NULL != (_deviceInfo = BT_HOGP_Delete_Device_Info_Entry(&BT_HOGP_DeviceInfoList, \
																		  	  	   BT_HOGP_ApplicationStateInfo.LEConnectionInfo.AddressType, \
																				   BT_HOGP_ApplicationStateInfo.LEConnectionInfo.BD_ADDR)))
						{BT_HOGP_Free_Device_Info_Entry_Memory(_deviceInfo);}
					}
					break;

					case latPairingStatus:
					BT_HOGP_DBG_Display(("Pairing Status: %s.\r\n", _boardStr));
					BT_HOGP_DBG_Display(("        Status: 0x%02X.\r\n", _authenticationEventData->Authentication_Event_Data.Pairing_Status.Status));

					/** Check to see if we have paired successfully with
					  * the device.
					  *                                      */
					if(GAP_LE_PAIRING_STATUS_NO_ERROR == _authenticationEventData->Authentication_Event_Data.Pairing_Status.Status)
					{
						BT_HOGP_DBG_Display(("        Key Size: %d.\r\n", \
											_authenticationEventData->Authentication_Event_Data.Pairing_Status.Negotiated_Encryption_Key_Size));

						/** Flag that no matter what we are not waiting
						  * on a Passkey any longer.
						  */
						BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Flags &= ~BT_HOGP_APPLICATION_MAILBOX_MESSAGE_ID_PASSKEY_ENTERED;

						/** Search for the Device entry for our current
						  * LE connection.
						  */
						if(NULL != (_deviceInfo = BT_HOGP_Search_Device_Info_Entry_By_BD_ADDR(&BT_HOGP_DeviceInfoList, \
																					 	 	  BT_HOGP_ApplicationStateInfo.LEConnectionInfo.AddressType, \
																							  BT_HOGP_ApplicationStateInfo.LEConnectionInfo.BD_ADDR)))
						{
							/* Save the encryption key size. */
							_deviceInfo->EncryptionKeySize = _authenticationEventData->Authentication_Event_Data.Pairing_Status.Negotiated_Encryption_Key_Size;
						}
					}
					else
					{
						/* Disconnect the Link. */
						BT_HOGP_Disconnect_LE_Device(_bluetoothStackID, _authenticationEventData->BD_ADDR);

						/* Delete the stored device info structure. */
						if(NULL != (_deviceInfo = BT_HOGP_Delete_Device_Info_Entry(&BT_HOGP_DeviceInfoList, \
																		  	  	  BT_HOGP_ApplicationStateInfo.LEConnectionInfo.AddressType, \
																				  BT_HOGP_ApplicationStateInfo.LEConnectionInfo.BD_ADDR)))
						{BT_HOGP_Free_Device_Info_Entry_Memory(_deviceInfo);}
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
static void BTPSAPI BT_HOGP_BSC_TimerCallback(unsigned int _bluetoothStackID, unsigned int _timerID, unsigned long _callbackParameter)
{
	/* Verify that the input parameters are semi-valid. */
	if((_bluetoothStackID) && (_timerID))
	{
		/* Verify that the LE Connection is still active. */
		if((BT_HOGP_ApplicationStateInfo.Flags & BT_HOGP_APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED) && \
		   (BT_HOGP_ApplicationStateInfo.LEConnectionInfo.SecurityTimerID == _timerID))
		{
			/* Invalidate the Timer ID that just expired. */
			BT_HOGP_ApplicationStateInfo.LEConnectionInfo.SecurityTimerID = 0;

			/* If the connection is not currently encrypted then we will send a Slave Security Request. */
			if(!(BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Flags & BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED))
			{BT_HOGP_Slave_Security_ReEstablishment(_bluetoothStackID, BT_HOGP_ApplicationStateInfo.LEConnectionInfo.BD_ADDR);}
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
static void BTPSAPI BT_HOGP_HCI_Sleep_Callback(Boolean_t _sleepAllowed, unsigned long _callbackParameter)
{
	/* Simply store the state internally. */
	BT_HOGP_SleepEnabledFlag = _sleepAllowed;
}

/**
  ***************************************************************************************************************************************
  * The following function is a utility function that is used to
  * notify the Battery Level to the connected LE device (if the device
  * has registered for notifications).
  ***************************************************************************************************************************************
  */
static void BT_HOGP_Notify_Battery_Level(bt_hogp_application_state_info_ts *_applicationStateInfo, Boolean_t _force)
{
	int _result;
	bt_hogp_device_info_ts *_deviceInfo;

	/* Make sure the input parameters are semi-valid. */
	if((_applicationStateInfo) && (_applicationStateInfo->Flags & BT_HOGP_APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED))
	{
		/* Search for the device info structure for this device. */
		if(NULL != (_deviceInfo = BT_HOGP_Search_Device_Info_Entry_By_BD_ADDR(&BT_HOGP_DeviceInfoList, _applicationStateInfo->LEConnectionInfo.AddressType, \
					_applicationStateInfo->LEConnectionInfo.BD_ADDR)))
		{
			/* Verify that we can send a notification to this device. */
			if((_deviceInfo->BASServerInformation.Battery_Level_Client_Configuration & GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE) && \
			   ((_force) || ((!_force) && (_deviceInfo->LostNotifiedBatteryLevel != _applicationStateInfo->BatteryLevel))))
			{
				/* Attempt to send the notification to the device. */
				_result = BAS_Notify_Battery_Level(_applicationStateInfo->BluetoothStackID, \
						  _applicationStateInfo->BASInstanceID, _applicationStateInfo->LEConnectionInfo.ConnectionID, \
						  (Byte_t)_applicationStateInfo->BatteryLevel);
				if(!_result){_deviceInfo->LostNotifiedBatteryLevel = _applicationStateInfo->BatteryLevel;}
				else{BT_HOGP_DBG_Display(("Error - BAS_Notify_Battery_Level() %d.\r\n", _result));}
			}
		}
	}

	BT_HOGP_DBG_Display(("Timer Callback.\r\n"));
}

/**
  ***************************************************************************************************************************************
  * The following function is a utility function that starts an advertising process.
  ***************************************************************************************************************************************
  */
static int BT_HOGP_Start_Advertising(unsigned int _bluetoothStackID)
{
	int _retVal;
	GAP_LE_Advertising_Parameters_t _advertisingParameters;
	GAP_LE_Connectability_Parameters_t _connectabilityParameters;

	/* First, check that valid Bluetooth Stack ID exists. */
	if(_bluetoothStackID)
	{
		/* Set up the advertising parameters. */
		_advertisingParameters.Advertising_Channel_Map = HCI_LE_ADVERTISING_CHANNEL_MAP_DEFAULT;
		_advertisingParameters.Scan_Request_Filter = fpNoFilter;
		_advertisingParameters.Connect_Request_Filter = fpNoFilter;
		_advertisingParameters.Advertising_Interval_Min = 50;
		_advertisingParameters.Advertising_Interval_Max = 100;

		/** Configure the Connectability Parameters.
		  * * NOTE * Since we do not ever put ourselves to be direct
		  *          connectable then we will set the DirectAddress to all
		  *          0s.
		  */
		_connectabilityParameters.Connectability_Mode = lcmConnectable;
		_connectabilityParameters.Own_Address_Type = latPublic;
		_connectabilityParameters.Direct_Address_Type = latPublic;
		ASSIGN_BD_ADDR(_connectabilityParameters.Direct_Address, 0, 0, 0, 0, 0, 0);

		/* Now enable advertising. */
		_retVal = GAP_LE_Advertising_Enable(_bluetoothStackID, TRUE, &_advertisingParameters, \
				  	  	  	  	  	  	    &_connectabilityParameters, BT_HOGP_GAP_LE_Event_Callback, 0);
		if(!_retVal){BT_HOGP_DBG_Display(("GAP_LE_Advertising_Enable success.\r\n"));}
		else
		{
			BT_HOGP_DBG_Display(("GAP_LE_Advertising_Enable returned %d.\r\n", _retVal));
			_retVal = BT_HOGP_FUNCTION_ERROR;
		}
	}
	else
	{
		/* No valid Bluetooth Stack ID exists. */
		_retVal = BT_HOGP_INVALID_STACK_ID_ERROR;
	}

	return _retVal;
}

/**
  ***************************************************************************************************************************************
  * The following function is a utility function that is used to
  * notify the Keyboard Report to the connected LE device (if the
  * device has registered for notifications).
  ***************************************************************************************************************************************
  */
static void BT_HOGP_Notify_Keyboard_Report(bt_hogp_application_state_info_ts *_applicationStateInfo)
{
	int _result;
	bt_hogp_device_info_ts *_deviceInfo;
	HIDS_Report_Reference_Data_t _reportReferenceData;

	/* Make sure the input parameters are semi-valid. */
	if((_applicationStateInfo) && (_applicationStateInfo->Flags & BT_HOGP_APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED))
	{
		/* Search for the device info structure for this device. */
		if(NULL != (_deviceInfo = BT_HOGP_Search_Device_Info_Entry_By_BD_ADDR(&BT_HOGP_DeviceInfoList, \
																	 	 	  _applicationStateInfo->LEConnectionInfo.AddressType, \
																			  _applicationStateInfo->LEConnectionInfo.BD_ADDR)))
		{
			/** Verify that the connection has registered for the current
			  * notifications based on the operating mode and that the link
			  * is currently encrypted.
			  */
			if(_applicationStateInfo->LEConnectionInfo.Flags & BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED)
			{
				/* Check to see what characteristic should be notified based on the operating mode. */
				_reportReferenceData.ReportID = 0;
				_reportReferenceData.ReportType = HIDS_REPORT_REFERENCE_REPORT_TYPE_INPUT_REPORT;

				/* Send the Report Mode Input Report notification. */
				_result = HIDS_Notify_Input_Report(_applicationStateInfo->BluetoothStackID, \
												  _applicationStateInfo->HIDSInstanceID, \
												  _applicationStateInfo->LEConnectionInfo.ConnectionID, rtReport, \
												  &_reportReferenceData, BT_HOGP_HID_KEYBOARD_INPUT_REPORT_SIZE, \
												  _applicationStateInfo->CurrentInputReport);

				_applicationStateInfo->CurrentInputReport[0] = 0;
				_applicationStateInfo->CurrentInputReport[2] = 0;
				/* Send the Report Mode Input Report notification. */
				_result |= HIDS_Notify_Input_Report(_applicationStateInfo->BluetoothStackID, \
												    _applicationStateInfo->HIDSInstanceID, \
												    _applicationStateInfo->LEConnectionInfo.ConnectionID, rtReport, \
												    &_reportReferenceData, BT_HOGP_HID_KEYBOARD_INPUT_REPORT_SIZE, \
												    _applicationStateInfo->CurrentInputReport);


				/* Check to see if any error occurred. */
				if(_result != BT_HOGP_HID_KEYBOARD_INPUT_REPORT_SIZE)
				{BT_HOGP_DBG_Display(("Error - HIDS_Notify_Input_Report() returned %d for %s mode.\r\n", _result, \
									 (pmBoot == _applicationStateInfo->HIDProtocolMode)?"Boot":"Keyboard"));}

				BT_HOGP_DBG_Display(("No Error %d \r\n",_deviceInfo->ReportKeyboardInputConfiguration));
			}else{BT_HOGP_DBG_Display(("Error %d \r\n",_deviceInfo->ReportKeyboardInputConfiguration));}
		}
	}
}

/**
  ***************************************************************************************************************************************
  * Check the mailbox status
  ***************************************************************************************************************************************
  */
uint8_t BT_HOGP_Check_Mailbox_Status(void)
{
	if(BT_HOGP_ApplicationStateInfo.Mailbox){return 1U;}
	else{return 0U;}
}

/**
  ***************************************************************************************************************************************
  * Main HOGP task handler
  ***************************************************************************************************************************************
  */
void BT_HOGP_Task_Handler(void)
{
	int _result;
	Byte_t _messageID;
	GAP_LE_Authentication_Response_Information_t _gapLeAuthenticationResponseInformation;

	/* Process the scheduler. */
	BTPS_ProcessScheduler();

	/* Wait on the application mailbox. */
	if(BTPS_WaitMailbox(BT_HOGP_ApplicationStateInfo.Mailbox, &_messageID))
	{
		switch(_messageID)
		{
			case BT_HOGP_APPLICATION_MAILBOX_MESSAGE_ID_LE_CONNECTED:
				/* Notify the battery level if necessary. */
				BT_HOGP_ApplicationStateInfo.LEConnectionInfo.connectionFlag = 1U;
				BT_HOGP_ApplicationStateInfo.BatteryLevel = BAT_Get_SOC();
				BT_HOGP_Notify_Battery_Level(&BT_HOGP_ApplicationStateInfo, TRUE);
			break;

			case BT_HOGP_APPLICATION_MAILBOX_MESSAGE_ID_LE_DISCONNECTED:
				/* Reset the HID Protocol Mode to the default mode (Report Mode). */
				BT_HOGP_ApplicationStateInfo.HIDProtocolMode = pmReport;
				BT_HOGP_ApplicationStateInfo.LEConnectionInfo.connectionFlag = 0U;
				/* Start an advertising process. */
				BT_HOGP_Start_Advertising(BT_HOGP_ApplicationStateInfo.BluetoothStackID);

				/* Clear the LE Connection Information. */
				BTPS_MemInitialize(&(BT_HOGP_ApplicationStateInfo.LEConnectionInfo), 0U, sizeof(BT_HOGP_ApplicationStateInfo.LEConnectionInfo));

				/* Clear the LE Connection Flag. */
				BT_HOGP_ApplicationStateInfo.Flags &= ~BT_HOGP_APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED;
			break;

			case BT_HOGP_APPLICATION_MAILBOX_MESSAGE_ID_PASSKEY_ENTERED:
				BT_HOGP_DBG_Display(("Responding with Passkey = %06lu.\r\n", BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Passkey));

				/* Verify that we are still waiting on a passkey. */
				if(BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Flags & BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_AWAITING_PASSKEY)
				{
					/* Flag that we are no longer waiting on a passkey. */
					BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Flags &= ~BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_AWAITING_PASSKEY;

					/** Parameters appear to be valid, go ahead and
					  * populate the response structure.
					  */
					_gapLeAuthenticationResponseInformation.GAP_LE_Authentication_Type = larPasskey;
					_gapLeAuthenticationResponseInformation.Authentication_Data_Length = (Byte_t)(sizeof(DWord_t));
					_gapLeAuthenticationResponseInformation.Authentication_Data.Passkey = (DWord_t)(BT_HOGP_ApplicationStateInfo.LEConnectionInfo.Passkey);

					/* Submit the Authentication Response. */
					_result = GAP_LE_Authentication_Response(BT_HOGP_ApplicationStateInfo.BluetoothStackID, \
															 BT_HOGP_ApplicationStateInfo.LEConnectionInfo.BD_ADDR, \
															 &_gapLeAuthenticationResponseInformation);
					if(!_result){BT_HOGP_DBG_Display(("Successfully responded with passkey.\r\n"));}
					else{BT_HOGP_DBG_Display(("Error - GAP_LE_Authentication_Response() %d when responding with passkey.\r\n", _result));}
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
void BT_HOGP_Send_Data_Reports(uint8_t* _data, uint8_t _nbr)
{
	if(BT_HOGP_ApplicationStateInfo.LEConnectionInfo.connectionFlag)
	{
		for(uint8_t _idx = 0U; _idx < _nbr; _idx++)
		{
			BT_HOGP_ApplicationStateInfo.CurrentInputReport[0] = ((BT_HOGP_OGP_HID_KEYS[*_data] & 0x80) ? 0x02 : 0x00);
			BT_HOGP_ApplicationStateInfo.CurrentInputReport[2] = (BT_HOGP_OGP_HID_KEYS[*_data++] & 0x7F);
			BT_HOGP_Notify_Keyboard_Report(&BT_HOGP_ApplicationStateInfo);
		}

		/* TAB */
		BT_HOGP_ApplicationStateInfo.CurrentInputReport[0] = ((BT_HOGP_OGP_HID_KEYS[9] & 0x80) ? 0x02 : 0x00);
		BT_HOGP_ApplicationStateInfo.CurrentInputReport[2] = (BT_HOGP_OGP_HID_KEYS[9] & 0x7F);
		BT_HOGP_Notify_Keyboard_Report(&BT_HOGP_ApplicationStateInfo);
	}
}

/**
  ***************************************************************************************************************************************
  * @brief Update battery level
  * @param None
  * @retval None
  ***************************************************************************************************************************************
  */
void BT_HOGP_Update_Battery_Level(void)
{
	if(BT_HOGP_ApplicationStateInfo.LEConnectionInfo.connectionFlag)
	{
		BT_HOGP_ApplicationStateInfo.BatteryLevel = BAT_Get_SOC();
		BT_HOGP_Notify_Battery_Level(&BT_HOGP_ApplicationStateInfo, TRUE);
	}
}

/**
  ***************************************************************************************************************************************
  * @brief Get bluetooth connection state
  * @param None
  * @retval Status (uint8_t)
  ***************************************************************************************************************************************
  */
uint8_t BT_HOGP_Get_Connection_Status(void)
{
	return BT_HOGP_ApplicationStateInfo.LEConnectionInfo.connectionFlag;
}

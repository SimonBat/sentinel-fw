#ifndef __BT_HOGP_H
#define __BT_HOGP_H

#include <stdio.h>
#include "stm32l4xx_hal.h"
#include "SS1BTPS.h" /* Main SS1 BT Stack Header */
#include "SS1BTGAT.h" /* Main SS1 GATT Header */
#include "SS1BTGAP.h" /* Main SS1 GAP Service Header */
#include "SS1BTBAS.h" /* Main SS1 BAS Service Header */
#include "SS1BTDIS.h" /* Main SS1 DIS Service Header */
#include "SS1BTHIDS.h" /* Main SS1 HIDS Service Header */
#include "BTPSKRNL.h" /* BTPS Kernel Header */
#include "HCITRANS.h" /* HCI Transport Layer Header */

#define BT_HOGP_APPLICATION_ERROR_INVALID_PARAMETERS       			(-1000)
#define BT_HOGP_APPLICATION_ERROR_UNABLE_TO_OPEN_STACK     			(-1001)
#define BT_HOGP_MAX_SUPPORTED_LINK_KEYS                    			(1) /* Max supported Link*/
#define BT_HOGP_FUNCTION_ERROR                             			(-4) /* Denotes that an */
/* Error occurred in execution of the Command Function */
#define BT_HOGP_INVALID_PARAMETERS_ERROR                   			(-6) /* Denotes that an */
/* Error occurred due to the fact that one or more of the required parameters were invalid */
#define BT_HOGP_UNABLE_TO_INITIALIZE_STACK                 			(-7) /* Denotes that an */
/* Error occurred while Initializing the Bluetooth Protocol Stack */
#define BT_HOGP_INVALID_STACK_ID_ERROR                     			(-8) /* Denotes that an */
/* Error occurred due to attempted execution of a Command when a Bluetooth Protocol Stack has not been opened */
/* The following defines the size of the application Keyboard Input Report */
#define BT_HOGP_HID_KEYBOARD_INPUT_REPORT_SIZE                  	8
#define BT_HOGP_APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED       	0x01
#define BT_HOGP_APPLICATION_STATE_INFO_FLAGS_CAPS_LOCKED   			0x02
/* Determine the Name we will use for this compilation */
#define BT_HOGP_LE_DEMO_DEVICE_NAME                              	"PWD_Manager"

/* The following define the PnP values that are assigned in the PnP */
/* ID characteristic of the Device Information Service */
#define BT_HOGP_PNP_ID_VENDOR_ID_STONESTREET_ONE                 	0x005E
#define BT_HOGP_PNP_ID_PRODUCT_ID                                	0x5555
#define BT_HOGP_PNP_ID_PRODUCT_VERSION                           	0x0001

/* Defines the bitmask flags that may be set in the Flags member of the DeviceInfo_t structure */
#define BT_HOGP_DEVICE_INFO_FLAGS_IRK_VALID                      	0x01
#define BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED          0x01
#define BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_AWAITING_PASSKEY   0x02
#define BT_HOGP_CONNECTION_INFO_FLAGS_CONNECTION_VALID              0x80
/* The following define the valid bits that may be set as part of the Keyboard Output Report */
#define BT_HOGP_HID_KEYBOARD_OUTPUT_REPORT_NUM_LOCK              	0x01
#define BT_HOGP_HID_KEYBOARD_OUTPUT_REPORT_CAPS_LOCK             	0x02
#define BT_HOGP_HID_KEYBOARD_OUTPUT_REPORT_SCOLL_LOCK            	0x04
#define BT_HOGP_HID_KEYBOARD_OUTPUT_REPORT_COMPOSE               	0x08
#define BT_HOGP_HID_KEYBOARD_OUTPUT_REPORT_KANA                  	0x10
/* The following defines are used with the application mailbox */
#define BT_HOGP_APPLICATION_MAILBOX_DEPTH                           16
#define BT_HOGP_APPLICATION_MAILBOX_SIZE                            BYTE_SIZE
#define BT_HOGP_APPLICATION_MAILBOX_MESSAGE_ID_LE_DISCONNECTED      0x01
#define BT_HOGP_APPLICATION_MAILBOX_MESSAGE_ID_LE_CONNECTED         0x02
#define BT_HOGP_APPLICATION_MAILBOX_MESSAGE_ID_PASSKEY_ENTERED      0x04

typedef char BT_HOGP_BOARD_STR_T[16];

/* The following structure is a container for information on connected devices */
typedef struct {
	unsigned char Flags;
	unsigned int ConnectionID;
	unsigned int PasskeyDigits;
	unsigned long Passkey;
	GAP_LE_Address_Type_t AddressType;
	BD_ADDR_t BD_ADDR;
	unsigned int SecurityTimerID;
	unsigned char connectionFlag;
} bt_hogp_connectionInfo_ts;

/* The following structure is used to hold all of the application state information */
typedef struct {
	unsigned int BluetoothStackID;
	unsigned int HCIEventCallbackHandle;
	Byte_t Flags;
	Mailbox_t Mailbox;
	HIDS_Protocol_Mode_t HIDProtocolMode;
	Byte_t CurrentInputReport[BT_HOGP_HID_KEYBOARD_INPUT_REPORT_SIZE];
	Byte_t CurrentOutputReport;
	unsigned int GAPSInstanceID;
	unsigned int DISInstanceID;
	unsigned int BASInstanceID;
	unsigned int HIDSInstanceID;
	unsigned int CurrentKeyFob;
	unsigned int NumberOfConnections;
	unsigned int BatteryLevel;
	bt_hogp_connectionInfo_ts LEConnectionInfo;
	unsigned int SPPServerPortID;
	DWord_t SPPServerSDPHandle;
	unsigned long ConnectTimer;
	unsigned long PreviousTickCount;
} bt_hogp_application_state_info_ts;

/* Structure used to hold all of the GAP LE Parameters */
typedef struct {
	GAP_LE_IO_Capability_t IOCapability;
	Boolean_t MITMProtection;
	Boolean_t OOBDataPresent;
} bt_hogp_GAPLE_parameters_ts;

/* The following structure for is used to hold a list of information on all paired devices */
typedef struct _tagDeviceInfo_t {
	Byte_t Flags;
	Byte_t EncryptionKeySize;
	GAP_LE_Address_Type_t AddressType;
	BD_ADDR_t BD_ADDR;
	BAS_Server_Information_t BASServerInformation;
	Word_t BootKeyboardInputConfiguration;
	Word_t ReportKeyboardInputConfiguration;
	unsigned int LostNotifiedBatteryLevel;
	Encryption_Key_t IRK;
	struct _tagDeviceInfo_t *NextDeviceInfoPtr;
} bt_hogp_device_info_ts;

/* Global functions definitions */
int BT_HOGP_Application_Init(HCI_DriverInformation_t *_hciDriverInformation, BTPS_Initialization_t *_btpsInitialization);
uint8_t BT_HOGP_Check_Mailbox_Status(void);
void BT_HOGP_Task_Handler(void);
void BT_HOGP_Send_Data_Reports(const uint8_t* _data, uint8_t _nbr);
void BT_HOGP_Update_Battery_Level(void);
uint8_t BT_HOGP_Get_Connection_Status(void);

#endif

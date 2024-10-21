#ifndef __BT_HOGP_H
#define __BT_HOGP_H

#include "stm32l4xx_hal.h"
#include "stdio.h"
#include "SS1BTPS.h" /* Main SS1 BT Stack Header */
#include "SS1BTGAT.h" /* Main SS1 GATT Header */
#include "SS1BTGAP.h" /* Main SS1 GAP Service Header */
#include "SS1BTBAS.h" /* Main SS1 BAS Service Header */
#include "SS1BTDIS.h" /* Main SS1 DIS Service Header */
#include "SS1BTHIDS.h" /* Main SS1 HIDS Service Header */
#include "BTPSKRNL.h" /* BTPS Kernel Header */
#include "HCITRANS.h" /* HCI Transport Layer Header */

#define HOGP_APPLICATION_ERROR_INVALID_PARAMETERS       	(-1000)
#define HOGP_APPLICATION_ERROR_UNABLE_TO_OPEN_STACK     	(-1001)
#define HOGP_MAX_SUPPORTED_LINK_KEYS                    	(1) /* Max supported Link*/
#define HOGP_FUNCTION_ERROR                             	(-4) /* Denotes that an */
/* Error occurred in execution of the Command Function */
#define HOGP_INVALID_PARAMETERS_ERROR                   	(-6) /* Denotes that an */
/* Error occurred due to the fact that one or more of the required parameters were invalid */
#define HOGP_UNABLE_TO_INITIALIZE_STACK                 	(-7) /* Denotes that an */
/* Error occurred while Initializing the Bluetooth Protocol Stack */
#define HOGP_INVALID_STACK_ID_ERROR                     	(-8) /* Denotes that an */
/* Error occurred due to attempted execution of a Command when a Bluetooth Protocol Stack has not been opened */
/* The following defines the size of the application Keyboard Input Report */
#define HID_KEYBOARD_INPUT_REPORT_SIZE                  	8
#define APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED       	0x01
#define APPLICATION_STATE_INFO_FLAGS_CAPS_LOCKED   			0x02
/* Determine the Name we will use for this compilation */
#define LE_DEMO_DEVICE_NAME                              	"PWD_Manager"

/* The following define the PnP values that are assigned in the PnP */
/* ID characteristic of the Device Information Service */
#define PNP_ID_VENDOR_ID_STONESTREET_ONE                 	0x005E
#define PNP_ID_PRODUCT_ID                                	0x5555
#define PNP_ID_PRODUCT_VERSION                           	0x0001

/* Defines the bitmask flags that may be set in the Flags member of the DeviceInfo_t structure */
#define DEVICE_INFO_FLAGS_IRK_VALID                      	0x01
#define CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED          0x01
#define CONNECTION_INFO_FLAGS_CONNECTION_AWAITING_PASSKEY   0x02
#define CONNECTION_INFO_FLAGS_CONNECTION_VALID              0x80
/* The following define the valid bits that may be set as part of the Keyboard Output Report */
#define HID_KEYBOARD_OUTPUT_REPORT_NUM_LOCK              	0x01
#define HID_KEYBOARD_OUTPUT_REPORT_CAPS_LOCK             	0x02
#define HID_KEYBOARD_OUTPUT_REPORT_SCOLL_LOCK            	0x04
#define HID_KEYBOARD_OUTPUT_REPORT_COMPOSE               	0x08
#define HID_KEYBOARD_OUTPUT_REPORT_KANA                  	0x10
/* The following defines are used with the application mailbox */
#define APPLICATION_MAILBOX_DEPTH                           16
#define APPLICATION_MAILBOX_SIZE                            BYTE_SIZE
#define APPLICATION_MAILBOX_MESSAGE_ID_LE_DISCONNECTED      0x01
#define APPLICATION_MAILBOX_MESSAGE_ID_LE_CONNECTED         0x02
#define APPLICATION_MAILBOX_MESSAGE_ID_PASSKEY_ENTERED      0x04

typedef char BoardStr_t[16];

/* The following structure is a container for information on connected devices */
typedef struct _tagConnectionInfo_t
{
	unsigned char         		Flags;
	unsigned int          		ConnectionID;
	unsigned int          		PasskeyDigits;
	unsigned long         		Passkey;
	GAP_LE_Address_Type_t 		AddressType;
	BD_ADDR_t             		BD_ADDR;
	unsigned int          		SecurityTimerID;
	unsigned char 				connectionFlag;
}ConnectionInfo_t;

/* The following structure is used to hold all of the application state information */
typedef struct _tagApplicationStateInfo_t
{
	unsigned int         		BluetoothStackID;
	unsigned int         		HCIEventCallbackHandle;
	Byte_t               		Flags;
	Mailbox_t            		Mailbox;
	HIDS_Protocol_Mode_t 		HIDProtocolMode;
	Byte_t               		CurrentInputReport[HID_KEYBOARD_INPUT_REPORT_SIZE];
	Byte_t               		CurrentOutputReport;
	unsigned int         		GAPSInstanceID;
	unsigned int         		DISInstanceID;
	unsigned int         		BASInstanceID;
	unsigned int         		HIDSInstanceID;
	unsigned int         		CurrentKeyFob;
	unsigned int         		NumberOfConnections;
	unsigned int         		BatteryLevel;
	ConnectionInfo_t     		LEConnectionInfo;
	unsigned int         		SPPServerPortID;
	DWord_t              		SPPServerSDPHandle;
	unsigned long        		ConnectTimer;
	unsigned long        		PreviousTickCount;
}ApplicationStateInfo_t;

/* Structure used to hold all of the GAP LE Parameters */
typedef struct _tagGAPLE_Parameters_t
{
	GAP_LE_IO_Capability_t		IOCapability;
	Boolean_t               	MITMProtection;
	Boolean_t               	OOBDataPresent;
}GAPLE_Parameters_t;

/* The following structure for is used to hold a list of information on all paired devices */
typedef struct _tagDeviceInfo_t
{
	Byte_t                   	Flags;
	Byte_t                   	EncryptionKeySize;
	GAP_LE_Address_Type_t    	AddressType;
	BD_ADDR_t                	BD_ADDR;
	BAS_Server_Information_t 	BASServerInformation;
	Word_t                   	BootKeyboardInputConfiguration;
	Word_t                   	ReportKeyboardInputConfiguration;
	unsigned int             	LostNotifiedBatteryLevel;
	Encryption_Key_t         	IRK;
	struct _tagDeviceInfo_t  	*NextDeviceInfoPtr;
}DeviceInfo_t;

/* Global functions definitions */
int HOGP_Application_Init(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization);
uint8_t HOGP_Check_Mailbox_Status(void);
void HOGP_Task_Handler(void);
void HOGP_Send_Data_Reports(uint8_t* _data, uint8_t _nbr);
void HOGP_Update_Battery_Level(void);
uint8_t HOGP_Get_Connection_Status(void);

#endif

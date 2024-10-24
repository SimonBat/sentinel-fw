/*****< scpsapi.h >************************************************************/
/*      Copyright 2012 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  SCPSAPI - Stonestreet One Bluetooth Scan Parameters Service (GATT         */
/*            based) API Type Definitions, Constants, and Prototypes.         */
/*                                                                            */
/*  Author:  Ajay Parashar                                                    */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   08/09/12  A. Parashar    Initial creation.                               */
/******************************************************************************/
#ifndef __SCPSAPIH__
#define __SCPSAPIH__

#include "SS1BTPS.h"         /* Bluetooth Stack API Prototypes/Constants.     */
#include "SS1BTGAT.h"        /* Bluetooth Stack GATT API Prototypes/Constants.*/
#include "SCPSType.h"        /* Scan Parameters Service Types/Constants.      */

   /* Error Return Codes.                                               */

   /* Error Codes that are smaller than these (less than -1000) are     */
   /* related to the Bluetooth Protocol Stack itself (see BTERRORS.H).  */
#define SCPS_ERROR_INVALID_PARAMETER                      (-1000)
#define SCPS_ERROR_INVALID_BLUETOOTH_STACK_ID             (-1001)
#define SCPS_ERROR_INSUFFICIENT_RESOURCES                 (-1002)
#define SCPS_ERROR_SERVICE_ALREADY_REGISTERED             (-1003)
#define SCPS_ERROR_INVALID_INSTANCE_ID                    (-1004)
#define SCPS_ERROR_MALFORMATTED_DATA                      (-1005)
#define SCPS_ERROR_UNKNOWN_ERROR                          (-1006)

   /* This defines the format of a _tagSCPS_Scan_Interval_Window_Data_t */
   /* This is used to represent  Scan Interval Window                   */
typedef struct _tagSCPS_Scan_Interval_Window_Data_t
{
   Word_t LE_Scan_Interval;
   Word_t LE_Scan_Window;
} SCPS_Scan_Interval_Window_Data_t;

#define SCPS_SCAN_INTERVAL_WINDOW_DATA_SIZE              (sizeof(SCPS_Scan_Interval_Window_Data_t))

   /* The following define the valid Read Request types that a server   */
   /* may receive in a etSCPS_Server_Read_Client_Configuration_Request  */
   /* or etSCPS_Server_Update_Client_Configuration_Request event.This is*/
   /* used by the SCPS_Send_Notification to denote the characteristic   */
   /* value to notify.                                                  */
   /* * NOTE * For each event it is up to the application to return (or */
   /*          write) the correct Client Configuration descriptor based */
   /*          on this value.                                           */
typedef enum
{
   ctScanRefresh
} SCPS_Characteristic_Type_t;

   /* The following enumeration covers all the events generated by the  */
   /* SCPS Service.These are used to determine the type of each event   */
   /* generated, and to ensure the proper union element is accessed for */
   /* the SCPS_Event_Data_t structure.                                  */
typedef enum
{
   etSCPS_Server_Read_Client_Configuration_Request,
   etSCPS_Server_Update_Client_Configuration_Request,
   etSCPS_Server_Write_Scan_Interval_Window_Request
} SCPS_Event_Type_t;

   /* The  SCPS Service Event is dispatched to a SCPS Server when       */
   /* a SCPSClient is attempting to read a descriptor. The ConnectionID */
   /* ConnectionType, and RemoteDevice specifiy the Client that is      */
   /* making the request.The DescriptorType specifies the Descriptor    */
   /* that the Client is attempting to read.The TransactionID specifies */
   /* the TransactionID of the request, this can be used when responding*/
   /* to the request using the SCPS_Client_Configuration_Read_Response()*/
   /* API function.                                                     */
typedef struct _tagSCPS_Read_Client_Configuration_Data_t
{
   unsigned int               InstanceID;
   unsigned int               ConnectionID;
   unsigned int               TransactionID;
   GATT_Connection_Type_t     ConnectionType;
   BD_ADDR_t                  RemoteDevice;
   SCPS_Characteristic_Type_t ClientConfigurationType;
} SCPS_Read_Client_Configuration_Data_t;

#define SCPS_READ_CLIENT_CONFIGURATION_DATA_SIZE         (sizeof(SCPS_Read_Client_Configuration_Data_t))

   /* The following SCPS Service Event is dispatched to SCPS Server when*/
   /* a SCPS Client has written a Client Configuration descriptor.  The */
   /* ConnectionID, ConnectionType, and RemoteDevice specifiy the Client*/
   /* that is making the update.  The ClientConfigurationType specifies */
   /* the Descriptor that the Client is writing.  The final member is   */
   /* the new Client Configuration for the specified characteristic.    */
typedef struct _tagSCPS_Client_Configuration_Update_Data_t
{
   unsigned int               InstanceID;
   unsigned int               ConnectionID;
   GATT_Connection_Type_t     ConnectionType;
   BD_ADDR_t                  RemoteDevice;
   SCPS_Characteristic_Type_t ClientConfigurationType;
   Word_t                     ClientConfiguration;
} SCPS_Client_Configuration_Update_Data_t;

#define SCPS_CLIENT_CONFIGURATION_UPDATE_DATA_SIZE       (sizeof(SCPS_Client_Configuration_Update_Data_t))


   /* The following is dispatched to a SCPS Server in response to the   */
   /* reception of request from a Client to write to the Scan Interval  */
   /* Window characteristics.                                           */
typedef struct _tagSCPS_Write_Scan_Interval_Window_Data_t
{
   unsigned int                     InstanceID;
   unsigned int                     ConnectionID;
   GATT_Connection_Type_t           ConnectionType;
   BD_ADDR_t                        RemoteDevice;
   SCPS_Scan_Interval_Window_Data_t ScanIntervalWindowData;
} SCPS_Write_Scan_Interval_Window_Data_t;

#define SCPS_WRITE_SCAN_INTERVAL_WINDOW_DATA_SIZE       (sizeof(SCPS_Write_Scan_Interval_Window_Data_t))

   /* The following structure represents the container structure for    */
   /* holding all SCPS Service Event Data.  This structure is received  */
   /* for each event generated.  The Event_Data_Type member is used to  */
   /* determine the appropriate union member element to access the      */
   /* contained data.  The Event_Data_Size member contains the total    */
   /* size of the data contained in this event.                         */
typedef struct _tagSCPS_Event_Data_t
{
   SCPS_Event_Type_t Event_Data_Type;
   Word_t            Event_Data_Size;
   union
   {
     SCPS_Read_Client_Configuration_Data_t   *SCPS_Read_Client_Configuration_Data;
     SCPS_Client_Configuration_Update_Data_t *SCPS_Client_Configuration_Update_Data;
     SCPS_Write_Scan_Interval_Window_Data_t  *SCPS_Write_Scan_Interval_Window_Data;
   } Event_Data;
} SCPS_Event_Data_t;

#define SCPS_EVENT_DATA_SIZE                             (sizeof(SCPS_Event_Data_t))

   /* The following structure contains the Handles that will need to be */
   /* cached by a SCPS client in order to only do service discoveryonce.*/
typedef struct _tagSCPS_Client_Information_t
{
   Word_t Scan_Interval_Window;
   Word_t Scan_Refresh;
   Word_t Scan_Refresh_Client_Configuration;
} SCPS_Client_Information_t;

#define SCPS_CLIENT_INFORMATION_DATA_SIZE                (sizeof(SCPS_Client_Information_t))

   /* The following structure contains all of the per Client data that  */
   /* will need to be stored by a SCPS Server.                          */
typedef struct _tagSCPS_Server_Information_t
{
   Word_t Scan_Refresh_Client_Configuration;
} SCPS_Server_Information_t;

#define SCPS_SERVER_INFORMATION_DATA_SIZE                (sizeof(SCPS_Server_Information_t))

   /* The following declared type represents the Prototype Function for */
   /* a SCPS Service Event Receive Data Callback.  This function will be*/
   /* called whenever an SCPS Service Event occurs that is associated   */
   /* with the specified Bluetooth Stack ID.  This function passes to   */
   /* the caller the Bluetooth Stack ID, the SCPS Event Data that       */
   /* occurred and the SCPS Service Event Callback Parameter that was   */
   /* specified when this Callback was installed.  The caller is free to*/
   /* use the contents of the SCPS Service Event Data ONLY in  context  */
   /* of this callback.  If the caller requires the Data for a longer   */
   /* period of time, then the callback function MUST copy the data into*/
   /* another Data Buffer This function is guaranteed NOT to be invoked */
   /* more than once simultaneously for the specified installed callback*/
   /* (i.e.  this function DOES NOT have be re-entrant).  It needs to be*/
   /* noted however, that if the same Callback is installed more than   */
   /* once, then the callbacks will be called serially.  Because of     */
   /* this, the processing in this function should be as efficient as   */
   /* possible.  It should also be noted that this function is called in*/
   /* the Thread Context of a Thread that the User does NOT own.        */
   /* Therefore, processing in this function should be as efficient as  */
   /* possible (this argument holds anyway because another SCPS Service */
   /* Event will not be processed while this function call is           */
   /* outstanding).                                                     */
   /* ** NOTE ** This function MUST NOT Block and wait for events that  */
   /*            can only be satisfied by Receiving SCPS Service Event  */
   /*            Packets.  A Deadlock WILL occur because NO SCPS Event  */
   /*            Callbacks will be issued while this function is        */
   /*            currently outstanding.                                 */
typedef void (BTPSAPI *SCPS_Event_Callback_t)(unsigned int BluetoothStackID, SCPS_Event_Data_t *SCPS_Event_Data, unsigned long CallbackParameter);

   /* SCPS Server API.                                                  */

   /* The following function is responsible for opening a SCPS Server.  */
   /* The first parameter is the Bluetooth Stack ID on which to open the*/
   /* server.  The second parameter is the Callback function to call    */
   /* when an event occurs on this Server Port.  The third parameter is */
   /* a user-defined callback parameter that will be passed to the      */
   /* callback function with each event.  The final parameter is a      */
   /* pointer to store the GATT Service ID of the registered SCPS       */
   /* service.  This can be used to include the service registered by   */
   /* this call.  This function returns the positive, non-zero, Instance*/
   /* ID or a negative error code.                                      */
   /* * NOTE * Only 1 SCPSServer may be open at a time, per Bluetooth   */
   /*          Stack ID.                                                */
   /* * NOTE * All Client Requests will be dispatch to the EventCallback*/
   /*          function that is specified by the second parameter to    */
   /*          this function.                                           */
BTPSAPI_DECLARATION int BTPSAPI SCPS_Initialize_Service(unsigned int BluetoothStackID, SCPS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_SCPS_Initialize_Service_t)(unsigned int BluetoothStackID, SCPS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID);
#endif

   /* The following function is responsible for opening a SCPS Server.  */
   /* The first parameter is the Bluetooth Stack ID on which to open the*/
   /* server.  The second parameter is the Callback function to call    */
   /* when an event occurs on this Server Port.  The third parameter is */
   /* a user-defined callback parameter that will be passed to the      */
   /* callback function with each event.  The fourth parameter is a     */
   /* pointer to store the GATT Service ID of the registered SCPS       */
   /* service.  This can be used to include the service registered by   */
   /* this call.  The final parameter is a pointer, that on input can be*/
   /* used to control the location of the service in the GATT database, */
   /* and on ouput to store the service handle range.  This function    */
   /* returns the positive, non-zero, Instance ID or a negative error   */
   /* code.                                                             */
   /* * NOTE * Only 1 SCPS Server may be open at a time, per Bluetooth  */
   /*          Stack ID.                                                */
   /* * NOTE * All Client Requests will be dispatch to the EventCallback*/
   /*          function that is specified by the second parameter to    */
   /*          this function.                                           */
BTPSAPI_DECLARATION int BTPSAPI SCPS_Initialize_Service_Handle_Range(unsigned int BluetoothStackID, SCPS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID, GATT_Attribute_Handle_Group_t *ServiceHandleRange);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_SCPS_Initialize_Service_Handle_Range_t)(unsigned int BluetoothStackID, SCPS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID, GATT_Attribute_Handle_Group_t *ServiceHandleRange);
#endif

   /* The following function is responsible for closing a previously    */
   /* opened SCPS Server.  The first parameter is the Bluetooth Stack ID*/
   /* on which to close the server.  The second parameter is the        */
   /* InstanceID that was returned from a successful call to            */
   /* SCPS_Initialize_Service().  This function returns a zero if       */
   /* successful or a negative return error code if an error occurs.    */
BTPSAPI_DECLARATION int BTPSAPI SCPS_Cleanup_Service(unsigned int BluetoothStackID, unsigned int InstanceID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_SCPS_Cleanup_Service_t)(unsigned int BluetoothStackID, unsigned int InstanceID);
#endif

   /* The following function is responsible for querying the number of  */
   /* attributes that are contained in the SCPS Service that is         */
   /* registered with a call to SCPS_Initialize_Service().  This        */
   /* function returns the non-zero number of attributes that are       */
   /* contained in a SCPS Server or zero on failure.                    */
BTPSAPI_DECLARATION unsigned int BTPSAPI SCPS_Query_Number_Attributes(void);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef unsigned int (BTPSAPI *PFN_SCPS_Query_Number_Attributes_t)(void);
#endif

   /* The following function is responsible for responding to a SCPSRead*/
   /* Client Configuration Request.  The first parameter is the         */
   /* Bluetooth Stack ID of the Bluetooth Device.  The second parameter */
   /* is the InstanceID returned from a successful call to              */
   /* SCPS_Initialize_Server().  The third is the Transaction ID of the */
   /* request.  The final parameter contains the Client Configuration to*/
   /* send to the remote device.  This function returns a zero if       */
   /* successful or a negative return error code if an error occurs.    */
BTPSAPI_DECLARATION int BTPSAPI SCPS_Read_Client_Configuration_Response(unsigned int BluetoothStackID, unsigned int InstanceID, unsigned int TransactionID, Word_t Client_Configuration);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_SCPS_Read_Client_Configuration_Response_t)(unsigned int BluetoothStackID, unsigned int InstanceID, unsigned int TransactionID, Word_t Client_Configuration);
#endif

   /* The following function is responsible for sending an Scan         */
   /* Refresh notification to a specified remote device.  The first     */
   /* parameter is the Bluetooth Stack ID of the Bluetooth Device.  The */
   /* second parameter is the InstanceID returned from a successful call*/
   /* to SCPS_Initialize_Server().  The third parameter is the          */
   /* ConnectionID of the remote device to send the notification to.    */
   /* The fourth parameter is the Scan Refresh value. The Scan Refresh  */
   /* value would be 0, which indicates Server requires refresh.        */
   /* This function returns a zero if successful or a negative          */
   /* return error code if an error occurs.                             */
BTPSAPI_DECLARATION int BTPSAPI SCPS_Notify_Scan_Refresh(unsigned int BluetoothStackID, unsigned int InstanceID, unsigned int ConnectionID, Byte_t ScanRefreshValue);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_SCPS_Notify_Scan_Refresh_t)(unsigned int BluetoothStackID, unsigned int InstanceID, unsigned int ConnectionID, Byte_t Scan_RefreshValue);
#endif

   /* SCPS Client API                                                   */

   /* The following function is responsible for formatting a Scan       */
   /* Interval Window Command into a user specified buffer.  The first  */
   /* parameter is the SCPS_Scan_Interval_Window_Data_t to format.  The */
   /* second parameter is BufferLength of the buffer.  The third        */
   /* parameter is Buffer which would contain the Scan Interval Window  */
   /* formatted data.  This function returns a zero if successful or a  */
   /* negative return error code if an error occurs.                    */
   /* * NOTE * The BufferLength and Buffer parameter must point to a    */
   /*          buffer of at least SCPS_SCAN_INTERVAL_WINDOW_SIZE in     */
   /*          size.                                                    */
BTPSAPI_DECLARATION int BTPSAPI SCPS_Format_Scan_Interval_Window(SCPS_Scan_Interval_Window_Data_t *Scan_Interval_Window, unsigned int BufferLength, Byte_t *Buffer);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_SCPS_Format_Scan_Interval_Window_t)(SCPS_Scan_Interval_Window_Data_t *Scan_Interval_Window, unsigned int BufferLength, Byte_t *Buffer);
#endif

#endif

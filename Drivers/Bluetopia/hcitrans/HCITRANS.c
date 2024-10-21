/**
  ***************************************************************************************************************************************
  * @file     HCITRANS.c
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

#include "HCITRANS.h"

static hcitr_uart_context_ts HCITR_UART_HANDLE;

/* Default UART configuration */
static BTPSCONST LL_USART_InitTypeDef HCITR_UART_CONFIG={
	115200L,
   	LL_USART_DATAWIDTH_8B,
   	LL_USART_STOPBITS_1,
   	LL_USART_PARITY_NONE,
   	LL_USART_DIRECTION_TX_RX,
   	LL_USART_HWCONTROL_RTS_CTS,
   	LL_USART_OVERSAMPLING_16
};

/* Local Function Prototypes */
static void HCITR_Tx_Interrupt(void);
static void HCITR_Rx_Interrupt(void);
static void HCITR_Set_Baud_Rate(USART_TypeDef *UartBase, uint32_t BaudRate);
static void HCITR_Configure_GPIO(GPIO_TypeDef *Port, uint32_t Pin, uint32_t Mode, uint32_t Pull, uint32_t Alternate);

/**
  ***************************************************************************************************************************************
  * @brief  The following function will reconfigure the BAUD rate without reconfiguring the entire port.
  *			This function is also potentially more accurate than the method used in the ST standard peripheral libraries.
  * @param  UART (USART_TypeDef*), baud rate (unsigned int)
  * @retval None
  ***************************************************************************************************************************************
  */
static void HCITR_Set_Baud_Rate(USART_TypeDef *UartBase, uint32_t BaudRate)
{
	LL_RCC_ClocksTypeDef RCC_ClocksStatus;
	unsigned int SourceFrequency;
	unsigned int Divider;
	unsigned int TempDiv;

	LL_RCC_GetSystemClocksFreq(&RCC_ClocksStatus);

	#if((HCITR_UART==1)||(HCITR_UART==6))
		SourceFrequency = RCC_ClocksStatus.PCLK2_Frequency;
	#else
		SourceFrequency = RCC_ClocksStatus.PCLK1_Frequency;
	#endif

	/** The following calculation will yield the integer divider
	  * concatenated with the fractional divider.  If 16-bit oversampling
	  * is used, the least significant 4 bits will be the fraction divider
	  * and if 8-bit oversampling is used, the least significant 3 bits
	  * will be the fraction divider.
	  */
	Divider=SourceFrequency/BaudRate;

	/** The integer divider must be between 1 and 4095 so if the divider
	  * value is less than 16, 8-bit oversampling must be used.
	  */
	if(Divider < 16)
	{
		/** When 8-bit oversampling is used, bits [2..0] of the BRR
		  * register represent the fractional divider and bits [15..4]
		  * represent the integer divider.  Because of this, the integer
		  * divider needs to be shifted left one bit before the baud rate
		  * control register is updated.
		  */
		TempDiv = Divider & 0x7;
		Divider = (Divider & ~0x07) << 1;
		Divider |= TempDiv;

		/* Disable the UART while updating the baud rate */
		UartBase->CR1 &= ~USART_CR1_UE;
		UartBase->CR1 |= USART_CR1_OVER8;
	}
	else
	{
		/* For lower frequencies, 16bit oversampling may be used */
		UartBase->CR1 &= ~USART_CR1_UE;
		UartBase->CR1 &= ~USART_CR1_OVER8;
	}

	UartBase->BRR = Divider;
	UartBase->CR1 |= USART_CR1_UE;
}

/**
  ***************************************************************************************************************************************
  * @brief  The following function is a utility function to set the configuration of a specified GPIO. This function accepts as its
  *			parameters the GPIO port, pin and mode of operation.
  * @param  UART (USART_TypeDef*), pin (uint32_t), mode (uint32_t), pull (uint32_t), alternate (uint32_t)
  * @retval None
  ***************************************************************************************************************************************
  */
static void HCITR_Configure_GPIO(GPIO_TypeDef *Port, uint32_t Pin, uint32_t Mode, uint32_t Pull, uint32_t Alternate)
{
	LL_GPIO_InitTypeDef GpioConfiguration;

	/* Setup the configuration structure */
	GpioConfiguration.Pin = 1 << Pin;
	GpioConfiguration.Mode = Mode;
	GpioConfiguration.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	GpioConfiguration.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GpioConfiguration.Pull = Pull;
	GpioConfiguration.Alternate = Alternate;
	/* Configure the GPIO */
	LL_GPIO_Init(Port, &GpioConfiguration);
}

/**
  ***************************************************************************************************************************************
  * @brief  Routine for the UART TX interrupt.
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
static void HCITR_Tx_Interrupt(void)
{
	/* Continue to transmit characters as long as there is data in the buffer and the transmit fifo is empty */
	while((HCITR_UART_HANDLE.TxBytesFree != HCITR_OUTPUT_BUFFER_SIZE) && (HCITR_UART_BASE->ISR & LL_USART_ISR_TXE))
	{
		/* Place the next character into the output buffer */
		LL_USART_TransmitData8(HCITR_UART_BASE, HCITR_UART_HANDLE.TxBuffer[HCITR_UART_HANDLE.TxOutIndex]);

		/* Adjust the character counts and wrap the index if necessary */
		HCITR_UART_HANDLE.TxBytesFree++;
		HCITR_UART_HANDLE.TxOutIndex++;
		if(HCITR_UART_HANDLE.TxOutIndex == HCITR_OUTPUT_BUFFER_SIZE){HCITR_UART_HANDLE.TxOutIndex = 0;}
	}

	/* If there are no more bytes in the queue then disable the transmit interrupt */
	if(HCITR_UART_HANDLE.TxBytesFree == HCITR_OUTPUT_BUFFER_SIZE){LL_USART_DisableIT_TXE(HCITR_UART_BASE);}
}

/**
  ***************************************************************************************************************************************
  * @brief  Routine for the UART RX interrupt.
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
static void HCITR_Rx_Interrupt(void)
{
	/* Continue reading data from the fifo until it is empty or the buffer is full */
	while((HCITR_UART_HANDLE.RxBytesFree) && (HCITR_UART_BASE->ISR & (LL_USART_ISR_RXNE|LL_USART_ISR_ORE)))
	{
		/* Read a character from the port into the receive buffer */
		HCITR_UART_HANDLE.RxBuffer[HCITR_UART_HANDLE.RxInIndex] = (unsigned char)LL_USART_ReceiveData8(HCITR_UART_BASE);

		/* Update the count variables */
		HCITR_UART_HANDLE.RxBytesFree--;
		HCITR_UART_HANDLE.RxInIndex++;
		if(HCITR_UART_HANDLE.RxInIndex == HCITR_INPUT_BUFFER_SIZE){HCITR_UART_HANDLE.RxInIndex = 0;}
	}

	/* If the buffer is full, disable the receive interrupt */
	if(!HCITR_UART_HANDLE.RxBytesFree){LL_USART_DisableIT_RXNE(HCITR_UART_BASE);}
}

/**
  ***************************************************************************************************************************************
  * @brief  Routine for the UART interrupt.
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void HCITR_UART_IRQ_HANDLER(void)
{
	if((LL_USART_IsActiveFlag_RXNE(HCITR_UART_BASE)||LL_USART_IsActiveFlag_ORE(HCITR_UART_BASE))&& \
		LL_USART_IsEnabledIT_RXNE(HCITR_UART_BASE))
   	{HCITR_Rx_Interrupt();}
	else if(LL_USART_IsEnabledIT_TXE(HCITR_UART_BASE)&&LL_USART_IsActiveFlag_TXE(HCITR_UART_BASE))
   	{HCITR_Tx_Interrupt();}
}

/**
  ***************************************************************************************************************************************
  * The following function is responsible for opening the HCI
  * Transport layer that will be used by Bluetopia to send and receive
  * COM (Serial) data.  This function must be successfully issued in
  * order for Bluetopia to function.  This function accepts as its
  * parameter the HCI COM Transport COM Information that is to be used
  * to open the port.  The final two parameters specify the HCI
  * Transport Data Callback and Callback Parameter (respectively) that
  * is to be called when data is received from the UART.  A successful
  * call to this function will return a non-zero, positive value which
  * specifies the HCITransportID that is used with the remaining
  * transport functions in this module.  This function returns a
  * negative return value to signify an error.
  ***************************************************************************************************************************************
  */
int BTPSAPI HCITR_COMOpen(HCI_COMMDriverInformation_t *COMMDriverInformation, HCITR_COMDataCallback_t COMDataCallback, unsigned long CallbackParameter)
{
	int ret_val;

	/* First, make sure that the port is not already open and make sure that valid COMM Driver Information was specified */
	if((!HCITR_UART_HANDLE.InitFlag) && (COMMDriverInformation) && (COMDataCallback))
	{
		/* Initialize the return value for success */
		ret_val = HCITR_TRANSPORT_ID;
		/* Initialize the context structure */
		BTPS_MemInitialize(&HCITR_UART_HANDLE, 0, sizeof(hcitr_uart_context_ts));
		/* Flag that the HCI Transport is open */
		HCITR_UART_HANDLE.InitFlag = 1;
		HCITR_UART_HANDLE.COMDataCallbackFunction = COMDataCallback;
		HCITR_UART_HANDLE.COMDataCallbackParameter = CallbackParameter;
		HCITR_UART_HANDLE.TxBytesFree = HCITR_OUTPUT_BUFFER_SIZE;
		HCITR_UART_HANDLE.RxBytesFree = HCITR_INPUT_BUFFER_SIZE;
		/* Enable the peripheral clocks for the UART and its GPIO */
		HCITR_Enable_UART_Periph_Clock();
		HCITR_RCC_CLK_CMD_ENABLE_GPIO_1(HCITR_RCC_CLK_GPIO_1);
		HCITR_RCC_CLK_CMD_ENABLE_GPIO_2(HCITR_RCC_CLK_GPIO_2);

		/* Configure the GPIO */
		HCITR_Configure_GPIO(HCITR_GPIO_PORT_2, HCITR_P2_RESET_PIN, LL_GPIO_MODE_OUTPUT,LL_GPIO_PULL_NO,HCITR_UART_GPIO_AF);
		HCITR_Set_Reset();
		HCITR_Configure_GPIO(HCITR_GPIO_PORT_1, HCITR_P1_TXD_PIN, LL_GPIO_MODE_ALTERNATE,LL_GPIO_PULL_NO,HCITR_UART_GPIO_AF);
		HCITR_Configure_GPIO(HCITR_GPIO_PORT_1, HCITR_P1_RXD_PIN, LL_GPIO_MODE_ALTERNATE,LL_GPIO_PULL_NO,HCITR_UART_GPIO_AF);
		HCITR_Configure_GPIO(HCITR_GPIO_PORT_1, HCITR_P1_RTS_PIN, LL_GPIO_MODE_ALTERNATE,LL_GPIO_PULL_NO,HCITR_UART_GPIO_AF);
		HCITR_Configure_GPIO(HCITR_GPIO_PORT_2, HCITR_P2_CTS_PIN, LL_GPIO_MODE_ALTERNATE,LL_GPIO_PULL_NO,HCITR_UART_GPIO_AF);

		/* Initialize the UART */
		LL_USART_Init(HCITR_UART_BASE,(LL_USART_InitTypeDef*)&HCITR_UART_CONFIG);
		/* Reconfigure the baud rate to make sure it is as accurate as possible */
		HCITR_Set_Baud_Rate(HCITR_UART_BASE, COMMDriverInformation->BaudRate);

		NVIC_SetPriority(HCITR_UART_IRQ, HCITR_INTERRUPT_PRIORITY);
		NVIC_EnableIRQ(HCITR_UART_IRQ);

		/* Enable the UART */
		LL_USART_Enable(HCITR_UART_BASE);
		/* Polling USART initialisation */
		while((!(LL_USART_IsActiveFlag_TEACK(HCITR_UART_BASE))) || (!(LL_USART_IsActiveFlag_REACK(HCITR_UART_BASE)))){};

		LL_USART_EnableIT_RXNE(HCITR_UART_BASE);

		/* Clear the reset */
		BTPS_Delay(10);
		HCITR_Clear_Reset();
		BTPS_Delay(150);
	}
	else{ret_val = HCITR_ERROR_UNABLE_TO_OPEN_TRANSPORT;}

	return(ret_val);
}

/**
  ***************************************************************************************************************************************
  * The following function is responsible for closing the specific HCI
  * Transport layer that was opened via a successful call to the
  * HCITR_COMOpen() function (specified by the first parameter).
  * Bluetopia makes a call to this function whenever an either
  * Bluetopia is closed, or an error occurs during initialization and
  * the driver has been opened (and ONLY in this case).  Once this
  * function completes, the transport layer that was closed will no
  * longer process received data until the transport layer is
  * Re-Opened by calling the HCITR_COMOpen() function.
  * * NOTE * This function *MUST* close the specified COM Port.  This
  *          module will then call the registered COM Data Callback
  *          function with zero as the data length and NULL as the
  *          data pointer.  This will signify to the HCI Driver that
  *          this module is completely finished with the port and
  *          information and (more importantly) that NO further data
  *          callbacks will be issued.  In other words the very last
  *          data callback that is issued from this module *MUST* be a
  *          data callback specifying zero and NULL for the data
  *          length and data buffer (respectively).
  ***************************************************************************************************************************************
  */
void BTPSAPI HCITR_COMClose(unsigned int HCITransportID)
{
	HCITR_COMDataCallback_t COMDataCallback;

	/* Check to make sure that the specified Transport ID is valid */
	if((HCITransportID == HCITR_TRANSPORT_ID) && (HCITR_UART_HANDLE.InitFlag))
	{
		/* Flag that the HCI Transport is no longer open */
		HCITR_UART_HANDLE.InitFlag = 0;
		NVIC_DisableIRQ(HCITR_UART_IRQ);
		/* Appears to be valid, go ahead and close the port */
		LL_USART_DisableIT_RXNE(HCITR_UART_BASE);
		LL_USART_DisableIT_TXE(HCITR_UART_BASE);
		/* Place the Bluetooth Device in Reset */
		HCITR_Set_Reset();
		/* Disable the peripheral clock for the UART */
		HCITR_Disable_UART_Periph_Clock();
		/* Note the Callback information */
		COMDataCallback = HCITR_UART_HANDLE.COMDataCallbackFunction;
		HCITR_UART_HANDLE.COMDataCallbackFunction = NULL;

		/** All finished, perform the callback to let the upper layer know
		  * that this module will no longer issue data callbacks and is
		  * completely cleaned up.
		  */
		if(COMDataCallback){(*COMDataCallback)(HCITransportID, 0, NULL, HCITR_UART_HANDLE.COMDataCallbackParameter);}

		HCITR_UART_HANDLE.COMDataCallbackParameter = 0;
	}
}

/**
  ***************************************************************************************************************************************
  * The following function is responsible for instructing the
  * specified HCI Transport layer (first parameter) that was opened
  * via a successful call to the HCITR_COMOpen() function to
  * reconfigure itself with the specified information.
  * * NOTE * This function does not close the HCI Transport specified
  *          by HCI Transport ID, it merely reconfigures the
  *          transport.  This means that the HCI Transport specified
  *          by HCI Transport ID is still valid until it is closed via
  *          the HCI_COMClose() function.
  ***************************************************************************************************************************************
  */
void BTPSAPI HCITR_COMReconfigure(unsigned int HCITransportID, HCI_Driver_Reconfigure_Data_t *DriverReconfigureData)
{
	HCI_COMMReconfigureInformation_t *ReconfigureInformation;

	/* Check to make sure that the specified Transport ID is valid */
	if((HCITransportID == HCITR_TRANSPORT_ID) && (HCITR_UART_HANDLE.InitFlag) && (DriverReconfigureData))
	{
		if((DriverReconfigureData->ReconfigureCommand == HCI_COMM_DRIVER_RECONFIGURE_DATA_COMMAND_CHANGE_COMM_PARAMETERS) && (DriverReconfigureData->ReconfigureData))
		{
			ReconfigureInformation = (HCI_COMMReconfigureInformation_t *)(DriverReconfigureData->ReconfigureData);

			/* Check if the baud rate needs to change */
			if(ReconfigureInformation->ReconfigureFlags & (HCI_COMM_RECONFIGURE_INFORMATION_RECONFIGURE_FLAGS_CHANGE_BAUDRATE | HCI_COMM_RECONFIGURE_INFORMATION_RECONFIGURE_FLAGS_CHANGE_PROTOCOL))
			{
				HCITR_Disable_Interrupts();
				HCITR_Set_Baud_Rate(HCITR_UART_BASE, ReconfigureInformation->BaudRate);
				HCITR_Enable_Interrupts();
			}
		}
	}
}

/**
  ***************************************************************************************************************************************
  * The following function is provided to allow a mechanism for
  * modules to force the processing of incoming COM Data.
  * * NOTE * This function is only applicable in device stacks that
  *          are non-threaded.  This function has no effect for device
  *          stacks that are operating in threaded environments.
  ***************************************************************************************************************************************
  */
void BTPSAPI HCITR_COMProcess(unsigned int HCITransportID)
{
	unsigned int MaxLength;
	unsigned int TotalLength;

	#ifdef HCITR_ENABLE_DEBUG_LOGGING
		unsigned int Index;
	#endif

	/* Check to make sure that the specified Transport ID is valid */
	if((HCITransportID == HCITR_TRANSPORT_ID) && (HCITR_UART_HANDLE.InitFlag))
	{
		/* Loop until the receive buffer is empty  */
		while((TotalLength = (HCITR_INPUT_BUFFER_SIZE - HCITR_UART_HANDLE.RxBytesFree)) != 0)
		{
			/** Determine the maximum number of characters that we can send
			  * before we reach the end of the buffer.  We need to process
			  * the smaller of the max characters or the number of
			  * characters that are in the buffer.
			  */
			MaxLength = (HCITR_INPUT_BUFFER_SIZE - HCITR_UART_HANDLE.RxOutIndex);
			if(TotalLength > MaxLength){TotalLength = MaxLength;}

			#ifdef HCITR_ENABLE_DEBUG_LOGGING
				if(HCITR_UART_HANDLE.DebugEnabled)
				{
					HCITR_DEBUG_PRINT(">");
					for(Index=0; Index<TotalLength; Index++){HCITR_DEBUG_PRINT(" %02X", HCITR_UART_HANDLE.RxBuffer[HCITR_UART_HANDLE.RxOutIndex + Index]);}
					HCITR_DEBUG_PRINT("\r\n");
				}
			#endif

			/* Call the upper layer back with the data */
			if(HCITR_UART_HANDLE.COMDataCallbackFunction)
            (*HCITR_UART_HANDLE.COMDataCallbackFunction)(HCITR_TRANSPORT_ID, TotalLength, &(HCITR_UART_HANDLE.RxBuffer[HCITR_UART_HANDLE.RxOutIndex]), HCITR_UART_HANDLE.COMDataCallbackParameter);

			/* Adjust the Out Index and handle any looping */
			HCITR_UART_HANDLE.RxOutIndex += TotalLength;
			if(HCITR_UART_HANDLE.RxOutIndex == HCITR_INPUT_BUFFER_SIZE){HCITR_UART_HANDLE.RxOutIndex = 0;}

			/* Credit the amount that was processed and make sure the receive interrupt is enabled */
			HCITR_Disable_Interrupts();
			HCITR_UART_HANDLE.RxBytesFree += TotalLength;
			LL_USART_EnableIT_RXNE(HCITR_UART_BASE);
			HCITR_Enable_Interrupts();
		}
	}
}

/**
  ***************************************************************************************************************************************
  * The following function is responsible for actually sending data
  * through the opened HCI Transport layer (specified by the first
  * parameter).  Bluetopia uses this function to send formatted HCI
  * packets to the attached Bluetooth Device.  The second parameter to
  * this function specifies the number of bytes pointed to by the
  * third parameter that are to be sent to the Bluetooth Device.  This
  * function returns a zero if the all data was transferred
  * successfully or a negative value if an error occurred.  This
  * function MUST NOT return until all of the data is sent (or an
  * error condition occurs).  Bluetopia WILL NOT attempt to call this
  * function repeatedly if data fails to be delivered.  This function
  * will block until it has either buffered the specified data or sent
  * all of the specified data to the Bluetooth Device.
  * * NOTE * The type of data (Command, ACL, SCO, etc.) is NOT passed
  *          to this function because it is assumed that this
  *          information is contained in the Data Stream being passed
  *          to this function.
  ***************************************************************************************************************************************
  */
int BTPSAPI HCITR_COMWrite(unsigned int HCITransportID, unsigned int Length, unsigned char *Buffer)
{
	int ret_val;
	int Count;
	int BytesFree;

	#ifdef HCITR_ENABLE_DEBUG_LOGGING
		unsigned int Index;
	#endif

	/** Check to make sure that the specified Transport ID is valid and
	  * the output buffer appears to be valid as well
	  */
	if((HCITransportID == HCITR_TRANSPORT_ID) && (HCITR_UART_HANDLE.InitFlag) && (Length) && (Buffer))
	{

		#ifdef HCITR_ENABLE_DEBUG_LOGGING
			if(HCITR_UART_HANDLE.DebugEnabled)
			{
				HCITR_DEBUG_PRINT("<");
				for(Index = 0; Index < Length; Index ++){HCITR_DEBUG_PRINT(" %02X", Buffer[Index]);}
				HCITR_DEBUG_PRINT("\r\n");
			}
		#endif

		/* Process all of the data */
		while(Length)
		{
			/* Wait for space in the transmit buffer */
			while(!HCITR_UART_HANDLE.TxBytesFree){}

			/** The data may have to be copied in 2 phases.  Calculate the
			  * number of character that can be placed in the buffer before
			  * the buffer must be wrapped.
			  */
			BytesFree = HCITR_UART_HANDLE.TxBytesFree;
			Count = Length;
			Count = (BytesFree < Count) ? BytesFree : Count;
			Count = ((HCITR_OUTPUT_BUFFER_SIZE - HCITR_UART_HANDLE.TxInIndex) < Count) ? (HCITR_OUTPUT_BUFFER_SIZE - HCITR_UART_HANDLE.TxInIndex) : Count;

			BTPS_MemCopy(&(HCITR_UART_HANDLE.TxBuffer[HCITR_UART_HANDLE.TxInIndex]), Buffer, Count);

			/* Adjust the index values */
			Buffer += Count;
			Length -= Count;
			HCITR_UART_HANDLE.TxInIndex += Count;
			if(HCITR_UART_HANDLE.TxInIndex == HCITR_OUTPUT_BUFFER_SIZE){HCITR_UART_HANDLE.TxInIndex = 0;}

			/* Update the bytes free and make sure the transmit interrupt is enabled */
			HCITR_Disable_Interrupts();
			HCITR_UART_HANDLE.TxBytesFree -= Count;
			LL_USART_EnableIT_TXE(HCITR_UART_BASE);
			HCITR_Enable_Interrupts();
		}

		ret_val = 0;
	}
	else{ret_val = HCITR_ERROR_WRITING_TO_PORT;}

	return(ret_val);
}

/**
  ***************************************************************************************************************************************
  * The following function is responsible for suspending the HCI COM
  * transport.  It will block until the transmit buffers are empty and
  * all data has been sent then put the transport in a suspended
  * state.  This function will return a value of 0 if the suspend was
  * successful or a negative value if there is an error.
  * * NOTE * An error will occur if the suspend operation was
  *          interrupted by another event, such as data being received
  *          before the transmit buffer was empty.
  * * NOTE * The calling function must lock the Bluetooth stack with a
  *          call to BSC_LockBluetoothStack() before this function is
  *          called.
  * * NOTE * This function should only be called when the baseband
  *          low-power protocol in use has indicated that it is safe
  *          to sleep.  Also, this function must be called
  *          successfully before any clocks necessary for the
  *          transport to operate are disabled.
  ***************************************************************************************************************************************
  */
int BTPSAPI HCITR_COMSuspend(unsigned int HCITransportID)
{
   return 0;
}

/**
  ***************************************************************************************************************************************
  * The following function is used to enable or disable debug logging
  * within HCITRANS.  The function accepts as its parameter a flag
  * which indicates if debugging should be enabled.  It returns zero
  * if successful or a negative value if there was an error.
  ***************************************************************************************************************************************
  */
int BTPSAPI HCITR_EnableDebugLogging(Boolean_t Enable)
{
	int ret_val;

	#ifdef HCITR_ENABLE_DEBUG_LOGGING
		HCITR_UART_HANDLE.DebugEnabled = Enable;
		ret_val = 0;
	#else
		ret_val = HCITR_ERROR_INVALID_PARAMETER;
	#endif

   return(ret_val);
}

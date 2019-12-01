#include "i2c.h"
#include <stdint.h>

/* Flag Definitions
 *
 * I2C_FLAG_DUALF: Dual flag (Slave mode)
 * I2C_FLAG_SMBHOST: SMBus host header (Slave mode)
 * I2C_FLAG_SMBDEFAULT: SMBus default header (Slave mode)
 * I2C_FLAG_GENCALL: General call header flag (Slave mode)
 * I2C_FLAG_TRA: Transmitter/Receiver flag
 * I2C_FLAG_BUSY: Bus busy flag
 * I2C_FLAG_MSL: Master/Slave flag
 * I2C_FLAG_SMBALERT: SMBus Alert flag
 * I2C_FLAG_TIMEOUT: Timeout or Tlow error flag
 * I2C_FLAG_PECERR: PEC error in reception flag
 * I2C_FLAG_OVR: Overrun/Underrun flag (Slave mode)
 * I2C_FLAG_AF: Acknowledge failure flag
 * I2C_FLAG_ARLO: Arbitration lost flag (Master mode)
 * I2C_FLAG_BERR: Bus error flag
 * I2C_FLAG_TXE: Data register empty flag (Transmitter)
 * I2C_FLAG_RXNE: Data register not empty (Receiver) flag
 * I2C_FLAG_STOPF: Stop detection flag (Slave mode)
 * I2C_FLAG_ADD10: 10-bit header sent flag (Master mode)
 * I2C_FLAG_BTF: Byte transfer finished flag
 * I2C_FLAG_ADDR: Address sent flag (Master mode) "ADSL" Address matched flag (Slave mode)"ENDAD"
 * I2C_FLAG_SB: Start bit flag (Master mode)
 *
 * Converting events to meaning:
 *   I2C_EVENT_MASTER_MODE_SELECT (0x30001) == I2C_FLAG_BUSY (0x00020000) | I2C_FLAG_MSL (0x00010000) | I2C_FLAG_SB (0x10000001)
 */

/* Should really introduce PEC (packet error checking) */

/******************************************************************************
* Function Name: ResetI2C
* Description  : This function to reset I2C (need?).
* Arguments    : None
* Return Value : None
******************************************************************************/
void ResetI2C() {
	warning("Resetting I2C bus");		// Function in panic
	/* disable i2c interrupts */
	__I2C1_CLK_ENABLE();
	__GPIOB_CLK_ENABLE();
	
	HAL_I2C_Master_Sequential_Receive_IT
    I2C_ITConfig(I2C1, I2C_IT_ERR | I2C_IT_EVT | I2C_IT_BUF, DISABLE);

    /* turn it off and on again ;) */
    I2C_SoftwareResetCmd(I2C1, ENABLE);
    I2C_SoftwareResetCmd(I2C1, DISABLE);

	/* disable the clock to i2c */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, DISABLE);

	/* Enable the clock to the pin's port */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    /* Setup the clock pin as an output pin */
    GPIO_InitTypeDef gpioClockStructure;
    gpioClockStructure.GPIO_Pin = GPIO_Pin_8;
    gpioClockStructure.GPIO_Mode = GPIO_Mode_OUT;
    gpioClockStructure.GPIO_Speed = GPIO_Speed_100MHz;
    gpioClockStructure.GPIO_OType = GPIO_OType_PP;
    gpioClockStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &gpioClockStructure);

    /* Setup the data pin as an input pin */
    GPIO_InitTypeDef gpioDataStructure;
    gpioDataStructure.GPIO_Pin = GPIO_Pin_9;
    gpioDataStructure.GPIO_Mode = GPIO_Mode_IN;
    gpioDataStructure.GPIO_Speed = GPIO_Speed_100MHz;
    gpioDataStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &gpioDataStructure);

	/*
	 * Sometimes an I2C slave can get stuck holding the data line low. This is the equivalent to a 'busy bus' and prevents the master generating a start condition.
	 * This can be fixed by clocking the line up to nine times and waiting for the SDA to go high after each clock.
	 * When SDA becomes high, you're good to go. Stop clocking, generate a start signal and proceed as normal. We're just always clocking 9 times, if SDA is high it will remain so.
	 * If after nine clocks something is still holding the line low, then you have bigger problems!
	 * See http://forums.parallax.com/showthread.php/112299-I2C-reset-%28part-of-protocol-or-by-device-manufacturer-only-%29
	 */
	WaitAMillisecond();

	for(int i = 0; i < 10; i++) {
		GPIO_SetBits(GPIOB, GPIO_Pin_8);
		WaitAMillisecond();

		GPIO_ResetBits(GPIOB, GPIO_Pin_8);
		WaitAMillisecond();
	}
}

/******************************************************************************
* Function Name: InitialiseI2C
* Description  : This function to initialize i2c.
* Arguments    : None
* Return Value : None
******************************************************************************/
void InitialiseI2C() {
	i2cHasProblem = false;
	i2cInUse = false;

	/* enable the clock to i2c1 */
	__I2C1_CLK_ENABLE();
	/* enable the clock to the pins that will be used */
	__GPIOB_CLK_ENABLE();
	
	/* setting the pins to open drain will ensure masters and slaves can only decide
	 * to pull the line low, or leave the line alone. In the case of leaving the line alone the pull up resistors
	 * will drive the input high.
	 *
	 * This allows for us to have multiple masters, or to allow slaves to stretch when they need more time.
	 * See http://www.i2c-bus.org/how-i2c-hardware-works/.
	 */
	/* ------------ Initialize for GPIO Alternate ---------------- */
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* ------------ Initialize for I2C1 Alternate ---------------- */
	I2C_HandleTypeDef I2C_InitStruct;
	I2C_InitStruct.Instance = I2C1;
	/* note that the clock speed should be able to go up to 400kHz */
	I2C_InitStruct.Init.ClockSpeed = 400000; 		// 100kHz

	/* Duty Cycle is 50% as per standard */
	I2C_InitStruct.Init.DutyCycle = I2C_DUTYCYCLE_2;

	/* Only useful for slave mode (will we do this later? maybe) */
	I2C_InitStruct.Init.OwnAddress1 = 0x00;
	I2C_InitStruct.Mode = HAL_I2C_MODE_MASTER;
	/* 7 bit acknowledge addresses, hmm strange. */
	I2C_InitStruct.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  I2C_InitStruct.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	I2C_InitStruct.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  I2C_InitStruct.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	HAL_I2C_Init(&I2C_InitStruct);
	/*
	 * How to interrupt priorities work?
	 * See https://www.aimagin.com/learn/index.php/STM32_Interrupt_Service_Routine_Priority
 	 */

	/* Configure the I2C event priority */
    NVIC_InitTypeDef NvicEventInterrupt;
	NvicEventInterrupt.NVIC_IRQChannel = I2C1_EV_IRQn;
	NvicEventInterrupt.NVIC_IRQChannelPreemptionPriority = 1;
	NvicEventInterrupt.NVIC_IRQChannelSubPriority = 0;
	NvicEventInterrupt.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NvicEventInterrupt);

    NVIC_InitTypeDef NvicErrorInterrupt;
    NvicErrorInterrupt.NVIC_IRQChannel = I2C1_ER_IRQn;
    NvicErrorInterrupt.NVIC_IRQChannelPreemptionPriority = 1;
    NvicErrorInterrupt.NVIC_IRQChannelSubPriority = 0;
    NvicErrorInterrupt.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NvicErrorInterrupt);

	/* Turn it on! */
	I2C_Cmd(I2C1, ENABLE);

	/* Disable interrupts for the error, event and buffer conditions until we are ready */
    I2C_ITConfig(I2C1, I2C_IT_ERR | I2C_IT_EVT | I2C_IT_BUF, DISABLE);
}

/******************************************************************************
* Function Name: InitialiseI2C
* Description  : This function to send address to communication with object.
* Arguments    : Peripheral, periphRegister, numofbytetoRead
* Return Value : None
******************************************************************************/
void ReadFromAddress(uint8_t peripheral, uint8_t periperalRegister, uint8_t numberOfBytesToRead) {
    I2C_ITConfig(I2C1, I2C_IT_ERR | I2C_IT_EVT | I2C_IT_BUF, ENABLE);

    peripheralAddress = peripheral;
    i2cInUse = true;
    i2cTransmitting = true;
    expectedNumberOfIncoming = numberOfBytesToRead;
	i2cMisunderstoodEvents = 0;

    /* setup the data that we will be writing to */
    expectedNumberOfOutgoing = 1;
    outgoing[0] = periperalRegister;

    I2C_GenerateSTART(I2C1, ENABLE);
}

/******************************************************************************
* Function Name: I2C1_EV_IRQHandler
* Description  : This function to service interrupt handler.
* Arguments    : None
* Return Value : None
******************************************************************************/
void I2C1_EV_IRQHandler(void) {

	uint32_t event = I2C_GetLastEvent(I2C1);

	switch (event) {
		case I2C_EVENT_MASTER_MODE_SELECT:
			I2C_AcknowledgeConfig(I2C1, ENABLE);

			if (i2cTransmitting) {
				I2C_Send7bitAddress(I2C1, peripheralAddress, I2C_Direction_Transmitter);
			} else {
				I2C_Send7bitAddress(I2C1, peripheralAddress, I2C_Direction_Receiver);
			}

			break;

		case I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED:
			incomingIndex = 0;

			break;

		case I2C_EVENT_MASTER_BYTE_RECEIVED:
		case I2C_EVENT_MASTER_BYTE_RECEIVED_AND_TRANSFER_FINISHED:

			/* double buffering means that NAK should be prepared two bytes before the end */
			if (incomingIndex + 2 == expectedNumberOfIncoming) {
				I2C_AcknowledgeConfig(I2C1, DISABLE);
			}

			if (incomingIndex < expectedNumberOfIncoming) {
				incoming[incomingIndex++] = I2C_ReceiveData(I2C1);
			}

			if (incomingIndex == expectedNumberOfIncoming) {
				I2C_GenerateSTOP(I2C1, ENABLE);
				i2cInUse = false;
			    I2C_ITConfig(I2C1, I2C_IT_ERR | I2C_IT_EVT | I2C_IT_BUF, DISABLE);
			}

	        break;

		case I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED:
			outgoingIndex = 0;
			I2C_SendData(I2C1, outgoing[outgoingIndex++]);

			break;

		case I2C_EVENT_MASTER_BYTE_TRANSMITTED:
			if (outgoingIndex < expectedNumberOfOutgoing) {
				I2C_SendData(I2C1, outgoing[outgoingIndex++]);
			} else if (outgoingIndex == expectedNumberOfOutgoing && i2cTransmitting) {
				/* transmitting is complete, time to read the response */
				i2cTransmitting = false;
				I2C_GenerateSTART(I2C1, ENABLE);
			}

	        break;

		default:
			i2cMisunderstoodEvents++;

			if (i2cMisunderstoodEvents > 1000) {
				warningWithValue("i2c handling unknown events, latest is", event);
			    I2C_ITConfig(I2C1, I2C_IT_ERR | I2C_IT_EVT | I2C_IT_BUF, DISABLE);
				i2cHasProblem = true;
			}

	        break;
	}
}

/******************************************************************************
* Function Name: I2C1_ER_IRQHandler
* Description  : This function to service error interrupt handler.
* Arguments    : None
* Return Value : None
******************************************************************************/
void I2C1_ER_IRQHandler(void) {
    /* no more interrupts! */
	I2C_ITConfig(I2C1, I2C_IT_ERR | I2C_IT_EVT | I2C_IT_BUF, DISABLE);
	i2cHasProblem = true;

	if (I2C_GetITStatus(I2C1, I2C_IT_AF)) {
		warning("i2c1 acknowledge failure");
		I2C_ClearITPendingBit(I2C1, I2C_IT_AF);

	} else if (I2C_GetITStatus(I2C1, I2C_IT_BERR)) {
		warning("i2c1 bus error");
		I2C_ClearITPendingBit(I2C1, I2C_IT_BERR);

	} else if (I2C_GetITStatus(I2C1, I2C_IT_ARLO)) {
		warning("i2c1 arbitration loss");
		I2C_ClearITPendingBit(I2C1, I2C_IT_ARLO);

	} else if (I2C_GetITStatus(I2C1, I2C_IT_OVR)) {
		warning("i2c1 overrun/underrun");
		I2C_ClearITPendingBit(I2C1, I2C_IT_OVR);

	} else if (I2C_GetITStatus(I2C1, I2C_IT_TIMEOUT)) {
		warning("i2c1 timeout/tlow");
		I2C_ClearITPendingBit(I2C1, I2C_IT_TIMEOUT);

	} else if (I2C_GetITStatus(I2C1, I2C_IT_PECERR)) {
		warning("i2c1 pec error");
		I2C_ClearITPendingBit(I2C1, I2C_IT_PECERR);

	} else {
		warning("i2c1 error unknown");
	}
}

/******************************************************************************
* Function Name: WaitForEvent
* Description  : This function to wait event with timeout.
* Arguments    : None
* Return Value : None
******************************************************************************/
void WaitForEvent(I2C_TypeDef* I2Cx, uint32_t event) {
	int32_t attempts = 0;
	int32_t maxAttempts = 5000;

	while(!I2C_CheckEvent(I2Cx, event) && attempts < maxAttempts) {
		attempts++;
	}

	if (attempts == maxAttempts) {
		panicWithValue("gave up waiting for I2C event", event);
		i2cHasProblem = true;
	}
}

/******************************************************************************
* Function Name: WaitUntilBusIsFree
* Description  : This function to wait to bus free.
* Arguments    : None
* Return Value : None
******************************************************************************/
void WaitUntilBusIsFree() {
	/* wait until the line is not busy */
	WaitForEvent(I2C1, I2C_FLAG_BUSY);
}

/******************************************************************************
* Function Name: SendStart
* Description  : This function to send start signal.
* Arguments    : None
* Return Value : None
******************************************************************************/
void SendStart() {
	i2cHasProblem = false;

	/* Begin comms! */
	I2C_GenerateSTART(I2C1, ENABLE);

	/* Start condition has been correctly released on the bus (will fail if another device attempts to communicate and bus not free) */
	/* hmmm. I don't like while loops that never stop. */
	WaitForEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT);
}

/******************************************************************************
* Function Name: SendAddress
* Description  : This function to send address want communicate.
* Arguments    : None
* Return Value : None
******************************************************************************/
/* Address should be the nominal write address of the peripheral. */
void SendAddress(uint8_t address, uint8_t direction) {
	if (i2cHasProblem) return;

	/* send the address */
	I2C_Send7bitAddress(I2C1, address, direction);

	/* Wait for peripheral to acknowledge (own up) to the sent address */
	if (direction == I2C_Direction_Transmitter) {
		WaitForEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);
	} else {
		WaitForEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED);
	}
}

/******************************************************************************
* Function Name: SendData
* Description  : This function to send data to peripheral.
* Arguments    : None
* Return Value : None
******************************************************************************/
void SendData(uint8_t data) {
	if (i2cHasProblem) return;

	/* make sure that the bus is not transmitting */
	WaitForEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTING);

	/* send the data */
	I2C_SendData(I2C1, data);

	/* wait for confirmation */
	/* Testing on this event over I2C_EVENT_MASTER_BYTE_TRANSMITTING is more reliable, and slower. */
	WaitForEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED);
}

/******************************************************************************
* Function Name: SendStop
* Description  : This function to send stop signal.
* Arguments    : None
* Return Value : None
******************************************************************************/
void SendStop() {
	if (i2cHasProblem) return;

	/* Communication is over for now */
	I2C_GenerateSTOP(I2C1, ENABLE);
}

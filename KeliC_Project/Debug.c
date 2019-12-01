#include "Board_LED.h"             				     // ::Board Support:LED
#include "stm32f4xx_hal.h" 				             // Keil::Device:STM32Cube HAL:Common
#include "cmsis_os.h"                          // CMSIS RTOS header file
#include <string.h>
#include <stdio.h>

/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/
/* ------------------------ Variable & Decleare -----------------------------*/
/* Thread */
void Debug_Thread (void const *argument);                             // thread function
osThreadId tid_Debug_Thread;                                          // thread id
osThreadDef (Debug_Thread, osPriorityNormal, 1, 0);                   // thread object

/* ------ Serial output USART ------ */
static USART_HandleTypeDef s_UARTHandle;
void InitialiseSerialOutput(void);

/* ------ Ring buffer to manager data serial ----- */
// Variable size of buffer ring
#define RING_BUFFER_SIZE 	1024
#define ERR_DATA_EMPTY		1

// Struct to store data
typedef struct RingBuffer
{
	uint8_t buffer[RING_BUFFER_SIZE];
	int head;
	int tail;
	int count;
} RingBuffer;

void  InitialiseRingBuffer(RingBuffer *_this);				// InitialiseRingBuffer
int RingBufferIsEmpty(RingBuffer *_this);					// Check buffer empty
int RingBufferIsFull(RingBuffer *_this);					// Check buffer full
uint8_t RingBufferPop(RingBuffer *_this);					// Pop data which don't need anymore
void RingBufferPut(RingBuffer *_this, uint16_t value);		// Put new data

/* ------ Analytics using send laptop ------ */
#define	ANALYTICS_CHARACTERS_TO_SEND_PER_FLUSH 		5
RingBuffer metricsRingBuffer;
void InitialiseAnalytics(void);
void RecordWarningMessage(char *message);
void RecordIntegerMetric(uint8_t type, uint8_t loopReference, uint32_t value);
void RecordFloatMetric(uint8_t type, uint8_t loopReference, float value);
void FlushAllMetrics(void);


/* ------------------------- Function of main ------------------------------*/
int Init_Debug_Thread (void) {
	
	InitialiseAnalytics();
	
  tid_Debug_Thread = osThreadCreate (osThread(Debug_Thread), NULL);
  if (!tid_Debug_Thread) return(-1);
  
  return(0);
}

void Debug_Thread (void const *argument) {

  while (1) {
		/* Monitor some sensor value */
		RecordWarningMessage("\r\n");
		RecordWarningMessage("");
		FlushAllMetrics();
		
		osDelay(1000);
    osThreadYield ();                                           // suspend thread
  }
}


/* -------------------------------- API --------------------------------------*/

/******************************************************************************
* Function Name: InitialiseSerialOutput
* Description  : This function initialize for serial output USART 4 (PC10/11).
* Arguments    : None
* Return Value : None
******************************************************************************/
void InitialiseSerialOutput(void){
	__USART4_CLK_ENABLE();
	__GPIOC_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Pin = GPIO_PIN_10;
	GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStructure.Alternate = GPIO_AF8_UART4;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

	s_UARTHandle.Instance        = UART4;
	s_UARTHandle.Init.BaudRate   = 115200;
	s_UARTHandle.Init.WordLength = USART_WORDLENGTH_8B;
	s_UARTHandle.Init.StopBits   = USART_STOPBITS_1;
	s_UARTHandle.Init.Parity     = USART_PARITY_NONE;
	s_UARTHandle.Init.Mode       = USART_MODE_TX;
	s_UARTHandle.State = HAL_USART_STATE_RESET;
	if ( HAL_USART_Init (&s_UARTHandle) != HAL_OK)
		asm("bkpt 255");
}

/******************************************************************************
* Function Name: InitialiseRingBuffer
* Description  : This function initialize ring buffer.
* Arguments    : Struct data ring buffer.
* Return Value : None
******************************************************************************/
void InitialiseRingBuffer(RingBuffer *_this) {
	/*
	The following clears:
		-> buf
		-> head
		-> tail
		-> count
		and sets head = tail
	*/
	memset(_this, 0, sizeof (*_this));	// Clear struct data this
}

/******************************************************************************
* Function Name: modulo_inc
* Description  : This function increase value.
* Arguments    : Value and max value.
* Return Value : New value.
******************************************************************************/
unsigned int modulo_inc (const unsigned int value, const unsigned int modulus)
{
    unsigned int my_value = value + 1;	// Increase value
    if (my_value >= modulus)			// Check value and modulus
    {
      my_value  = 0;					// Reset value
    }
    return my_value;
}

/******************************************************************************
* Function Name: modulo_dec
* Description  : This function decrease value.
* Arguments    : Value and min value.
* Return Value : New value or return error (max value - 1)
******************************************************************************/
unsigned int modulo_dec (const unsigned int value, const unsigned int modulus)
{
    return (0 == value) ? (modulus - 1) : (value - 1);		// Check value to know decrease value or modulus
}

/******************************************************************************
* Function Name: RingBufferIsEmpty
* Description  : This function check buffer is empty.
* Arguments    : Struct buffer ring data.
* Return Value : True (Empty), False (Not)
******************************************************************************/
int RingBufferIsEmpty(RingBuffer *_this) {
	return 0 == _this->count;					// Return result of compare
}

/******************************************************************************
* Function Name: RingBufferIsFull
* Description  : This function check buffer is full.
* Arguments    : Struct buffer ring data.
* Return Value : True (Full), False (Not).
******************************************************************************/
int RingBufferIsFull(RingBuffer *_this) {
	return _this->count >= RING_BUFFER_SIZE;	// Return check count of data in ring buffer
}

/******************************************************************************
* Function Name: RingBufferPop
* Description  : This function pop data at tail.
* Arguments    : Struct buffer ring data.
* Return Value : None
******************************************************************************/
uint8_t RingBufferPop(RingBuffer *_this) {
	// Check data is right or not ?
	if (_this->count <= 0) {
		return ERR_DATA_EMPTY;
	}

	uint8_t value = _this->buffer[_this->tail];				// Get tail buffer
	_this->tail = modulo_inc(_this->tail, RING_BUFFER_SIZE);	// Increase tail to next data can insert
	--_this->count;												// Decrease count
	return value;
}

/******************************************************************************
* Function Name: RingBufferPut
* Description  : This function put new data at head.
* Arguments    : Struct buffer ring data.
* Return Value : None
******************************************************************************/
void RingBufferPut(RingBuffer *_this, uint16_t value) {
	// Check count with max size buffer
	if (_this->count < RING_BUFFER_SIZE) {
	  _this->buffer[_this->head] = value;						// Add new value
	  _this->head = modulo_inc(_this->head, RING_BUFFER_SIZE);	// Increase head
	  ++_this->count;											// Increase count
	}
}

void InitialiseAnalytics() {
	InitialiseRingBuffer(&metricsRingBuffer);
	InitialiseSerialOutput();
}

void WriteStringToBuffer(char* value) {
	char* letter = value;

	while(*letter) {
		RingBufferPut(&metricsRingBuffer, *letter++);
	}
}

void WriteCharacterToBuffer(uint16_t value) {
	RingBufferPut(&metricsRingBuffer, value);
}

void RecordWarningMessage(char *message) {
	WriteStringToBuffer("info.warn:W:");
	WriteStringToBuffer(message);
}

void RecordPanicMessage(char *message) {
	WriteStringToBuffer("info.err-:E:");
	WriteStringToBuffer(message);
}

void RecordIntegerMetric(uint8_t type, uint8_t loopReference, uint32_t value) {
	WriteCharacterToBuffer('S');
	WriteCharacterToBuffer(type);
	WriteCharacterToBuffer(loopReference);

	uint8_t valueHighest = (value >> 24) & 0xFF;
	uint8_t valueHigh = (value >> 16) & 0xFF;
	uint8_t valueLow = (value >> 8) & 0xFF;
	uint8_t valueLowest = (value >> 0) & 0xFF;

	WriteCharacterToBuffer(valueHighest);
	WriteCharacterToBuffer(valueHigh);
	WriteCharacterToBuffer(valueLow);
	WriteCharacterToBuffer(valueLowest);
}

void RecordFloatMetric(uint8_t type, uint8_t loopReference, float value) {
	WriteCharacterToBuffer('S');
	WriteCharacterToBuffer(type);
	WriteCharacterToBuffer(loopReference);

	int32_t roundedValue = (value * 1000000);
	uint8_t valueHighest = (roundedValue >> 24) & 0xFF;
	uint8_t valueHigh = (roundedValue >> 16) & 0xFF;
	uint8_t valueLow = (roundedValue >> 8) & 0xFF;
	uint8_t valueLowest = (roundedValue >> 0) & 0xFF;

	WriteCharacterToBuffer(valueHighest);
	WriteCharacterToBuffer(valueHigh);
	WriteCharacterToBuffer(valueLow);
	WriteCharacterToBuffer(valueLowest);
}

void FlushMetrics() {
	uint8_t metricsFlushed = 0;
	char buffer[1];
	
	while(metricsFlushed < ANALYTICS_CHARACTERS_TO_SEND_PER_FLUSH && metricsRingBuffer.count > 0) {
		HAL_USART_Transmit(&s_UARTHandle, (uint8_t *)buffer, sprintf(buffer, "%c", RingBufferPop(&metricsRingBuffer)), 10);
		metricsFlushed++;
	}
}

void FlushAllMetrics() {
	char buffer[1];
	while(metricsRingBuffer.count > 0) {
		HAL_USART_Transmit(&s_UARTHandle, (uint8_t *)buffer, sprintf(buffer, "%c", RingBufferPop(&metricsRingBuffer)), 10);
		osDelay(10);
	}
}

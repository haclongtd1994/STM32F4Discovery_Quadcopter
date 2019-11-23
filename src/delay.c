//******************************************************************************

// https://my.st.com/public/STe2ecommunities/mcu/Lists/cortex_mx_stm32/Flat.aspx?RootFolder=https://my.st.com/public/STe2ecommunities/mcu/Lists/cortex_mx_stm32/compensating%20latencies%20on%20STM32F4%20interrupts&FolderCTID=0x01200200770978C69A1141439FE559EB459D7580009C4E14902C3CDE46A77F0FFD06506F5B&currentviews=2116

// Register to control watchdog timer
volatile unsigned int *DWT_CYCCNT   = (volatile unsigned int *)0xE0001004; 
// Count cycle
volatile unsigned int *DWT_CONTROL  = (volatile unsigned int *)0xE0001000;
// Debug exception and monitor control register
volatile unsigned int *SCB_DEMCR        = (volatile unsigned int *)0xE000EDFC;

//******************************************************************************

#include <delay.h>

/******************************************************************************
* Function Name: EnableTiming
* Description  : This function to enable time of watchdog timer.
* Arguments    : None
* Return Value : None
******************************************************************************/
void EnableTiming(void)
{
    static int enabled = 0;						// Variable to store previous data status of timer
	// Check if not enable will enable to start timer
    if (!enabled)
    {
        *SCB_DEMCR = *SCB_DEMCR | 0x01000000;	// Stop check error
        *DWT_CYCCNT = 0; 						// Reset the counter
        *DWT_CONTROL = *DWT_CONTROL | 1 ; 		// enable the counter

        enabled = 1;
    }
}

/******************************************************************************
* Function Name: TimingDelay
* Description  : This function to delay with cycle of chip.
* Arguments    : Cycle want delay
* Return Value : None
******************************************************************************/
void TimingDelay(unsigned int tick)
{
    unsigned int start, current;

    start = *DWT_CYCCNT;						// Read first data of count timer

    do
    {
        current = *DWT_CYCCNT;					//Read current value 
    } while((current - start) < tick);			//Loop to true
}

/******************************************************************************
* Function Name: WaitASecond
* Description  : This function to delay 1 second.
* Arguments    : None
* Return Value : None
******************************************************************************/
void WaitASecond() {
	TimingDelay(160000000);
}

/******************************************************************************
* Function Name: EnableTiming
* Description  : This function to delay 1 ms.
* Arguments    : None
* Return Value : None
******************************************************************************/
void WaitAMillisecond() {
	TimingDelay(160000);
}

/******************************************************************************
* Function Name: EnableTiming
* Description  : This function delay miliseconds.
* Arguments    : None
* Return Value : None
******************************************************************************/
void WaitAFewMillis(int16_t millis) {
	TimingDelay(160000000 / 1000 * millis);
}

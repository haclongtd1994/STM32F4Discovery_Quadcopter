#include <systick.h>
#include <stm32f4xx_gpio.h>
#include <stm32f4xx_it.h>

/******************************************************************************
* Function Name: InitialiseSysTick
* Description  : This function initialize systick timer.
* Arguments    : None
* Return Value : None
******************************************************************************/
void InitialiseSysTick() {
	secondsElapsed = 0;
	intermediateMillis = 0;

	// init the system ticker to trigger an interrupt every millisecond
	// this will call the SysTick_Handler
	// note that milliseconds are only used because calling it every second (ideal) fails.
	// This is presumably due to the ideal number of ticks being too many to store in a register.
	//Turn on systick timer and interrupt handler for systick in file core_cm4.h
  if (SysTick_Config(SystemCoreClock / 1000)) {
    HardFault_Handler();					//Call function in stm32f4xx_it.h to monitor to user
  }
}

// note that the SysTick handler is already defined in startup_stm32f40xx.s
// It is defined with a weak reference, so that anyone other definition will be used over the default
/******************************************************************************
* Function Name: SysTick_Handler
* Description  : This function service interrupt of systick timer.
* Arguments    : None
* Return Value : None
******************************************************************************/
void SysTick_Handler(void)
{
    intermediateMillis++;					// Increase milisecond
    if (intermediateMillis % 1000 == 0) {
        secondsElapsed++;					// Increase second
    }

    // After 49 days, what to do? Kaboom! Until we require something better.
	// Check if milisecond max is 0xffffffff (UINT_LEAST32_MAX)
    if (intermediateMillis == UINT_LEAST32_MAX) {
        HardFault_Handler();				//Call function in stm32f4xx_it.h to monitor to user
    }
}

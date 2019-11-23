#include <pwm_input.h>
#include <stm32f4xx_gpio.h>
#include <stm32f4xx_it.h>
#include <math.h>

/* Todo: This probably should account for startup, and for when the ICValues are 0 (when will this happen?) */
/* Todo: The clock speed stuff should be moved out into it's own file / class / struct */

RCC_ClocksTypeDef RCC_Clocks;

/******************************************************************************
* Function Name: MeasurePWMInput
* Description  : This function to measure pwm.
* Arguments    : Timer, GPIO, Pin, Pinsource
* Return Value : Structure of PWMInput
******************************************************************************/
struct PWMInput* MeasurePWMInput(TIM_TypeDef *TIMx, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint8_t GPIO_PinSource) {

	struct PWMInput* pwmInput;
	uint32_t ahbPeripheralPort;
	uint8_t gpioAlternateFunction;
	uint8_t nvicInterruptChannel;

	/* Note all of these timers are on AHB1, whichs means they run at 80,000,000 Hz. */
	if(TIMx == TIM4) {
		pwmInput = &pwmInputTimer4;
		ahbPeripheralPort = RCC_AHB1Periph_GPIOB;
		gpioAlternateFunction = GPIO_AF_TIM4;
		nvicInterruptChannel = TIM4_IRQn;

		/* AHB1 Peripherals run at half the HCLK Speed */
	    pwmInput->hclckDivisor = 2.0f;

	    /* Enable the timer clock */
	    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	} else if (TIMx == TIM5) {
		pwmInput = &pwmInputTimer5;
		ahbPeripheralPort = RCC_AHB1Periph_GPIOA;
		gpioAlternateFunction = GPIO_AF_TIM5;
		nvicInterruptChannel = TIM5_IRQn;

		/* AHB1 Peripherals run at half the HCLK Speed */
	    pwmInput->hclckDivisor = 2.0f;

	    /* Enable the timer clock */
	    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

	} else if (TIMx == TIM9) {
		pwmInput = &pwmInputTimer9;
		ahbPeripheralPort = RCC_AHB1Periph_GPIOE;
		gpioAlternateFunction = GPIO_AF_TIM9;
		nvicInterruptChannel = TIM1_BRK_TIM9_IRQn;

		/* AHB2 Peripherals run at HCLK Speed */
	    pwmInput->hclckDivisor = 1.0f;

	    /* Enable the timer clock */
	    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM9, ENABLE);

	} else if (TIMx == TIM12) {
		pwmInput = &pwmInputTimer12;
		ahbPeripheralPort = RCC_AHB1Periph_GPIOB;
		gpioAlternateFunction = GPIO_AF_TIM12;
		nvicInterruptChannel = TIM8_BRK_TIM12_IRQn;

		/* AHB1 Peripherals run at half the HCLK Speed */
	    pwmInput->hclckDivisor = 2.0f;

	    /* Enable the timer clock */
	    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM12, ENABLE);
	} else {
		/* Some timers just can't handle it. */
	    HardFault_Handler();
	}


    /* Initialise duty cycle and freq to zero, surely there is a better place to do this? */
	pwmInput->dutyCycle = 0.0;
	pwmInput->frequency = 0.0;

	/* Work out the system / bus / timer clock speed */
    RCC_GetClocksFreq(&RCC_Clocks);

    /* Enable the clock to the GPIO Port */
    RCC_AHB1PeriphClockCmd(ahbPeripheralPort, ENABLE);

    /* Turn on PB06, it will be connected to Timer 4, Channel 1.
     * Timer Channel 2 will also be used, I believe this renders pin PB7 unusable.
     */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP ;
    GPIO_Init(GPIOx, &GPIO_InitStructure);

    /* Connect TIM pin to AF2 */
    GPIO_PinAFConfig(GPIOx, GPIO_PinSource, gpioAlternateFunction);

    /* init the timer:
     * It doesn't really matter what prescaler we use, because the duty cycle is calculated as a percentage.
     *    (as long as the prescalar ensures that the counter will not overflow)
     * The maximum (16bit) period should never be reached, as we will reset the counter before we get there.
     */
	TIM_TimeBaseInitTypeDef timerInitStructure;
	timerInitStructure.TIM_Prescaler = 1000;
	timerInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	timerInitStructure.TIM_Period = 65535;
	timerInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	timerInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIMx, &timerInitStructure);

	/* Enable the Timer counter */
	TIM_Cmd(TIMx, ENABLE);

	/* We're attempting to not have a prescalar, that is, not divide the incoming signal.
	 * I wonder this prescalar differs from the above prescalar.
	 * Channel 1 is configured to capture on the rising edge.
	 * */
	TIM_ICInitTypeDef TIM_ICInitStructure1;
	TIM_ICInitStructure1.TIM_Channel = TIM_Channel_1;
	TIM_ICInitStructure1.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure1.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure1.TIM_ICPrescaler = 0;
	TIM_ICInitStructure1.TIM_ICFilter = 0;
	TIM_ICInit(TIMx, &TIM_ICInitStructure1);

	/*
	 * Channel 2 is configured to capture on the falling edge.
	 * */
	TIM_ICInitTypeDef TIM_ICInitStructure2;
	TIM_ICInitStructure2.TIM_Channel = TIM_Channel_2;
	TIM_ICInitStructure2.TIM_ICPolarity = TIM_ICPolarity_Falling;
	TIM_ICInitStructure2.TIM_ICSelection = TIM_ICSelection_IndirectTI;
	TIM_ICInitStructure2.TIM_ICPrescaler = 0;
	TIM_ICInitStructure2.TIM_ICFilter = 0;
	TIM_ICInit(TIMx, &TIM_ICInitStructure2);

	/* Ensure that Channel two is set up as a slave, and that it resets the counters on a falling edge */
	TIM_SelectInputTrigger(TIMx, TIM_TS_TI1FP1);
	TIM_SelectSlaveMode(TIMx, TIM_SlaveMode_Reset);
	TIM_SelectMasterSlaveMode(TIMx, TIM_MasterSlaveMode_Enable);

	/* Enable the interrupt that gets fired when the timer counter hits the period */
	TIM_ITConfig(TIMx, TIM_IT_Update, ENABLE);

	/* Enable the Timer interrupts */
	NVIC_InitTypeDef nvicStructure;
	nvicStructure.NVIC_IRQChannel = nvicInterruptChannel;
	nvicStructure.NVIC_IRQChannelPreemptionPriority = 0;
	nvicStructure.NVIC_IRQChannelSubPriority = 1;
	nvicStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvicStructure);

	return pwmInput;
}

/******************************************************************************
* Function Name: TIM4_IRQHandler
* Description  : This function to service interrupt handler of timer 4.
* Arguments    : None
* Return Value : None
******************************************************************************/
/* Timer 4 interrupt handler */
void TIM4_IRQHandler()
{
	/* makes sure the interrupt status is not reset (and therefore SET?) */
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET) {
    	/* ensure that the timer doesn't get triggered again */
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

        uint32_t IC1Value = TIM_GetCapture1(TIM4);
    	uint32_t IC2Value = TIM_GetCapture2(TIM4);

    	/* As a percentage */
    	float updatedDutyCycle = (IC2Value * 100.0f) / IC1Value;

    	/* HCLK is the Advanced High Speed Bus (AHB) Clock Speed, which is a
           factor of the System Clock (one, at the moment, hence is the same) */
    	float updatedFrequency = (RCC_Clocks.HCLK_Frequency) / pwmInputTimer4.hclckDivisor / (IC1Value * 1000);

    	/* eliminate noise that is more than twice the previous duty cycle */
    	if (isnan(updatedDutyCycle)
    		|| isnan(updatedFrequency)
    		|| updatedDutyCycle <= 0.0
    		|| updatedFrequency <= 0.0
    		|| IC1Value == IC2Value) {
    		return;
    	}

    	pwmInputTimer4.dutyCycle = updatedDutyCycle;
		pwmInputTimer4.frequency = updatedFrequency;
    }
}

/******************************************************************************
* Function Name: TIM5_IRQHandler
* Description  : This function to service interrupt handler of timer 5.
* Arguments    : None
* Return Value : None
******************************************************************************/
void TIM5_IRQHandler() {
	/* makes sure the interrupt status is not reset (and therefore SET?) */
    if (TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET) {
    	/* ensure that the timer doesn't get triggered again */
        TIM_ClearITPendingBit(TIM5, TIM_IT_Update);

        uint32_t IC1Value = TIM_GetCapture1(TIM5);
    	uint32_t IC2Value = TIM_GetCapture2(TIM5);

    	/* As a percentage */
    	float updatedDutyCycle = (IC2Value * 100.0f) / IC1Value;

    	/* HCLK is the Advanced High Speed Bus (AHB) Clock Speed, which is a
           factor of the System Clock (one, at the moment, hence is the same) */
    	float updatedFrequency = (RCC_Clocks.HCLK_Frequency) / pwmInputTimer5.hclckDivisor / (IC1Value * 1000);

    	/* eliminate noise that is more than twice the previous duty cycle */
    	if (isnan(updatedDutyCycle)
    		|| isnan(updatedFrequency)
    		|| updatedDutyCycle <= 0.0
    		|| updatedFrequency <= 0.0
    		|| IC1Value == IC2Value) {
    		return;
    	}

		pwmInputTimer5.dutyCycle = updatedDutyCycle;
		pwmInputTimer5.frequency = updatedFrequency;
    }
}

/******************************************************************************
* Function Name: TIM1_BRK_TIM9_IRQHandler
* Description  : This function to service interrupt handler of timer 1 break timer 9.
* Arguments    : None
* Return Value : None
******************************************************************************/
void TIM1_BRK_TIM9_IRQHandler() {
	/* makes sure the interrupt status is not reset (and therefore SET?) */
    if (TIM_GetITStatus(TIM9, TIM_IT_Update) != RESET) {
    	/* ensure that the timer doesn't get triggered again */
        TIM_ClearITPendingBit(TIM9, TIM_IT_Update);

        uint32_t IC1Value = TIM_GetCapture1(TIM9);
    	uint32_t IC2Value = TIM_GetCapture2(TIM9);

    	/* As a percentage */
    	float updatedDutyCycle = (IC2Value * 100.0f) / IC1Value;

    	/* HCLK is the Advanced High Speed Bus (AHB) Clock Speed, which is a
           factor of the System Clock (one, at the moment, hence is the same) */
    	float updatedFrequency = (RCC_Clocks.HCLK_Frequency) / pwmInputTimer9.hclckDivisor / (IC1Value * 1000);

    	/* eliminate noise that is more than twice the previous duty cycle */
    	if (isnan(updatedDutyCycle)
    		|| isnan(updatedFrequency)
    		|| updatedDutyCycle <= 0.0
    		|| updatedFrequency <= 0.0
    		|| IC1Value == IC2Value) {
    		return;
    	}

    	pwmInputTimer9.dutyCycle = updatedDutyCycle;
		pwmInputTimer9.frequency = updatedFrequency;
    }
}

/******************************************************************************
* Function Name: TIM8_BRK_TIM12_IRQHandler
* Description  : This function to service interrupt handler of timer 8 break timer 12.
* Arguments    : None
* Return Value : None
******************************************************************************/
void TIM8_BRK_TIM12_IRQHandler() {
	/* makes sure the interrupt status is not reset (and therefore SET?) */
    if (TIM_GetITStatus(TIM12, TIM_IT_Update) != RESET) {
    	/* ensure that the timer doesn't get triggered again */
        TIM_ClearITPendingBit(TIM12, TIM_IT_Update);

        uint32_t IC1Value = TIM_GetCapture1(TIM12);
    	uint32_t IC2Value = TIM_GetCapture2(TIM12);

    	/* As a percentage */
    	float updatedDutyCycle = (IC2Value * 100.0f) / IC1Value;

    	/* HCLK is the Advanced High Speed Bus (AHB) Clock Speed, which is a
           factor of the System Clock (one, at the moment, hence is the same) */
    	float updatedFrequency = (RCC_Clocks.HCLK_Frequency) / pwmInputTimer12.hclckDivisor / (IC1Value * 1000);

    	/* eliminate noise that is more than twice the previous duty cycle */
    	if (isnan(updatedDutyCycle)
    		|| isnan(updatedFrequency)
    		|| updatedDutyCycle <= 0.0
    		|| updatedFrequency <= 0.0
    		|| IC1Value == IC2Value) {
    		return;
    	}

    	pwmInputTimer12.dutyCycle = updatedDutyCycle;
    	pwmInputTimer12.frequency = updatedFrequency;
    }
}

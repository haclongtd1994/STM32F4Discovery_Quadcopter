#include <remote_controls.h>

/******************************************************************************
* Function Name: InitialiseRemoteControls
* Description  : This function initialize remote comtrols.
* Arguments    : None
* Return Value : None
******************************************************************************/
void InitialiseRemoteControls() {
  /* throttle: all together now! power (collective pitch?)
   * Channel 1 on the RC receiver
   */
  throttle = MeasurePWMInput(TIM4, GPIOB, GPIO_Pin_6, GPIO_PinSource6); 	// channel 2 - PB.07

  /* rudder: spin to the left or right on a flat plane
   * Channel 4 on the RC receiver
   */
  remotePidProportional = MeasurePWMInput(TIM5, GPIOA, GPIO_Pin_0, GPIO_PinSource0); 		// channel 2 - PA.01

  /* airleron: fly sideways left or right
   * Channel 2 on the RC receiver
   */
  remotePidIntegral = MeasurePWMInput(TIM9, GPIOE, GPIO_Pin_5, GPIO_PinSource5);	// channel 2 - PE.05

  /* elevator: fly forwards or backwards
   * Channel 3 on the RC receiver
   */
  resetAngularPosition = MeasurePWMInput(TIM12, GPIOB, GPIO_Pin_14, GPIO_PinSource14); // channel 2 - PB.15
}

/******************************************************************************
* Function Name: CalculatePercentageOfMaximum
* Description  : This function calculate percentage.
* Arguments    : Dutycycle, frequency
* Return Value : Float
******************************************************************************/
/* note this makes assumptions about the minimum and maximum of duty cycles */
float CalculatePercentageOfMaximum(float dutyCycle, float frequency) {
	/* how can I tell if something is NAN? */

	/* A duty cycle of 2ms is on for 11% of the time @ 55Hz (18.181818ms period) */
	float maximum = 2.0 / (1000 / frequency) * 100;
	/* A duty cycle of 1ms is on for 5.5% of the time @ 55Hz (18.181818ms period) */
	float minimum = 1.0 / (1000 / frequency) * 100;

	float percentageOn = (dutyCycle - (maximum - minimum)) / minimum * 100.0;

	if (percentageOn > 100.0) {
		return 100.0;
	}

	if (percentageOn < 0.0) {
		return 0.0;
	}

	return percentageOn;
}

/******************************************************************************
* Function Name: ReadRemoteThrottle
* Description  : This function calculate percentage of throttle.
* Arguments    : None
* Return Value : Float
******************************************************************************/
float ReadRemoteThrottle() {
	return CalculatePercentageOfMaximum(throttle->dutyCycle, throttle->frequency);
}

/******************************************************************************
* Function Name: ReadRemotePidProportional
* Description  : This function calculate percentage of PID proportional.
* Arguments    : None
* Return Value : Float
******************************************************************************/
float ReadRemotePidProportional() {
	return CalculatePercentageOfMaximum(remotePidProportional->dutyCycle, remotePidProportional->frequency);
}

/******************************************************************************
* Function Name: ReadRemotePidIntegral
* Description  : This function calculate percentage of PID Integral.
* Arguments    : None
* Return Value : Float
******************************************************************************/
float ReadRemotePidIntegral() {
	return CalculatePercentageOfMaximum(remotePidIntegral->dutyCycle, remotePidIntegral->frequency);
}

/******************************************************************************
* Function Name: ReadResetAngularPosition
* Description  : This function calculate percentage of position of angular.
* Arguments    : None
* Return Value : Float
******************************************************************************/
float ReadResetAngularPosition() {
	return CalculatePercentageOfMaximum(resetAngularPosition->dutyCycle, resetAngularPosition->frequency);
}

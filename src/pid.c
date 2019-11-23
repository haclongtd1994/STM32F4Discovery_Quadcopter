
#include <pid.h>
#include <math.h>

/******************************************************************************
* Function Name: InitialisePid
* Description  : This function initialize pid.
* Arguments    : Proportinal, integral, diff
* Return Value : Structure PID
******************************************************************************/
/* note, cumulative error should really be bounded to the last know 'good' state, or the last x calcuations etc */
struct Pid InitialisePid(float proportional, float integral, float differential) {
	Pid pid;

	pid.proportional = proportional;
	pid.integral = integral;
	pid.differential = differential;
	pid.lastError = 0.0;
	pid.cumulativeError = 0.0;

	return pid;
}

/******************************************************************************
* Function Name: CalculatePidAdjustment
* Description  : This function calculate PID Adj.
* Arguments    : PID, current, target
* Return Value : Float
******************************************************************************/
float CalculatePidAdjustment(Pid* pid, float current, float target) {

	if (isnan(current) || isnan(target)) {
		return 0.0;
	}

    float error = target - current;
    float diff = error - pid->lastError;
    pid->lastError = error;
    pid->cumulativeError += error;

    return (pid->proportional * error) +
           (pid->integral * pid->cumulativeError) +
           (pid->differential * diff);
}

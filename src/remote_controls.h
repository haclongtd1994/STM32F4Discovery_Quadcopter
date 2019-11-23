#ifndef REMOTE_CONTROLS_H_
#define REMOTE_CONTROLS_H_

#include <stdint.h>
#include <pwm_input.h>
// Store throttle
struct PWMInput* throttle;

struct PWMInput* remotePidProportional;

struct PWMInput* remotePidIntegral;

struct PWMInput* resetAngularPosition;

void InitialiseRemoteControls();			// Function to initialize remote controls

/* These will come back as a percentage */
float ReadRemoteThrottle();					// Read value of throttle

float ReadRemotePidProportional();			// Read value of PID proportional

float ReadRemotePidIntegral();				// Read value of PID Integral

float ReadResetAngularPosition();			// Read value of position of angular at reset



#endif

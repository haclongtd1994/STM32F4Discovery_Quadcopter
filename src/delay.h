
#ifndef __DELAY_H
#define __DELAY_H

#include <stdint.h>

void EnableTiming();					// Function to enable delay timer
void TimingDelay(unsigned int tick);	// Function delay with cycle of chip
void WaitASecond();						// Function delay 1 second
void WaitAMillisecond();				// Function delay 1 milisecond
void WaitAFewMillis(int16_t millis);	// Function delay miliseconds

#endif

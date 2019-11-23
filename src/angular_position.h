#ifndef ANGULAR_POSITION_H_
#define ANGULAR_POSITION_H_

#include <systick.h>
#include <stdbool.h>

bool sensorToggle;

typedef struct AngularPosition {
	float x;
	float y;
	float z;
}AngularPosition;

struct AngularPosition angularPosition;

// Function to initialize Angular Position
void InitialiseAngularPosition();

// Function to read angular position
void ReadAngularPosition();

// Function to Reset angular position
void ResetToAngularZeroPosition();

#endif

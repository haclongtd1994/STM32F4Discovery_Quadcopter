
#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include <i2c.h>
#include <stdint.h>

extern bool isReadingAccelerometer;

typedef struct AccelerometerReading {
	float x;
	float y;
	float z;
	float xG;
	float yG;
	float zG;
	float xOffset;
	float yOffset;
	float zOffset;
	uint32_t readings;
}AccelerometerReading;

extern struct AccelerometerReading accelerometerReading;

// Function to initialize Accelermeter
void InitialiseAccelerometer();

// Function to read value from i2c
void ReadAccelerometer();

#endif

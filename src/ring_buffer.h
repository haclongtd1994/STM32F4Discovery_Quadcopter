#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#include <stdint.h>

// Variable size of buffer ring
#define RING_BUFFER_SIZE 1024

// Struct to store data
typedef struct RingBuffer
{
	uint16_t buffer[RING_BUFFER_SIZE];
	int head;
	int tail;
	int count;
} RingBuffer;

void  InitialiseRingBuffer(RingBuffer *_this);				// InitialiseRingBuffer
int RingBufferIsEmpty(RingBuffer *_this);					// Check buffer empty
int RingBufferIsFull(RingBuffer *_this);					// Check buffer full
uint16_t RingBufferPop(RingBuffer *_this);					// Pop data which don't need anymore
void RingBufferPut(RingBuffer *_this, uint16_t value);		// Put new data


#endif

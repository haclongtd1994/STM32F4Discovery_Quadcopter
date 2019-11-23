
#ifndef PWM_INPUT_H
#define PWM_INPUT_H

#include <stm32f4xx_tim.h>
#include <stdint.h>

// Struct data to manage PWM input
typedef struct PWMInput {
    volatile float dutyCycle;
    volatile float frequency;
    uint8_t hclckDivisor;

}PWMInput;

// Initialize 4 PWM input on board
struct PWMInput pwmInputTimer4;
struct PWMInput pwmInputTimer5;
struct PWMInput pwmInputTimer9;
struct PWMInput pwmInputTimer12;

// Function to measure PWM
struct PWMInput* MeasurePWMInput(TIM_TypeDef *TIMx, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint8_t GPIO_PinSource);

#endif

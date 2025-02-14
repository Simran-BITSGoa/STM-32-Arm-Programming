/**
 * p7_4.c - Generate Sawtooth Waveform using PWM (TIM1 CH1 on PA8)
 *
 * Since STM32F401RE has no DAC, we use TIM1 PWM to generate a 
 * sawtooth waveform by continuously increasing the duty cycle.
 *
 * **Hardware Configuration:**
 * - **PA8 (TIM1 CH1 PWM Output)** ? Analog Approximation
 *
 * **Waveform Properties:**
 * - **PWM Frequency = 1 kHz**
 * - **12-bit resolution (0-4095)**
 * - **Sawtooth shape (0% to 100% duty cycle)**
 */

#include "stm32f4xx.h"

void TIM1_PWM_Init(void);
void delayMs(int n);

int main(void)
{
    uint16_t dutyCycle = 0;

    /* === 1) Initialize TIM1 PWM on PA8 === */
    TIM1_PWM_Init();

    /* === 2) Continuous Sawtooth Waveform Generation === */
    while (1)
    {
        TIM1->CCR1 = dutyCycle;  // Set duty cycle

        dutyCycle++;             // Increment duty cycle (sawtooth shape)
        
        if (dutyCycle > 4095) dutyCycle = 0;  // Reset at full scale (sawtooth reset)

        delayMs(1);  // Adjust waveform speed
    }
}

/**
 * Initialize TIM1 PWM on PA8 (TIM1 CH1)
 */
void TIM1_PWM_Init(void)
{
    /* Enable GPIOA & TIM1 Clock */
    RCC->AHB1ENR |= (1U << 0);  // Enable GPIOA clock
    RCC->APB2ENR |= (1U << 0);  // Enable TIM1 clock

    /* Configure PA8 as Alternate Function (AF1 for TIM1) */
    GPIOA->MODER &= ~(3U << 16);  // Clear PA8 mode
    GPIOA->MODER |= (2U << 16);   // Set PA8 as Alternate Function mode
    GPIOA->AFR[1] |= (1U << 0);   // Set PA8 to AF1 (TIM1 CH1)

    /* Configure TIM1 for PWM */
    TIM1->PSC = 15;               // Prescaler: 16 MHz / (PSC + 1) = 1 MHz timer clock
    TIM1->ARR = 4095;             // 12-bit resolution (0-4095)
    TIM1->CCR1 = 0;               // Start with 0% duty cycle
    TIM1->CCMR1 |= (6U << 4);     // Set PWM Mode 1 (OC1M = 110)
    TIM1->CCMR1 |= (1U << 3);     // Enable preload for CCR1
    TIM1->CCER |= (1U << 0);      // Enable TIM1 CH1 output
    TIM1->BDTR |= (1U << 15);     // Enable TIM1 main output (MOE)
    TIM1->CR1 |= (1U << 0);       // Enable TIM1
}

/**
 * Simple blocking delay (ms)
 */
void delayMs(int n)
{
    volatile int i;
    for (; n > 0; n--)
        for (i = 0; i < 3195; i++) __NOP();
}

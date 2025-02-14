/**
 * p7_3.c - LM34 Temperature Sensor (Fahrenheit) with ADC1 & USART2
 *
 * This program reads temperature data from the LM34 sensor connected to PC0 (ADC1 Channel 10)
 * and prints the converted temperature in Fahrenheit (°F) over USART2 (9600 baud).
 *
 * **Hardware Connections:**
 * - **PC0 (ADC1 Channel 10)** -> LM34 Sensor Output
 * - **PA2 (USART2 TX)** -> Serial Output to ST-Link Virtual COM Port
 *
 * **Formula:**
 *   - LM34 Output = **10mV per °F**
 *   - **Temperature (°F) = (ADC Value / 4095) * 3.3V * 100**
 */

#include "stm32f4xx.h"
#include <stdio.h>

void USART2_init(void);
int USART2_write(int c);
void delayMs(int n);

int main(void)
{
    uint32_t result;
    double temperature;

    /* === 1) Enable GPIOC Clock & Configure PC0 as Analog Input === */
    RCC->AHB1ENR |= (1U << 2);    // Enable GPIOC clock
    GPIOC->MODER |= (3U << 0);    // Set PC0 as analog mode

    /* === 2) Configure ADC1 (Channel 10 - PC0) === */
    RCC->APB2ENR |= (1U << 8);    // Enable ADC1 clock
    ADC1->CR2 = 0;                // Software trigger mode
    ADC1->SQR3 = 10;              // ADC1 Channel 10 (PC0)
    ADC1->SQR1 = 0;               // Only 1 conversion in sequence
    ADC1->SMPR1 = (3U << 0);      // Set sampling time to 15 cycles (better accuracy)
    ADC1->CR2 |= 1;               // Enable ADC1

    /* === 3) Initialize USART2 for Serial Output (9600 baud) === */
    USART2_init();
    printf("LM34 Temperature Sensor (ADC1 CH10)\r\n");

    /* === 4) Main Loop: Read ADC & Print Temperature === */
    while (1)
    {
        ADC1->CR2 |= (1U << 30);  // Start ADC conversion (SWSTART)

        while (!(ADC1->SR & (1U << 1))) {}  // Wait for ADC conversion complete (EOC flag)

        result = ADC1->DR;  // Read ADC result

        // Convert ADC value to Fahrenheit (°F)
        temperature = ((double)result / 4095.0) * 3.3 * 100.0;

        // Print result to USART2
        printf("ADC: %d, Temp: %.2f°F\r\n", result, temperature);

        delayMs(1000);  // 1-second delay for stability
    }
}

/**
 * Initialize USART2 for 9600 baud serial output
 */
void USART2_init(void)
{
    /* Enable GPIOA & USART2 Clocks */
    RCC->AHB1ENR |= (1U << 0);    // Enable GPIOA clock
    RCC->APB1ENR |= (1U << 17);   // Enable USART2 clock

    /* Configure PA2 (USART2 TX) */
    GPIOA->MODER &= ~(3U << 4);  // Clear PA2 mode
    GPIOA->MODER |= (2U << 4);   // Set PA2 to Alternate Function (AF7)
    GPIOA->AFR[0] |= (7U << 8);  // Set PA2 to AF7 (USART2_TX)

    /* Configure USART2 */
    USART2->BRR = 0x0683;  // 9600 baud @ 16 MHz
    USART2->CR1 = (1U << 3) | (1U << 13);  // Enable TX & USART2
}

/**
 * Send a character over USART2
 */
int USART2_write(int ch)
{
    while (!(USART2->SR & (1U << 7))) {}  // Wait until TX buffer is empty
    USART2->DR = ch;
    return ch;
}

/**
 * Redirect printf() to USART2
 */
int fputc(int c, FILE *f)
{
    return USART2_write(c);
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

/**
 * p7_1.c - ADC1 Channel 1 (PA1) Input Conversion & LED Control
 *
 * Reads an analog voltage on PA1, performs ADC conversion,
 * and toggles the LED (PA5) based on bit 8 of the ADC result.
 *
 * **Hardware Connections:**
 * - **PA1** ? ADC1 Channel 1 (Analog Input)
 * - **PA5** ? On-board LED (LD2, Output)
 */

#include "stm32f4xx.h"

int main(void)
{
    uint32_t result;  // Fixed signed-unsigned conversion issue

    /* === 1) Enable GPIOA Clock & Configure PA5 as Output === */
    RCC->AHB1ENR |= (1U << 0);      // Enable GPIOA clock
    GPIOA->MODER &= ~(uint32_t)(0x00000C00); // Clear PA5 mode (Fix signedness warning)
    GPIOA->MODER |=  (uint32_t)(0x00000400); // Set PA5 as Output

    /* === 2) Configure PA1 as Analog Input (ADC1 Channel 1) === */
    GPIOA->MODER |= (3U << 2);  // Set PA1 to Analog mode (11)

    /* === 3) Configure ADC1 === */
    RCC->APB2ENR |= (1U << 8);  // Enable ADC1 clock
    ADC1->CR2 = 0;              // Software trigger mode
    ADC1->SQR3 = 1;             // Set sequence start at Channel 1
    ADC1->SQR1 = 0;             // Only 1 conversion
    ADC1->CR2 |= 1;             // Enable ADC1

    /* === 4) Continuous ADC Read & LED Control Loop === */
    while (1)
    {
        ADC1->CR2 |= (1U << 30);  // Start ADC conversion (SWSTART)

        while (!(ADC1->SR & (1U << 1)))  // Wait for End of Conversion (EOC)
        {
            // Polling EOC bit until conversion completes
        }

        result = (uint32_t)ADC1->DR;  // Read ADC result (Fix signedness warning)

        if (result & 0x100)  // Check if bit 8 is set
        {
            GPIOA->BSRR = (1U << 5);  // Turn ON LED (PA5)
        }
        else
        {
            GPIOA->BSRR = (1U << (5 + 16));  // Turn OFF LED (PA5)
        }
    }

    return 0;  // Best practice for main function
}

/**
 * p2_2.c - Toggle On-board Green LED (LD2) using SysTick (16 MHz)
 *
 * This program toggles the on-board LED (PA5) every 500 ms using SysTick.
 * The system clock is running at 16 MHz.
 *
 * - SysTick is configured to count down from 7,999,999 to 0 (for 500ms delay).
 * - The LED toggles when SysTick's COUNTFLAG is set.
 *
 * Pin Assignments:
 *   - PA5 => On-board LED (LD2 on Nucleo-F401RE)
 */

#include "stm32f4xx.h"  // or "stm32f401xe.h" for STM32F401RE

int main(void)
{
    /* 1) Enable GPIOA clock (for LED on PA5) */
    RCC->AHB1ENR |= (1 << 0);   // Enable GPIOA

    /* 2) Set PA5 as output */
    GPIOA->MODER &= ~0x00000C00;  // Clear mode bits for PA5
    GPIOA->MODER |=  0x00000400;  // Set PA5 as output mode

    /* 3) Configure SysTick for 500ms delay */
    SysTick->LOAD = 8000000 - 1;  // (16 MHz / 2) - 1 = 500 ms delay
    SysTick->VAL = 0;             // Clear current value
    SysTick->CTRL = 5;            // Enable SysTick, No Interrupt, System Clock

    while (1)
    {
        /* 4) Wait for COUNTFLAG (bit 16 in SysTick->CTRL) */
        if (SysTick->CTRL & 0x10000)
        {
            GPIOA->ODR ^= (1 << 5);  // Toggle PA5 (LD2)
        }
    }
}

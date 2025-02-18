/**
 * p10_1.c - Relay control on STM32F401RE
 *
 * Toggles PA5 high/low every 500 ms.
 * If a relay is connected to PA5 (through a transistor driver), 
 * it will switch on/off accordingly.
 * On the Nucleo-F401RE board, PA5 is also the user LED.
 */

#include "stm32f4xx.h"

void delayMs(int n);

int main(void)
{
    /* 1) Enable GPIOA clock */
    RCC->AHB1ENR |= (1U << 0);  // bit0 => GPIOA

    /* 2) Configure PA5 as output */
    GPIOA->MODER &= ~(3U << (2 * 5));  // clear mode bits for PA5
    GPIOA->MODER |=  (1U << (2 * 5));  // set PA5 to output

    while(1)
    {
        /* Turn relay/LED on (PA5=1) */
        GPIOA->BSRR = (1U << 5);   // set PA5
        delayMs(500);

        /* Turn relay/LED off (PA5=0) */
        GPIOA->BSRR = (1U << (5 + 16));  // reset PA5
        delayMs(500);
    }
}

/**
 * Simple software delay ~1 ms for each n @16 MHz sysclock
 */
void delayMs(int n)
{
    volatile int i;
    for (; n > 0; n--)
    {
        for (i = 0; i < 3195; i++)
        {
            __NOP();  // Prevent optimization
        }
    }
}

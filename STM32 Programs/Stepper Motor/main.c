/**
 * p10_2.c - Stepper Motor Control on STM32F401RE
 *
 * Drives a unipolar stepper on PA7, PA6, PA5, PA4.
 * Steps array cycles through patterns at ~100 ms intervals.
 * Assumes a 16 MHz system clock for the delay loop.
 */

#include "stm32f4xx.h"

void delayMs(int n);

int main(void)
{
    /* Step pattern for unipolar stepper:
       bits correspond to PA7..PA4 => (7..4)
         0x90 = 1001 0000 => A7 & A4
         0x30 = 0011 0000 => A5 & A4
         0x60 = 0110 0000 => A6 & A5
         0xC0 = 1100 0000 => A7 & A6
    */
    const uint8_t steps[] = {0x90, 0x30, 0x60, 0xC0};
    int i = 0;

    /* 1) Enable GPIOA clock */
    RCC->AHB1ENR |= (1U << 0);  // bit0 => GPIOA

    /* 2) Configure PA7..PA4 as outputs 
     * Clear pin mode bits for A7..A4 => bits 15..8 in MODER
     * Then set them to 01 => output mode => 0x55 in that nibble
    */
    GPIOA->MODER &= ~0x0000FF00; // Clear modes for A7..A4
    GPIOA->MODER |=  0x00005500; // Set A7..A4 as output

    while (1)
    {
        /* 3) Write pattern to bits 7..4 
         * (This will overwrite the lower bits of ODR as well, 
         * so ensure those aren't used. Alternatively, mask or shift 
         * if you only want to affect bits 7..4.)
         */
        GPIOA->ODR = steps[i++ & 3];

        delayMs(100);
    }
}

/**
 * Simple blocking delay:
 * ~1 ms per 'n' at 16 MHz
 */
void delayMs(int n)
{
    volatile int i;
    for (; n > 0; n--)
    {
        for (i = 0; i < 3195; i++)
        {
            __NOP();
        }
    }
}

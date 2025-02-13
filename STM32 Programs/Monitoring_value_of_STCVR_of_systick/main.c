/* p5_1.c Toggle Green LED (LD2) on STM32F446RE Nucleo64 using
SysTick
* This program configures SysTick to be a 24-bit free-running
* down-counter. Bit 23 of SysTick current value is written to
* the LED (PA5) continuously.
* SysTick is based on system clock running at 16 MHz.
* So bit 23 of SysTick current value toggles about 1 Hz
* (16,000,000 Hz / 2^23 = 1.907 Hz).
*
* This program was tested with Keil uVision v5.24a with DFP v2.11.0
*/
#include "stm32f4xx.h"
void delayMs(int n);
int main(void) {
RCC->AHB1ENR |= 1; /* enable GPIOA clock */
GPIOA->MODER &= ~0x00000C00; /* clear pin mode */
GPIOA->MODER |= 0x00000400; /* set pin to output mode */
/* Configure SysTick */
SysTick->LOAD = 0xFFFFFF; /* reload with max value */
SysTick->CTRL
= 5; /* enable it, no interrupt, use system clock */
while (1) {
/* take bit 23 of SysTick current value and shift it to bit 5 then
write it to PortA */
GPIOA->ODR = (SysTick->VAL >> (23 - 5)) & 0x00000020;
}
}
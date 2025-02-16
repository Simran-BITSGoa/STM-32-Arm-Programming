*/
* This program configures TIM2 with prescaler divides by 10
* so that the timer counter is counting at 1.6 MHz.
* ARR register is loaded with 26666 and CCR1 is loaded with 8888.
* The channel 1 is configured as PWM mode. The output of Ch1 is
* turned on when the counter starts counting from 0. When the
* counter matches the content of CCR1, Ch1 output is turned off.
* When the counter matches ARR, the counter is cleared to 0 and
* the output is turned on and the counter starts counting up again.
* The LED will be on for 8889/26667 = 30% of the time.
* The output pin of TIM2 Ch1 is PA5 and the alternate function
* of PA5 should be set to AF1.
*
* This program was tested with Keil uVision v5.24a with DFP v2.11.0
*/
#include "stm32f4xx.h"
int main(void) {
RCC->AHB1ENR |= 1; /* enable GPIOA clock */
GPIOA->AFR[0] |= 0x00100000; /* PA5 pin for tim2 */
GPIOA->MODER &=
~0x00000C00;
GPIOA->MODER |= 0x00000800;
/* setup TIM2 */
RCC->APB1ENR |= 1; /* enable TIM2 clock */
TIM2->PSC = 10 - 1; /*
divided by 10 */
TIM2->ARR = 26667 - 1; /* divided by 26667 */
TIM2->CNT = 0;
TIM2->CCMR1 = 0x0060; /* PWM mode */
TIM2->CCER = 1; /* enable PWM Ch1 */
TIM2->CCR1 = 8889 - 1; /* pulse width 1/3 of the period */
TIM2->CR1
= 1; /* enable timer */
while(1) {
}
}
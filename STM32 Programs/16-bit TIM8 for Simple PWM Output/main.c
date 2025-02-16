/* p11_2.c Using TIM8 for PWM Output
*
* This program configures TIM8 with prescaler divides by 10
* so
that the timer counter is counting at 1.6 MHz.
* ARR register is loaded with 26666 and CCR1 is loaded with 8888.
*
The channel 1 is configured as PWM mode. The output of Ch1N is
*
turned on when the counter starts counting from 0. When the *
counter matches the content of CCR1, Ch1N output is turned off.
*
When the counter matches ARR, the counter is cleared to 0 and
* the
output is turned on and the counter starts counting up again.
* The
LED will be on for 8889/26667 = 30% of the time.
* The output pin of TIM8 Ch1N is PA5 and the alternate function
* of
PA5 should be set to AF1.
*
* This program is similar to p11_1.c except:
* 1. Each PWM output channel (Chx) has a complementary output (ChxN)
* and we are using the Ch1N output.
* 2. The output needs to be enabled at TIM8_BDTR register.
*
* This program was tested with Keil uVision v5.24a with DFP v2.11.0
*/
#include "stm32f4xx.h"
int main(void) {
RCC->AHB1ENR |= 1; /* enable GPIOA clock */
GPIOA->AFR[0] |=
0x00300000; /* PA5 pin for TIM8 */
GPIOA->MODER &= ~0x00000C00;
GPIOA->MODER |= 0x00000800;
/* setup TIM8 */
RCC->APB2ENR |= 2; /* enable TIM8 clock */
TIM8->PSC = 10 - 1; /*
divided by 10 */
TIM8->ARR = 26667 - 1; /* divided by 26667 */
TIM8->CNT = 0;
TIM8->CCMR1 = 0x0060; /* PWM mode */
TIM8->CCER = 4; /* enable PWM Ch1N */
TIM8->CCR1 = 8889 - 1; /* pulse width 1/3 of the period */
TIM8-
>BDTR |= 0x8000; /* enable output */
TIM8->CR1 = 1; /* enable timer */
while(1) {
}
}
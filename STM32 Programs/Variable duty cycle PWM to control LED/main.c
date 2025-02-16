/* p11_4.c Variable duty cycle PWM to control LED on duration
*
* This program is the same as p11_3.c except the frequency is much
*
slower so that the change of the duration when the LED is on
* can
be observed by naked eyes.
*
* This program was tested with Keil uVision v5.24a with DFP v2.11.0
*/
#include "stm32f4xx.h"
void delayMs(int n);
int main(void) {
RCC->AHB1ENR |= 1; /* enable GPIOA clock */
GPIOA->AFR[0] |= 0x00300000; /* PA5 pin for TIM8 */
GPIOA->MODER &=
~0x00000C00;
GPIOA->MODER |= 0x00000800;
/* setup TIM8 */
RCC->APB2ENR |= 2; /* enable TIM8 clock */
TIM8->PSC = 16000 - 1; /*
divided by 16000 */
TIM8->ARR = 1000 - 1; /* divided by 1000 */
TIM8->CNT = 0;
TIM8->CCMR1 = 0x0068; /* PWM mode */
TIM8->CCER = 4; /* enable PWM
Ch1N */
TIM8->CCR1 = 10; /* pulse width */
TIM8->BDTR |= 0x8000; /*
enable output */
TIM8->CR1 = 1; /* enable timer */
while(1) {
TIM8->CCR1 = TIM8->CCR1 * 110 / 100;
if (TIM8->CCR1 > 1000)
TIM8->CCR1 = 10;
delayMs(400);
}
}
/* 16 MHz SYSCLK */
void delayMs(int n) {
int i;
for (; n > 0; n--)
for (i = 0; i < 3195; i++) ;
}
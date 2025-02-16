/* p11_5.c Comparison between edge-aligned and center-aligned PWM
*
* In this program, TIM8 Ch1 is configured as center-aligned PWM with
* output on PC6 pin and TIM2 Ch1 is configured as edge-aligned PWM
*
with output on PA5 pin for comparison.
*
* You will find two differences between these two PWM generation:
*
1. For edge-aligned PWM, the counter counts from 0 to the value
* of
ARR then it takes one clock cycle to reset to 0. So to
* count 1000
clock cycles, the ARR should be loaded with 999.
* For centeraligned
PWM, the counter counts up from 0 to the
* value of ARR then
immediately counts down back to 0. So to
* count 1000 clock cycles,
the ARR should be loaded with 1000.
* 2. For edge-aligned PWM, each
output cycle the counter counts * from 0 to the value of ARR. For
center-aligned PWM, each
* output cycle, the counter counts from 0
up to the value of
* ARR then back down to 0. For the same value of
ARR, the * center-aligned PWM takes twice as long as edge-aligned
PWM
* to complete an output cycle.
*
* This program was tested with Keil uVision v5.24a with DFP v2.11.0
*/
#include "stm32f4xx.h"
void TIM8_center_aligned(void);
void TIM2_edge_aligned(void);
int main(void) {
TIM8_center_aligned();
TIM2_edge_aligned();
}
}
void TIM8_center_aligned(void) {
RCC->AHB1ENR |= 4; /* enable GPIOC clock */
GPIOC->AFR[0] |= 0x03000000; /* PC6 pin for tim8 Ch1 */
GPIOC->MODER
&= ~0x00003000;
GPIOC->MODER |= 0x00002000;
/* setup TIM8 */
RCC->APB2ENR |= 2; /* enable TIM8 clock */
TIM8->PSC = 16 - 1; /*
divided by 16 */
TIM8->ARR = 1000; /* divided by 1000 */
TIM8->CNT = 0;
TIM8->CCMR1 = 0x0060; /* PWM mode */
TIM8->CCER = 1; /* enable PWM Ch1 */
TIM8->CCR1 = 300 - 1; /* pulse
width 300 */
TIM8->BDTR |= 0x8000; /* enable output */
TIM8->CR1 = 0x21; /* center-aligned, enable timer */
}
void TIM2_edge_aligned(void) {
RCC->AHB1ENR |= 1; /* enable GPIOA clock */
GPIOA->AFR[0] |= 0x00100000; /* PA5 pin for tim2 */
GPIOA->MODER &=
~0x00000C00;
GPIOA->MODER |= 0x00000800;
/* setup TIM2 */
RCC->APB1ENR |= 1; /* enable TIM2 clock */
TIM2->PSC = 16 - 1; /*
divided by 16 */
TIM2->ARR = 1000 - 1; /* divided by 1000 */
TIM2-
>CNT = 0;
TIM2->CCMR1 = 0x0060; /* PWM mode */
TIM2->CCER = 1; /* enable PWM Ch1 */
TIM2->CCR1 = 300 - 1; /* pulse
width 300 */
TIM2->CR1 = 1; /* edge-aligned enable timer */
}
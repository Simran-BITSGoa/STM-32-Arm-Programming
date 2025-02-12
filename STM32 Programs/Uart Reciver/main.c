/**
 * p4_2.c (For STM32F401RE @ 16 MHz, 9600 Baud)
 *
 * Receives characters over USART2 at 9600 baud and blinks the on-board LED (PA5)
 * according to the received character (mod 16). Wait for each blink sequence
 * to finish before sending another character.
 *
 * Pin Connections (Nucleo-F401RE):
 *   - PA3 = USART2 RX (connected to ST-Link VCOM)
 *   - PA5 = On-board LED (LD2)
 *
 * Ensure that your system clock is configured for 16 MHz in startup code
 * (e.g. using HSI at 16 MHz, no PLL).
 */

#include "stm32f4xx.h"  // Or "stm32f401xe.h" depending on your CMSIS setup

void USART2_init(void);
char USART2_read(void);
void LED_blink(int value);
void delayMs(int n);

int main(void)
{
    char c;

    /* 1. Enable GPIOA clock (for PA5 LED & PA3). */
    RCC->AHB1ENR |= (1U << 0);  // bit 0 => GPIOA

    /* 2. Configure PA5 as output (LED). */
    GPIOA->MODER &= ~0x00000C00;  // clear mode bits for PA5
    GPIOA->MODER |=  0x00000400;  // set PA5 to output

    /* 3. Initialize USART2 at 9600 baud. */
    USART2_init();

    while (1)
    {
        /* 4. Read one character from USART2. */
        c = USART2_read();

        /* 5. Blink LED according to the character (mod 16). */
        LED_blink(c);
    }
}

/**
 * Configure USART2 for:
 *  - 16 MHz system clock
 *  - 9600 baud
 *  - 8 data bits, no parity, 1 stop bit
 *  - Oversampling by 16
 *  - RX on PA3 (AF7)
 */
void USART2_init(void)
{
    /* a) Enable GPIOA clock (already set, but safe to repeat). */
    RCC->AHB1ENR |= (1U << 0);

    /* b) Enable USART2 clock on APB1. */
    RCC->APB1ENR |= (1U << 17);  // bit 17 => USART2

    /* c) Calculate BRR for 9600 baud @ 16 MHz
     *    USARTDIV = 16,000,000 / (16 * 9600) = 104.1667
     *    Mantissa = 104 = 0x68, Fraction = ~2.6667 => ~3 (0x3)
     *    => BRR = (0x68 << 4) + 0x3 = 0x683
     */
    USART2->BRR = 0x0683;

    /* d) Configure CR1, CR2, CR3 for 8N1, Rx enable, no parity, etc. */
    USART2->CR1 = 0x0004;    // RE=1
    USART2->CR2 = 0x0000;    // 1 stop bit
    USART2->CR3 = 0x0000;    // no flow control

    /* e) Enable USART2 by setting UE bit. */
    USART2->CR1 |= 0x2000;   // UE=1

    /* f) Configure PA3 as USART2_RX, alternate function AF7. */
    GPIOA->AFR[0] &= ~(0xF << 12);  // clear AFR for PA3
    GPIOA->AFR[0] |=  (0x7 << 12);  // AF7
    GPIOA->MODER  &= ~(0x3 << 6);   // clear mode for PA3
    GPIOA->MODER  |=  (0x2 << 6);   // alt-function mode (10)
}

/**
 * Read a single character from USART2 (blocking).
 */
char USART2_read(void)
{
    /* Wait until RXNE (Receive Data Register Not Empty) is set */
    while (!(USART2->SR & 0x20)) {
        // spin
    }
    /* Return the received data (DR). */
    return (char)USART2->DR;
}

/**
 * Blink on-board LED (PA5) "value" times, mod 16.
 */
void LED_blink(int value)
{
    value &= 0x0F; // mod 16

    while (value-- > 0)
    {
        /* Turn LED on */
        GPIOA->BSRR = (1U << 5);
        delayMs(200);

        /* Turn LED off */
        GPIOA->BSRR = (1U << (5 + 16));
        delayMs(200);
    }

    /* Additional pause before next character */
    delayMs(800);
}

/**
 * Approximate 1 ms delay for 16 MHz using a simple loop.
 * If you need more accurate timing, consider using SysTick or TIM.
 */
void delayMs(int n)
{
    volatile int i;
    for (; n > 0; n--)
    {
        for (i = 0; i < 2000; i++) {
            __NOP();
        }
    }
}

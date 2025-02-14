/**
 * p8_1.c - Sending A-Z characters over SPI1 (STM32F401RE)
 *
 * This program configures SPI1 as a master and sends characters 
 * 'A' to 'Z' to an SPI slave.
 *
 * **SPI1 Pin Configuration:**
 * - **PA5 (SCK)  - SPI1 Clock**
 * - **PA7 (MOSI) - SPI1 Master Output, Slave Input**
 * - **PA4 (SS)   - SPI1 Slave Select (Manual Control)**
 *
 * **SPI1 Settings:**
 * - **Mode 0 (CPOL = 0, CPHA = 0)**
 * - **Baud rate = 1 MHz**
 * - **8-bit data frame, MSB first**
 */

#include "stm32f4xx.h"

void delayMs(int n);
void SPI1_init(void);
void SPI1_write(unsigned char data);

int main(void)
{
    char c;
    
    SPI1_init();  // Initialize SPI1

    while (1)
    {
        for (c = 'A'; c <= 'Z'; c++)  // Send A-Z characters
        {
            SPI1_write(c);  // Write character via SPI1
            delayMs(100);   // Delay for stability
        }
    }
}

/**
 * Initialize SPI1 in Master Mode
 */
void SPI1_init(void)
{
    /* Enable GPIOA & SPI1 Clocks */
    RCC->AHB1ENR |= (1U << 0);   // Enable GPIOA clock
    RCC->APB2ENR |= (1U << 12);  // Enable SPI1 clock

    /* Configure PA5 (SCK) & PA7 (MOSI) as Alternate Function (AF5 for SPI1) */
    GPIOA->MODER &= ~((3U << 10) | (3U << 14)); // Clear PA5, PA7 mode
    GPIOA->MODER |=  ((2U << 10) | (2U << 14)); // Set PA5, PA7 as AF mode
    GPIOA->AFR[0] |= ((5U << 20) | (5U << 28)); // Set PA5, PA7 to AF5 (SPI1)

    /* Configure PA4 as Manual Slave Select (GPIO Output) */
    GPIOA->MODER &= ~(3U << 8);  // Clear PA4 mode
    GPIOA->MODER |=  (1U << 8);  // Set PA4 as output

    /* Configure SPI1 */
    SPI1->CR1 = (1U << 2)   // Master mode
              | (3U << 3)   // Baud rate = fPCLK/16 (1 MHz)
              | (1U << 6);  // Enable SPI1

    SPI1->CR2 = 0;  // Default settings (8-bit data, no interrupts)
}

/**
 * Send a byte via SPI1
 */
void SPI1_write(unsigned char data)
{
    GPIOA->BSRR = (1U << 20); // Assert slave select (PA4 LOW)

    while (!(SPI1->SR & (1U << 1))) {} // Wait until TX buffer empty
    SPI1->DR = data;                   // Transmit data

    while (SPI1->SR & (1U << 7)) {} // Wait for transmission complete

    GPIOA->BSRR = (1U << 4); // Deassert slave select (PA4 HIGH)
}

/**
 * Simple blocking delay (ms)
 */
void delayMs(int n)
{
    volatile int i;
    for (; n > 0; n--)
        for (i = 0; i < 3195; i++) __NOP();
}

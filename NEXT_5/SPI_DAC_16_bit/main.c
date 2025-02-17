/**
 * p8_3.c - Generating Sawtooth Waveform on LTC1661 using SPI1 (STM32F401RE)
 *
 * This program configures SPI1 as a master and generates a sawtooth waveform
 * by writing to the LTC1661 Digital-to-Analog Converter (DAC) using a 16-bit frame.
 *
 * **SPI1 Pin Configuration:**
 * - **PA5 (SCK)  - SPI1 Clock**
 * - **PA7 (MOSI) - SPI1 Master Output, Slave Input**
 * - **PA4 (SS)   - SPI1 Slave Select (Manual Control)**
 *
 * **SPI1 Settings:**
 * - **Mode 0 (CPOL = 0, CPHA = 0)**
 * - **Baud rate = 1 MHz**
 * - **16-bit data frame, MSB first**
 */

#include "stm32f4xx.h"

void SPI1_init(void);
void DAC_write(uint16_t data);
void delayMs(int n);

int main(void)
{
    uint16_t i;
    
    SPI1_init();  // Initialize SPI1

    while (1)
    {
        for (i = 0; i < 1024; i++)  // Generate a 10-bit sawtooth waveform
        {
            DAC_write(i);  // Write data to DAC
            delayMs(1);    // Adjust waveform frequency
        }
    }
}

/**
 * Initialize SPI1 in Master Mode (16-bit data frame)
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
              | (1U << 11)  // 16-bit data format
              | (1U << 6);  // Enable SPI1

    SPI1->CR2 = 0;  // Default settings (16-bit data, no interrupts)
}

/**
 * Send a 10-bit value to the LTC1661 DAC over SPI1 (16-bit frame)
 */
void DAC_write(uint16_t data)
{
    uint16_t spiData;

    /* Ensure data is within 10-bit range */
    data &= 0x03FF; 

    /* Prepare 16-bit SPI data (command + 10-bit data) */
    spiData = 0x9000 | (data << 2);

    /* Assert Slave Select (PA4 LOW) */
    GPIOA->BSRR = (1U << 20);

    /* Transmit 16-bit data */
    while (!(SPI1->SR & (1U << 1))) {}  // Wait until TX buffer empty
    SPI1->DR = spiData;

    /* Wait for transmission to complete */
    while (SPI1->SR & (1U << 7)) {} 

    /* Deassert Slave Select (PA4 HIGH) */
    GPIOA->BSRR = (1U << 4);
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
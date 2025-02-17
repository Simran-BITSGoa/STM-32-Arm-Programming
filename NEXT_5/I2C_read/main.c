/**
 * P9_2.c - I2C Byte Read from DS1337 RTC (STM32F401RE)
 *
 * This program initializes I2C1 and continuously reads
 * the seconds register (0x00) from the DS1337 RTC.
 * 
 * The LED (PA5) on the Nucleo board blinks every second
 * based on the least significant bit (LSB) of the read value.
 *
 * **I2C1 Pin Configuration:**
 * - **PB8 (I2C1_SCL) - I2C Clock Line**
 * - **PB9 (I2C1_SDA) - I2C Data Line**
 *
 * **I2C1 Settings:**
 * - **Clock Speed: 100 kHz (Standard Mode)**
 * - **Peripheral Clock: 16 MHz**
 */

#include "stm32f4xx.h"

#define SLAVE_ADDR 0x68  /* 1101 000 (DS1337 I2C Address) */

void I2C1_init(void);
int I2C1_byteRead(uint8_t saddr, uint8_t maddr, uint8_t *data);
void delayMs(int n);

int main(void)
{
    uint8_t data;

    I2C1_init();  // Initialize I2C1

    /* Configure PA5 for the Green LED (LD2) */
    RCC->AHB1ENR |= (1U << 0);  // Enable GPIOA clock
    GPIOA->MODER &= ~(3U << 10); // Clear PA5 mode
    GPIOA->MODER |= (1U << 10);  // Set PA5 as output

    while (1)
    {
        I2C1_byteRead(SLAVE_ADDR, 0x00, &data);  // Read seconds register (0x00)

        if (data & 1)
            GPIOA->ODR |= (1U << 5);  // Turn on LED
        else
            GPIOA->ODR &= ~(1U << 5);  // Turn off LED

        delayMs(1000);  // Delay for 1 second
    }
}

/**
 * Initialize I2C1 (PB8 = SCL, PB9 = SDA)
 */
void I2C1_init(void)
{
    /* Enable GPIOB and I2C1 Clocks */
    RCC->AHB1ENR |= (1U << 1);   // Enable GPIOB clock
    RCC->APB1ENR |= (1U << 21);  // Enable I2C1 clock

    /* Configure PB8 (SCL) & PB9 (SDA) as Alternate Function (AF4 for I2C1) */
    GPIOB->AFR[1] &= ~((0xF << 0) | (0xF << 4));  // Clear AF
    GPIOB->AFR[1] |= ((4 << 0) | (4 << 4));       // Set PB8, PB9 to AF4 (I2C1)

    /* Set PB8, PB9 to Alternate Function mode */
    GPIOB->MODER &= ~((3U << 16) | (3U << 18));  // Clear PB8, PB9 mode
    GPIOB->MODER |=  ((2U << 16) | (2U << 18));  // Set to Alternate Function mode

    /* Configure PB8, PB9 as Open-Drain with Pull-ups */
    GPIOB->OTYPER |=  (1U << 8) | (1U << 9);   // Open-Drain mode
    GPIOB->PUPDR  &= ~((3U << 16) | (3U << 18));  // Clear pull-up/pull-down
    GPIOB->PUPDR  |=  ((1U << 16) | (1U << 18));  // Enable pull-up resistors

    /* Reset and Enable I2C1 */
    I2C1->CR1 |= (1U << 15);  // Software Reset
    I2C1->CR1 &= ~(1U << 15); // Release from Reset

    /* Configure I2C1 for 100 kHz Standard Mode */
    I2C1->CR2 = (16U << 0);  // Peripheral Clock = 16 MHz
    I2C1->CCR = 80U;         // 100 kHz Clock
    I2C1->TRISE = 17U;       // Maximum Rise Time
    I2C1->CR1 |= (1U << 0);  // Enable I2C1
}

/**
 * Read a byte from DS1337 RTC
 * saddr: I2C Slave Address (0x68 for DS1337)
 * maddr: Memory/Register Address to Read
 * data:  Pointer to store the read value
 */
int I2C1_byteRead(uint8_t saddr, uint8_t maddr, uint8_t *data)
{
    volatile int tmp;

    /* Wait until the bus is not busy */
    while (I2C1->SR2 & (1U << 1));

    /* Generate Start Condition */
    I2C1->CR1 |= (1U << 8);
    while (!(I2C1->SR1 & (1U << 0)));  // Wait for Start flag

    /* Send Slave Address with Write Bit (0) */
    I2C1->DR = (saddr << 1);  // Address + Write
    while (!(I2C1->SR1 & (1U << 1)));  // Wait for Address flag
    tmp = I2C1->SR2;  // Clear Address flag

    /* Send Memory/Register Address */
    while (!(I2C1->SR1 & (1U << 7)));
    I2C1->DR = maddr;  // Send Register Address

    /* Generate Restart Condition */
    I2C1->CR1 |= (1U << 8);
    while (!(I2C1->SR1 & (1U << 0)));  // Wait for Restart flag

    /* Send Slave Address with Read Bit (1) */
    I2C1->DR = (saddr << 1) | 1;
    while (!(I2C1->SR1 & (1U << 1)));  // Wait for Address flag
    I2C1->CR1 &= ~(1U << 10);  // Disable Acknowledge
    tmp = I2C1->SR2;  // Clear Address flag

    /* Generate Stop Condition */
    I2C1->CR1 |= (1U << 9);

    /* Read Data */
    while (!(I2C1->SR1 & (1U << 6)));  // Wait until RXNE flag is set
    *data = I2C1->DR;

    return 0;  // Success
}

/**
 * Simple delay function (blocking)
 */
void delayMs(int n)
{
    volatile int i;
    for (; n > 0; n--)
        for (i = 0; i < 3195; i++) __NOP();
}

/**
 * P9_3.c - I2C Burst Write to DS1337 (STM32F401RE)
 *
 * This program initializes I2C1 and writes multiple bytes 
 * (time and date data) to the DS1337 Real-Time Clock starting 
 * from register 0.
 *
 * I2C1 Pin Configuration:
 *   - PB8: I2C1_SCL
 *   - PB9: I2C1_SDA
 *
 * I2C Settings:
 *   - 100 kHz standard mode
 *   - Peripheral Clock = 16 MHz
 */

#include "stm32f4xx.h"

#define SLAVE_ADDR 0x68   /* 1101 000 (DS1337 I2C Address) */

/* Example array to set time/date in DS1337:
   Index:  Register:
   0 => seconds
   1 => minutes
   2 => hours
   3 => day (1-7)
   4 => date (1-31)
   5 => month (1-12)
   6 => year (last 2 digits)
*/
char timeDateToSet[7] = {
    0x55,  // seconds
    0x58,  // minutes
    0x10,  // hours (24hr format)
    0x03,  // day (e.g. Tuesday=3)
    0x26,  // date
    0x09,  // month
    0x17   // year (20xx => 0x17 = 2017)
};

void I2C1_init(void);
void I2C1_burstWrite(uint8_t saddr, uint8_t maddr, int n, char *data);
void delayMs(int n);

int main(void)
{
    I2C1_init();  // Initialize I2C1

    // Write 7 bytes to DS1337 starting at register 0
    // This sets seconds, minutes, hours, day, date, month, year
    I2C1_burstWrite(SLAVE_ADDR, 0x00, 7, timeDateToSet);

    while (1)
    {
        // Infinite loop
    }
}

/**
 * I2C1 Initialization: PB8=SCL, PB9=SDA
 */
void I2C1_init(void)
{
    /* Enable GPIOB and I2C1 Clocks */
    RCC->AHB1ENR |= (1U << 1);   // Enable GPIOB clock
    RCC->APB1ENR |= (1U << 21);  // Enable I2C1 clock

    /* Configure PB8 (SCL) & PB9 (SDA) as Alternate Function (AF4 for I2C1) */
    GPIOB->AFR[1] &= ~((0xF << 0) | (0xF << 4));   // Clear AF
    GPIOB->AFR[1] |=  ((4 << 0) | (4 << 4));       // Set PB8, PB9 to AF4 (I2C1)

    /* Set PB8, PB9 to Alternate Function mode */
    GPIOB->MODER &= ~((3U << 16) | (3U << 18)); // Clear PB8, PB9 mode
    GPIOB->MODER |=  ((2U << 16) | (2U << 18)); // Set to Alternate Function mode

    /* Configure PB8, PB9 as open-drain with pull-ups */
    GPIOB->OTYPER |=  (1U << 8) | (1U << 9);    // Open-drain
    GPIOB->PUPDR  &= ~((3U << 16) | (3U << 18));// Clear pull-up/pull-down
    GPIOB->PUPDR  |=  ((1U << 16) | (1U << 18));// Enable pull-up

    /* Reset and Enable I2C1 */
    I2C1->CR1 |= (1U << 15);    // Software Reset
    I2C1->CR1 &= ~(1U << 15);   // Release from Reset

    /* Configure I2C1 for 100 kHz standard mode */
    I2C1->CR2   = 16U;    // Peripheral clock = 16 MHz
    I2C1->CCR   = 80U;    // 100 kHz clock
    I2C1->TRISE = 17U;    // max rise time
    I2C1->CR1  |= (1U << 0); // Enable I2C1
}

/**
 * I2C Burst Write: writes multiple bytes (n) to consecutive registers
 * in a device with I2C slave address saddr, starting at memory address maddr.
 */
void I2C1_burstWrite(uint8_t saddr, uint8_t maddr, int n, char *data)
{
    volatile int tmp;
    int i;

    /* 1) Wait until the bus is not busy */
    while (I2C1->SR2 & (1U << 1));

    /* 2) Generate Start Condition */
    I2C1->CR1 &= ~(1U << 11); // Disable POS (just in case)
    I2C1->CR1 |= (1U << 8);   // START
    while (!(I2C1->SR1 & (1U << 0)));  // Wait for START flag (SB=1)

    /* 3) Send Slave Address + Write (LSB=0) */
    I2C1->DR = (saddr << 1);  // SADDR+W
    while (!(I2C1->SR1 & (1U << 1)));  // Wait for ADDR=1
    tmp = I2C1->SR2;          // Clear ADDR flag

    /* 4) Send Memory/Register Address */
    while (!(I2C1->SR1 & (1U << 7)));  // Wait TXE=1 (data register empty)
    I2C1->DR = maddr;

    /* 5) Write All the Data Bytes */
    for (i = 0; i < n; i++)
    {
        while (!(I2C1->SR1 & (1U << 7))); // Wait TXE=1
        I2C1->DR = *data++;
    }

    /* 6) Wait for Byte Transfer to Finish (BTF=1) */
    while (!(I2C1->SR1 & (1U << 2)));

    /* 7) Generate Stop Condition */
    I2C1->CR1 |= (1U << 9);
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

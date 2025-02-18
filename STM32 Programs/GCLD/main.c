/**
 * p12_1.c - PCD8544 (Nokia 5110) GLCD via SPI with STM32F401RE
 *
 * Writes 'A', 'B', 'C' on the display at startup.
 *
 * Pin Connections (STM32F401RE):
 *   RST -> PB10
 *   CE  -> PA8  (Software SS)
 *   DC  -> PB6
 *   DIN -> PA7  (SPI1 MOSI)
 *   CLK -> PA5  (SPI1 SCK)
 *   VCC -> 3.3V
 *   GND -> GND
 *   LIGHT -> optional LED backlight
 *
 * Assumes 16 MHz system clock.
 */

#include "stm32f4xx.h"

/* Display size constants for the Nokia5110 GLCD */
#define GLCD_WIDTH  84
#define GLCD_HEIGHT 48

/* Prototypes */
void GLCD_init(void);
void GLCD_clear(void);
void GLCD_setCursor(uint8_t x, uint8_t y);
void GLCD_data_write(uint8_t data);
void GLCD_command_write(uint8_t data);
void GLCD_putchar(int c);

void SPI_init(void);
void SPI_write(uint8_t data);

/* Sample font table for letters A, B, C */
const uint8_t font_table[][6] =
{
    {0x7E, 0x11, 0x11, 0x11, 0x7E, 0}, /* A */
    {0x7F, 0x49, 0x49, 0x49, 0x36, 0}, /* B */
    {0x3E, 0x41, 0x41, 0x41, 0x22, 0}  /* C */
};

int main(void)
{
    GLCD_init();   // Init the PCD8544 controller
    GLCD_clear();  // Clear display & home cursor

    /* Write letters A, B, C */
    GLCD_putchar(0); // 'A'
    GLCD_putchar(1); // 'B'
    GLCD_putchar(2); // 'C'

    while (1)
    {
        // Infinite loop
    }
}

/**
 * Write a character index (0..2 => A,B,C) to the display
 */
void GLCD_putchar(int c)
{
    int i;
    for (i = 0; i < 6; i++)
    {
        GLCD_data_write(font_table[c][i]);
    }
}

/**
 * Set cursor position on the display
 * x => 0..83 (columns), y => 0..5 (banks of 8 rows => 48/8=6)
 */
void GLCD_setCursor(uint8_t x, uint8_t y)
{
    GLCD_command_write(0x80 | x);  // column
    GLCD_command_write(0x40 | y);  // bank (8-pixel rows)
}

/**
 * Clear the entire display by writing zeros
 * Then set cursor to home (0,0)
 */
void GLCD_clear(void)
{
    int32_t index;
    for (index = 0; index < (GLCD_WIDTH * GLCD_HEIGHT / 8); index++)
    {
        GLCD_data_write(0x00);
    }
    GLCD_setCursor(0, 0);
}

/**
 * Initialize the GLCD (PCD8544)
 * - SPI1 for data
 * - PB6 => DC, PB10 => RST, PA8 => CE
 */
void GLCD_init(void)
{
    SPI_init();

    /* PB6 => DC, PB10 => RESET */
    RCC->AHB1ENR |= (1U << 1);        // Enable GPIOB clock
    GPIOB->MODER &= ~0x00303000;      // Clear PB6/PB10 modes
    GPIOB->MODER |=  0x00101000;      // PB6,PB10 => output

    /* Hardware reset of GLCD controller (PCD8544) */
    GPIOB->BSRR = (1U << (10 + 16));  // assert RESET (PB10=0)
    GPIOB->BSRR = (1U << 10);        // deassert RESET (PB10=1)

    /* Extended command mode for config */
    GLCD_command_write(0x21); // extended command
    GLCD_command_write(0xB8); // LCD Vop (contrast)
    GLCD_command_write(0x04); // temp coefficient
    GLCD_command_write(0x14); // bias mode 1:48

    /* Normal command mode */
    GLCD_command_write(0x20); // standard commands
    GLCD_command_write(0x0C); // normal display mode
}

/**
 * Write a byte to the GLCD's data register
 */
void GLCD_data_write(uint8_t data)
{
    /* DC => 1 => data register */
    GPIOB->BSRR = (1U << 6);  // PB6=1 => data mode
    SPI_write(data);
}

/**
 * Write a byte to the GLCD's command register
 */
void GLCD_command_write(uint8_t data)
{
    /* DC => 0 => command register */
    GPIOB->BSRR = (1U << (6 + 16));  // PB6=0 => command mode
    SPI_write(data);
}

/**
 * Initialize SPI1:
 * - SCK => PA5
 * - MOSI => PA7
 * - SW-based SS => PA8
 * 8-bit, CPOL=0, CPHA=0, master mode
 */
void SPI_init(void)
{
    /* 1) Enable GPIOA clock & SPI1 clock */
    RCC->AHB1ENR |= (1U << 0);   // GPIOA
    RCC->APB2ENR |= (1U << 12);  // SPI1

    /* 2) Configure PA5 (SCK), PA7 (MOSI) as AF5 */
    GPIOA->AFR[0] &= ~0xF0F00000;
    GPIOA->AFR[0] |=  0x50500000;   // AF5 for SCK & MOSI

    /* SCK => PA5, MOSI => PA7 in alt func */
    GPIOA->MODER &= ~0x0000CC00;  // Clear mode for A5,A7
    GPIOA->MODER |=  0x00008800;  // Alt func

    /* 3) PA8 => Software CE => Output */
    GPIOA->MODER &= ~(3U << 16);
    GPIOA->MODER |=  (1U << 16);  // PA8 => output

    /* 4) Configure SPI1 in Master, CPOL=0, CPHA=0, 8-bit, fPCLK/16 => 1 MHz if PCLK=16 MHz */
    SPI1->CR1 = 0x31C;  // BR=011 => /16, MSTR=1, CPOL=0, CPHA=0, 8-bit
    SPI1->CR2 = 0;
    SPI1->CR1 |= (1U << 6); // SPE=1 => enable SPI
}

/**
 * Write one byte via SPI1
 */
void SPI_write(uint8_t data)
{
    /* assert CE => 0 => active low */
    GPIOA->BSRR = (1U << (8 + 16)); // PA8=0

    /* wait until TXE=1 => shift reg empty */
    while (!(SPI1->SR & 2)) {}
    SPI1->DR = data; 

    /* wait until BSY=0 => done transmitting */
    while (SPI1->SR & (1U << 7)) {}

    /* deassert CE => 1 */
    GPIOA->BSRR = (1U << 8);
}

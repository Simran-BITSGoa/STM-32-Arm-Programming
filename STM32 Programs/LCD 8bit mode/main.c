/**
 * p3_2.c (Adapted for STM32F401RE @ 16 MHz)
 *
 * This program initializes a standard 16x2 (HD44780-type) LCD in 8-bit
 * mode and repeatedly displays "Hello". The LCD is cleared after a delay,
 * and the cycle repeats.
 *
 * Pin Assignments:
 *   - PC0..PC7 => LCD D0..D7
 *   - PB5      => LCD RS
 *   - PB6      => LCD R/W
 *   - PB7      => LCD EN
 *
 * Make sure the MCU system clock is truly 16 MHz (e.g., HSI @ 16 MHz).
 */

#include "stm32f4xx.h"  // or "stm32f401xe.h" depending on your CMSIS setup

/* Function Prototypes */
void LCD_init(void);
void PORTS_init(void);
void LCD_command(unsigned char cmd);
void LCD_command_noPoll(unsigned char cmd);
void LCD_data(char data);
void LCD_ready(void);
void delayMs(int n);

/* Define control pins for PB5..7 */
#define RS 0x20  /* PB5 mask for Register Select (1<<5) */
#define RW 0x40  /* PB6 mask for Read/Write (1<<6) */
#define EN 0x80  /* PB7 mask for Enable (1<<7) */

int main(void)
{
    LCD_init();  // Initialize LCD

    while (1)
    {
        // Write "Hello" on LCD
        LCD_data('H');
        LCD_data('e');
        LCD_data('l');
        LCD_data('l');
        LCD_data('o');

        delayMs(500);

        // Clear LCD display (command 0x01)
        LCD_command(0x01);

        delayMs(500);
    }
}

/**
 * Initialize the LCD controller: 8-bit mode, 2 lines, 5x7 chars, etc.
 */
void LCD_init(void)
{
    PORTS_init();

    /* LCD power-on initialization sequence typically requires
       some 'no-poll' commands before the busy flag is reliable. */
    delayMs(30);               // Wait 30ms after power rises
    LCD_command_noPoll(0x30);  // Function set (8-bit) -- no poll
    delayMs(10);
    LCD_command_noPoll(0x30);  // Repeat
    delayMs(1);
    LCD_command_noPoll(0x30);  // Now we can poll busy flag

    // Now the LCD is ready to accept normal commands
    LCD_command(0x38);  // 8-bit, 2 line, 5x7 font
    LCD_command(0x06);  // Move cursor right after each char
    LCD_command(0x01);  // Clear screen, cursor to home
    LCD_command(0x0F);  // Display on, cursor on, blinking
}

/**
 * Enable clocks to GPIOB (control) and GPIOC (data).
 * Set PB5..PB7 to output. Set PC0..PC7 to output.
 */
void PORTS_init(void)
{
    /* Enable clock for GPIOB & GPIOC */
    RCC->AHB1ENR |= (1 << 1) | (1 << 2);  // bits 1=>GPIOB, 2=>GPIOC

    /* PB5 (RS), PB6 (R/W), PB7 (EN) as outputs */
    GPIOB->MODER &= ~0x0000FC00;  // clear PB5..PB7 mode
    GPIOB->MODER |=  0x00005400;  // set PB5..PB7 as outputs (01)

    // Make sure EN and R/W are low
    GPIOB->BSRR = (EN | RW) << 16;

    /* PC0..PC7 as outputs for LCD data */
    GPIOC->MODER &= ~0x0000FFFF;  // clear PC0..PC7
    GPIOC->MODER |=  0x00005555;  // set PC0..PC7 as outputs
}

/**
 * Wait until the LCD controller is not busy (D7=0).
 * This requires reading the LCD status, so we switch data pins to input.
 */
void LCD_ready(void)
{
    char status;

    // 1) Make PC0..PC7 inputs
    GPIOC->MODER &= ~0x0000FFFF;  // set PC0..PC7 to input (00)

    // RS=0 (status), R/W=1 (read)
    GPIOB->BSRR = RS << 16;  // RS = 0
    GPIOB->BSRR = RW;        // RW = 1

    do {
        // EN=1
        GPIOB->BSRR = EN;
        delayMs(0);  // small timing gap

        // Read status from PC0..PC7
        status = (char)(GPIOC->IDR & 0xFF);

        // EN=0
        GPIOB->BSRR = EN << 16;
        delayMs(0);
    }
    while (status & 0x80); // Check busy flag (bit7)

    // 2) Return data pins to output mode
    GPIOB->BSRR = RW << 16; // R/W=0
    GPIOC->MODER &= ~0x0000FFFF;
    GPIOC->MODER |=  0x00005555;
}

/**
 * Send a command (8-bit) to the LCD, after polling BF.
 */
void LCD_command(unsigned char cmd)
{
    LCD_ready();                 // Wait until not busy

    // RS=0, R/W=0
    GPIOB->BSRR = (RS | RW) << 16;

    // Put command on data pins
    GPIOC->ODR = (GPIOC->ODR & ~0xFF) | cmd;

    // Pulse EN high
    GPIOB->BSRR = EN;
    delayMs(0);
    GPIOB->BSRR = EN << 16;
}

/**
 * Send a command (8-bit) to the LCD, without polling BF (for early init).
 */
void LCD_command_noPoll(unsigned char cmd)
{
    // RS=0, R/W=0
    GPIOB->BSRR = (RS | RW) << 16;

    // Put command on data pins
    GPIOC->ODR = (GPIOC->ODR & ~0xFF) | cmd;

    // Pulse EN high
    GPIOB->BSRR = EN;
    delayMs(0);
    GPIOB->BSRR = EN << 16;
}

/**
 * Write a single character (8-bit) to the LCD.
 */
void LCD_data(char data)
{
    LCD_ready();  // Wait until not busy

    // RS=1 (data), R/W=0 (write)
    GPIOB->BSRR = RS;
    GPIOB->BSRR = RW << 16;

    // Put data on data pins
    GPIOC->ODR = (GPIOC->ODR & ~0xFF) | data;

    // Pulse EN high
    GPIOB->BSRR = EN;
    delayMs(0);
    GPIOB->BSRR = EN << 16;
}

/**
 * Approximate delay in ms for a ~16 MHz clock using a simple loop.
 * Originally ~3195 loops for 1 ms at 16 MHz.
 *
 * For more accurate or non-blocking timing, use SysTick or a TIM peripheral.
 */
void delayMs(int n)
{
    volatile int i;
    for (; n > 0; n--)
    {
        for (i = 0; i < 3195; i++) {
            __NOP();
        }
    }
}

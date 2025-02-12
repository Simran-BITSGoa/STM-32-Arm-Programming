/**
 * p3_5.c (Adapted for STM32F401RE @ 16 MHz)
 *
 * This program scans a 4x4 matrix keypad and returns a unique
 * number for each key pressed (1..16). The built-in LED (on PA5)
 * blinks that number of times for each key press.
 *
 * Pin Connections (Nucleo-F401RE):
 *   - PC0..PC3 => Keypad Columns
 *   - PC4..PC7 => Keypad Rows
 *   - PA5      => On-board LED (LD2)
 *
 * Make sure the system clock is actually running at 16 MHz:
 *   - If you're using the default Nucleo-F401RE settings, you
 *     likely have 84 MHz from the PLL. Adjust your startup code
 *     or clock config accordingly if you want 16 MHz operation.
 */

#include "stm32f4xx.h"  // or "stm32f401xe.h" depending on your environment

void delay(void);
void delayMs(int n);
void keypad_init(void);
char keypad_getkey(void);
void LED_init(void);
void LED_blink(int value);

int main(void) {
    unsigned char key;

    keypad_init();
    LED_init();

    while (1) {
        key = keypad_getkey(); /* read the keypad */
        if (key != 0) {
            LED_blink(key);    /* blink LED 'key' times */
        }
    }
}

/**
 * Initializes PC0..PC3 (columns) and PC4..PC7 (rows).
 * Columns have pull-up resistors enabled, so they read '1' when no key is pressed.
 */
void keypad_init(void) {
    /* Enable GPIOC clock (bit 2 => GPIOC) */
    RCC->AHB1ENR |= (1UL << 2);

    /* Set PC0..PC7 to input mode initially */
    GPIOC->MODER  &= ~0x0000FFFF;    // Clear pin modes [PC0..PC7]

    /* Enable pull-up resistors for columns (PC0..PC3).
       Rows (PC4..PC7) will be driven as outputs one at a time. */
    GPIOC->PUPDR   =  0x00000055;    // Pull-up on PC0..PC3 (0101 0101)
}

/**
 * Non-blocking function to read the keypad.
 * If a key is pressed, returns a unique code (1..16).
 * If no key is pressed, returns 0.
 */
char keypad_getkey(void) {
    int row, col;

    /* Each element in row_mode[] sets exactly one of PC4..PC7 to output mode.
       row_low[] drives that row low, row_high[] drives it high again. */
    const int row_mode[] = {
        0x00000100,  // PC4 as output
        0x00000400,  // PC5 as output
        0x00001000,  // PC6 as output
        0x00004000   // PC7 as output
    };
    const int row_low[] = {
        0x00100000,  // drive PC4 low
        0x00200000,  // drive PC5 low
        0x00400000,  // drive PC6 low
        0x00800000   // drive PC7 low
    };
    const int row_high[] = {
        0x00000010,  // drive PC4 high
        0x00000020,  // drive PC5 high
        0x00000040,  // drive PC6 high
        0x00000080   // drive PC7 high
    };

    /* 1) Check if any key is pressed: drive all rows low, read columns. */
    GPIOC->MODER = 0x00005500;    // PC4..PC7 => output
    GPIOC->BSRR  = 0x00F00000;    // drive PC4..PC7 low (bits 20..23)
    delay();                      // short wait

    col = GPIOC->IDR & 0x000F;    // read PC0..PC3
    GPIOC->MODER &= ~0x0000FF00;  // disable row drive (PC4..PC7 => input)

    if (col == 0x000F) {
        // all columns = 1 => no key pressed
        return 0;
    }

    /* 2) A key is pressed. Activate one row at a time to find which. */
    for (row = 0; row < 4; row++) {
        // Disable all row pins drive:
        GPIOC->MODER &= ~0x0000FF00;

        // Enable only the current row pin as output:
        GPIOC->MODER |= row_mode[row];

        // Drive that row low:
        GPIOC->BSRR = row_low[row];
        delay(); // wait for signals

        // Read columns again:
        col = GPIOC->IDR & 0x000F;

        // Drive row high before disabling
        GPIOC->BSRR = row_high[row];

        // If any column is 0 => the pressed key is in this row
        if (col != 0x000F) {
            break;
        }
    }

    // Optionally drive all rows high again
    GPIOC->BSRR = 0x000000F0;
    GPIOC->MODER &= ~0x0000FF00;

    if (row == 4) {
        // No key found
        return 0;
    }

    /* 3) Determine which column is pressed. Return unique code 1..16. */
    if (col == 0x000E) return (row * 4 + 1);  // column 0
    if (col == 0x000D) return (row * 4 + 2);  // column 1
    if (col == 0x000B) return (row * 4 + 3);  // column 2
    if (col == 0x0007) return (row * 4 + 4);  // column 3

    return 0; // safety fallback
}

/**
 * Initialize PA5 as output (Nucleo-F401RE on-board LED).
 */
void LED_init(void) {
    /* Enable GPIOA clock (bit 0 => GPIOA) */
    RCC->AHB1ENR |= (1UL << 0);

    /* Set PA5 to output mode */
    GPIOA->MODER &= ~0x00000C00;  // clear bits for PA5
    GPIOA->MODER |=  0x00000400;  // set PA5 to output
}

/**
 * Blink on-board LED (PA5) "value" times, mod 16.
 */
void LED_blink(int value) {
    value &= 0x0F;  // mod 16

    for (; value > 0; value--) {
        // Turn LED on
        GPIOA->BSRR = (1UL << 5);
        delayMs(200);

        // Turn LED off
        GPIOA->BSRR = (1UL << (5 + 16));
        delayMs(200);
    }
    // Small pause before next event
    delayMs(200);
}

/**
 * Very short delay (non-accurate) to let signals settle.
 */
void delay(void) {
    volatile int i;
    for (i = 0; i < 20; i++) {
        __NOP();
    }
}

/**
 * Approximate millisecond delay for a 16 MHz system clock.
 * Original code used 3195 loops for ~1 ms. 
 * You can adjust if desired.
 */
void delayMs(int n) {
    volatile int i;
    for (; n > 0; n--) {
        for (i = 0; i < 3195; i++) {
            __NOP();
        }
    }
}

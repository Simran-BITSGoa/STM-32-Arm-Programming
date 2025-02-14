/**
 * p7_2.c - STM32F401RE On-Chip Temperature Sensor with USART2
 *
 * This program reads the internal temperature sensor (ADC1 Channel 18)
 * and prints the temperature (°C) over USART2 (9600 baud).
 *
 * **Hardware Configuration:**
 * - **TIM2 CH2** triggers ADC1 conversion every **1 second**.
 * - **ADC1 Channel 18** measures **internal temperature sensor**.
 * - **USART2 (PA2)** sends temperature data to **Tera Term / Serial Monitor**.
 *
 * **Formula (from STM32F4 Reference Manual):**
 *   Temperature (°C) = { (VSENSE – V25) / Avg_Slope } + 25
 *   - V25 = 0.76V
 *   - Avg_Slope = 2.5mV/°C
 */

#include "stm32f4xx.h"
#include <stdio.h>

void USART2_init(void);
int USART2_write(int c);
void delayMs(int n);

int main(void)
{
    int data;
    double voltage, temperature;

    /* === 1) Enable GPIOA Clock === */
    RCC->AHB1ENR |= (1U << 0);  // Enable GPIOA clock

    /* === 2) Configure TIM2 to Trigger ADC1 at 1 Hz === */
    RCC->APB1ENR |= (1U << 0);  // Enable TIM2 clock
    TIM2->PSC = 1600 - 1;        // Divide 16 MHz by 1600 ? 10 kHz timer clock
    TIM2->ARR = 10000 - 1;       // Divide 10 kHz by 10000 ? 1 Hz ADC trigger
    TIM2->CNT = 0;               // Clear counter
    TIM2->CCMR1 = 0x00006800;    // PWM1 mode, preload enable
    TIM2->CCER = 0x0010;         // CH2 enable
    TIM2->CCR2 = 50 - 1;         // Set duty cycle
    TIM2->CR1 = 1;               // Enable TIM2

    /* === 3) Configure ADC1 for Internal Temperature Sensor (Channel 18) === */
    RCC->APB2ENR |= (1U << 8);   // Enable ADC1 clock
    ADC->CCR |= (1U << 23);      // Enable temperature sensor
    ADC->CCR &= ~(1U << 22);     // Disable VBAT sensor (required)
    
    ADC1->SMPR1 = (3U << 24);    // Sampling time (15 cycles min)
    ADC1->SQR3 = 18;             // ADC1 Channel 18 (Temperature sensor)
    ADC1->CR2 = (3U << 24);      // Trigger: EXTSEL=3 (TIM2 CH2), EXTEN=11 (rising edge)
    ADC1->CR2 |= 1;              // Enable ADC1

    /* === 4) Initialize USART2 for Serial Output (9600 baud) === */
    USART2_init();
    printf("STM32F401RE Internal Temperature Sensor (ADC1 CH18)\r\n");

    /* === 5) Main Loop: Read ADC & Print Temperature === */
    while (1)
    {
        while (!(ADC1->SR & (1U << 1))) {}  // Wait for ADC conversion (EOC flag)
        
        data = ADC1->DR;  // Read ADC result
        
        // Convert ADC value to voltage
        voltage = (double)data / 4095 * 3.3;

        // Convert voltage to temperature (°C)
        temperature = ((voltage - 0.76) / 0.0025) + 25;

        // Print result to USART2
        printf("ADC: %d, Temp: %.2f°C\r\n", data, temperature);
        
        delayMs(1000);  // 1-second delay for stability
    }
}

/**
 * Initialize USART2 for 9600 baud serial output
 */
void USART2_init(void)
{
    /* Enable GPIOA & USART2 Clocks */
    RCC->AHB1ENR |= (1U << 0);    // Enable GPIOA clock
    RCC->APB1ENR |= (1U << 17);   // Enable USART2 clock

    /* Configure PA2 (USART2 TX) */
    GPIOA->MODER &= ~(3U << 4);  // Clear PA2 mode
    GPIOA->MODER |= (2U << 4);   // Set PA2 to Alternate Function (AF7)
    GPIOA->AFR[0] |= (7U << 8);  // Set PA2 to AF7 (USART2_TX)

    /* Configure USART2 */
    USART2->BRR = 0x0683;  // 9600 baud @ 16 MHz
    USART2->CR1 = (1U << 3) | (1U << 13);  // Enable TX & USART2
}

/**
 * Send a character over USART2
 */
int USART2_write(int ch)
{
    while (!(USART2->SR & (1U << 7))) {}  // Wait until TX buffer is empty
    USART2->DR = ch;
    return ch;
}

/**
 * Redirect printf() to USART2
 */
int fputc(int c, FILE *f)
{
    return USART2_write(c);
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

/**
 * p13_2.c - Acquire data from ADC by DMA (STM32F401RE)
 *
 * TIM2 channel 2 @1 kHz triggers ADC1 conversions on channel 0 (PA0),
 * data is collected by DMA2 stream0, then printed over USART2.
 *
 * System clock assumed 16 MHz.
 * 
 * Pins:
 *   - PA0 = ADC1 channel 0 (analog)
 *   - PB3 = TIM2_CH2 output (AF1)
 *   - PA2 = USART2_TX (AF7)
 */

#include "stm32f4xx.h"
#include <stdio.h>

#define ADCBUFSIZE    64

void USART2_init(void);
void DMA2_init(void);
void DMA2_Stream0_setup(uint32_t src, uint32_t dst, int len);
void TIM2_init(void);
void ADC1_init(void);

/* Buffers for ADC data & ASCII output */
uint8_t  adcbuf[ADCBUFSIZE];  
char     uartbuf[ADCBUFSIZE * 5]; 

volatile int done = 1;  // Flag from DMA ISR

int main(void)
{
    int i;
    char *p;

    USART2_init();   // For printing results
    DMA2_init();     // DMA2 clock & interrupt
    TIM2_init();     // 1 kHz trigger
    ADC1_init();     // 8-bit, triggered by TIM2 CH2

    while (1)
    {
        done = 0; // Clear done flag

        // Start a DMA -> ADC data transfer
        DMA2_Stream0_setup((uint32_t)adcbuf, (uint32_t)&(ADC1->DR), ADCBUFSIZE);

        // Wait for buffer to fill
        while (!done) {}

        // Convert the ADC data into decimal ASCII
        p = uartbuf;
        for (i = 0; i < ADCBUFSIZE; i++)
        {
            // Each sample is 8-bit => 0..255
            sprintf(p, "%3d ", adcbuf[i]);
            p += 4; // 4 chars per sample: e.g. "255 "
        }

        // Print results over USART2
        for (i = 0; i < (p - uartbuf); i++)
        {
            while (!(USART2->SR & (1U << 6))) {} // Wait for TXE or TC
            USART2->DR = uartbuf[i];
        }
    }
}

/**
 * Initialize ADC1:
 *  - PA0 -> ADC1 channel 0 (analog)
 *  - 8-bit resolution
 *  - Trigger = TIM2 CH2 rising edge
 */
void ADC1_init(void)
{
    /* 1) Enable GPIOA clock, set PA0 to analog */
    RCC->AHB1ENR |= (1U << 0);   // GPIOA clock
    GPIOA->MODER |= (3U << 0);   // PA0 in analog mode

    /* 2) Enable ADC1 clock */
    RCC->APB2ENR |= (1U << 8);   // ADC1EN

    /* 3) Configure 8-bit resolution & external trigger */
    ADC1->CR1 = 0x02000000;       // 8-bit (RES=2 => 0x2000000)
    // CR2: EXTEN=01(rising edge), EXTSEL=011 => TIM2 CH2
    ADC1->CR2 = 0x13000000;       
    ADC1->CR2 |= (1U << 10);      // EOCS=1 => EOC after each conversion

    /* 4) Enable ADC1 */
    ADC1->CR2 |= (1U << 0);       // ADON=1
}

/**
 * Initialize TIM2 channel 2 output @1 kHz,
 * used to trigger the ADC conversions.
 *  - PB3 => TIM2_CH2 (AF1)
 */
void TIM2_init(void)
{
    /* 1) Enable GPIOB clock, set PB3 to AF1 */
    RCC->AHB1ENR |= (1U << 1);   // GPIOB clock
    GPIOB->MODER |= (2U << 6);   // PB3 as Alternate Function
    GPIOB->AFR[0] |= (1U << 12); // PB3 => AF1 (TIM2_CH2)

    /* 2) Enable TIM2 clock */
    RCC->APB1ENR |= (1U << 0);

    /* 3) Setup TIM2 => 1 kHz 
     * PSC=160 => /160 => 100kHz
     * ARR=100 => /100 => 1kHz
     */
    TIM2->PSC  = 160 - 1;  
    TIM2->ARR  = 100 - 1;  
    TIM2->CNT  = 0;

    /* 4) PWM1 mode on CH2 => toggling or PWM for trigger signal */
    TIM2->CCMR1 = 0x6800; // PWM1 mode, preload
    TIM2->CCER  = 0x10;   // CH2 enable
    TIM2->CCR2  = 50 - 1; // 50% duty
}

/**
 * Initialize DMA2:
 *  - Enable clock
 *  - Enable Stream0 interrupt
 */
void DMA2_init(void)
{
    RCC->AHB1ENR |= (1U << 22); // DMA2EN
    DMA2->HIFCR   = 0x003F;     // Clear all Stream0 flags
    NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

/**
 * Setup DMA2 Stream0 for ADC1:
 *  - ADC1 => peripheral-to-memory
 *  - 8-bit data, increment memory
 *  - length=ADCBUFSIZE
 *  - triggered by EOC each time => buffer
 */
void DMA2_Stream0_setup(uint32_t dst, uint32_t src, int len)
{
    /* 1) Disable Stream0 */
    DMA2_Stream0->CR &= ~1U;
    while (DMA2_Stream0->CR & 1U) {} // wait for disable

    /* 2) Clear flags, set addresses, length */
    DMA2->HIFCR = 0x003F;
    DMA2_Stream0->PAR  = src;   // Peripheral = ADC1->DR
    DMA2_Stream0->M0AR = dst;   // Memory = adcbuf
    DMA2_Stream0->NDTR = len;

    /* 3) Channel0 => ADC1_0, 
     * DIR=Periph-to-Mem, 8-bit, mem incr,
     * Enable interrupts
     */
    DMA2_Stream0->CR  = 0x00000000; 
    DMA2_Stream0->CR |= 0x00000400; // PSIZE=8-bit, MSIZE=8-bit, MINC=1, DIR=Periph->Mem
    DMA2_Stream0->CR |= 0x16;       // TCIE, TEIE, DMEIE

    DMA2_Stream0->FCR = 0;         // Direct mode, no FIFO

    /* 4) Enable Stream0 */
    DMA2_Stream0->CR |= 1U;

    /* 5) Enable ADC-DMA transfer, start Timer2 */
    ADC1->CR2 |= (1U << 8); // DDS=1 => ADC continues DMA after each conversion if needed
    ADC1->CR2 |= (1U << 9); // EOC after each conversion => ensures each sample triggers DMA
    ADC1->CR2 |= (1U << 0); // ensure ADC is on
    TIM2->CR1   = 1;        // enable timer2
}

/**
 * DMA2 Stream0 IRQ Handler:
 * - If error, handle
 * - On completion, disable Stream0, clear flags, disable triggers
 */
void DMA2_Stream0_IRQHandler(void)
{
    /* Check error flags if needed (TE, DME) */
    if (DMA2->HISR & 0x000C)
    {
        // handle errors
        while (1) {}
    }

    /* disable DMA2 Stream0 */
    DMA2_Stream0->CR = 0;
    DMA2_Stream0->M0AR = 0;
    DMA2_Stream0->PAR  = 0;

    /* clear flags */
    DMA2->LIFCR = 0x003F;

    /* disable ADC's DMA trigger, disable Timer2 */
    ADC1->CR2 &= ~(1U << 8); // DDS=0
    ADC1->CR2 &= ~(1U << 9); // no DMA
    TIM2->CR1 &= ~1U;        // disable Timer2

    /* set done flag */
    done = 1;
}

/**
 * Initialize USART2 @ 9600, PA2 as TX
 */
void USART2_init(void)
{
    /* 1) Enable clocks */
    RCC->AHB1ENR  |= (1U << 0);   // GPIOA
    RCC->APB1ENR  |= (1U << 17);  // USART2

    /* 2) PA2 => USART2_TX (AF7) */
    GPIOA->AFR[0] &= ~(0xF << 8); // clear alt
    GPIOA->AFR[0] |=  (7U << 8);  // AF7
    GPIOA->MODER  &= ~(3U << (2*2));
    GPIOA->MODER  |=  (2U << (2*2));

    /* 3) Configure USART2 => 9600 8N1 */
    USART2->BRR  = 0x0683;  // 9600@16MHz
    USART2->CR1  = 0x0008;  // TE=1
    USART2->CR2  = 0x0000;  // 1 stop bit
    USART2->CR3  = 0x0000;  // no flow
    USART2->CR1 |= 0x2000;  // UE=1
}

/**
 * p13_1.c - Send a string to USART2 via DMA (STM32F401RE)
 *
 * On STM32F401RE, USART2_TX is on PA2 (AF7).
 * DMA1, Stream 6, Channel 4 is used for USART2_TX.
 *
 * Baud Rate: 9600, System Clock: 16 MHz.
 */

#include "stm32f4xx.h"

void USART2_init(void);
void DMA1_init(void);
void DMA1_Stream6_setup(unsigned int src, unsigned int dst, int len);

volatile int done = 1;  // Flag to indicate DMA completion

int main(void)
{
    char alphabets[] = "abcdefghijklmnopqrstuvwxyz";
    char message[80];
    int size = sizeof(alphabets) - 1; // minus 1 if you don't want the null terminator
    int i;

    USART2_init();
    DMA1_init();

    while (1)
    {
        /* Prepare the message for transfer */
        for (i = 0; i < size; i++)
            message[i] = alphabets[i];

        /* Send the message out by USART2 using DMA */
        while (!done) {}  // wait until DMA data transfer is done
        done = 0;         // clear done flag

        DMA1_Stream6_setup((unsigned int)message, (unsigned int)&USART2->DR, size);
    }
}

/**
 * Initialize USART2: 
 * - PA2 as TX (AF7) 
 * - 9600 baud at 16 MHz 
 * - 8-bit data, no parity, 1 stop bit
 */
void USART2_init(void)
{
    /* 1) Enable GPIOA clock & USART2 clock */
    RCC->AHB1ENR  |= (1U << 0);    // GPIOA clock
    RCC->APB1ENR  |= (1U << 17);   // USART2 clock

    /* 2) Configure PA2 as USART2_TX (AF7) */
    GPIOA->AFR[0] &= ~(0xF << 8);   // clear alt func for PA2
    GPIOA->AFR[0] |=  (7 << 8);     // AF7 for USART2
    GPIOA->MODER  &= ~(3U << (2 * 2));   // clear mode for PA2
    GPIOA->MODER  |=  (2U << (2 * 2));   // set alt func for PA2

    /* 3) Configure USART2 for 9600 baud, 8N1 */
    USART2->BRR  = 0x0683;   // 9600 @16 MHz
    USART2->CR1  = 0x0008;   // TE=1, 8-bit data
    USART2->CR2  = 0x0000;   // 1 stop bit
    USART2->CR3  = 0x0000;   // no flow control
    USART2->CR1 |= 0x2000;   // UE=1, enable USART2

    /* 4) Clear TC flag, enable TC interrupt */
    USART2->SR &= ~0x40;     // clear TC flag
    USART2->CR1 |= 0x0040;   // enable transmit-complete interrupt
    NVIC_EnableIRQ(USART2_IRQn);
}

/**
 * Initialize DMA1: 
 * - Enable clock & set up NVIC interrupt
 */
void DMA1_init(void)
{
    RCC->AHB1ENR |= (1U << 21); // DMA1 clock enable
    DMA1->HIFCR   = 0x003F0000; // clear all Stream6 interrupt flags
    NVIC_EnableIRQ(DMA1_Stream6_IRQn);
}

/**
 * Configure DMA1 Stream6 for USART2_TX:
 *  - Stream6, Channel4 for USART2_TX
 *  - Mem-to-Peripheral, 8-bit data, memory increment
 *  - Enable interrupts
 *  - Start the DMA & enable USART2 DMA TX
 */
void DMA1_Stream6_setup(unsigned int src, unsigned int dst, int len)
{
    /* 1) Disable DMA1 Stream6 */
    DMA1_Stream6->CR &= ~1;
    while (DMA1_Stream6->CR & 1) {} // wait for disable
    DMA1->HIFCR = 0x003F0000;       // clear all interrupt flags for Stream6

    /* 2) Set up addresses, number of data items */
    DMA1_Stream6->PAR  = dst;  // Peripheral Addr = USART2->DR
    DMA1_Stream6->M0AR = src;  // Memory source
    DMA1_Stream6->NDTR = len;  // Number of data items

    /* 3) Configure DMA channel selection & direction:
     *  - Channel4 for USART2_TX
     *  - DIR=Memory-to-Peripheral
     *  - Mem increment, 8-bit data sizes
     *  - Transfer complete & error interrupts
     */
    DMA1_Stream6->CR = 0x08000000; // Channel4
    DMA1_Stream6->CR |= 0x00000440; // Mem-to-Peripheral, mem incr, 8-bit
    DMA1_Stream6->CR |= 0x16;       // enable interrupts (TCIE, TEIE, DMEIE)

    /* 4) No FIFO, direct mode */
    DMA1_Stream6->FCR = 0;

    /* 5) Enable DMA1 Stream6 */
    DMA1_Stream6->CR |= 1;

    /* 6) Clear TX Complete & enable USART2 TX DMA */
    USART2->SR  &= ~0x0040;  // clear TC
    USART2->CR3 |=  0x0080;  // DMAT=1, enable transmitter DMA
}

/**
 * DMA1 Stream6 Interrupt Handler
 * Clears interrupt flags. If error, handle accordingly.
 */
void DMA1_Stream6_IRQHandler(void)
{
    /* Check error flags in HISR/HIFCR if needed */
    if (DMA1->HISR & 0x000C0000) // TE or DME error
    {
        // handle error...
        while (1) {}
    }

    /* Clear all interrupt flags of Stream6 */
    DMA1->HIFCR = 0x003F0000;

    /* Disable Stream6's TC interrupt if desired */
    DMA1_Stream6->CR &= ~0x10;
}

/**
 * USART2 Interrupt Handler
 * - On Transmit Complete, set 'done' flag
 */
void USART2_IRQHandler(void)
{
    USART2->SR  &= ~0x0040; // clear TC flag
    done         = 1;       // signal data transfer done
}

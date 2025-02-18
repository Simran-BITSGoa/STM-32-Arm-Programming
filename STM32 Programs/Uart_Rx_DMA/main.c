#include "stm32f4xx.h"
#include <stdio.h>

#define RX_SIZE 16

void USART2_init(void);
void DMA1_init(void);
void DMA1_Stream5_setup(uint32_t dst, uint32_t src, int len);

volatile int done = 0;
char rxBuf[RX_SIZE];
char printBuf[64];

/* Main - re-arms DMA for 16 bytes, waits for completion, prints the data. */
int main(void)
{
    int i;

    USART2_init();   // Enable USART2 @9600, 8N1, PA2=Tx, PA3=Rx
    DMA1_init();     // Enable DMA1 & interrupt

    while (1)
    {
        done = 0;
        /* (Re)arm the DMA to receive 16 bytes into rxBuf. */
        DMA1_Stream5_setup((uint32_t)rxBuf, (uint32_t)&USART2->DR, RX_SIZE);

        /* Wait until DMA receives 16 bytes. */
        while (!done) {}

        /* Simple echo logic: print the received data. */
        sprintf(printBuf, "Received: %s\r\n", rxBuf);

        /* Block until all printBuf is sent out, using a simple loop. */
        for (i = 0; printBuf[i] != '\0'; i++)
        {
            while (!(USART2->SR & (1 << 7))) {}  // Wait TXE=1
            USART2->DR = printBuf[i];
        }
    }
}

/* Configure USART2:
 * - PA2 = Tx (AF7), PA3 = Rx (AF7)
 * - 9600 baud @16MHz
 * - TE=1, RE=1
 * - Enable DMAR=1 for RX DMA
 */
void USART2_init(void)
{
    /* 1) Enable clocks: GPIOA, USART2 */
    RCC->AHB1ENR |= (1U << 0);    // GPIOA
    RCC->APB1ENR |= (1U << 17);   // USART2

    /* 2) PA2 => Tx, PA3 => Rx, both AF7 */
    // PA2
    GPIOA->AFR[0] &= ~(0xF << 8);
    GPIOA->AFR[0] |=  (7 << 8);     // AF7 for PA2
    GPIOA->MODER  &= ~(3U << (2 * 2));
    GPIOA->MODER  |=  (2U << (2 * 2));  // Alt func

    // PA3
    GPIOA->AFR[0] &= ~(0xF << 12);
    GPIOA->AFR[0] |=  (7 << 12);    // AF7 for PA3
    GPIOA->MODER  &= ~(3U << (3 * 2));
    GPIOA->MODER  |=  (2U << (3 * 2));  // Alt func

    /* 3) Configure USART2 for 9600 baud, 8N1, TE=1, RE=1 */
    USART2->BRR = 0x0683;      // 9600 @16MHz
    USART2->CR1 = (1U << 3)    // TE=1
                | (1U << 2);   // RE=1
    USART2->CR2 = 0x0000;
    USART2->CR3 = 0x0000;

    /* 4) Enable the USART & set DMAR=1 for RX DMA usage */
    USART2->CR1 |= (1U << 13); // UE=1
    USART2->CR3 |= (1U << 6);  // DMAR=1 => enable DMA for Rx
}

/* Initialize DMA1 & NVIC for Stream5 interrupts. */
void DMA1_init(void)
{
    RCC->AHB1ENR |= (1U << 21); // DMA1 clock enable

    // Clear any leftover flags
    DMA1->HIFCR = 0x003F0000;   // or 0x0F7D0F7D for all streams

    // Enable DMA1 Stream5 IRQ at NVIC
    NVIC_EnableIRQ(DMA1_Stream5_IRQn);
}

/* Set up DMA1 Stream5 for USART2_RX:
 * - Channel4 for USART2_RX
 * - Peripheral-to-memory
 * - 8-bit data, memory increment
 * - 'len' items => set NDTR
 */
void DMA1_Stream5_setup(uint32_t dst, uint32_t src, int len)
{
    /* 1) Disable Stream5 */
    DMA1_Stream5->CR &= ~1U;
    while (DMA1_Stream5->CR & 1U) {} // wait for disable

    /* 2) Clear flags & set addresses */
    DMA1->HIFCR = 0x003F0000;  // Clear Stream5 flags
    DMA1_Stream5->PAR  = src;  // Peripheral => USART2->DR
    DMA1_Stream5->M0AR = dst;  // Memory => rxBuf
    DMA1_Stream5->NDTR = len;  // # items

    /* 3) Channel4 => (bits 27..25=100), 
     * Dir=Periph->Mem, Mem increment, 8-bit
     * Enable interrupts => TCIE, TEIE, DMEIE
     */
    // Bits: CHSEL=4(<<25 => 100b), DIR=00 => P->M, MINC=1 => bit10, PSIZE=8-bit, MSIZE=8-bit => bits11..13=0, TEIE=1<<2, DMEIE=1<<1, TCIE=1<<4 => 0x16
    DMA1_Stream5->CR = 0; // Clear everything
    DMA1_Stream5->CR |= (4U << 25); // CHSEL=4 => for USART2_RX
    DMA1_Stream5->CR |= (1U << 10); // MINC=1
    // data sizes => default 8-bit => 0
    // TEIE=1<<2, DMEIE=1<<1 => combine them with TCIE=1<<4 => 0x16
    DMA1_Stream5->CR |= 0x16; // TEIE,DMEIE,TCIE
    // DIR=00 => peripheral to memory => default

    DMA1_Stream5->FCR = 0; // direct mode => no FIFO

    /* 4) Enable Stream5 */
    DMA1_Stream5->CR |= 1U; 
}

/* DMA1 Stream5 Interrupt => signals that 'len' bytes were received */
void DMA1_Stream5_IRQHandler(void)
{
    // Check if TE or DME error
    if (DMA1->HISR & 0x000C0000)
    {
        // handle error
        while (1) {}
    }

    // Clear flags
    DMA1->HIFCR = 0x003F0000;

    // Disable Stream5
    DMA1_Stream5->CR = 0;

    // Indicate done
    done = 1;
}

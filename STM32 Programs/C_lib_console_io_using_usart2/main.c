/* p4_5.c C library Console I/O using USART2 at 9600 Baud
*
* This program demonstrates the use of C library console I/O.
* The
functions fputc() and fgetc() are implemented using
* USART2_write() and USART2_read() for character I/O.
* In the fgetc(), the received character is echoed and if a '\r'
*
is received, a pair of '\r', '\n' is echoed.
* In fputc() and fgetc(), the file descripter is not checked. All
*
file I/O's are directed to the console.
*
* By default, the clock is running at 16 MHz.
* The USART is configured for 9600 Baud.
* PA2 - USART2 TX (AF7)
* PA3 - USART2 RX (AF7)
* Use Tera Term on the host PC to send keystrokes and observe the
display
* of the characters echoed.
*
* This program was tested with Keil uVision v5.24a with DFP v2.11.0
*/
#include "stm32F4xx.h"
#include <stdio.h>
void USART2_init(void);
void delayMs(int);
int USART2_write(int c);
int USART2_read(void);
int main(void) {
int n;
char str[80];
USART2_init();
printf("Test stdio library console I/O functions\r\n");
fprintf(stdout, " test for stdout\r\n");
fprintf(stderr, " test for stderr\r\n");
while (1) {
printf("please enter a number: ");
scanf("%d", &n);
printf("the number entered is: %d\r\n", n);
printf("please type a
character string: ");
gets(str);
printf("the character string entered is: ");
puts(str);
printf("\r\n");
}
}
/* initialize USART2 to transmit at 9600 Baud */
void USART2_init
(void) {
RCC->AHB1ENR |= 1; /* Enable GPIOA clock */
RCC->APB1ENR |= 0x20000; /* Enable USART2 clock */
/* Configure PA2, PA3 for USART2 TX, RX */
GPIOA->AFR[0] &= ~0xFF00;
GPIOA->AFR[0] |= 0x7700; /* alt7 for USART2 */
GPIOA->MODER &= ~0x00F0;
GPIOA->MODER |= 0x00A0; /* enable alt. function for PA2, PA3 */
USART2->BRR = 0x0683; /* 9600 baud @ 16 MHz */
USART2->CR1 = 0x000C;
/* enable Tx, Rx, 8-bit data */
USART2->CR2 = 0x0000; /* 1 stop bit
*/
USART2->CR3 = 0x0000; /* no flow control */
USART2->CR1 |= 0x2000;
/* enable USART2 */
}
/* Write a character to USART2 */
int USART2_write (int ch) {
while (!(USART2->SR & 0x0080)) {} // wait until Tx buffer empty
USART2->DR = (ch & 0xFF);
return ch;
}
/* Read a character from USART2 */
int USART2_read(void) {
while (!(USART2->SR & 0x0020)) {} // wait until char arrives
return USART2->DR;
}
/* The code below is the interface to the C standard I/O library.
*
All the I/O are directed to the console, which is UART3.
*/
struct __FILE { int handle; };
FILE __stdin = {0};
FILE __stdout = {1};
FILE __stderr = {2};
/* Called by C library console/file input
* This function echoes the character received.
* If the character is
'\r', it is substituted by '\n'.
*/
int fgetc(FILE *f) {
int c;
c = USART2_read(); /* read the character from console */
if (c == '\r') { /* if '\r', after it is echoed, a '\n' is
appended*/
USART2_write(c); /* echo */
c = '\n';
}
USART2_write(c); /* echo */
return c;
}
/* Called by C library console/file output */
int fputc(int c, FILE *f) {
return USART2_write(c); /* write the character to console */
}
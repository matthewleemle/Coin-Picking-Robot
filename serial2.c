#include  "stm32f05xxx.h"

#define PCLK 48000000L

void initUART2(long int BaudRate)
{
	// Turn on the clock for GPIOA (usart 2 uses it)
	RCC_AHBENR  |= BIT17;
	
	//Configure PA2 (TXD for USART2, pin 8 in LQFP32 package)
	GPIOA_OSPEEDR  |= BIT6; // MEDIUM SPEED
	GPIOA_OTYPER   &= ~BIT2; // Push-pull
	GPIOA_MODER    |= BIT5; // AF-Mode
	GPIOA_AFRL     |= BIT8 ; // AF1 selected
	
	//Configure PA3 (RXD for USART2, pin 9 in LQFP32 package)
	GPIOA_MODER    |= BIT7; // AF-Mode
	GPIOA_AFRL     |= BIT12;  // AF1 selected
	
	RCC_APB1ENR    |= BIT17; // Turn on the clock for the USART2 peripheral

	USART2_CR1 |= (BIT2 | BIT3 ); // Enable Transmitter, Receiver.
	USART2_CR2 = 0x00000000;
	USART2_CR3 = 0x00000000;           
	USART2_BRR = PCLK / BaudRate;
	USART2_CR1 |= BIT0; // Enable Usart2
}

void putchar2(unsigned char c)
{
	USART2_TDR=c;
	while((USART2_ISR&BIT6)==0);  // Wait for transmission complete
}

unsigned char getchar2(void)
{
    unsigned char c;
    
    while((USART2_ISR&BIT5)==0);  // Wait for reception complete
    c=USART2_RDR;
    putchar2(c);
    return (c);
}

void send_string2 (unsigned char * buff)
{
	while(*buff)
	{
	    putchar2(*buff);
	    buff++;
	}
}

int get_string2 (unsigned char * buff, int max)
{
	unsigned int j;
	unsigned char c;
	
	for(j=0; j<(max-1); j++)
	{
		c=getchar2();
		buff[j]=c;
		if((c=='\n') || (c=='\r')) break;
	}
	buff[j]=0;
	return (j);
}


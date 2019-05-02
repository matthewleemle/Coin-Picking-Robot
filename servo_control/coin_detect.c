#include "stm32f05xxx.h"
#include "serial.h"

#define F_CPU 48000000L

void delay(int dly)
{
	while( dly--);
}

char HexDigit[]="0123456789ABCDEF";

void PrintNumber(int N, int Base, int digits)
{
	int j;
	#define NBITS 32
	char buff[NBITS+1];
	buff[NBITS]=0;

	j=NBITS-1;
	while ( (N>0) | (digits>0) )
	{
		buff[j--]=HexDigit[N%Base];
		N/=Base;
		if(digits!=0) digits--;
	}
	eputs(&buff[j+1]);
}

void wait_1ms(void)
{
	// For SysTick info check the STM32F0xxx Cortex-M0 programming manual page 85.
	STK_RVR = (F_CPU/1000L) - 1;  // set reload register, counter rolls over from zero, hence -1
	STK_CVR = 0; // load the SysTick counter
	STK_CSR = 0x05; // Bit 0: ENABLE, BIT 1: TICKINT, BIT 2:CLKSOURCE
	while((STK_CSR & BIT16)==0); // Bit 16 is the COUNTFLAG.  True when counter rolls over from zero.
	STK_CSR = 0x00; // Disable Systick counter
}

void waitms(int len)
{
	while(len--) wait_1ms();
}

#define PIN_PERIOD_18 (GPIOA_IDR&BIT8)

// GetPeriod() seems to work fine for frequencies between 300Hz and 600kHz.
long int GetPeriod (int n)
{
	int i;
	unsigned int saved_TCNT1a, saved_TCNT1b;

	STK_RVR = 0xffffff;  // 24-bit counter set to check for signal present
	STK_CVR = 0xffffff; // load the SysTick counter
	STK_CSR = 0x05; // Bit 0: ENABLE, BIT 1: TICKINT, BIT 2:CLKSOURCE
	while (PIN_PERIOD_18!=0) // Wait for square wave to be 0
	{
		if(STK_CSR & BIT16) return 0;
	}
	STK_CSR = 0x00; // Disable Systick counter

	STK_RVR = 0xffffff;  // 24-bit counter set to check for signal present
	STK_CVR = 0xffffff; // load the SysTick counter
	STK_CSR = 0x05; // Bit 0: ENABLE, BIT 1: TICKINT, BIT 2:CLKSOURCE
	while (PIN_PERIOD_18==0) // Wait for square wave to be 1
	{
		if(STK_CSR & BIT16) return 0;
	}
	STK_CSR = 0x00; // Disable Systick counter

	STK_RVR = 0xffffff;  // 24-bit counter reset
	STK_CVR = 0xffffff; // load the SysTick counter to initial value
	STK_CSR = 0x05; // Bit 0: ENABLE, BIT 1: TICKINT, BIT 2:CLKSOURCE
	for(i=0; i<n; i++) // Measure the time of 'n' periods
	{
		while (PIN_PERIOD_18!=0) // Wait for square wave to be 0
		{
			if(STK_CSR & BIT16) return 0;
		}
		while (PIN_PERIOD_18==0) // Wait for square wave to be 1
		{
			if(STK_CSR & BIT16) return 0;
		}
	}
	STK_CSR = 0x00; // Disable Systick counter

	return 0xffffff-STK_CVR;
}

void pin18period(void){
	long int count;
	float T, f;

		count=GetPeriod(100);
		if(count>0)
		{
			T=count/(F_CPU*100.0);
			f=1/T;
			/*
			eputs("f=");
			PrintNumber(f, 10, 7);
			eputs("Hz, count=");
			PrintNumber(count, 10, 6);
			eputs("\r");
			*/
		}
		else
		{
			eputs("NO SIGNAL                     \r");
			f= 0;
		}
		waitms(200);
		return f;
}

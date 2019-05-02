#include  "stm32f05xxx.h"

void initADC(void)
{
	RCC_AHBENR |= BIT18;        // Turn on GPIOB
	RCC_APB2ENR |= BIT9;        // Turn on ADC 
	GPIOB_MODER |= (BIT2+BIT3); // Select analog mode for PB1 (pin 15 of LQFP32 package)
	ADC_CR |= BIT31;            // Begin ADCCalibration
	while ((ADC_CR & BIT31));   // Wait for calibration complete
	ADC_SMPR=7;                 // Long sampling time for more stable measurements
	//ADC_CHSELR |= BIT17;      // Select Channel 17, internal reference
	ADC_CHSELR |= BIT9;         // Select Channel 9
	ADC_CCR |= BIT22;	        // Enable the reference voltage
	ADC_CR |= BIT0;             // Enable the ADC
}

int readADC(void)
{
	ADC_CR |=  BIT2;            // Trigger a conversion
	while ( (ADC_CR & BIT2) );  // Wait for End of Conversion
	return ADC_DR;              // ADC_DR has the 12 bits out of the ADC
}

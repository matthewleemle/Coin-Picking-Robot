#include "stm32f05xxx.h"
#include <stdio.h>
#include <stdlib.h>


void delay_ms (int msecs)
{
	int ticks;
	ISR_frc=0;
	ticks=msecs/20;
	while(ISR_frc<ticks);
}


void magnet(int boolean){
	if(boolean==1){
	  GPIOB_ODR |= BIT4;
	}
	if(boolean==0){
	  GPIOB_ODR &= ~(BIT4);

	}
}

void servoPulseTop (int pw1){
 	ISR_pw_top = pw1;
}

void servoPulseBottom (int pw2){
	ISR_pw_bottom = pw2;
}

void pickup() {
	int i = 0;
	bottomServo = ON;
	servoPulseBottom(200);
	delay_ms(800);
	bottomServo = OFF;

	topServo = ON;
	servoPulseTop(240);
	delay_ms(800);
	topServo = OFF;

	magnet(ON);

	for(i=200; i>120; i-=2){
		bottomServo = ON;
		servoPulseBottom(i);
		delay_ms(20);
		bottomServo = OFF;
	}

	topServo = ON;
	servoPulseTop(130);
	delay_ms(800);
	topServo = OFF;

	bottomServo = ON;
	servoPulseBottom(100);
	delay_ms(400);
	bottomServo = OFF;

	magnet(OFF);

	topServo = ON;
	servoPulseTop(60);
	delay_ms(400);
	topServo = OFF;

	bottomServo = ON;
	servoPulseBottom(60);
	delay_ms(400);
	bottomServo = OFF;
}

#include "stm32f05xxx.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "servo_control.c"
#include "coin_detect.c"

#define SYSCLK 8000000L
#define TICK_FREQ 1000L
#define ON 1
#define OFF 0
#define CLOCKWISE 0
#define COUNTERCLOCKWISE 1
#define LEFT_TURNING_TIME 3 //in seconds
#define RIGHT_TURNING_TIME 3
#define DEF_F 100000L

volatile unsigned char pwm_count=0;
volatile unsigned int time_count=0;
volatile unsigned int left_wheel_speed;
volatile unsigned int right_wheel_speed;
volatile unsigned int left_wheel;
volatile unsigned int right_wheel;
volatile unsigned int left_wheel_dir;
volatile unsigned int right_wheel_dir;
volatile unsigned int interrupt_count;

// servo control variables
volatile int ISR_pw_top=100, ISR_pw_bottom=100,ISR_cnt_top=0, ISR_cnt_bottom=0,ISR_frc=0;
volatile int topServo;
volatile int bottomServo;

void both_wheel_speed_change (int);
void left_wheel_speed_change (int);
void right_wheel_speed_change (int);
void go_straight(void);
void go_backwards(void);
void turnLeft(void);
void turnRight(void);
void turn_off(void);
void turn_on(void);
void pin25(int);
void pin26(int);
void pin21(int);
void pin22(int);
void waitms(int);
void turn_deg(int);

// servo control
int egets(char *s, int Max);
void magnet(int);
void servoPulseTop(int);
void servoPulseBottom(int);

// coin detector
void pin18period(void);

void Timer1ISR(void) // interrupts every 1ms
{
	TIM1_SR &= ~BIT0; // clear update interrupt flag

	interrupt_count++;
	time_count++;

	// Servo control
	ISR_cnt_top++;
	ISR_cnt_bottom++;

	if(pwm_count>100) pwm_count = 0;

	if(interrupt_count==100){
			pwm_count+=10;

  	if(left_wheel==ON)  // currently only left wheel code written
		{
    	if(left_wheel_dir == CLOCKWISE)
			{
    		if(pwm_count>left_wheel_speed) // OUT0 = pwmcount>left_wheel_speed? 0:1;
				{
      		pin25(0);
      	}
      	else
				{
        	pin25(1);
    		}
    		pin26(0);
			}

			if(left_wheel_dir == COUNTERCLOCKWISE)
			{
      	if(pwm_count>left_wheel_speed)
				{
	    		pin26(0);
      	}
      	else
				{
      		pin26(1);
      	}
				pin25(0); 													//  OUT1 = 0;
			}
		}

		if(right_wheel==ON)
		{
			if(right_wheel_dir == COUNTERCLOCKWISE)
			{
	    	if(pwm_count>right_wheel_speed) // OUT0 = pwmcount>left_wheel_speed? 0:1;
				{
	     		pin21(0);
	     	}
	     	else
				{
        	pin21(1);
	   		}
	    	pin22(0);
			} 																		//  OUT1 = 0;

			if(right_wheel_dir == CLOCKWISE)
			{
    		if(pwm_count>right_wheel_speed)
				{
	      	pin22(0);
				}
	     	else
				{
	     		pin22(1);
	     	}
				pin21(0);                          //  OUT1 = 0;
			}
		}

		if(right_wheel==OFF)
		{
			pin22(0);
			pin21(0);
		}

		if(left_wheel==OFF)
		{
			pin25(0);
			pin26(0);
		}
		interrupt_count = 0;
	}

	// Servo Control
	if(topServo == ON){
		if(ISR_cnt_top<ISR_pw_top)
		{
			GPIOA_ODR |= BIT4; // PA4=1 pin 10
		}
		else
		{
			GPIOA_ODR &= ~(BIT4); // PA4=1 10
		}
		if(ISR_cnt_top>=2000)
		{
			ISR_cnt_top=0; // 2000 * 10us=20ms
			ISR_frc++;
		}
	}

	if (bottomServo==ON){
		if(ISR_cnt_bottom<ISR_pw_bottom)
		{
			GPIOA_ODR |= BIT5; // PA5=1 pin 11
		}
		else
		{
			GPIOA_ODR &= ~(BIT5); // PA5=1 pin 11
		}
		if(ISR_cnt_bottom>=2000)
		{
			ISR_cnt_bottom=0; // 2000 * 10us=20ms
			ISR_frc++;
		}
	}
}



void SysInit()
{
	// Set up output port bit for blinking LED
	RCC_AHBENR |= 0x00020000;  // peripheral clock enable for port A
	RCC_AHBENR |= 0x00040000; // peripheral clock enable for port B

	GPIOA_MODER |= BIT8; // Make pin PA4 output (pin 10)
	GPIOA_MODER |= BIT10; // Make pin PA5 output (pin 11)
	// Information here: http://hertaville.com/stm32f0-gpio-tutorial-part-1.html
	GPIOA_MODER &= ~(BIT16 | BIT17); // Make pin PA8 input
	GPIOA_MODER |= BIT22; // Make pin PA11 output (pin 21)
	GPIOA_MODER |= BIT24; // Make pin PA12 output (pin 22)
	GPIOA_MODER |= BIT30; // Make pin 25 output
	GPIOB_MODER |= BIT6; // Make pin 26 output
	GPIOB_MODER |= BIT8; // Make pin 27 output

	// Activate pull up for pin PA8:
	GPIOA_PUPDR |= BIT16;
	GPIOA_PUPDR &= ~(BIT17);

	// Set up timer
	RCC_APB2ENR |= BIT11; // turn on clock for timer1
	TIM1_ARR = SYSCLK/DEF_F;
	ISER |= BIT13;        // enable timer interrupts in the NVIC
	TIM1_CR1 |= BIT4;     // Downcounting
	TIM1_CR1 |= BIT0;     // enable counting
	TIM1_DIER |= BIT0;    // enable update event (reload event) interrupt
	enable_interrupts();

}

void both_wheel_speed_change (int input_speed){ //changes both wheel speed
	left_wheel_speed = input_speed;
	right_wheel_speed = input_speed;
}

void left_wheel_speed_change (int input_speed){ // changes the left wheel speed
	left_wheel_speed = input_speed;
}

void right_wheel_speed_change (int input_speed){ // changes the right wheel speed
  right_wheel_speed = input_speed;
}

void right_wheel_on(void){
	right_wheel = ON;
}

void left_wheel_on(void){
	left_wheel = ON;
}




void turn_on(void){ // turns on the robot
  right_wheel = ON;
  left_wheel = ON;
}

void turn_off(void){ // turns off the robot
  right_wheel = OFF;
  left_wheel = OFF;
}

void go_straight(void){ // makes the robot go straight
	left_wheel_dir = COUNTERCLOCKWISE;
  right_wheel_dir = CLOCKWISE;
  right_wheel_on();
  left_wheel_on();
}

void go_backwards(void){ // turns the robot backwards
  left_wheel_dir = CLOCKWISE;
  right_wheel_dir = COUNTERCLOCKWISE;
  right_wheel_on();
  left_wheel_on();
}

void turnLeft(void){ //turns the robot left
	right_wheel_dir = CLOCKWISE;
  left_wheel_dir = CLOCKWISE;
  right_wheel_on();
  left_wheel_on();
}

void turnRight(void){ // turns the robot right
	right_wheel_dir = COUNTERCLOCKWISE;
  left_wheel_dir = COUNTERCLOCKWISE;
  right_wheel_on();
	left_wheel_on();
}

void pin25(int boolean){ // switches pin25 output
  if(boolean==1){
    	GPIOA_ODR |= BIT15; // PA11=1
  }

  if(boolean==0){
    GPIOA_ODR &= ~(BIT15); // PA11=0
  }
}

void pin26(int boolean){ // switches pin26 output
  if(boolean==1){
    GPIOB_ODR |= BIT3;
  }

  if(boolean==0){
    GPIOB_ODR &= ~(BIT3);
  }
}

void pin21(int boolean){
  if(boolean==1){
    	GPIOA_ODR |= BIT11; // PA11=1
  }

  if(boolean==0){
    GPIOA_ODR &= ~(BIT11); // PA11=0
  }
}

void pin22(int boolean){
  if(boolean==1){
    GPIOA_ODR |= BIT12; // PA12=1
  }

  if(boolean==0){
  	GPIOA_ODR &= ~(BIT12); // PA12=0
  }
}

void waitms(int ms){ //pauses main program but does not stop interrupts
	time_count = 0;
	while(time_count<= ms);
}

void turn_deg(int time){
	both_wheel_speed_change(50);
	turnRight();
	waitms(time);
  turn_off();
}

// pin18period gets period for coin detector (every 2 seconds)
// pickup activates servo to pickup one coin

int main(void)
{
  char buff[17];

	float var = 10000;
	SysInit();
	waitms(500); // Wait for putty to start.

	while(1)
	{
		var = 20000.0/(454*2.4);
		both_wheel_speed_change(50);
		turn_on();
		left_wheel_dir = CLOCKWISE;
		right_wheel_dir = COUNTERCLOCKWISE;

	}
	return 0;
}

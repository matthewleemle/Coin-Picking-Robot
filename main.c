#include "stm32f05xxx.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "serial.h"
#include "adc.h"
#include "serial2.h"
#include "serial2.c"


#define SYSCLK 8000000L
#define SYSCLKTIM3 48000000L
#define TICK_FREQ 1000L
#define CLOCKWISE 0
#define COUNTERCLOCKWISE 1
#define LEFT_TURNING_TIME 3 // in seconds
#define RIGHT_TURNING_TIME 3
#define DEF_F 100000L
#define ON 1
#define OFF 0
#define F_CPU 48000000L
#define PIN_PERIOD_18 (GPIOA_IDR&BIT8)
#define FREQ_SPEAKER 500L

volatile unsigned char pwm_count=0;
volatile unsigned int time_count=0;
volatile unsigned int time_count_3=0;
volatile unsigned int left_wheel_speed;
volatile unsigned int right_wheel_speed;
volatile unsigned int left_wheel;
volatile unsigned int right_wheel;
volatile unsigned int left_wheel_dir;
volatile unsigned int right_wheel_dir;
volatile unsigned int interrupt_count;
volatile unsigned int ISR_cnt_top;
volatile unsigned int ISR_cnt_bottom;
volatile unsigned int servo_status;
volatile unsigned int i;
volatile unsigned int coin_num;
volatile unsigned int led_count;
volatile unsigned int turn_left_flag;
volatile unsigned int turn_right_flag;
volatile unsigned int turn_flag;
volatile unsigned int speaker_sw = 0;
volatile unsigned int speaker;
volatile unsigned int speaker_count = 0;

volatile float a1,a2;
volatile int j[2];

unsigned char buff[100];

// servo control variables
volatile int topServo;
volatile int bottomServo;
volatile unsigned int ISR_frc=0;
volatile unsigned int ISR_pw_top=0, ISR_pw_bottom=0;
volatile unsigned int speaker;

/*************************** ROBOTO *********************************/
volatile unsigned robot_mode;
volatile int freq;
volatile float freq_f;
volatile int coin_detect;
volatile unsigned int pickup_speaker = 0;

char HexDigit[]="0123456789ABCDEF";

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
void turn_time(int);
long int GetPeriod(long long int);
float pin18period(void);
void delay_ms (int);
void printADC (void);

// servo control
int egets(char *s, int Max);
void magnet(int);
void servoPulseTop(int);
void servoPulseBottom(int);
void pickup(void);
void initServoPos(void);

// coin detector
void metal_detect(void);

// perimeter detector (ADC)
// void initADC(void);
int readADC(void);

void Timer2ISR(void){
  TIM2_SR &= ~(BIT0);

  if (pickup_speaker == 1)
  {
    speaker_count++;

    if (speaker_count == 2500)
    {
      if (speaker == 1)
      {
        speaker = 0;
        speaker_count = 0;
      }
      else
      {
        speaker = 1;
        speaker_count = 0;
      }
    }

    if (speaker == 1)
    {
      if (speaker_sw == 0)
      {
        GPIOA_ODR |= BIT6; // speaker
        speaker_sw = 1;
      }
      else
      {
        GPIOA_ODR &= ~(BIT6); // speaker
        speaker_sw = 0;
      }
    }
  }
}

void Timer14ISR(void){
  TIM14_SR &= ~(BIT0);

  if(turn_right_flag==1&&turn_flag==1)
  {
		led_count++;
    if(led_count==1000)
    {
      GPIOB_ODR |= BIT5;
	  }
    else if(led_count > 2000)
    {
		    GPIOB_ODR &= ~(BIT5);
		    led_count = 0;
    }
  }

	if(turn_left_flag==1&&turn_flag==1)
	{
		led_count++;
		if(led_count==1000)
		{
			GPIOB_ODR |= BIT7;
		}
		else if(led_count > 2000)
		{
				GPIOB_ODR &= ~(BIT7);
				led_count = 0;
		}
	}

	if(turn_flag == 0)
  {
		GPIOB_ODR |= (BIT5);
		GPIOB_ODR |= (BIT7);
	}

}

void Timer3ISR(void){
	TIM3_SR &= ~BIT0; // clear update interrupt flag

	ISR_cnt_top++;
	ISR_cnt_bottom++;

	if(topServo == ON)
  {
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

	if (bottomServo==ON)
  {
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


void Timer1ISR(void) // interrupts every 1ms
{
	TIM1_SR &= ~BIT0; // clear update interrupt flag
  pwm_count+=10;
	time_count++;

	if(pwm_count>100) pwm_count = 0;

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
}


void SysInit()
{
	// Set up output port bit for blinking LED
	RCC_AHBENR |= 0x00020000;  // peripheral clock enable for port A
	RCC_AHBENR |= 0x00040000; // peripheral clock enable for port B
	GPIOA_MODER |= BIT8; // Make pin PA4 output (pin 10)
	GPIOA_MODER |= BIT10; // Make pin PA5 output (pin 11)
	GPIOA_MODER |= BIT0;
	// Information here: http://hertaville.com/stm32f0-gpio-tutorial-part-1.html
	GPIOA_MODER &= ~(BIT16 | BIT17); // Make pin PA8 input
	GPIOA_MODER |= BIT22; // Make pin PA11 output (pin 21)
	GPIOA_MODER |= BIT24; // Make pin PA12 output (pin 22)
	GPIOA_MODER |= BIT30; // Make pin 25 output
	GPIOB_MODER |= BIT6; // Make pin 26 output
	GPIOB_MODER |= BIT8; // Make pin 27 output

	GPIOB_MODER |= BIT10; // Make 28 output

	GPIOB_MODER |= BIT12; //pin 29

	GPIOB_MODER |= BIT14; // pin 30
	GPIOA_MODER |= BIT14; // pin 13

  GPIOA_MODER |= BIT12; // Make pin PA6 output (pin 12)

// enable timer 14

	RCC_APB1ENR |= BIT8; // turn on clock for timer14
	TIM14_ARR = SYSCLK/TICK_FREQ;
	ISER |= BIT19;        // enable timer interrupts in the NVIC
  TIM14_CR1 |= BIT0;     // enable counting
	TIM14_DIER1 |= BIT0;    // enable update event (reload event) interrupt

	// Activate pull up for pin PA8:
	GPIOA_PUPDR |= BIT16;
	GPIOA_PUPDR &= ~(BIT17);

	// Set up timer 1 (first timer)
	RCC_APB2ENR |= BIT11; // turn on clock for timer1
	TIM1_ARR = SYSCLK/TICK_FREQ;
	ISER |= BIT13;        // enable timer interrupts in the NVIC
	TIM1_CR1 |= BIT4;     // Downcounting
  TIM1_CR1 |= BIT0;     // enable counting
	TIM1_DIER |= BIT0;    // enable update event (reload event) interrupt

	// Set up output port bit for blinking LED
	RCC_AHBENR |= 0x00020000;  // peripheral clock enable for port A
	// GPIOA_MODER |= 0x00000001; // Make pin PA0 output (pin 6)

	// Set up timer 3 (second timer)
	RCC_APB1ENR |= BIT1;  // TIM 3 timer clock enable
	TIM3_ARR = SYSCLKTIM3/DEF_F;
	ISER |= BIT16;        // enable timer interrupts in the NVIC
	TIM3_CR1 |= BIT4;     //downcounting
 	TIM3_CR1 |= BIT0; 		// enable timer 3
	TIM3_DIER |= BIT0; 		// enable interrupt

  // Set up timer 2
	RCC_APB1ENR |= BIT0;  // TIM 2 timer clock enable
	TIM2_ARR = SYSCLK/FREQ_SPEAKER; // CHANGE VALUE
	ISER |= BIT15;        // enable timer interrupts in the NVIC
	TIM2_CR1 |= BIT4;     // downcounting
 	TIM2_CR1 |= BIT0; 		// enable timer 2
	TIM2_DIER |= BIT0; 		// enable interrupt

	enable_interrupts();

	initADC();




}

/********************************** WHEEL CONTROL *****************************/

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

void turnRight(void){// turns the robot right
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

void turn_time(int time){
	both_wheel_speed_change(100);

	if(turn_left_flag==1){
	   turnLeft();
     turn_left_flag = 0;
 	   turn_right_flag = 1;
     waitms(time);
  }
	else if(turn_right_flag==1){
		turnRight();
    turn_right_flag = 0;
		turn_left_flag = 1;
    waitms(time/2);
  }

  turn_off();
}

/********************************* SERVO CONTROL *******************************/

void delay_ms (int msecs)
{
	int ticks;
	ISR_frc=0;
	ticks=msecs/20;
	while(ISR_frc<ticks);
}

void magnet(int boolean){
	if(boolean==1){
	  GPIOB_ODR |= BIT4;		// pin 27
	}
	if(boolean==0){
	  GPIOB_ODR &= ~(BIT4);		// pin 27

	}
}

void servoPulseTop (int pw1){
 	ISR_pw_top = pw1;
}

void servoPulseBottom (int pw2){
	ISR_pw_bottom = pw2;
}

void pickup(void) {
	GPIOA_ODR &= ~(BIT7); // pin 13 LED on
	GPIOB_ODR &= ~(BIT6); // pin 29 LED on
	int i = 0;
	bottomServo = ON;
	servoPulseBottom(200);
	delay_ms(400);
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

  for(i=240; i>60; i-=2){
    topServo = ON;
    servoPulseTop(i);
    delay_ms(20);
    topServo = OFF;
  }

  for(i=120; i>100; i-=2){
  	bottomServo = ON;
  	servoPulseBottom(i);
  	delay_ms(20);
  	bottomServo = OFF;
  }

  for(i=60; i<160; i+=2){
  	topServo = ON;
  	servoPulseTop(i);
  	delay_ms(20);
  	topServo = OFF;
  }

	magnet(OFF);
	waitms(2000);

	topServo = ON;
	servoPulseTop(60);
	delay_ms(500);
	topServo = OFF;

	bottomServo = ON;
	servoPulseBottom(60);
	delay_ms(400);
	bottomServo = OFF;

	GPIOA_ODR |= (BIT7); // pin 13 LED off
	GPIOB_ODR |= BIT6; // pin 29 LED off

}

void initServoPos(void) {
	topServo = ON;
	servoPulseTop(60);
	delay_ms(800);
	topServo = OFF;

	bottomServo = ON;
	servoPulseBottom(60);
	delay_ms(800);
	bottomServo = OFF;
}

/******************************* COIN DETECTOR *********************************/

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

// GetPeriod() seems to work fine for frequencies between 300Hz and 600kHz.
long int GetPeriod (long long int n)
{
	long long int i;
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

float pin18period(void){
	long long int count;
	float T, f;

		count=GetPeriod(10000);
		if(count>0)
		{
			T=count/(F_CPU*10000.0);
			f=1/T;
		}
		else
		{
	//		eputs("NO SIGNAL                     \r");
			f= 0;
		}
		waitms(10);

		return f;
}

void metal_detect(void)
{
	coin_detect=0;
	freq = (int) pin18period();
	if (freq>59700)
	{
		//PrintNumber(freq,10,7);
 		//eputs("++++++++++++++++++++++\r\n");
		coin_detect++;
	}
	else
	{
	   //PrintNumber(freq,10,7);
	   //eputs("----------------------\r\n");
	}
//  sprintf(buff,"%d",freq);
//  send_string2(buff);
//  send_string2("\r\n");

	if (coin_detect>0)
	{
		coin_detect=0;
		turn_off();
		waitms(100);
		for (i = 0; i < 5; i++)
		{
			freq = (int) pin18period();
			if (freq>60700)
			{
				//PrintNumber(freq,10,7);
				//eputs("++++++++++++++++++++++\r\n");
				coin_detect++;
			}
			else
			{
				//PrintNumber(freq,10,7);
    		//eputs("----------------------\r\n");
			}
  //    sprintf(buff,"%d",freq);
//      send_string2(buff);
  //    send_string2("\r\n");
			waitms(50);
		}
	}

	if (coin_detect>4)
	{
    pickup_speaker = 1;
		turn_on();
		go_backwards();
		waitms(1500);
		turn_off();
	  pickup();
		coin_num++;
    pickup_speaker = 0;
    sprintf(buff,"Coins picked up: %d", coin_num);
    send_string2(buff);
    send_string2("\r\n");
	}
//	eputs("\r\n");

	turn_on();
	go_straight();
}

/************************** PERIMETER DETECT *********************************/


void printADC(void){

		ADC_CHSELR = BIT8;          // Select Channel 8
		j[0]=readADC();
		a1=(j[0]*3.3)/4096.0;
		ADC_CHSELR = BIT9;          // Select Channel 9
		j[1]=readADC();
		a2=(j[1]*3.3)/4096.0;
		j[0] = (int)(a1 * 1000);
		j[1] = (int)(a2 * 1000);

	//	printf("ADC[8]=%d V=%fV, ADC[9]=%d V=%fV,\r", j1, a1, j2, a2);
	/*	PrintNumber(j[0], 10, 4);
		eputs("\r\n");
		PrintNumber(j[1], 10, 4);
		eputs("\r\n");*/
		//fflush(stdout);
}

/*********************************** MAIN *************************************/

// pin18period gets period for coin detector (every 2 seconds)
// pickup activates servo to pickup one coin
int main(void)
{
	SysInit();
  pickup_speaker = 0;


	robot_mode = 0;

	topServo = ON;
	servoPulseTop(60);
	delay_ms(800);
	topServo = OFF;

	bottomServo = ON;
	servoPulseBottom(60);
	delay_ms(800);
	bottomServo = OFF;

  both_wheel_speed_change(100);
	coin_num = 0;

	turn_left_flag = 1;

	GPIOA_ODR |= (BIT7); // pin 13 LED on
	GPIOB_ODR |= (BIT6); // pin 29 LED on

  initUART2(9600);

  while(1)
  {
    send_string2("Mode? C for Coinpicking, R for remote control: ");
    get_string2(buff, sizeof(buff)-1);
    send_string2(buff);
    send_string2("\r\n");

    if(buff[0]== 'C'){
      robot_mode = 1;
      break;
    }

    if(buff[0]== 'R'){
      robot_mode = 0;
      break;
    }
  }


	while(1)
	{
    if(robot_mode == 0)
    {
      GPIOA_ODR |= (BIT7); // pin 13 LED on
    	GPIOB_ODR |= (BIT6); // pin 29 LED on
      get_string2(buff, sizeof(buff)-1);
      send_string2(buff);
      send_string2("\r\n");

      if (buff[0] == 'r'){
        turn_flag = 1;
        turnRight();
        turn_left_flag = 1;
        turn_right_flag = 0;
      }

      if (buff[0] == 'l'){
        turn_flag = 1;
        turnLeft();
        turn_left_flag = 0;
        turn_right_flag = 1;
      }

      if (buff[0] == 'f'){
        turn_flag = 0;
        go_straight();
      }

      if (buff[0] == 'b'){
        turn_flag = 0;
        go_backwards();
      }

      if(buff[0] == 'o'){
        turn_flag = 0;
        turn_off();
      }

      if(buff[0] == 'C'){
        turn_flag = 0;
        GPIOA_ODR &= ~(BIT7); // pin 13 LED off
      	GPIOB_ODR &= ~(BIT6); // pin 29 LED off
        robot_mode = 1;
      }

      if(buff[0] == 'p') {
        pickup_speaker = 1;
        turn_flag = 0;
        pickup();
        pickup_speaker = 0;
      }
    }

    if (robot_mode == 1)
    {
      if (coin_num<20)
      {
  			go_straight();
  		  metal_detect();
        printADC();

  			if(j[0]>(1500)||j[1]>(1500))
        {
  				go_backwards();
  				waitms(3000);
  				turn_flag = 1;
  				turn_time(6000);
  				turn_flag = 0;
  			}
  		}
  		else
      {
  		    turn_off();
          coin_detect = 0;
          robot_mode=0;
  		}
    }
  }
	return 0;
}

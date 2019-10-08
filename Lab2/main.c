#include <lpc17xx.h>
#include "GLCD.h"
#include <stdio.h>
#include <cmsis_os2.h>

void questionOne(void){
	
	//initializing LED	
	LPC_GPIO1->FIODIR |= 0xB0000000;
	LPC_GPIO2->FIODIR |= 0x0000007C;
	//turn off all leds
	LPC_GPIO1->FIOCLR |= 0xB0000000;
	LPC_GPIO2->FIOCLR |= 0x0000007C;
	
	int displayNum=123;	
	int testNum = displayNum;
	
	testNum &= 1;	
	if (testNum!=0){
		LPC_GPIO2->FIOSET |=64 ;
	}
	testNum=displayNum;
	testNum &= 2;
	if (testNum!=0){
		LPC_GPIO2->FIOSET |=32 ;
		testNum=displayNum;
	}
	
	testNum &= 4;
	if (testNum){
		LPC_GPIO2->FIOSET |=16 ;
		}
	testNum=displayNum;
	
	testNum &= 8;
	if (testNum){
		LPC_GPIO2->FIOSET |=8 ;
		}
	testNum=displayNum;

	testNum &= 16;
	if (testNum){
		LPC_GPIO2->FIOSET |=4 ;
		}
	testNum=displayNum;

	testNum &= 32;
	if (testNum){
		LPC_GPIO1->FIOSET |= 0x80000000;
		}
	testNum=displayNum;
	
	testNum &= 64;
	if (testNum){
		LPC_GPIO1->FIOSET |= 0x20000000;
		}
	testNum=displayNum;

	testNum &= 128;
	if (testNum){
		LPC_GPIO1->FIOSET |= 0x10000000;
		}
	testNum=displayNum;
	
	
	while(1);	
}	

void joystickQuestion(void){
	while(1){
		//initialize sensors
	int checkUp=(LPC_GPIO1->FIOPIN &= 0x800000);
	int checkRight=(LPC_GPIO1->FIOPIN &= 0x1000000);
	int checkDown=(LPC_GPIO1->FIOPIN &= 0x2000000);
	int checkLeft=(LPC_GPIO1->FIOPIN &= 0x4000000);
	int checkPushed=(LPC_GPIO1->FIOPIN &= 0x100000);
	
		//display strings
	if (checkUp==0)
		printf("UP, ");
	else if (checkRight==0)
		printf("RIGHT, ");
	else if (checkDown==0)
		printf("DOWN, ");
	else if (checkLeft==0)
		printf("LEFT, ");	
	
	if (checkPushed==0)
		printf("pressed\n");
	else if (checkPushed!=0)
		printf("not pressed\n");
	}
}

void pushbuttonQuestion(void){
		while (1){
		int checkButton=(LPC_GPIO2->FIOPIN &= 0x400);
		if (checkButton==0)
			printf("Pressed\n");
	}
}

void potentiometerQuestion(void *arg){
	LPC_PINCON->PINSEL1 &= ~(0x03<<18);//clear bits 18 and 19
	LPC_PINCON->PINSEL1 |= (0x01<<18);
	LPC_SC->PCONP |= (1<<12);
	LPC_ADC->ADCR = (1 << 2) |     // select AD0.2 pin
									(4 << 8) |  // ADC clock is 25MHz/5 
									(1 << 21);     // enable ADC

	while(1){
		LPC_ADC->ADCR |= (1<<24);
		if (LPC_ADC->ADGDR & (1<<31)){
			//check value
			uint16_t checkSet = (LPC_ADC->ADGDR);
			//display value
			printf("%d\n", checkSet>>4);
		}
	}
}

void printHello(void){
	GLCD_Init();
	GLCD_Clear(0x001F);//clear to blue

	unsigned char s []= "Hello,world!";
	GLCD_SetBackColor(0x001F);
	GLCD_DisplayString(4,4,1,s);
	GLCD_SetTextColor(0xFFFF);
}

void joystickLCD(void *arg){
	GLCD_Init();
	GLCD_Clear(0x001F);//clear to blue
	GLCD_SetBackColor(0x001F);
	GLCD_SetTextColor(0xFFFF);
	
	while(1){
	//initialize sensors
		int checkUp=(LPC_GPIO1->FIOPIN &= 0x800000);
		int checkRight=(LPC_GPIO1->FIOPIN &= 0x1000000);
		int checkDown=(LPC_GPIO1->FIOPIN &= 0x2000000);
		int checkLeft=(LPC_GPIO1->FIOPIN &= 0x4000000);
		int checkPushed=(LPC_GPIO1->FIOPIN &= 0x100000);
			
		unsigned char up [] = "UP        ";
		unsigned char right [] = "RIGHT     ";
		unsigned char down [] = "DOWN      ";
		unsigned char left [] = "LEFT      ";	
		unsigned char noDir [] = "NO DIR    ";
		unsigned char pressed []= "Pressed    ";
		unsigned char notPressed []= "Not pressed";
		
		//display strings 
		if (checkUp==0)
			GLCD_DisplayString(4,4,1,up);
		else if (checkRight==0)
			GLCD_DisplayString(4,4,1,right);
		else if (checkLeft==0)
			GLCD_DisplayString(4,4,1,left);
		else if (checkDown==0)
			GLCD_DisplayString(4,4,1,down);
		else 
			GLCD_DisplayString(4,4,1,noDir);

		if (checkPushed==0)
			GLCD_DisplayString(5,4,1,pressed);
		else 
			GLCD_DisplayString(5,4,1,notPressed);
	}	
}

void readPush(void *arg){

		LPC_GPIO2->FIODIR |= 0x0000007C;
	LPC_GPIO1->FIODIR |= 0xB0000000;
	
	while(1)
	{
		osThreadYield();
		if((LPC_GPIO2->FIOPIN & 0x400) == 0){
			if((LPC_GPIO2->FIOPIN &= 0x40) !=0) 
				LPC_GPIO2->FIOCLR |= 0x40;
			else
				LPC_GPIO2->FIOSET |= 0x00000040;
			}
			while((LPC_GPIO2->FIOPIN & 0x400) == 0)
			;
	}
		

	}

int main(void){
	printf(":D"); //initialize UART 
	osKernelInitialize();
	osThreadNew(potentiometerQuestion, NULL, NULL);
	osThreadNew(joystickLCD, NULL, NULL);
	osThreadNew(readPush, NULL, NULL);
	
	osKernelStart();
	while (1){}

}
	
	
	


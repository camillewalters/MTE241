#include <lpc17xx.h>
#include "GLCD.h"
#include <stdio.h>
#include <cmsis_os2.h>
#include <stdbool.h>
#include "lfsr113.h"
#include "random.h"
#include <stdlib.h>
#include <time.h>

typedef struct points{
 unsigned int x;
 unsigned int y;
}Point;


//global variables

int foodX;
int foodY;
unsigned char direction = 'R';
bool pause = false;
unsigned int speed = 6;
//this array holds values at the points where food or a snake can be
unsigned int screen[40][27]={0};
unsigned int length=3;
//this array holds the points of the worm and their location in the screen array
Point worm[200];
int score=0;
unsigned char bigPixel []={0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F,
0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F,
0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F,
0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F,
0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F,
0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F,
0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F,
0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F,0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F,
0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F,
0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F,
0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F,
0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F,
0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F,
0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F,
0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F};
unsigned char blackPixel[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//osEventFlagsId_t pauseScreen;
	
//Initialize LEDs
void initializeLED(void){
	//initializing LED	
	LPC_GPIO1->FIODIR |= 0xB0000000;
	LPC_GPIO2->FIODIR |= 0x0000007C;
	//turn off all leds
	LPC_GPIO1->FIOCLR |= 0xB0000000;
	LPC_GPIO2->FIOCLR |= 0x0000007C;
}

//Initialize Potentiometer
void initializePotent(void){
	LPC_PINCON->PINSEL1 &= ~(0x03<<18);//clear bits 18 and 19
	LPC_PINCON->PINSEL1 |= (0x01<<18);
	LPC_SC->PCONP |= (1<<12);
	LPC_ADC->ADCR = (1 << 2) |     // select AD0.2 pin
									(4 << 8) |  // ADC clock is 25MHz/5 
									(1 << 21);     // enable ADC
}

//Function to display an 8X8 pixel blue square
void displayPixel(int xVal,int yVal){
	GLCD_Bitmap(xVal*8,(yVal*8)+16,8,8,bigPixel);
}

//Function to display an 8X8 pixel black square, which erases existing displayed squares
void erasePixel(int xVal,int yVal){
	GLCD_Bitmap(xVal*8, (yVal*8)+16, 8,8,blackPixel);
}

//initializes snake: 3 pixels centered on the screen
void initSnake(){
	screen[15][14]=1;
	screen[14][14]=1;
	screen[13][14]=1;
	displayPixel(13,14);
	displayPixel(14,14);
	displayPixel(15,14);
	//worm[0]
	worm[0].x=15;
	worm[0].y=14;
	worm[1].x=14;
	worm[1].y=14;
	//tail
	worm[2].x=13;
	worm[2].y=14;
	
}

//initialize screen with black background and bar under text
void initializeScreen(void){
	GLCD_Init();
	GLCD_Clear(0);//clear to white
	GLCD_SetBackColor(0);
	GLCD_SetTextColor(0xFFFF);
	GLCD_Bargraph(0,16,320,6,0xFFFF);
}

//displays when snake touches itself or the barrier. Will display message in the middle of the screen
void youLose(void){
	int baseSpeed = osKernelGetTickFreq(); //figure out speed in ticks
	char message[]="You Lose!";
	GLCD_SetTextColor(0xFFFF);
	GLCD_DisplayString(5,5,1,message);
	osDelay(osWaitForever);
}	

//converts joystick readings to directions
void joystick(void *arg){
	while(1){
		int checkUp=(LPC_GPIO1->FIOPIN &= 0x800000);
		int checkRight=(LPC_GPIO1->FIOPIN &= 0x1000000);
		int checkDown=(LPC_GPIO1->FIOPIN &= 0x2000000);
		int checkLeft=(LPC_GPIO1->FIOPIN &= 0x4000000);
		int checkPushed=(LPC_GPIO1->FIOPIN &= 0x100000);
		
		//snake can't turn back on itself. if player attempts to do this, ignore
		if(checkUp==0){
			if (direction!='D'){
			direction = 'U';}}
		else if (checkRight==0){
			if (direction!='L'){
			direction = 'R';}}
		else if (checkDown==0){
			if (direction!='U'){
			direction = 'D';}}
		else if (checkLeft==0){
			if (direction!='R'){
			direction = 'L';}}

			//if you push and move the joystick a direction, assume it goes the direction and does not pause
			
		if(checkPushed==0&&!(checkLeft==0||checkUp==0||checkDown==0||checkRight==0)){
			pause = !pause;
			while((LPC_GPIO1->FIOPIN &= 0x100000)==0)
				;
		}
		osThreadYield();
	}
}

//converts potentiometer readings to speed values. there are 6 possible speeds
void potentiometer(void *arg){
	while(1){
		LPC_ADC->ADCR|=(1<<24);
			if (LPC_ADC->ADGDR & (1<<31)){
			//check value
			uint16_t checkSet = (LPC_ADC->ADGDR);
				printf("%d\n",checkSet);
			//uint16_t checkSet2=checkSet;
			//display value
					if (checkSet>=450&&checkSet<11300){
						speed=6;
					}
					else if(checkSet>=11300&&checkSet<22150){
						speed=5;
					}
					else if(checkSet>=22150&&checkSet<33000){
						speed=4;
					}
					else if(checkSet>=33000&&checkSet<43850){
						speed=3;
					}
					else if(checkSet>=43850&&checkSet<54700){
						speed=2;
					}
					else if(checkSet>=54700&&checkSet<65500){
						speed=1;
					}
				}
//			osThreadYield();
		}
}

//lights up number of LEDs according to speed
void LED(void *arg){
	while(1){
		osThreadYield();
		if(speed==1){
			LPC_GPIO2->FIOCLR |=16 ;
			LPC_GPIO2->FIOCLR |=8 ;
			LPC_GPIO2->FIOCLR |=4 ;
			LPC_GPIO1->FIOCLR |= 0x80000000;
			LPC_GPIO1->FIOCLR |= 0x20000000;
			LPC_GPIO1->FIOSET |= 0x10000000;		
		}
		else if(speed==2){
			LPC_GPIO2->FIOCLR |=16 ;
			LPC_GPIO2->FIOCLR |=8 ;
			LPC_GPIO2->FIOCLR |=4 ;
			LPC_GPIO1->FIOCLR |= 0x80000000;
			LPC_GPIO1->FIOSET |= 0x20000000;
			LPC_GPIO1->FIOSET |= 0x10000000;		
		}
		else if(speed==3){
			LPC_GPIO2->FIOCLR |=16 ;
			LPC_GPIO2->FIOCLR |=8 ;
			LPC_GPIO2->FIOCLR |=4 ;
			LPC_GPIO1->FIOSET |= 0x80000000;
			LPC_GPIO1->FIOSET |= 0x20000000;
			LPC_GPIO1->FIOSET |= 0x10000000;		
		}
		else if(speed==4){
			LPC_GPIO2->FIOCLR |=16 ;
			LPC_GPIO2->FIOCLR |=8 ;
			LPC_GPIO2->FIOSET |=4 ;
			LPC_GPIO1->FIOSET |= 0x80000000;
			LPC_GPIO1->FIOSET |= 0x20000000;
			LPC_GPIO1->FIOSET |= 0x10000000;		
		}
		else if(speed==5){
			LPC_GPIO2->FIOCLR |=16 ;
			LPC_GPIO2->FIOSET |=8 ;
			LPC_GPIO2->FIOSET |=4 ;
			LPC_GPIO1->FIOSET |= 0x80000000;
			LPC_GPIO1->FIOSET |= 0x20000000;
			LPC_GPIO1->FIOSET |= 0x10000000;		
		}
		else if(speed==6){
			LPC_GPIO2->FIOSET |=16 ;
			LPC_GPIO2->FIOSET |=8 ;
			LPC_GPIO2->FIOSET |=4 ;
			LPC_GPIO1->FIOSET |= 0x80000000;
			LPC_GPIO1->FIOSET |= 0x20000000;
			LPC_GPIO1->FIOSET |= 0x10000000;		
		}		
	}
}

//displays score at the top of the screen
void titleBar(void *arg){
	unsigned char title [100];
	while(1){		
		
    sprintf(title,"WORM || Score: %d",score);		
		GLCD_DisplayString(0,10,0,title);
		osThreadYield();
	}
}

//clears tail location and deletes the pixel
void updateTail(void){
	//clearing
	screen[worm[length-1].x][worm[length-1].y]=0;
	erasePixel(worm[length-1].x, worm[length-1].y);
}

//generates food in an available spot. assigns screen array value to 3 in that spot
void food(){
	
	//making sure food is not generated where the snake already is
	do{
		foodX=lfsr113()%40;
		foodY=lfsr113()%27;
	}while(screen[foodX][foodY]!=0);
	screen[foodX][foodY]=3;
	displayPixel(foodX,foodY);
}

//moves the snake by updating screen and worm array and displaying and clearing pixels
//handles eating food, hitting boundary and hitting another part of the snake

void snake(void *arg){
	int sum=0;
	int baseSpeed = osKernelGetTickFreq(); //figure out speed in ticks
	while (1){
		if(direction=='R'){
			sum=screen[worm[0].x+1][worm[0].y]+1;
			if (worm[0].x==40){
				youLose();
			}	
		}
		else if(direction=='L'){
			sum=screen[worm[0].x-1][worm[0].y]+1;
			if (worm[0].x==1){
				youLose();
			}	
		}
		else if(direction=='U'){
			sum=screen[worm[0].x][worm[0].y-1]+1;
			if (worm[0].y==1){
				youLose();
			}	
		}
		else if (direction=='D'){
			sum=screen[worm[0].x][worm[0].y+1]+1;
			if (worm[0].y==27){
				youLose();
			}	
		}
		
		if (sum==1){
//			erasePixel(worm[length-1].x,worm[length-1].y);
			updateTail();
			for(int i=length-1; i>=1; i--){
				worm[i]=worm[i-1];
			}
			if(direction=='R'){
				worm[0].x++;
			}
			else if(direction=='L'){
				worm[0].x--;
		  }
			else if(direction=='U'){
				worm[0].y--;
			}
			else if (direction=='D'){
				worm[0].y++;
			}
			screen[worm[0].x][worm[0].y]=1;
			displayPixel(worm[0].x, worm[0].y);
		}
		if(sum==2){
			youLose();
		}
		if(sum==4){
			for(int i=length; i>0; i--){
				worm[i]=worm[i-1];
			}
			worm[0].x=foodX;
			worm[0].y=foodY;
			screen[foodX][foodY]=1;
			//erasePixel(foodX,foodY);
			length++;
			score=score+(length*speed);
			food();			
		}	
		while(pause);
		osDelay(baseSpeed/(speed*4));
		//osThreadYield();	
	}
}	

int main(void){
	printf("yyy");
	initializeLED();
	initializePotent();
	initializeScreen();
	initSnake();
	food();
	

	osKernelInitialize();
	osThreadNew(joystick, NULL, NULL);
	osThreadNew(snake,NULL,NULL);
	osThreadNew(LED,NULL,NULL);
	osThreadNew(potentiometer,NULL,NULL);
	osThreadNew(titleBar,NULL,NULL);
	osKernelStart();
	while(1){}
	}

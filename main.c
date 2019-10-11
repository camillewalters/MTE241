#include <lpc17xx.h>
#include <stdio.h>
#include "MPU9250.h"
#include "sensor_fusion.h"
#include <cmsis_os2.h>
#include "led.h"

osMutexId_t mutexA;
osMutexId_t mutexB;
float globalPitch, globalRoll, globalYaw;

void readData(void* arg){
	while(1){
		osMutexAcquire(mutexA, osWaitForever);
		MPU9250_read_gyro();
		MPU9250_read_acc();
	//keeps reading readmag until valid
	do{
		MPU9250_read_mag();
		osThreadYield();
	}
	while((MPU9250_st_value&16)==0);
	osMutexRelease(mutexA);
	}
}

void fusionProcess(void* arg){
	int gx,gy,gz,ax,ay,az,mx,my,mz;

	while(1){
		osMutexAcquire(mutexA, osWaitForever);
		gx=MPU9250_gyro_data[0];
		gy=MPU9250_gyro_data[1];
		gz=MPU9250_gyro_data[2];
		ax=MPU9250_accel_data[0];
		ay=MPU9250_accel_data[1];
		az=MPU9250_accel_data[2];
		mx=MPU9250_mag_data[0];
		my=MPU9250_mag_data[1];
		mz=MPU9250_mag_data[2];
		osMutexRelease(mutexA);
		sensor_fusion_update(gx,gy,gz,ax,ay,az,mx,my,mz);
		float pitch = sensor_fusion_getPitch(); 
		float yaw = sensor_fusion_getYaw();
		float roll = sensor_fusion_getRoll();
		osMutexAcquire(mutexB,osWaitForever);
		globalPitch=pitch;
		globalRoll=roll;
		globalYaw=yaw;
		osMutexRelease(mutexB);
	}

}

void sendData (void* arg){
	float pitch,roll,yaw;
	while(1){
	osMutexAcquire(mutexB, osWaitForever);
	pitch=globalPitch;
	roll=globalRoll;
	yaw=globalYaw;
	osMutexRelease(mutexB);
		
	printf("%f,%f,%f\n", -1*roll,yaw, pitch);
	}
}

int main(void){
	LED_setup();
	printf("hi");
	
	MPU9250_init(1,1);
	LED_display(MPU9250_whoami());
	
	sensor_fusion_init();
	sensor_fusion_begin(100);
	
	osKernelInitialize();
	mutexA=osMutexNew(NULL);
	mutexB=osMutexNew(NULL);	

	osThreadNew(readData,NULL,NULL);
	osThreadNew(fusionProcess,NULL,NULL);
	osThreadNew(sendData,NULL,NULL);

	osKernelStart();
	while(1){};

	
}

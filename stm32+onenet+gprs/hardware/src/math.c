#include "stm32f10x.h"

#include "math.h"
#include "stdio.h"
#include <string.h>

#include "delay.h"
#include "master.h"
#include "crc16.h"

u16 time_cnt =0;	

#define N 10
  char Temps(){
	int sum=0;
	int count;
	for(count=0;count<N;count++)
	{
		sum+=Temp.F_data;
		DelayXms(50);
	}
		return (char)(sum/N);
}
	
 char Cods(){
		int sum=0;
		int count;
 for(count=0;count<N;count++)
	{
		sum+=Cod.F_data;
		DelayXms(50);
	}
		return (char)(sum/N);
	}
 
  char Tocs(){
		int sum=0;
		int count;
 for(count=0;count<N;count++)
	{
		sum+=Toc.F_data;
		DelayXms(50);
	}
		return (char)(sum/N);
	}   
	
  char Ntus(){
		int sum=0;
		int count;
 for(count=0;count<N;count++)
	{
		sum+=NTU.F_data;
		DelayXms(50);
	}
		return (char)(sum/N);
	}
	
	  char Tss(){
		int sum=0;
		int count;
 for(count=0;count<N;count++)
	{
		sum+=Turbidity.F_data;
		DelayXms(50);
	}
		return (char)(sum/N);
	}


int main(void){
	unsigned short timeCount = 0;
	
	 if(++timeCount%50==5){
		  Master_Service(0x01,0x03,0x2600,0x0007);
			DelayXms(100);
	    RS485_RX_Service();
	    Master_Service(0x01,0x03,0x1200,0x0002);
			DelayXms(100);
	    RS485_RX_Service();	
		  Master_Service(0x02,0x03,0x2600,0x0004);
			DelayXms(100);
		  RS485_RX_Service();
			}
}
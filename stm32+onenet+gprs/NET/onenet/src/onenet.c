/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	onenet.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2017-05-08
	*
	*	版本： 		V1.1
	*
	*	说明： 		与onenet平台的数据交互接口层
	*
	*	修改记录：	V1.0：协议封装、返回判断都在同一个文件，并且不同协议接口不同。
	*				V1.1：提供统一接口供应用层使用，根据不同协议文件来封装协议相关的内容。
	************************************************************
	************************************************************
	************************************************************
**/

//单片机头文件
#include "stm32f10x.h"

//网络设备
#include "m6312.h"

//协议文件
#include "onenet.h"
#include "mqttkit.h"

//硬件驱动
#include "usart.h"
#include "delay.h"
#include "master.h"
#include "crc16.h"
#include "bsp_adc.h"


//C库
#include <string.h>
#include <stdio.h>


#define PROID		"182370"

#define AUTH_INFO	"iSw7iiAZCXsyEWKnTpkiMhebkKs="

#define DEVID		"502129186"


extern unsigned char esp8266_buf[128];

extern __IO uint32_t ADC_ConvertedValue[NOFCHANEL];//ADC1转换的电压值通过MDA传到SRAM

float ADC_ConvertedValueLocal[NOFCHANEL*2];//局部变量，用于保存计算后的电压值


//==========================================================
//	函数名称：	OneNet_DevLink
//
//	函数功能：	与onenet创建连接
//
//	入口参数：	无
//
//	返回参数：	1-成功	0-失败
//
//	说明：		与onenet平台建立连接
//==========================================================
_Bool OneNet_DevLink(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//协议包

	unsigned char *dataPtr;
	
	_Bool status = 1;
	
	UsartPrintf(USART_DEBUG, "OneNet_DevLink\r\n"
							"PROID: %s,	AUIF: %s,	DEVID:%s\r\n"
                        , PROID, AUTH_INFO, DEVID);
	
	if(MQTT_PacketConnect(PROID, AUTH_INFO, DEVID, 256, 0, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
	{
		M6312_SendData(mqttPacket._data, mqttPacket._len);				//上传平台
		
		dataPtr = M6312_GetIPD(250);									//等待平台响应
		if(dataPtr != NULL)
		{
			if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
			{                   
				switch(MQTT_UnPacketConnectAck(dataPtr))
				{
					case 0:UsartPrintf(USART_DEBUG, "Tips:	连接成功\r\n");status = 0;break;
					
					case 1:UsartPrintf(USART_DEBUG, "WARN:	连接失败：协议错误\r\n");break;
					case 2:UsartPrintf(USART_DEBUG, "WARN:	连接失败：非法的clientid\r\n");break;
					case 3:UsartPrintf(USART_DEBUG, "WARN:	连接失败：服务器失败\r\n");break;
					case 4:UsartPrintf(USART_DEBUG, "WARN:	连接失败：用户名或密码错误\r\n");break;
					case 5:UsartPrintf(USART_DEBUG, "WARN:	连接失败：非法链接(比如token非法)\r\n");break;
					default:UsartPrintf(USART_DEBUG, "ERR:	连接失败：未知错误\r\n");break;
				}
			}
		}
		
		MQTT_DeleteBuffer(&mqttPacket);								//删包
	}
	else
		UsartPrintf(USART_DEBUG, "WARN:	MQTT_PacketConnect Failed\r\n");
	
	return status;
	
}


//#define N 10
//  char Temps(){
//	int sum=0;
//	int count;
//	for(count=0;count<N;count++)
//	{
//		sum+=Temp.F_data;
//		DelayXms(50);
//	}
//		return (char)(sum/N);
//}
//	
// char Cods(){
//		int sum=0;
//		int count;
// for(count=0;count<N;count++)
//	{
//		sum+=Cod.F_data;
//		DelayXms(50);
//	}
//		return (char)(sum/N);
//	}
// 
//  char Tocs(){
//		int sum=0;
//		int count;
// for(count=0;count<N;count++)
//	{
//		sum+=Toc.F_data;
//		DelayXms(50);
//	}
//		return (char)(sum/N);
//	}   
//	
//  char Ntus(){
//		int sum=0;
//		int count;
// for(count=0;count<N;count++)
//	{
//		sum+=NTU.F_data;
//		DelayXms(50);
//	}
//		return (char)(sum/N);
//	}
//	
//	  char Tss(){
//		int sum=0;
//		int count;
// for(count=0;count<N;count++)
//	{
//		sum+=Turbidity.F_data;
//		DelayXms(50);
//	}
//		return (char)(sum/N);
//	}

//#define N 11
//float Temps()
//{
//	char value_buf[N];
//	char count,i,j,temp;
//	for(count=0;count<N;count++)
//	{
//		value_buf[count]=Temp.F_data;
//		DelayXms(50);
//	}
//	for(j=0;j<N-1;j++)
//	{
//		for(i=0;i<N-j;i++)
//		{
//			if(value_buf[i]>value_buf[i+1])
//			{
//				temp=value_buf[i];
//				value_buf[i]=value_buf[i+1];
//				value_buf[i+1]=temp;
//			}
//		}
//		
//	}
//	return value_buf[(N-1)/2];
//}
//	
//float Cods()
//{
//	char value_buf[N];
//	char count,i,j,cod;
//	
//	for(count=0;count<N;count++)
//	{
//		value_buf[count]=Cod.F_data;
//		DelayXms(50);
//	}
//	for(j=0;j<N-1;j++)
//	{
//		for(i=0;i<N-j;i++)
//		{
//			if(value_buf[i]>value_buf[i+1])
//			{
//				cod=value_buf[i];
//				value_buf[i]=value_buf[i+1];
//				value_buf[i+1]=cod;
//			}
//		}
//		
//	}
//	return value_buf[(N-1)/2];
//}

//float Tocs()
//{
//	char value_buf[N];
//	char count,i,j,toc;
//	for(count=0;count<N;count++)
//	{
//		value_buf[count]=Toc.F_data;
//		DelayXms(50);
//	}
//	for(j=0;j<N-1;j++)
//	{
//		for(i=0;i<N-j;i++)
//		{
//			if(value_buf[i]>value_buf[i+1])
//			{
//				toc=value_buf[i];
//				value_buf[i]=value_buf[i+1];
//				value_buf[i+1]=toc;
//			}
//		}
//		
//	}
//	return value_buf[(N-1)/2];
//}

//float Ntus()
//{
//	float value_buf[N];
//	char count,i,j,Ntu;
//	for(count=0;count<N;count++)
//	{
//		value_buf[count]=NTU.F_data;
//		DelayXms(50);
//	}
//	for(j=0;j<N-1;j++)
//	{
//		for(i=0;i<N-j;i++)
//		{
//			if(value_buf[i]>value_buf[i+1])
//			{
//				Ntu=value_buf[i];
//				value_buf[i]=value_buf[i+1];
//				value_buf[i+1]=Ntu;
//			}
//		}
//		
//	}
//	return (float)value_buf[(N-1)/2];
//}

//float Tss()
//{
//	float value_buf[N];
//	char count,i,j,Tss;
//	for(count=0;count<N;count++)
//	{
//		value_buf[count]=Turbidity.F_data;
//		DelayXms(50);
//	}
//	for(j=0;j<N-1;j++)
//	{
//		for(i=0;i<N-j;i++)
//		{
//			if(value_buf[i]>value_buf[i+1])
//			{
//				Tss=value_buf[i];
//				value_buf[i]=value_buf[i+1];
//				value_buf[i+1]=Tss;
//			}
//		}
//		
//	}
//	return (float)value_buf[(N-1)/2];
//}




unsigned char OneNet_FillBuf(char *buf)
{
	uint16_t temp0=0,temp1=0;
	
	char text[64];
	double ph;
	double tds;
//	double CODr;
//	double TOCr;
  double Bod;
	
	temp0 = (ADC_ConvertedValue[0]&0XFFFF0000) >> 16;
	temp1 = (ADC_ConvertedValue[0]&0XFFFF);
	
	ADC_ConvertedValueLocal[1] =(float) temp1/4096*3.3;
	ADC_ConvertedValueLocal[0] =(float) temp0/4096*3.3;	
	
	ph=-5.9647*ADC_ConvertedValueLocal[1]+20.255;
  tds=(153.42*ADC_ConvertedValueLocal[0]*ADC_ConvertedValueLocal[0]*ADC_ConvertedValueLocal[0]-255.86*ADC_ConvertedValueLocal[0]*ADC_ConvertedValueLocal[0]+857.39*ADC_ConvertedValueLocal[0])*1;

//	CODr=0.981*Cods()+0.013;
//	TOCr=0.40009*CODr;
  Bod=0.4*Cod.F_data;


  memset(text, 0, sizeof(text));
	strcpy(buf, ",;");
	
	memset(text, 0, sizeof(text));
	sprintf(text, "Tempreture,%.2f;",Temp.F_data);
	strcat(buf, text);
	
  memset(text, 0, sizeof(text));
	sprintf(text, "PH,%.2f;",ph);
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "TDS,%.2f;",tds);
	strcat(buf, text);     
	
	memset(text, 0, sizeof(text));
	sprintf(text, "Cod,%.2f;",Cod.F_data);
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "Toc,%.2f;",Toc.F_data);
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "NTU,%.2f;",NTU.F_data);
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "Bod,%.2f;",Bod); 
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
  sprintf(text, "Tss,%.2f;",Turbidity.F_data);
	strcat(buf, text);
	
	return strlen(buf);

}
	


//==========================================================
//	函数名称：	OneNet_SendData
//
//	函数功能：	上传数据到平台
//
//	入口参数：	type：发送数据的格式
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNet_SendData(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};												//协议包
	
	char buf[256];
	
	short body_len = 0, i = 0;
	
	UsartPrintf(USART_DEBUG, "Tips:	OneNet_SendData-MQTT\r\n");
	
	memset(buf, 0, sizeof(buf));
	
	body_len = OneNet_FillBuf(buf);																	//获取当前需要发送的数据流的总长度
	
	if(body_len)
	{
		if(MQTT_PacketSaveData(DEVID, body_len, NULL, 5, &mqttPacket) == 0)							//封包
		{
			for(; i < body_len; i++)
				mqttPacket._data[mqttPacket._len++] = buf[i];
			
			M6312_SendData(mqttPacket._data, mqttPacket._len);										//上传数据到平台
			UsartPrintf(USART_DEBUG, "Send %d Bytes\r\n", mqttPacket._len);
			
			MQTT_DeleteBuffer(&mqttPacket);															//删包
		}
		else
			UsartPrintf(USART_DEBUG, "WARN:	MQTT_NewBuffer Failed\r\n");
	}
	
}

//==========================================================
//	函数名称：	OneNet_RevPro
//
//	函数功能：	平台返回数据检测
//
//	入口参数：	dataPtr：平台返回的数据
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNet_RevPro(unsigned char *cmd)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};								//协议包
	
	char *req_payload = NULL;
	char *cmdid_topic = NULL;
	
	unsigned short req_len = 0;
	
	unsigned char type = 0;
	
	short result = 0;

	char *dataPtr = NULL;
	char numBuf[10];
	int num = 0;
	
	type = MQTT_UnPacketRecv(cmd);
	switch(type)
	{
		case MQTT_PKT_CMD:															//命令下发
			
			result = MQTT_UnPacketCmd(cmd, &cmdid_topic, &req_payload, &req_len);	//解出topic和消息体
			if(result == 0)
			{
				UsartPrintf(USART_DEBUG, "cmdid: %s, req: %s, req_len: %d\r\n", cmdid_topic, req_payload, req_len);
				
				if(MQTT_PacketCmdResp(cmdid_topic, req_payload, &mqttPacket) == 0)	//命令回复组包
				{
					UsartPrintf(USART_DEBUG, "Tips:	Send CmdResp\r\n");
					
					M6312_SendData(mqttPacket._data, mqttPacket._len);				//回复命令
					MQTT_DeleteBuffer(&mqttPacket);									//删包
				}
			}
		
		break;
			
		case MQTT_PKT_PUBACK:														//发送Publish消息，平台回复的Ack
		
			if(MQTT_UnPacketPublishAck(cmd) == 0)
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Publish Send OK\r\n");
			
		break;
		
		default:
			result = -1;
		break;
	}
	
	M6312_Clear();										//清空缓存
	
	if(result == -1)
		return;
	
	dataPtr = strchr(req_payload, '}');					//搜索'}'

	if(dataPtr != NULL && result != -1)					//如果找到了
	{
		dataPtr++;
		
		while(*dataPtr >= '0' && *dataPtr <= '9')		//判断是否是下发的命令控制数据
		{
			numBuf[num++] = *dataPtr++;
		}
		
		num = atoi((const char *)numBuf);				//转为数值形式
	}

	if(type == MQTT_PKT_CMD || type == MQTT_PKT_PUBLISH)
	{
		MQTT_FreeBuffer(cmdid_topic);
		MQTT_FreeBuffer(req_payload);
	}

}

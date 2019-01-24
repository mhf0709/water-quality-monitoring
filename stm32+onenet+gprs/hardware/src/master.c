#include "master.h"
#include "delay.h"
#include "crc16.h"

#define modbus_time 40

u8 RS485_RX_BUFF[100];												//接收缓冲区100字节
u16 RS485_RX_CNT=0;														//接收计数器
u8 RS485_RxFlag=0;														//接收一帧结束标记

u8 RS485_TX_BUFF[8];													//发送缓冲区
u16 RS485_TX_CNT=0;														//发送计数器
u8 RS485_TxFlag=0;														//发送一帧结束标记


u8 TX_RX_SET=0; //发送，接受命令切换。 0 发送模式 1接收模式
u8 ComErr=8; //0代表通讯正常
             //1代表CRC错误
						// 2代表功能码错误

RS_485_DATA Temp,Cod,Toc,NTU;		//温度    COD  TOC  浊度

RS_485_DATA Turbidity;	//污浊度


//初始化USART3
void RS485_Init(u32 bound)
{
        GPIO_InitTypeDef GPIO_InitStructure;
        USART_InitTypeDef USART_InitStructure;
        NVIC_InitTypeDef NVIC_InitStructure;
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO,ENABLE);					//使能GPIOB时钟
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);					//使能串口3时钟
        
        GPIO_InitStructure.GPIO_Pin=GPIO_Pin_10;											//PB10（TX）复用推挽输出
        GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;
        GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
        GPIO_Init(GPIOB,&GPIO_InitStructure);
        GPIO_SetBits(GPIOB,GPIO_Pin_10);															//默认高电平
        
        GPIO_InitStructure.GPIO_Pin=GPIO_Pin_11;												//PB11（RX）输入上拉
        GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IN_FLOATING;   		//GPIO_Mode_IN_FLOATING(浮空输入)/////////////////////////////////////////////
        GPIO_Init(GPIOB,&GPIO_InitStructure);
        
//        GPIO_InitStructure.GPIO_Pin=GPIO_Pin_12;									//通用推挽输出->PB12（RE/DE）通用推挽输出//////////////////////////////////////////////////////////////////////
//        GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
//        GPIO_Init(GPIOB,&GPIO_InitStructure);
//        GPIO_ResetBits(GPIOB,GPIO_Pin_12);//默认接收状态
        
        USART_DeInit(USART3);																				//复位串口3
        USART_InitStructure.USART_BaudRate=bound;
        USART_InitStructure.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
        USART_InitStructure.USART_WordLength=USART_WordLength_8b;
        USART_InitStructure.USART_StopBits=USART_StopBits_1;
        USART_InitStructure.USART_Mode=USART_Mode_Rx|USART_Mode_Tx;								//收发模式

        USART_InitStructure.USART_Parity=USART_Parity_No;//无校验
        USART_Init(USART3,&USART_InitStructure);													//初始化串口3
        
        USART_ClearITPendingBit(USART3,USART_IT_RXNE);									//清楚串口3接受中断标志
        USART_ITConfig(USART3,USART_IT_RXNE,ENABLE);										//使能串口3接收中断
        
        NVIC_InitStructure.NVIC_IRQChannel=USART3_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority=2;
        NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
        NVIC_Init(&NVIC_InitStructure);
        
        USART_Cmd(USART3,ENABLE);																					//使能串口3
//        RS485_TX_EN=1;//默认为接收模式
        
        Timer7_Init();//定时器7初始化，用于监视空闲时间
}

///////////////////////////////////////////////////////////////////////////////////////////////
////定时器7初始化---功能：判断从机返回的数据是否接受完成

void Timer7_Init(void)
{
        TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
        NVIC_InitTypeDef NVIC_InitStructure;

        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE); //TIM7时钟使能 

        //TIM7初始化设置
				TIM_TimeBaseStructure.TIM_Period = modbus_time; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值，4ms，Tou=((arr+1)*(psc+1))/Tclk;Tclk:晶振，72000000
        TIM_TimeBaseStructure.TIM_Prescaler =7199; //设置用来作为TIMx时钟频率除数的预分频值 设置计数频率为10kHz
        TIM_TimeBaseStructure.TIM_ClockDivision = 0; //设置时钟分割:TDTS = Tck_tim
        TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
        TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位

        TIM_ITConfig( TIM7, TIM_IT_Update, ENABLE );//TIM7 允许更新中断

        //TIM7中断分组配置
        NVIC_InitStructure.NVIC_IRQChannel =TIM7_IRQn;  //TIM7中断
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;  //先占优先级2级
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //从优先级3级
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
        NVIC_Init(&NVIC_InitStructure);  //根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器                                                                  
}






///////////////////////////////////////////////////////////////////////////////////
void USART3_IRQHandler(void)//串口2中断服务程序
{
	   
        u8 res;
        u8 err;
	 
        if(USART_GetITStatus(USART3,USART_IT_RXNE)!=RESET)
        {
                if(USART_GetFlagStatus(USART3,USART_FLAG_NE|USART_FLAG_FE|USART_FLAG_PE)) err=1;//检测到噪音、帧错误或校验错误
                else err=0;
                res=USART_ReceiveData(USART3); 																	//读接收到的字节，同时相关标志自动清除
                
                if((RS485_RX_CNT<100)&&(err==0))
                {
                        RS485_RX_BUFF[RS485_RX_CNT]=res;
                        RS485_RX_CNT++;
                        
                        TIM_ClearITPendingBit(TIM7,TIM_IT_Update);								//清除定时器溢出中断
                        TIM_SetCounter(TIM7,0);																		//当接收到一个新的字节，将定时器7复位为0，重新计时（相当于喂狗）
                        TIM_Cmd(TIM7,ENABLE);																			//开始计时
                }
        }
}

/////////////////////////////////////////////////////////////////////////////////////

//用定时器7判断接收空闲时间，当空闲时间大于指定时间，认为一帧结束
//定时器7中断服务程序         
void TIM7_IRQHandler(void)
{                                                                   
        if(TIM_GetITStatus(TIM7,TIM_IT_Update)!=RESET)
        {
                TIM_ClearITPendingBit(TIM7,TIM_IT_Update);//清除中断标志
                TIM_Cmd(TIM7,DISABLE);//停止定时器
 //               RS485_TX_EN=1;//停止接收，切换为发送状态
                RS485_RxFlag=1;//置位帧结束标记
        }
}

////////////////////////////////////////////////////////////////////////////
//发送n个字节数据
//buff:发送区首地址
//len：发送的字节数
void RS485_SendData(u8 *buff,u8 len)
{ 
 //       RS485_TX_EN=1;//切换为发送模式
        while(len--)
        {
                while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==RESET);					//等待发送区为空
                USART_SendData(USART3,*(buff++));
        }
        while(USART_GetFlagStatus(USART3,USART_FLAG_TC)==RESET);								//等待发送完成
				TX_RX_SET=1;																													 //发送命令完成，定时器T4处理接收到的数据
	//			RS485_TX_EN=0;
}



//Modbus功能码03处理程序///////////////////////////////////////////////////////////////////////////////////////
//读保持寄存器
void Master_03_Slove(u8 board_adr,u8 function_03,u16 start_adress,u16 num)//传感器地址  功能码 寄存器起始地址 读取数量
{ 	
			u16 calCRC;
	
		RS485_TX_BUFF[0] = board_adr;														//传感器地址  
		RS485_TX_BUFF[1 ]= function_03;													//功能码 
    RS485_TX_BUFF[2] = HI(start_adress);  									//寄存器起始地址  高8位
    RS485_TX_BUFF[3] = LOW(start_adress); 									//寄存器起始地址  低8位
    RS485_TX_BUFF[4] = HI(num);															//读取数量 高8位
    RS485_TX_BUFF[5] = LOW(num);														//读取数量 低8位
    calCRC=CRC_Compute(RS485_TX_BUFF,6);										//生成CRC16校验码
    RS485_TX_BUFF[6]=(calCRC>>8)&0xFF;										//CRC高8位		在前
    RS485_TX_BUFF[7]=(calCRC)&0xFF;												//CRC低8位		在后
//	  RS485_TX_BUFF[7]=(calCRC>>8)&0xFF;											//CRC高8位	在后
//    RS485_TX_BUFF[6]=(calCRC)&0xFF;													//CRC低8位    在前
	
	RS485_SendData(RS485_TX_BUFF,8);	
}


void Master_Service(u8 board_adr,u8 function,u16 start_adress,u16 num)//传感器地址  功能码 寄存器起始地址 读取数量
{

	switch(function)
	{
		case 03:
				Master_03_Slove(board_adr,function,start_adress,num);
				break;
	}
}
#include "usart.h"
/////////////////////////////////////////////////////////////////////////////////////
//RS485服务程序，用于处理接收到的数据(请在主函数中循环调用)

void RS485_RX_Service(void)
{
		u16 calCRC;
        u16 recCRC;
        if(RS485_RxFlag==1)
        {
//#ifdef Debug
//	 printf("\r\n我接收到了一帧数据\r\n");
//#endif	
//#ifdef Debug
//	 printf("\r\n数据为\r\n");
//	usart1_SendData(RS485_RX_BUFF,19);
//#endif	
                if(RS485_RX_BUFF[0]==0x01||RS485_RX_BUFF[0]==0x02||RS485_RX_BUFF[0]==0x03||RS485_RX_BUFF[0]==0x04||RS485_RX_BUFF[0]==0x05||RS485_RX_BUFF[0]==0x06||RS485_RX_BUFF[0]==0x07|RS485_RX_BUFF[0]==0x08)//地址正确
                {
                        if((RS485_RX_BUFF[1]==01)||(RS485_RX_BUFF[1]==02)||(RS485_RX_BUFF[1]==03)||(RS485_RX_BUFF[1]==05)||(RS485_RX_BUFF[1]==06)||(RS485_RX_BUFF[1]==15)||(RS485_RX_BUFF[1]==16))//功能码正确
												{
                                        calCRC=CRC_Compute(RS485_RX_BUFF,RS485_RX_CNT-2);//计算所接收数据的CRC
                                        recCRC=RS485_RX_BUFF[RS485_RX_CNT-1]|(((u16)RS485_RX_BUFF[RS485_RX_CNT-2])<<8);//接收到的CRC(低字节在前，高字节在后)
                                        if(calCRC==recCRC)//CRC校验正确
                                        {
                                                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                                                switch(RS485_RX_BUFF[1])//根据不同的功能码进行处理
                                                {                                                                
																										case 03: //读多个寄存器															
																														Modbus_03_Solve();
																														break;                                                                                       
                                                }
																				}
                                        else//CRC校验错误
                                        {
												  ComErr=14;

                                        }                                                                                       
												}
                        else//功能码错误
                        {
//#ifdef Debug
//	 printf("\r\n我没有接到数据\r\n");
//	usart1_SendData(RS485_RX_BUFF,19);
//#endif
														if((RS485_RX_BUFF[1]==0x81)||(RS485_RX_BUFF[1]==0x82)||(RS485_RX_BUFF[1]==0x83)||(RS485_RX_BUFF[1]==0x85)||(RS485_RX_BUFF[1]==0x86)||(RS485_RX_BUFF[1]==0x8F)||(RS485_RX_BUFF[1]==0x90))
														{
															switch(RS485_RX_BUFF[2])
															{
																case 0x01:
																			ComErr=11;
																			break;
																case 0x02:
																			ComErr=12;
																			break;
																case 0x03:
																			ComErr=13;
																			break;
																case 0x04:
																			ComErr=14;
																			break;
																
															}
															TX_RX_SET=0; //命令完成	
														}
                        }
          }
                                
				RS485_RxFlag=0;//复位帧结束标志
				RS485_RX_CNT=0;//接收计数器清零 
				TX_RX_SET=0; //命令完成
        }                
}


//Modbus功能码03处理程序///////////////////////////////////////////////////////////////////////////////////////已验证程序OK
//读保持寄存器

void Modbus_03_Solve(void)			//接收处理
{
	switch(RS485_RX_BUFF[0])
	{
		case 1:
					if(RS485_RX_BUFF[2]==0x0E)			//温度  COD  TOC
					{
							Temp.dat[0]=RS485_RX_BUFF[3];			//温度
							Temp.dat[1]=RS485_RX_BUFF[4];
							Temp.dat[2]=RS485_RX_BUFF[5];
							Temp.dat[3]=RS485_RX_BUFF[6];
						
							Cod.dat[0]=RS485_RX_BUFF[7];			//COD
							Cod.dat[1]=RS485_RX_BUFF[8];
							Cod.dat[2]=RS485_RX_BUFF[9];
							Cod.dat[3]=RS485_RX_BUFF[10];
						
							Toc.dat[0]=RS485_RX_BUFF[11];			//TOC	
							Toc.dat[1]=RS485_RX_BUFF[12];
							Toc.dat[2]=RS485_RX_BUFF[13];
							Toc.dat[3]=RS485_RX_BUFF[14];
					}
					else if(RS485_RX_BUFF[2]==0x04)		    //浊度
					{
							NTU.dat[0]=RS485_RX_BUFF[3];			//温度
							NTU.dat[1]=RS485_RX_BUFF[4];
							NTU.dat[2]=RS485_RX_BUFF[5];
							NTU.dat[3]=RS485_RX_BUFF[6];
					}
						break;
		case 2:
					if(RS485_RX_BUFF[2]==0x08)
					{
						Turbidity.dat[0]=RS485_RX_BUFF[7];			//污浊度另一个传感器
						Turbidity.dat[1]=RS485_RX_BUFF[8];
						Turbidity.dat[2]=RS485_RX_BUFF[9];
						Turbidity.dat[3]=RS485_RX_BUFF[10];
					}
					break;
	}
		TX_RX_SET=0; //命令完成
}





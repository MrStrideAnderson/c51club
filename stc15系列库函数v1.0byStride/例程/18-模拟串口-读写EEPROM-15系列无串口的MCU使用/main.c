
/*------------------------------------------------------------------*/
/* --- STC MCU International Limited -------------------------------*/
/* --- STC 1T Series MCU RC Demo -----------------------------------*/
/* --- Mobile: (86)13922805190 -------------------------------------*/
/* --- Fax: 86-0513-55012956,55012947,55012969 ---------------------*/
/* --- Tel: 86-0513-55012928,55012929,55012966 ---------------------*/
/* --- Web: www.GXWMCU.com -----------------------------------------*/
/* --- QQ:  800003751 ----------------------------------------------*/
/* If you want to use the program or the program referenced in the  */
/* article, please specify in which data and procedures from STC    */
/*------------------------------------------------------------------*/



/*************	本程序功能说明	**************

				测试说明

通过串口对STC内部自带的EEPROM(FLASH)进行读写测试。

对FLASH做扇区擦除、写入、读出的操作，命令指定地址。

固定波特率:  9600,8,N,1. 
默认主时钟:  22118400HZ.

串口命令设置: (字母不区分大小写)
	E 0x0012 	        --> 对EEPROM进行扇区擦除操作，指定十六进制地址.
	W 0x0012 1234567890	--> 对EEPROM进行扇区写入操作，指定十六进制地址，. 后面为写入内容.
	R 0x0012 16	        --> 对EEPROM进行扇区读出操作，指定十六进制地址，. 

注意：为了通用，程序不识别地址是否有效，用户自己根据具体的型号来决定。

*/

#include	"config.h"
#include	"EEPROM.h"


/*************	本地常量声明	**************/


/*************	本地变量声明	**************/

#define Timer0_Reload		(65536 - MAIN_Fosc / BaudRate / 3)
#define RxLength		32		//接收缓冲长度
#define	Tmp_Length		32

sbit P_RXB = P3^0;			//定义模拟串口接收IO
sbit P_TXB = P3^1;			//定义模拟串口发送IO

u8	idata	tmp[Tmp_Length];

bit  TxBusyFlag;		//正在发送一个字节, 用户设置1开始发送, 发送完毕系统清自动0
bit	B_Rx_OK;	 		//接收完的标志位, 收到数据块系统设置1, 用户处理数据后必须清0
u8	Rx_Cnt;				//接收到的字节数, 用户处理数据后必须清0
u8  idata RxBuf[RxLength];	//接收缓冲
u8	TxSBUF;				//发送数据寄存器, 用户写入要发送的数据, 把TxBusyFlag设置为1,系统就会发送出去, 发送完成后系统把TxBusyFlag清0.

//===== 下面这些为系统使用的常量或变量, 用户不可见，请勿修改 =============

#define	RxBitLenth	9		//8个数据位+1个停止位
#define	TxBitLenth	9		//8个数据位+1个停止位
u8  TxShiftReg,RxShiftReg;	//发送 接收 移位
u8  TxSample,RxSample;		//发送和接收检测 采样计数器(3倍速率检测)
u8  TxBitCnt,RxBitCnt;		//发送和接收的数据 位计数器
u8	RxTimeOut;		//接收超时计数
bit	RxStartFlag;			//正在接收一个字节(已收到起始位)

//=====================================================================

/*************	本地函数声明	**************/
void	TxString(u8 *p);


/*************  外部函数和变量声明 *****************/




//========================================================================
// 函数: void UART_INIT(void)
// 描述: UART模块的初始变量.
// 参数: 无.
// 返回: 无.
// 版本: V1.0, 2012-10-22
//========================================================================
void UART_INIT(void)
{
	TxBusyFlag = 0;
	RxStartFlag = 0;
	TxSample = 3;
	RxSample = 4;
	RxTimeOut = 0;
	B_Rx_OK = 0;
	Rx_Cnt = 0;
}

//========================================================================
// 函数: u8	CheckData(u8 dat)
// 描述: 将字符"0~9,A~F或a~f"转成十六进制.
// 参数: dat: 要检测的字符.
// 返回: 0x00~0x0F为正确. 0xFF为错误.
// 版本: V1.0, 2012-10-22
//========================================================================
u8	CheckData(u8 dat)
{
	if((dat >= '0') && (dat <= '9'))		return (dat-'0');
	if((dat >= 'A') && (dat <= 'F'))		return (dat-'A'+10);
	return 0xff;
}

//========================================================================
// 函数: u16	GetAddress(void)
// 描述: 计算各种输入方式的地址.
// 参数: 无.
// 返回: 16位EEPROM地址.
// 版本: V1.0, 2013-6-6
//========================================================================
u16	GetAddress(void)
{
	u16	address;
	u8	i,j;
	
	address = 0;
	if((RxBuf[2] == '0') && (RxBuf[3] == 'X'))
	{
		for(i=4; i<8; i++)
		{
			j = CheckData(RxBuf[i]);
			if(j >= 0x10)	return 65535;	//error
			address = (address << 4) + j;
		}
		return (address);
	}
	return	65535;	//error
}

/**************** 获取要读出数据的字节数 ****************************/
u8	GetDataLength(void)
{
	u8	i;
	u8	length;
	
	length = 0;
	for(i=9; i<Rx_Cnt; i++)
	{
		if(CheckData(RxBuf[i]) >= 10)	break;
		length = length * 10 + CheckData(RxBuf[i]);
	}
	return (length);
}

//========================================================================
// 函数: void main(void)
// 描述: 主程序.
// 参数: 无.
// 返回: 无.
// 版本: V1.0, 2012-10-22
//========================================================================
void	TxByte(u8 dat)
{
	TxSBUF = dat;		//要发送的数据写入发送缓冲
	TxBusyFlag = 1;			//启动发送
	while(TxBusyFlag);		//等待发送完成
}

//========================================================================
// 函数: void main(void)
// 描述: 主程序.
// 参数: 无.
// 返回: 无.
// 版本: V1.0, 2012-10-22
//========================================================================
void main(void)
{
	u8	i,j;
	u16 addr;

	Timer0_1T();
	Timer0_AsTimer();
	Timer0_16bitAutoReload();
	Timer0_Load(Timer0_Reload);
	Timer0_InterruptEnable();
//	PT0 = 1;	//定时器0高优先级中断
	Timer0_Run();
	EA = 1;						//打开总中断					open global interrupt switch

	UART_INIT();				//UART模块的初始变量
	TxString("STC15F/L系列单片机EEPROM测试程序!\r\n");	//发送一串提示字符
	
	while (1)
	{
		if (B_Rx_OK)	//接收完的标志位, 收到数据块系统设置1, 用户处理数据后必须清0
		{
			F0 = 0;
			if(Rx_Cnt > 0)	//确认有数据
			{
		//		for(i=0; i<Rx_Cnt; i++)	TxByte(RxBuf[i]);	//把收到的数据原样返回,用于测试

				if((Rx_Cnt >= 8) && (RxBuf[1] == ' '))	//最短命令为8个字节
				{
					for(i=0; i<8; i++)
					{
						if((RxBuf[i] >= 'a') && (RxBuf[i] <= 'z'))	RxBuf[i] = RxBuf[i] - 'a' + 'A';	//小写转大写
					}
					addr = GetAddress();
					if(addr < 63488)	//限制在0~123扇区
					{
						if(RxBuf[0] == 'E')	//PC请求擦除一个扇区
						{
							EEPROM_SectorErase(addr);
							TxString("扇区擦除完成\r\n");
							F0 = 1;
						}

						else if((RxBuf[0] == 'W') && (Rx_Cnt >= 10) && (RxBuf[8] == ' '))	//写入N个字节
						{
							j = Rx_Cnt - 9;
							if(j > Tmp_Length)	j = Tmp_Length;	//越界检测
							for(i=0; i<j; i++)	tmp[i] = 0xff;
							i = EEPROM_read_Compare(addr,tmp,j);	//检测要写入的空间是否为空
							if(i > 0)	TxString("要写入的地址为非空,不能写入,请先擦除!\r\n");
							else
							{
								EEPROM_write_n(addr,&RxBuf[9],j);		//写N个字节
								i = EEPROM_read_Compare(addr,&RxBuf[9],j);	//比较写入的数据
								if(i == 0)
								{
									TxString("已写入");
									if(j >= 100)	{TxByte(j/100+'0');	j = j % 100;}
									if(j >= 10)		{TxByte(j/10+'0');	j = j % 10;}
									TxByte(j%10+'0');
									TxString("字节数据!\r\n");
								}
								else	TxString("写入错误!\r\n");
							}
							F0 = 1;
						}

						else if((RxBuf[0] == 'R') && (Rx_Cnt >= 10) && (RxBuf[8] == ' '))	//PC请求返回N字节EEPROM数据
						{
							j = GetDataLength();
							if(j > Tmp_Length)	j = Tmp_Length;	//越界检测
							if(j > 0)
							{
								TxString("读出");
								TxByte(j/10+'0');
								TxByte(j%10+'0');
								TxString("个字节数据如下：\r\n");
								EEPROM_read_n(addr,tmp,j);
								for(i=0; i<j; i++)	TxByte(tmp[i]);
								TxByte(0x0d);
								TxByte(0x0a);
								F0 = 1;
							}
						}
					}
				}
			}
			if(!F0)	TxString("命令错误!\r\n");
			Rx_Cnt  = 0;	//清除字节数
			B_Rx_OK = 0;	//清除接收完成标志
		}
	}
}



//========================================================================
// 函数: void	TxString(u8 *p)
// 描述: 发送一个数据块, 遇到0结束.
// 参数: *p: 数据块的指针.
// 返回: 无.
// 版本: V1.0, 2012-10-22
//========================================================================
void	TxString(u8 *p)
{
	for( ; *p > 0; p++)	TxByte(*p);
}


//========================================================================
// 函数: void tm0(void) interrupt 1
// 描述: 定时器0中断程序, for UART 以波特率3倍的速度采样判断 开始位.
// 参数: 无.
// 返回: 无.
// 版本: V1.0, 2012-10-22
//========================================================================

void tm0(void) interrupt 1
{
	//====================== 模拟串口接收程序 ========================================
	if (RxStartFlag)			//已接收到起始位
	{
		if (--RxSample == 0)			//接收数据以定时器的1/3来接收
		{
			RxSample = 3;               //重置接收计数器  接收数据以定时器的1/3来接收	reset send baudrate counter
			if (--RxBitCnt == 0)		//接收完一帧数据
			{
				RxStartFlag = 0;        //停止接收			stop receive
				if(P_RXB && !B_Rx_OK)	//确认停止位正确,并且缓冲已空
				{
					RxBuf[Rx_Cnt] = RxShiftReg;     //存储数据到缓冲区	save the data to RBUF
					if(++Rx_Cnt >= RxLength)	Rx_Cnt = 0;	//溢出判断
					RxTimeOut = 105;				//超时计数初值, 35个位的时间(对应5个字节), 参考MODBUS协议
				}
			}
			else
			{
				RxShiftReg >>= 1;			  //把接收的单b数据 暂存到 RxShiftReg(接收缓冲)
				if (P_RXB) RxShiftReg |= 0x80;  //shift RX data to RX buffer
			}
		}
	}

	else if (!P_RXB)		//判断是不是开始位 P_RXB=0;
	{
		RxStartFlag = 1;       //如果是则设置开始接收标志位 	set start receive flag
		RxSample = 4;       //初始化接收波特率计数器       	initial receive baudrate counter
		RxBitCnt = RxBitLenth;       //初始化接收的数据位数(8个数据位+1个停止位)    initial receive bit number (8 data bits + 1 stop bit)
	}
	if(RxTimeOut > 0)	//接收超时处理
	{
		if(--RxTimeOut == 0)	B_Rx_OK = 1;	//标志已收到一帧数据
	}

	//====================== 模拟串口发送程序 ========================================
	if (TxBusyFlag)			//发送开始标志位   judge whether sending
	{
		if (--TxSample == 0)			//发送数据以定时器的1/3来发送
		{
			TxSample = 3;				//重置发送计数器   reset send baudrate counter
			if (TxBitCnt == 0)			//发送计数器为0 表明单字节发送还没开始
			{
				P_TXB = 0;			//发送开始位     					send start bit
				TxShiftReg = TxSBUF;		//把缓冲的数据放到发送的buff		load data from TxSBUF to TxShiftReg
				TxBitCnt = TxBitLenth;	//发送数据位数 (8数据位+1停止位)	initial send bit number (8 data bits + 1 stop bit)
			}
			else					//发送计数器为非0 正在发送数据
			{
				if (--TxBitCnt == 0)	//发送计数器减为0 表明单字节发送结束
				{
					P_TXB = 1;			//送停止位数据
					TxBusyFlag = 0;	//发送停止位    			stop send
				}
				else
				{
					TxShiftReg >>= 1;		//把最低位送到 CY(益处标志位) shift data to CY
					P_TXB = CY;		//发送单b数据				write CY to TX port
				}
			}
		}
	}
}



/*---------------------------------------------------------------------*/
/* --- STC MCU International Limited ----------------------------------*/
/* --- STC 1T Series MCU Demo Programme -------------------------------*/
/* --- Mobile: (86)13922805190 ----------------------------------------*/
/* --- Fax: 86-0513-55012956,55012947,55012969 ------------------------*/
/* --- Tel: 86-0513-55012928,55012929,55012966 ------------------------*/
/* --- Web: www.GXWMCU.com --------------------------------------------*/
/* --- QQ:  800003751 -------------------------------------------------*/
/* 如果要在程序中使用此代码,请在程序中注明使用了宏晶科技的资料及程序   */
/*---------------------------------------------------------------------*/



/*************	本程序功能说明	**************

驱动LCD1602字符屏.

用户可以修改宏来选择时钟频率.

显示效果为: LCD显示时间.

使用Timer0的16位自动重装来产生1ms节拍,程序运行于这个节拍下, 用户修改MCU主时钟频率时,自动定时于1ms.

******************************************/

#include	"config.H"
#include	"LCD1602.h"
#include	"delay.h"


/*************	本地变量声明	**************/

u8	hour,minute,second;
u8	msecond;

/*************	本地函数声明	**************/


/****************  外部函数声明和外部变量声明 *****************/


/********************** 显示时钟函数 ************************/
void	DisplayRTC(void)
{
	WriteChar(2,5,hour / 10 + '0');
	WriteChar(2,6,hour % 10 +'0');
	WriteChar(2,7,'-');
	WriteChar(2,8,minute / 10+'0');
	WriteChar(2,9,minute % 10+'0');
	WriteChar(2,10,'-');
	WriteChar(2,11,second / 10 +'0');
	WriteChar(2,12,second % 10 +'0');
}

/********************** RTC演示函数 ************************/
void	RTC(void)
{
	if(++second >= 60)
	{
		second = 0;
		if(++minute >= 60)
		{
			minute = 0;
			if(++hour >= 24)	hour = 0;
		}
	}
}


/********************** 主函数 ************************/
void main(void)
{

	Initialize_LCD();
	ClearLine(1);
	ClearLine(2);

	PutString(1,1,"-- Clock demo --");
	
	hour   = 12;	//初始化时间值
	minute = 0;
	second = 0;
	DisplayRTC();
	
	while(1)
	{
		delay_ms(10);
		{
			if(++msecond >= 100)	//1秒到
			{
				msecond = 0;
				RTC();
				DisplayRTC();
			}

		}
	}
} 
/**********************************************/


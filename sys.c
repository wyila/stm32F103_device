#include "sys.h"

//********************************************************************************  
//THUMB指令不支持汇编内联
//采用如下方法实现执行汇编指令WFI  
__asm void WFI_SET(void)
{
	WFI;		  
}

__asm void INTX_DISABLE(void)
{
	CPSID I;		  
}

__asm void INTX_ENABLE(void)
{
	CPSIE I;		  
}

__asm void MSR_MSP(uint32_t addr) 
{
	MSR MSP, r0 			//set Main Stack value
	BX r14
}

//关CPU所有中断
__asm void CPU_IntDis (void)
{
	CPSID   I
	BX      LR
}

//开CPU所有中断
__asm void CPU_IntEn (void)
{
	CPSIE   I
	BX      LR
}


/*********************************
 *
 * author: wbl
 * date: 2022-04-13
 *
*********************************/
//这里要根据cpu主频来改，有时候外部晶振用的12M
//超频时候要把us = 8000改成us = 12000
void delay_ms(unsigned int ms)
{
    #ifdef INC_FREERTOS_H
    vTaskDelay(ms);
    #else
    int us;
    while(ms--)
    {
        us = 8000;
        while(us--);
    }
    #endif
}
//这里同上 i = 8 改成 i = 11
void delay_us(unsigned int us)
{
    
    int i = 8;
    while(us--)
    {
        i = 8;
        while(i--);
    }
    
    
}

#define HSE_CLOCK    8
//SYS_CLOCK: 选定的系统时钟频率36~72
//HSE_CLOCK: 外部晶振频率，有时候不一定是8MHz
void Stm32_Clock_Init(unsigned char SYS_CLOCK)
{
	unsigned char temp = 0;
    unsigned char PLL;
	
	//预判一波
	if(SYS_CLOCK >= 8000000)
		PLL = ((SYS_CLOCK / 1000000) / HSE_CLOCK) - 2;
	else
		PLL = (SYS_CLOCK / HSE_CLOCK) - 2;
	if(PLL > 7)
	{
		SystemInit();
		return;
	}
	
    //复位
    RCC->APB1RSTR = 0x00000000;//复位结束
	RCC->APB2RSTR = 0x00000000;
  	RCC->AHBENR = 0x00000014;  //睡眠模式闪存和SRAM时钟使能.其他关闭
  	RCC->APB2ENR = 0x00000000; //外设时钟关闭
  	RCC->APB1ENR = 0x00000000;
	RCC->CR |= 0x00000001;     //使能内部高速时钟HSION
	RCC->CFGR &= 0xF8FF0000;   //复位SW[1:0],HPRE[3:0],PPRE1[2:0],PPRE2[2:0],ADCPRE[1:0],MCO[2:0]
	RCC->CR &= 0xFEF6FFFF;     //复位HSEON,CSSON,PLLON
	RCC->CR &= 0xFFFBFFFF;     //复位HSEBYP
	RCC->CFGR &= 0xFF80FFFF;   //复位PLLSRC, PLLXTPRE, PLLMUL[3:0] and USBPRE
	RCC->CIR = 0x00000000;     //关闭所有中断
    
    //配置向量表
    SCB->VTOR = 0x08000000;//设置NVIC的向量表偏移寄存器
    
    
 	RCC->CR|=0x00010000;  //外部高速时钟使能HSEON
	while(!(RCC->CR>>17));//等待外部时钟就绪
	RCC->CFGR=0X00000400; //APB1=DIV2;APB2=DIV1;AHB=DIV1;
	RCC->CFGR|=PLL<<18;   //设置PLL值 2~16
	RCC->CFGR|=1<<16;	  //PLLSRC ON
	FLASH->ACR|=0x32;	  //FLASH 2个延时周期
	RCC->CR|=0x01000000;  //PLLON
	while(!(RCC->CR>>25));//等待PLL锁定
	RCC->CFGR|=0x00000002;//PLL作为系统时钟
	while(temp!=0x02)     //等待PLL作为系统时钟设置成功
	{   
		temp=RCC->CFGR>>2;
		temp&=0x03;
	}    
}

//使用printf函数会调用到这个函数
//要包含stdio.h
//这里用的串口二，将来使用其他外设在这里更改
int fputc(int ch, FILE *stream)
{
    while((USART2 -> SR & 0x40) == 0);
    USART2 -> DR = ch;
    return ch;
}
/**************** end *******************/






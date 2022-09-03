#include "sys.h"

volatile unsigned int SysTime = 0;//系统心跳

/************************************************************
 *
 * Author: wbl
 * 
 * Version: V1.0
 * Date: 2022-04-13
 *
 * Version: V1.1
 * Date: 2022-09-03
 * Log: 添加延时初始化、引入"心跳"概念，往后的程序及其依赖这个心跳，
 *      延时函数由原来消耗CPU时间更改为使用Systick定时器进行延时，
 *      通用性拉满，理论上只要是ARM芯片都能用，不受程序优化等级影响。
 * Author's words: 哈哈哈，已经过去这么久了，真怀念那段时光。我还
 *                 以为，这份代码再也用不上
 * 
*************************************************************/
//延时函数依靠滴答定时器(SysTick)运行，要注意SysTick是递减定时器！！！！！！

//延时1微秒的基数
unsigned short int fac_us = 0;

//延时1毫秒的基数
unsigned short int fac_ms = 0;

//初始化Systick定时器，延时函数理论上只要是ARM内核都能用
void delay_init(void)
{
    SysTime = 0;
    //配置滴答定时器
    SysTick_Config(SystemCoreClock / 1000);//1ms中断一次
    fan_ms = SysTick->LOAD;//取基数
    if(fan_ms > 1000)
        fan_us = fan_ms / 1000;
    else
        fan_us = 1;
}


void delay_ms(unsigned int ms)
{
    unsigned int first_val = SysTick->VAL;//记录刚进来的时候定时器的值
    unsigned int delay = ms + SysTime;
    
    if(ms == 0)
        return;
    ms += SysTime;
    
    //不正常情况（SysTime这个变量总有溢出的一天，虽说是小概率。或者说你并没有开启滴答定时器）
    if(ms < SysTime)
        while(ms < SysTime);//等待SysTime归零
    //下面是正常情况
    //等待延时
    while(ms > SysTime);
    //延时差不多达标，但又没完全达标
    while(1)
    {//既然程序能走到这里，SysTick->VAL的值一定等于fan_ms，或者稍微比它小一点
        if((SysTick->VAL + first_val) <= fam_ms)//差就差在这里
            break;//即使没有初始化定时器也能退出
    }
}

//要求：系统时钟大于10MHz
//微秒延时
void delay_us(unsigned int us)
{
    unsigned int first_val = SysTick->VAL;//记录刚进来的时候定时器的值
    unsigned int ms = 0;
    
    if(us == 0)
        return;
    
    if(us >= 1000)//超过1ms
    {
        ms += (us / 1000 + Systime);
        us %= 1000;//求余
        delay_ms(ms);//机智
        ms = 1;//标志一下
    }
    
    us *= fan_us;//取滴答定时器延时x us的实际值
    if(us > first_val)
    {
        ms = SysTime + 1;
        while(ms <= SysTime);//等待定时器重置
        first_val = fan_ms;//这里fan_ms用SysTick->LOAD更标准
    }
    //到了这里SysTick->Val一定不会比us小
    us = first_val - us;//定时器计数到这，就表示延时完成
    while(SysTick->VAL > us)//等待最后延时
        if(SysTick->VAL > first)//小概率
            break;
    //10 10 0
}

//滴答定时器中断函数
void SysTick_Handler(void)
{
    SysTime++;
}


/************************ end **************************/

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






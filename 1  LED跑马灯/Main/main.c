#include "led.h"
#include "../USER/KEY/key.h"
#include "../USER/TIMER/timer.h"
#include "../USER/BUZZER/buzzer.h"


int main(void)
{ 
	u8 temp_keyresult = 0;
	HAL_Init();                    	//初始化HAL库    
	Stm32_Clock_Init(336,8,2,7);  	//设置时钟,168Mhz
	delay_init();		  //初始化延时函数
	LED_Init();		    //初始化LED端口
	key_hardware_init();	//初始化KEY端口
	timer6_software_init();	//初始化定时器
	
	while(1)
	{
		
	}
}






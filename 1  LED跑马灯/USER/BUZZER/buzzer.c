#include "buzzer.h" 


//LED对应IO初始化
void LED_Init(void)
{    	 
    GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_GPIOE_CLK_ENABLE();           //开启GPIOE时钟
	  __HAL_RCC_GPIOG_CLK_ENABLE();           //开启GPIOE时钟
	
    GPIO_Initure.Pin=GPIO_PIN_3|GPIO_PIN_4; //PF9,10
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //推挽输出
    GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;     //高速
    HAL_GPIO_Init(GPIOE,&GPIO_Initure);
	
	  GPIO_Initure.Pin=GPIO_PIN_9; //
    HAL_GPIO_Init(GPIOG,&GPIO_Initure);	
	
    HAL_GPIO_WritePin(GPIOE,GPIO_PIN_3,GPIO_PIN_SET);	//PE3置1，默认初始化后灯灭
    HAL_GPIO_WritePin(GPIOE,GPIO_PIN_4,GPIO_PIN_SET);	//PE4置1，默认初始化后灯灭
	  HAL_GPIO_WritePin(GPIOG,GPIO_PIN_9,GPIO_PIN_SET);	//PG9置1，默认初始化后灯灭  启*明*欣*欣
}







#include "key.h"

/*
	这里的目的是写一个通用的按键驱动程序出来，方便以后
	进行使用移植。
	这里假定的是没有经过电容滤波的按键
*/

GPIO_InitTypeDef key_gpio_init_struct;


/*
	按键硬件初始化
*/
void key_hardware_init(void)
{
	//时钟初始化
	__HAL_RCC_GPIOF_CLK_ENABLE();
	key_gpio_init_struct.Mode = GPIO_MODE_INPUT;
	key_gpio_init_struct.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
	key_gpio_init_struct.Pull = GPIO_PULLUP;
	key_gpio_init_struct.Speed = GPIO_SPEED_FAST;
	HAL_GPIO_Init(GPIOF,&key_gpio_init_struct);
}

/*
	按键软件初始化
*/
void key_software_init(void)
{
	
}



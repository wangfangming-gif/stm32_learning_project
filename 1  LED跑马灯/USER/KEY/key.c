#include "key.h"

/*
	这里的目的是写一个通用的按键驱动程序出来，方便以后
	进行使用移植。
	这里假定的是没有经过电容滤波的按键
*/

GPIO_InitTypeDef key_gpio_init_struct;


#define MAX_DOUBLE_THRESHOLD 50					//按键双击间隔
#define MAX_SIGNAL_DEBOUNCE_THRESHOLD 3	//消抖间隔


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
	key_params[0].key_port = GPIOE;
	key_params[0].key_gpio_pin = GPIO_PIN_3;
	key_params[0].key_activate_power = GPIO_PIN_RESET;		//按下是低电平

	key_params[0].key_current_state = key_state_none;					//当前按键状态
	key_params[0].key_current_pin_state = key_pin_up;	//当前按键的电平状态

	key_params[0].key_debounce_count = 0;				//消抖次数
	key_params[0].key_debounce_threshold = MAX_SIGNAL_DEBOUNCE_THRESHOLD;		//消抖阈值

	key_params[0].key_double_state = 0;					//按键状态，1代表按下一次，0代表按下0次
	key_params[0].key_double_debounce_count = 0;			//双击间隔计数
	key_params[0].key_double_debounce_threhold = MAX_DOUBLE_THRESHOLD;		//双击最大间隔

}





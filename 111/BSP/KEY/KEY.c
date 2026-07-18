

#include "../BSP/KEY/KEY.h"

#include <stdio.h>


typedef enum 
{
	key_state_none = 1,		//无
	key_state_down = 1<<1,		//按下
	key_state_up = 1<<2,			//弹起
	key_state_click = 1<<3,	//点击
	key_state_double_click = 1<<4,		//双击
}key_state;

typedef enum
{
	key_pin_down,
	key_pin_up,
}key_pin_state;

typedef struct
{
	GPIO_TypeDef * key_port;
	uint16_t key_gpio_pin;
	uint8_t key_activate_power;

	uint8_t key_current_state;					//当前按键状态
	key_pin_state key_current_pin_state;	//当前按键的电平状态
	
	uint8_t key_debounce_count;				//消抖次数
	uint8_t key_debounce_threshold;		//消抖阈值

	uint8_t key_double_state;					//按键状态，1代表按下一次，0代表按下0次
	uint8_t key_double_debounce_count;			//双击间隔计数
	uint8_t key_double_debounce_threhold;		//双击最大间隔
}key_struct_params;



#define MAX_DOUBLE_THRESHOLD 30						//按键双击间隔
#define MAX_SIGNAL_DEBOUNCE_THRESHOLD 2		//消抖间隔
#define KEY_NUMBER 4											//按键数量

GPIO_InitTypeDef led_gpio_init_struct;

//按键参数结构体初始化
key_struct_params key_params[KEY_NUMBER];

static uint8_t get_key_state(uint8_t index);
static uint8_t read_key_down_state(uint8_t index);
static void key_params_register(uint8_t index,GPIO_TypeDef * temp_port,uint16_t temp_pin,
													uint8_t temp_activate_power);
void key_soft_init(void);
void key_gpio_init(void);


static key_click_callback_t key_click_callback = NULL;
static key_click_callback_t key_double_click_callback = NULL;

void key_click_register_task(key_click_callback_t callback)
{
	key_click_callback = callback;
}
void key_double_click_register_task(key_click_callback_t callback)
{
	key_double_click_callback = callback;
}


//按键初始化
void key_init(void)
{
	key_gpio_init();
	key_soft_init();
}

//初始化一下KEY的GPIO引脚
void key_gpio_init(void)
{
		KEY1_RCC_ENABLE;
		KEY2_RCC_ENABLE;
		KEY3_RCC_ENABLE;
		KEY4_RCC_ENABLE;
		
    led_gpio_init_struct.Mode = GPIO_MODE_INPUT;    //输入模式
    led_gpio_init_struct.Pull = GPIO_PULLUP;
    led_gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;

		led_gpio_init_struct.Pin = KEY1_PIN;
		HAL_GPIO_Init(KEY1_PORT,&led_gpio_init_struct);

		led_gpio_init_struct.Pin = KEY2_PIN;
		HAL_GPIO_Init(KEY2_PORT,&led_gpio_init_struct);

		led_gpio_init_struct.Pin = KEY3_PIN;
		HAL_GPIO_Init(KEY3_PORT,&led_gpio_init_struct);

		led_gpio_init_struct.Pin = KEY4_PIN;
		HAL_GPIO_Init(KEY4_PORT,&led_gpio_init_struct);
}

/**
 * Key软件参数初始化
 */
void key_soft_init(void)
{
	key_params_register(1,KEY1_PORT,KEY1_PIN,KEY1_ACTIVATE_POWER);
	key_params_register(2,KEY2_PORT,KEY2_PIN,KEY2_ACTIVATE_POWER);
	key_params_register(3,KEY3_PORT,KEY3_PIN,KEY3_ACTIVATE_POWER);
	key_params_register(4,KEY4_PORT,KEY4_PIN,KEY4_ACTIVATE_POWER);
}


static void key_params_register(uint8_t index,
													GPIO_TypeDef * temp_port,
													uint16_t temp_pin,
													uint8_t temp_activate_power)
{
	if((index <= 0) || (index > KEY_NUMBER))
	{
		return;
	}
	//KEY0按键	
	key_params[index-1].key_port = temp_port;
	key_params[index-1].key_gpio_pin = temp_pin;
	key_params[index-1].key_activate_power = temp_activate_power;		//这个代表按下电平是什么

	key_params[index-1].key_current_state = key_state_none;					//当前按键状态
	key_params[index-1].key_current_pin_state = key_pin_up; 				//这个代表按键处于的状态，up就是按键松开,down就是按键按下

	key_params[index-1].key_debounce_count = 0;				//消抖次数
	key_params[index-1].key_debounce_threshold = MAX_SIGNAL_DEBOUNCE_THRESHOLD;		//消抖阈值

	key_params[index-1].key_double_state = 0;					//按键状态，1代表按下一次，0代表按下0次
	key_params[index-1].key_double_debounce_count = 0;			//双击间隔计数
	key_params[index-1].key_double_debounce_threhold = MAX_DOUBLE_THRESHOLD;		//双击最大间隔
}



/*
	获得当前按下状态，1代表按下，0代表释放，-1代表错误
*/
static uint8_t read_key_down_state(uint8_t index)
{
	uint8_t temp_state = HAL_GPIO_ReadPin(key_params[index].key_port,
																				key_params[index].key_gpio_pin);
	if(key_params[index].key_activate_power == GPIO_PIN_RESET)		//如果按下之后是低电平
	{
		if(temp_state == GPIO_PIN_RESET)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else			//如果按下之后是高电平
	{
		if(temp_state == GPIO_PIN_RESET)	
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}
}


/*
	在定时函数中，10ms扫描一次
*/
void bsp_key_10ms_scan(void)
{
	uint8_t my_key_state = 0;
	uint8_t temp_index = 0;

	for(temp_index =0;temp_index<KEY_NUMBER;temp_index++)
	{
		my_key_state = 0;
		my_key_state = get_key_state(temp_index);

		//单击事件
		if(my_key_state & key_state_click)
		{
			if(key_click_callback != NULL)
			{
				key_click_callback(temp_index);
			}
			key_params[temp_index].key_current_state &= ~key_state_click;
		}
		//双击事件
		if(my_key_state & key_state_double_click)
		{
			if(key_double_click_callback != NULL)
			{
				key_double_click_callback(temp_index);
			}
			key_params[temp_index].key_current_state &= ~key_state_double_click;
		}

		if(my_key_state & key_state_down) //按下
		{
			key_params[temp_index].key_current_state &= ~key_state_down;
		}
		if(my_key_state & key_state_up)	//弹起
		{
			key_params[temp_index].key_current_state &= ~key_state_up;
		}
	}
}

/*
	更新按键状态
*/
static uint8_t get_key_state(uint8_t index)
{

	uint8_t temp_state = 0;

	temp_state = read_key_down_state(index);

	if(temp_state == 1)		//如果按下了
	{
		if(key_params[index].key_current_pin_state == key_pin_up)	//如果之前状态是抬起状态的话
		{

			key_params[index].key_debounce_count++;


			if(key_params[index].key_debounce_count >=
			   key_params[index].key_debounce_threshold)		//等待稳定
			{
				key_params[index].key_debounce_count = 0;

				//更新稳定状态
				key_params[index].key_current_pin_state = key_pin_down;	//稳定后，将其设置为按下状态

				//产生按下事件
				key_params[index].key_current_state |= key_state_down;	//按下事件
			}
		}
		else	//如果之前状态是按下状态的话
		{
			key_params[index].key_debounce_count = 0;		//继续清空数据
		}
	}
	else	//如果当前按键电平是释放状态
	{
		if(key_params[index].key_current_pin_state == key_pin_down)	//如果之前状态是按下
		{

			key_params[index].key_debounce_count++;


			if(key_params[index].key_debounce_count >=
			   key_params[index].key_debounce_threshold)	//等待稳定
			{
				key_params[index].key_debounce_count = 0;
				//更新稳定状态
				key_params[index].key_current_pin_state = key_pin_up;	//变成抬起状态
				key_params[index].key_current_state |= key_state_up;	//发生抬起事件

				//如果使用双击检测的话
				if(KEY_DOUBLE_CLICK_ON == 1)
				{
					if(key_params[index].key_double_state == 0)
					{
						//第一次释放
						key_params[index].key_double_state = 1;
						key_params[index].key_double_debounce_count = 0;
					
					}
					else if(key_params[index].key_double_state == 1)
					{
						//第二次释放
						key_params[index].key_double_state = 2;
					}
				}
				else	//如果不使用双击检测，那么这里需要判断下是否触发单击事件
				{
					if(KEY_CLICK_ON == 1)
					{
						key_params[index].key_current_state |= key_state_click;
					}
				}

			}
		}
		else		//如果之前状态就是抬起
		{
			key_params[index].key_debounce_count = 0;
		}
	}

	//如果使用双击的话
	if(KEY_DOUBLE_CLICK_ON == 1)
	{
		if(key_params[index].key_double_state == 1) //单击计时
		{
			if(key_params[index].key_current_pin_state == key_pin_up) //如果之前状态为抬起
			{
				key_params[index].key_double_debounce_count++;
				if(key_params[index].key_double_debounce_count >=
					 key_params[index].key_double_debounce_threhold)		//等待超时
				{
					key_params[index].key_double_state = 0;
					key_params[index].key_double_debounce_count = 0;
					//单击
					key_params[index].key_current_state |= key_state_click;
				}
			}
		}
		else if(key_params[index].key_double_state == 2)	//双击确认
		{
			key_params[index].key_double_state = 0;
			key_params[index].key_double_debounce_count = 0;
			key_params[index].key_current_state |= key_state_double_click;
		}
	}


	return key_params[index].key_current_state;
}



#ifndef __KEY_H
#define __KEY_H

#include "common.h"

typedef enum 
{
	key_state_none,		//无
	key_state_down,		//按下
	key_state_up,			//弹起
	key_state_click,	//点击
	key_state_double_click,		//双击
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
	uint8_t key_gpio_state;
	uint8_t key_activate_power;

	key_state key_current_state;					//当前按键状态
	key_pin_state key_current_pin_state;	//当前按键的电平状态
	
	uint8_t key_debounce_count;				//消抖次数
	uint8_t key_debounce_threshold;		//消抖阈值

	uint8_t key_double_state;					//按键状态，1代表按下一次，0代表按下0次
	uint8_t key_double_debounce_count;			//双击间隔计数
	uint8_t key_double_debounce_threhold;		//双击最大间隔
}key_struct_params;

//按键参数结构体初始化
key_struct_params key_params[4];


void key_hardware_init(void);
void key_software_init(void);


#endif

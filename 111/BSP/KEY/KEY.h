
#ifndef _KEY_H
#define _KEY_H

#include "stm32f0xx_hal.h"

//是否启用单击、双击的检测。
#define KEY_CLICK_ON 1
#define KEY_DOUBLE_CLICK_ON	0


typedef void (*key_click_callback_t)(uint8_t key_id);
typedef void (*key_double_click_callback_t)(uint8_t key_id);

/*
	这个地方顺序不能修改，从0开始递增下去
*/
typedef enum
{
	KEY1 = 0,
	KEY2 = 1,
	KEY3 = 2,
	KEY4 = 3,
}MYKEY;

#define KEY1_RCC_ENABLE  __HAL_RCC_GPIOB_CLK_ENABLE()
#define KEY1_PORT GPIOB
#define KEY1_PIN  GPIO_PIN_4
#define KEY1_ACTIVATE_POWER GPIO_PIN_RESET

#define KEY2_RCC_ENABLE  __HAL_RCC_GPIOB_CLK_ENABLE()
#define KEY2_PORT GPIOB
#define KEY2_PIN  GPIO_PIN_5
#define KEY2_ACTIVATE_POWER GPIO_PIN_RESET

#define KEY3_RCC_ENABLE  __HAL_RCC_GPIOB_CLK_ENABLE()
#define KEY3_PORT GPIOB
#define KEY3_PIN  GPIO_PIN_6
#define KEY3_ACTIVATE_POWER GPIO_PIN_RESET


#define KEY4_RCC_ENABLE  __HAL_RCC_GPIOB_CLK_ENABLE()
#define KEY4_PORT GPIOB
#define KEY4_PIN  GPIO_PIN_7
#define KEY4_ACTIVATE_POWER GPIO_PIN_RESET


void key_init(void);						//按键初始化
void bsp_key_10ms_scan(void);		//每10ms调用一次


//回调函数：按键单击
void key_click_register_task(key_click_callback_t callback);
//回调函数：按键双击
void key_double_click_register_task(key_click_callback_t callback);


#endif





#include "timer.h"
#include "led.h"


TIM_HandleTypeDef timer6_handle_struct;


/*
	使用Timer6做一个10ms的定时器
*/

void timer6_software_init(void)
{
	
	__HAL_RCC_TIM6_CLK_ENABLE();	//打开时钟
	
	timer6_handle_struct.Instance = TIM6;
	
	timer6_handle_struct.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	timer6_handle_struct.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;				//时钟分频
	timer6_handle_struct.Init.CounterMode = TIM_COUNTERMODE_UP;					//计数模式
	timer6_handle_struct.Init.Period = 100-1;							//
	timer6_handle_struct.Init.Prescaler = 8400-1;	
	HAL_TIM_Base_Init(&timer6_handle_struct);

	HAL_NVIC_SetPriority(TIM6_DAC_IRQn,1,1);
	HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
	
	//打开更新中断
	__HAL_TIM_ENABLE_IT(&timer6_handle_struct,TIM_IT_UPDATE);	
	__HAL_TIM_CLEAR_FLAG(&timer6_handle_struct,TIM_FLAG_UPDATE);
	HAL_TIM_Base_Start_IT(&timer6_handle_struct);
	
}

//TIM6的中断服务函数
void TIM6_DAC_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&timer6_handle_struct);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM6)	//定时器6
	{
		
	}
}


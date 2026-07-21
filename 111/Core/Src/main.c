/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "../BSP/RTT/SEGGER_RTT.h"
#include "../BSP/KEY/KEY.h"
#include "../BSP/LED/LED.h"
#include "../BSP/HT16220/ht16220.h"
#include "../BSP/LCD/segment_lcd_ui.h"
#include "../BSP/LCD/lcd_gpio.h"
#include "../BSP/FLASH/flash_param.h"
#include "../BSP/USART_YWY/usart_ywy.h"
#include "../BSP/USART_CONTROL/usart_control.h"

void key_click_handle(uint8_t key_id);
void key_double_click_handle(uint8_t key_id);
static void flash_read_params_init(void);



uint32_t high_oil_level_alarm_mm; 			//高油位报警
uint32_t high_oil_level_warn_mm;				//高油位预警
uint32_t low_oil_level_alram_mm;				//低油位报警
uint32_t low_oil_level_warn_mm; 				//低油位预警
uint32_t high_water_level_alarm_mm; 		//高水位报警
uint32_t ywy_open_alarm_flag;						//是否开启报警标志位,0代表不开启，1代表开启
uint32_t high_water_shield_mm;					//水屏蔽高度
uint32_t device_id; 										//设备地址
uint32_t device_length; 								//探杆长度
uint32_t oil_compensation;							//油位补偿
uint32_t water_compensation;						//水位补偿

float max_ywy_height = 2000;			//最大油高参数
float ywy_oil_height = 0; 		//液位仪油高
float ywy_water_height = 0; 	//液位仪水高
float ywy_temp1 = 0;	
float ywy_temp2 = 0;
float ywy_temp3 = 0;	
float ywy_temp4 = 0;
float ywy_temp5 = 0;	
float ywy_temp = 0; 					//液位仪温度

FlashParamData_t ywy_flash_params;
LCD_UI_Config_t ywy_show_params;

uint8_t timer_test_flag = 0;

void SystemClock_Config(void);

int main(void)
{
	uint32_t temp_data = 0;

	static uint8_t usart_printf_buffer[32];
	uint8_t usart_printf_count = 0;

  HAL_Init();
  SystemClock_Config();	
	flash_read_params_init();			//初始化flash
  MX_TIM17_Init();
  MX_TIM16_Init();
	TIM17_Start();	
	ht16220_gpio_init();
	key_init();	//初始化KEY
	led_init();	//初始化led
	LCD_UI_Init();
	usart_ywy_dma_init(9600);
	usart2_control_dma_init(9600);
	LCD_UI_SetConfig(&ywy_show_params);

	SEGGER_RTT_SetTerminal(0); 
	SEGGER_RTT_WriteString(0, "Project Start\r\n");
		
	key_click_register_task(key_click_handle);
	key_double_click_register_task(key_double_click_handle);
	
  while (1)
  {
		com_ywy_handle();
		uart2_ywy_send_check();
    my_usart2_receive_handle();
		
		LCD_UI_SetMeasurements((int32_t)(ywy_oil_height*10), 
													(int16_t)(ywy_temp*10), 
													(int32_t)(ywy_water_height*10));

		if(ywy_oil_height > max_ywy_height)	//如果液位仪高度大于最大油高
		{
			temp_data = 100;
		}
		else
		{
			temp_data = (uint32_t)((ywy_oil_height / max_ywy_height)*100);
		}
		
		LCD_UI_SetProgressPercent(temp_data);
		LCD_UI_Task();

		if(timer_test_flag == 1)
		{
			LED0_TOGGLE();
			LED1_TOGGLE();
			
			usart_printf_count = sprintf(usart_printf_buffer,"data:%d,%.2f,%.2f\r\n",temp_data,ywy_oil_height,max_ywy_height);
			SEGGER_RTT_SetTerminal(0); 
			SEGGER_RTT_Write(0, usart_printf_buffer,usart_printf_count);
			timer_test_flag = 0;
		}
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void key_click_handle(uint8_t key_id)
{
	if(key_id == KEY1)
	{
		SEGGER_RTT_SetTerminal(0); 
		SEGGER_RTT_WriteString(0, "Key1_CLICK\r\n");

	  LCD_UI_KeyEvent(LCD_KEY_UP);
	}
	if(key_id == KEY2)
	{
		SEGGER_RTT_SetTerminal(0); 
		SEGGER_RTT_WriteString(0, "Key2_CLICK\r\n");

		LCD_UI_KeyEvent(LCD_KEY_DOWN);
	}
	if(key_id == KEY3)
	{
		SEGGER_RTT_SetTerminal(0); 
		SEGGER_RTT_WriteString(0, "Key3_CLICK\r\n");

  	LCD_UI_KeyEvent(LCD_KEY_SET);
	}
	if(key_id == KEY4)
	{
		SEGGER_RTT_SetTerminal(0); 
		SEGGER_RTT_WriteString(0, "Key4_CLICK\r\n");
	}
}

void key_double_click_handle(uint8_t key_id)
{
	if(key_id == KEY1)
	{
		SEGGER_RTT_SetTerminal(0); 
		SEGGER_RTT_WriteString(0, "Key1_D_CLICK\r\n");
	}
	if(key_id == KEY2)
	{
		SEGGER_RTT_SetTerminal(0); 
		SEGGER_RTT_WriteString(0, "Key2_D_CLICK\r\n");
	}
	if(key_id == KEY3)
	{
		SEGGER_RTT_SetTerminal(0); 
		SEGGER_RTT_WriteString(0, "Key3_D_CLICK\r\n");
	}
	if(key_id == KEY4)
	{
		SEGGER_RTT_SetTerminal(0); 
		SEGGER_RTT_WriteString(0, "Key4_D_CLICK\r\n");
	}
}

static void flash_read_params_init(void)
{
	FlashParamInitResult_t result;
	FlashParam_Init(&ywy_flash_params);

	//设置下参数
	high_oil_level_alarm_mm = ywy_flash_params.flash_high_oil_level_alarm_mm; 			//高油位报警
	high_oil_level_warn_mm = ywy_flash_params.flash_high_oil_level_warn_mm;				//高油位预警
	low_oil_level_alram_mm = ywy_flash_params.flash_low_oil_level_alram_mm;				//低油位报警
	low_oil_level_warn_mm = ywy_flash_params.flash_low_oil_level_warn_mm; 				//低油位预警
	high_water_level_alarm_mm = ywy_flash_params.flash_high_water_level_alarm_mm; 		//高水位报警
	ywy_open_alarm_flag = ywy_flash_params.flash_ywy_open_alarm_flag; 					//是否开启报警标志位,0代表不开启，1代表开启
	high_water_shield_mm = ywy_flash_params.flash_high_water_shield_mm;					//水屏蔽高度
	device_id = ywy_flash_params.flash_device_id; 										//设备地址
	device_length = ywy_flash_params.flash_device_length; 								//探杆长度
	oil_compensation = ywy_flash_params.flash_oil_compensation;							//油位补偿
	water_compensation = ywy_flash_params.flash_water_compensation;						//水位补偿

	ywy_show_params.high_oil_alarm = high_oil_level_alarm_mm;
	ywy_show_params.high_oil_warning = high_oil_level_warn_mm;
	ywy_show_params.low_oil_alarm = low_oil_level_alram_mm;
	ywy_show_params.low_oil_warning = low_oil_level_warn_mm;
	ywy_show_params.high_water_alarm = high_water_level_alarm_mm;
	ywy_show_params.alarm_enable = ywy_open_alarm_flag;
	ywy_show_params.water_shield_height = high_water_shield_mm;
	ywy_show_params.device_address = device_id;
	ywy_show_params.probe_length = device_length;
	ywy_show_params.oil_compensation = oil_compensation;
	ywy_show_params.water_compensation = water_compensation;
}

//lcd退出设置之后的回调函数
void LCD_UI_ConfigChangedCallback(const LCD_UI_Config_t *config)
{
		//设置下参数
	ywy_flash_params.flash_high_oil_level_alarm_mm = config->high_oil_alarm; 			//高油位报警
	ywy_flash_params.flash_high_oil_level_warn_mm = config->high_oil_warning;				//高油位预警
	ywy_flash_params.flash_low_oil_level_alram_mm = config->low_oil_alarm;				//低油位报警
	ywy_flash_params.flash_low_oil_level_warn_mm = config->low_oil_warning; 				//低油位预警
	ywy_flash_params.flash_high_water_level_alarm_mm = config->high_water_alarm; 		//高水位报警
	ywy_flash_params.flash_ywy_open_alarm_flag = config->alarm_enable; 					//是否开启报警标志位,0代表不开启，1代表开启
	ywy_flash_params.flash_high_water_shield_mm = config->water_shield_height;					//水屏蔽高度
	ywy_flash_params.flash_device_id = config->device_address; 										//设备地址
	ywy_flash_params.flash_device_length = config->probe_length; 								//探杆长度
	ywy_flash_params.flash_oil_compensation = config->oil_compensation;							//油位补偿
	ywy_flash_params.flash_water_compensation = config->water_compensation;		

		//设置下参数
	high_oil_level_alarm_mm = ywy_flash_params.flash_high_oil_level_alarm_mm; 			//高油位报警
	high_oil_level_warn_mm = ywy_flash_params.flash_high_oil_level_warn_mm;				//高油位预警
	low_oil_level_alram_mm = ywy_flash_params.flash_low_oil_level_alram_mm;				//低油位报警
	low_oil_level_warn_mm = ywy_flash_params.flash_low_oil_level_warn_mm; 				//低油位预警
	high_water_level_alarm_mm = ywy_flash_params.flash_high_water_level_alarm_mm; 		//高水位报警
	ywy_open_alarm_flag = ywy_flash_params.flash_ywy_open_alarm_flag; 					//是否开启报警标志位,0代表不开启，1代表开启
	high_water_shield_mm = ywy_flash_params.flash_high_water_shield_mm;					//水屏蔽高度
	device_id = ywy_flash_params.flash_device_id; 										//设备地址
	device_length = ywy_flash_params.flash_device_length; 								//探杆长度
	oil_compensation = ywy_flash_params.flash_oil_compensation;							//油位补偿
	water_compensation = ywy_flash_params.flash_water_compensation;						//水位补偿

	//保存一下参数
	FlashParam_Save(&ywy_flash_params);
}



/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

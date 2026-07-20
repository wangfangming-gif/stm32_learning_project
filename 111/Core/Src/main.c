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

#include "../BSP/USART_YWY/usart_ywy.h"

void key_click_handle(uint8_t key_id);
void key_double_click_handle(uint8_t key_id);

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

uint8_t timer_test_flag = 0;

void SystemClock_Config(void);

int main(void)
{
  HAL_Init();

  SystemClock_Config();
//  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM17_Init();
  MX_TIM16_Init();
  /* USER CODE BEGIN 2 */	
	TIM17_Start();	
	ht16220_gpio_init();
	key_init();	//初始化KEY
	led_init();	//初始化led
	LCD_UI_Init();
	usart_ywy_dma_init(9600);

	SEGGER_RTT_SetTerminal(0); 
	SEGGER_RTT_WriteString(0, "Project Start\r\n");
		
	key_click_register_task(key_click_handle);
	key_double_click_register_task(key_double_click_handle);
	
	LCD_UI_SetMeasurements(763, 254, 128);
	/* 高液位报警阈值1234mm，报警功能关闭 */
	LCD_UI_SetHighLevelConfig(1234UL, false);
	/* 顶部进度条显示65% */
	LCD_UI_SetProgressPercent(65U);


  while (1)
  {
		com_ywy_handle();
		uart_ywy_send_check();
		LCD_UI_SetMeasurements(763, 254, 128);
		LCD_UI_Task();

		if(timer_test_flag == 1)
		{
			LED0_TOGGLE();
			LED1_TOGGLE();
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

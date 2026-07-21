/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f0xx_hal.h"
#include "../BSP/FLASH/flash_param.h"

void Error_Handler(void);

#define HIGH_OIL_LEVEL_ALARM_MM 0
#define HIGH_OIL_LEVEL_WARN_MM 0
#define LOW_OIL_LEVEL_ALRAM_MM 0
#define LOW_OIL_LEVEL_WARN_MM 0
#define HIGH_WATER_LEVEL_ALARM_MM 0
#define YWY_OPEN_ALARM_FLAG 0
#define HIGH_WATER_SHIELD_MM 30
#define DEVICE_ID 225
#define DEVICE_LENGTH 0
#define OIL_COMPENSATION 0
#define WATER_COMPENSATION 0

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

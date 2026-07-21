#ifndef __SEGMENT_LCD_UI_H
#define __SEGMENT_LCD_UI_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
  LCD_KEY_UP = 0,
  LCD_KEY_DOWN,
  LCD_KEY_SET
} LCD_KeyEvent_t;

typedef enum
{
  LCD_PAGE_MAIN = 0,
  LCD_PAGE_HIGH_OIL_ALARM,
  LCD_PAGE_HIGH_OIL_WARNING,
  LCD_PAGE_LOW_OIL_ALARM,
  LCD_PAGE_LOW_OIL_WARNING,
  LCD_PAGE_HIGH_WATER_ALARM,
  LCD_PAGE_ALARM_ENABLE,
  LCD_PAGE_WATER_SHIELD_HEIGHT,
  LCD_PAGE_DEVICE_ADDRESS,
  LCD_PAGE_PROBE_LENGTH,
  LCD_PAGE_OIL_COMPENSATION,
  LCD_PAGE_WATER_COMPENSATION,
  LCD_PAGE_COUNT
} LCD_Page_t;

/*
 * 所有需要保存到Flash的配置参数都使用uint32_t。
 * 数值界面显示范围为0~99999，alarm_enable只使用0或1。
 */
typedef struct
{
  uint32_t high_oil_alarm;
  uint32_t high_oil_warning;
  uint32_t low_oil_alarm;
  uint32_t low_oil_warning;
  uint32_t high_water_alarm;
  uint32_t alarm_enable;
  uint32_t water_shield_height;
  uint32_t device_address;
  uint32_t probe_length;
  uint32_t oil_compensation;
  uint32_t water_compensation;
} LCD_UI_Config_t;

typedef struct
{
  int32_t oil_height_x10;
  int32_t water_height_x10;
  int16_t temperature_x10;
  uint8_t progress_percent;
  LCD_UI_Config_t config;
} LCD_UI_Data_t;

void LCD_UI_Init(void);
void LCD_UI_Tick10ms(void);
void LCD_UI_Task(void);
void LCD_UI_KeyEvent(LCD_KeyEvent_t key);

void LCD_UI_SetMeasurements(int32_t oil_x10, int16_t temp_x10, int32_t water_x10);
void LCD_UI_SetProgressPercent(uint8_t percent);

/* 从Flash读取配置后，使用该函数一次性装载全部配置。 */
void LCD_UI_SetConfig(const LCD_UI_Config_t *config);
void LCD_UI_GetConfig(LCD_UI_Config_t *config);

/*
 * 兼容原工程接口：
 * threshold_mm写入高油位报警值，enable写入总报警开关。
 */
void LCD_UI_SetHighLevelConfig(uint32_t threshold_mm, bool enable);

void LCD_UI_GetData(LCD_UI_Data_t *data);
LCD_Page_t LCD_UI_GetPage(void);
bool LCD_UI_IsEditing(void);

/*
 * 用户按SET退出设置模式后调用。
 * 可以在用户自己的.c文件中实现同名函数，并在其中调用Flash保存函数。
 */
void LCD_UI_ConfigChangedCallback(const LCD_UI_Config_t *config);

#endif

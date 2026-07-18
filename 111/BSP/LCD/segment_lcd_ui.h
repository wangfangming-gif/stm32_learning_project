#ifndef __SEGMENT_LCD_UI_H
#define __SEGMENT_LCD_UI_H

#include <stdint.h>
#include <stdbool.h>

/*
 * 按键事件定义。
 * 按键扫描、消抖、长短按判断由用户现有按键模块完成，
 * LCD 模块只接收已经确认的“单击事件”。
 */
typedef enum
{
  LCD_KEY_UP = 0,      /* 非编辑状态：上一界面；编辑状态：当前值加 1/切换 ON-OFF */
  LCD_KEY_DOWN,        /* 非编辑状态：下一界面；阈值编辑状态：移动闪烁位 */
  LCD_KEY_SET          /* 进入或退出设置模式 */
} LCD_KeyEvent_t;

/* 三个显示界面。 */
typedef enum
{
  LCD_PAGE_MAIN = 0,               /* 主界面：油高、温度、水高 */
  LCD_PAGE_HIGH_LEVEL_VALUE,       /* 高油位报警阈值设置界面 */
  LCD_PAGE_HIGH_LEVEL_ENABLE,      /* 高油位报警启用/关闭界面 */
  LCD_PAGE_COUNT                   /* 页面数量，仅用于循环切换 */
} LCD_Page_t;

/* LCD 需要使用的全部业务数据。 */
typedef struct
{
  int32_t oil_height_x10;          /* 油高，单位 0.1mm；763 表示 76.3mm */
  int32_t water_height_x10;        /* 水高，单位 0.1mm */
  int16_t temperature_x10;         /* 温度，单位 0.1℃；当前屏幕按整数℃显示 */
  uint32_t high_level_mm;          /* 高油位报警阈值，单位 mm，范围 0~99999 */
  uint8_t progress_percent;        /* 顶部进度条百分比，范围 0~100 */
  bool high_level_enable;          /* true=ON，false=OFF */
} LCD_UI_Data_t;

/* 初始化界面状态和 HT16220。 */
void LCD_UI_Init(void);

/*
 * 每 10ms 调用一次，建议放在定时器中断。
 * 函数内部只做闪烁计数和置标志，不操作 GPIO，不会长时间占用中断。
 */
void LCD_UI_Tick10ms(void);

/*
 * 放在 main 的 while(1) 中持续调用。
 * 负责处理闪烁、重新生成显示缓存，并分步发送 LCD 数据。
 */
void LCD_UI_Task(void);

/* 用户按键模块产生单击事件后调用。 */
void LCD_UI_KeyEvent(LCD_KeyEvent_t key);

/* 更新主界面的油高、温度、水高。 */
void LCD_UI_SetMeasurements(int32_t oil_x10, int16_t temp_x10, int32_t water_x10);

/* 从 EEPROM/Flash 读取配置后，可调用该函数装载报警参数。 */
void LCD_UI_SetHighLevelConfig(uint32_t threshold_mm, bool enable);

/* 设置顶部进度条百分比，范围 0~100。 */
void LCD_UI_SetProgressPercent(uint8_t percent);

/* 获取当前界面模块中的完整数据副本。 */
void LCD_UI_GetData(LCD_UI_Data_t *data);

/* 获取当前页面。 */
LCD_Page_t LCD_UI_GetPage(void);

/* 判断当前是否处于设置模式。 */
bool LCD_UI_IsEditing(void);

/*
 * 配置改变回调。
 * 用户退出设置模式时调用，可在自己的工程中实现该函数，
 * 将阈值和开关状态保存到 EEPROM/Flash。
 * 默认实现为弱函数，什么也不做。
 */
void LCD_UI_ConfigChangedCallback(uint32_t threshold_mm, bool enable);


#endif

#ifndef __FLASH_PARAM_H
#define __FLASH_PARAM_H

#include "stm32f0xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/*
 * STM32F030C8T6:
 * Flash总容量：64KB
 * Flash范围：0x08000000 ~ 0x0800FFFF
 * 每页大小：1KB
 *
 * 本模块占用最后一页：
 * 0x0800FC00 ~ 0x0800FFFF
 *
 * 必须在链接配置中把最后1KB预留出来，不能让程序代码占用。
 */
#define FLASH_PARAM_PAGE_ADDRESS  0x0800FC00UL
#define FLASH_PARAM_PAGE_SIZE     0x00000400UL

#define FLASH_PARAM_MAGIC         0x50415241UL
#define FLASH_PARAM_VERSION       1UL

/*
 * 需要保存的参数统一放在这里。
 * 所有字段均使用uint32_t。
 *
 * 后续增加参数时：
 * 1. 在结构体末尾增加uint32_t字段；
 * 2. 修改FlashParam_SetDefault()里的默认值；
 * 3. FLASH_PARAM_VERSION加1。
 */
typedef struct
{
  uint32_t flash_high_oil_level_alarm_mm;				//高油位报警
  uint32_t flash_high_oil_level_warn_mm;				//高油位预警
  uint32_t flash_low_oil_level_alram_mm;				//低油位报警
  uint32_t flash_low_oil_level_warn_mm;					//低油位预警
  uint32_t flash_high_water_level_alarm_mm;			//高水位报警
  uint32_t flash_ywy_open_alarm_flag;						//报警是否开启标志
  uint32_t flash_high_water_shield_mm;					//水屏蔽高度
  uint32_t flash_device_id;											//设备地址
  uint32_t flash_device_length;									//探杆长度
  uint32_t flash_oil_compensation;							//油位补偿
  uint32_t flash_water_compensation;						//水位补偿
} FlashParamData_t;

typedef enum
{
  FLASH_PARAM_INIT_LOADED = 0,
  FLASH_PARAM_INIT_DEFAULT_SAVED,
  FLASH_PARAM_INIT_DEFAULT_SAVE_FAILED,
  FLASH_PARAM_INIT_ARGUMENT_ERROR
} FlashParamInitResult_t;

/*
 * 启动时调用：
 * Flash有效 -> 读取保存参数；
 * 第一次使用/数据损坏/版本改变 -> 使用默认值并写入Flash。
 */
FlashParamInitResult_t FlashParam_Init(FlashParamData_t *param);
/* 从Flash读取有效参数。 */
bool FlashParam_Load(FlashParamData_t *param);
/* 保存参数；参数没有变化时不会擦写Flash。 */
bool FlashParam_Save(const FlashParamData_t *param);
/* 判断当前Flash参数记录是否有效。 */
bool FlashParam_IsValid(void);
/* 仅给RAM参数填写默认值，不写Flash。 */
void FlashParam_SetDefault(FlashParamData_t *param);
/* 恢复默认参数并写入Flash。 */
bool FlashParam_ResetToDefault(FlashParamData_t *param);

#endif

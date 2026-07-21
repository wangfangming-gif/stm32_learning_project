#include "flash_param.h"
#include "main.h"
#include <string.h>


typedef struct
{
  uint32_t magic;
  uint32_t version;
  uint32_t data_size;
  uint32_t crc32;
  FlashParamData_t data;
} FlashParamRecord_t;

/* 编译期检查：参数记录必须能放进最后一个1KB页面。 */
typedef char FlashParamRecordSizeCheck[
  (sizeof(FlashParamRecord_t) <= FLASH_PARAM_PAGE_SIZE) ? 1 : -1
];

/* 编译期检查：F0按半字写入，记录长度必须为偶数。 */
typedef char FlashParamRecordAlignCheck[
  ((sizeof(FlashParamRecord_t) & 0x01U) == 0U) ? 1 : -1
];

static uint32_t flash_param_crc32(const uint8_t *data, uint32_t length)
{
  uint32_t crc = 0xFFFFFFFFUL;
  uint32_t i;
  uint8_t bit;

  for(i = 0U; i < length; i++)
  {
    crc ^= data[i];

    for(bit = 0U; bit < 8U; bit++)
    {
      if((crc & 1UL) != 0UL)
        crc = (crc >> 1) ^ 0xEDB88320UL;
      else
        crc >>= 1;
    }
  }

  return ~crc;
}

static void flash_param_read_record(FlashParamRecord_t *record)
{
  memcpy(record,(const void *)FLASH_PARAM_PAGE_ADDRESS,sizeof(FlashParamRecord_t));
}

static bool flash_param_record_valid(const FlashParamRecord_t *record)
{
  uint32_t crc;

  if(record->magic != FLASH_PARAM_MAGIC)		//魔数标识错误
    return false;

  if(record->version != FLASH_PARAM_VERSION)	//版本错误
    return false;

  if(record->data_size != sizeof(FlashParamData_t))	//数据长度错误
    return false;

  crc = flash_param_crc32((const uint8_t *)&record->data,	
                          sizeof(FlashParamData_t));	

  if(crc != record->crc32)		//crc校验错误
    return false;

  return true;
}

static bool flash_param_program_halfword(uint32_t address, uint16_t data)
{
	//写入16位的数据到flash当中
  if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                       address,
                       (uint64_t)data) != HAL_OK)
  {
    return false;
  }
											 
	//判断写入值是否正确
  return (*(volatile const uint16_t *)address == data);
}

/* 仅给RAM参数填写默认值，不写Flash。 */
void FlashParam_SetDefault(FlashParamData_t *param)
{
	//设置默认值
  if(param == NULL)
    return;

  memset(param, 0, sizeof(FlashParamData_t));
	
  param->flash_high_oil_level_alarm_mm = HIGH_OIL_LEVEL_ALARM_MM;				//高油位报警
  param->flash_high_oil_level_warn_mm = HIGH_OIL_LEVEL_WARN_MM;				//高油位预警
  param->flash_low_oil_level_alram_mm = LOW_OIL_LEVEL_ALRAM_MM;				//低油位报警
  param->flash_low_oil_level_warn_mm = LOW_OIL_LEVEL_WARN_MM;					//低油位预警
  param->flash_high_water_level_alarm_mm = HIGH_WATER_LEVEL_ALARM_MM;			//高水位报警
  param->flash_ywy_open_alarm_flag = YWY_OPEN_ALARM_FLAG;						//报警是否开启标志
  param->flash_high_water_shield_mm = HIGH_WATER_SHIELD_MM;					//水屏蔽高度
  param->flash_device_id = DEVICE_ID;											//设备地址
  param->flash_device_length = DEVICE_LENGTH;									//探杆长度
  param->flash_oil_compensation = OIL_COMPENSATION;							//油位补偿
  param->flash_water_compensation = WATER_COMPENSATION;						//水位补偿

}

/* 判断当前Flash参数记录是否有效。 */
bool FlashParam_IsValid(void)
{
  FlashParamRecord_t record;

  flash_param_read_record(&record);		//将数据从flash中读取出来

  return flash_param_record_valid(&record);	//判断数据是否损坏或者有错误之类的
}

/* 从Flash读取有效参数。 */
bool FlashParam_Load(FlashParamData_t *param)
{
  FlashParamRecord_t record;

  if(param == NULL)
    return false;

  flash_param_read_record(&record);

  if(!flash_param_record_valid(&record))	
    return false;		//如果有错误就返回false

	//将读出来的数据复制到指针指向的地址中
  memcpy(param, &record.data, sizeof(FlashParamData_t));

  return true;
}

/* 保存参数；参数没有变化时不会擦写Flash。 */
bool FlashParam_Save(const FlashParamData_t *param)
{
  FLASH_EraseInitTypeDef erase;
  FlashParamRecord_t record;
  FlashParamData_t old_param;

  uint32_t page_error = 0U;
  uint32_t offset;
  uint16_t halfword;
  bool result = false;

  if(param == NULL)
    return false;

  /*
   * 参数与Flash中一致时直接返回，避免无意义擦写。
   */
  if(FlashParam_Load(&old_param))
  {
    if(memcmp(&old_param, param, sizeof(FlashParamData_t)) == 0)
      return true;
  }

  memset(&record, 0xFF, sizeof(record));

  record.magic = FLASH_PARAM_MAGIC;
  record.version = FLASH_PARAM_VERSION;
  record.data_size = sizeof(FlashParamData_t);
  memcpy(&record.data, param, sizeof(FlashParamData_t));
  record.crc32 = flash_param_crc32((const uint8_t *)&record.data,
                                   sizeof(FlashParamData_t));

  if(HAL_FLASH_Unlock() != HAL_OK)	
    return false;

  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP |
                         FLASH_FLAG_PGERR |
                         FLASH_FLAG_WRPERR);

  erase.TypeErase = FLASH_TYPEERASE_PAGES;
  erase.PageAddress = FLASH_PARAM_PAGE_ADDRESS;
  erase.NbPages = 1U;

  if(HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK)
    goto exit;

  /*
   * STM32F030按16位半字写入。
   *
   * 先跳过前4字节magic，写入其余数据；
   * 全部写完后，最后再写magic。
   *
   * 如果写入中途掉电，magic仍不是完整值，
   * 下次启动会认为记录无效并使用默认参数。
   */
  for(offset = sizeof(record.magic);
      offset < sizeof(FlashParamRecord_t);
      offset += 2U)
  {
    memcpy(&halfword,
           ((const uint8_t *)&record) + offset,
           sizeof(halfword));

    if(!flash_param_program_halfword(
         FLASH_PARAM_PAGE_ADDRESS + offset,
         halfword))
    {
      goto exit;
    }
  }

  memcpy(&halfword,
         ((const uint8_t *)&record.magic) + 0U,
         sizeof(halfword));

  if(!flash_param_program_halfword(
       FLASH_PARAM_PAGE_ADDRESS,
       halfword))
  {
    goto exit;
  }

  memcpy(&halfword,
         ((const uint8_t *)&record.magic) + 2U,
         sizeof(halfword));

  if(!flash_param_program_halfword(
       FLASH_PARAM_PAGE_ADDRESS + 2U,
       halfword))
  {
    goto exit;
  }

  result = true;

exit:
  HAL_FLASH_Lock();

  if(!result)
    return false;

  __DSB();
  __ISB();

  if(!FlashParam_Load(&old_param))
    return false;

  return memcmp(&old_param,
                param,
                sizeof(FlashParamData_t)) == 0;
}

/*
 * 初始化函数
 * 启动时调用：
 * Flash有效 -> 读取保存参数；
 * 第一次使用/数据损坏/版本改变 -> 使用默认值并写入Flash。
 */
FlashParamInitResult_t FlashParam_Init(FlashParamData_t *param)
{
  if(param == NULL)
    return FLASH_PARAM_INIT_ARGUMENT_ERROR;

  if(FlashParam_Load(param))
    return FLASH_PARAM_INIT_LOADED;

  FlashParam_SetDefault(param);

  if(FlashParam_Save(param))
    return FLASH_PARAM_INIT_DEFAULT_SAVED;

  return FLASH_PARAM_INIT_DEFAULT_SAVE_FAILED;
}

/* 恢复默认参数并写入Flash。 */
bool FlashParam_ResetToDefault(FlashParamData_t *param)
{
  if(param == NULL)
    return false;

  FlashParam_SetDefault(param);

  return FlashParam_Save(param);
}

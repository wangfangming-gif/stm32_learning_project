#include "main.h"
#include "../BSP/HT16220/ht16220.h"

#define LCD_CS_GPIO_RCC		 __HAL_RCC_GPIOB_CLK_ENABLE()
#define LCD_CS_GPIO_Port   GPIOB
#define LCD_CS_Pin         GPIO_PIN_10

#define LCD_WR_GPIO_RCC		 __HAL_RCC_GPIOB_CLK_ENABLE()
#define LCD_WR_GPIO_Port   GPIOB
#define LCD_WR_Pin         GPIO_PIN_11

#define LCD_DATA_GPIO_RCC		 __HAL_RCC_GPIOB_CLK_ENABLE()
#define LCD_DATA_GPIO_Port GPIOB
#define LCD_DATA_Pin       GPIO_PIN_12

void ht16220_gpio_init(void);

void HT16220_PortCS(uint8_t level);
void HT16220_PortWR(uint8_t level);
void HT16220_PortDATA(uint8_t level);
void HT16220_PortDelay(void);


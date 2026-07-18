#include "../BSP/HT16220/ht16220.h"
#include "../BSP/LCD/lcd_gpio.h"

GPIO_InitTypeDef ht16220_gpio_initstruct;

void ht16220_gpio_init(void)
{
	LCD_CS_GPIO_RCC;
	LCD_WR_GPIO_RCC;
	LCD_DATA_GPIO_RCC;
	
	ht16220_gpio_initstruct.Mode = GPIO_MODE_OUTPUT_PP;
	ht16220_gpio_initstruct.Pull = GPIO_PULLDOWN;
	ht16220_gpio_initstruct.Speed = GPIO_SPEED_FREQ_HIGH;
	
	ht16220_gpio_initstruct.Pin = LCD_CS_Pin;
	HAL_GPIO_Init(LCD_CS_GPIO_Port,&ht16220_gpio_initstruct);

	ht16220_gpio_initstruct.Pin = LCD_WR_Pin;
	HAL_GPIO_Init(LCD_WR_GPIO_Port,&ht16220_gpio_initstruct);

	ht16220_gpio_initstruct.Pin = LCD_DATA_Pin;
	HAL_GPIO_Init(LCD_DATA_GPIO_Port,&ht16220_gpio_initstruct);
}

void HT16220_PortCS(uint8_t level)
{
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port,
                    LCD_CS_Pin,
                    level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void HT16220_PortWR(uint8_t level)
{
  HAL_GPIO_WritePin(LCD_WR_GPIO_Port,
                    LCD_WR_Pin,
                    level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void HT16220_PortDATA(uint8_t level)
{
  HAL_GPIO_WritePin(LCD_DATA_GPIO_Port,
                    LCD_DATA_Pin,
                    level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}


void HT16220_PortDelay(void)
{
  volatile uint32_t i;
  for(i = 0; i < 4U; i++)
    __NOP();
}


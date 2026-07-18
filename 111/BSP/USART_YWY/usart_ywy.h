#ifndef __USART_YWY_H
#define __USART_YWY_H

#include "stm32f0xx_hal.h"


void usart_ywy_dma_init(uint32_t Temp_BaudRate);
uint8_t uart_send_data(uint8_t *data, uint16_t len);
void uart_send_check(void);

#endif

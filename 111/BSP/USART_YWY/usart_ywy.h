#ifndef __USART_YWY_H
#define __USART_YWY_H

#include "stm32f0xx_hal.h"

void usart_ywy_dma_init(uint32_t Temp_BaudRate);
void uart_ywy_send_check(void);

void com_ywy_handle(void);
void modbus_time_done(void);

#endif

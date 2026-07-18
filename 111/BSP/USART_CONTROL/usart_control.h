#ifndef __USART_CONTROL_H
#define __USART_CONTROL_H

#include "stm32f0xx_hal.h"

void usart_ywy_test_dma_init(uint32_t Temp_BaudRate);

void uart_ywy_send_fifo_init(void);
uint8_t uart_ywy_send_data(uint8_t *data, uint16_t len);
uint8_t uart_ywy_send_string(char *str);
void uart_ywy_send_check(void);

#endif

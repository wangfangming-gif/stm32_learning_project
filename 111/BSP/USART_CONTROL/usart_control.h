#ifndef __USART_CONTROL_H
#define __USART_CONTROL_H

#include "stm32f0xx_hal.h"

#define tx_rx_2_buffercount 64
void usart2_control_dma_init(uint32_t Temp_BaudRate);

uint8_t uart2_send_data(uint8_t *data, uint16_t len);
void uart2_ywy_send_check(void);
void my_usart2_receive_handle(void);


#endif

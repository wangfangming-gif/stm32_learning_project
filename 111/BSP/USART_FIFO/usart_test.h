#ifndef __USART_TEST_H
#define __USART_TEST_H

#include "stm32f0xx_hal.h"

#define UART_DMA_SEND_BUFFER_SIZE    256

typedef struct 
{
    uint8_t *buffer;
    uint16_t size;
    uint16_t head;
    uint16_t tail;
}uart_receive_struct;

typedef struct
{
	uint8_t *buffer;						//数据缓冲区
	uint16_t size;
	uint16_t head;
	uint16_t tail;	

	uint16_t *buffercount;				//单次的发送数据数量
	uint16_t buffercount_size;
	uint16_t buffercount_head;
	uint16_t buffercount_tail;	
}uart_send_struct;


//-------------------------------------发送FIFO-------------------------------------------------
//发送环形缓冲区的初始化
void send_loop_buffer_init(uart_send_struct *temp_value,
													 uint8_t *temp_buffer,
													 uint16_t temp_buffer_size,
													 uint16_t *temp_buffer_count,
													 uint16_t temp_buffercount_size);
//发送环形缓冲区写入
uint8_t send_loop_buffer_push(uart_send_struct *temp_value, uint8_t *buffer, uint16_t count);
//发送唤醒缓冲区读取
uint16_t send_loop_buffer_pop(uart_send_struct *temp_value, uint8_t *buffer);


//-------------------------------------接收FIFO-------------------------------------------------
//首先是环形缓冲区的初始化
void loop_buffer_init(uart_receive_struct *temp_value,uint8_t *temp_buffer,uint16_t temp_size);
//环形缓冲区批量写入
uint16_t loop_buffer_push_array(uart_receive_struct *rb, const uint8_t *data, uint16_t len);
//环形缓冲区批量读取
uint16_t loop_buffer_pop_array(uart_receive_struct *rb, uint8_t *data, uint16_t len);

#endif

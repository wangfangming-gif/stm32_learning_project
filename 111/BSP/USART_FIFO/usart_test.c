#include "../BSP/USART_FIFO/usart_test.h"
#include <string.h>

static uint16_t send_loop_buffer_data_free(uart_send_struct *temp_value);
static uint16_t send_loop_buffer_count_free(uart_send_struct *temp_value);
static uint8_t send_loop_buffer_count_empty(uart_send_struct *temp_value);
static void uart_send_enter_critical(void);
static void uart_send_exit_critical(void);

static uint16_t loop_buffer_next_index(uint16_t temp_index,uint16_t temp_size);
static uint16_t loop_buffer_available(const uart_receive_struct *temp_value);
static uint16_t loop_buffer_free(const uart_receive_struct *temp_value);
static uint8_t loop_buffer_push(uart_receive_struct *temp_value,const uint8_t insert_data);
static uint8_t loop_buffer_pop(uart_receive_struct *temp_value, uint8_t *data);

//-------------------------------------发送FIFO-------------------------------------------------

static void uart_send_enter_critical(void)
{
	__disable_irq();
}

static void uart_send_exit_critical(void)
{
	__enable_irq();
}

static uint16_t send_loop_buffer_data_free(uart_send_struct *temp_value)
{
	if(temp_value->head >= temp_value->tail)
	{
		return temp_value->size - (temp_value->head - temp_value->tail) - 1;
	}
	else
	{
		return temp_value->tail - temp_value->head - 1;
	}
}

static uint16_t send_loop_buffer_count_free(uart_send_struct *temp_value)
{
	if(temp_value->buffercount_head >= temp_value->buffercount_tail)
	{
		return temp_value->buffercount_size - (temp_value->buffercount_head - temp_value->buffercount_tail) - 1;
	}
	else
	{
		return temp_value->buffercount_tail - temp_value->buffercount_head - 1;
	}
}

static uint8_t send_loop_buffer_count_empty(uart_send_struct *temp_value)
{
	if(temp_value->buffercount_head == temp_value->buffercount_tail)
	{
		return 1;
	}

	return 0;
}

void send_loop_buffer_init(uart_send_struct *temp_value,
													 uint8_t *temp_buffer,
													 uint16_t temp_buffer_size,
													 uint16_t *temp_buffer_count,
													 uint16_t temp_buffercount_size)
{
	temp_value->buffer = temp_buffer;
	temp_value->size = temp_buffer_size;
	temp_value->head = 0;
	temp_value->tail = 0;

	temp_value->buffercount = temp_buffer_count;
	temp_value->buffercount_size = temp_buffercount_size;
	temp_value->buffercount_head = 0;
	temp_value->buffercount_tail = 0;
}

uint8_t send_loop_buffer_push(uart_send_struct *temp_value, uint8_t *buffer, uint16_t count)
{
	uint16_t i;

	if((temp_value == 0) || (buffer == 0) || (count == 0))
	{
		return 0;
	}

	if(count > UART_DMA_SEND_BUFFER_SIZE)
	{
		return 0;
	}

	uart_send_enter_critical();

	if((send_loop_buffer_data_free(temp_value) < count) || (send_loop_buffer_count_free(temp_value) == 0))
	{
		uart_send_exit_critical();
		return 0;
	}

	for(i = 0; i < count; i++)
	{
		temp_value->buffer[temp_value->head] = buffer[i];
		temp_value->head++;
		if(temp_value->head >= temp_value->size)
		{
			temp_value->head = 0;
		}
	}

	temp_value->buffercount[temp_value->buffercount_head] = count;
	temp_value->buffercount_head++;
	if(temp_value->buffercount_head >= temp_value->buffercount_size)
	{
		temp_value->buffercount_head = 0;
	}

	uart_send_exit_critical();

	return 1;
}

uint16_t send_loop_buffer_pop(uart_send_struct *temp_value, uint8_t *buffer)
{
	uint16_t i;
	uint16_t count;

	if((temp_value == 0) || (buffer == 0))
	{
		return 0;
	}

	uart_send_enter_critical();

	if(send_loop_buffer_count_empty(temp_value))
	{
		uart_send_exit_critical();
		return 0;
	}

	count = temp_value->buffercount[temp_value->buffercount_tail];
	temp_value->buffercount_tail++;
	if(temp_value->buffercount_tail >= temp_value->buffercount_size)
	{
		temp_value->buffercount_tail = 0;
	}

	for(i = 0; i < count; i++)
	{
		buffer[i] = temp_value->buffer[temp_value->tail];
		temp_value->tail++;
		if(temp_value->tail >= temp_value->size)
		{
			temp_value->tail = 0;
		}
	}

	uart_send_exit_critical();

	return count;
}


//-------------------------------------接收FIFO-------------------------------------------------

//这里得到下一个index
static uint16_t loop_buffer_next_index(uint16_t temp_index,uint16_t temp_size)
{
	temp_index++;
	if(temp_index >= temp_size)
	{
		return 0;
	}
	return temp_index;
}
//环形缓冲区
static uint16_t loop_buffer_available(const uart_receive_struct *temp_value)
{
	if(temp_value->head >= temp_value->tail)
	{
		return (temp_value->head - temp_value->tail);
	}
	else
	{
		return (temp_value->size - temp_value->tail + temp_value->head);
	}
}
//环形缓冲区已经有的字节
static uint16_t loop_buffer_free(const uart_receive_struct *temp_value)
{
	return temp_value->size - loop_buffer_available(temp_value) - 1;
}
//环形缓冲区写入
static uint8_t loop_buffer_push(uart_receive_struct *temp_value,const uint8_t insert_data)
{
	//得到下一个index
	uint16_t push_temp_index = loop_buffer_next_index(temp_value->head,temp_value->size);

	//如果next之后，头和尾相等，那么就不能在环形缓冲区添加
	if(push_temp_index == temp_value->tail)
	{
		return 0;
	}
	else
	{
		temp_value->buffer[temp_value->head] = insert_data;
		temp_value->head = push_temp_index;
		return 1;
	}
}

//环形缓冲区读取
static uint8_t loop_buffer_pop(uart_receive_struct *temp_value, uint8_t *data)
{
	uint16_t push_temp_index = loop_buffer_next_index(temp_value->tail,temp_value->size);

	if(temp_value->tail == temp_value->head)
	{
		return 0;
	}
	else
	{
		*data = temp_value->buffer[temp_value->tail];
		temp_value->tail = push_temp_index;
		return 1;
	}
}

//首先是环形缓冲区的初始化
void loop_buffer_init(uart_receive_struct *temp_value,uint8_t *temp_buffer,uint16_t temp_size)
{
	temp_value->buffer = temp_buffer;
	temp_value->size = temp_size;

	temp_value->head = 0;	//头部是0
	temp_value->tail = 0;	//然后尾部也是0
}

//环形缓冲区批量写入
uint16_t loop_buffer_push_array(uart_receive_struct *rb, const uint8_t *data, uint16_t len)
{
	uint16_t temp_len = 0;
	for(temp_len = 0;temp_len < len;temp_len++)
	{
		if(loop_buffer_push(rb,data[temp_len]) == 0)
		{
			break;
		}
	}
	return temp_len;
}


//环形缓冲区批量读取
uint16_t loop_buffer_pop_array(uart_receive_struct *rb, uint8_t *data, uint16_t len)
{
	uint16_t temp_len = 0;

	for(temp_len = 0;temp_len < len;temp_len++)
	{
		if(loop_buffer_pop(rb,&data[temp_len]) == 0)
		{
			break;
		}
	}
	return temp_len;
}


#include "../BSP/USART_YWY/usart_ywy.h"
#include "../BSP/USART_FIFO/usart_test.h"
#include "../BSP/CRC/crc.h"

#include "../BSP/RTT/SEGGER_RTT.h"

#include "../Core/Inc/stm32f0xx_it.h"

#include <stdio.h>
#include <string.h>


UART_HandleTypeDef usart_handle;
DMA_HandleTypeDef usart_rx_handle;
DMA_HandleTypeDef usart_tx_handle;


//---------------------------USART处理-------------------------------

uart_receive_struct modbus_recieve_struct;
uart_send_struct uart_send_fifo;

#define UART_RECEIVE_FIFO_SIZE 512
uint8_t uart_receive_fifo_buffer[UART_RECEIVE_FIFO_SIZE];

#define tx_rx_buffercount 128
uint8_t usart_rx_buffer[tx_rx_buffercount];

volatile uint8_t usart_receive_done = 0U;
volatile uint16_t usart_receive_length = 0U;
volatile uint32_t usart_receive_lost_count = 0U;

volatile uint8_t usart_send_done = 0;	//发送结束标志位,1代表发送完成，0代表发送未完成


#define UART_SEND_BUFFER_SIZE        512
#define UART_SEND_COUNT_SIZE         64
#define UART_DMA_SEND_BUFFER_SIZE    256

static uint8_t uart_send_buffer[UART_SEND_BUFFER_SIZE];
static uint16_t uart_send_count_buffer[UART_SEND_COUNT_SIZE];
static uint8_t uart_dma_send_buffer[UART_DMA_SEND_BUFFER_SIZE];
//---------------------------USART处理-------------------------------


//---------------------------液位仪处理-------------------------------
uint16_t com_ywy_send_done_flag = 0;
uint8_t com_ywy_delay_done_flag = 0;
float ywy_oil_height = 0;
float ywy_water_height = 0;
float ywy_temp = 0;
uint32_t ywy_temp_data = 0;

uint16_t com_ywy_id = 225;				//液位仪开始询问地址

//液位仪探杆询问指令
uint8_t com_ywy_send_buffer[8] = {0x00,0x03,0x00,0x02,0x00,0x0B,0x00,0x00};

#define UART_ANALYZE_BUFFER_SIZE 128
static uint8_t uart_analyze_buffer[UART_ANALYZE_BUFFER_SIZE];
//---------------------------液位仪处理-------------------------------



void usart_ywy_dma_init(uint32_t Temp_BaudRate)
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_USART1_CLK_ENABLE();
	__HAL_RCC_DMA1_CLK_ENABLE();

	GPIO_InitTypeDef gpio_init;
	gpio_init.Alternate = GPIO_AF1_USART1;
	gpio_init.Mode = GPIO_MODE_AF_PP;
	gpio_init.Pin = GPIO_PIN_9;
	gpio_init.Pull = GPIO_PULLUP;
	gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA,&gpio_init);

	gpio_init.Pin = GPIO_PIN_10;
	HAL_GPIO_Init(GPIOA,&gpio_init);	//10和9

	usart_handle.Instance = USART1;
	usart_handle.Init.BaudRate = Temp_BaudRate;
	usart_handle.Init.Mode = UART_MODE_TX_RX;
	usart_handle.Init.Parity = UART_PARITY_NONE;
	usart_handle.Init.StopBits = UART_STOPBITS_1;
	usart_handle.Init.WordLength = UART_WORDLENGTH_8B;
	usart_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;	
	HAL_UART_Init(&usart_handle);

		//接下来设置DMA
	usart_tx_handle.Instance = DMA1_Channel2;
	usart_tx_handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
	usart_tx_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	usart_tx_handle.Init.MemInc = DMA_MINC_ENABLE;
	usart_tx_handle.Init.Mode = DMA_NORMAL;
	usart_tx_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	usart_tx_handle.Init.PeriphInc = DMA_PINC_DISABLE;
	usart_tx_handle.Init.Priority = DMA_PRIORITY_MEDIUM;		
	HAL_DMA_Init(&usart_tx_handle);

	usart_rx_handle.Instance = DMA1_Channel3;
	usart_rx_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
	usart_rx_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	usart_rx_handle.Init.MemInc = DMA_MINC_ENABLE;
	usart_rx_handle.Init.Mode = DMA_NORMAL;
	usart_rx_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	usart_rx_handle.Init.PeriphInc = DMA_PINC_DISABLE;
	usart_rx_handle.Init.Priority = DMA_PRIORITY_MEDIUM;	
	HAL_DMA_Init(&usart_rx_handle);

	__HAL_LINKDMA(&usart_handle,hdmatx,usart_tx_handle);
	__HAL_LINKDMA(&usart_handle,hdmarx,usart_rx_handle);

	//随便设置一个中断优先级
	HAL_NVIC_SetPriority(USART1_IRQn,0,1);
	HAL_NVIC_EnableIRQ(USART1_IRQn);

	//接收的NVIC
	HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn,1,0);
	HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

	loop_buffer_init(&modbus_recieve_struct,
                 uart_receive_fifo_buffer,
                 UART_RECEIVE_FIFO_SIZE);

 	usart_send_done = 0;
	send_loop_buffer_init(&uart_send_fifo,
												uart_send_buffer,
												UART_SEND_BUFFER_SIZE,
												uart_send_count_buffer,
												UART_SEND_COUNT_SIZE);

	HAL_UARTEx_ReceiveToIdle_DMA(&usart_handle,
               		usart_rx_buffer,
               		tx_rx_buffercount);

	/* 不需要DMA半传输事件 */
	__HAL_DMA_DISABLE_IT(usart_handle.hdmarx, DMA_IT_HT);

}

uint8_t uart_send_data(uint8_t *data, uint16_t len)
{
	return send_loop_buffer_push(&uart_send_fifo, data, len);
}

//发送循环检查函数，这个要放在main循环中一直不断的进行检查
void uart_ywy_send_check(void)
{
	uint16_t len;

	if(usart_send_done)
	{
		return;
	}

	len = send_loop_buffer_pop(&uart_send_fifo, uart_dma_send_buffer);
	if(len == 0)
	{
		return;
	}

	usart_send_done = 1;

	if(HAL_UART_Transmit_DMA(&usart_handle, uart_dma_send_buffer, len) != HAL_OK)
	{
		usart_send_done = 0;
	}
}

void com_ywy_handle(void)
{
	uint16_t com_ywy_crc;
	uint16_t temp_len;
	static uint8_t usart_printf_buffer[32];
	uint8_t usart_printf_count = 0;
	
	//如果还没有发送过ywy的询问指令的话，那么就发送
	if(com_ywy_send_done_flag == 0)	
	{
		com_ywy_send_buffer[0] = com_ywy_id;
		com_ywy_crc = CRC16(com_ywy_send_buffer,6);
		com_ywy_send_buffer[6] = com_ywy_crc & 0x00FF;
		com_ywy_send_buffer[7] = com_ywy_crc >> 8;
		//发送询问指令
		uart_send_data(com_ywy_send_buffer,8);

		com_ywy_send_done_flag = 1;	//发送标志位置1
	}
	//如果发送过询问指令了，那么就等待时间到，时间到了之后，我们读取一下
	//看看有没有数据，如果有数据，那么我们就解析，如果没有数据我们就接着
	//向下询问，解析之前的过程中，数据保持为0
	else 
	{
		if(com_ywy_delay_done_flag == 1)
		{
			if(usart_receive_done == 0)
			{
				SEGGER_RTT_SetTerminal(0); 
				SEGGER_RTT_WriteString(0, "ywy c_error!\r\n");

				com_ywy_id++;
				if(com_ywy_id > 236)
				{
					com_ywy_id = 225;
				}
			}
			else
			{
				temp_len = loop_buffer_pop_array(&modbus_recieve_struct,
																		uart_analyze_buffer,
																		UART_ANALYZE_BUFFER_SIZE);
				//验证CRC校验
				com_ywy_crc  = CRC16(uart_analyze_buffer,temp_len-2);

				if(((com_ywy_crc >> 8) ==  uart_analyze_buffer[temp_len-1]) && 
					((com_ywy_crc & 0x00FF) == uart_analyze_buffer[temp_len - 2]) &&
					(temp_len == 27))
				{
					ywy_temp_data = 0;
					ywy_temp_data = uart_analyze_buffer[3];
					ywy_temp_data = (ywy_temp_data<<8) + uart_analyze_buffer[4];
					ywy_temp_data = (ywy_temp_data<<8) + uart_analyze_buffer[5];
					ywy_temp_data = (ywy_temp_data<<8) + uart_analyze_buffer[6];
					ywy_oil_height = (float)ywy_temp_data / 65536.0;

					ywy_temp_data = 0;
					ywy_temp_data = uart_analyze_buffer[7];
					ywy_temp_data = (ywy_temp_data<<8) + uart_analyze_buffer[8];
					ywy_temp_data = (ywy_temp_data<<8) + uart_analyze_buffer[9];
					ywy_temp_data = (ywy_temp_data<<8) + uart_analyze_buffer[10];
					ywy_water_height = (float)ywy_temp_data / 65536.0;

					ywy_temp_data = 0;
				  ywy_temp_data = uart_analyze_buffer[13];
					ywy_temp_data = (ywy_temp_data << 8) +  uart_analyze_buffer[14];
					ywy_temp = (float)ywy_temp_data / 16;

					usart_printf_count = sprintf(usart_printf_buffer,"ywy:%.2f, %.2f, %.2f\r\n",ywy_oil_height,ywy_water_height,ywy_temp);
					SEGGER_RTT_SetTerminal(0); 
					SEGGER_RTT_Write(0, usart_printf_buffer,usart_printf_count);
				}
				else
				{
					SEGGER_RTT_SetTerminal(0); 
					SEGGER_RTT_WriteString(0, "ywy error!\r\n");

					com_ywy_id++;
					if(com_ywy_id > 236)
					{
						com_ywy_id = 225;
					}
				}
			}
			com_ywy_delay_done_flag = 0;
			com_ywy_send_done_flag = 0; 
			usart_receive_done = 0;
		}
		else
		{

		}
	}
}

void modbus_time_done(void)
{
	static uint16_t com_ywy_conn_time_out = 0;
	if((com_ywy_send_done_flag == 1) && 
		(com_ywy_delay_done_flag == 0))
	{
		com_ywy_conn_time_out++;
		if(com_ywy_conn_time_out > 70)
		{
			com_ywy_delay_done_flag = 1;
			com_ywy_conn_time_out = 0;
		}
	}
  else
  {

	}
}

//发送完毕的回调函数
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == USART1)		//如果是USART1的话
	{
		usart_send_done = 0;
	}
}


/*
	接收完毕的回调函数
*/
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart,
                                uint16_t Size)
{

	if(huart->Instance == USART1)		//串口1
	{
			loop_buffer_push_array(&modbus_recieve_struct,
																				usart_rx_buffer,
																				Size);
			usart_receive_done = 1U;		
		/*
		 * DMA是NORMAL模式。
		 * 每次空闲事件后需要重新启动下一次接收。
		 */
		if(HAL_UARTEx_ReceiveToIdle_DMA(&usart_handle,
																		usart_rx_buffer,
																		tx_rx_buffercount) == HAL_OK)
		{
			__HAL_DMA_DISABLE_IT(usart_handle.hdmarx,DMA_IT_HT);
		}

	}
}


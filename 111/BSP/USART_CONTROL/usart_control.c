#include "../BSP/USART_CONTROL/usart_control.h"
#include "../BSP/USART_FIFO/usart_test.h"
#include "../BSP/CRC/crc.h"

#include "../BSP/RTT/SEGGER_RTT.h"

#include "../Core/Inc/stm32f0xx_it.h"

#include <stdio.h>
#include <string.h>


UART_HandleTypeDef usart2_handle;
DMA_HandleTypeDef usart2_rx_handle;
DMA_HandleTypeDef usart2_tx_handle;


//---------------------------USART处理-------------------------------

uart_receive_struct modbus2_recieve_struct;
uart_send_struct uart2_send_fifo;

#define UART_RECEIVE_FIFO_SIZE 64
uint8_t uart2_receive_fifo_buffer[UART_RECEIVE_FIFO_SIZE];


uint8_t usart2_rx_buffer[tx_rx_2_buffercount];

volatile uint8_t usart2_receive_done = 0U;

volatile uint8_t usart2_send_done = 0;	//是否可发送标志位，0代表可发送，1代表不可以发送


#define UART_SEND_BUFFER_SIZE        64
#define UART_SEND_COUNT_SIZE         16
//#define UART_DMA_SEND_BUFFER_SIZE    64

static uint8_t uart_send_buffer[UART_SEND_BUFFER_SIZE];
static uint16_t uart_send_count_buffer[UART_SEND_COUNT_SIZE];
static uint8_t uart_dma_send_buffer[UART_DMA_SEND_BUFFER_SIZE];
//---------------------------USART处理-------------------------------

#define UART2_ANALYZE_BUFFER_SIZE 48
static uint8_t uart2_analyze_buffer[UART2_ANALYZE_BUFFER_SIZE];

extern uint32_t device_id; 										//设备地址
extern float ywy_oil_height; 		//液位仪油高
extern float ywy_water_height; 	//液位仪水高
extern float ywy_temp1;	
extern float ywy_temp2;
extern float ywy_temp3;	
extern float ywy_temp4;
extern float ywy_temp5;	

void usart2_control_dma_init(uint32_t Temp_BaudRate)
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_USART2_CLK_ENABLE();
	__HAL_RCC_DMA1_CLK_ENABLE();

	GPIO_InitTypeDef gpio_init;
	GPIO_InitTypeDef gpio_en_init;
	gpio_init.Alternate = GPIO_AF1_USART2;
	gpio_init.Mode = GPIO_MODE_AF_PP;
	gpio_init.Pin = GPIO_PIN_2;
	gpio_init.Pull = GPIO_PULLUP;
	gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA,&gpio_init);

	gpio_init.Pin = GPIO_PIN_3;
	HAL_GPIO_Init(GPIOA,&gpio_init);	//2和3

	//使能引脚
	gpio_en_init.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_en_init.Pin = GPIO_PIN_4;
	gpio_en_init.Pull = GPIO_PULLUP;
	gpio_en_init.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA,&gpio_en_init);

	usart2_handle.Instance = USART2;
	usart2_handle.Init.BaudRate = Temp_BaudRate;
	usart2_handle.Init.Mode = UART_MODE_TX_RX;
	usart2_handle.Init.Parity = UART_PARITY_NONE;
	usart2_handle.Init.StopBits = UART_STOPBITS_1;
	usart2_handle.Init.WordLength = UART_WORDLENGTH_8B;
	usart2_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;	
	HAL_UART_Init(&usart2_handle);

		//接下来设置DMA
	usart2_tx_handle.Instance = DMA1_Channel4;
	usart2_tx_handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
	usart2_tx_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	usart2_tx_handle.Init.MemInc = DMA_MINC_ENABLE;
	usart2_tx_handle.Init.Mode = DMA_NORMAL;
	usart2_tx_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	usart2_tx_handle.Init.PeriphInc = DMA_PINC_DISABLE;
	usart2_tx_handle.Init.Priority = DMA_PRIORITY_MEDIUM;		
	HAL_DMA_Init(&usart2_tx_handle);

	usart2_rx_handle.Instance = DMA1_Channel5;
	usart2_rx_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
	usart2_rx_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	usart2_rx_handle.Init.MemInc = DMA_MINC_ENABLE;
	usart2_rx_handle.Init.Mode = DMA_NORMAL;
	usart2_rx_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	usart2_rx_handle.Init.PeriphInc = DMA_PINC_DISABLE;
	usart2_rx_handle.Init.Priority = DMA_PRIORITY_MEDIUM;	
	HAL_DMA_Init(&usart2_rx_handle);

	__HAL_LINKDMA(&usart2_handle,hdmatx,usart2_tx_handle);
	__HAL_LINKDMA(&usart2_handle,hdmarx,usart2_rx_handle);

	//随便设置一个中断优先级
	HAL_NVIC_SetPriority(USART2_IRQn,0,1);
	HAL_NVIC_EnableIRQ(USART2_IRQn);

	//接收的NVIC
	HAL_NVIC_SetPriority(DMA1_Channel4_5_IRQn,1,0);
	HAL_NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);

	loop_buffer_init(&modbus2_recieve_struct,
									uart2_receive_fifo_buffer,
                 UART_RECEIVE_FIFO_SIZE);

 	usart2_send_done = 0;
	send_loop_buffer_init(&uart2_send_fifo,
												uart_send_buffer,
												UART_SEND_BUFFER_SIZE,
												uart_send_count_buffer,
												UART_SEND_COUNT_SIZE);

	HAL_UARTEx_ReceiveToIdle_DMA(&usart2_handle,
               		usart2_rx_buffer,
               		tx_rx_2_buffercount);

	/* 不需要DMA半传输事件 */
	__HAL_DMA_DISABLE_IT(usart2_handle.hdmarx, DMA_IT_HT);

}

uint8_t uart2_send_data(uint8_t *data, uint16_t len)
{
	return send_loop_buffer_push(&uart2_send_fifo, data, len);
}

//发送循环检查函数，这个要放在main循环中一直不断的进行检查
void uart2_ywy_send_check(void)
{
	uint16_t len;

	if(usart2_send_done)
	{
		return;
	}

	len = send_loop_buffer_pop(&uart2_send_fifo, uart_dma_send_buffer);
	if(len == 0)
	{
		return;
	}

	usart2_send_done = 1;

	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);
	if(HAL_UART_Transmit_DMA(&usart2_handle, uart_dma_send_buffer, len) != HAL_OK)
	{
		usart2_send_done = 0;
	}
}

void my_usart2_receive_handle(void)
{
	uint16_t temp_crc_data;
	uint32_t temp_data;
	uint16_t temp_len = 0;
	uint16_t temp_receive_ask_length = 0;
	static uint8_t temp_send_back_data[48];
	//如果有收到数据的话
	if(usart2_receive_done == 1)
	{
		temp_len = loop_buffer_pop_array(&modbus2_recieve_struct,
																uart2_analyze_buffer,
																UART2_ANALYZE_BUFFER_SIZE);
		temp_receive_ask_length = uart2_analyze_buffer[4]*256 + uart2_analyze_buffer[5];
		if(temp_len == 8 && ((temp_receive_ask_length == 9) || (temp_receive_ask_length == 11)))
		{
			if((uart2_analyze_buffer[0] == device_id) && 
				(uart2_analyze_buffer[1] == 0x03) &&
				(uart2_analyze_buffer[2] == 0x00) &&
				(uart2_analyze_buffer[3] == 0x02))		//设备地址相同，那么就判断crc
			{
				temp_crc_data = CRC16(uart2_analyze_buffer,6);
				if(((temp_crc_data & 0x00FF) == uart2_analyze_buffer[6]) &&
					((temp_crc_data >> 8) == uart2_analyze_buffer[7]))		//crc正确，就返回数据
				{
					temp_send_back_data[0] = device_id;
					temp_send_back_data[1] = 0x03;
					temp_data = (uint32_t)(ywy_oil_height * 65536);
					temp_send_back_data[3] = (uint8_t)(temp_data >> 24);
					temp_send_back_data[4] = (uint8_t)((temp_data >> 16) & 0x00FF0000);
					temp_send_back_data[5] = (uint8_t)((temp_data >> 8) & 0x0000FF00);
					temp_send_back_data[6] = (uint8_t)(temp_data & 0x000000FF);

					temp_data = (uint32_t)(ywy_water_height * 65536);
					temp_send_back_data[7] = (uint8_t)(temp_data >> 24);
					temp_send_back_data[8] = (uint8_t)((temp_data >> 16) & 0x00FF0000);
					temp_send_back_data[9] = (uint8_t)((temp_data >> 8) & 0x0000FF00);
					temp_send_back_data[10] = (uint8_t)(temp_data & 0x000000FF);
					
					temp_data = (uint16_t)(ywy_temp1 * 16);
					temp_send_back_data[11] = (uint8_t)(temp_data >> 8);
					temp_send_back_data[12] = (uint8_t)(temp_data & 0x00FF);
					
					temp_data = (uint16_t)(ywy_temp2 * 16);
					temp_send_back_data[13] = (uint8_t)(temp_data >> 8);
					temp_send_back_data[14] = (uint8_t)(temp_data & 0x00FF);
					
					temp_data = (uint16_t)(ywy_temp3 * 16);
					temp_send_back_data[15] = (uint8_t)(temp_data >> 8);
					temp_send_back_data[16] = (uint8_t)(temp_data & 0x00FF);
					
					temp_data = (uint16_t)(ywy_temp4 * 16);
					temp_send_back_data[17] = (uint8_t)(temp_data >> 8);
					temp_send_back_data[18] = (uint8_t)(temp_data & 0x00FF);
					
					temp_data = (uint16_t)(ywy_temp5 * 16);
					temp_send_back_data[19] = (uint8_t)(temp_data >> 8);
					temp_send_back_data[20] = (uint8_t)(temp_data & 0x00FF);
					
					if(temp_receive_ask_length == 9)
					{
						temp_send_back_data[2] = 18;
					}
					else
					{
						temp_send_back_data[2] = 22;
					}

					
				}
			}
		}

		usart2_receive_done = 0;
	}
}


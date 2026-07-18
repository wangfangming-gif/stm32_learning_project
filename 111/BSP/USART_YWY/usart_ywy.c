#include "../BSP/USART_YWY/usart_ywy.h"
#include "../BSP/USART_FIFO/usart_test.h"
#include <string.h>


UART_HandleTypeDef usart_handle;
DMA_HandleTypeDef usart_rx_handle;
DMA_HandleTypeDef usart_tx_handle;

uart_receive_struct modbus_recieve_struct;
static uart_send_struct uart_send_fifo;

#define tx_rx_buffercount 128
uint8_t usart_rx_buffer[tx_rx_buffercount];

volatile uint8_t usart_send_done = 0;	//发送结束标志位,1代表发送完成，0代表发送未完成



#define UART_SEND_BUFFER_SIZE        512
#define UART_SEND_COUNT_SIZE         64
#define UART_DMA_SEND_BUFFER_SIZE    256

static uint8_t uart_send_buffer[UART_SEND_BUFFER_SIZE];
static uint16_t uart_send_count_buffer[UART_SEND_COUNT_SIZE];
static uint8_t uart_dma_send_buffer[UART_DMA_SEND_BUFFER_SIZE];

static void uart_send_fifo_init(void);



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

	//这里需要开始使用DMA
	HAL_UARTEx_ReceiveToIdle_DMA(&usart_handle,usart_rx_buffer,tx_rx_buffercount);
	__HAL_DMA_DISABLE_IT(usart_handle.hdmarx, DMA_IT_HT);

}

static void uart_send_fifo_init(void)
{
	usart_send_done = 0;
	send_loop_buffer_init(&uart_send_fifo,uart_send_buffer,UART_SEND_BUFFER_SIZE,uart_send_count_buffer,UART_SEND_COUNT_SIZE);
}

uint8_t uart_send_data(uint8_t *data, uint16_t len)
{
	return send_loop_buffer_push(&uart_send_fifo, data, len);
}

void uart_send_check(void)
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

////等待发送完毕
//void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
//{
//	if(huart->Instance == USART1)		//如果是USART1的话
//	{
//		usart_send_done = 0;
//	}
//}


//void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
//{
//	if(huart->Instance  == USART1)
//	{
//		loop_buffer_push_array(&modbus_recieve_struct,usart_rx_buffer,Size);
//
//		HAL_UARTEx_ReceiveToIdle_DMA(&usart_handle,usart_rx_buffer,tx_rx_buffercount);
//
//		__HAL_DMA_DISABLE_IT(usart_handle.hdmarx, DMA_IT_HT);
//	}
//}

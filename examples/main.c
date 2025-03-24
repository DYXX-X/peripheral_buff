#include "stm32f1xx_hal.h"
#include "stm32f1xx_it.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "peripheral_buff.h"

#if __GNUC__
int _write(int file, char *ptr, int len)
{
    int DataIdx;
    for (DataIdx = 0; DataIdx < len; DataIdx++)
    {
        while ((USART1->SR & 0X40) == 0)
            ;
        USART1->DR = (uint8_t)*ptr++;
    }
    return len;
}
#else
int fputc(int ch, FILE *stream)
{
    while ((USART1->SR & 0X40) == 0)
        ;
    USART1->DR = (uint8_t)ch;
    return ch;
}
#endif
void SystemClock_Config(void)
{
    HAL_StatusTypeDef ret = HAL_OK;
    RCC_OscInitTypeDef RCC_OscInitStructure;
    RCC_ClkInitTypeDef RCC_ClkInitStructure;
    RCC_OscInitStructure.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStructure.HSEState = RCC_HSE_ON;
    RCC_OscInitStructure.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStructure.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStructure.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStructure.PLL.PLLMUL = RCC_PLL_MUL9;
    ret = HAL_RCC_OscConfig(&RCC_OscInitStructure);
    if (ret != HAL_OK)
        Error_Handler();
    RCC_ClkInitStructure.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStructure.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStructure.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStructure.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStructure.APB2CLKDivider = RCC_HCLK_DIV1;
    ret = HAL_RCC_ClockConfig(&RCC_ClkInitStructure, FLASH_LATENCY_2);
    if (ret != HAL_OK)
        Error_Handler();
}

PeripheralBufHandle PeripheralUart1Buf;
UART_HandleTypeDef uart1_handle;

void USART1_Init(int Baud)
{
    {
        GPIO_InitTypeDef GPIO_Initure;
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_USART1_CLK_ENABLE();
        GPIO_Initure.Pin = GPIO_PIN_9;
        GPIO_Initure.Mode = GPIO_MODE_AF_PP;
        GPIO_Initure.Pull = GPIO_PULLUP;
        GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_Initure);
        GPIO_Initure.Pin = GPIO_PIN_10;
        GPIO_Initure.Mode = GPIO_MODE_AF_INPUT;
        HAL_GPIO_Init(GPIOA, &GPIO_Initure);

        HAL_NVIC_EnableIRQ(USART1_IRQn);
        HAL_NVIC_SetPriority(USART1_IRQn, 3, 3);
    }
    uart1_handle.Instance = USART1;
    uart1_handle.Init.BaudRate = Baud;
    uart1_handle.Init.WordLength = UART_WORDLENGTH_8B;
    uart1_handle.Init.StopBits = UART_STOPBITS_1;
    uart1_handle.Init.Parity = UART_PARITY_NONE;
    uart1_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart1_handle.Init.Mode = UART_MODE_TX_RX;
    if (HAL_UART_Init(&uart1_handle) != HAL_OK)
    {
        Error_Handler();
    }
}

void USART1_IRQHandler()
{
    HAL_UART_IRQHandler(&uart1_handle);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        HAL_UART_Receive_IT(&uart1_handle, (uint8_t *)PeripheralUart1Buf->Buf, PeripheralUart1Buf->Size);
    }
}

char uart1_buffer[1024];
int uart1_buffer_len;

int main()
{
    HAL_Init();
    SystemClock_Config();
    USART1_Init(921600);

    PeripheralUart1Buf = PeripheralBuffer_Init((const uint16_t *)&uart1_handle.RxXferCount, 1024);
    if (!PeripheralUart1Buf)
    {
        printf("peripheral_buffer 初始化失败！(calloc();返回 NULL，检查 startup_stm32fxx_hd.s Heap_Size 值)\n");
        printf("相关网址： https://blog.csdn.net/weixin_42518229/article/details/108574311 \n");
    }
    HAL_UART_Receive_IT(&uart1_handle, (uint8_t *)PeripheralUart1Buf->Buf, PeripheralUart1Buf->Size);

    while (1)
    {
        if (PeripheralBuffer_ReadInterval(PeripheralUart1Buf))
        {
            uart1_buffer_len = PeripheralBuffer_ReadAvailable(PeripheralUart1Buf);
            if (uart1_buffer_len > 0)
            {
                if (uart1_buffer_len > sizeof(uart1_buffer))
                    uart1_buffer_len = sizeof(uart1_buffer);
                printf("数据来临 %d\n", uart1_buffer_len);
                PeripheralBuffer_ReadBytes(PeripheralUart1Buf, uart1_buffer, uart1_buffer_len);
                uart1_buffer[uart1_buffer_len] = 0;
                printf("%s\n", uart1_buffer);
            }
            else
            {
                printf("err %d\n", uart1_buffer_len);
            }
        }
        HAL_Delay(10);
    }
    return 0;
}

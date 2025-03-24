# STM32外部缓冲区
## 介绍：<br/>
STM32 HAL库 外部缓冲区，实现数据缓冲功能，可用于UART，IIC，SPI...等外设，适用于数据不定时，不定量接收场景！

## 实现原理：<br/>
HAL 库 UART，SPI，IIC等外设的结构体句柄  [xx_HandleTypeDef] 几乎都有一个成员，RxXferCount 即 Rx传输计数器。此成员每接收一个字节，值就+1，Rx传输已完成回调调用之后，值就重新计数。由此值变化特性，就可以构建一个环形缓冲区！<br/>

## 用户代码片段：<br/>
```
#include "peripheral_buff.h"

PeripheralBufHandle PeripheralUart1Buf;
UART_HandleTypeDef uart1_handle;

char uart1_buffer[1024];
int uart1_buffer_len;

int main()
{
    HAL_Init();
    SystemClock_Config(); // 此处未实现
    USART1_Init(921600);  // 此处未实现

    // 缓冲区初始化
    PeripheralUart1Buf = PeripheralBuffer_Init((const uint16_t *)&uart1_handle.RxXferCount, 1024);
    if (!PeripheralUart1Buf)
    {
        printf("peripheral_buffer 初始化失败！(calloc();返回 NULL，检查 startup_stm32fxx_hd.s Heap_Size 值)\n");
        printf("相关网址： https://blog.csdn.net/weixin_42518229/article/details/108574311 \n");
    }
    // 开启中断接收
    HAL_UART_Receive_IT(&uart1_handle, (uint8_t *)PeripheralUart1Buf->Buf, PeripheralUart1Buf->Size);

    while (1)
    {

        if (PeripheralBuffer_ReadInterval(PeripheralUart1Buf)) // 判断当前传输是否已停止,非必要
        {
            uart1_buffer_len = PeripheralBuffer_ReadAvailable(PeripheralUart1Buf); // 读取缓冲区内内容长度
            if (uart1_buffer_len > 0)
            {
                if (uart1_buffer_len > sizeof(uart1_buffer))
                    uart1_buffer_len = sizeof(uart1_buffer);
                PeripheralBuffer_ReadBytes(PeripheralUart1Buf, uart1_buffer, uart1_buffer_len); // 读取缓冲区内内容
                uart1_buffer[uart1_buffer_len] = 0;
                printf("缓冲区数据 %s\n", uart1_buffer);
            }
        }
        HAL_Delay(10);
    }
    return 0;
}
```

## 效果说明：<br/>
串口监视器发啥收啥，无卡顿！<br/>

## 性能测试：<br/>
单次发送500字节，间隔20ms，运行一小时，无卡顿，无损坏，无乱序！<br/>

## 性能瓶颈：<br/>
分配合适的RAM数值。
移植RTOS，增加cpu利用率。

## debug：<br/>
1.PeripheralBuffer_Init 返回 NULL：calloc生请内存失败，减小数值，或者修改 `startup_stm32fxx_hd.s` `Heap_Size` 值。 [[Heap_Size]](https://blog.csdn.net/weixin_42518229/article/details/108574311)  <br/>
2.`uart1_buffer`大小根据实际合理分配，如`PeripheralBuffer_Init`分配值大于`uart1_buffer`需检查每次读取长度。<br/>
3.接收大文件需要更大缓冲区。<br/>

## 编译
由于项目构建于GCC编译器，而stm32常用开发环境为Keil，若源码编译失败，可尝试链接lib文件夹下的静态库，有GCC与Keil的两种版本可挑选使用。

## 示例
详细示例程序，见examples文件夹。

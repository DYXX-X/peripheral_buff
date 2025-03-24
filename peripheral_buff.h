#ifndef __PERIPHERAL_BUFF_H
#define __PERIPHERAL_BUFF_H

/*
  版本 1.1.0
  修改日期 25/03/23
*/

#include "stdint.h"
#include "stdbool.h"

typedef struct _PeripheralBufHandles
{
  uint16_t Size; // 缓冲区长度
  char Buf[1];   // 缓冲区指针
} *PeripheralBufHandle;

#ifdef __cplusplus
extern "C"
{
#endif
  /**
   * @brief 缓冲区初始化
   * @param RxXferCount 外设句柄 RxXferCount 成员指针
   * @param Len 给定的缓冲区长度
   * @return 缓冲区句柄 为NULL则内存不足！
   */
  PeripheralBufHandle PeripheralBuffer_Init(const uint16_t *RxXferCount, uint16_t Len);
  /**
   * @brief 重置缓冲区
   * @param Handle buffer_init 返回的缓冲区句柄
   */
  void PeripheralBuffer_Rest(PeripheralBufHandle Handle);
  /**
   * @brief 判断当前传输是否已停止
   * @param Handle PeripheralBuffer_Init 返回的缓冲区句柄
   * @return
   *        true：已停止
   *        false：传输中
   */
  bool PeripheralBuffer_ReadInterval(PeripheralBufHandle Handle);
  /**
   * @brief 读取缓冲区内内容长度
   * @param Handle PeripheralBuffer_Init 返回的缓冲区句柄
   * @return 缓冲区内内容长度 单位：字节
   */
  int PeripheralBuffer_ReadAvailable(PeripheralBufHandle Handle);
  /**
   * @brief 读取缓冲区内内容
   * @param Handle PeripheralBuffer_Init 返回的缓冲区句柄
   * @param Data 存储内容指针
   * @param Len 读取长度 单位：字节
   * @return 已读取长度 单位：字节
   */
  int PeripheralBuffer_ReadBytes(PeripheralBufHandle Handle, void *Data, uint16_t Len);
  /**
   * @brief 删除缓冲区
   * @param Handle PeripheralBuffer_Init 返回的缓冲区句柄
   */
  void PeripheralBuffer_DeInit(PeripheralBufHandle Handle);
#ifdef __cplusplus
}
#endif
#endif
#include <peripheral_buff.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define USER_OFFSETOF(TYPE, MEMBER) ((size_t)&((TYPE *)0)->MEMBER)
/**
 * @brief 根据结构体成员地址获取整个结构体的首地址
 * @param ptr:        指向成员的指针
 * @param type:       它嵌入的容器结构的类型
 * @param member:     结构体中成员的名称
 * @return 结构体的首地址
 */
#define USER_CONTAINER_OF(ptr, type, member) ({\
        const typeof( ((type *)0)->member ) *__mptr = (const typeof( ((type *)0)->member ) *)(ptr); \
        (type *)( (char *)__mptr - USER_OFFSETOF(type,member) ); })

#if __GNUC__
#define _CONTAINER_OF __containerof
#else
#define _CONTAINER_OF(ptr, type, member) ((type *)(((int)ptr) - (int)(&(((type *)0)->member))))
#endif

/**
 * @brief 获取计数值
 * @param pr: 计数值指针
 */
#define RX_XFER_COUNT(pr) (*(pr->Count.BufCount))

typedef struct _PrivateBufHandle
{
    // buffer计数器 定义
    struct _Count
    {
        const uint16_t *BufCount; // buffer计数器指针，外部传入
    } Count;

    // buf读取 定义
    struct _ReadBuf
    {
        char *BasicBuf;    // 缓冲区基地址
        int16_t LastIxdex; // 上次读取索引
        int16_t Len;       // 储存变量，用于储存读取长度
    } ReadBuf;

    // 缓冲区可用计算 定义
    struct _ReadAvailable
    {
        uint16_t LastBufCount; // 上次接收计数
        uint16_t Len;          // 缓冲区可用字节数
    } ReadAvailable;

    // 内容传输？ 定义
    struct _ReadInterval
    {
        uint16_t LastBufCount; // 上次接收计数
    } ReadInterval;

    // 缓冲区 定义
    struct _Buf
    {
        uint16_t Size; // 缓冲区长度
        char Buf[1];   // 缓冲区指针
    } Buf;
} *PrivateBufHandle;

/**
 * @brief 缓冲区初始化
 * @param RxXferCount 外设句柄 RxXferCount 成员指针
 * @param Len 给定的缓冲区长度
 * @return 缓冲区句柄 为NULL则内存不足！
 */
PeripheralBufHandle PeripheralBuffer_Init(const uint16_t *RxXferCount, uint16_t Len)
{
    PrivateBufHandle PrivateHandle = calloc(1, sizeof(PrivateBufHandle) + Len - 1);
    if (PrivateHandle == NULL)
        return NULL;

    PrivateHandle->Count.BufCount = RxXferCount;
    PrivateHandle->Buf.Size = Len;
    PrivateHandle->ReadInterval.LastBufCount = Len;
    PrivateHandle->ReadAvailable.Len = 0;
    PrivateHandle->ReadAvailable.LastBufCount = Len;
    PrivateHandle->ReadBuf.BasicBuf = PrivateHandle->Buf.Buf;

    return (PeripheralBufHandle)&PrivateHandle->Buf;
}

/**
 * @brief 重置缓冲区
 * @param Handle buffer_init 返回的缓冲区句柄
 */
void PeripheralBuffer_Rest(PeripheralBufHandle Handle)
{
    if (Handle == NULL)
        return;
    // PrivateBufHandle UserPr = _CONTAINER_OF(Handle, struct _PrivateBufHandle, Buf);
    PrivateBufHandle UserPr = _CONTAINER_OF(&Handle->Buf, struct _PrivateBufHandle, Buf.Buf);
    UserPr->ReadAvailable.LastBufCount = RX_XFER_COUNT(UserPr);
    UserPr->ReadBuf.LastIxdex = UserPr->Buf.Size - UserPr->ReadAvailable.LastBufCount;
}

/**
 * @brief 判断当前传输是否已停止
 * @param Handle PeripheralBuffer_Init 返回的缓冲区句柄
 * @return
 *        true：已停止
 *        false：传输中
 */
bool PeripheralBuffer_ReadInterval(PeripheralBufHandle Handle)
{
    if (Handle == NULL)
        return true;
    PrivateBufHandle UserPr = _CONTAINER_OF(&Handle->Buf, struct _PrivateBufHandle, Buf.Buf);
    if (UserPr->ReadInterval.LastBufCount == RX_XFER_COUNT(UserPr))
        return true;

    UserPr->ReadInterval.LastBufCount = RX_XFER_COUNT(UserPr);
    return false;
}

/**
 * @brief 读取缓冲区内内容长度
 * @param Handle PeripheralBuffer_Init 返回的缓冲区句柄
 * @return 缓冲区内内容长度 单位：字节
 */
int PeripheralBuffer_ReadAvailable(PeripheralBufHandle Handle)
{
    if (Handle == NULL)
        return -1;

    PrivateBufHandle UserPr = _CONTAINER_OF(&Handle->Buf, struct _PrivateBufHandle, Buf.Buf);
    uint16_t Count = RX_XFER_COUNT(UserPr);
    if (UserPr->ReadAvailable.LastBufCount == Count)
        return 0;

    if (UserPr->ReadAvailable.LastBufCount < Count)
        UserPr->ReadAvailable.LastBufCount += UserPr->Buf.Size;

    UserPr->ReadAvailable.Len = UserPr->ReadAvailable.LastBufCount - Count;
    return UserPr->ReadAvailable.Len;
}

/**
 * @brief 读取缓冲区内内容
 * @param Handle PeripheralBuffer_Init 返回的缓冲区句柄
 * @param Data 存储内容指针
 * @param Len 读取长度 单位：字节
 * @return 已读取长度 单位：字节
 */
int PeripheralBuffer_ReadBytes(PeripheralBufHandle Handle, void *Data, uint16_t Len)
{
    if (Handle == NULL)
        return -1;

    PrivateBufHandle UserPr = _CONTAINER_OF(&Handle->Buf, struct _PrivateBufHandle, Buf.Buf);
    if (Len > UserPr->Buf.Size)
        Len = UserPr->Buf.Size;

    if (Len > UserPr->ReadAvailable.Len)
        Len = UserPr->ReadAvailable.Len;

    if ((UserPr->ReadBuf.LastIxdex + Len) > UserPr->Buf.Size)
    {
        UserPr->ReadBuf.Len = UserPr->Buf.Size - UserPr->ReadBuf.LastIxdex;
        memcpy(Data, UserPr->ReadBuf.BasicBuf + UserPr->ReadBuf.LastIxdex, UserPr->ReadBuf.Len);
        memcpy((((char *)Data) + UserPr->ReadBuf.Len), UserPr->ReadBuf.BasicBuf, Len - UserPr->ReadBuf.Len);
        UserPr->ReadBuf.LastIxdex = Len - UserPr->ReadBuf.Len;
        UserPr->ReadAvailable.LastBufCount = UserPr->Buf.Size - UserPr->ReadBuf.LastIxdex;
    }
    else
    {
        memcpy(Data, UserPr->ReadBuf.BasicBuf + UserPr->ReadBuf.LastIxdex, Len);
        UserPr->ReadBuf.LastIxdex += Len;
        UserPr->ReadAvailable.LastBufCount -= Len;
    }
    return Len;
}

/**
 * @brief 删除缓冲区
 * @param Handle PeripheralBuffer_Init 返回的缓冲区句柄
 */
void PeripheralBuffer_DeInit(PeripheralBufHandle Handle)
{
    if (Handle == NULL)
        return;

    PrivateBufHandle UserPr = _CONTAINER_OF(&Handle->Buf, struct _PrivateBufHandle, Buf.Buf);
    free(UserPr);
}
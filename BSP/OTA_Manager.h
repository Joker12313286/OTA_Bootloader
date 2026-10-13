#ifndef __OTA_MANAGER__H__
#define  __OTA_MANAGER__H__

#include "main.h"
#include "W25Q64.h"

//最后一个扇形区作为OTA是否升级的标志位		4kB
//OTA升级标志位地址
#define OTA_FLAG_ADDR      (0x7ff000)    //8MB-4KB	

/*--------------------- 标志值定义 ---------------------*/
typedef enum
{
    OTA_FLAG_IDLE=0xFF,                           /**< 空闲状态（扇区擦除后的默认值） */
    OTA_FLAG_UPDATE_PENDING=0xA5,                 /**< 有新固件待升级（App下载完成后写入） */
    OTA_FLAG_UPDATE_SUCCESS=0X5A,                 /**< 升级成功（Bootloader升级完成后写入） */
    OTA_FLAG_UPDATE_FAILED=0XF0                   /**< 升级失败（Bootloader检测到错误后写入） */
}OTAFlagStatus;

/**
  * @brief 初始化 OTA 标志管理
  * @note  首次使用时擦除标志扇区，确保标志位处于已知状态
  *        只需在系统初始化时调用一次
  * @return 0: 成功, 1: 失败
  */
 uint8_t OTA_Flag_Init(void);

/**
  * @brief 写入 OTA 升级标志
  * @param flag 要写入的标志值（只能是OTAFlagStatus有的值）
  * @return 0: 成功, 1: 失败
  * @note  写入前会先擦除整个扇区，然后写入1字节标志到扇区起始地址
  */
uint8_t OTA_Flag_Write(OTAFlagStatus flag);

/**
  * @brief 读取 OTA 升级标志
  * @param pFlag 存储读取到的标志值
  * @return 0: 成功, 1: 失败
  */
uint8_t OTA_Flag_Read(OTAFlagStatus *pFlag);

/**
  * @brief 清除 OTA 升级标志（恢复到空闲状态）
  * @return 0: 成功, 1: 失败
  * @note  直接擦除整个标志扇区，等价于写入 OTA_FLAG_IDLE
  *        注意：擦除后标志值变为 0xFF
  */
uint8_t OTA_Flag_Clear(void);

#endif

#include "OTA_Manager.h"

/**
  * @brief 初始化 OTA 标志管理
  * @note  首次使用时擦除标志扇区，确保标志位处于已知状态
  *        只需在系统初始化时调用一次
  * @return 0: 成功, 1: 失败
  */
uint8_t OTA_Flag_Init(void)
{
    // 先读取当前标志，如果已经是空闲状态（0xFF）则跳过擦除
    OTAFlagStatus current_flag;
    if(OTA_Flag_Read(&current_flag)==0&&current_flag==OTA_FLAG_IDLE)
    {
        // 已经是空闲状态，无需操作
        return 0;
    }
    // 否则执行擦除
    W25Q64_SectorErase(OTA_FLAG_ADDR);
    if(W25Q64_WaitBusy()!=0)
    {
        // 超时失败
        return 1;
    }
    return 0;
}

/**
  * @brief 写入 OTA 升级标志
  * @param flag 要写入的标志值（只能是OTAFlagStatus有的值）
  * @return 0: 成功, 1: 失败
  * @note  写入前会先擦除整个扇区，然后写入1字节标志到扇区起始地址
  */
uint8_t OTA_Flag_Write(OTAFlagStatus flag)
{
    // 1. 先擦除整个标志扇区（将全部数据变为 0xFF）
    W25Q64_SectorErase(OTA_FLAG_ADDR);
    if(W25Q64_WaitBusy()!=0)
    {
        // 擦除超时
        return 1;
    }

    // 2. 写入标志值到扇区起始地址（页编程最多256字节，写1字节没问题）
    W25Q64_PageProgram(OTA_FLAG_ADDR,&flag,1);
    if(W25Q64_WaitBusy()!=0)
    {
        return 1;
    }
    return 0;
}

/**
  * @brief 读取 OTA 升级标志
  * @param pFlag 存储读取到的标志值
  * @return 0: 成功, 1: 失败
  */
uint8_t OTA_Flag_Read(OTAFlagStatus *pFlag)
{
    if (pFlag == NULL) {
        return 1;  // 参数错误
    }

    // 从标志扇区起始地址读取1字节
    W25Q64_ReadData(OTA_FLAG_ADDR, pFlag, 1);
    return 0;
}

/**
  * @brief 清除 OTA 升级标志（恢复到空闲状态）
  * @return 0: 成功, 1: 失败
  * @note  直接擦除整个标志扇区，等价于写入 OTA_FLAG_IDLE
  *        注意：擦除后标志值变为 0xFF
  */
uint8_t OTA_Flag_Clear(void)
{
    W25Q64_SectorErase(OTA_FLAG_ADDR);
    if (W25Q64_WaitBusy() != 0) {
        return 1;  // 擦除超时
    }
    return 0;
}

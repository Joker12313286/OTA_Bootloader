#ifndef __W25Q64_H
#define __W25Q64_H

#include "main.h"
#include "W25Q64_Ins.h"

//外部flash页大小（W25Q64位4K）
#define ExFLASH_PAGE_SIZE         4096

/*--------------------- 外部 Flash 固件存储位置 ---------------------*/
#define OTA_FW_SIZE_ADDR       0X00000000          // 固件大小存储地址  大小0x00000000 ~ 0x00000003      4B
#define OTA_FW_DOWNLOAD_ADDR   0X00000004          // 新固件地址        大小0x00000004 ~ 0x005FFFFF     6MB
#define OTA_FW_BACKUP_ADDR     0X00600000          // 备份固件地址      大小0x00600000 ~ 0x007FEFFF     2MB-4KB
#define OTA_FW_FLAG_ADDR       0x007ff000          // 固件标志地址      大小0x007ff000 ~ 0x007fffffff   4KB

// 备份区尾部存储 CRC 的偏移（用于校验备份完整性）
#define OTA_BACKUP_CRC_OFFSET (APP_MAX_SIZE)      // 备份区最后4字节存CRC

void W25Q64_Init(void);
void W25Q64_ReadID(uint8_t *MID, uint16_t *DID);
void W25Q64_PageProgram(uint32_t Address, uint8_t *DataArray, uint16_t Count);
void W25Q64_SectorErase(uint32_t Address);
void W25Q64_ReadData(uint32_t Address, uint8_t *DataArray, uint32_t Count);
uint8_t W25Q64_WaitBusy(void);
void W25Q64_WriteEnable(void);

#endif

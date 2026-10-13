#ifndef __BOOTLOADER__H__
#define  __BOOTLOADER__H__

#include "main.h"
#include "W25Q64.h"
#include "OTA_Manager.h"
#include "app_verify.h"

/*--------------------- 内部 Flash 分区定义 ---------------------*/
//flash起始地址 64K
#define FLASH_BASE_ADDRESS 0x08000000
//栈起始地址 空间为20K 范围：0x20000000 ~ 0x20005000
#define STACK_ADDRESS      0x20000000
//程序写入的起始地址（加偏移）->A区起始位置 假设B区大小16K（空间为0x4000） A区为64K-16K（空间为0xC000）
#define APP_START_ADDR   FLASH_BASE_ADDRESS + 0x4000
#define APP_END_ADDR     APP_START_ADDR + 0xC000
#define APP_MAX_SIZE            (APP_END_ADDR - APP_START_ADDR) // 48KB

// 内部Flash页大小（STM32F103C8T6为1KB）
//#define InFLASH_PAGE_SIZE         1024




/*--------------------- 函数声明 ---------------------*/
//返回值 0为升级成功 1为超时 2为升级失败回滚失败 3为升级失败回滚成功 4为备份失败 
uint8_t Perform_OTA_Upgrade(void);       // 执行固件搬运 + 升级失败回滚机制
void Bootloader_Flag_Handler(void);      // 标志处理 + 升级决策
void Bootloader_to_APP(void);            // 跳转到 App

//返回值 0表示擦除成功  1表示擦除失败尝试回滚 2为擦除地址超出阈值
static uint8_t IntFlash_ErasePageIfNeeded(uint32_t target_addr);   //内部Flash页擦除（只在跨页时执行）

// 记录当前已经擦除到的页地址（用于跨页时自动擦除新页）
static uint32_t current_page_addr = 0xFFFFFFFF;

#endif

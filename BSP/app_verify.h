#ifndef __APP_VERIFY__H__
#define  __APP_VERIFY__H__

#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include "W25Q64.h"

/*--------------------- 返回值定义 ---------------------*/
#define VERIFY_PASS     0
#define VERIFY_FAIL     1

/*--------------------- 栈顶地址 ---------------------*/
//栈起始地址 空间为20K 范围：0x20000000 ~ 0x20005000
#define STACK_ADDRESS      0x20000000

/*--------------------- 魔数（固件标识） ---------------------*/
// 魔数：放在固件开头（外部偏移0，内部偏移8）
#define APP_MAGIC_NUMBER   0x5A5A5A5A   

/*--------------------- CRC 校验 ---------------------*/
/**
  * @brief 计算外部 Flash 中指定区域的 CRC32（不含尾部4字节CRC字段）
  * @param addr  外部 Flash 起始地址
  * @param size  固件总大小（含尾部CRC）
  * @return 计算出的 CRC32 值
  */
uint32_t Verify_CRC32_External(uint32_t addr, uint32_t size);

/**
  * @brief 计算内部 Flash 中指定区域的 CRC32（不含尾部4字节CRC字段）
  * @param addr  内部 Flash 起始地址
  * @param size  固件总大小（含尾部CRC）
  * @return 计算出的 CRC32 值
  */
uint32_t Verify_CRC32_Internal(uint32_t addr, uint32_t size);

/**
  * @brief 校验外部 Flash 固件是否有效（魔数 + CRC）
  * @param addr  外部 Flash 起始地址
  * @param size  固件大小（含尾部CRC）
  * @return VERIFY_PASS: 校验通过, VERIFY_FAIL: 校验失败
  */
uint8_t Verify_Firmware_External(uint32_t addr, uint32_t size);

/**
  * @brief 校验内部 Flash 固件是否有效（栈顶 + 魔数 + CRC）
  * @param addr  内部 Flash 起始地址（如 APP_START_ADDR）
  * @param size  固件大小（含尾部CRC）
  * @return VERIFY_PASS: 校验通过, VERIFY_FAIL: 校验失败
  */
uint8_t Verify_Firmware_Internal(uint32_t addr, uint32_t size);

// TODO: 预留——签名验证、白名单验证
// uint8_t Verify_Firmware_Signature(uint32_t addr, uint32_t size);
// uint8_t Verify_Firmware_Whitelist(uint32_t addr, uint32_t size);

#endif

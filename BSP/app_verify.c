#include "app_verify.h"

/*固件包格式
*内部 栈顶地址（4B）+复位向量（4B）+魔数（4B）+固件数据+CRC32校验码（4B）
*外部 魔数（4B）+固件数据+CRC32校验码（4B）
*/

/**
  * @brief CRC32 计算核心函数（软件实现）适合单次计算CRC值
  * @param data   数据指针
  * @param len    数据长度
  * @return 计算出的 CRC32 值
  */
/* static uint32_t CRC32_Calculate(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}
 */

/**
  * @brief CRC32 分段累加核心（不取反，不重置初始值）第一次使用该函数需设置CRC初始值为0xFFFFFFFF
  * @param crc_in 上一轮计算的 CRC 中间值
  * @param data   数据指针
  * @param len    数据长度
  * @return 累加后的 CRC 中间值（未取反）
  */
static uint32_t CRC32_Update(uint32_t crc_in, const uint8_t *data, uint32_t len)
{
    uint32_t crc = crc_in;  // 关键：延续上一轮的状态，绝不重置为 0xFFFFFFFF
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;  // 关键：不取反，保留原始中间状态
}

/**
  * @brief 计算外部 Flash 中指定区域的 CRC32（不含尾部4字节CRC字段）
  * @param addr  外部 Flash 起始地址
  * @param size  固件总大小（含尾部CRC）
  * @return 计算出的 CRC32 值
  */
 uint32_t Verify_CRC32_External(uint32_t addr, uint32_t size)
 {
    if(size<4)return 0; // 至少要有CRC字段，否则返回错误值（调用方会校验失败）
    uint8_t buf[256] = {0};
    uint32_t crc = 0xFFFFFFFF;
    uint32_t remaining = size - 4;  // 不包含尾部CRC
    uint32_t read_addr = addr;
    while(remaining>0)
    {
        uint32_t read_len = remaining>256?256:remaining;
        W25Q64_ReadData(read_addr,buf,read_len);
        crc = CRC32_Update(crc,buf,read_len);
        read_addr += read_len;
        remaining -= read_len;
    }
    return ~crc;
 }

/**
  * @brief 计算内部 Flash 中指定区域的 CRC32（不含尾部4字节CRC字段）
  * @param addr  内部 Flash 起始地址
  * @param size  固件总大小（含尾部CRC）
  * @return 计算出的 CRC32 值
  */
 uint32_t Verify_CRC32_Internal(uint32_t addr, uint32_t size)
 {
    if(size<4)return 0; // 至少要有CRC字段，否则返回错误值（调用方会校验失败）s
    uint32_t read_addr = addr;
    uint8_t buf[256] = {0};
    uint32_t crc = 0xFFFFFFFF;
    uint32_t remaining = size - 4;
    while(remaining>0)
    {
        uint32_t read_len = remaining>256?256:remaining;
        for(uint32_t i=0;i<read_len;i++)
        {
            buf[i] = *(uint8_t*)(read_addr+i);
        }
        crc = CRC32_Update(crc,buf,read_len);
        read_addr += read_len;
        remaining -= read_len;
    }
    return ~crc;
 }

/**
  * @brief 校验外部 Flash 固件是否有效（魔数 + CRC）
  * @param addr  外部 Flash 起始地址
  * @param size  固件大小（含尾部CRC）
  * @return VERIFY_PASS: 校验通过, VERIFY_FAIL: 校验失败
  */
uint8_t Verify_Firmware_External(uint32_t addr, uint32_t size)
{
    // 1. 检查大小是否至少能容纳魔数(4) + CRC(4)
    if (size < 8) return VERIFY_FAIL;

    // 2. 读取魔数（开头4字节）
    uint32_t magic = 0;
    uint8_t buf[4] = {0};
    W25Q64_ReadData(addr,buf,4);
    magic = (uint32_t)buf[0]|
            ((uint32_t)buf[1]<<8)|
            ((uint32_t)buf[2]<<16)|
            ((uint32_t)buf[3]<<24);
    if(magic != APP_MAGIC_NUMBER)
    {
        return VERIFY_FAIL;
    }

     // 3. 读取尾部CRC（固件最后4字节）
    uint32_t crc_expected;
    uint8_t crc_buf[4] = {0};
    W25Q64_ReadData(addr+size-4,crc_buf,4);
    crc_expected = (uint32_t)crc_buf[0]|
                   ((uint32_t)crc_buf[1]<<8)|
                   ((uint32_t)crc_buf[2]<<16)|
                   ((uint32_t)crc_buf[3]<<24);
     // 4. 计算CRC（从开头到尾部CRC之前）
    uint32_t crc_calc = Verify_CRC32_External(addr,size);
    if(crc_calc!=crc_expected)
    {
        return VERIFY_FAIL;
    }
    return VERIFY_PASS;
}

/**
  * @brief 校验内部 Flash 固件是否有效（栈顶 + 魔数 + CRC）
  * @param addr  内部 Flash 起始地址（如 APP_START_ADDR）
  * @param size  固件大小（含尾部CRC）
  * @return VERIFY_PASS: 校验通过, VERIFY_FAIL: 校验失败
  */
uint8_t Verify_Firmware_Internal(uint32_t addr, uint32_t size)
{
    // 1. 检查大小是否至少能容纳栈顶(4) +复位向量（4） + 魔数(4) + CRC(4) = 16字节
    if (size < 16) return VERIFY_FAIL;

    // 1.1 栈顶地址校验（快速检查）
    uint32_t stack_ptr = *(volatile uint32_t*)addr;
    if((stack_ptr & 0xffff0000) != STACK_ADDRESS)
    {
        return VERIFY_FAIL;
    }

    //1.2 魔数校验（紧接栈顶）
    uint32_t magic = 0;
    uint8_t *p_magic = (uint8_t*)(addr+8);
    magic = (uint32_t)p_magic[0]|
            ((uint32_t)p_magic[1]<<8)|
            ((uint32_t)p_magic[2]<<16)|
            ((uint32_t)p_magic[3]<<24);
    if(magic != APP_MAGIC_NUMBER)
    {
        return VERIFY_FAIL;
    }

    // 2. 读取尾部CRC（固件最后4字节）
    uint32_t crc_expected; 
    uint8_t *p_crc = (uint8_t*)(addr + size -4);
    crc_expected = (uint32_t)p_crc[0]|
                   ((uint32_t)p_crc[1]<<8)|
                   ((uint32_t)p_crc[2]<<16)|
                   ((uint32_t)p_crc[3]<<24);

    // 3. 计算CRC（从addr开始，长度为size-4，不包括尾部CRC） 函数内部以自动执行故直接size即可
    uint32_t crc_calc = Verify_CRC32_Internal(addr,size);
    if(crc_calc!=crc_expected)
    {
        return VERIFY_FAIL;
    }
    return VERIFY_PASS;
}

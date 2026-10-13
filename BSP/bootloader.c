#include "bootloader.h"

// 重试计数器（静态局部变量，仅在 Perform_OTA_Upgrade 中使用）
static uint8_t retry_count = 0;

/**
  * @brief 内部Flash页擦除（只在跨页时执行）
  * @param target_addr 目标写入地址
  */
static uint8_t IntFlash_ErasePageIfNeeded(uint32_t target_addr)
{
    uint32_t page_addr = target_addr - (target_addr % FLASH_PAGE_SIZE);
    //如果地址错误，自动回滚
    if(page_addr<APP_START_ADDR || page_addr>APP_END_ADDR)
    {
        return 2;
    }
    if(page_addr != current_page_addr)
    {
        // 跨页了，擦除新页
        FLASH_EraseInitTypeDef EraseInitStruct;
        uint32_t PageError = 0;
        EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
        EraseInitStruct.Banks = FLASH_BANK_1;
        EraseInitStruct.PageAddress = page_addr;
        EraseInitStruct.NbPages = 1;
        if(HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK)
        {
            // 擦除失败，上锁并跳转到回滚
            return 1;
        }
        current_page_addr = page_addr;
    }
    return 0;
}

/**
  * @brief 核心 OTA 升级函数：从 W25Q64 搬运固件到内部 Flash（App区）。失败具备回滚机制
  * @return 0: 成功, 1: 失败
  */
 uint8_t Perform_OTA_Upgrade(void)
 {
    uint8_t buf[256] = {0};
    uint32_t fw_size = 0;
    uint32_t internal_addr = APP_START_ADDR;
    uint32_t backup_addr = OTA_FW_BACKUP_ADDR;
    uint32_t external_addr = OTA_FW_DOWNLOAD_ADDR;
    uint32_t remaining = 0;
    uint32_t crc_backup_calc = 0;
    //uint32_t crc_backup_expected = 0;
    uint32_t crc_internal = 0;
    // ==================== 第1步：读取新固件大小 ====================
    W25Q64_ReadData(OTA_FW_SIZE_ADDR, (uint8_t *)&fw_size, 4);
    if(fw_size == 0 || fw_size > APP_MAX_SIZE)
    {
        return 1;   // 固件大小异常
    }

    // ==================== 第2步：备份旧固件到 W25Q64 Backup 区 ====================
    // 2.1 擦除 Backup 区（先擦除整个 Backup 区，确保干净）
     // 这里简化：擦除 Backup 区的前几个扇区（覆盖旧固件大小即可）
    backup_addr = OTA_FW_BACKUP_ADDR;
    uint32_t backup_remaining = APP_MAX_SIZE;
     // 按4KB扇区擦除 Backup 区
     while(backup_remaining > 0)
     {
        W25Q64_SectorErase(backup_addr);
        backup_addr += ExFLASH_PAGE_SIZE;
        if(backup_remaining > ExFLASH_PAGE_SIZE)
        {
            backup_remaining -= ExFLASH_PAGE_SIZE;
        }
        else
        {
            backup_remaining = 0;
        }
     }
     // 2.2 从内部 Flash 读出旧固件，写入 Backup 区
     backup_addr = OTA_FW_BACKUP_ADDR;
     internal_addr = APP_START_ADDR;
     backup_remaining = APP_MAX_SIZE;
     while(backup_remaining > 0)
     {
        uint32_t read_len = (backup_remaining > 256) ? 256 : backup_remaining;
        // 从内部 Flash 读旧固件
        for(uint32_t i = 0; i < read_len; i++)
        {
            buf[i] = *(uint8_t *)(internal_addr + i);
        }
        // 写入 Backup 区
        W25Q64_PageProgram(backup_addr,buf,read_len);
        
        backup_addr += read_len;
        internal_addr += read_len;
        backup_remaining -= read_len;
     }
     // 2.3 校验备份数据完整性（比较内部Flash和Backup区的CRC）
     crc_internal = Verify_CRC32_Internal(APP_START_ADDR,APP_MAX_SIZE);
     crc_backup_calc = Verify_CRC32_External(OTA_FW_BACKUP_ADDR,APP_MAX_SIZE);
     if(crc_internal != crc_backup_calc)
     {
        // 备份数据损坏，直接返回失败（无法回滚）
        return 4;
     }

     // ==================== 第3步：解锁内部 Flash，准备写入新固件 ====================
     HAL_FLASH_Unlock();
     //开局重置current_page_addr的值，避免无法擦除首页
     current_page_addr = 0xFFFFFFFF;
     if(IntFlash_ErasePageIfNeeded(APP_START_ADDR) != 0)
     {
        goto ROLLBACK;
     }
     

     // ==================== 第4步：写入新固件 ====================
     internal_addr = APP_START_ADDR;
     external_addr = OTA_FW_DOWNLOAD_ADDR;
     remaining = fw_size;
     while(remaining > 0)
     {
        uint32_t write_len = (remaining > 256) ? 256 : remaining;
        W25Q64_ReadData(external_addr,buf,write_len);
        for(uint32_t i = 0; i < write_len; i+=2)
        {
            uint16_t data = (buf[i+1]<<8)|buf[i];
            if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,internal_addr+i,data) != HAL_OK)
            {
                //写入失败，回滚到备份区
                HAL_FLASH_Lock();
                goto ROLLBACK;
            }
        }
        external_addr += write_len;
        internal_addr += write_len;
        remaining -= write_len;
     }

    // ==================== 第5步：校验新固件（CRC 校验） ====================
    if (Verify_Firmware_External(OTA_FW_DOWNLOAD_ADDR, fw_size) != VERIFY_PASS) {
        HAL_FLASH_Lock();
        goto ROLLBACK;
    }

    // ==================== 第6步：锁定 Flash，返回成功 ====================
    HAL_FLASH_Lock();
    retry_count = 0; // 重试计数器清零
    return 0;  // 升级成功

    // ==================== 回滚标签 ====================
ROLLBACK:
     // 从 Backup 区恢复旧固件到内部 Flash
    // 注意：此时内部 Flash 可能部分被破坏，需要重新擦除再写入
    HAL_FLASH_Unlock();
    // 7.1 重新擦除 APP 区（先擦除所有页）
     current_page_addr = 0xFFFFFFFF;
     if(IntFlash_ErasePageIfNeeded(APP_START_ADDR) != 0)
     {
        //回滚尝试3次，不行直接返回2
        retry_count++;
        if(retry_count==3) {HAL_FLASH_Lock(); return 2;}
        goto ROLLBACK;
     }
    // 7.2 从 Backup 区恢复旧固件
     internal_addr = APP_START_ADDR;
     backup_addr = OTA_FW_BACKUP_ADDR;
     backup_remaining = APP_MAX_SIZE;
     while(backup_remaining > 0)
     {
        uint32_t read_len = (backup_remaining > 256) ? 256 : backup_remaining;
        W25Q64_ReadData(backup_addr,buf,read_len);
        for(uint32_t i=0;i<read_len;i+=2)
        {
            uint16_t data = (buf[i+1]<<8)|buf[i];
            if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,internal_addr+i,data) != HAL_OK)
            {
                //写入失败，回滚失败
                HAL_FLASH_Lock();
                return 2;
            }
        }
        backup_addr += read_len;
        internal_addr += read_len;
        backup_remaining -= read_len;
     }
     // 🔥 7.3 校验恢复后的固件（栈顶 + 魔数 + CRC）
     if (Verify_Firmware_Internal(APP_START_ADDR, APP_MAX_SIZE) != VERIFY_PASS) {
        HAL_FLASH_Lock();
        return 2;  // 恢复的数据无效
    }

     HAL_FLASH_Lock();
     retry_count = 0;  // 重置重试计数器
     return 3;  // 回滚完成，但升级失败
 }
 


/**
  * @brief 核心 OTA 升级函数：从 W25Q64 搬运固件到内部 Flash（App区）
  * @return 0: 成功, 1: 失败
  */
/* uint8_t Perform_OTA_Upgrade(void)
{
    uint32_t external_addr=OTA_FW_START_ADDR;   // 外部Flash读地址
    uint32_t internal_addr=APP_START_ADDR;      // 内部Flash写地址（App起始）
    uint8_t buf[256]=0;                         // 缓存区(一页大小) 开局先清零，避免脏数据，避免写入失败还有残留数据
    uint32_t fw_size=0;                         // 固件大小
    uint32_t remaining=0;

    // 1. 读取固件大小（存储在外部Flash的前4个字节）
    W25Q64_ReadData(OTA_FW_SIZE_ADDR,(uint8_t *)&fw_size,4);
    if(fw_size == 0||(fw_size>APP_MAX_SIZE))
    {
        // 固件大小异常
        return 1;
    }

    // 2. 解锁内部 Flash
    HAL_FLASH_Unlock();

    // 3. 重置页地址跟踪变量
    current_page_addr = APP_START_ADDR;

    // 4. 开始搬运
    remaining=fw_size;
    external_addr = OTA_FW_START_ADDR; // 跳过固件大小字段

    while(remaining>0)
    {
        uint32_t write_len = (remaining>256)?256 : remaining;

        // 4.1 擦除内部 Flash 的当前页（如果跨页则擦除新页）
        IntFlash_ErasePageIfNeeded(internal_addr);

         // 4.2 从外部 Flash 读取一页数据
        W25Q64_ReadData(external_addr,buf,write_len);
         
         // 4.3 写入内部 Flash（按半字编程）
        for(uint32_t i=0;i<write_len;i+=2)
        {
            uint32_t data = (buf[i+1]<<8)|buf[i];
            if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,internal_addr+i,data) != HAL_OK)
            {
                //写入失败
                HAL_FLASH_Lock();
                return 1;           
            }
        }

         // 4.4 更新地址和剩余长度
         external_addr += write_len;
         internal_addr += write_len;
         remaining -= write_len;
    }

    // 5. 锁定内部 Flash
    HAL_FLASH_Lock();
    return 0;
} */


/**
  * @brief 标志处理 + 升级决策（在 Bootloader 启动时调用一次）
  */
 //此处有bug，对于OTA_FLAG_UPDATE_PENDING里面不应该对其进行调整操作，不然会导致标志位一直存在（但如果APP会一直检测则无事）
void Bootloader_Flag_Handler(void)
{
    OTAFlagStatus flag;
     // 1. 读取 OTA 标志
    OTA_Flag_Read(&flag);
    if(OTA_Flag_Read(&flag) != 0 )
    {
        // 读取失败，可能硬件错误，直接跳转 App（或者报错）
        Bootloader_to_APP();
        return;
    }
    // 2. 根据标志执行不同操作
    switch (flag)
    { 
        // 有新固件！执行升级            
        case OTA_FLAG_UPDATE_PENDING :
            if(Perform_OTA_Upgrade()==0)
            {
                // 升级成功，写入成功标志
                OTA_Flag_Write(OTA_FLAG_UPDATE_SUCCESS);
            }
            else
            {
                // 升级失败，写入失败标志
                OTA_Flag_Write(OTA_FLAG_UPDATE_FAILED);
            }
            // 无论成败，都跳转到 App（App 可以根据标志显示不同提示）
            Bootloader_to_APP();
            break;

        // 上次升级成功，可以清除标志并跳转
        case OTA_FLAG_UPDATE_SUCCESS :
            // 上次升级成功，可以清除标志并跳转
            OTA_Flag_Clear();  // 清掉标志，避免无限循环
            Bootloader_to_APP();
            break;

        // 上次升级失败，可以尝试重新升级或直接跳转旧 App    
        case OTA_FLAG_UPDATE_FAILED  :
            // 上次升级失败，可以尝试重新升级或直接跳转旧 App
            // 这里可以根据策略决定：重试 3 次？还是直接跳过？
            OTA_Flag_Clear();  // 暂时清除，避免卡死
            Bootloader_to_APP();
            break;
            
        // 空闲状态，直接跳转    
        default:
            Bootloader_to_APP();
            break;
        }
}


/**
  * @brief 从 Bootloader 跳转到 App
  * @note  校验 App 的栈顶和复位向量合法性，然后跳转
  */
void Bootloader_to_APP(void)
{
    typedef void (*pFunc)(void);

    // 1. 读取 App 的栈顶地址和复位向量
    uint32_t app_start_ptr = *(volatile uint32_t *)(APP_START_ADDR);
    uint32_t app_reset_handle = *(volatile uint32_t *)(APP_START_ADDR + 4);

    // 2. 校验合法性
    if ((app_start_ptr & 0xffff0000) != STACK_ADDRESS) {
        return;  // 栈顶地址无效
    }
    if (app_reset_handle < APP_START_ADDR || app_reset_handle > APP_END_ADDR) {
        return;  // 复位向量无效
    }
    
    // 3. 关闭外设和中断
    __disable_irq();
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
    HAL_DeInit();

    // 4. 设置栈指针并重定向中断向量表
    __set_MSP(app_start_ptr);
    SCB->VTOR = app_start_ptr;

    // 5. 跳转到 App 的复位中断
    pFunc jump_to_app = (pFunc)app_reset_handle;
    jump_to_app();
}

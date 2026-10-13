#ifndef __W25Q64_INS_H
#define __W25Q64_INS_H

/**
  * @brief W25Q64BV Flash 指令码定义
  * @note  参考数据手册 Section 11.2 (Instruction Set Table)
  *        所有指令均为8位（1字节），SPI通信时高位（MSB）在前
  */

/*--------------------- 写使能/禁止指令 ---------------------*/
#define W25Q64_WRITE_ENABLE        0x06    /**< 写使能：将状态寄存器的WEL位设为1，执行写/擦除前必须先发送此指令 */
#define W25Q64_WRITE_DISABLE       0x04    /**< 写禁止：将WEL位清零，操作完成后会自动清零，一般无需手动调用 */

/*--------------------- 状态寄存器操作指令 ---------------------*/
#define W25Q64_READ_STATUS_REGISTER_1   0x05    /**< 读状态寄存器1：返回8位状态（BUSY/WEL/BP等），常用于查询忙状态 */
#define W25Q64_READ_STATUS_REGISTER_2   0x35    /**< 读状态寄存器2：返回8位状态（QE/SRP1等），用于查询Quad模式使能状态 */
#define W25Q64_WRITE_STATUS_REGISTER    0x01    /**< 写状态寄存器：修改保护配置或QE位，执行前需先发送写使能指令 */

/*--------------------- 编程（写入）指令 ---------------------*/
#define W25Q64_PAGE_PROGRAM         0x02    /**< 页编程：将数据写入Flash，每页最多256字节，写入前目标区域必须已擦除为0xFF */
#define W25Q64_QUAD_PAGE_PROGRAM    0x32    /**< 四线页编程：使用4个IO口同时传输数据，需先将QE位设为1（速度快但需更多引脚） */

/*--------------------- 擦除指令 ---------------------*/
#define W25Q64_BLOCK_ERASE_64KB     0xD8    /**< 64KB块擦除：擦除一块64KB大小的区域，最小擦除单位（相比扇区擦除更快） */
#define W25Q64_BLOCK_ERASE_32KB     0x52    /**< 32KB块擦除：擦除一块32KB大小的区域 */
#define W25Q64_SECTOR_ERASE_4KB     0x20    /**< 4KB扇区擦除：最细粒度的擦除操作，擦除1个扇区（4KB），适合存储小参数 */
#define W25Q64_CHIP_ERASE           0xC7    /**< 整片擦除：擦除整个8MB芯片（耗时约30秒），非常耗时，请谨慎使用！*/

/*--------------------- 擦除暂停/恢复指令（用于打断长时间擦除） ---------------------*/
#define W25Q64_ERASE_SUSPEND        0x75    /**< 擦除暂停：在擦除过程中暂停（执行此指令后可以读其他区域，但不能写） */
#define W25Q64_ERASE_RESUME         0x7A    /**< 擦除恢复：恢复之前暂停的擦除操作 */

/*--------------------- 电源管理指令 ---------------------*/
#define W25Q64_POWER_DOWN           0xB9    /**< 掉电模式：进入低功耗状态（电流<1μA），唤醒前只能识别释放掉电指令 */
#define W25Q64_RELEASE_POWER_DOWN_HPM_DEVICE_ID   0xAB    /**< 退出掉电模式 / 高性能模式 / 读设备ID（多用途指令，详见数据手册） */

/*--------------------- 高性能模式（用于高频快速读） ---------------------*/
#define W25Q64_HIGH_PERFORMANCE_MODE    0xA3    /**< 进入高性能模式：为高频读操作做准备（提高内部电压，读取速度更快） */
#define W25Q64_CONTINUOUS_READ_MODE_RESET   0xFF    /**< 连续读模式复位：退出连续读模式，恢复标准SPI指令识别（详见数据手册） */

/*--------------------- 读ID/识别指令 ---------------------*/
#define W25Q64_MANUFACTURER_DEVICE_ID   0x90    /**< 读厂家ID和设备ID：返回Winbond厂家ID(EFh)和具体型号ID(40h=64Mbit) */
#define W25Q64_READ_UNIQUE_ID           0x4B    /**< 读64位唯一ID：每个芯片出厂时独有的序列号（需特殊定制，普通版本可能不支持） */
#define W25Q64_JEDEC_ID                 0x9F    /**< 读JEDEC标准ID：最常用的识别指令，返回3字节（厂家EFh + 类型40h + 容量17h） */

/*--------------------- 读取数据指令（按速度/引脚数分类） ---------------------*/
#define W25Q64_READ_DATA            0x03    /**< 标准读：最基础读指令，速率上限33MHz，无需额外时钟 */
#define W25Q64_FAST_READ            0x0B    /**< 快速读：速率上限80MHz，需要额外8个dummy时钟来准备数据 */
#define W25Q64_FAST_READ_DUAL_OUTPUT    0x3B    /**< 双线输出快速读：使用2个IO脚输出数据，速率=2×标准SPI */
#define W25Q64_FAST_READ_DUAL_IO       0xBB    /**< 双线IO快速读：地址和数据都用2个IO脚传输，减少指令开销，适合XIP */
#define W25Q64_FAST_READ_QUAD_OUTPUT   0x6B    /**< 四线输出快速读：使用4个IO脚输出数据，速率=4×标准SPI（需QE=1） */
#define W25Q64_FAST_READ_QUAD_IO       0xEB    /**< 四线IO快速读：地址和数据都用4个IO脚传输，最高性能（需QE=1） */
#define W25Q64_OCTAL_WORD_READ_QUAD_IO    0xE3    /**< 八字节四线快速读：类似0xEB但无需dummy时钟，读出速度最快（需地址低4位为0） */

/*--------------------- 通用常量 ---------------------*/
#define W25Q64_DUMMY_BYTE           0xFF    /**< Dummy字节：在快速读等指令中填充的空数据，发送0xFF即可 */

#endif

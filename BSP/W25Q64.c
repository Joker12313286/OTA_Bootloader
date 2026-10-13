#include "main.h"
#include "W25Q64_Ins.h"
#include "gpio.h"
#include "spi.h"

// 使用 GPIO_PIN_RESET 拉低CS（选中），GPIO_PIN_SET 拉高CS（释放）
#define W25Q64_CS_LOW()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define W25Q64_CS_HIGH()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)

//超时时间 5s
#define W25Q64_TIMEOUT_MS     5000


void W25Q64_Init(void)
{
    W25Q64_CS_HIGH();        // 初始释放CS
    //HAL_Delay(1);            // 给芯片一点稳定时间（可选）
}

void W25Q64_ReadID(uint8_t *MID, uint16_t *DID)
{
    uint8_t cmd = W25Q64_JEDEC_ID;
    uint8_t response[3] = {0};

    W25Q64_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 1000);
    HAL_SPI_Receive(&hspi1, response, 3, 1000);
    W25Q64_CS_HIGH();

    *MID  = response[0];
    *DID  = (response[1] << 8) | response[2];
}

void W25Q64_WriteEnable(void)
{
    uint8_t cmd = W25Q64_WRITE_ENABLE;
    W25Q64_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 1000);
    W25Q64_CS_HIGH();
}
uint8_t W25Q64_WaitBusy(void)   
{
    uint8_t cmd = W25Q64_READ_STATUS_REGISTER_1;
    uint8_t status;
	uint16_t timeout=HAL_GetTick();
    do {
        W25Q64_CS_LOW();
        HAL_SPI_Transmit(&hspi1, &cmd, 1, 1000);
        HAL_SPI_Receive(&hspi1, &status, 1, 1000);
        W25Q64_CS_HIGH();
		if (HAL_GetTick()-timeout>W25Q64_TIMEOUT_MS)
		{
			//超时
			return 1;
		}
		
    } while ((status & 0x01) == 0x01);   // BUSY位为1表示忙
	return 0;
}

void W25Q64_PageProgram(uint32_t Address, uint8_t *DataArray, uint16_t Count)
{
    uint8_t cmd[4] = {
        W25Q64_PAGE_PROGRAM,
        (uint8_t)((Address >> 16) & 0xFF),
        (uint8_t)((Address >> 8) & 0xFF),
        (uint8_t)(Address & 0xFF)
    };
    W25Q64_WriteEnable();
    W25Q64_CS_LOW();
    HAL_SPI_Transmit(&hspi1, cmd, 4, 1000);
    HAL_SPI_Transmit(&hspi1, DataArray, Count, 1000);
    W25Q64_CS_HIGH();
    W25Q64_WaitBusy();
}

void W25Q64_SectorErase(uint32_t Address)
{
    uint8_t cmd[4] = {
        W25Q64_SECTOR_ERASE_4KB,
        (uint8_t)((Address >> 16) & 0xFF),
        (uint8_t)((Address >> 8) & 0xFF),
        (uint8_t)(Address & 0xFF)
    };
    W25Q64_WriteEnable();
    W25Q64_CS_LOW();
    HAL_SPI_Transmit(&hspi1, cmd, 4, 1000);
    W25Q64_CS_HIGH();
    W25Q64_WaitBusy();
}

void W25Q64_ReadData(uint32_t Address, uint8_t *DataArray, uint32_t Count)
{
    uint8_t cmd[4] = {
        W25Q64_READ_DATA,
        (uint8_t)((Address >> 16) & 0xFF),
        (uint8_t)((Address >> 8) & 0xFF),
        (uint8_t)(Address & 0xFF)
    };
    W25Q64_CS_LOW();
    HAL_SPI_Transmit(&hspi1, cmd, 4, 1000);
    HAL_SPI_Receive(&hspi1, DataArray, Count, 1000);
    W25Q64_CS_HIGH();
}

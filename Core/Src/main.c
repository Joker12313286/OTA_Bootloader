/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bootloader.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define RXBUFFERSIZE  512

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t RxBuffer[RXBUFFERSIZE]={0};
volatile uint16_t RxLen = 0;
volatile uint8_t Flash_Flag=0;
volatile uint32_t Flash_write_offset = 0;
volatile uint32_t RxSumLen = 0;
//末尾可能出现的当个字节
//uint8_t last_byte = 0;
//uint8_t last_byte_flag = 0;



uint8_t MID=0;
uint16_t DID=0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef * huart, uint16_t Size)
{
  if(huart->Instance == USART1)
  {
    RxLen = Size;
    RxSumLen += Size;
    Flash_Flag = 1;
    //HAL_UART_Transmit_DMA(&huart1, RxBuffer, Size);
    //HAL_UARTEx_ReceiveToIdle_DMA(&huart1, RxBuffer, sizeof(RxBuffer));
  }
}

// 按半字（2字节）写入，地址必须为偶数
static void Int_flash_write_halfword(uint32_t write_addr, uint16_t len)
{
    uint16_t data16 = 0;
    // 确保 len 是偶数
    len = len - (len % 2);
    for(uint16_t i = 0; i < len; i += 2) {
        data16 = (RxBuffer[i+1] << 8) | RxBuffer[i];
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, write_addr + i, data16) != HAL_OK) {
            Error_Handler();
        }
    }
}

// 页擦除函数（只在跨页时擦除）
// 页擦除函数
static void Int_flash_erase_page_if_needed(uint32_t target_addr)
{
    uint32_t page_addr = target_addr - (target_addr % FLASH_PAGE_SIZE);
    if(page_addr != current_page_addr) {
        FLASH_EraseInitTypeDef EraseInitStruct;
        uint32_t PageError = 0;
        EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
        EraseInitStruct.Banks = FLASH_BANK_1;
        EraseInitStruct.PageAddress = page_addr;
        EraseInitStruct.NbPages = 1;
        if(HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK) {
            Error_Handler();
        }
        current_page_addr = page_addr;
    }
}

void Int_bootloader_jump_to_APP(void)
{
  typedef void (*pFunc)(void);
  //1.校验
  //栈顶地址的值
  uint32_t app_start_ptr=*(volatile uint32_t *)(APP_START_ADDR);
  uint32_t app_reset_handle=*(volatile uint32_t *)(APP_START_ADDR+4);
  //1.1校验栈顶地址
  if((app_start_ptr & 0xffff0000) != STACK_ADDRESS)
  {
    return;
  }
  //1.2校验复位中断地址
  if (app_reset_handle<APP_START_ADDR||app_reset_handle > APP_END_ADDR)
  {
    return;
  }

  //2.注销bootloader程序
  //2.1关闭中断
  NVIC_DisableIRQ(EXTI9_5_IRQn);
  NVIC_DisableIRQ(USART1_IRQn);
  //关闭systick
  SysTick->CTRL=0;
  SysTick->LOAD=0;
  SysTick->VAL=0;
  //注销hal设置
  HAL_DeInit();
    __disable_irq();
  //2.2设置堆栈指针
  __set_MSP(app_start_ptr);
  //2.3重定向中断向量表
  SCB->VTOR=app_start_ptr;
  //2.4跳转到A程序的复位中断
  pFunc jump_to_app = (pFunc)app_reset_handle;
  jump_to_app();
} */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  W25Q64_Init();
  //HAL_UARTEx_ReceiveToIdle_DMA(&huart1, RxBuffer, sizeof(RxBuffer));

  //启动bootloader程序，内部进行判断是否升级和跳转
  Bootloader_Flag_Handler();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

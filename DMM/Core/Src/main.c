/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "eth.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"
#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include <stdio.h>
#include "bsp_AD7190.h"
#include "bsp_debug_usart.h"
#include "lcd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define ZERO_MODE  0
#define AD7190_FILTER_DEPTH 8
// 放大倍数=R2/R1=1500/40200
#define OPA_RES_R1              40200  // 40.2k 运放输入端电阻
#define OPA_RES_R2              1500  // 1.5k 运放反馈电阻
#define REFERENCE_VOLTAGE       3297  // 参考电压（放大1000倍）
#define BIAS_VOLTAGE_IN1        0x39CF8C  // 输入1偏置电压，即把IN1和GND短接时AD7190转换结果
#define BIAS_VOLTAGE_IN2        0x39CF52  // 输入2偏置电压，即把IN2和GND短接时AD7190转换结果
#define BIAS_VOLTAGE_IN3        0x39CF4D  // 输入3偏置电压，即把IN3和GND短接时AD7190转换结果
#define BIAS_VOLTAGE_IN4        0x39D027  // 输入4偏置电压，即把IN4和GND短接时AD7190转换结果
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
__IO int32_t ad7190_data[4]; // AD7190原始转换结果
__IO int32_t bias_data[4];   // 零点电压的AD转换结果
__IO double voltage_data[4]; // 电压值（单位：mV）
__IO uint8_t flag=0;         // 启动采集标志
__IO int8_t number = 0;          // 当前处理的通道
__IO int32_t filtered_data[4]; // 滤波后的转换结果

static int32_t filter_buf[4][AD7190_FILTER_DEPTH];
static uint8_t filter_index[4];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
int32_t AD7190_Filter(int channel, int32_t sample);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int32_t AD7190_Filter(int channel, int32_t sample)
{
    if(channel < 0 || channel >= 4)
    {
        return sample;
    }

    filter_buf[channel][filter_index[channel]] = sample;
    filter_index[channel] = (filter_index[channel] + 1) % AD7190_FILTER_DEPTH;

    int64_t sum = 0;
    for(int i = 0; i < AD7190_FILTER_DEPTH; ++i)
    {
        sum += filter_buf[channel][i];
    }

    return (int32_t)(sum / AD7190_FILTER_DEPTH);
}
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

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_FSMC_Init();
  MX_ETH_Init();
  //MX_UART4_Init();
  MX_DEBUG_USART_Init();
  //MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  if(AD7190_Init()==0)
  {
   // printf("获取不到 AD7190 !\n");
   lcd_show_str(10, 0*20 + 10, 16, "AD7190 Init fail", RED);
    while(1)
    {
      HAL_Delay(1000);
      if(AD7190_Init())
        break;
    }
  }
  ad7190_unipolar_multichannel_conf();
  bias_data[0]=BIAS_VOLTAGE_IN1;
  bias_data[1]=BIAS_VOLTAGE_IN2;
  bias_data[2]=BIAS_VOLTAGE_IN3;
  bias_data[3]=BIAS_VOLTAGE_IN4;
  flag=1;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(flag == 2)
	  {
#if ZERO_MODE==1
		  // printf("IN%d. 0x%05X\n",number,bias_data[number]);
#else
		if(number >= 0 && number < 4)
		{
			int32_t raw = ad7190_data[number]>>4;
			filtered_data[number] = AD7190_Filter(number, raw);
			voltage_data[number]=filtered_data[number];
			voltage_data[number]=voltage_data[number]*REFERENCE_VOLTAGE/OPA_RES_R2*OPA_RES_R1/0xFFFFF;

			char disp[20];
			sprintf(disp, "CH%d:%6.3fV", number+1, voltage_data[number]/1000.0);
			lcd_show_str(10, (number + 1) * 20 + 10, 16, disp, RED);
		}
#endif
		flag = 1;
	  }
	  HAL_Delay(10);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}


void HAL_SYSTICK_Callback(void)
{
  uint8_t sample[4];
  int8_t temp;

  if((flag)&&(AD7190_RDY_STATE()==0))
  {
    HAL_SPI_Receive(&hspi_AD7190,sample,4,0xFF);
    if((sample[3]&0x80)==0)
    {
      temp=(sample[3]&0x07)-4;
      if(temp>=0)
      {
#if ZERO_MODE==1
        bias_data[temp]=((sample[0]<<16)|(sample[1]<<8)|sample[2]);
#else
        ad7190_data[temp]=((sample[0]<<16) | (sample[1]<<8) | sample[2])-bias_data[temp];
#endif
        number=temp;
        flag=2;
      }
      else
      {
        //char buf[32];
        //sprintf(buf, "error:0x%X", sample[3]);
        //lcd_show_str(10, 60, 16, buf, RED);

        flag++;
        if(flag>=10)
        {
          flag=0;
          printf("失败！！！\n");
        }
      }
    }
  }
}
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO2, RCC_MCO2SOURCE_PLLI2SCLK, RCC_MCODIV_2);

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);                // 配置并启动系统滴答定时器
  /* 系统滴答定时器时钟源 */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* 系统滴答定时器中断优先级配置 */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
  /** Enables the Clock Security System
  */
  HAL_RCC_EnableCSS();
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_PLLI2S;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 200;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
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
  *
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

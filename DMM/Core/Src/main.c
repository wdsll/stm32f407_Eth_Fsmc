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

#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include <stdio.h>
#include "bsp_AD7190.h"
#include "bsp_debug_usart.h"
#include "lcd.h"
#include "adc_mux.h"
#include <math.h>
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

// 放大倍数=R2/R1=2000/6800倍
#define OPA_RES_R1              6800  // 6.8k 运放输入端电阻
#define OPA_RES_R2              2000  // 2k 运放反馈电阻
#define REFERENCE_VOLTAGE       3297  // 参考电压（放大1000倍）
#define BIAS_VOLTAGE_IN1        0xFAB3E  // 输入1偏置电压，即把IN1和GND短接时AD7190转换结果
#define BIAS_VOLTAGE_IN2        0xF9DCA  // 输入2偏置电压，即把IN2和GND短接时AD7190转换结果
#define BIAS_VOLTAGE_IN3        0xFA8A4  // 输入3偏置电压，即把IN3和GND短接时AD7190转换结果
#define BIAS_VOLTAGE_IN4        0xFA9EB  // 输入4偏置电压，即把IN4和GND短接时AD7190转换结果

#define ADC20_FULL_SCALE 0xFFFFF

/* 私有变量 ------------------------------------------------------------------*/
__IO int32_t ad7190_data[4]; // AD7190原始转换结果
__IO int32_t bias_data[4];   // 零点电压的AD转换结果
__IO double voltage_data[4]; // 电压值（单位：mV）
__IO uint8_t flag=0;         // 启动采集标志
__IO int8_t number;          // 当前处理的通道
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

__IO int32_t filtered_data[4]; // 滤波后的转换结果

static int32_t filter_buf[4][AD7190_FILTER_DEPTH];
static uint8_t filter_index[4];
static uint32_t s_last=0;
static float s_last_v=0.0f;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
int32_t AD7190_Filter(int channel, int32_t sample);

void  DMM_SetCurrentChannel(uint8_t ad_ch);   // 0..3，映射到 ad7190_data[ad_ch]
float DMM_ReadCurrent_A(uint8_t use_filter);  // 物理电流（A）
float DMM_ReadCurrent_FSratio(uint8_t use_filter); // ADC 满量程占比 0..1

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//电压值读取
__attribute__((weak)) float VRange_Scale(void) { return 1.0f; }
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

float MEAS_ReadVoltage_V(uint8_t ch, uint8_t use_filter)
{
    fun_select_v();
    //int32_t code24 = ad7190_data[ch];
    int32_t code20 = (ad7190_data[ch] >> 4);
    if (use_filter) {
        code20 = AD7190_Filter((int)ch, code20);
    }
    /* 3) 将 ADC 码 -> 前端输入电压（V）
            - REFERENCE_VOLTAGE/ADC20_FULL_SCALE: 把码值变成“ADC 端的电压标度”
            - (R1/R2): 还原运放前的真实电压（你原公式）
            - VRange_Scale(): 当前量程的“还原系数”（分压/衰减的倒数）
       */
    double mv = (double)code20
              * (double)REFERENCE_VOLTAGE
              * (double)OPA_RES_R1 / (double)OPA_RES_R2
              / (double)ADC20_FULL_SCALE
              * (double)VRange_Scale();
   // s_last_v=v;
    s_last_v= (float)(mv / 1000.0);
    s_last=code20;

    return s_last_v;
}
// 把 20位码值换算成输入端电压（V）
static inline float code20_to_vinV(int32_t code20)
{
    double mv = (double)code20
              * (double)REFERENCE_VOLTAGE
              * (double)OPA_RES_R1 / (double)OPA_RES_R2
              / (double)ADC20_FULL_SCALE
              * (double)VRange_Scale();
    return (float)(mv / 1000.0);
}
// 读取“最近一次更新”的那个通道的电压（V）; use_filter=1 开滤波
float MEAS_ReadVoltage_V_Last(uint8_t use_filter)
{
    int ch = (int)number;                 // 0..3
    if (ch < 0 || ch > 3) ch = 0;

    int32_t code24 = ad7190_data[ch];     // 已减零偏
    int32_t code20 = (code24 >> 4);       // 按你的习惯压到 20bit

    if (use_filter) {
        code20 = AD7190_Filter(ch, code20);
    }

    float v = code20_to_vinV(code20);
    //s_last_v = v; s_last_raw = (uint32_t)code24;
    return v;
}

// 如需读取“指定通道”的版本（0..3）
float MEAS_ReadVoltage_V_Single(uint8_t ch, uint8_t use_filter)
{
    if (ch > 3) return 0.0f;
    int32_t code24 = ad7190_data[ch];
    int32_t code20 = (code24 >> 4);
    if (use_filter) code20 = AD7190_Filter((int)ch, code20);
    float v = code20_to_vinV(code20);
    //s_last_v = v; s_last_raw = (uint32_t)code24;
    return v;
}

//电流值读取
static uint8_t s_i_ad_ch = 0;

/* —— 当前量程的增益（例如分流/ADG 档位），缺省 1.0：你可在量程切换时更新 —— */
//IPath_VperA()：把“1A 通过被测端 → ADC 端会产生多少伏”算出来（分流电阻×放大器增益×任何比例），
//例如 1A → 50 mV（放大后）→ 0.05f

__attribute__((weak)) float IRange_Gain(void){ return 1.0f; }
//IRange_Gain()：量程切换后的倍数（例如 1×、10×、100×…），在 i_range_apply() 里同步更新一个全局变量，
//IRange_Gain() 返回它即可
/* —— 电流链路“V/A”常数（ADC 端电压 / 通过被测电流），缺省 1.0：按你的硬件填 —— */
__attribute__((weak)) float IPath_VperA(void){ return 1.0f; }

void DMM_SetCurrentChannel(uint8_t ad_ch)
{
	s_i_ad_ch = (ad_ch<=3)? ad_ch : 0;
}
static inline float code20_to_v_adc(int32_t code20){
    double mv = (double)code20 * (double)REFERENCE_VOLTAGE / (double)ADC20_FULL_SCALE;
    return (float)(mv / 1000.0);
}
/* 满量程占比（0..1），用于自动量程 */
float DMM_ReadCurrent_FSratio(uint8_t use_filter)
{
    int32_t code24 = ad7190_data[s_i_ad_ch];
    int32_t code20 = (code24 >> 4);
    if (use_filter) code20 = AD7190_Filter((int)s_i_ad_ch, code20);

    /* 单极性时直接归一化；若双极性，可用 fabsf */
    float fs = fabsf((float)code20) / (float)ADC20_FULL_SCALE;
    if (fs > 1.0f)
    		fs = 1.0f;
    return fs;
}

float DMM_ReadCurrent_A(uint8_t use_filter)
{
    int32_t code24 = ad7190_data[s_i_ad_ch];
    int32_t code20 = (code24 >> 4);
    if (use_filter) code20 = AD7190_Filter((int)s_i_ad_ch, code20);

    float v_adc = code20_to_v_adc(code20);        // ADC 端电压（V）
    float I = v_adc / (IPath_VperA() * IRange_Gain());
    return I;
}

void MEAS_I_Autorange_1kHz(void)
{
    // 若 fun_select_i() 在你板子上无意义，可以删掉
     fun_select_i();

    float fs_ratio = DMM_ReadCurrent_FSratio(0); // 0=不滤波，响应更快
    if (fs_ratio > CAL_I_RANGE_UP_TH && s_i_range < 7) {
        i_range_apply(s_i_range + 1);
    } else if (fs_ratio < CAL_I_RANGE_DN_TH && s_i_range > 0) {
        i_range_apply(s_i_range - 1);
    }
}
//电阻测量
__attribute__((weak)) float RV_Chain_Scale(void) {
    /* 默认：沿用电压读数那套还原（OPA_RES_R1/OPA_RES_R2 * VRange_Scale） */
    extern float VRange_Scale(void);
    /* 若你已把 OPA 等效写进电压公式，这里可以直接 return VRange_Scale(); */
    return VRange_Scale();
}

/* RI_Path_VperA: 电流链路的“V/A”比例（ADC 端获得多少伏对应 1A）
 *   例如：分流 0.1Ω、放大 50×，则 V/A = 0.1 * 50 = 5 V/A
 */
__attribute__((weak)) float RI_Path_VperA(void) { return 1.0f; }

/* RI_Range_Gain: 电流量程的增益（随 ADG 档位变化），默认 1.0；
 *   你在 i_range_apply() 里更新一个全局量，然后让这个函数返回那个量即可。
 */
__attribute__((weak)) float RI_Range_Gain(void) { return 1.0f; }

/* ---- R 模式使用的 AD 通道号（默认 V=0, I=1；按你板子设置） ---- */
static uint8_t s_r_v_adch = 0;
static uint8_t s_r_i_adch = 1;

void DMM_R_SetChannels(uint8_t v_ad_ch, uint8_t i_ad_ch)
{
    s_r_v_adch = (v_ad_ch <= 3) ? v_ad_ch : 0;
    s_r_i_adch = (i_ad_ch <= 3) ? i_ad_ch : 1;
}

static inline float code20_to_vadc(int32_t code20)
{
    double mv = (double)code20 * (double)REFERENCE_VOLTAGE / (double)ADC20_FULL_SCALE;
    return (float)(mv / 1000.0);
}

float DMM_R_Read(uint8_t use_filter, float* r_out, float* i_out, float* v_out)
{
	fun_select_v();
	int32_t code20_v = (ad7190_data[s_r_v_adch] >> 4);
	/* 1) Vdut —— 取电压通道码值，必要时滤波，再换算为被测两端电压 */
	if (use_filter)
		code20_v = AD7190_Filter((int)s_r_v_adch, code20_v);

	    float Vadc = code20_to_vadc(code20_v);
	    float Vdut = Vadc * RV_Chain_Scale();

	/* 2) Idut —— 取电流通道码值，必要时滤波，再换算为电流 */
	int32_t code20_i = (ad7190_data[s_r_i_adch] >> 4);
	if (use_filter)
		code20_i = AD7190_Filter((int)s_r_i_adch, code20_i);
    float Vadc_I = code20_to_vadc(code20_i);
    float Idut   = Vadc_I / (RI_Path_VperA() * RI_Range_Gain());
    float Rdut = (fabsf(Idut) > 1e-12f) ? (Vdut / Idut) : 0.0f;

    if (r_out)
    	*r_out = Rdut;
    if (i_out)
    	*i_out = Idut;
    if (v_out)
    	*v_out = Vdut;
    return Rdut;
}

void MEAS_ReadResistance_Ohm(float* r,float* i,float* v)
{
    (void)DMM_R_Read(1, r, i, v);
}

void MEAS_R_Autorange_1kHz(void)
{

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
  //MX_ETH_Init();
  //MX_UART4_Init();
  MX_DEBUG_USART_Init();
  lcd_init();
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
		  //voltage = MovingAverageFilter(voltage); // 调用滤波函数
		  //printf("IN%d. 0x%05X\n",number,bias_data[number]);
		  int32_t filtered_bias = AD7190_Filter(number, bias_data[number]);
		     char disp[20];
		     sprintf(disp, "IN%d. 0x%05X", number, filtered_bias);
		     lcd_show_str(10, (number + 1) * 20 + 10, 16, disp, RED);
#else
		if(number >= 0 && number < 4)
		{
			//int32_t raw = ad7190_data[number]>>4;
		    //filtered_data[number] = AD7190_Filter(number, raw);
			//voltage_data[number]=filtered_data[number];
			//voltage_data[number]=voltage_data[number]*REFERENCE_VOLTAGE/OPA_RES_R2*OPA_RES_R1/0xFFFFF;

		   voltage_data[number]=ad7190_data[number]>>4;
		   voltage_data[number]=voltage_data[number]*REFERENCE_VOLTAGE/OPA_RES_R2*OPA_RES_R1/0xFFFFF;
		   //printf("IN%d. 0x%05X->%0.3fV\n",number,ad7190_data[number],voltage_data[number]/1000);+

			char disp[40];
			sprintf(disp, "CH%d:%6.6fV", number+1, voltage_data[number]/1000.0);
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
  RCC_OscInitStruct.PLL.PLLQ = 7;
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

  /** Enables the Clock Security System
  */
  HAL_RCC_EnableCSS();
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/100);
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
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

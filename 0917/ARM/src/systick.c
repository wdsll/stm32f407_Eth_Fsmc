/**
  ******************************************************************************
  * @file    bsp_SysTicks.c
  * @author  �Ž�
  * @version V1.0.0
  * @date    2022-8-19
  * @brief   ϵͳ�δ�ʱ����ʱ����
  ******************************************************************************
  */
#include "systick.h"


volatile static uint8_t  fac_us = 0;
volatile static uint16_t fac_ms = 0;

/*******************************************************************************
* Function Name  : systick_config
* Description    : ϵͳ�δ�ʱ��ʱ������
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void systick_delay_config(void)
{
	//�ڲ�ʱ��8��Ƶ
	systick_clksource_set(SYSTICK_CLKSOURCE_HCLK_DIV8);
	fac_us = SystemCoreClock/8000000;          //��ʱ1us��Ҫ��ʱ������ 
  fac_ms = fac_us * 1000;                    //��ʱ1ms��Ҫ��ʱ������ 
	
}

/*******************************************************************************
* Function Name  : delay_us
* Description    : us����ʱ����
* Input          : nus ������ʱ����us
* Output         : None
* Return         : None
*******************************************************************************/
void delay_us(uint32_t nus)
{
    uint32_t temp;
    
    /* reload the count value */
    SysTick->LOAD = (uint32_t)(nus * fac_us);         		
    /* clear the current count value */ 
    SysTick->VAL = 0x0000U;
    /* enable the systick timer */
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
    /* wait for the COUNTFLAG flag set */
    do{
        temp = SysTick->CTRL;
    }while((temp&SysTick_CTRL_ENABLE_Msk)&&!(temp & SysTick_CTRL_COUNTFLAG_Msk));  //�ȴ�ʱ�䵽��   
    /* disable the systick timer */
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    /* clear the current count value */
    SysTick->VAL = 0x0000U;
}




/*******************************************************************************
* Function Name  : delay_ms
* Description    : ms����ʱ����
* Input          : nms ������ʱ����ms
* Output         : None
* Return         : None
*******************************************************************************/
void delay_ms(uint32_t nms)
{
    uint32_t temp;
    
    /* reload the count value */
    SysTick->LOAD = (uint32_t)(nms * fac_ms);
    /* clear the current count value */
    SysTick->VAL = 0x0000U;
    /* enable the systick timer */
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
    /* wait for the COUNTFLAG flag set */
    do{
        temp = SysTick->CTRL;
    }while((temp&SysTick_CTRL_ENABLE_Msk)&&!(temp & SysTick_CTRL_COUNTFLAG_Msk));  //�ȴ�ʱ�䵽��
    /* disable the systick timer */
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    /* clear the current count value */
    SysTick->VAL = 0x0000U;
}








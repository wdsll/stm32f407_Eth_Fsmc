/*******************************************************************************
 * LlcAdc.h
 *
 *  Created on: 2019-11-15
 *      Author: Gavin.D
*******************************************************************************/
#ifndef LLCADC_H_
#define LLCADC_H_

#include "DSP28x_Project.h"    // Device Headerfile and Examples Include File

/*******************************************************************************
**                               ������
*******************************************************************************/
// =============================================================================
// ����˿�
// =============================================================================
#define ADCINA0     0x0
#define ADCINA1     0x1
#define ADCINA2     0x2
#define ADCINA3     0x3
#define ADCINA4     0x4
#define ADCINA5     0x5
#define ADCINA6     0x6
#define ADCINA7     0x7
#define ADCINB0     0x8
#define ADCINB1     0x9
#define ADCINB2     0xA
#define ADCINB3     0xB
#define ADCINB4     0xC
#define ADCINB5     0xD
#define ADCINB6     0xE
#define ADCINB7     0xF

// =============================================================================
// ���ȼ�
// =============================================================================
#define PRIORITY_NONE    0x00
#define PRIORITY_SOC0_0  0x01
#define PRIORITY_SOC0_1  0x02
#define PRIORITY_SOC0_2  0x03
#define PRIORITY_SOC0_3  0x04
#define PRIORITY_SOC0_4  0x05
#define PRIORITY_SOC0_5  0x06
#define PRIORITY_SOC0_6  0x07

// =============================================================================
// EOC
// =============================================================================
#define ADC_EOC15   0xF

// =============================================================================
// ����Դ
// =============================================================================
#define ADCTRIG_SOFT        0
#define ADCTRIG_CPU_TINT0   1
#define ADCTRIG_CPU_TINT1   2
#define ADCTRIG_CPU_TINT2   3
#define ADCTRIG_XINT2       4
#define ADCTRIG_EPWM1_SOCA  5
#define ADCTRIG_EPWM1_SOCB  6
#define ADCTRIG_EPWM2_SOCA  7
#define ADCTRIG_EPWM2_SOCB  8

// =============================================================================
// ����ͨ��
// =============================================================================
#define CH_IFAST    ADCINA1   // ���ٵ���
#define CH_ISLOW    ADCINA1   // ���ٵ���
#define CH_DC380    ADCINA3   // DC380V��ѹ
#define CH_USBON    ADCINA6   // 13V��ѹ��û�õ�

#define CH_ISHORT   ADCINA2   // ���ȵ���
#define CH_VFAST    ADCINB2   // ���ٵ�ѹ (ֱ�������ѹ)
#define CH_VSLOW    ADCINB3   // ���ٵ�ѹ (��ص�ѹ)
#define CH_TEMP     ADCINB4   // �����¶�
#define CH_TEMPO    ADCINB7   // �ⲿ�¶�
#define CH_DSP_TEMP ADCINA5   // DSPоƬ�¶�,��ֵΪ�̶�ֵ,����Ӳ����·����

// =============================================================================
// �������,��Ҫ��SOCx����Ĳ���ͨ�����Ӧ,���ܵ����޸�
// =============================================================================
#define ADRESULT_IFAST    AdcResult.ADCRESULT0
#define ADRESULT_VFAST    AdcResult.ADCRESULT4
#define ADRESULT_ACIN     AdcResult.ADCRESULT5
#define ADRESULT_ISLOW    AdcResult.ADCRESULT6
#define ADRESULT_VSLOW    AdcResult.ADCRESULT7
#define ADRESULT_ISHORT   AdcResult.ADCRESULT8
                         // AdcResult.ADCRESULT9
#define ADRESULT_USBON    AdcResult.ADCRESULT10
#define ADRESULT_TEMP     AdcResult.ADCRESULT11
#define ADRESULT_DSP_TEMP AdcResult.ADCRESULT14
#define ADRESULT_OTEMP    AdcResult.ADCRESULT15

// =============================================================================
// �������(�����ĳ�֮Ϊ����ֵ)���ٷ���
// =============================================================================
#define VOUT_DATA       SampleData.DataVout     // �����ѹ����ֵ
#define IOUT_DATA       SampleData.DataIout     // �����������ֵ
#define BAT_DATA        SampleData.DataVbat     // ��ص�ѹ����ֵ
#define TEMP_DATA       SampleData.DataTemp     // �����¶ȷ���ֵ
// =============================================================================
// AD����ϵ��
// =============================================================================
#define K_EXP     15                // ָ�����2^15
#define K_VOUT_A  3215///3185
#define K_VOUT_B  (-131612)

#define K_VBAT_A  3215///3185
#define K_VBAT_B  (-131612)

#define K_IOUT_A  39650//3068139650  39650
#define K_IOUT_B  -612434//(-813400)

#define LEN_NORMAL_ARRAY  32 // ��������: normal, �����˲������˲�����,һ�㳤��
#define LEN_SHORT_ARRAY   16 // ��������: Short, �����˲������˲�����,�̳���
/*******************************************************************************
**                               ��ؽṹ��
*******************************************************************************/
// ---------------------------------------------------------
// 32��ͨ�����˲��ṹ��
// ---------------------------------------------------------
struct stARRAY {
    int16 Array[LEN_NORMAL_ARRAY];  // ������������
    int32 SumData;                  // ������
    Uint16 Flag;                    // �µ����ݱ����������λ��
};
#define INIT_ARRAY_DATA {{\
    0,0,0,0,0,0,0,0,\
    0,0,0,0,0,0,0,0,\
    0,0,0,0,0,0,0,0,\
    0,0,0,0,0,0,0,0,},\
    0,0,}
// ---------------------------------------------------------
// 16��ͨ�����˲��ṹ��
// ---------------------------------------------------------
struct stSHORTARRAY {
    int16 Array[LEN_SHORT_ARRAY];  // ������������
    int32 SumData;                  // ������
    Uint16 Flag;                    // �µ����ݱ����������λ��
};
#define INIT_SHORT_ARRAY_DATA {{\
    0,0,0,0,0,0,0,0,\
    0,0,0,0,0,0,0,0,},\
    0,0,}
// ---------------------------------------------------------
// �������ݽ���ṹ��
// ---------------------------------------------------------
struct stSAMPLEDATA {
    int16 DataIout;  // ���ٵ���
    int16 DataVbat;  // ��ص�ѹ
    int16 DataVout;  // �����ѹ
    int16 DataTemp;  // �¶�
    int16 DataOutTemp;////

};
// ---------------------------------------------------------
// ����״̬�ṹ�壬��ʾĳ�������Ƿ����:0δ���,1�����
// ---------------------------------------------------------
struct stSAMPLESTAT {
    Uint16 Iout    :  1;
    Uint16 Vbat    :  1;
    Uint16 Vout    :  1;
};
union unSAMPLESTAT {
    Uint16 all;
    struct stSAMPLESTAT bit;
};

/*******************************************************************************
**                              ��������
*******************************************************************************/
extern struct stSAMPLEDATA   SampleData;        // ADC�����������λ��
extern union  unSAMPLESTAT   SampleState;       // ����״̬��ʾ�Ĵ���
/*******************************************************************************
**                               ��������
*******************************************************************************/

/*******************************************************************************
**                               ��������
*******************************************************************************/
extern void AdcInit(void);
extern void InitAdcPara(void);
extern void ADCManage(void);

#endif /* LLCADC_H_ */

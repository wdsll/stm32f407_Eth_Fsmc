/*******************************************************************************
 * LlcAdc.c
 *
 *  Created on: 2019-11-15
 *      Author: Gavin.D
*******************************************************************************/
#include "LlcAdc.h"

/*******************************************************************************
**                               �궨��
*******************************************************************************/

/*******************************************************************************
**                               ��ؽṹ��
*******************************************************************************/
struct stSAMPLEDATA   SampleData;
union  unSAMPLESTAT   SampleState;
static struct stARRAY DataArrayVout       = INIT_ARRAY_DATA;
static struct stARRAY DataArrayVbat       = INIT_ARRAY_DATA;
static struct stARRAY DataArrayIout       = INIT_ARRAY_DATA;
static struct stSHORTARRAY DataArrayTemp  = INIT_SHORT_ARRAY_DATA;
/*******************************************************************************
**                               ��������
*******************************************************************************/

/*******************************************************************************
**                               ��������
*******************************************************************************/
void AdcInit(void);
void InitAdcPara(void);
void ADCManage(void);
static Uint16 ADCFinish(void);

/*******************************************************************************
** ��������:    AdcInit
** ��������:    ��ʼ��ADC���輰���ж�
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
void AdcInit(void)
{
    InitAdc();
    EALLOW;
// -----------------------------------------------------------------------------
// ���ò���ģʽΪ˳�����ģʽ,���Ϊ��һ����ģʽ
// -----------------------------------------------------------------------------
    AdcRegs.ADCSAMPLEMODE.all       = 0;
// -----------------------------------------------------------------------------
// ʹ���ڲ��¶ȴ�����
// -----------------------------------------------------------------------------
    AdcRegs.ADCCTL1.bit.TEMPCONV    = 1;
    AdcRegs.ADCCTL1.bit.ADCREFSEL   = 1;
// -----------------------------------------------------------------------------
// ����SOCx��Ӧ����ͨ��,[����]��궨��RESULT_XXX���Ӧ
// -----------------------------------------------------------------------------
    AdcRegs.ADCSOC0CTL.bit.CHSEL    = CH_IFAST;  // ���ٵ����źŲ���,��PWM����
    AdcRegs.ADCSOC1CTL.bit.CHSEL    = CH_IFAST;
    AdcRegs.ADCSOC2CTL.bit.CHSEL    = CH_IFAST;  // ���ٵ�ѹ�źŲ���,�ɶ�ʱ��0(20us)����
    AdcRegs.ADCSOC3CTL.bit.CHSEL    = CH_IFAST;
    AdcRegs.ADCSOC4CTL.bit.CHSEL    = CH_VFAST;
    AdcRegs.ADCSOC5CTL.bit.CHSEL    = CH_VFAST;
    AdcRegs.ADCSOC6CTL.bit.CHSEL    = CH_ISLOW;  // ����Ϊ�����źŲ���
    AdcRegs.ADCSOC7CTL.bit.CHSEL    = CH_VSLOW;
    AdcRegs.ADCSOC8CTL.bit.CHSEL    = CH_ISHORT;
    AdcRegs.ADCSOC9CTL.bit.CHSEL    = CH_ISHORT;
    AdcRegs.ADCSOC10CTL.bit.CHSEL   = CH_USBON;
    AdcRegs.ADCSOC11CTL.bit.CHSEL   = CH_TEMP;
    AdcRegs.ADCSOC12CTL.bit.CHSEL   = CH_TEMP;
    AdcRegs.ADCSOC13CTL.bit.CHSEL   = CH_DSP_TEMP;
    AdcRegs.ADCSOC14CTL.bit.CHSEL   = CH_DSP_TEMP;
    AdcRegs.ADCSOC15CTL.bit.CHSEL   = CH_TEMPO;
// -----------------------------------------------------------------------------
// ����SOCx����Դ
// -----------------------------------------------------------------------------
    AdcRegs.ADCSOC0CTL.bit.TRIGSEL  = ADCTRIG_CPU_TINT0;
    AdcRegs.ADCSOC1CTL.bit.TRIGSEL  = ADCTRIG_CPU_TINT0;
    AdcRegs.ADCSOC2CTL.bit.TRIGSEL  = ADCTRIG_CPU_TINT0;
    AdcRegs.ADCSOC3CTL.bit.TRIGSEL  = ADCTRIG_CPU_TINT0;
    AdcRegs.ADCSOC4CTL.bit.TRIGSEL  = ADCTRIG_CPU_TINT0;
    AdcRegs.ADCSOC5CTL.bit.TRIGSEL  = ADCTRIG_CPU_TINT0;
    AdcRegs.ADCSOC6CTL.bit.TRIGSEL  = ADCTRIG_CPU_TINT0;
    AdcRegs.ADCSOC7CTL.bit.TRIGSEL  = ADCTRIG_CPU_TINT0;
    AdcRegs.ADCSOC8CTL.bit.TRIGSEL  = ADCTRIG_CPU_TINT0;
    AdcRegs.ADCSOC9CTL.bit.TRIGSEL  = ADCTRIG_CPU_TINT0;
    AdcRegs.ADCSOC10CTL.bit.TRIGSEL = ADCTRIG_CPU_TINT0;
    AdcRegs.ADCSOC11CTL.bit.TRIGSEL = ADCTRIG_CPU_TINT1;
    AdcRegs.ADCSOC12CTL.bit.TRIGSEL = ADCTRIG_CPU_TINT1;
    AdcRegs.ADCSOC13CTL.bit.TRIGSEL = ADCTRIG_CPU_TINT1;
    AdcRegs.ADCSOC14CTL.bit.TRIGSEL = ADCTRIG_CPU_TINT1;
    AdcRegs.ADCSOC15CTL.bit.TRIGSEL = ADCTRIG_CPU_TINT1;
// -----------------------------------------------------------------------------
// ���ò�������(���ڿ�� = ACQPS_Value + 1)
// -----------------------------------------------------------------------------
    #define ACQPS_Value 6
    AdcRegs.ADCSOC0CTL.bit.ACQPS    = ACQPS_Value;
    AdcRegs.ADCSOC1CTL.bit.ACQPS    = ACQPS_Value;
    AdcRegs.ADCSOC2CTL.bit.ACQPS    = ACQPS_Value;
    AdcRegs.ADCSOC3CTL.bit.ACQPS    = ACQPS_Value;
    AdcRegs.ADCSOC4CTL.bit.ACQPS    = ACQPS_Value;
    AdcRegs.ADCSOC5CTL.bit.ACQPS    = ACQPS_Value;
    AdcRegs.ADCSOC6CTL.bit.ACQPS    = ACQPS_Value;
    AdcRegs.ADCSOC7CTL.bit.ACQPS    = ACQPS_Value;
    AdcRegs.ADCSOC8CTL.bit.ACQPS    = ACQPS_Value;
    AdcRegs.ADCSOC9CTL.bit.ACQPS    = ACQPS_Value;
    AdcRegs.ADCSOC10CTL.bit.ACQPS   = ACQPS_Value;
    AdcRegs.ADCSOC11CTL.bit.ACQPS   = ACQPS_Value;
    AdcRegs.ADCSOC12CTL.bit.ACQPS   = ACQPS_Value;
    AdcRegs.ADCSOC13CTL.bit.ACQPS   = ACQPS_Value;
    AdcRegs.ADCSOC14CTL.bit.ACQPS   = ACQPS_Value;
    AdcRegs.ADCSOC15CTL.bit.ACQPS   = ACQPS_Value;
    #undef ACQPS_Value
// -----------------------------------------------------------------------------
// ���ò���ͨ�����ȼ�:ͨ��0 & 1 : Ϊ���ٵ�������ͨ��,�����������,��Ϊ������PWMʱЧ
// -----------------------------------------------------------------------------
    AdcRegs.SOCPRICTL.bit.SOCPRIORITY = PRIORITY_SOC0_6;
// -----------------------------------------------------------------------------
// �����жϱ�־��������: �������ʱ
// -----------------------------------------------------------------------------
    AdcRegs.ADCCTL1.bit.INTPULSEPOS = 1;
// -----------------------------------------------------------------------------
// ����ADC�жϴ���SOCx
// -----------------------------------------------------------------------------
    AdcRegs.ADCINTSOCSEL1.all  = 0;
    AdcRegs.ADCINTSOCSEL2.all  = 0;
// -----------------------------------------------------------------------------
// �����ж��źŵĴ���Դ
// -----------------------------------------------------------------------------
    AdcRegs.INTSEL1N2.bit.INT1SEL   = ADC_EOC15;
// -----------------------------------------------------------------------------
// ʹ���ж��ź�,��������ζ��ᴥ���ж�,������ʹ���ź�
// -----------------------------------------------------------------------------
    AdcRegs.INTSEL1N2.bit.INT1E     = 1;
// -----------------------------------------------------------------------------
// ������������,���´β�������ǰǰ�β�����ɱ�־��������
// -----------------------------------------------------------------------------
    AdcRegs.INTSEL1N2.bit.INT1CONT  = 0;
    EDIS;
    DELAY_US(1000L);
}

/*******************************************************************************
** ��������:    InitAdcPara
** ��������:    ��ʼ��ADC�����������
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
void InitAdcPara(void)
{
    SampleData.DataIout      = 0;
    SampleData.DataVbat      = 0;
    SampleData.DataVout      = 0;
    SampleData.DataTemp      = 0;
    SampleState.all          = 0;
}

/*******************************************************************************
** ��������:    ADCFinish
** ��������:    �������AD�����Ƿ����
** ��ʽ����:    ��
** ���ز���:    ��ɷ���1,����ɷ���0
*******************************************************************************/
static Uint16 ADCFinish(void)
{
    Uint16 result;
    if (AdcRegs.ADCINTFLG.bit.ADCINT1)
    {
        AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;    //Clear ADCINT1
        result = 1;                              //ת�����
    }
    else
    {
        result = 0;
    }
    return (result);
}


/*******************************************************************************
** ��������:    ADCManage
** ��������:    ADC������,���е�ADCǰ̨�����������ﴦ��
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
void ADCManage(void)
{
    if (ADCFinish())//ת�����
    {
        // ---------------------------------------------------------
        // ���ٵ�ѹ/�����ѹ�����˲�����,�䲻����������ɵĲ���,��������������˲�
        // ���ٵ�ѹÿ20us��һ�β���,Ϊ�����ڴ�ռ��,�ȶ�ÿ20(LEN_VF)��������һ��
        // ��ֵ�˲�����,Ȼ���ٽ��о�ֵ�˲�
        // ---------------------------------------------------------
        DataArrayVout.SumData -= DataArrayVout.Array[DataArrayVout.Flag];
        DataArrayVout.Array[DataArrayVout.Flag] = ADRESULT_VFAST;
        DataArrayVout.SumData += DataArrayVout.Array[DataArrayVout.Flag];
        SampleData.DataVout = DataArrayVout.SumData >> 5;
        if (++DataArrayVout.Flag >= LEN_NORMAL_ARRAY)
        {
            DataArrayVout.Flag = 0;
        }
        // ---------------------------------------------------------
        // ���ٵ��������˲�����,��ֵ�˲�
        // ---------------------------------------------------------
        DataArrayIout.SumData -= DataArrayIout.Array[DataArrayIout.Flag];
        DataArrayIout.Array[DataArrayIout.Flag] = ADRESULT_ISLOW - 220;
        DataArrayIout.SumData += DataArrayIout.Array[DataArrayIout.Flag];
        SampleData.DataIout = DataArrayIout.SumData >> 5;
        if (++DataArrayIout.Flag >= LEN_NORMAL_ARRAY)
        {
            DataArrayIout.Flag = 0;
        }
        // ---------------------------------------------------------
        // ���ٵ�ѹ(��ص�ѹ)�����˲�����,��ֵ�˲�
        // ---------------------------------------------------------
        DataArrayVbat.SumData -= DataArrayVbat.Array[DataArrayVbat.Flag];
        DataArrayVbat.Array[DataArrayVbat.Flag] = ADRESULT_VSLOW;
        DataArrayVbat.SumData += DataArrayVbat.Array[DataArrayVbat.Flag];
        SampleData.DataVbat= DataArrayVbat.SumData >> 5;
        if (++DataArrayVbat.Flag >= LEN_NORMAL_ARRAY)
        {
            DataArrayVbat.Flag = 0;
        }
        // ---------------------------------------------------------
        // ���ٵ�ѹ,���ٵ���,���ٵ�ѹ�Ľ����־��λ
        // ---------------------------------------------------------
        SampleState.bit.Vout = 1;
        SampleState.bit.Vbat = 1;
        SampleState.bit.Iout = 1;

        DataArrayTemp.SumData -= DataArrayTemp.Array[DataArrayTemp.Flag];
        DataArrayTemp.Array[DataArrayTemp.Flag] = 4095 - ADRESULT_TEMP;
        DataArrayTemp.SumData += DataArrayTemp.Array[DataArrayTemp.Flag];
        SampleData.DataTemp = DataArrayTemp.SumData >> 4;
        if (++DataArrayTemp.Flag >= LEN_SHORT_ARRAY)
        {
            DataArrayTemp.Flag = 0;
        }
    }
}


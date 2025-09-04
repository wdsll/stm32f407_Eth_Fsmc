/*******************************************************************************
 * LlcCurve.h
 *
 *  Created on: 2019-11-15
 *      Author: Gavin.D
*******************************************************************************/
#ifndef LLCCURVE_H_
#define LLCCURVE_H_

#include "DSP28x_Project.h"    // Device Headerfile and Examples Include File

/*******************************************************************************
**                               �궨��
*******************************************************************************/
#define BATCELLS            12                    //��ؽ�������ظ����ܵĵ�ش���
#define BATCAPACITY         251                   //�����������Ƶ�ظ�������
#define BATCAPACITY_1         225        //�����������Ƶ�ظ�������b02:T105(225Ah)(Trojan)
#define BATCAPACITY_2         200        //�����������Ƶ�ظ�������b07:T125(240Ah)(Trojan)
#define BATCAPACITY_3         225        //�����������Ƶ�ظ�������b10:T145(260Ah)(Trojan)
#define BATCAPACITY_4         170        //�����������Ƶ�ظ�������b11:T1275(300Ah)(Trojan)
#define BATCAPACITY_5         200        //�����������Ƶ�ظ�������b100:DT1275(200Ah)(Leoch)
#define BATCAPACITY_6         240        //�����������Ƶ�ظ�������b101:DT1275(100Ah)(Leoch)
#define BATCAPACITY_7         160        //�����������Ƶ�ظ�������b102:DT126(160Ah)(Leoch)
#define BATCAPACITY_8         155        //�����������Ƶ�ظ�������b103:DT103(155Ah)(Leoch)
#define BATCAPACITY_9         115        //�����������Ƶ�ظ�������b104:27TMH(115Ah)(Trojan)
#define BATCAPACITY_10         170       //�����������Ƶ�ظ�������b94:Hoppecke
#define BATCAPACITY_11         200       //�����������Ƶ�ظ�������b56:Hoppecke
#define BATCAPACITY_12         220       //�����������Ƶ�ظ�������b132:Hoppecke
#define BATCAPACITY_13         170       //�����������Ƶ�ظ�������b14:Hoppecke
#define BATCAPACITY_14         200       //�����������Ƶ�ظ�������b176:Hoppecke
#define BATCAPACITY_15         300        //�����������Ƶ�ظ�������b134:Hoppecke
#define BATCAPACITY_16         240       //�����������Ƶ�ظ�������b91:FULLRIVER
#define BATCAPACITY_17         115       //�����������Ƶ�ظ�������b105:FULLRIVER
#define BATCAPACITY_18         150       //�����������Ƶ�ظ�������b106:FULLRIVER
#define BATCAPACITY_19         160       //�����������Ƶ�ظ�������b107FULLRIVER
//#define BATCAPACITY_20         224       //�����������Ƶ�ظ�������b108:FULLRIVER
#define BATCAPACITY_20         200        //�����������Ƶ�ظ�������b100:DT1275(200Ah)(Leoch)
#define BATCAPACITY_21         250       //�����������Ƶ�ظ�������b109:FULLRIVER
#define BATCAPACITY_22         250       //�����������Ƶ�ظ�������b109:FULLRIVER
#define CHARGR_TIME         1800000      //���ʱ�䵥λ�����Сʱ
#define Ress_Line           34*2>>10              ///  1.5��5.1��ŷ��������3.4m��/m///1024==1000
#define Line_Lengths        15/100             ///  15//1.5m  ����ߵ��߾��볤��
#define Line_Loss_Vol       ((int32)((IOUT_DATA * K_IOUT_A + K_IOUT_B) >> K_EXP)*2/1000)
// =============================================================================
// ��ǰ�����ļ���Ӧ�ĵ�س������
// =============================================================================
#define CHGSTAT_PRECHARGE       0   // Ԥ��״̬
#define CHGSTAT_CONCURCHARGE    1   // �������һ״̬
#define CHGSTAT_SCONCURCHARGE   2   // ��������״̬
#define CHGSTAT_CONVOLCHARGE    3   // ��ѹ���һ״̬
#define CHGSTAT_BCONVOLCHARGE   4   // ������״̬
#define CHGSTAT_FLOATCHARGE     5   // ����״̬
#define CHGSTAT_EQMODECHARGE    6   // EQģʽ���
#define CHGSTAT_PAUSEMODE       7   // ��ͣ���״̬

// T105 225Ah==>  A# 121# ����3-EVF-225 225AH
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ�� 0
#define PRECHARGECMI_1        ((int32)900)             //ָ�����0.01C
#define PRECHARGECMV_1        ((int32)220*BATCELLS/10)           //ָ���ѹ����ܵ�ѹ 24.2
#define PRECHARGEENDV_1       ((int32)200*BATCELLS/10)           //Ԥ���������ܵ�ѹ 24
#define PRECHARGEENDCNT_1     ((int32)1*CHARGR_TIME)             //Ԥ�����ʱ��0.5H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ�������� 1
#define CONCURCHARGECMI_1     ((int32)13*BATCAPACITY_1)             //ָ�����0.13C    29.25A
#define CONCURCHARGECMV_1     ((int32)248*BATCELLS/10)           //ָ���ѹ����ܵ�ѹ// 29.8
#define CONCURCHARGEENDI_1    ((int32)11*BATCAPACITY_1)         //????
#define CONCURCHARGEENDV_1    ((int32)246*BATCELLS/10)           //������������ܵ�ѹ// 29.6
#define CONCURCHARGEENDCNT_1    ((int32)16*CHARGR_TIME)            ////8Сʱ???
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��2
#define SCONCURCHARGECMI_1    ((int32)13*BATCAPACITY_1)             //ָ�����0.11C   24��75A
#define SCONCURCHARGECMV_1    ((int32)252*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ30.2
#define SCONCURCHARGEENDV_1   ((int32)248*BATCELLS/10)            //������������ܵ�ѹ30����
#define SCONCURCHARGEENDCNT_1 ((int32)8*CHARGR_TIME)               //��������ʱ��4H???
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��3
#define CONVOLCHARGECMI_1     ((int32)11*BATCAPACITY_1)               //ָ�����0.12C
#define CONVOLCHARGECMV_1     ((int32)246*BATCELLS/10)               //ָ���ѹ����ܵ�ѹ30����
#define CONVOLCHARGEENDI_1    ((int32)1*BATCAPACITY_1)               //��ѹ��������ܵ���
#define CONVOLCHARGEENDCNT_1  ((int32)1*CHARGR_TIME)                 //��ѹ����ʱ��0.5H
//������׶γ����Ϣ��������ѹָ���תʱ��4
#define BCONVOLCHARGECMI_1    ((int32)1*BATCAPACITY_1)               //ָ�����0.01C
#define BCONVOLCHARGECMV_1    ((int32)248*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ 27.6
#define BCONVOLCHARGEENDV_1   ((int32)246*BATCELLS/10)             //������������ܵ�ѹ 31.1
#define BCONVOLCHARGEENDCNT_1 ((int32)6*CHARGR_TIME)              //��ѹ����ʱ��3H
//����׶γ����Ϣ��������ѹָ���תʱ��    ����5
#define FLOATCHARGECMI_1      ((int32)1*BATCAPACITY_1)            //ָ�����0.01C
#define FLOATCHARGECMV_1      ((int32)230*BATCELLS/10)              //ָ���ѹ����ܵ�ѹ 27.6
#define FLOATCHARGEENDCNT_1   ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��1H
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_1  ((int32)7*BATCAPACITY_1)                   //ָ�����0.07C
#define EQVoltCMD_1  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_1   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H

//Trojan T125 240Ah  C#  B71 ʥ��  3-EV-200
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ�� 0
#define PRECHARGECMI_2        ((int32)900)             //ָ�����0.01C
#define PRECHARGECMV_2        ((int32)206*BATCELLS/10)                 //ָ���ѹ����ܵ�ѹ24.2
#define PRECHARGEENDV_2       ((int32)200*BATCELLS/10)            //Ԥ���������ܵ�ѹ 24
#define PRECHARGEENDCNT_2     ((int32)2*CHARGR_TIME)              //Ԥ�����ʱ��2H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ�������� 1
#define CONCURCHARGECMI_2     ((int32)14*BATCAPACITY_2)              //ָ�����0.12C
#define CONCURCHARGECMV_2     ((int32)228*BATCELLS/10)            //ָ���ѹ����ܵ�ѹ29.6
#define CONCURCHARGEENDI_2    ((int32)13*BATCAPACITY_2)        //??   10
#define CONCURCHARGEENDV_2    ((int32)217*BATCELLS/10)            //������������ܵ�ѹ29.6 ����ת��
#define CONCURCHARGEENDCNT_2    ((int32)12*CHARGR_TIME)           ////6Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ�� 2
#define SCONCURCHARGECMI_2    ((int32)14*BATCAPACITY_2)              //ָ�����0.11C
#define SCONCURCHARGECMV_2    ((int32)248*BATCELLS/10)          //ָ���ѹ����ܵ�ѹ29.6
#define SCONCURCHARGEENDV_2   ((int32)237*BATCELLS/10)            //������������ܵ�ѹ29.6
#define SCONCURCHARGEENDCNT_2 ((int32)2*CHARGR_TIME)             //��������ʱ��1H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ�� 3
#define CONVOLCHARGECMI_2     ((int32)14*BATCAPACITY_2)              //ָ�����0.12C
#define CONVOLCHARGECMV_2     ((int32)247*BATCELLS/10)               //ָ���ѹ����ܵ�ѹ29.6
#define CONVOLCHARGEENDI_2    ((int32)2*BATCAPACITY_2)               //��ѹ��������ܵ���//0.02c
#define CONVOLCHARGEENDCNT_2  ((int32)4*CHARGR_TIME)                 //��ѹ����ʱ��2H
//������׶γ����Ϣ��������ѹָ���תʱ�� 4
#define BCONVOLCHARGECMI_2    ((int32)2*BATCAPACITY_2)               //ָ�����0.02C
#define BCONVOLCHARGECMV_2    ((int32)234*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ 28
#define BCONVOLCHARGEENDV_2   ((int32)232*BATCELLS/10)             //������������ܵ�ѹ33
#define BCONVOLCHARGEENDCNT_2 ((int32)4*CHARGR_TIME)               //��ѹ����ʱ��2H
//����׶γ����Ϣ��������ѹָ���תʱ�� 5
#define FLOATCHARGECMI_2      ((int32)1*BATCAPACITY_2)             //ָ�����0.01C
#define FLOATCHARGECMV_2      ((int32)233*BATCELLS/10)            //ָ���ѹ����ܵ�ѹ28
#define FLOATCHARGEENDCNT_2   ((int32)4*CHARGR_TIME)              //Ԥ�����ʱ��2H
//EQ���׶���Ϣ������ʱ��ָ�� 6
#define EQCurrCMD_2  ((int32)7*BATCAPACITY_2)                   //ָ�����0.07C
#define EQVoltCMD_2  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_2   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H

//����b10:Trojan T145 260Ah      A#  156 ���� 3-EVF-225 225A
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_3        ((int32)500)                //ָ�����0.01C
#define PRECHARGECMV_3        ((int32)206*BATCELLS/10)              //ָ���ѹ����ܵ�ѹ24.2
#define PRECHARGEENDV_3       ((int32)200*BATCELLS/10)             //Ԥ���������ܵ�ѹ24
#define PRECHARGEENDCNT_3     ((int32)1*CHARGR_TIME)               //Ԥ�����ʱ��0.5H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
#define CONCURCHARGECMI_3     ((int32)14*BATCAPACITY_3)           //ָ�����0.13C
#define CONCURCHARGECMV_3     ((int32)292)            //ָ���ѹ����ܵ�ѹ 29.8
#define CONCURCHARGEENDI_3    ((int32)12*BATCAPACITY_3)           //????
#define CONCURCHARGEENDV_3    ((int32)288)            //������������ܵ�ѹ 28.8
#define CONCURCHARGEENDCNT_3    ((int32)16*CHARGR_TIME)            ////8Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define SCONCURCHARGECMI_3    ((int32)2400)             //ָ�����24A
#define SCONCURCHARGECMV_3    ((int32)298)             //ָ���ѹ����ܵ�ѹ32.2
#define SCONCURCHARGEENDV_3   ((int32)296)             //������������ܵ�ѹ32
#define SCONCURCHARGEENDCNT_3 ((int32)8*CHARGR_TIME)               //��������ʱ��4H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
#define CONVOLCHARGECMI_3     ((int32)12*BATCAPACITY_3)              //ָ�����0.11C
#define CONVOLCHARGECMV_3     ((int32)267*BATCELLS/10)               //ָ���ѹ����ܵ�ѹ32
#define CONVOLCHARGEENDI_3    ((int32)1*BATCAPACITY_3)               //��ѹ��������ܵ���
#define CONVOLCHARGEENDCNT_3  ((int32)1*CHARGR_TIME)                 //��ѹ����ʱ��0.5H
//������׶γ����Ϣ��������ѹָ���תʱ��
#define BCONVOLCHARGECMI_3    ((int32)1*BATCAPACITY_3)               //ָ�����0.01C
#define BCONVOLCHARGECMV_3    ((int32)232*BATCELLS/10)              //ָ���ѹ����ܵ�ѹ27.6
#define BCONVOLCHARGEENDV_3   ((int32)230*BATCELLS/10)              //������������ܵ�ѹ35.8
#define BCONVOLCHARGEENDCNT_3 ((int32)6*CHARGR_TIME)                //��ѹ����ʱ��3H
//����׶γ����Ϣ��������ѹָ���תʱ��
#define FLOATCHARGECMI_3      ((int32)1*BATCAPACITY_3)              //ָ�����0.01C
#define FLOATCHARGECMV_3      ((int32)230*BATCELLS/10)               //ָ���ѹ����ܵ�ѹ27.6
#define FLOATCHARGEENDCNT_3   ((int32)2*CHARGR_TIME)                //Ԥ�����ʱ��1H
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_3  ((int32)7*BATCAPACITY_3)                     //ָ�����0.07C
#define EQVoltCMD_3  ((int32)272*BATCELLS/10)                    //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_3   ((int32)12*CHARGR_TIME)                     //Ԥ�����ʱ��1H

//����b11 Trojan T1275 300Ah  ==> 199   ʥ�� 3-EV-170
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_4        ((int32)500)           //ָ�����0.01C
#define PRECHARGECMV_4        ((int32)208*BATCELLS/10)           //ָ���ѹ����ܵ�ѹ24.2
#define PRECHARGEENDV_4       ((int32)200*BATCELLS/10)          //Ԥ���������ܵ�ѹ 24
#define PRECHARGEENDCNT_4     ((int32)2*CHARGR_TIME)              //Ԥ�����ʱ��1H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������  1
#define CONCURCHARGECMI_4     ((int32)15*BATCAPACITY_4)              //ָ�����0.12C
#define CONCURCHARGECMV_4     ((int32)226*BATCELLS/10)              //ָ���ѹ����ܵ�ѹ 29.6
#define CONCURCHARGEENDI_4    ((int32)14*BATCAPACITY_4)               //??11
#define CONCURCHARGEENDV_4    ((int32)215*BATCELLS/10)              //������������ܵ�ѹ29.4���ϣ�
#define CONCURCHARGEENDCNT_4  ((int32)12*CHARGR_TIME)               //6Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��  2
#define SCONCURCHARGECMI_4    ((int32)14*BATCAPACITY_4)              //ָ�����0.12C
#define SCONCURCHARGECMV_4    ((int32)252*BATCELLS/10)              //ָ���ѹ����ܵ�ѹ29.6
#define SCONCURCHARGEENDV_4   ((int32)240*BATCELLS/10)             //������������ܵ�ѹ 29.4
#define SCONCURCHARGEENDCNT_4 ((int32)2*CHARGR_TIME)               //��������ʱ��1H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ�� 3
#define CONVOLCHARGECMI_4     ((int32)14*BATCAPACITY_4)              //ָ�����0.12C
#define CONVOLCHARGECMV_4     ((int32)248*BATCELLS/10)               //ָ���ѹ����ܵ�ѹ30
#define CONVOLCHARGEENDI_4    ((int32)2*BATCAPACITY_4)               //��ѹ��������ܵ���
#define CONVOLCHARGEENDCNT_4  ((int32)4*CHARGR_TIME)                   //��ѹ����ʱ��2H
//������׶γ����Ϣ��������ѹָ���תʱ�� 4
#define BCONVOLCHARGECMI_4    ((int32)2*BATCAPACITY_4)               //ָ�����0.02C
#define BCONVOLCHARGECMV_4    ((int32)235*BATCELLS/10)               //ָ���ѹ����ܵ�ѹ 28
#define BCONVOLCHARGEENDV_4   ((int32)233*BATCELLS/10)               //������������ܵ�ѹ33
#define BCONVOLCHARGEENDCNT_4 ((int32)4*CHARGR_TIME)                 //��ѹ����ʱ��2H
//����׶γ����Ϣ��������ѹָ���תʱ�� 5
#define FLOATCHARGECMI_4      ((int32)1*BATCAPACITY_4)              //ָ�����0.01C
#define FLOATCHARGECMV_4      ((int32)233*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ28
#define FLOATCHARGEENDCNT_4   ((int32)4*CHARGR_TIME)               //Ԥ�����ʱ�� 2
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_4    ((int32)7*BATCAPACITY_4)                   //ָ�����0.07C
#define EQVoltCMD_4    ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_4    ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��6H


////
//����b02:Trojan T105 225Ah//�ڶ�������
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_5        ((int32)500)             //ָ�����0.01C
#define PRECHARGECMV_5        ((int32)209*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define PRECHARGEENDV_5       ((int32)199*BATCELLS/10)             //Ԥ���������ܵ�ѹ
#define PRECHARGEENDCNT_5     ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��2H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
#define CONCURCHARGECMI_5     ((int32)12*BATCAPACITY_5)              //ָ�����0.12C
#define CONCURCHARGECMV_5     ((int32)224*BATCELLS/10)              //ָ���ѹ����ܵ�ѹ//234
#define CONCURCHARGEENDI_5    ((int32)11*BATCAPACITY_5)
#define CONCURCHARGEENDV_5    ((int32)220*BATCELLS/10)             //������������ܵ�ѹ//230
#define CONCURCHARGEENDCNT_5    ((int32)10*CHARGR_TIME)            ////5Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define SCONCURCHARGECMI_5    ((int32)12*BATCAPACITY_5)              //ָ�����0.12C
#define SCONCURCHARGECMV_5    ((int32)235*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define SCONCURCHARGEENDV_5   ((int32)233*BATCELLS/10)             //������������ܵ�ѹ
#define SCONCURCHARGEENDCNT_5 ((int32)14*CHARGR_TIME)               //��������ʱ��7H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
#define CONVOLCHARGECMI_5     ((int32)12*BATCAPACITY_5)              //ָ�����0.12C
#define CONVOLCHARGECMV_5     ((int32)243*BATCELLS/10)               //ָ���ѹ����ܵ�ѹ
#define CONVOLCHARGEENDI_5    ((int32)2*BATCAPACITY_5)               //��ѹ��������ܵ���
#define CONVOLCHARGEENDCNT_5  ((int32)3*CHARGR_TIME)                 //��ѹ����ʱ��1.5H
//������׶γ����Ϣ��������ѹָ���תʱ��
#define BCONVOLCHARGECMI_5    ((int32)2*BATCAPACITY_5)               //ָ�����0.02C
#define BCONVOLCHARGECMV_5    ((int32)270*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define BCONVOLCHARGEENDV_5   ((int32)268*BATCELLS/10)             //������������ܵ�ѹ
#define BCONVOLCHARGEENDCNT_5 ((int32)3*CHARGR_TIME)               //��ѹ����ʱ��1.5H
//����׶γ����Ϣ��������ѹָ���תʱ��
#define FLOATCHARGECMI_5      ((int32)2*BATCAPACITY_5)                 //ָ�����0.01C
#define FLOATCHARGECMV_5      ((int32)223*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define FLOATCHARGEENDCNT_5   ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��1H
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_5  ((int32)7*BATCAPACITY_5)                   //ָ�����0.07C
#define EQVoltCMD_5  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_5   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H

//����b07:Trojan T125 240Ah �ڶ�������
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_6        ((int32)500)             //ָ�����0.01C
#define PRECHARGECMV_6        ((int32)21*BATCELLS)                 //ָ���ѹ����ܵ�ѹ
#define PRECHARGEENDV_6       ((int32)200*BATCELLS/10)             //Ԥ���������ܵ�ѹ
#define PRECHARGEENDCNT_6     ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��2H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
#define CONCURCHARGECMI_6     ((int32)12*BATCAPACITY_6)              //ָ�����0.12C
#define CONCURCHARGECMV_6     ((int32)234*BATCELLS/10)              //ָ���ѹ����ܵ�ѹ//234
#define CONCURCHARGEENDI_6    ((int32)11*BATCAPACITY_6)
#define CONCURCHARGEENDV_6    ((int32)220*BATCELLS/10)             //������������ܵ�ѹ//230
#define CONCURCHARGEENDCNT_6    ((int32)10*CHARGR_TIME)            ////5Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define SCONCURCHARGECMI_6    ((int32)12*BATCAPACITY_6)              //ָ�����0.12C
#define SCONCURCHARGECMV_6    ((int32)245*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define SCONCURCHARGEENDV_6   ((int32)233*BATCELLS/10)             //������������ܵ�ѹ
#define SCONCURCHARGEENDCNT_6 ((int32)14*CHARGR_TIME)               //��������ʱ��7H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
#define CONVOLCHARGECMI_6     ((int32)12*BATCAPACITY_6)              //ָ�����0.12C
#define CONVOLCHARGECMV_6     ((int32)243*BATCELLS/10)               //ָ���ѹ����ܵ�ѹ
#define CONVOLCHARGEENDI_6    ((int32)2*BATCAPACITY_6)               //��ѹ��������ܵ���
#define CONVOLCHARGEENDCNT_6  ((int32)3*CHARGR_TIME)                 //��ѹ����ʱ��1.5H
//������׶γ����Ϣ��������ѹָ���תʱ��
#define BCONVOLCHARGECMI_6    ((int32)2*BATCAPACITY_6)               //ָ�����0.02C
#define BCONVOLCHARGECMV_6    ((int32)270*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define BCONVOLCHARGEENDV_6   ((int32)268*BATCELLS/10)             //������������ܵ�ѹ
#define BCONVOLCHARGEENDCNT_6 ((int32)3*CHARGR_TIME)               //��ѹ����ʱ��1.5H
//����׶γ����Ϣ��������ѹָ���תʱ��
#define FLOATCHARGECMI_6      ((int32)2*BATCAPACITY_6)                 //ָ�����0.01C
#define FLOATCHARGECMV_6      ((int32)223*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define FLOATCHARGEENDCNT_6   ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��1H
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_6   ((int32)7*BATCAPACITY_6)                   //ָ�����0.07C
#define EQVoltCMD_6   ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_6   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H

// //�ڶ������ӽ���
////����///b100:Leoch DT1275 6V-200Ah
////Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
//#define PRECHARGECMI_5        ((int32)BATCAPACITY_5)               //ָ�����0.05C
//#define PRECHARGECMV_5        ((int32)20*BATCELLS)                 //ָ���ѹ����ܵ�ѹ
//#define PRECHARGEENDV_5       ((int32)199*BATCELLS/10)             //Ԥ���������ܵ�ѹ23.9V
//#define PRECHARGEENDCNT_5     ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��1H
////�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
//#define CONCURCHARGECMI_5     ((int32)15*BATCAPACITY_5)            //ָ�����0.15C
//#define CONCURCHARGECMV_5     ((int32)23*BATCELLS)                 //ָ���ѹ����ܵ�ѹ//25
//#define CONCURCHARGEENDI_5    ((int32)12*BATCAPACITY_5)
//#define CONCURCHARGEENDV_5    ((int32)225*BATCELLS/10)             //������������ܵ�ѹ28.64V///245
//#define CONCURCHARGEENDCNT_5    ((int32)6*CHARGR_TIME)              ////3Сʱ
////���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
//#define SCONCURCHARGECMI_5    ((int32)15*BATCAPACITY_5)            //ָ�����0.15C
//#define SCONCURCHARGECMV_5    ((int32)246*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
//#define SCONCURCHARGEENDV_5   ((int32)244*BATCELLS/10)             //������������ܵ�ѹ29.4V
//#define SCONCURCHARGEENDCNT_5 ((int32)8*CHARGR_TIME)               //��������ʱ��7H
////��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
//#define CONVOLCHARGECMI_5     ((int32)15*BATCAPACITY_5)            //ָ�����0.15C
//#define CONVOLCHARGECMV_5     ((int32)245*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ29.4V
//#define CONVOLCHARGEENDI_5    ((int32)5*BATCAPACITY_5)            //��ѹ��������ܵ���
//#define CONVOLCHARGEENDCNT_5  ((int32)6*CHARGR_TIME)               //��ѹ����ʱ��3H
////������׶γ����Ϣ��������ѹָ���תʱ��
//#define BCONVOLCHARGECMI_5    ((int32)5*BATCAPACITY_5)            //ָ�����0.05C
//#define BCONVOLCHARGECMV_5    ((int32)268*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
//#define BCONVOLCHARGEENDV_5   ((int32)264*BATCELLS/10)             //�����������ܵ�ѹ32.4V
//#define BCONVOLCHARGEENDI_5   ((int32)15*BATCAPACITY_5/10)         //ָ�����0.015C
//#define BCONVOLCHARGEENDCNT_5 ((int32)8*CHARGR_TIME)               //�������ʱ��4H
////����׶γ����Ϣ��������ѹָ���תʱ��
//#define FLOATCHARGECMI_5      ((int32)BATCAPACITY_5)               //ָ�����0.01C
//#define FLOATCHARGECMV_5      ((int32)229*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ27V
//#define FLOATCHARGEENDCNT_5   ((int32)CHARGR_TIME)               //�������ʱ��0.5H
////EQ���׶���Ϣ������ʱ��ָ��
//#define EQCurrCMD_5  ((int32)7*BATCAPACITY_5)                   //ָ�����0.07C
//#define EQVoltCMD_5  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
//#define EQTimeCMD_5   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��6H
//
////����///101:Leoch DT1275 6V-100Ah  13h max
////Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
//#define PRECHARGECMI_6        ((int32)BATCAPACITY_6)               //ָ�����0.05C
//#define PRECHARGECMV_6        ((int32)20*BATCELLS)                 //ָ���ѹ����ܵ�ѹ
//#define PRECHARGEENDV_6       ((int32)199*BATCELLS/10)             //Ԥ���������ܵ�ѹ23.9V
//#define PRECHARGEENDCNT_6     ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��1H
////�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
//#define CONCURCHARGECMI_6     ((int32)15*BATCAPACITY_6)            //ָ�����0.15C
//#define CONCURCHARGECMV_6     ((int32)23*BATCELLS)                 //ָ���ѹ����ܵ�ѹ//25
//#define CONCURCHARGEENDI_6    ((int32)12*BATCAPACITY_6)
//#define CONCURCHARGEENDV_6    ((int32)225*BATCELLS/10)             //������������ܵ�ѹ28.64V///245
//#define CONCURCHARGEENDCNT_6    ((int32)6*CHARGR_TIME)              ////3Сʱ
////���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
//#define SCONCURCHARGECMI_6    ((int32)15*BATCAPACITY_6)            //ָ�����0.15C
//#define SCONCURCHARGECMV_6    ((int32)246*BATCELLS/10)                 //ָ���ѹ����ܵ�ѹ
//#define SCONCURCHARGEENDV_6   ((int32)244*BATCELLS/10)             //������������ܵ�ѹ29.4V
//#define SCONCURCHARGEENDCNT_6 ((int32)8*CHARGR_TIME)               //��������ʱ��7H
////��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
//#define CONVOLCHARGECMI_6     ((int32)15*BATCAPACITY_6)            //ָ�����0.15C
//#define CONVOLCHARGECMV_6     ((int32)245*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ29.4V
//#define CONVOLCHARGEENDI_6    ((int32)5*BATCAPACITY_6)            //��ѹ��������ܵ���
//#define CONVOLCHARGEENDCNT_6  ((int32)6*CHARGR_TIME)               //��ѹ����ʱ��3H
////������׶γ����Ϣ��������ѹָ���תʱ��
//#define BCONVOLCHARGECMI_6    ((int32)5*BATCAPACITY_6)            //ָ�����0.05C
//#define BCONVOLCHARGECMV_6    ((int32)268*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
//#define BCONVOLCHARGEENDV_6   ((int32)264*BATCELLS/10)             //�����������ܵ�ѹ
//#define BCONVOLCHARGEENDI_6   ((int32)15*BATCAPACITY_6/10)         //ָ�����0.015C
//#define BCONVOLCHARGEENDCNT_6 ((int32)8*CHARGR_TIME)               //�������ʱ��4H
////����׶γ����Ϣ��������ѹָ���תʱ��
//#define FLOATCHARGECMI_6      ((int32)BATCAPACITY_6)               //ָ�����0.01C
//#define FLOATCHARGECMV_6      ((int32)229*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ27V
//#define FLOATCHARGEENDCNT_6   ((int32)CHARGR_TIME)               //�������ʱ��0.5H
////EQ���׶���Ϣ������ʱ��ָ��
//#define EQCurrCMD_6  ((int32)7*BATCAPACITY_6)                   //ָ�����0.07C
//#define EQVoltCMD_6  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
//#define EQTimeCMD_6   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��6H







//����///102:Leoch DT106 6V-160Ah
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_7        ((int32)BATCAPACITY_7)               //ָ�����0.05C
#define PRECHARGECMV_7        ((int32)202*BATCELLS/10)                 //ָ���ѹ����ܵ�ѹ
#define PRECHARGEENDV_7       ((int32)199*BATCELLS/10)             //Ԥ���������ܵ�ѹ23.9V
#define PRECHARGEENDCNT_7     ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��1H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
#define CONCURCHARGECMI_7     ((int32)15*BATCAPACITY_7)            //ָ�����0.15C
#define CONCURCHARGECMV_7     ((int32)23*BATCELLS)                 //ָ���ѹ����ܵ�ѹ//25
#define CONCURCHARGEENDI_7    ((int32)12*BATCAPACITY_7)
#define CONCURCHARGEENDV_7    ((int32)225*BATCELLS/10)             //������������ܵ�ѹ28.64V///245
#define CONCURCHARGEENDCNT_7    ((int32)6*CHARGR_TIME)              ////3Сʱ���ϣ�
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define SCONCURCHARGECMI_7    ((int32)15*BATCAPACITY_7)            //ָ�����0.15C
#define SCONCURCHARGECMV_7    ((int32)246*BATCELLS/10)                 //ָ���ѹ����ܵ�ѹ
#define SCONCURCHARGEENDV_7   ((int32)244*BATCELLS/10)             //������������ܵ�ѹ29.4V
#define SCONCURCHARGEENDCNT_7 ((int32)8*CHARGR_TIME)               //��������ʱ��7H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
#define CONVOLCHARGECMI_7     ((int32)15*BATCAPACITY_7)            //ָ�����0.15C
#define CONVOLCHARGECMV_7     ((int32)245*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ29.4V
#define CONVOLCHARGEENDI_7    ((int32)5*BATCAPACITY_7)            //��ѹ��������ܵ���
#define CONVOLCHARGEENDCNT_7  ((int32)6*CHARGR_TIME)               //��ѹ����ʱ��3H
//������׶γ����Ϣ��������ѹָ���תʱ��
#define BCONVOLCHARGECMI_7    ((int32)5*BATCAPACITY_7)            //ָ�����0.05C
#define BCONVOLCHARGECMV_7    ((int32)268*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define BCONVOLCHARGEENDV_7   ((int32)264*BATCELLS/10)             //�����������ܵ�ѹ
#define BCONVOLCHARGEENDI_7   ((int32)15*BATCAPACITY_7/10)         //ָ�����0.015C
#define BCONVOLCHARGEENDCNT_7 ((int32)8*CHARGR_TIME)               //�������ʱ��4H
//����׶γ����Ϣ��������ѹָ���תʱ��
#define FLOATCHARGECMI_7      ((int32)BATCAPACITY_7)               //ָ�����
#define FLOATCHARGECMV_7      ((int32)229*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define FLOATCHARGEENDI_7    ((int32)BATCAPACITY_7)                //��ѹ��������ܵ���
#define FLOATCHARGEENDCNT_7   ((int32)CHARGR_TIME)                 //�������ʱ��
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_7  ((int32)7*BATCAPACITY_7)                        //ָ�����0.07C
#define EQVoltCMD_7  ((int32)272*BATCELLS/10)                        //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_7   ((int32)12*CHARGR_TIME)                        //Ԥ�����ʱ��6H

//����///103:Leoch DT106 6V-155Ah  13h max
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_8        ((int32)BATCAPACITY_8)               //ָ�����0.05C
#define PRECHARGECMV_8        ((int32)202*BATCELLS/10)                 //ָ���ѹ����ܵ�ѹ
#define PRECHARGEENDV_8       ((int32)199*BATCELLS/10)             //Ԥ���������ܵ�ѹ23.9V
#define PRECHARGEENDCNT_8     ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��1H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
#define CONCURCHARGECMI_8     ((int32)15*BATCAPACITY_8)            //ָ�����0.15C
#define CONCURCHARGECMV_8     ((int32)23*BATCELLS)                 //ָ���ѹ����ܵ�ѹ//25
#define CONCURCHARGEENDI_8    ((int32)12*BATCAPACITY_8)
#define CONCURCHARGEENDV_8    ((int32)225*BATCELLS/10)             //������������ܵ�ѹ28.64V///245
#define CONCURCHARGEENDCNT_8    ((int32)6*CHARGR_TIME)              ////3Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define SCONCURCHARGECMI_8    ((int32)15*BATCAPACITY_8)            //ָ�����0.15C
#define SCONCURCHARGECMV_8    ((int32)246*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define SCONCURCHARGEENDV_8   ((int32)244*BATCELLS/10)             //������������ܵ�ѹ29.4V
#define SCONCURCHARGEENDCNT_8 ((int32)8*CHARGR_TIME)               //��������ʱ��7H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
#define CONVOLCHARGECMI_8     ((int32)15*BATCAPACITY_8)            //ָ�����0.15C
#define CONVOLCHARGECMV_8     ((int32)245*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ29.4V
#define CONVOLCHARGEENDI_8    ((int32)5*BATCAPACITY_8)            //��ѹ��������ܵ���
#define CONVOLCHARGEENDCNT_8  ((int32)6*CHARGR_TIME)               //��ѹ����ʱ��3H
//������׶γ����Ϣ��������ѹָ���תʱ��
#define BCONVOLCHARGECMI_8    ((int32)5*BATCAPACITY_8)            //ָ�����0.05C
#define BCONVOLCHARGECMV_8    ((int32)266*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define BCONVOLCHARGEENDV_8   ((int32)265*BATCELLS/10)             //�����������ܵ�ѹ
#define BCONVOLCHARGEENDI_8   ((int32)15*BATCAPACITY_8/10)         //ָ�����0.015C
#define BCONVOLCHARGEENDCNT_8 ((int32)8*CHARGR_TIME)               //�������ʱ��4H
//����׶γ����Ϣ��������ѹָ���תʱ��
#define FLOATCHARGECMI_8      ((int32)5*BATCAPACITY_8)               //ָ�����0.05C
#define FLOATCHARGECMV_8      ((int32)227*BATCELLS/10)              //ָ���ѹ����ܵ�ѹ
#define FLOATCHARGEENDI_8     ((int32)BATCAPACITY_8)                //�����������ܵ���
#define FLOATCHARGEENDCNT_8   ((int32)4*CHARGR_TIME)                //�������ʱ��2H
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_8  ((int32)7*BATCAPACITY_8)                   //ָ�����0.07C
#define EQVoltCMD_8  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_8   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H

//����104:Trojan 27TMH 115Ah
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_9        ((int32)BATCAPACITY_9)               //ָ�����0.01C
#define PRECHARGECMV_9        ((int32)202*BATCELLS/10)                 //ָ���ѹ����ܵ�ѹ
#define PRECHARGEENDV_9       ((int32)199*BATCELLS/10)             //Ԥ���������ܵ�ѹ
#define PRECHARGEENDCNT_9     ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��2H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
#define CONCURCHARGECMI_9     ((int32)12*BATCAPACITY_9)              //ָ�����0.12C
#define CONCURCHARGECMV_9     ((int32)224*BATCELLS/10)              //ָ���ѹ����ܵ�ѹ//234
#define CONCURCHARGEENDI_9    ((int32)11*BATCAPACITY_9)
#define CONCURCHARGEENDV_9    ((int32)220*BATCELLS/10)             //������������ܵ�ѹ//230
#define CONCURCHARGEENDCNT_9    ((int32)10*CHARGR_TIME)            ////5Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define SCONCURCHARGECMI_9    ((int32)12*BATCAPACITY_9)              //ָ�����0.12C
#define SCONCURCHARGECMV_9    ((int32)235*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define SCONCURCHARGEENDV_9   ((int32)233*BATCELLS/10)             //������������ܵ�ѹ
#define SCONCURCHARGEENDCNT_9 ((int32)14*CHARGR_TIME)               //��������ʱ��7H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
#define CONVOLCHARGECMI_9     ((int32)12*BATCAPACITY_9)              //ָ�����0.12C
#define CONVOLCHARGECMV_9     ((int32)235*BATCELLS/10)               //ָ���ѹ����ܵ�ѹ
#define CONVOLCHARGEENDI_9    ((int32)2*BATCAPACITY_1)               //��ѹ��������ܵ���
#define CONVOLCHARGEENDCNT_9  ((int32)3*CHARGR_TIME)                 //��ѹ����ʱ��1.5H
//������׶γ����Ϣ��������ѹָ���תʱ��
#define BCONVOLCHARGECMI_9    ((int32)2*BATCAPACITY_9)               //ָ�����0.02C
#define BCONVOLCHARGECMV_9    ((int32)270*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define BCONVOLCHARGEENDV_9   ((int32)268*BATCELLS/10)             //������������ܵ�ѹ
#define BCONVOLCHARGEENDCNT_9 ((int32)3*CHARGR_TIME)               //��ѹ����ʱ��1.5H
//����׶γ����Ϣ��������ѹָ���תʱ��
#define FLOATCHARGECMI_9      ((int32)2*BATCAPACITY_9)                 //ָ�����0.01C
#define FLOATCHARGECMV_9      ((int32)223*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define FLOATCHARGEENDCNT_9   ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��1H
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_9  ((int32)7*BATCAPACITY_9)                   //ָ�����0.07C
#define EQVoltCMD_9  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_9   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H

//����L10:b94:Hoppecke 170Ah
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_10        ((int32)BATCAPACITY_10)               //ָ�����0.01C
#define PRECHARGECMV_10        ((int32)202*BATCELLS/10)                 //ָ���ѹ����ܵ�ѹ
#define PRECHARGEENDV_10       ((int32)199*BATCELLS/10)             //Ԥ���������ܵ�ѹ
#define PRECHARGEENDCNT_10     ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��1H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
#define CONCURCHARGECMI_10     ((int32)15*BATCAPACITY_10)              //ָ�����0.15C
#define CONCURCHARGECMV_10     ((int32)23.2*BATCELLS)                 //ָ���ѹ����ܵ�ѹ
#define CONCURCHARGEENDI_10    ((int32)14*BATCAPACITY_10)
#define CONCURCHARGEENDV_10    ((int32)22.6*BATCELLS)                 //������������ܵ�ѹ
#define CONCURCHARGEENDCNT_10  ((int32)4*CHARGR_TIME)               ////2Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define SCONCURCHARGECMI_10    ((int32)15*BATCAPACITY_10)              //ָ�����0.15C
#define SCONCURCHARGECMV_10    ((int32)238*BATCELLS/10)                 //ָ���ѹ����ܵ�ѹ
#define SCONCURCHARGEENDV_10   ((int32)236*BATCELLS/10)              //������������ܵ�ѹ28.4
#define SCONCURCHARGEENDCNT_10 ((int32)10*CHARGR_TIME)               //��������ʱ��5H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
#define CONVOLCHARGECMI_10     ((int32)15*BATCAPACITY_10)               //ָ�����0.15C
#define CONVOLCHARGECMV_10     ((int32)239*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define CONVOLCHARGEENDI_10    ((int32)8*BATCAPACITY_10/10)         //��ѹ��������ܵ���0.008C
#define CONVOLCHARGEENDCNT_10  ((int32)6*CHARGR_TIME)               //��ѹ����ʱ��3H
//������׶γ����Ϣ��������ѹָ���תʱ��
#define BCONVOLCHARGECMI_10    ((int32)8*BATCAPACITY_10/10)         ////0.008C
#define BCONVOLCHARGECMV_10    ((int32)260*BATCELLS/10)             ////31.2V
#define BCONVOLCHARGEENDV_10   ((int32)258*BATCELLS/10)             ///31
#define BCONVOLCHARGEENDCNT_10 ((int32)10*CHARGR_TIME)              ////5Hours
//����׶γ����Ϣ��������ѹָ���תʱ��
#define FLOATCHARGECMI_10      ((int32)2*BATCAPACITY_10)                 //ָ�����0.01C
#define FLOATCHARGECMV_10      ((int32)229*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ6.9*4V
#define FLOATCHARGEENDCNT_10   ((int32)2*CHARGR_TIME)               //�������ʱ��4H
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_10  ((int32)7*BATCAPACITY_10)                   //ָ�����0.07C
#define EQVoltCMD_10  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_10   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H

//����///L11:b56:Hoppecke 200Ah
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_11        ((int32)BATCAPACITY_11)               //ָ�����0.01C
#define PRECHARGECMV_11        ((int32)20*BATCELLS)
//ָ���ѹ����ܵ�ѹ
#define PRECHARGEENDV_11       ((int32)199*BATCELLS/10)             //Ԥ���������ܵ�ѹ
#define PRECHARGEENDCNT_11     ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��1H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
#define CONCURCHARGECMI_11     ((int32)15*BATCAPACITY_11)              //ָ�����0.15C
#define CONCURCHARGECMV_11     ((int32)23.2*BATCELLS)                 //ָ���ѹ����ܵ�ѹ
#define CONCURCHARGEENDI_11    ((int32)14*BATCAPACITY_11)
#define CONCURCHARGEENDV_11    ((int32)22.6*BATCELLS)                 //������������ܵ�ѹ
#define CONCURCHARGEENDCNT_11  ((int32)4*CHARGR_TIME)               ////2Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define SCONCURCHARGECMI_11    ((int32)15*BATCAPACITY_11)              //ָ�����0.15C
#define SCONCURCHARGECMV_11    ((int32)238*BATCELLS/10)                 //ָ���ѹ����ܵ�ѹ
#define SCONCURCHARGEENDV_11   ((int32)236*BATCELLS/10)              //������������ܵ�ѹ28.4
#define SCONCURCHARGEENDCNT_11 ((int32)10*CHARGR_TIME)               //��������ʱ��5H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
#define CONVOLCHARGECMI_11     ((int32)15*BATCAPACITY_11)               //ָ�����0.15C
#define CONVOLCHARGECMV_11     ((int32)239*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define CONVOLCHARGEENDI_11    ((int32)8*BATCAPACITY_11/10)         //��ѹ��������ܵ���0.008C
#define CONVOLCHARGEENDCNT_11  ((int32)6*CHARGR_TIME)               //��ѹ����ʱ��3H
//������׶γ����Ϣ��������ѹָ���תʱ��
#define BCONVOLCHARGECMI_11    ((int32)8*BATCAPACITY_11/10)         ////0.008C
#define BCONVOLCHARGECMV_11    ((int32)260*BATCELLS/10)             ////31.2V
#define BCONVOLCHARGEENDV_11   ((int32)258*BATCELLS/10)             ///31
#define BCONVOLCHARGEENDCNT_11 ((int32)10*CHARGR_TIME)              ////5Hours
//����׶γ����Ϣ��������ѹָ���תʱ��
#define FLOATCHARGECMI_11      ((int32)2*BATCAPACITY_11)                 //ָ�����0.01C
#define FLOATCHARGECMV_11      ((int32)229*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ6.9*4V
#define FLOATCHARGEENDCNT_11   ((int32)CHARGR_TIME)               //�������ʱ��0.5H
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_11  ((int32)7*BATCAPACITY_11)                   //ָ�����0.07C
#define EQVoltCMD_11  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_11   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H

//����L12:b132:Hoppecke 220Ah
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_12        ((int32)BATCAPACITY_12)               //ָ�����0.01C
#define PRECHARGECMV_12        ((int32)202*BATCELLS/10)                 //ָ���ѹ����ܵ�ѹ
#define PRECHARGEENDV_12       ((int32)199*BATCELLS/10)             //Ԥ���������ܵ�ѹ
#define PRECHARGEENDCNT_12     ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��1H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
#define CONCURCHARGECMI_12     ((int32)15*BATCAPACITY_12)              //ָ�����0.15C
#define CONCURCHARGECMV_12     ((int32)23.2*BATCELLS)                 //ָ���ѹ����ܵ�ѹ
#define CONCURCHARGEENDI_12    ((int32)14*BATCAPACITY_12)
#define CONCURCHARGEENDV_12    ((int32)22.6*BATCELLS)                 //������������ܵ�ѹ
#define CONCURCHARGEENDCNT_12  ((int32)4*CHARGR_TIME)               ////2Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define SCONCURCHARGECMI_12    ((int32)15*BATCAPACITY_12)              //ָ�����0.15C
#define SCONCURCHARGECMV_12    ((int32)238*BATCELLS/10)                 //ָ���ѹ����ܵ�ѹ
#define SCONCURCHARGEENDV_12   ((int32)236*BATCELLS/10)              //������������ܵ�ѹ28.4
#define SCONCURCHARGEENDCNT_12 ((int32)10*CHARGR_TIME)               //��������ʱ��5H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
#define CONVOLCHARGECMI_12     ((int32)15*BATCAPACITY_12)               //ָ�����0.15C
#define CONVOLCHARGECMV_12     ((int32)239*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define CONVOLCHARGEENDI_12    ((int32)8*BATCAPACITY_11/10)         //��ѹ��������ܵ���0.008C
#define CONVOLCHARGEENDCNT_12  ((int32)6*CHARGR_TIME)               //��ѹ����ʱ��3H
//������׶γ����Ϣ��������ѹָ���תʱ��
#define BCONVOLCHARGECMI_12    ((int32)8*BATCAPACITY_12/10)         ////0.008C
#define BCONVOLCHARGECMV_12    ((int32)260*BATCELLS/10)             ////31.2V
#define BCONVOLCHARGEENDV_12   ((int32)258*BATCELLS/10)             ///31
#define BCONVOLCHARGEENDCNT_12 ((int32)10*CHARGR_TIME)              ////5Hours
//����׶γ����Ϣ��������ѹָ���תʱ��
#define FLOATCHARGECMI_12      ((int32)BATCAPACITY_12)                 ////ָ�����0.01C
#define FLOATCHARGECMV_12      ((int32)227*BATCELLS/10)             ////ָ���ѹ����ܵ�ѹ6.9*4V
#define FLOATCHARGEENDCNT_12   ((int32)CHARGR_TIME)               ////�������ʱ��0.5
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_12  ((int32)7*BATCAPACITY_12)                   //ָ�����0.07C
#define EQVoltCMD_12  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_12   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H

//����L13:b14:Hoppecke 115Ah    B# 129 ���� 3-EV-170 170AH
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_13        ((int32)900)               //ָ�����0.01C
#define PRECHARGECMV_13        ((int32)204*BATCELLS/10)              //ָ���ѹ����ܵ�ѹ  24.2
#define PRECHARGEENDV_13       ((int32)200*BATCELLS/10)              //Ԥ���������ܵ�ѹ24
#define PRECHARGEENDCNT_13     ((int32)2*CHARGR_TIME)                //Ԥ�����ʱ��1H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
#define CONCURCHARGECMI_13     ((int32)18*BATCAPACITY_13)              //ָ�����0.195C
#define CONCURCHARGECMV_13     ((int32)248*BATCELLS/10)                //ָ���ѹ����ܵ�ѹ29.8
#define CONCURCHARGEENDI_13    ((int32)17*BATCAPACITY_13)               //??18
#define CONCURCHARGEENDV_13    ((int32)246*BATCELLS/10)                 //������������ܵ�ѹ29.6
#define CONCURCHARGEENDCNT_13  ((int32)12*CHARGR_TIME)                  ////7Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define SCONCURCHARGECMI_13    ((int32)18*BATCAPACITY_13)            //ָ�����0.19C
#define SCONCURCHARGECMV_13    ((int32)248*BATCELLS/10)                //ָ���ѹ����ܵ�ѹ29.8
#define SCONCURCHARGEENDV_13   ((int32)246*BATCELLS/10)               //������������ܵ�ѹ29.6
#define SCONCURCHARGEENDCNT_13 ((int32)2*CHARGR_TIME)                //��������ʱ��6H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
#define CONVOLCHARGECMI_13     ((int32)18*BATCAPACITY_13)           //ָ�����0.15C
#define CONVOLCHARGECMV_13     ((int32)246*BATCELLS/10)            //ָ���ѹ����ܵ�ѹ29.6
#define CONVOLCHARGEENDI_13    ((int32)15*BATCAPACITY_13/10)     //��ѹ��������ܵ���0.015C
#define CONVOLCHARGEENDCNT_13  ((int32)6*CHARGR_TIME)              //��ѹ����ʱ��3H
//������׶γ����Ϣ��������ѹָ���תʱ��
#define BCONVOLCHARGECMI_13    ((int32)15*BATCAPACITY_13/10)           /*ָ�����0.015C*/
#define BCONVOLCHARGECMV_13    ((int32)252*BATCELLS/10)            //ָ���ѹ
#define BCONVOLCHARGEENDV_13   ((int32)250*BATCELLS/10)            ///������ѹ30
#define BCONVOLCHARGEENDCNT_13 ((int32)2*CHARGR_TIME)             ////ʱ������1Hours*/
//����׶γ����Ϣ��������ѹָ���תʱ��
#define FLOATCHARGECMI_13      ((int32)15*BATCAPACITY_13/10)         //ָ�����0.015C
#define FLOATCHARGECMV_13      ((int32)250*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ30
#define FLOATCHARGEENDCNT_13   ((int32)4*CHARGR_TIME)              //Ԥ�����ʱ��1H*/
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_13  ((int32)7*BATCAPACITY_13)                   /*//ָ�����0.07C*/
#define EQVoltCMD_13  ((int32)272*BATCELLS/10)                   /*//ָ���ѹ����ܵ�ѹ*/
#define EQTimeCMD_13   ((int32)12*CHARGR_TIME)                    /*//Ԥ�����ʱ��1H*/

//����14:b176:Hoppecke 260Ah   B# 214 �ڽ���  3-EVF-200  200AH
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_14        ((int32)900)               //ָ�����0.01C
#define PRECHARGECMV_14        ((int32)208*BATCELLS/10)              //ָ���ѹ����ܵ�ѹ 24.2
#define PRECHARGEENDV_14       ((int32)200*BATCELLS/10)             //Ԥ���������ܵ�ѹ 24
#define PRECHARGEENDCNT_14     ((int32)2*CHARGR_TIME)                //Ԥ�����ʱ��1H

//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
#define CONCURCHARGECMI_14     ((int32)15*BATCAPACITY_14)            //ָ�����0.15C
#define CONCURCHARGECMV_14     ((int32)238*BATCELLS/10)                  //ָ���ѹ����ܵ�ѹ29.8
#define CONCURCHARGEENDI_14    ((int32)14*BATCAPACITY_14)                //18??
#define CONCURCHARGEENDV_14    ((int32)226*BATCELLS/10)                 //������������ܵ�ѹ29.6
#define CONCURCHARGEENDCNT_14  ((int32)12*CHARGR_TIME)                 ////6Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define SCONCURCHARGECMI_14    ((int32)14*BATCAPACITY_14)            //ָ�����0.15C
#define SCONCURCHARGECMV_14    ((int32)248*BATCELLS/10)                //ָ���ѹ����ܵ�ѹ 29.8
#define SCONCURCHARGEENDV_14   ((int32)246*BATCELLS/10)              //������������ܵ�ѹ29.6
#define SCONCURCHARGEENDCNT_14 ((int32)2*CHARGR_TIME)               //��������ʱ��1H

//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
#define CONVOLCHARGECMI_14     ((int32)14*BATCAPACITY_14)           //ָ�����0.14C
#define CONVOLCHARGECMV_14     ((int32)246*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ 29.6
#define CONVOLCHARGEENDI_14    ((int32)15*BATCAPACITY_14/10)         //��ѹ��������ܵ���0.015C
#define CONVOLCHARGEENDCNT_14  ((int32)6*CHARGR_TIME)               //��ѹ����ʱ��3H
//������׶γ����Ϣ��������ѹָ���תʱ��
#define BCONVOLCHARGECMI_14    ((int32)15*BATCAPACITY_14/10)         //0.015C
#define BCONVOLCHARGECMV_14    ((int32)252*BATCELLS/10)              // 30.2
#define BCONVOLCHARGEENDV_14   ((int32)250*BATCELLS/10)              // 30
#define BCONVOLCHARGEENDCNT_14 ((int32)4*CHARGR_TIME)                //2Hours
//����׶γ����Ϣ��������ѹָ���תʱ��
#define FLOATCHARGECMI_14      ((int32)15*BATCAPACITY_14/10)          //ָ�����0.015C
#define FLOATCHARGECMV_14      ((int32)250*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ30
#define FLOATCHARGEENDCNT_14   ((int32)4*CHARGR_TIME)                //Ԥ�����ʱ��2H
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_14  ((int32)7*BATCAPACITY_14)                   //ָ�����0.07C
#define EQVoltCMD_14  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_14   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H

//����15:b134:Hoppecke 90Ah ==> D# B11 �� T1275 300A
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_15        ((int32)500)        //ָ�����0.005C
#define PRECHARGECMV_15        ((int32)208*BATCELLS/10)           //ָ���ѹ����ܵ�ѹ24.2
#define PRECHARGEENDV_15       ((int32)200*BATCELLS/10)           //Ԥ���������ܵ�ѹ 24
#define PRECHARGEENDCNT_15     ((int32)2*CHARGR_TIME)              //Ԥ�����ʱ��1H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ�������� 1
#define CONCURCHARGECMI_15     ((int32)10*BATCAPACITY_15)              //ָ�����0.10C
#define CONCURCHARGECMV_15     ((int32)236*BATCELLS/10)                //ָ���ѹ����ܵ�ѹ29.6
#define CONCURCHARGEENDI_15    ((int32)10*BATCAPACITY_15)   //???  1
#define CONCURCHARGEENDV_15    ((int32)215*BATCELLS/10)                //������������ܵ�ѹ����ת��
#define CONCURCHARGEENDCNT_15  ((int32)20*CHARGR_TIME)               //10Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ�� 2
#define SCONCURCHARGECMI_15    ((int32)10*BATCAPACITY_15)              //ָ�����0.10C
#define SCONCURCHARGECMV_15    ((int32)246*BATCELLS/10)                 //ָ���ѹ����ܵ�ѹ29.6
#define SCONCURCHARGEENDV_15   ((int32)235*BATCELLS/10)             //������������ܵ�ѹ29.4
#define SCONCURCHARGEENDCNT_15 ((int32)6*CHARGR_TIME)              //��������ʱ��3H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��  3
#define CONVOLCHARGECMI_15     ((int32)10*BATCAPACITY_15)               //ָ�����0.10C
#define CONVOLCHARGECMV_15     ((int32)245*BATCELLS/10)           //ָ���ѹ����ܵ�ѹ29.4
#define CONVOLCHARGEENDI_15    ((int32)15*BATCAPACITY_15/10)        //��ѹ��������ܵ���0.015C
#define CONVOLCHARGEENDCNT_15  ((int32)2*CHARGR_TIME)                  //��ѹ����ʱ��1H
//������׶γ����Ϣ��������ѹָ���תʱ��4
#define BCONVOLCHARGECMI_15    ((int32)15*BATCAPACITY_15/10)         ////0.015C
#define BCONVOLCHARGECMV_15    ((int32)272*BATCELLS/10)              //// 32.6
#define BCONVOLCHARGEENDV_15   ((int32)270*BATCELLS/10)              ///32.4
#define BCONVOLCHARGEENDCNT_15 ((int32)1*CHARGR_TIME)               ////0.5Hours
//����׶γ����Ϣ��������ѹָ���תʱ��
#define FLOATCHARGECMI_15      ((int32)15*BATCAPACITY_15/10)           //ָ�����0.015C
#define FLOATCHARGECMV_15      ((int32)270*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ32.4
#define FLOATCHARGEENDCNT_15   ((int32)1*CHARGR_TIME)             //Ԥ�����ʱ��0.5H
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_15  ((int32)7*BATCAPACITY_15)                   //ָ�����0.07C
#define EQVoltCMD_15  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_15   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H

//����16:b91 FULLRIVE:300Ah     E# 202 ���� 3-EV-240
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_16        ((int32)900)                  //ָ�����0.003C =9A
#define PRECHARGECMV_16        ((int32)232*BATCELLS/10)              //ָ���ѹ����ܵ�ѹ24.2
#define PRECHARGEENDV_16       ((int32)200*BATCELLS/10)             //Ԥ���������ܵ�ѹ 24
#define PRECHARGEENDCNT_16     ((int32)1*CHARGR_TIME)               //Ԥ�����ʱ��0.5H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ�������� 1
#define CONCURCHARGECMI_16     ((int32)15*BATCAPACITY_16)             //ָ�����0.14C
#define CONCURCHARGECMV_16     ((int32)237*BATCELLS/10)                  //ָ���ѹ����ܵ�ѹ29.6
#define CONCURCHARGEENDI_16    ((int32)3450)              //��������???
#define CONCURCHARGEENDV_16    ((int32)225*BATCELLS/10)             //������������ܵ�ѹ 29.4���ϣ�
#define CONCURCHARGEENDCNT_16  ((int32)14*CHARGR_TIME)               //7Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ�� 2
#define SCONCURCHARGECMI_16    ((int32)3450)              //ָ�����0.14C
#define SCONCURCHARGECMV_16    ((int32)247*BATCELLS/10)                //ָ���ѹ����ܵ�ѹ29.6
#define SCONCURCHARGEENDV_16   ((int32)245*BATCELLS/10)            //������������ܵ�ѹ 29.4
#define SCONCURCHARGEENDCNT_16 ((int32)2*CHARGR_TIME)               //��������ʱ��1H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ�� 3
#define CONVOLCHARGECMI_16     ((int32)14*BATCAPACITY_16)            //ָ�����0.14C 3360
#define CONVOLCHARGECMV_16     ((int32)250*BATCELLS/10)              //ָ���ѹ����ܵ�ѹ 30  250
#define CONVOLCHARGEENDI_16    ((int32)10*BATCAPACITY_16/10)         //��ѹ��������ܵ���0.015C
#define CONVOLCHARGEENDCNT_16  ((int32)8*CHARGR_TIME)               //��ѹ����ʱ��4H
//������׶γ����Ϣ��������ѹָ���תʱ�� 4
#define BCONVOLCHARGECMI_16    ((int32)1*BATCAPACITY_16)         //
#define BCONVOLCHARGECMV_16    ((int32)272*BATCELLS/10)             //33.2
#define BCONVOLCHARGEENDV_16   ((int32)270*BATCELLS/10)             //33
#define BCONVOLCHARGEENDCNT_16 ((int32)8*CHARGR_TIME)               //
//����׶γ����Ϣ��������ѹָ���תʱ��
#define FLOATCHARGECMI_16      ((int32)15*BATCAPACITY_16/10)           //ָ�����0.015C
#define FLOATCHARGECMV_16      ((int32)275*BATCELLS/10)              //ָ���ѹ����ܵ�ѹ 33
#define FLOATCHARGEENDCNT_16   ((int32)1*CHARGR_TIME/1000000)        //Ԥ�����ʱ��1MS
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_16   ((int32)7*BATCAPACITY_16)                   //ָ�����0.07C
#define EQVoltCMD_16   ((int32)272*BATCELLS/10)                    //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_16   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H

//����17:b105 FULLRIVE:115Ah
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_17        ((int32)BATCAPACITY_17)               //ָ�����0.03C
#define PRECHARGECMV_17        ((int32)20*BATCELLS)                 //ָ���ѹ����ܵ�ѹ
#define PRECHARGEENDV_17       ((int32)199*BATCELLS/10)             //Ԥ���������ܵ�ѹ
#define PRECHARGEENDCNT_17     ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��2H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
#define CONCURCHARGECMI_17     ((int32)25*BATCAPACITY_17)              //ָ�����0.12C(��׼0.15-0.35C)
#define CONCURCHARGECMV_17     ((int32)22.5*BATCELLS)                 //ָ���ѹ����ܵ�ѹ////24.5
#define CONCURCHARGEENDI_17    ((int32)22*BATCAPACITY_17)
#define CONCURCHARGEENDV_17    ((int32)225*BATCELLS/10)             //������������ܵ�ѹ28.2V///235
#define CONCURCHARGEENDCNT_17  ((int32)4*CHARGR_TIME)               ////2Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define SCONCURCHARGECMI_17    ((int32)25*BATCAPACITY_17)              //ָ�����0.12C
#define SCONCURCHARGECMV_17    ((int32)241*BATCELLS/10)                 //ָ���ѹ����ܵ�ѹ
#define SCONCURCHARGEENDV_17   ((int32)239*BATCELLS/10)             //������������ܵ�ѹ29.4V
#define SCONCURCHARGEENDCNT_17 ((int32)6*CHARGR_TIME)               //��������ʱ��8H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
#define CONVOLCHARGECMI_17     ((int32)25*BATCAPACITY_17)              //ָ�����0.12C
#define CONVOLCHARGECMV_17     ((int32)240*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ29.4V
#define CONVOLCHARGEENDI_17    ((int32)15*BATCAPACITY_17/10)               //��ѹ��������ܵ���
#define CONVOLCHARGEENDCNT_17  ((int32)16*CHARGR_TIME)               //��ѹ����ʱ��3H
//������׶γ����Ϣ��������ѹָ���תʱ��
#define BCONVOLCHARGECMI_17    ((int32)15*BATCAPACITY_17/10)         //
#define BCONVOLCHARGECMV_17    ((int32)227*BATCELLS/10)             //
#define BCONVOLCHARGEENDV_17   ((int32)260*BATCELLS/10)             // this way ongly use time limiting the charge mode
#define BCONVOLCHARGEENDCNT_17 ((int32)16*CHARGR_TIME)
//����׶γ����Ϣ��������ѹָ���תʱ��
#define FLOATCHARGECMI_17      ((int32)1.5*BATCAPACITY_17)           //ָ�����0.015C
#define FLOATCHARGECMV_17      ((int32)259*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define FLOATCHARGEENDCNT_17   ((int32)CHARGR_TIME)               //Ԥ�����ʱ��0.5H
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_17  ((int32)7*BATCAPACITY_17)                   //ָ�����0.07C
#define EQVoltCMD_17  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_17   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H

//����L18:b106 FULLRIVE:150Ah
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_18        ((int32)BATCAPACITY_18)             //ָ�����0.03C
#define PRECHARGECMV_18        ((int32)20*BATCELLS)                 //ָ���ѹ����ܵ�ѹ
#define PRECHARGEENDV_18       ((int32)199*BATCELLS/10)             //Ԥ���������ܵ�ѹ
#define PRECHARGEENDCNT_18     ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��2H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
#define CONCURCHARGECMI_18     ((int32)20*BATCAPACITY_18)              //ָ�����0.12C
#define CONCURCHARGECMV_18     ((int32)224*BATCELLS/10)              //ָ���ѹ����ܵ�ѹ//234
#define CONCURCHARGEENDI_18    ((int32)24*BATCAPACITY_18)
#define CONCURCHARGEENDV_18    ((int32)220*BATCELLS/10)             //������������ܵ�ѹ//230
#define CONCURCHARGEENDCNT_18    ((int32)4*CHARGR_TIME)            ////5Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define SCONCURCHARGECMI_18    ((int32)20*BATCAPACITY_18)              //ָ�����0.13C
#define SCONCURCHARGECMV_18    ((int32)241*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define SCONCURCHARGEENDV_18   ((int32)239*BATCELLS/10)             //������������ܵ�ѹ
#define SCONCURCHARGEENDCNT_18 ((int32)10*CHARGR_TIME)               //��������ʱ��10H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
#define CONVOLCHARGECMI_18     ((int32)20*BATCAPACITY_18)              //ָ�����0.25C
#define CONVOLCHARGECMV_18     ((int32)240*BATCELLS/10)               //ָ���ѹ����ܵ�ѹ
#define CONVOLCHARGEENDI_18    ((int32)15*BATCAPACITY_18/10)               //��ѹ��������ܵ���
#define CONVOLCHARGEENDCNT_18  ((int32)16*CHARGR_TIME)                 //��ѹ����ʱ��8H
//������׶γ����Ϣ��������ѹָ���תʱ��
#define BCONVOLCHARGECMI_18    ((int32)15*BATCAPACITY_18/10)               //ָ�����0.015C
#define BCONVOLCHARGECMV_18    ((int32)227*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define BCONVOLCHARGEENDV_18   ((int32)260*BATCELLS/10)             //������������ܵ�ѹ
#define BCONVOLCHARGEENDCNT_18 ((int32)16*CHARGR_TIME)               //��ѹ����ʱ��8H
//����׶γ����Ϣ��������ѹָ���תʱ��
#define FLOATCHARGECMI_18      ((int32)2*BATCAPACITY_18)                 //ָ�����0.01C
#define FLOATCHARGECMV_18      ((int32)223*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define FLOATCHARGEENDCNT_18   ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��1H
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_18  ((int32)7*BATCAPACITY_18)                   //ָ�����0.07C
#define EQVoltCMD_18  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_18   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H
/////////////////////////////////���Ĵ�����b201  DT1275  12V  200AH///////////////////////////
//����19:       b201    160Ah
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_19        ((int32)900)             //ָ�����0.03C
#define PRECHARGECMV_19        ((int32)22*BATCELLS)                 //ָ���ѹ����ܵ�ѹ 261
#define PRECHARGEENDV_19       ((int32)200*BATCELLS/10)             //Ԥ���������ܵ�ѹ
#define PRECHARGEENDCNT_19     ((int32)1*CHARGR_TIME)               //Ԥ�����ʱ��2H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ�������� 1
#define CONCURCHARGECMI_19     ((int32)16*BATCAPACITY_19)            //ָ�����0.2C
#define CONCURCHARGECMV_19     ((int32)244*BATCELLS/10)              //ָ���ѹ����ܵ�ѹ 29.2
#define CONCURCHARGEENDI_19    ((int32)15*BATCAPACITY_19)
#define CONCURCHARGEENDV_19    ((int32)235*BATCELLS/10)             //������������ܵ�ѹ28.8
#define CONCURCHARGEENDCNT_19    ((int32)12*CHARGR_TIME)             ////2Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��  2
#define SCONCURCHARGECMI_19    ((int32)12*BATCAPACITY_19)           //ָ�����0.2C   19.2A
#define SCONCURCHARGECMV_19    ((int32)245*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ29.4
#define SCONCURCHARGEENDV_19   ((int32)243*BATCELLS/10)             //������������ܵ�ѹ
#define SCONCURCHARGEENDCNT_19 ((int32)4*CHARGR_TIME)               //��������ʱ��8H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ�� 3
#define CONVOLCHARGECMI_19     ((int32)500)            //ָ�����0.2C
#define CONVOLCHARGECMV_19     ((int32)247*BATCELLS/10)               //ָ���ѹ����ܵ�ѹ
#define CONVOLCHARGEENDI_19    ((int32)300)         //��ѹ��������ܵ���
#define CONVOLCHARGEENDCNT_19  ((int32)4*CHARGR_TIME)               //��ѹ����ʱ��3H
//������׶γ����Ϣ��������ѹָ���תʱ�� 4
#define BCONVOLCHARGECMI_19    ((int32)160)         //
#define BCONVOLCHARGECMV_19    ((int32)268*BATCELLS/10)             //32.16
#define BCONVOLCHARGEENDV_19   ((int32)266*BATCELLS/10)             // 32.04time limiting the charge mode
#define BCONVOLCHARGEENDCNT_19 ((int32)4*CHARGR_TIME)
//����׶γ����Ϣ��������ѹָ���תʱ�� 5
#define FLOATCHARGECMI_19      ((int32)160)                 //ָ�����0.01C
#define FLOATCHARGECMV_19      ((int32)230*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define FLOATCHARGEENDCNT_19   ((int32)8*CHARGR_TIME)               //Ԥ�����ʱ��1H
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_19  ((int32)7*BATCAPACITY_19)                   //ָ�����0.07C
#define EQVoltCMD_19  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_19   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H
/////////////////////////////////���Ĵ�����b201  DT1275  12V  200AH///////////////////////////////////////////

//����19:b107 FULLRIVE:230Ah
////Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
//#define PRECHARGECMI_19        ((int32)BATCAPACITY_19)             //ָ�����0.03C
//#define PRECHARGECMV_19        ((int32)20*BATCELLS)                 //ָ���ѹ����ܵ�ѹ
//#define PRECHARGEENDV_19       ((int32)199*BATCELLS/10)             //Ԥ���������ܵ�ѹ
//#define PRECHARGEENDCNT_19     ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��2H
////�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
//#define CONCURCHARGECMI_19     ((int32)20*BATCAPACITY_19)            //ָ�����0.2C
//#define CONCURCHARGECMV_19     ((int32)224*BATCELLS/10)              //ָ���ѹ����ܵ�ѹ
//#define CONCURCHARGEENDI_19    ((int32)24*BATCAPACITY_19)
//#define CONCURCHARGEENDV_19    ((int32)220*BATCELLS/10)             //������������ܵ�ѹ
//#define CONCURCHARGEENDCNT_19    ((int32)4*CHARGR_TIME)             ////2Сʱ
////���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
//#define SCONCURCHARGECMI_19    ((int32)20*BATCAPACITY_19)           //ָ�����0.2C
//#define SCONCURCHARGECMV_19    ((int32)241*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
//#define SCONCURCHARGEENDV_19   ((int32)239*BATCELLS/10)             //������������ܵ�ѹ
//#define SCONCURCHARGEENDCNT_19 ((int32)20*CHARGR_TIME)               //��������ʱ��8H
////��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
//#define CONVOLCHARGECMI_19     ((int32)20*BATCAPACITY_19)            //ָ�����0.2C
//#define CONVOLCHARGECMV_19     ((int32)24*BATCELLS/10)               //ָ���ѹ����ܵ�ѹ
//#define CONVOLCHARGEENDI_19    ((int32)15*BATCAPACITY_19/10)         //��ѹ��������ܵ���
//#define CONVOLCHARGEENDCNT_19  ((int32)16*CHARGR_TIME)               //��ѹ����ʱ��3H
////������׶γ����Ϣ��������ѹָ���תʱ��
//#define BCONVOLCHARGECMI_19    ((int32)15*BATCAPACITY_19/10)         //
//#define BCONVOLCHARGECMV_19    ((int32)227*BATCELLS/10)             //
//#define BCONVOLCHARGEENDV_19   ((int32)260*BATCELLS/10)             // this way ongly use time limiting the charge mode
//#define BCONVOLCHARGEENDCNT_19 ((int32)16*CHARGR_TIME)
////����׶γ����Ϣ��������ѹָ���תʱ��
//#define FLOATCHARGECMI_19      ((int32)2*BATCAPACITY_19)                 //ָ�����0.01C
//#define FLOATCHARGECMV_19      ((int32)223*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
//#define FLOATCHARGEENDCNT_19   ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��1H
////EQ���׶���Ϣ������ʱ��ָ��
//#define EQCurrCMD_19  ((int32)7*BATCAPACITY_19)                   //ָ�����0.07C
//#define EQVoltCMD_19  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
//#define EQTimeCMD_19   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H
///////
//ԭ������20:b108 FULLRIVE:224Ah      ����������B100  DT1275  12V  200AH
////Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
//#define PRECHARGECMI_20        ((int32)BATCAPACITY_20)               //ָ�����0.03C
//#define PRECHARGECMV_20        ((int32)20*BATCELLS)                 //ָ���ѹ����ܵ�ѹ
//#define PRECHARGEENDV_20       ((int32)199*BATCELLS/10)             //Ԥ���������ܵ�ѹ
//#define PRECHARGEENDCNT_20     ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��2H
////�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
//#define CONCURCHARGECMI_20     ((int32)20*BATCAPACITY_20)              //ָ�����0.12C
//#define CONCURCHARGECMV_20     ((int32)224*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ//234
//#define CONCURCHARGEENDI_20    ((int32)14*BATCAPACITY_20)
//#define CONCURCHARGEENDV_20    ((int32)220*BATCELLS/10)             //������������ܵ�ѹ//230
//#define CONCURCHARGEENDCNT_20    ((int32)4*CHARGR_TIME)            ////5Сʱ
////���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
//#define SCONCURCHARGECMI_20    ((int32)20*BATCAPACITY_20)              //ָ�����0.12C
//#define SCONCURCHARGECMV_20    ((int32)241*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
//#define SCONCURCHARGEENDV_20   ((int32)239*BATCELLS/10)             //������������ܵ�ѹ
//#define SCONCURCHARGEENDCNT_20 ((int32)10*CHARGR_TIME)               //��������ʱ��8H
////��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
//#define CONVOLCHARGECMI_20     ((int32)25*BATCAPACITY_20)              //ָ�����0.12C
//#define CONVOLCHARGECMV_20     ((int32)240*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
//#define CONVOLCHARGEENDI_20    ((int32)15*BATCAPACITY_20/10)               //��ѹ��������ܵ���
//#define CONVOLCHARGEENDCNT_20  ((int32)16*CHARGR_TIME)               //��ѹ����ʱ��3H
////������׶γ����Ϣ��������ѹָ���תʱ��
//#define BCONVOLCHARGECMI_20    ((int32)15*BATCAPACITY_20/10)         //
//#define BCONVOLCHARGECMV_20    ((int32)227*BATCELLS/10)             //
//#define BCONVOLCHARGEENDV_20   ((int32)260*BATCELLS/10)             // this way ongly use time limiting the charge mode
//#define BCONVOLCHARGEENDCNT_20 ((int32)16*CHARGR_TIME)
////����׶γ����Ϣ��������ѹָ���תʱ��
//#define FLOATCHARGECMI_20      ((int32)2*BATCAPACITY_20)                 //ָ�����0.01C
//#define FLOATCHARGECMV_20      ((int32)223*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
//#define FLOATCHARGEENDCNT_20   ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��1H
////EQ���׶���Ϣ������ʱ��ָ��
//#define EQCurrCMD_20  ((int32)7*BATCAPACITY_20)                   //ָ�����0.07C
//#define EQVoltCMD_20  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
//#define EQTimeCMD_20   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H

/////////////////////////////////����������B100  DT1275  12V  200AH////////////////////////////
//����///b100:Leoch DT1275 6V-200Ah
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_20        ((int32)BATCAPACITY_20)               //ָ�����0.05C
#define PRECHARGECMV_20        ((int32)20*BATCELLS)                 //ָ���ѹ����ܵ�ѹ
#define PRECHARGEENDV_20       ((int32)199*BATCELLS/10)             //Ԥ���������ܵ�ѹ23.9V
#define PRECHARGEENDCNT_20     ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��1H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
#define CONCURCHARGECMI_20     ((int32)15*BATCAPACITY_20)            //ָ�����0.15C
#define CONCURCHARGECMV_20     ((int32)23*BATCELLS)                 //ָ���ѹ����ܵ�ѹ//25
#define CONCURCHARGEENDI_20    ((int32)12*BATCAPACITY_20)
#define CONCURCHARGEENDV_20    ((int32)225*BATCELLS/10)             //������������ܵ�ѹ28.64V///245
#define CONCURCHARGEENDCNT_20    ((int32)6*CHARGR_TIME)              ////3Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define SCONCURCHARGECMI_20    ((int32)15*BATCAPACITY_20)            //ָ�����0.15C
#define SCONCURCHARGECMV_20    ((int32)246*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define SCONCURCHARGEENDV_20   ((int32)244*BATCELLS/10)             //������������ܵ�ѹ29.4V
#define SCONCURCHARGEENDCNT_20 ((int32)8*CHARGR_TIME)               //��������ʱ��7H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
#define CONVOLCHARGECMI_20     ((int32)15*BATCAPACITY_20)            //ָ�����0.15C
#define CONVOLCHARGECMV_20     ((int32)245*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ29.4V
#define CONVOLCHARGEENDI_20    ((int32)5*BATCAPACITY_20)            //��ѹ��������ܵ���
#define CONVOLCHARGEENDCNT_20  ((int32)6*CHARGR_TIME)               //��ѹ����ʱ��3H
//������׶γ����Ϣ��������ѹָ���תʱ��
#define BCONVOLCHARGECMI_20    ((int32)5*BATCAPACITY_20)            //ָ�����0.05C
#define BCONVOLCHARGECMV_20    ((int32)268*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define BCONVOLCHARGEENDV_20   ((int32)264*BATCELLS/10)             //�����������ܵ�ѹ32.4V
#define BCONVOLCHARGEENDI_20   ((int32)15*BATCAPACITY_20/10)         //ָ�����0.015C
#define BCONVOLCHARGEENDCNT_20 ((int32)8*CHARGR_TIME)               //�������ʱ��4H
//����׶γ����Ϣ��������ѹָ���תʱ��
#define FLOATCHARGECMI_20      ((int32)BATCAPACITY_20)               //ָ�����0.01C
#define FLOATCHARGECMV_20      ((int32)229*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ27V
#define FLOATCHARGEENDCNT_20   ((int32)CHARGR_TIME)               //�������ʱ��0.5H
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_20  ((int32)7*BATCAPACITY_20)                   //ָ�����0.07C
#define EQVoltCMD_20  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_20   ((int32)4*CHARGR_TIME)                    //Ԥ�����ʱ��6H
/////////////////////////////////����������B100  DT1275  12V  200AH/////////////////////////////////////////

//����21:b109 FULLRIVE:250Ah
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_21        ((int32)BATCAPACITY_21)               //ָ�����0.03C
#define PRECHARGECMV_21       ((int32)20*BATCELLS)                 //ָ���ѹ����ܵ�ѹ
#define PRECHARGEENDV_21       ((int32)199*BATCELLS/10)             //Ԥ���������ܵ�ѹ
#define PRECHARGEENDCNT_21     ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��2H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
#define CONCURCHARGECMI_21     ((int32)20*BATCAPACITY_21)              //ָ�����0.12C
#define CONCURCHARGECMV_21     ((int32)224*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ//234
#define CONCURCHARGEENDI_21    ((int32)14*BATCAPACITY_21)
#define CONCURCHARGEENDV_21    ((int32)220*BATCELLS/10)             //������������ܵ�ѹ//230
#define CONCURCHARGEENDCNT_21    ((int32)4*CHARGR_TIME)            ////5Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define SCONCURCHARGECMI_21    ((int32)20*BATCAPACITY_21)              //ָ�����0.12C
#define SCONCURCHARGECMV_21    ((int32)241*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define SCONCURCHARGEENDV_21   ((int32)239*BATCELLS/10)             //������������ܵ�ѹ
#define SCONCURCHARGEENDCNT_21 ((int32)20*CHARGR_TIME)               //��������ʱ��10H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
#define CONVOLCHARGECMI_21     ((int32)20*BATCAPACITY_21)              //ָ�����0.25C
#define CONVOLCHARGECMV_21     ((int32)240*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define CONVOLCHARGEENDI_21    ((int32)15*BATCAPACITY_21/10)               //��ѹ��������ܵ���
#define CONVOLCHARGEENDCNT_21  ((int32)16*CHARGR_TIME)               //��ѹ����ʱ��8H
//������׶γ����Ϣ��������ѹָ���תʱ��
#define BCONVOLCHARGECMI_21    ((int32)15*BATCAPACITY_21/10)               //ָ�����0.015C
#define BCONVOLCHARGECMV_21    ((int32)227*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define BCONVOLCHARGEENDV_21   ((int32)260*BATCELLS/10)             //������������ܵ�ѹ
#define BCONVOLCHARGEENDCNT_21 ((int32)16*CHARGR_TIME)               //��ѹ����ʱ��8H
//����׶γ����Ϣ��������ѹָ���תʱ��
#define FLOATCHARGECMI_21      ((int32)2*BATCAPACITY_21)                 //ָ�����0.01C
#define FLOATCHARGECMV_21      ((int32)223*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define FLOATCHARGEENDCNT_21   ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��1H
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_21  ((int32)7*BATCAPACITY_21)                   //ָ�����0.07C
#define EQVoltCMD_21  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_21   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H

////����22:b110 :255Ah
////Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
//#define PRECHARGECMI_22        ((int32)BATCAPACITY_22)               //ָ�����0.03C
//#define PRECHARGECMV_22       ((int32)20*BATCELLS)                 //ָ���ѹ����ܵ�ѹ
//#define PRECHARGEENDV_22       ((int32)199*BATCELLS/10)             //Ԥ���������ܵ�ѹ
//#define PRECHARGEENDCNT_22     ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��2H
////�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
//#define CONCURCHARGECMI_22     ((int32)20*BATCAPACITY_22)              //ָ�����0.12C
//#define CONCURCHARGECMV_22     ((int32)224*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ//234
//#define CONCURCHARGEENDI_22    ((int32)14*BATCAPACITY_22)
//#define CONCURCHARGEENDV_22    ((int32)220*BATCELLS/10)             //������������ܵ�ѹ//230
//#define CONCURCHARGEENDCNT_22    ((int32)4*CHARGR_TIME)            ////5Сʱ
////���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
//#define SCONCURCHARGECMI_22    ((int32)20*BATCAPACITY_22)              //ָ�����0.12C
//#define SCONCURCHARGECMV_22    ((int32)241*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
//#define SCONCURCHARGEENDV_22   ((int32)239*BATCELLS/10)             //������������ܵ�ѹ
//#define SCONCURCHARGEENDCNT_22 ((int32)20*CHARGR_TIME)               //��������ʱ��10H
////��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
//#define CONVOLCHARGECMI_22     ((int32)20*BATCAPACITY_22)              //ָ�����0.25C
//#define CONVOLCHARGECMV_22     ((int32)240*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
//#define CONVOLCHARGEENDI_22    ((int32)15*BATCAPACITY_22/10)               //��ѹ��������ܵ���
//#define CONVOLCHARGEENDCNT_22  ((int32)16*CHARGR_TIME)               //��ѹ����ʱ��8H
////������׶γ����Ϣ��������ѹָ���תʱ��
//#define BCONVOLCHARGECMI_22    ((int32)15*BATCAPACITY_22/10)               //ָ�����0.015C
//#define BCONVOLCHARGECMV_22    ((int32)227*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
//#define BCONVOLCHARGEENDV_22   ((int32)260*BATCELLS/10)             //������������ܵ�ѹ
//#define BCONVOLCHARGEENDCNT_22 ((int32)16*CHARGR_TIME)               //��ѹ����ʱ��8H
////����׶γ����Ϣ��������ѹָ���תʱ��
//#define FLOATCHARGECMI_22      ((int32)2*BATCAPACITY_22)                 //ָ�����0.01C
//#define FLOATCHARGECMV_22      ((int32)223*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
//#define FLOATCHARGEENDCNT_22   ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��1H
////EQ���׶���Ϣ������ʱ��ָ��
//#define EQCurrCMD_22  ((int32)7*BATCAPACITY_22)                   //ָ�����0.07C
//#define EQVoltCMD_22  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
//#define EQTimeCMD_22   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H

//����22:b110 :255Ah   ��5�μ�����B111  1.5A  24V
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define PRECHARGECMI_22        ((int32)150)               //ָ�����0.03C  1.5A
#define PRECHARGECMV_22       ((int32)21*BATCELLS)                 //ָ���ѹ����ܵ�ѹ
#define PRECHARGEENDV_22       ((int32)200*BATCELLS/10)             //Ԥ���������ܵ�ѹ
#define PRECHARGEENDCNT_22     ((int32)24*CHARGR_TIME)               //Ԥ�����ʱ��2H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
#define CONCURCHARGECMI_22     ((int32)20*BATCAPACITY_22)              //ָ�����0.12C
#define CONCURCHARGECMV_22     ((int32)210*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ//234
#define CONCURCHARGEENDI_22    ((int32)14*BATCAPACITY_22)
#define CONCURCHARGEENDV_22    ((int32)190*BATCELLS/10)             //������������ܵ�ѹ//230
#define CONCURCHARGEENDCNT_22    ((int32)4*CHARGR_TIME)            ////5Сʱ
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
#define SCONCURCHARGECMI_22    ((int32)150)              //ָ�����0.12C
#define SCONCURCHARGECMV_22    ((int32)210*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define SCONCURCHARGEENDV_22   ((int32)150)             //������������ܵ�ѹ
#define SCONCURCHARGEENDCNT_22 ((int32)20*CHARGR_TIME)               //��������ʱ��10H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
#define CONVOLCHARGECMI_22     ((int32)150)              //ָ�����0.25C
#define CONVOLCHARGECMV_22     ((int32)210*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define CONVOLCHARGEENDI_22    ((int32)150)               //��ѹ��������ܵ���
#define CONVOLCHARGEENDCNT_22  ((int32)16*CHARGR_TIME)               //��ѹ����ʱ��8H
//������׶γ����Ϣ��������ѹָ���תʱ��
#define BCONVOLCHARGECMI_22    ((int32)150)               //ָ�����0.015C
#define BCONVOLCHARGECMV_22    ((int32)210*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define BCONVOLCHARGEENDV_22   ((int32)200*BATCELLS/10)             //������������ܵ�ѹ
#define BCONVOLCHARGEENDCNT_22 ((int32)16*CHARGR_TIME)               //��ѹ����ʱ��8H
//����׶γ����Ϣ��������ѹָ���תʱ��
#define FLOATCHARGECMI_22      ((int32)2*BATCAPACITY_22)                 //ָ�����0.01C
#define FLOATCHARGECMV_22      ((int32)200*BATCELLS/10)             //ָ���ѹ����ܵ�ѹ
#define FLOATCHARGEENDCNT_22   ((int32)2*CHARGR_TIME)               //Ԥ�����ʱ��1H
//EQ���׶���Ϣ������ʱ��ָ��
#define EQCurrCMD_22  ((int32)7*BATCAPACITY_22)                   //ָ�����0.07C
#define EQVoltCMD_22  ((int32)272*BATCELLS/10)                   //ָ���ѹ����ܵ�ѹ
#define EQTimeCMD_22   ((int32)12*CHARGR_TIME)                    //Ԥ�����ʱ��1H
/*******************************************************************************
**                               ��ؽṹ��
*******************************************************************************/





/*******************************************************************************
**                               ��������
*******************************************************************************/

/*******************************************************************************
**                               ��������
*******************************************************************************/

#endif /* LLCCURVE_H_ */

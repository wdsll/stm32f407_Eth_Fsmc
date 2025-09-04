
/*
    Ӧ��˵�������ʴ���EEPROMǰ�����ȵ���һ�� bsp_InitI2C()�������ú�I2C��ص�GPIO.
*/

#include "eeprom_24xx.h"

Uint16 ReadBuf[100];
/*
*********************************************************************************************************
*   �� �� ��: ee_ReadByte
*   ����˵��: �Ӵ���EEPROMָ����ַ����ʼ��ȡ��������
*   ��    ��:  _usAddress : ��ʼ��ַ,�ֵ�ַ��
*            _usSize : ���ݳ��ȣ���λΪ��
*            _pReadBuf : ��Ŷ��������ݵĻ�����ָ��
*   �� �� ֵ: 0 ��ʾʧ�ܣ�1��ʾ�ɹ�
*********************************************************************************************************
*/

Uint16 ee_ReadByte(Uint8 *_pReadBuf, Uint16 _usAddress, Uint16 _usSize)
{

    Uint16 i;
    Uint16 usAddr;
    /* ���ô���EEPROM�漴��ȡָ�����У�������ȡ�����ֽ� */
    usAddr = _usAddress;
    /* ��1��������I2C���������ź� */
    i2c_Start();

    /* ��2������������ֽڣ���7bit�ǵ�ַ��bit0�Ƕ�д����λ��0��ʾд��1��ʾ�� */
    i2c_SendByte(EE_DEV_ADDR | I2C_WR); /* �˴���дָ�� */

    /* ��3��������ACK */
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;  /* EEPROM������Ӧ�� */
    }

    /* ��4���������ֽڵ�ַ��24C02ֻ��256�ֽڣ����1���ֽھ͹��ˣ������24C04���ϣ���ô�˴���Ҫ���������ַ */
    if (EE_ADDR_BYTES == 1)
    {
        i2c_SendByte((Uint16)_usAddress);
        if (i2c_WaitAck() != 0)
        {
            goto cmd_fail;  /* EEPROM������Ӧ�� */
        }
    }
    else
    {
        i2c_SendByte(usAddr >> 8);
        if (i2c_WaitAck() != 0)
        {
            goto cmd_fail;  /* EEPROM������Ӧ�� */
        }

        i2c_SendByte(usAddr);
        if (i2c_WaitAck() != 0)
        {
            goto cmd_fail;  /* EEPROM������Ӧ�� */
        }
    }

    /* ��6������������I2C���ߡ����濪ʼ��ȡ���� */
    i2c_Start();

    /* ��7������������ֽڣ���7bit�ǵ�ַ��bit0�Ƕ�д����λ��0��ʾд��1��ʾ�� */
    i2c_SendByte(EE_DEV_ADDR | I2C_RD); /* �˴��Ƕ�ָ�� */

    /* ��8��������ACK */
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;  /* EEPROM������Ӧ�� */
    }
    /* ��9����ѭ����ȡ���� */
    for (i = 0; i < _usSize; i++)
    {
        _pReadBuf[i] = i2c_ReadByte() & 0x00ff;/* ��1���ֽ� */

        /* ÿ����1���ֽں���Ҫ����Ack�� ���һ���ֽڲ���ҪAck����Nack */
        if (i != (_usSize - 1))
        {
            i2c_Ack();  /* �м��ֽڶ����CPU����ACK�ź�(����SDA = 0) */
        }
        else
        {
            i2c_NAck(); /* ���1���ֽڶ����CPU����NACK�ź�(����SDA = 1) */
        }
    }
    /* ����I2C����ֹͣ�ź� */
    i2c_Stop();
    return 1;   /* ִ�гɹ� */

cmd_fail: /* ����ִ��ʧ�ܺ��мǷ���ֹͣ�źţ�����Ӱ��I2C�����������豸 */
    /* ����I2C����ֹͣ�ź� */
    i2c_Stop();
    return 0;
}
/*
*********************************************************************************************************
*   �� �� ��: ee_ReadWord
*   ����˵��: �Ӵ���EEPROMָ����ַ����ʼ��ȡ��������
*   ��    ��:  _usAddress : ��ʼ��ַ,�ֽڵ�ַ��
*            _usSize : ���ݳ��ȣ���λΪ�ֽ�
*            _pReadBuf : ��Ŷ��������ݵĻ�����ָ��
*   �� �� ֵ: 0 ��ʾʧ�ܣ�1��ʾ�ɹ�
*********************************************************************************************************
*/
Uint16 ee_ReadWord(Uint16 *_pReadBuf, Uint16 _usAddress, Uint16 _usSize)
{
    Uint16 i;
    Uint16 usAddr;
    if(_usSize % 2) return 0;//�ֽ������������ֶ���
    /* ���ô���EEPROM�漴��ȡָ�����У�������ȡ�����ֽ� */
    usAddr = _usAddress;
    /* ��1��������I2C���������ź� */
    i2c_Start();

    /* ��2������������ֽڣ���7bit�ǵ�ַ��bit0�Ƕ�д����λ��0��ʾд��1��ʾ�� */
    i2c_SendByte(EE_DEV_ADDR | I2C_WR); /* �˴���дָ�� */

    /* ��3��������ACK */
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;  /* EEPROM������Ӧ�� */
    }

    /* ��4���������ֽڵ�ַ��24C02ֻ��256�ֽڣ����1���ֽھ͹��ˣ������24C04���ϣ���ô�˴���Ҫ���������ַ */
    if (EE_ADDR_BYTES == 1)
    {
        i2c_SendByte((Uint16)_usAddress);
        if (i2c_WaitAck() != 0)
        {
            goto cmd_fail;  /* EEPROM������Ӧ�� */
        }
    }
    else
    {
        i2c_SendByte(usAddr >> 8);
        if (i2c_WaitAck() != 0)
        {
            goto cmd_fail;  /* EEPROM������Ӧ�� */
        }

        i2c_SendByte(usAddr);
        if (i2c_WaitAck() != 0)
        {
            goto cmd_fail;  /* EEPROM������Ӧ�� */
        }
    }

    /* ��6������������I2C���ߡ����濪ʼ��ȡ���� */
    i2c_Start();

    /* ��7������������ֽڣ���7bit�ǵ�ַ��bit0�Ƕ�д����λ��0��ʾд��1��ʾ�� */
    i2c_SendByte(EE_DEV_ADDR | I2C_RD); /* �˴��Ƕ�ָ�� */

    /* ��8��������ACK */
    if (i2c_WaitAck() != 0)
    {
        goto cmd_fail;  /* EEPROM������Ӧ�� */
    }
    /* ��9����ѭ����ȡ���� */
    for (i = 0; i < (_usSize>>1); i++)
    {
        _pReadBuf[i] = i2c_ReadByte() & 0x00ff;/* ��1���ֽ� */

        i2c_Ack();  /* �м��ֽڶ����CPU����ACK�ź�(����SDA = 0) */

        _pReadBuf[i] |= ((i2c_ReadByte() & 0x00ff) << 8);  /* ��1���ֽ� */

        /* ÿ����1���ֽں���Ҫ����Ack�� ���һ���ֽڲ���ҪAck����Nack */
        if (i != ((_usSize>>1) - 1))
        {
            i2c_Ack();  /* �м��ֽڶ����CPU����ACK�ź�(����SDA = 0) */
        }
        else
        {
            i2c_NAck(); /* ���1���ֽڶ����CPU����NACK�ź�(����SDA = 1) */
        }
    }
    /* ����I2C����ֹͣ�ź� */
    i2c_Stop();
    return 1;   /* ִ�гɹ� */

cmd_fail: /* ����ִ��ʧ�ܺ��мǷ���ֹͣ�źţ�����Ӱ��I2C�����������豸 */
    /* ����I2C����ֹͣ�ź� */
    i2c_Stop();
    return 0;
}

/*
*********************************************************************************************************
*   �� �� ��: ee_WriteWord
*   ����˵��: ����EEPROMָ����ַд���������ݣ�����ҳд�������д��Ч��
*   ��    ��:  _usAddress : ��ʼ��ַ(�ֽ�)
*            _usSize : ���ݳ��ȣ���λΪ�ֽ�
*            _pWriteBuf : ��Ŷ��������ݵĻ�����ָ��
*   �� �� ֵ: 0 ��ʾʧ�ܣ�1��ʾ�ɹ�
*********************************************************************************************************
*/
Uint16 ee_WriteWord(Uint16 *_pWriteBuf, Uint16 _usAddress, Uint16 _usSize)
{
    Uint16 i,j,m;
    Uint16 usAddr;
    Uint16 ReadAdr;
    Uint16 ReadLen;
    Uint16 ReadSum;
    WriteEnable();

    /*
        д����EEPROM�������������������ȡ�ܶ��ֽڣ�ÿ��д����ֻ����ͬһ��page��
        ����24xx02��page size = 8
        �򵥵Ĵ�����Ϊ�����ֽ�д����ģʽ��ÿд1���ֽڣ������͵�ַ
        Ϊ���������д��Ч��: ����������page wirte������
    */
    if(_usSize % 2) return 0;//�ֽ������������ֶ���
    usAddr = _usAddress;
    ReadSum = _usSize;
    ReadAdr = _usAddress;
    j = 0;
    for (i = 0; i < _usSize; i++)
    {
        /* �����͵�1���ֽڻ���ҳ���׵�ַʱ����Ҫ���·��������źź͵�ַ */
        if ((i == 0) || (usAddr & (EE_PAGE_SIZE - 1)) == 0)
        {
            /*���ڣ�������ֹͣ�źţ������ڲ�д������*/
            i2c_Stop();

            /* ͨ���������Ӧ��ķ�ʽ���ж��ڲ�д�����Ƿ����, һ��С�� 10ms
                CLKƵ��Ϊ200KHzʱ����ѯ����Ϊ30������
            */
            for (m = 0; m < 1000; m++)
            {
                /* ��1��������I2C���������ź� */
                i2c_Start();

                /* ��2������������ֽڣ���7bit�ǵ�ַ��bit0�Ƕ�д����λ��0��ʾд��1��ʾ�� */
                i2c_SendByte(EE_DEV_ADDR | I2C_WR); /* �˴���дָ�� */

                /* ��3��������һ��ʱ�ӣ��ж������Ƿ���ȷӦ�� */
                if (i2c_WaitAck() == 0)
                {
                    break;
                }
            }
            if (m  == 1000)
            {
                goto cmd_fail;  /* EEPROM����д��ʱ */
            }

            /* ��4���������ֽڵ�ַ��24C02ֻ��256�ֽڣ����1���ֽھ͹��ˣ������24C04���ϣ���ô�˴���Ҫ���������ַ */
            if (EE_ADDR_BYTES == 1)
            {
                i2c_SendByte((Uint16)usAddr);
                if (i2c_WaitAck() != 0)
                {
                    goto cmd_fail;  /* EEPROM������Ӧ�� */
                }
            }
            else
            {
                i2c_SendByte(usAddr >> 8);
                if (i2c_WaitAck() != 0)
                {
                    goto cmd_fail;  /* EEPROM������Ӧ�� */
                }

                i2c_SendByte(usAddr);
                if (i2c_WaitAck() != 0)
                {
                    goto cmd_fail;  /* EEPROM������Ӧ�� */
                }
            }
        }
        /* ��6������ʼд������ */
        if((i % 2)==0)
            i2c_SendByte(_pWriteBuf[j]&0x00ff);
        else
            i2c_SendByte((_pWriteBuf[j++]>>8)&0x00ff);
        /* ��7��������ACK */
        if (i2c_WaitAck() != 0)
        {
            goto cmd_fail;  /* EEPROM������Ӧ�� */
        }
        usAddr+=1;  /* ��ַ��1 */
    }

    /* ����ִ�гɹ�������I2C����ֹͣ�ź� */
    i2c_Stop();
    WriteDisable();
    ReadSum = _usSize;
    ReadAdr = _usAddress;
    while(ReadSum)
    {
        if(ReadSum>100)
        {
            ReadLen=100;
            ReadSum = ReadSum-100;
        }
        else
        {
            ReadSum = 0;
            ReadLen=ReadSum;
        }
        if(ee_ReadWord(ReadBuf,ReadAdr, ReadLen)==1)
        {
            for(i=0;i<(ReadLen>>1);i++)
            {
                if(_pWriteBuf[i]!=ReadBuf[i])
                {
                    goto cmd_fail;  /* EEPROM������Ӧ�� */
                }
            }
            _pWriteBuf = _pWriteBuf + (ReadLen>>1);
            ReadAdr = ReadAdr + ReadLen;
        }
        else
        {
            goto cmd_fail;  /* EEPROM������Ӧ�� */
        }
    }

    return 1;

cmd_fail: /* ����ִ��ʧ�ܺ��мǷ���ֹͣ�źţ�����Ӱ��I2C�����������豸 */
    /* ����I2C����ֹͣ�ź� */
    i2c_Stop();
    WriteDisable();
    return 0;
}

/*
*********************************************************************************************************
*   �� �� ��: ee_WriteByte
*   ����˵��: ����EEPROMָ����ַд���������ݣ�����ҳд�������д��Ч��
*   ��    ��:  _usAddress : ��ʼ��ַ(�ֽ�)
*            _usSize : ���ݳ��ȣ���λΪ�ֽ�
*            _pWriteBuf : ��Ŷ��������ݵĻ�����ָ��
*   �� �� ֵ: 0 ��ʾʧ�ܣ�1��ʾ�ɹ�
*********************************************************************************************************
*/
Uint16 ee_WriteByte(Uint8 *_pWriteBuf, Uint16 _usAddress, Uint16 _usSize)
{
    Uint16 i,m;
    Uint16 usAddr;
    Uint16 ReadAdr;
    Uint16 ReadLen;
    Uint16 ReadSum;
    WriteEnable();

    /*
        д����EEPROM�������������������ȡ�ܶ��ֽڣ�ÿ��д����ֻ����ͬһ��page��
        ����24xx02��page size = 8
        �򵥵Ĵ�����Ϊ�����ֽ�д����ģʽ��ÿд1���ֽڣ������͵�ַ
        Ϊ���������д��Ч��: ����������page wirte������
    */

    usAddr = _usAddress;
    ReadSum = _usSize;
    ReadAdr = _usAddress;
    for (i = 0; i < _usSize; i++)
    {
        /* �����͵�1���ֽڻ���ҳ���׵�ַʱ����Ҫ���·��������źź͵�ַ */
        if ((i == 0) || (usAddr & (EE_PAGE_SIZE - 1)) == 0)
        {
            /*���ڣ�������ֹͣ�źţ������ڲ�д������*/
            i2c_Stop();

            /* ͨ���������Ӧ��ķ�ʽ���ж��ڲ�д�����Ƿ����, һ��С�� 10ms
                CLKƵ��Ϊ200KHzʱ����ѯ����Ϊ30������
            */
            for (m = 0; m < 1000; m++)
            {
                /* ��1��������I2C���������ź� */
                i2c_Start();

                /* ��2������������ֽڣ���7bit�ǵ�ַ��bit0�Ƕ�д����λ��0��ʾд��1��ʾ�� */
                i2c_SendByte(EE_DEV_ADDR | I2C_WR); /* �˴���дָ�� */

                /* ��3��������һ��ʱ�ӣ��ж������Ƿ���ȷӦ�� */
                if (i2c_WaitAck() == 0)
                {
                    break;
                }
            }
            if (m  == 1000)
            {
                goto cmd_fail;  /* EEPROM����д��ʱ */
            }

            /* ��4���������ֽڵ�ַ��24C02ֻ��256�ֽڣ����1���ֽھ͹��ˣ������24C04���ϣ���ô�˴���Ҫ���������ַ */
            if (EE_ADDR_BYTES == 1)
            {
                i2c_SendByte((Uint16)usAddr);
                if (i2c_WaitAck() != 0)
                {
                    goto cmd_fail;  /* EEPROM������Ӧ�� */
                }
            }
            else
            {
                i2c_SendByte(usAddr >> 8);
                if (i2c_WaitAck() != 0)
                {
                    goto cmd_fail;  /* EEPROM������Ӧ�� */
                }

                i2c_SendByte(usAddr);
                if (i2c_WaitAck() != 0)
                {
                    goto cmd_fail;  /* EEPROM������Ӧ�� */
                }
            }
        }

        /* ��6������ʼд������ */
        i2c_SendByte(_pWriteBuf[i]&0x00ff);

        /* ��7��������ACK */
        if (i2c_WaitAck() != 0)
        {
            goto cmd_fail;  /* EEPROM������Ӧ�� */
        }
        usAddr+=1;  /* ��ַ��2 */
    }

    /* ����ִ�гɹ�������I2C����ֹͣ�ź� */
    i2c_Stop();
    WriteDisable();
    ReadSum = _usSize;
    ReadAdr = _usAddress;
    while(ReadSum)
    {
        if(ReadSum>100)
        {
            ReadLen=100;
            ReadSum = ReadSum-100;
        }
        else
        {
            ReadSum = 0;
            ReadLen=ReadSum;
        }
        if(ee_ReadByte((Uint8 *)ReadBuf,ReadAdr, ReadLen)==1)
        {
            for(i=0;i<ReadLen;i++)
            {
                if((_pWriteBuf[i] & 0x00ff)!= ReadBuf[i])
                {
                    goto cmd_fail;  /* EEPROM������Ӧ�� */
                }
            }
            _pWriteBuf = _pWriteBuf + ReadLen;
            ReadAdr = ReadAdr + ReadLen;
        }
        else
        {
            goto cmd_fail;  /* EEPROM������Ӧ�� */
        }
    }

    return 1;

cmd_fail: /* ����ִ��ʧ�ܺ��мǷ���ֹͣ�źţ�����Ӱ��I2C�����������豸 */
    /* ����I2C����ֹͣ�ź� */
    i2c_Stop();
    WriteDisable();
    return 0;
}

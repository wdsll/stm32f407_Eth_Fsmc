/* ATSAMD21G18A minimal application. Derived framework: Microchip Harmony 3.
 * Internal open-loop DFLL ~48 MHz; no external crystal required.
 * SysTick is polled, so do not add long blocking tasks here.
 */
#include <stdint.h>
#include <stddef.h>
#include "definitions.h"
#include "board.h"


static uint8_t s_tx_message[] = "ATSAMD21G18A UART OK\r\n";

volatile bool g_uart_started = false;
volatile bool g_uart_done = false;
/* Watch 窗口观察这些变量 */
volatile bool g_i2c_test_started = false;
volatile bool g_i2c_test_done = false;
volatile bool g_i2c_test_ok = false;
volatile uint32_t g_i2c_test_error = 0U;

/*
 * 示例数据：
 * 必须按实际从设备协议调整。
 * 若连接另一个 MCU 作为测试从机，可以直接接收这四字节。
 */
static uint8_t s_i2c_test_tx[] = {
    0x55U, 0xAAU, 0x01U, 0x02U
};

/* I2C 传输完成或发生错误时，由驱动在中断中调用 */
static void I2C_TestCallback(uintptr_t context)
{
    (void)context;

    g_i2c_test_error = (uint32_t)SERCOM1_I2C_ErrorGet();

    g_i2c_test_ok =
        (g_i2c_test_error == (uint32_t)SERCOM_I2C_ERROR_NONE);

    /* 最后置完成标志 */
    g_i2c_test_done = true;
}

/*
 * 功能：向指定的 7 位从机地址提交一次发送请求。
 * 参数：slave_addr 为未左移的地址，例如 0x50，而不是 0xA0。
 * 返回：true 表示请求被接受，最终结果看回调变量。
 *
 * 注意：发送期间不可修改 s_i2c_test_tx。
 */
static bool I2C_MasterSendTest(uint16_t slave_addr)
{
	if((slave_addr<0x08U)||(slave_addr > 0x77U) ||SERCOM1_I2C_IsBusy())
	{
		return false;
	}
	g_i2c_test_started = false;
	g_i2c_test_done = false;
	g_i2c_test_ok = false;
	g_i2c_test_error = 0U;
	
	SERCOM1_I2C_CallbackRegister(I2C_TestCallback, 0U);

	g_i2c_test_started = SERCOM1_I2C_Write(
			slave_addr,
			s_i2c_test_tx,
			sizeof(s_i2c_test_tx)
	);
	return g_i2c_test_started;
}
int main(void)
{
    uint32_t elapsed = 0U;
    SYS_Initialize(NULL);
/*
 * 0x50 仅为示例，必须换成实际从机的 7 位地址。
 * 若连接 EEPROM、传感器等，应先按其协议修改发送数据。
 */
		(void)I2C_MasterSendTest(0x50U);
    g_uart_started = SERCOM0_USART_Write(s_tx_message,sizeof(s_tx_message) - 1U);
    if (g_uart_started)
    {
        while (SERCOM0_USART_WriteIsBusy())
        {
        }
        g_uart_done = true;
    }
    while (true)
    {

    }
}

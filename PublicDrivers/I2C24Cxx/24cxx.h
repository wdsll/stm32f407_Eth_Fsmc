#ifndef __24CXX_H
#define __24CXX_H

#include "main.h"
#include "i2c.h"

#define I2C_HANDLE		hi2c1
#define DEV_ADDR_24CXX	0x00A0

#define PAGE_SIZE_24CXX	0x0008
#define MEM_SIZE_24CXX	(uint16_t)256

HAL_StatusTypeDef EP24C_IsDeviceReady(void);
HAL_StatusTypeDef EP24C_WriteOneByte(uint16_t memAddress, uint8_t byteData);
HAL_StatusTypeDef EP24C_ReadOneByte(uint16_t memAddress, uint8_t *byteData);
HAL_StatusTypeDef EP24C_ReadBytes(uint16_t memAddress, uint8_t *pBuffer, uint16_t bufferLen);
HAL_StatusTypeDef EP24C_WriteInOnePage(uint16_t memAddress, uint8_t *pBuffer, uint16_t bufferLen);
HAL_StatusTypeDef EP24C_WriteLongData(uint16_t memAddress, uint8_t *pBuffer, uint16_t bufferLen);

#endif //__24CXX_H

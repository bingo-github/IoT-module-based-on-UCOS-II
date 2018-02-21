#ifndef	__CRC16_H_
#define __CRC16_H_
#include "stm32f10x.h"

uint16_t crc16(uint8_t *puchMsg, uint16_t usDataLen);
uint8_t check_crc16(uint8_t* buffer, uint16_t bufferLen, uint8_t CRC16H, uint8_t CRC16L);

#endif

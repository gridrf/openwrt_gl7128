#ifndef __ILORACHIP_H__
#define __ILORACHIP_H__

#include <stdint.h>
#include "board.h"

class ILoRaChip
{
public:
	virtual void Sleep() = 0;
	virtual void Send(uint8_t *buffer, uint8_t size) = 0;
	virtual void Rx(uint32_t timeout) = 0;
	virtual void SetRfTxPower(int8_t power) = 0;
	virtual void SetChannel(uint32_t freq) = 0;
};

#endif

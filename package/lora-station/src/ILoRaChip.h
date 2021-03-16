#ifndef __ILORACHIP_H__
#define __ILORACHIP_H__

#include <stdint.h>
#include "board.h"
#include "sx1276Regs-Fsk.h"
#include "sx1276Regs-LoRa.h"

class ILoRaChip
{
public:
	virtual void Init() = 0;
	virtual void Sleep() = 0;
	virtual void SetPublicNetwork(bool enable) = 0;
	virtual void Send(uint8_t *buffer, uint8_t size) = 0;
	virtual void Rx(uint32_t timeout) = 0;
	virtual void SetRfTxPower(int8_t power) = 0;
	virtual void SetChannel(uint32_t freq) = 0;
	virtual void SetTxConfig(RadioModems_t modem, int8_t power, uint32_t fdev,
		uint32_t bandwidth, uint32_t datarate,
		uint8_t coderate, uint16_t preambleLen,
		bool fixLen, bool crcOn, bool freqHopOn,
		uint8_t hopPeriod, bool iqInverted, uint32_t timeout)= 0;

	virtual void SetRxConfig(RadioModems_t modem, uint32_t bandwidth,
		uint32_t datarate, uint8_t coderate,
		uint32_t bandwidthAfc, uint16_t preambleLen,
		uint16_t symbTimeout, bool fixLen,
		uint8_t payloadLen,
		bool crcOn, bool freqHopOn, uint8_t hopPeriod,
		bool iqInverted, bool rxContinuous) = 0;
	virtual void SetMaxPayloadLength(RadioModems_t modem, uint8_t max) = 0;
};

#endif

#ifndef __RADIO_EVENT_H__
#define __RADIO_EVENT_H__

#include <stdint.h>

class RadioEvent
{
public:
	virtual void OnTxDone(void) = 0;
	virtual void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) = 0;
	virtual void OnTxTimeout(void) = 0;
	virtual void OnRxTimeout(void) = 0;
	virtual void OnRxError(void) = 0;
	virtual void OnFhssChangeChannel(uint8_t currentChannel) = 0;
	virtual void OnCadDone(bool channelActivityDetected) = 0;
};

#endif

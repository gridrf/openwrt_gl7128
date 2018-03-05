#ifndef __RADIO_H__
#define __RADIO_H__

#include "board.h"
#include "RadioEvent.h"
#include "ILoRaChip.h"
#include "MessagerHandler.h"

#define BUFFER_SIZE                                 64 // Define the payload size here
#define RX_TIMEOUT_VALUE                            1000

typedef enum
{
	LOWPOWER,
	RX,
	RX_TIMEOUT,
	RX_ERROR,
	TX,
	TX_TIMEOUT,
}States_t;

class Radio:public RadioEvent
{
private:
	States_t State;
	LoRa_Config *_conf;

	int16_t RssiValue;
	int8_t SnrValue;

	uint16_t BufferSize;
	uint8_t Buffer[BUFFER_SIZE];
	ILoRaChip *_chip;

public:
	Radio(LoRa_Config *conf);
	~Radio();

	void Init(ILoRaChip *chip);

	void OnTxDone(void);
	void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
	void OnTxTimeout(void);
	void OnRxTimeout(void);
	void OnRxError(void);
	void OnFhssChangeChannel(uint8_t currentChannel);
	void OnCadDone(bool channelActivityDetected);

	void Process(MessagerHandler *handler);
};

#endif

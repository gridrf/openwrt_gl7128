/* Copyright (C) 2018  GridRF Radio Team(tech@gridrf.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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

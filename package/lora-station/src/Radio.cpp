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
#include "Radio.h"
#include <string.h>
#include "board.h"
#include <stdio.h>


Radio::Radio(LoRa_Config *conf)
:_conf(conf)
{
	State = LOWPOWER;
	RssiValue = 0;
	SnrValue = 0;
	BufferSize = 0;
}

Radio::~Radio()
{
}


void Radio::Init(ILoRaChip *chip)
{
	_chip = chip;
}

void Radio::OnTxDone(void)
{
	_chip->Sleep();
	State = TX;
}

void Radio::OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
	//printf("OnRxDone\n");
	_chip->Sleep();
	BufferSize = size;
	memcpy(Buffer, payload, BufferSize);
	RssiValue = rssi;
	SnrValue = snr;
	State = RX;
}

void Radio::OnTxTimeout(void)
{
	_chip->Sleep();
	State = TX_TIMEOUT;
}

void Radio::OnRxTimeout(void)
{
	printf("OnRxTimeout\n");
	_chip->Sleep();
	State = RX_TIMEOUT;
}

void Radio::OnRxError(void)
{
	printf("OnRxError\n");
	_chip->Sleep();
	State = RX_ERROR;
}

void Radio::OnFhssChangeChannel(uint8_t currentChannel)
{
	printf("OnFhssChangeChannel\n");
}

void Radio::OnCadDone(bool channelActivityDetected)
{
	printf("OnCadDone\n");
}

void Radio::Process(MessagerHandler *handler)
{
	switch (State)
	{
	case RX:
		if (BufferSize > 0)
		{
		    handler->OnPacket(Buffer, BufferSize, RssiValue, SnrValue);
		}
		_chip->Rx(0);
		State = LOWPOWER;
		break;
	case TX:
		{
			if(_conf->tx_freq != _conf->frequency){
				_conf->tx_freq = _conf->frequency;				
				_chip->SetChannel(_conf->frequency);
			}
			_chip->Rx(0);
			State = LOWPOWER;
			break;
		}
	case RX_TIMEOUT:
	case RX_ERROR:
		_chip->Rx(0);
		State = LOWPOWER;
		break;
	case TX_TIMEOUT:
		_chip->Rx(0);
		State = LOWPOWER;
		break;
	case LOWPOWER:
	default:
		// Set low power
		break;
	}
}


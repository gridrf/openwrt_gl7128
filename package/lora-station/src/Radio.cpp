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

void Radio::chipReset(void)
{
	_chip->Init();
	_chip->SetChannel(_conf->frequency + _conf->ppm);

	if(_conf->is_public_network){
		_chip->SetPublicNetwork( true );
	}

	if (_conf->modem == 0) {
		_chip->SetTxConfig(MODEM_LORA, _conf->power, 0, _conf->bandwidth,
			_conf->spreading_factor, _conf->codingrate,
			_conf->preamble_length, LORA_FIX_LENGTH_PAYLOAD_ON,
			true, 0, 0, _conf->tx_iqInverted, 3000);

		_chip->SetRxConfig(MODEM_LORA, _conf->bandwidth, _conf->spreading_factor,
			_conf->codingrate, 0, _conf->preamble_length,
			LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
			0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

		
	  	_chip->SetMaxPayloadLength(MODEM_LORA, BUFFER_SIZE);
	}
	else {
		_chip->SetTxConfig(MODEM_FSK, _conf->power, _conf->fdev, 0,
			_conf->datarate, 0,
			_conf->preamble_length, FSK_FIX_LENGTH_PAYLOAD_ON,
			true, 0, 0, 0, 3000);

		_chip->SetRxConfig(MODEM_FSK, _conf->fsk_bandwidth, _conf->datarate, 0,
			_conf->afc_bandwidth, _conf->preamble_length,
			0, FSK_FIX_LENGTH_PAYLOAD_ON, 0, true,
			0, 0, false, false);
	  	_chip->SetMaxPayloadLength(MODEM_FSK, BUFFER_SIZE);
	}
	_chip->Rx(0);
}

void Radio::OnTxDone(void)
{
	printf("OnTxDone\n");
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
	printf("OnTxTimeout\n");
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
				_chip->SetChannel(_conf->frequency+_conf->ppm);
			}
			_chip->Rx(0);
			State = LOWPOWER;
			break;
		}
	case TX_TIMEOUT:
	case RX_TIMEOUT:
	case RX_ERROR:
		{
			this->chipReset();
			State = LOWPOWER;
			break;
		}
	case LOWPOWER:
	default:
		// Set low power
		break;
	}
}




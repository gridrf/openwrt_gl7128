#ifndef __RADIO_H__
#define __RADIO_H__

#include "board.h"
#include "RadioEvent.h"
#include "ILoRaChip.h"
#include "MessagerHandler.h"
#include "IRadio.h"

#define BUFFER_SIZE                                 255 // Define the payload size here
#define RX_TIMEOUT_VALUE                            1000


#define RF_FREQUENCY                                433175000 // Hz
#define RF_PPM_OFFSET                               -20000 // Hz
#define TX_OUTPUT_POWER                             0         // dBm
#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
//  1: 250 kHz,
//  2: 500 kHz,
//  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
//  2: 4/6,
//  3: 4/7,
//  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         5         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

#define FSK_FDEV                                    25000     // Hz
#define FSK_DATARATE                                50000     // bps
#define FSK_BANDWIDTH                               50000     // Hz
#define FSK_AFC_BANDWIDTH                           83333     // Hz
#define FSK_PREAMBLE_LENGTH                         5         // Same for Tx and Rx
#define FSK_FIX_LENGTH_PAYLOAD_ON                   false


typedef enum
{
	LOWPOWER,
	RX,
	RX_TIMEOUT,
	RX_ERROR,
	TX,
	TX_TIMEOUT,
}States_t;

class Radio:public RadioEvent, public IRadio
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
	void chipReset(void);
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


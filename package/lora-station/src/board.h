#ifndef __BOARD_H__
#define __BOARD_H__

#include <unistd.h>
#include <stdint.h>

#define RADIO_RESET                                 11
#define RADIO_DIO_0                                 21
#define RADIO_DIO_1                                 20
#define RADIO_DIO_2                                 19
#define RADIO_DIO_3                                 18
#define RADIO_DIO_4                                 39
#define RADIO_DIO_5                                 40
#define RADIO_RTX                                   44

#define DelayMs(ms)                                 usleep(ms * 1000)

typedef enum
{
	MODEM_FSK = 0,
	MODEM_LORA,
}RadioModems_t;

typedef struct
{
	uint32_t modem;
	char gateway_id[20];
	char server_address[200];
	uint16_t serv_port_up;
	uint16_t serv_port_down;
	bool is_public_network;
	bool websocket_open;
	uint16_t websocket_port;
	uint32_t keepalive_interval;
	uint32_t frequency;
	uint32_t tx_freq;
	uint8_t power;
	uint8_t preamble_length;
	uint32_t datarate;
	uint32_t fdev;
	uint32_t fsk_bandwidth;
	uint32_t afc_bandwidth;
	uint8_t bandwidth;
	uint8_t spreading_factor;
	uint8_t codingrate;
	bool tx_iqInverted;
}LoRa_Config;

#endif

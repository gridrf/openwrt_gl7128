#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "spi.h"
#include "SX1276.h"
#include "gpio.h"
#include "Radio.h"
#include "TimerEvent.h"
#include "GpioControl.h"
#include "board.h"
#include "PacketForwarder.h"
#include "LoRaWebSocket.h"
#include "json-c/json.h"
#include "MessagerHandler.h"

#ifdef _WIN32
#define usleep(us) Sleep(us / 1000)

#define CONFIG_FILE "F:\\projects\\SX1276\\SX1276\\lora-station.json"
#else
#define CONFIG_FILE "/var/etc/lora-station.json"
#endif

#define DEFAULT_KEEPALIVE_INTERVAL 10000
#define DEFAULT_DOWNLINK_PORT 1780
#define DEFAULT_MAC_ADDR "70188B0CA611"



bool LoadConfig(LoRa_Config *conf)
{
	json_object *server_conf = NULL;
	json_object *radio_conf = NULL;
	json_object *value = NULL;
	json_object *root = json_object_from_file(CONFIG_FILE);

	conf->modem = 0;
	conf->is_public_network = false;
        conf->websocket_open = false;
	conf->frequency = RF_FREQUENCY;
	conf->power = TX_OUTPUT_POWER;
	conf->ppm = RF_PPM_OFFSET;
	conf->preamble_length = LORA_PREAMBLE_LENGTH;
	conf->fdev = FSK_FDEV;
	conf->datarate = FSK_DATARATE;
	conf->fsk_bandwidth = FSK_BANDWIDTH;
	conf->afc_bandwidth = FSK_AFC_BANDWIDTH;
	conf->bandwidth = LORA_BANDWIDTH;
	conf->spreading_factor = LORA_SPREADING_FACTOR;
	conf->codingrate = LORA_CODINGRATE;
	conf->tx_iqInverted = LORA_IQ_INVERSION_ON;

	if (root == NULL) {
		return false;
	}

	if (json_object_object_get_ex(root, "server", &server_conf)) {
		const char *gateway_id = NULL;
		const char *server_address = NULL;

		if (json_object_object_get_ex(server_conf, "gateway_id", &value)) {
			gateway_id = json_object_get_string(value);
		}
		if (json_object_object_get_ex(server_conf, "server_address", &value)) {
			server_address = json_object_get_string(value);
		}
		if (json_object_object_get_ex(server_conf, "serv_port_up", &value)) {
			conf->serv_port_up = json_object_get_int(value);
		}
		if (json_object_object_get_ex(server_conf, "serv_port_down", &value)) {
			conf->serv_port_down = json_object_get_int(value);
		}
		if (json_object_object_get_ex(server_conf, "is_public_network", &value)) {
			conf->is_public_network = json_object_get_boolean(value);
		}
		if (json_object_object_get_ex(server_conf, "websocket_open", &value)) {
			conf->websocket_open = json_object_get_boolean(value);
		}
		if (json_object_object_get_ex(server_conf, "websocket_port", &value)) {
			conf->websocket_port = json_object_get_int(value);
		}
		if (json_object_object_get_ex(server_conf, "keepalive_interval", &value)) {
			conf->keepalive_interval = json_object_get_int(value);
		}

		strcpy(conf->gateway_id, gateway_id);
		strcpy(conf->server_address, server_address);

		if (strlen(gateway_id) == 0) {
			strcpy(conf->gateway_id, DEFAULT_MAC_ADDR);
		}

		if (conf->serv_port_down == 0) {
			conf->serv_port_down = DEFAULT_DOWNLINK_PORT;
		}

		if (conf->keepalive_interval == 0) {
			conf->keepalive_interval = DEFAULT_KEEPALIVE_INTERVAL;
		}
	}
	else {
		json_object_put(root);
		return false;
	}


	if (json_object_object_get_ex(root, "radio", &radio_conf)) {
		if (json_object_object_get_ex(radio_conf, "modem", &value)) {
			std::string modem = json_object_get_string(value);
			if (modem.compare("FSK") == 0) {
				conf->modem = 1;
			}
		}
		if (json_object_object_get_ex(radio_conf, "frequency", &value)) {
			conf->frequency = json_object_get_int64(value);
			conf->tx_freq = conf->frequency;
		}
		if (json_object_object_get_ex(radio_conf, "ppm", &value)) {
			conf->ppm = json_object_get_int64(value);
		}
		if (json_object_object_get_ex(radio_conf, "power", &value)) {
			conf->power = json_object_get_int(value);
		}
		if (json_object_object_get_ex(radio_conf, "preamble", &value)) {
			conf->preamble_length = json_object_get_int(value);
		}
		if (json_object_object_get_ex(radio_conf, "fdev", &value)) {
			conf->fdev = json_object_get_int(value);
		}
		if (json_object_object_get_ex(radio_conf, "datarate", &value)) {
			conf->datarate = json_object_get_int(value);
		}
		if (json_object_object_get_ex(radio_conf, "fsk_bandwidth", &value)) {
			conf->fsk_bandwidth = json_object_get_int(value);
		}
		if (json_object_object_get_ex(radio_conf, "afc_bandwidth", &value)) {
			conf->afc_bandwidth = json_object_get_int(value);
		}
		if (json_object_object_get_ex(radio_conf, "bandwidth", &value)) {
			conf->bandwidth = json_object_get_int(value);
		}
		if (json_object_object_get_ex(radio_conf, "spreading_factor", &value)) {
			conf->spreading_factor = json_object_get_int(value);
		}
		if (json_object_object_get_ex(radio_conf, "codingrate", &value)) {
			conf->codingrate = json_object_get_int(value);
		}
		if (json_object_object_get_ex(radio_conf, "tx_iqInverted", &value)) {
			conf->tx_iqInverted = json_object_get_boolean(value);
		}

		json_object_put(root);
		return true;
	}

	return false;
}

int main(void)
{
	LoRa_Config _conf;

	LoadConfig(&_conf);
	gpio_init();
	SpiInit();
/*
	//radio test
	gpio_setdirecton(RADIO_RTX, 1);
	gpio_setdirecton(RADIO_RESET, 1);
	Radio *radio = new Radio(&_conf);
	TimerEvent *timer = new TimerEvent();
	SX1276 *sx1276 = new SX1276(radio, timer);
	radio->Init(sx1276);
	sx1276->Init();
	sx1276->SetTxContinuousWave(433000000, 20, 10);
*/
	gpio_setdirecton(RADIO_RTX, 1);
	gpio_setdirecton(RADIO_RESET, 1);

	// Radio initialization
	Radio *radio = new Radio(&_conf);
	TimerEvent *timer = new TimerEvent();
	SX1276 *sx1276 = new SX1276(radio, timer);
	sx1276->RegisterTimer();
    LoRaWebSocket *webSocket = NULL;
	MessagerHandler *msgHandler = new MessagerHandler(&_conf, sx1276, radio, timer);

	GPIOControl *dioEvt[5];
	for(int i=0;i<5;i++){
		dioEvt[i] = new GPIOControl(i, 0, 1, sx1276);
	}
		
	PacketForwarder *pkfw = new PacketForwarder(&_conf);
	pkfw->Init(timer, msgHandler);
	msgHandler->AddHandler(pkfw);

	if(_conf.websocket_open){
	   webSocket = new LoRaWebSocket(_conf.websocket_port);
	   webSocket->Init(msgHandler);
	   msgHandler->AddHandler(webSocket);
        }

	radio->Init(sx1276);
	radio->chipReset();
	

	while (1)
	{
		timer->Process();
		radio->Process(msgHandler);
		usleep(1000);
	}

	pkfw->Stop();
	delete(pkfw);
	if(_conf.websocket_open){
	   delete(webSocket);
	   webSocket = NULL;
	}

	delete(msgHandler);
	delete(sx1276);
	delete(radio);
	Spi_Close();
	gpio_free();
	
	for(int i=0;i<5;i++){
		delete(dioEvt[i]);
	}

	return 0;
}

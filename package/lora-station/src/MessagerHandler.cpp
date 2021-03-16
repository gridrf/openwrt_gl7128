#include "MessagerHandler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>
#include "Packet.h"
#include "json-c/json.h"

using namespace std;

#ifdef _WIN32
int gettimeofday(struct timeval *tp, void *tzp)
{
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;

	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	tp->tv_sec = (long)clock;
	tp->tv_usec = wtm.wMilliseconds * 1000;

	return (0);
}

#endif

MessagerHandler::MessagerHandler(LoRa_Config *conf, ILoRaChip *chip, IRadio *radio, TimerEvent *timer)
	:_conf(conf), _chip(chip), _radio(radio), _timer(timer)
{
	if (_conf->modem == 0) {
		modem = MODEM_LORA;
		uint32_t bandwidth = 0;

		switch (_conf->bandwidth) {
		case 0:
			bandwidth = 125;
			break;
		case 1:
			bandwidth = 250;
			break;
		case 2:
			bandwidth = 500;
			break;
		}
		sprintf(lora_datarate, "SF%dBW%d%c", _conf->spreading_factor, bandwidth, '\0');
		sprintf(lora_codingrate, "4/%d%c", _conf->codingrate + 4, '\0');
	}
	else {
		modem = MODEM_FSK;
	}

	this->epoch = this->UTCTime();
}

MessagerHandler::~MessagerHandler()
{
	msgQueue.clear();
}

uint64_t MessagerHandler::UTCTime()
{
	//#ifdef WINDOWS
	//	return GetTickCount();
	//#else
	struct timeval tv;
	if (gettimeofday(&tv, NULL) != 0)
		return 0;

	return ((uint64_t)tv.tv_sec * 1000000) + tv.tv_usec;
	//#endif
}


uint16_t MessagerHandler::NowTime(uint64_t utc)
{
	float time;
	time = (utc - epoch) / (float)4;
	return (uint16_t)(time + 0.5);
}

uint16_t MessagerHandler::GetToken()
{
	return NowTime(UTCTime());
}

void MessagerHandler::AddHandler(IMessager *handler)
{
	this->msgQueue.push_back(handler);
}


void MessagerHandler::OnPacket(uint8_t *buffer, int size, int16_t rssi, int8_t snr)
{
	Packet rxpkt;
	uint64_t utcTime = UTCTime();
	uint16_t token = NowTime(utcTime);
	rxpkt.WriteString("{\"rxpk\":[");

	time_t now = utcTime / 1000000;
	tm *now_tm = gmtime(&now);
	rxpkt.WriteStringFmt("{\"time\":\"%04i-%02i-%02iT%02i:%02i:%02i.%03liZ\",", (now_tm->tm_year) + 1900, (now_tm->tm_mon) + 1, now_tm->tm_mday, now_tm->tm_hour,
		now_tm->tm_min, now_tm->tm_sec, utcTime % 1000);


	rxpkt.WriteStringFmt("\"tmst\":%llu,", utcTime - epoch);
	rxpkt.WriteString("\"chan\" : 0,");
	rxpkt.WriteString("\"rfch\" : 0,");
	rxpkt.WriteStringFmt("\"freq\":%f,", _conf->frequency / 1000000.0);
	rxpkt.WriteString("\"stat\" : 1,");

	if (modem == MODEM_LORA) {
		rxpkt.WriteString("\"modu\" : \"LORA\",");
		rxpkt.WriteStringFmt("\"datr\":\"%s\",", lora_datarate);
		rxpkt.WriteStringFmt("\"codr\":\"%s\",", lora_codingrate);
	}
	else {
		rxpkt.WriteString("\"modu\" : \"FSK\",");
		rxpkt.WriteStringFmt("\"datr\":%d,", _conf->datarate);
	}
	rxpkt.WriteStringFmt("\"rssi\":%d,", rssi);
	rxpkt.WriteStringFmt("\"lsnr\":%d,", snr);
	rxpkt.WriteStringFmt("\"size\":%d,", size);

	char temp[255] = { 0 };
	base64.bin_to_b64(buffer, size, temp, 255);

	rxpkt.WriteStringFmt("\"data\" : \"%s\"", temp);
	rxpkt.WriteString("}]}\0");

	std::vector<IMessager*>::const_iterator it = msgQueue.begin();
	for (; it != msgQueue.end(); it++) {
		(*it)->OnPacket(token, rxpkt.GetBuffer(), rxpkt.GetLength());
	}
}

void MessagerHandler::OnTxPacket(const char *txpk)
{
	json_object *root = json_tokener_parse(txpk);
	if (root != NULL) {
		json_object *txJson = NULL;
		if (json_object_object_get_ex(root, "txpk", &txJson)) {
			json_object *value = NULL;
			const char *modu = NULL;
			const char *datr = NULL;
			const char *codr = NULL;
			const char *data = NULL;

			RadioMessage LoraPkt = {0};
			RadioMessage *msg = &LoraPkt;

			if (json_object_object_get_ex(txJson, "imme", &value)) {
				msg->imme = json_object_get_boolean(value);
			}
			if (json_object_object_get_ex(txJson, "tmst", &value)) {
				msg->tmst = json_object_get_int64(value);
			}
			if (json_object_object_get_ex(txJson, "freq", &value)) {
				msg->freq = 1000000 * json_object_get_double(value);
			}
			if (json_object_object_get_ex(txJson, "rfch", &value)) {
				msg->rfch = json_object_get_int(value);
			}

			if (json_object_object_get_ex(txJson, "powe", &value)) {
				msg->powe = json_object_get_int(value);
			}

			if (json_object_object_get_ex(txJson, "ipol", &value)) {
				msg->ipol = json_object_get_boolean(value);
			}

			if (json_object_object_get_ex(txJson, "size", &value)) {
				msg->size = json_object_get_int(value);
			}
			if (json_object_object_get_ex(txJson, "modu", &value)) {
				modu = json_object_get_string(value);
			}

			if (json_object_object_get_ex(txJson, "datr", &value)) {
				datr = json_object_get_string(value);
			}

			if (json_object_object_get_ex(txJson, "codr", &value)) {
				codr = json_object_get_string(value);
			}

			if (json_object_object_get_ex(txJson, "data", &value)) {
				data = json_object_get_string(value);
			}

			if (strcmp(modu, "FSK") == 0) {
				msg->model = MODEM_FSK;
			}

			std::string dedatr(datr);
			if (dedatr.find_first_of("SF") != string::npos) {
				int idx = dedatr.find("BW");
				if (idx > 0) {
					msg->spreading_factor = atoi(dedatr.substr(2, idx - 2).c_str());
					int _bandwidth = atoi(dedatr.substr(idx + 2, dedatr.length() - 2 - idx).c_str());
					switch (_bandwidth) {
					case 125:
					{
						msg->bandwidth = 0;
						break;
					}
					case 250:
					{
						msg->bandwidth = 1;
						break;
					}
					case 500:
					{
						msg->bandwidth = 2;
						break;
					}
					}
				}
			}

			std::string decodr(codr);
			if (decodr.find_first_of("4/") != string::npos) {
				msg->codingrate = atoi(decodr.substr(2, 1).c_str());
				msg->codingrate -= 4;
			}

			//uint8_t *loraData = msg->data;

			msg->size = base64.b64_to_bin(data, strlen(data), msg->data, 256);
			if (msg->size > 0) {
				if (_chip != NULL) {
					if (msg->imme) {
						if (_conf->frequency != msg->freq) {
							_conf->tx_freq = msg->freq;
							_chip->SetChannel(msg->freq+_conf->ppm);
						}

						if (_conf->power != msg->powe) {
							_chip->SetRfTxPower(msg->powe);
						}
						_chip->Send(msg->data, msg->size);
						//printf("SendRF to Node immediately.\n");
						//delete(msg);
					}
					else {
						uint64_t expireDate = this->UTCTime() - epoch;
						if(expireDate > msg->tmst){
						     printf("expire %d , tmst %d drop packet!\n", expireDate, msg->tmst);
						    // return;
						}else{
							RadioMessage *packet = new RadioMessage();
							packet->freq = msg->freq;
							packet->powe = msg->powe;
							packet->imme = msg->imme;
							packet->ipol = msg->ipol;
							packet->tmst = msg->tmst;
							packet->rfch = msg->rfch;
							packet->size = msg->size;
							packet->model= msg->model;
							packet->spreading_factor= msg->spreading_factor;
							packet->bandwidth= msg->bandwidth;
							packet->codingrate= msg->codingrate;

							memcpy(packet->data, msg->data, sizeof(msg->data));

							packet->sendTimeoutTimer.user = packet;
							_timer->RegisterTimer(this, &packet->sendTimeoutTimer);
							_timer->TimerSetValue(&packet->sendTimeoutTimer, (uint64_t)(packet->tmst - expireDate) / 1000);
							_timer->TimerStart(&packet->sendTimeoutTimer);

							printf("timer packet:%d\n", packet->tmst - expireDate);
						}
					}
				}
				//printf("SendRF to Node.\n");
			}
		}else if (json_object_object_get_ex(root, "module_reset", &txJson)) {
			_radio->chipReset();
			printf("RESET Radio.\n");
		}

	    json_object_put(root);
	}
}


bool MessagerHandler::OnTimeoutIrq(void *user)
{
	printf("ontimer irq:%d\n", this->UTCTime() - epoch);
	RadioMessage *userMsg = (RadioMessage *)user;
	if (_conf->frequency != userMsg->freq) {
		_conf->tx_freq = userMsg->freq;
		_chip->SetChannel(userMsg->freq+_conf->ppm);
	}

	if (_conf->power != userMsg->powe) {
		_chip->SetRfTxPower(userMsg->powe);
	}
	_chip->Send(userMsg->data, userMsg->size);
	_timer->RemoveTimer(&userMsg->sendTimeoutTimer);
	delete(userMsg);
	
	printf("ontimer irq finish!\n");
	return false;
}

/*
Copyright (C) 2018  GridRF Radio Team(tech@gridrf.com)

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
#ifndef __MESSAGE_HANDLER_H__
#define __MESSAGE_HANDLER_H__

#include <list>
#include "IMessager.h"
#include "ILoRaChip.h"
#include "board.h"
#include "Base64.h"
#include "IMessageSender.h"
#include "TimerHandler.h"
#include "TimerEvent.h"

#ifdef _WIN32
#include <WinSock2.h>
#include <Windows.h>
#else
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

typedef struct _RadioMessage
{
	uint32_t freq;
	uint8_t powe;
	bool imme;
	bool ipol;
	uint32_t tmst;
	int32_t rfch;
	uint8_t data[256];
	int32_t size;
	RadioModems_t model;
	uint8_t spreading_factor;
	uint8_t bandwidth;
	uint8_t codingrate;

	TimerEvent_t sendTimeoutTimer;
}RadioMessage;

class MessagerHandler:public IMessageSender, public TimerHandler
{
private:
	LoRa_Config *_conf;
	RadioModems_t modem;
	ILoRaChip *_chip;
	Base64  base64;
	TimerEvent *_timer;

	uint64_t epoch;
	char lora_datarate[20];
	char lora_codingrate[20];

	std::list<IMessager *> msgQueue;

private:
	uint64_t UTCTime();
	uint16_t NowTime(uint64_t utc);

public:
	MessagerHandler(LoRa_Config *conf, ILoRaChip *chip, TimerEvent *timer);
	~MessagerHandler();

	void AddHandler(IMessager *handler);
	uint16_t GetToken();
	void OnPacket(uint8_t *buffer, int size, int16_t rssi, int8_t snr);
	void OnTxPacket(const char *txpk);
	bool OnTimeoutIrq(void *user);
};

#endif

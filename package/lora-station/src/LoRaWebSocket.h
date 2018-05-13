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
#ifndef __LORAWEBSOCKET_H__
#define __LORAWEBSOCKET_H__

#include "WebSocketServer.h"
#include <pthread.h>
#include "IMessager.h"
#include "IMessageSender.h"

class LoRaWebSocket:public WebSocketServer, public IMessager
{
private:
	pthread_t _websockThread;
	IMessageSender *msgSender;

public:
	LoRaWebSocket(uint16_t port);
	~LoRaWebSocket();

	void Init(IMessageSender *sender);

	void OnPacket(uint16_t token, uint8_t *buffer, int size);

	void onConnect(int socketID);
	void onMessage(int socketID, uint8_t *data, int size);
	void onDisconnect(int socketID);
	void onError(int socketID, const string& message);
};

#endif


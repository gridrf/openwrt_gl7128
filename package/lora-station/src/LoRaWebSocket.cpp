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
#include "LoRaWebSocket.h"
#include <time.h>
#include "Packet.h"

void *websocketThreadCallback(void *ptr)
{
	LoRaWebSocket *lsok = (LoRaWebSocket *)ptr;
	lsok->run();

	return NULL;
}

LoRaWebSocket::LoRaWebSocket(uint16_t port)
	:WebSocketServer(port)
{
}


LoRaWebSocket::~LoRaWebSocket()
{
	pthread_join(_websockThread, NULL);
}


void LoRaWebSocket::Init(IMessageSender *sender)
{
	msgSender = sender;
	pthread_create(&_websockThread, NULL, websocketThreadCallback, this);
}


void LoRaWebSocket::onConnect(int socketID)
{
	//Util::log("New connection");
}

void LoRaWebSocket::onMessage(int socketID, uint8_t *data, int size)
{
	// Reply back with the same message
	msgSender->OnTxPacket((const char *)data);
}

void LoRaWebSocket::onDisconnect(int socketID)
{
	//Util::log("Disconnect");
}

void LoRaWebSocket::onError(int socketID, const string& message)
{
	//Util::log("Error: " + message);
}

void LoRaWebSocket::OnPacket(uint16_t token, uint8_t *buffer, int size)
{
	string pktstr((char *)buffer);

	this->broadcast(pktstr);
}

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

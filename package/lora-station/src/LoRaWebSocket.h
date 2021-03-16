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


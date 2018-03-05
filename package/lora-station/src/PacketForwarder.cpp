#include "PacketForwarder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <fcntl.h>
#include "Packet.h"
#include "json.h"

#define PROTOCOL_VERSION 2

void *sockThreadCallback(void *ptr)
{
	PacketForwarder *pkfw = (PacketForwarder *)ptr;
	pkfw->Run();

	return NULL;
}

PacketForwarder::PacketForwarder(LoRa_Config *conf)
	:_conf(conf), isRunning(false)
{
	struct hostent *he;
	struct sockaddr_in *sin = &this->ser_sin;

	sscanf(_conf->gateway_id, "%llx", &mac);

	if (strlen(_conf->server_address) == 0) {
		printf("not set server address\n");
		return;
	}

	if ((he = gethostbyname(_conf->server_address)) == NULL) {
		printf("gethostbyname err\n");
		return;
	}

	memcpy(&sin->sin_addr, he->h_addr_list[0], he->h_length);

	sin->sin_family = AF_INET;
	sin->sin_port = htons(_conf->serv_port_up);
}


PacketForwarder::~PacketForwarder()
{
}


int PacketForwarder::Init(TimerEvent *timer, IMessageSender *sender)
{
	int err;
	struct sockaddr_in sin;
	int udpBufferSize = UDP_BUFFER_SIZE;
	this->msgSender = sender;

	this->socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == this->socket_fd)
	{
		printf("socket error! error code is %d/n", errno);
		return -1;
	}

	sin.sin_family = AF_INET;
	sin.sin_port = htons(_conf->serv_port_down); //random port  from 1000 
	sin.sin_addr.s_addr = INADDR_ANY;
	
	setsockopt(this->socket_fd, SOL_SOCKET, SO_RCVBUF, (const char*)&udpBufferSize, sizeof(int));
	setsockopt(this->socket_fd, SOL_SOCKET, SO_SNDBUF, (const char*)&udpBufferSize, sizeof(int));

	err = bind(this->socket_fd, (struct sockaddr*)&sin, sizeof(struct sockaddr));
	if (SOCKET_ERROR == err)
	{
		printf("sock bind err\n");
		return -1;
	}
	
	timer->RegisterTimer(this, &heatTimeoutTimer);
	timer->TimerSetValue(&heatTimeoutTimer, _conf->keepalive_interval);
	timer->TimerStart(&heatTimeoutTimer);

	isRunning = true;

	pthread_create(&_sockThread, NULL, sockThreadCallback, this);
	return 0;
}


void PacketForwarder::OnPacket(uint16_t token,  uint8_t *buffer, int size)
{
	Packet rxpkt;

	rxpkt.WriteByte(PROTOCOL_VERSION);

	rxpkt.WriteInt16(token);//1 - 2 | random token
	rxpkt.WriteByte(0x00); //PUSH_DATA identifier  0x00
	rxpkt.WriteInt64(mac); //Gateway unique identifier(MAC address)
	//JSON object, starting with{ , ending with }, see section 4
	rxpkt.WriteBuffer(buffer, size);
	this->Send(rxpkt.GetBuffer(), rxpkt.GetLength());
}

//all test
int PacketForwarder::Send(uint8_t *buffer, int size)
{
	int nSendSize = sendto(this->socket_fd, (const char *)buffer, size, 0, (struct sockaddr*)&this->ser_sin, sizeof(struct sockaddr));
	if (SOCKET_ERROR == nSendSize)
	{
		printf("sendto error!, error code is %d\r\n", errno);
		return -1;
	}
	return nSendSize;
}

//all test
void PacketForwarder::OnSockRead(uint8_t *buffer, int size, struct sockaddr_in *from_addr)
{
	Packet txpkt(buffer, size);

	uint8_t ver = txpkt.ReadByte();
	uint16_t token = txpkt.ReadInt16();
	uint8_t method = txpkt.ReadByte();

	//PULL_RESP identifier 0x03
	if (method == 0x3) {
		uint8_t buffer[512] = { 0 };
		int strLength = txpkt.Available();
		txpkt.ReadBuffer(buffer, strLength);
		msgSender->OnTxPacket((const char *)buffer);
	}
}

//all test
void PacketForwarder::Run()
{
	int ret;
	struct sockaddr_in sin_from;
	char buffer[UDP_BUFFER_SIZE];
#ifdef WIN32
	int addr_len = sizeof(struct sockaddr);
	struct fd_set fds; 
	int iMode = 0;//0 == block  1 == non-block
	ioctlsocket(socket_fd, FIONBIO, (u_long FAR*) &iMode);
#else
	socklen_t addr_len = sizeof(struct sockaddr);
	fd_set fds; 
	fcntl(socket_fd, F_SETFL, O_NONBLOCK);
#endif

	while (isRunning) {
		FD_ZERO(&fds);
		FD_SET(socket_fd, &fds);
		ret = select(socket_fd+1, &fds, NULL, NULL, NULL);
		if (FD_ISSET(socket_fd, &fds)) {
			FD_CLR(socket_fd, &fds);

			ret = recvfrom(socket_fd, buffer, UDP_BUFFER_SIZE, 0, (struct sockaddr*)&sin_from, &addr_len);
			if (SOCKET_ERROR != ret) {
				this->OnSockRead((uint8_t *)buffer, ret, &sin_from);
			}
		}
	}

	isRunning = false;
}

//all test
bool PacketForwarder::OnTimeoutIrq(void *user)
{
	//send downstream check
	Packet rxpkt;
	rxpkt.WriteByte(PROTOCOL_VERSION);
	rxpkt.WriteInt16(msgSender->GetToken());//1 - 2 | random token
	rxpkt.WriteByte(0x02); //PUSH_DATA identifier  0x00
	rxpkt.WriteInt64(mac); //Gateway unique identifier(MAC address)
	this->Send(rxpkt.GetBuffer(), rxpkt.GetLength());

	heatTimeoutTimer.Timestamp = heatTimeoutTimer.ReloadValue;
	heatTimeoutTimer.IsRunning = true; //forever
	return true;
}

void PacketForwarder::Stop()
{
	isRunning = false;
	pthread_join(_sockThread, NULL);
}

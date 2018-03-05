#ifndef __IMESSAGESENDER_H__
#define __IMESSAGESENDER_H__

#include <stdint.h>

class IMessageSender
{
public:
	virtual uint16_t GetToken() = 0;
	virtual void OnTxPacket(const char *txpk) = 0;
};

#endif

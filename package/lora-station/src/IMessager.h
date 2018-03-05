#ifndef __IMESSAGER_H__
#define __IMESSAGER_H__

#include <stdint.h>

class IMessager
{
public:
	virtual void OnPacket(uint16_t token, uint8_t *buffer, int size) = 0;
};

#endif

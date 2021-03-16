#ifndef __GPIO_CONTROL_H__
#define __GPIO_CONTROL_H__

#include <stdint.h>
#include <string>
#include "pthread.h"
#include "GpioIrqHandler.h"

class GPIOControl
{
private:
    int _pollFD;
	bool _is_running;
    pthread_t _isrThread;
	GPIO_IRQ_Handler *_isr;

public:
   GPIOControl(uint32_t id,
	            uint32_t direction,
  				uint32_t edge,
  				GPIO_IRQ_Handler *isr);
	
	~GPIOControl();
	void isrLoop();
	
private:
	const unsigned short _id;
	const uint32_t      _direction;
	const uint32_t		_edge;
};

#endif



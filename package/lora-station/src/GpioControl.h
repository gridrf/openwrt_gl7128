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



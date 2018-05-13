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
#ifndef __GPIO_IRQ_HANDLER_H__
#define __GPIO_IRQ_HANDLER_H__

#include <stdio.h>

class GPIO_IRQ_Handler
{
public:
	virtual void    OnDio0Irq( void ) = 0;
	virtual void    OnDio1Irq( void )= 0;
	virtual void    OnDio2Irq( void )= 0;
	virtual void    OnDio3Irq( void )= 0;
	virtual void    OnDio4Irq( void )= 0;
	virtual void    OnDio5Irq( void )= 0;

	void onISRInterrupt(uint32_t pin, uint32_t val)
	{
		switch(pin)
		{
			case 0:
			{
				this->OnDio0Irq ();
				break;
			}
			case 1:
			{
				this->OnDio1Irq ();
				break;
			}
			case 2:
			{
				this->OnDio2Irq ();
				break;
			}
			case 3:
			{
				this->OnDio3Irq ();
				break;
			}
			case 4:
			{
				this->OnDio4Irq ();
				break;
			}
			case 5:
			{
				this->OnDio5Irq ();
				break;
			}
		}
	}
};

#endif

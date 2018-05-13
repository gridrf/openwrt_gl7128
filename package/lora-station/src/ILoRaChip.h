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
#ifndef __ILORACHIP_H__
#define __ILORACHIP_H__

#include <stdint.h>
#include "board.h"

class ILoRaChip
{
public:
	virtual void Sleep() = 0;
	virtual void Send(uint8_t *buffer, uint8_t size) = 0;
	virtual void Rx(uint32_t timeout) = 0;
	virtual void SetRfTxPower(int8_t power) = 0;
	virtual void SetChannel(uint32_t freq) = 0;
};

#endif

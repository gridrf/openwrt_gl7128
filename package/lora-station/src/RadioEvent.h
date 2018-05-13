/* Copyright (C) 2018  GridRF Radio Team(tech@gridrf.com)

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
#ifndef __RADIO_EVENT_H__
#define __RADIO_EVENT_H__

#include <stdint.h>

class RadioEvent
{
public:
	virtual void OnTxDone(void) = 0;
	virtual void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) = 0;
	virtual void OnTxTimeout(void) = 0;
	virtual void OnRxTimeout(void) = 0;
	virtual void OnRxError(void) = 0;
	virtual void OnFhssChangeChannel(uint8_t currentChannel) = 0;
	virtual void OnCadDone(bool channelActivityDetected) = 0;
};

#endif

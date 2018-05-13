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
#ifndef __TIMEREVENT_H__
#define __TIMEREVENT_H__

#include <stdint.h>
#include <list>
#include "TimerHandler.h"

typedef struct TimerEvent_s
{
	uint32_t Timestamp;         //! Current timer value
	uint32_t ReloadValue;       //! Timer delay value
	bool IsRunning;             //! Is the timer currently running	
	void *user;
	TimerHandler *handler;
}TimerEvent_t;

class TimerEvent
{
private:
	unsigned long startTime;
	uint32_t lastElapsedTime;
	std::list<TimerEvent_t *> timerEvents;

public:
	TimerEvent();
	~TimerEvent();

	void RegisterTimer(TimerHandler *handler, TimerEvent_t *obj);
	void RemoveTimer(TimerEvent_t *obj);
	void TimerStart(TimerEvent_t *obj);
	void TimerStop(TimerEvent_t *obj);
	void TimerSetValue(TimerEvent_t *obj, uint32_t value);
	unsigned long TimerGetCurrentTime();
	uint32_t TimerGetElapsedTime(uint32_t carrierSenseTime);
	void Process(void);
};

#endif


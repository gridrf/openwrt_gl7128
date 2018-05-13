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
#include "TimerEvent.h"
#include <stdio.h>
#include <time.h>

TimerEvent::TimerEvent()
{
	startTime = TimerGetCurrentTime();
	lastElapsedTime = startTime;
}

TimerEvent::~TimerEvent()
{
}

void TimerEvent::Process(void)
{
	uint32_t nowTime = TimerGetElapsedTime(startTime);
	uint32_t elapsedTime = nowTime - lastElapsedTime;
	lastElapsedTime = nowTime;

	if (elapsedTime == 0) {
		//usleep(10000);
		return;
	}

	std::list<TimerEvent_t *>::const_iterator it = timerEvents.begin();
	for (; it != timerEvents.end(); it++)
	{
		if ((*it)->IsRunning) {
			if (elapsedTime >= (*it)->Timestamp)
			{
				(*it)->Timestamp = 0;
			}
			else {
				(*it)->Timestamp -= elapsedTime;
			}

			if ((*it)->Timestamp == 0) {
				(*it)->IsRunning = false;
				if(!(*it)->handler->OnTimeoutIrq((*it)->user)){
					break;
				}
			}
		}
	}
}


void TimerEvent::RegisterTimer(TimerHandler *handler, TimerEvent_t *obj)
{
	obj->Timestamp = 0;
	obj->ReloadValue = 0;
	obj->IsRunning = false;
	obj->handler = handler;

	timerEvents.push_back(obj);
	//printf("RegisterTimer = %d\n", TimerGetElapsedTime(startTime));
}

void TimerEvent::RemoveTimer(TimerEvent_t *obj)
{
	timerEvents.remove(obj);
}

void TimerEvent::TimerStart(TimerEvent_t *obj)
{
	obj->Timestamp = obj->ReloadValue;
	obj->IsRunning = true;
	lastElapsedTime = TimerGetElapsedTime(startTime);
}

void TimerEvent::TimerStop(TimerEvent_t *obj)
{
	obj->IsRunning = false;
}

void TimerEvent::TimerSetValue(TimerEvent_t *obj, uint32_t value)
{
	obj->Timestamp = value;
	obj->ReloadValue = value;
}

unsigned long TimerEvent::TimerGetCurrentTime()
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

uint32_t TimerEvent::TimerGetElapsedTime(uint32_t carrierSenseTime)
{
	return TimerGetCurrentTime() - carrierSenseTime;
}


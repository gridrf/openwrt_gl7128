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


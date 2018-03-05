#ifndef __TIMERHANDLER_H__
#define __TIMERHANDLER_H__

class TimerHandler
{
public:
	virtual bool OnTimeoutIrq(void *user) = 0;
};

#endif

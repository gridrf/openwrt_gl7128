#include "GpioControl.h"
#include <fcntl.h>
#include <poll.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

using namespace std;

#define LORA_PATH_DIR_FMT "/sys/class/gpio/lora-dio%d/direction"
#define LORA_PATH_EDGE_FMT "/sys/class/gpio/lora-dio%d/edge"
#define LORA_PATH_VALUE_FMT "/sys/class/gpio/lora-dio%d/value"

void *threadCallback(void *ptr)
{
	GPIOControl *ctl = (GPIOControl *)ptr;
	ctl->isrLoop();
}

GPIOControl::GPIOControl(uint32_t id,
	            uint32_t direction,
  				uint32_t edge, 
                GPIO_IRQ_Handler *isr):
	_id(id),
	_direction(direction),
	_edge(edge),
	_isr(isr)
{
	int fd;
	char path[200];
	sprintf(path, LORA_PATH_DIR_FMT, id);
	fd = open(path, O_WRONLY);
	if(fd < 0)
	{
		printf("Unable to set direction for GPIO %d\n", _id);
		return;
	}

	write(fd, _direction ==0 ? "in": "out", _direction ==0 ? 3:4);

	close(fd);

	if(_direction == 0){
		memset(path, 0, sizeof(path));
		sprintf(path, LORA_PATH_EDGE_FMT, id);
		fd = open(path, O_WRONLY);
  		if(fd < 0)
  		{
    			printf("Unable to set edge for GPIO %d\n", id);
			return;
  		}

  		if     ( _edge == 0)	write(fd, "none\n", 5);
  		else if( _edge == 1)	write(fd, "rising\n", 7);
  		else if( _edge == 2)	write(fd, "falling\n", 8);
  		else if( _edge == 3)    write(fd, "both\n", 5);
		
		close(fd);

		_is_running = true;

		pthread_create(&_isrThread, NULL, threadCallback, this); 
	}
}

GPIOControl::~GPIOControl()
{
	_is_running = false;
	pthread_join(_isrThread, NULL);

	// Do not close the file descriptor for the sysfs value file until _pollThread() has joined.
	// This prevents reuse of this file descriptor by the kernel for other threads in this
	// process while the descriptor is still in use in the poll() system call.
}

void GPIOControl::isrLoop()
{
	int fd;
	char path[200];
	
	sprintf(path, LORA_PATH_VALUE_FMT, _id);
    	fd = open(path, O_RDONLY); // closed in destructor
    	if( fd < 0 )
    	{
        	perror("open");
        	cout << "Unable to open " << path;
    	}
	
	const int MAX_BUF = 2; // either 1 or 0 plus EOL
	char buf[MAX_BUF];
	struct pollfd fdset[1];
	int nfds = 1;

	memset((void*)fdset, 0, sizeof(fdset));

    	fdset[0].fd     = fd;
    	fdset[0].events = POLLPRI;
	fdset[0].revents = 0;

        //clear
    	lseek(fd, 0, SEEK_SET);
	read(fd, buf, MAX_BUF);

	while(_is_running)
    	{
		const int rc = poll(fdset, nfds, -1);
		if( rc == 1 )
		{
    			lseek(fd, 0, SEEK_SET);
        		const ssize_t nbytes = read(fd, buf, MAX_BUF);
        		if( nbytes != MAX_BUF ) // See comment above
        		{
           			cout << "GPIO " << _id << " read2() badness...";
        		}
				 
        		uint32_t val;
        		if     ( buf[0] == '0' )  val = 0;
        		else if( buf[0] == '1' )  val = 1;

			//if((_edge == 1 && val == 1) || (_edge == 2 && val == 0)){
				if(_isr!=NULL){
					_isr->onISRInterrupt(_id, val);
				}
			//}
		}
	}
	close(fd);
}


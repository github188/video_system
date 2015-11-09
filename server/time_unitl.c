#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include "common.h"


#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"timer"


int get_time(struct timeval *tp)
{

	int ret = -1;
	if(NULL == tp)
	{
		dbg_printf("please check the param ! \n");
		return (-1);
	}
	struct timespec	ts;
	ret = clock_gettime(CLOCK_MONOTONIC, &ts);
	if(ret == -1)
	{
		dbg_printf("get time fail ! \n");
		return(-2);
	}
	tp->tv_sec = ts.tv_sec;
	tp->tv_usec = ts.tv_nsec / 1000;
	return(0);
}


void add_time(struct timeval *a, struct timeval *b,struct timeval *res)
{
	timeradd(a,b,res);
}


void sub_time(struct timeval *a, struct timeval *b,struct timeval *res)
{
	int result = timercmp(a,b,>=);
	if(result)
		timersub(a,b,res);
	else
		timersub(b,a,res);
}


void time_sleep(unsigned int second,unsigned int usec)
{
	struct timeval delay;  
	delay.tv_sec = second;  
	delay.tv_usec = usec;  
	select(0, NULL, NULL, NULL, &delay);  	
}
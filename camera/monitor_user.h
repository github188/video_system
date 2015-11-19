#ifndef _monitor_user_h
#define _monitor_user_h


#include "common.h"


#define  MAX_LOST_COUNT		(10)

typedef struct monitor_user
{

	int lost_count[2];
	pthread_t monitor_ptid;
	pthread_fun  monitor_fun;;

}monitor_user_t;








#endif  /*_monitor_user_h*/
#ifndef _heart_beat_h
#define _heart_beat_h

#include "common.h"
#include "ring_queue.h"
#include "handle_packet.h"



typedef struct heart_beat_handle
{

	pthread_mutex_t mutex_heart;
	pthread_cond_t cond_heart;
	volatile unsigned int is_run;
	pthread_t heartbeat_ptid;
	pthread_fun  heartbeat_fun;;

}heart_beat_handle_t;


void *  heartbeat_handle_new(void);
void heartbeat_handle_destroy(void * arg);
int heartbeat_start(void * dev);
int heartbeat_stop(void * dev);





#endif  /*_heart_beat_h*/
#ifndef _process_loin_h
#define  _process_loin_h

#include "common.h"
#include "ring_queue.h"
#include "handle_packet.h"



typedef struct process_loin_handle
{

	pthread_mutex_t mutex_loin;
	pthread_cond_t cond_loin;
	volatile unsigned int is_run;
	struct sockaddr addr; 
	pthread_t loin_ptid;
	pthread_fun  loin_fun;;

}process_loin_handle_t;


void *  loin_handle_new(void);
void loin_handle_destroy(void * arg);
int loin_start(void * dev,void * addr);
int loin_stop(void * dev);






#endif  /*_process_loin_h*/


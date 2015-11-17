#ifndef  _net_recv_h
#define	 _net_recv_h

#include "common.h"
#include "eventfd.h"
#include "ring_queue.h"
#include "data_packet.h"
#include "handle_packet.h"
#include "socket_init.h"




typedef struct net_recv_handle
{
	pthread_mutex_t mutex_recv;
	pthread_cond_t cond_recv;
	ring_queue_t recv_msg_queue;
	volatile unsigned int recv_msg_num;

	int event_fd;
	pthread_t netrecv_ptid;
	pthread_t netprocess_ptid;
	pthread_fun recv_fun;
	pthread_fun process_fun;

}net_recv_handle_t;


void * recv_handle_new(void);
void recv_handle_destroy(void * arg);



#endif  /*_net_recv_h*/
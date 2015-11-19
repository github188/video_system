#ifndef  _net_send_h
#define  _net_send_h

#include "common.h"
#include "data_packet.h"
#include "ring_queue.h"
#include "time_unitl.h"

#define		RESEND_PACKET_MAX_NUM	(1024)
#define		RESEND_TIMES			(3)
#define		RESEND_TIME_INTERVAL	(800) 

typedef struct net_send_handle
{
	pthread_mutex_t mutex_send;
	pthread_cond_t cond_send;
	ring_queue_t send_msg_queue;
	volatile unsigned int send_msg_num;

	
	void * resend[RESEND_PACKET_MAX_NUM];
	volatile int packet_num; 
	pthread_mutex_t mutex_resend;
	pthread_cond_t cond_resend;
	volatile unsigned int resend_msg_num;

	pthread_t netsend_ptid;
	pthread_t netresend_ptid;
	pthread_fun send_fun;
	pthread_fun resend_fun;
		
}net_send_handle_t;





int send_remove_packet(void *send_handle, int index);
int  send_push_msg(void *send_handle,void * data );
void * send_handle_new(void);
void send_handle_destroy(void * arg);

#endif  /*_net_send_h*/
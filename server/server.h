#ifndef  _server_h
#define  _server_h

#include "common.h"
#include "data_packet.h"
#include "ring_queue.h"
#include "time_unitl.h"
#include "handle_packet.h"



#define		RESEND_PACKET_MAX_NUM	(1024)
#define		RESEND_TIMES			(3)
#define		RESEND_TIME_INTERVAL	(800)  
#define	 	SERVER_PORT				(8003u)
#define	 	DEVICE_MAX_NUM			(4096u)


typedef  struct  dev_info
{
	int is_run;
	struct sockaddr dev_addr;
	struct sockaddr dev_localaddr;
	char dev_name[60];
	
}dev_info_t;





typedef struct server_handle
{
	int server_socket;
	
	pthread_mutex_t mutex_server;
	pthread_cond_t cond_server;
	ring_queue_t server_msg_queue;
	volatile unsigned int server_msg_num;


	pthread_mutex_t mutex_send;
	pthread_cond_t cond_send;
	ring_queue_t send_msg_queue;
	volatile unsigned int send_msg_num;


	void * resend[RESEND_PACKET_MAX_NUM];
	volatile int packet_num; 
	pthread_mutex_t mutex_resend;
	pthread_cond_t cond_resend;
	volatile unsigned int resend_msg_num;

	dev_info_t * dev[DEVICE_MAX_NUM];

	handle_packet_fun_t * fun;
	
}server_handle_t;







#endif  /*_server_h*/
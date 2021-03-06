#ifndef  _system_init_h
#define _system_init_h

#include "common.h"
#include "socket_init.h"
#include "net_send.h"
#include "net_recv.h"
#include "handle_packet.h"
#include "process_loin.h"
#include "heart_beat.h"





typedef struct camera_handle
{

	struct sockaddr peeraddr;
	char id_num;
	
	socket_handle_t * socket;
	net_send_handle_t * send;
	net_recv_handle_t * recv;
	handle_packet_fun_t * fun;
	process_loin_handle_t * loin;
	heart_beat_handle_t * beatheart;
	

}camera_handle_t;



extern  camera_handle_t * camera;

int system_init(void);


#endif  /*_system_init_h*/
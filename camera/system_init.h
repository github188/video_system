#ifndef  _system_init_h
#define _system_init_h

#include "socket_init.h"
#include "net_send.h"
#include "net_recv.h"
#include "handle_packet.h"



typedef struct camera_handle
{
	socket_handle_t * socket;
	net_send_handle_t * send;
	net_recv_handle_t * recv;
	handle_packet_fun_t * fun;

}camera_handle_t;



extern  camera_handle_t * camera;

int system_init(void);


#endif  /*_system_init_h*/
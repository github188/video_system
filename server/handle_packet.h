#ifndef  _handle_packet_h
#define  _handle_packet_h


#include "common.h"
#include "data_packet.h"



typedef struct handle_packet_fun
{
	packet_type_m type;
	int  (*handle_packet)(void * handle,void * data);
}handle_packet_fun_t;

handle_packet_fun_t *  get_handle_packet_fun(void);



int  send_register_packet(void);


#endif  /*_handle_packet_h*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "common.h"
#include "handle_packet.h"
#include "system_init.h"
#include "data_packet.h"
#include "net_send.h"


#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"handle_packet"



static system_handle_t * system_info = NULL;




int  send_register_packet(void)
{


	int ret = -1;
	if(NULL == system_info)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}
	system_handle_t * handle =system_info;
	
	
	register_packet_t * rpacket = calloc(1,sizeof(*rpacket));
	if(NULL == rpacket)
	{
		dbg_printf("calloc is fail ! \n");
		return(-1);
	}

	send_packet_t *spacket = calloc(1,sizeof(*spacket));
	if(NULL == spacket)
	{
		dbg_printf("calloc is fail ! \n");
		return(-2);
	}

	rpacket->head.type = REGISTER_PACKET;
	rpacket->head.index = 0xFFFF;
	rpacket->head.packet_len = sizeof(rpacket->x);
	rpacket->x = 'r';

	spacket->sockfd = handle->servce_socket;
	spacket->data = rpacket;
	spacket->length = sizeof(register_packet_t);
	spacket->to = handle->servaddr;

	spacket->type = RELIABLE_PACKET;
	spacket->is_resend = 0;
	spacket->resend_times = 0;
	spacket->index = 0xFFFF;
	spacket->ta.tv_sec = 0;
	spacket->ta.tv_usec = 800*1000;
	ret = netsend_push_msg(spacket);
	if(ret != 0)
	{
		dbg_printf("netsend_push_msg is fail ! \n");
		free(rpacket);
		rpacket = NULL;

		free(spacket);
		spacket = NULL;

		return(-1);
	}
	
	return(0);
}




int handle_packet_init(void)
{
	if(NULL != system_info)
	{
		dbg_printf("handle has been init ! \n");
		return(-1);
	}

	system_info = system_gethandle();
	
	return(0);
}
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


int  send_peer_packet(void)
{


	int ret = -1;
	if(NULL == system_info)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}
	system_handle_t * handle =system_info;
	
	send_packet_t *spacket = NULL;
	peer_packet_t * rpacket = (peer_packet_t*)calloc(1,sizeof(*rpacket));
	if(NULL == rpacket)
	{
		dbg_printf("calloc is fail ! \n");
		goto fail;
	}

	spacket = (send_packet_t*)calloc(1,sizeof(*spacket));
	if(NULL == spacket)
	{
		dbg_printf("calloc is fail ! \n");
		goto fail;
	}

	rpacket->head.type = PEER_PACKET;
	rpacket->head.index = 0xFFFF;
	rpacket->head.packet_len = sizeof(rpacket->dev_name);
	rpacket->head.ret = 0xFF;

	memset(rpacket->dev_name,'\0',sizeof(rpacket->dev_name));
	strcpy(rpacket->dev_name,"camera_0");

	spacket->sockfd = handle->servce_socket;
	spacket->data = rpacket;
	spacket->length = sizeof(peer_packet_t);
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
		goto fail;
	}

	return(0);

	
fail:

	if(NULL != rpacket)
	{
		free(rpacket);
		rpacket = NULL;
	}

	if(NULL != spacket)
	{
		free(spacket);
		spacket = NULL;
	}
	return(0);
}



int  handle_peer_ask(void * arg)
{

	peer_ask_packet_t * packet = (peer_ask_packet_t*)arg;
	if(NULL == packet)
	{
		dbg_printf("the packet is not right ! \n");
		return(-1);
	}
	netsend_remove_packet(packet->head.index);
	
	return(0);
}


int handle_packet_init(void)
{
	if(NULL != system_info)
	{
		dbg_printf("handle has been init ! \n");
		return(-1);
	}

	system_info = (system_handle_t*)system_gethandle();
	
	return(0);
}
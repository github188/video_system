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
#include "process_loin.h"


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
		goto fail;
	}

	send_packet_t *spacket = calloc(1,sizeof(*spacket));
	if(NULL == spacket)
	{
		dbg_printf("calloc is fail ! \n");
		goto fail;
	}

	rpacket->head.type = REGISTER_PACKET;
	rpacket->head.index = 0xFFFF;
	rpacket->head.packet_len = sizeof(rpacket->x);
	rpacket->x = 'r';

	memset(rpacket->dev_name,'\0',sizeof(rpacket->dev_name));
	strcpy(rpacket->dev_name,"camera_0");

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



int  handle_register_ask(void * arg)
{

	struct sockaddr * src_addres = (struct sockaddr *)arg;
	packet_header_t * header =	(packet_header_t *)(src_addres+1); 
	register_ask_packet_t * packet = (register_ask_packet_t*)(header);
	if(NULL == packet)
	{
		dbg_printf("the packet is not right ! \n");
		return(-1);
	}
	netsend_remove_packet(packet->head.index);
	
	return(0);
}



int  handle_hole_ask(void * arg)
{


	int ret = -1;

	system_handle_t * handle =system_info;
	struct sockaddr * src_addres = (struct sockaddr *)arg;
	packet_header_t * header =	(packet_header_t *)(src_addres+1); 
	hole_packet_t * packet = (hole_packet_t*)(header);
	
	if(NULL == handle || NULL==packet)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}


	hole_packet_ask_t * rpacket = calloc(1,sizeof(*rpacket));
	if(NULL == rpacket)
	{
		dbg_printf("calloc is fail ! \n");
		goto fail;
	}

	send_packet_t *spacket = calloc(1,sizeof(*spacket));
	if(NULL == spacket)
	{
		dbg_printf("calloc is fail ! \n");
		goto fail;
	}

	rpacket->head.type = HOLE_PACKET_ASK;
	rpacket->head.index = packet->head.index;
	rpacket->head.packet_len = sizeof(rpacket->dev_addr);
	rpacket->dev_addr = packet->dev_addr;


	spacket->sockfd = handle->servce_socket;
	spacket->data = rpacket;
	spacket->length = sizeof(loin_packet_ask_t);
	spacket->to = handle->servaddr;

	spacket->type = UNRELIABLE_PACKET;
	
	ret = netsend_push_msg(spacket);
	if(ret != 0)
	{
		dbg_printf("netsend_push_msg is fail ! \n");
		goto fail;
	}


	loin_start_process(&packet->dev_addr);

	
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




int send_active_channel_packet(void * dest_addr)
{


	int ret = -1;

	system_handle_t * handle =system_info;
	struct sockaddr * addr = (struct sockaddr*)(dest_addr);
	if(NULL == system_info || NULL==addr)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}


	active_channle_t * rpacket = calloc(1,sizeof(*rpacket));
	if(NULL == rpacket)
	{
		dbg_printf("calloc is fail ! \n");
		goto fail;
	}

	send_packet_t *spacket = calloc(1,sizeof(*spacket));
	if(NULL == spacket)
	{
		dbg_printf("calloc is fail ! \n");
		goto fail;
	}

	rpacket->head.type = ACTIVE_CHANNEL_PACKET;
	rpacket->head.index = 0xFFFF;
	rpacket->head.packet_len = sizeof(rpacket->a);


	spacket->sockfd = handle->servce_socket;
	spacket->data = rpacket;
	spacket->length = 1/*sizeof(active_channle_t)*/;
	spacket->to = *addr;

	spacket->type = UNRELIABLE_PACKET;
	
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


int  handle_active_channel_ask(void * arg)
{

	struct sockaddr * src_addres = (struct sockaddr *)arg;
	packet_header_t * header =	(packet_header_t *)(src_addres+1); 
	active_channle_ask_t * packet = (active_channle_ask_t*)(header);
	
	if(NULL == packet)
	{
		dbg_printf("the packet is not right ! \n");
		return(-1);
	}
	loin_stop_process(NULL);
	dbg_printf("handle_active_channel_ask ! \n");
	
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
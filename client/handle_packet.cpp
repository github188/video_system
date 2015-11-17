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
#define 	FILE_NAME 	"handle_packet:"



static system_handle_t * system_info = NULL;
int  send_loin_packet(struct sockaddr * addr);




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

	struct sockaddr * src_addres = (struct sockaddr *)arg;
	packet_header_t * header =	(packet_header_t *)(src_addres+1); 
	peer_ask_packet_t * packet = (peer_ask_packet_t*)(header);
	
	if(NULL == packet)
	{
		dbg_printf("the packet is not right ! \n");
		return(-1);
	}
	netsend_remove_packet(packet->head.index);
	if(0 == packet->head.ret)
	{
		send_loin_packet(&packet->dev_addr);
		dbg_printf("i receive the peer ask ! \n");
	}
	else
	{
		dbg_printf("the dev is not exit ! \n");
		return(-2);

	}
	
	return(0);
}


int  send_loin_packet(struct sockaddr * addr)
{


	int ret = -1;
	if(NULL == system_info)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}
	system_handle_t * handle =system_info;
	
	send_packet_t *spacket = NULL;
	loin_packet_t * rpacket = (loin_packet_t*)calloc(1,sizeof(*rpacket));
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

	rpacket->head.type = LOIN_PACKET;
	rpacket->head.index = 0xFFFF;
	rpacket->head.packet_len = sizeof(loin_packet_t)-sizeof(packet_header_t);
	rpacket->head.ret = 0xFF;
	
	rpacket->dev_addr = *addr;
	memset(rpacket->dev_name,'\0',sizeof(rpacket->dev_name));
	strcpy(rpacket->dev_name,"camera_0");

	
	spacket->sockfd = handle->servce_socket;
	spacket->data = rpacket;
	spacket->length = sizeof(loin_packet_t);
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


int  handle_loin_ask(void * arg)
{

	struct sockaddr * src_addres = (struct sockaddr *)arg;
	packet_header_t * header =	(packet_header_t *)(src_addres+1); 
	loin_packet_ask_t * packet = (loin_packet_ask_t*)(header);
	
	if(NULL == packet)
	{
		dbg_printf("the packet is not right ! \n");
		return(-1);
	}
	netsend_remove_packet(packet->head.index);
	dbg_printf("handle_loin_ask \n");
	loin_start_process(&packet->dev_addr);
	
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

	send_packet_t *spacket = NULL;
	active_channle_ask_t * rpacket = (active_channle_ask_t*)calloc(1,sizeof(*rpacket));
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

	rpacket->head.type = ACTIVE_CHANNEL_ASK;
	rpacket->head.index = 0xFFFF;
	rpacket->head.packet_len = sizeof(rpacket->a);


	spacket->sockfd = handle->servce_socket;
	spacket->data = rpacket;
	spacket->length = sizeof(active_channle_ask_t);
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

	system_info = (system_handle_t*)system_gethandle();
	
	return(0);
}
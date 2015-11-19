#include "handle_packet.h"
#include "system_init.h"
#include "net_send.h"


#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"handle_packet:"

#define  INVADE_ID_NUM		(100)

int  send_peer_packet(void * dev)
{


	int ret = -1;
	camera_handle_t * camera_dev = (camera_handle_t*)dev;
	if(NULL == camera_dev || NULL == camera_dev->socket || NULL == camera_dev->send)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}

	net_send_handle_t * send_handle = camera_dev->send;
	socket_handle_t * handle = camera_dev->socket;
	struct sockaddr localaddr = camera_dev->socket->localaddr;
	

	peer_packet_t * rpacket = calloc(1,sizeof(*rpacket));
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

	rpacket->head.type = PEER_PACKET;
	rpacket->head.index = 0xFFFF;
	rpacket->head.packet_len = sizeof(peer_packet_t);
	rpacket->p = 'p';
	rpacket->localaddr = localaddr;
	memset(rpacket->dev_name,'\0',sizeof(rpacket->dev_name));
	strcpy(rpacket->dev_name,"camera_0");
	
	spacket->sockfd = handle->local_socket;
	spacket->data = rpacket;
	spacket->length = sizeof(register_packet_t);
	spacket->to = handle->servaddr;

	spacket->type = RELIABLE_PACKET;
	spacket->is_resend = 0;
	spacket->resend_times = 0;
	spacket->index = 0xFFFF;
	spacket->ta.tv_sec = 0;
	spacket->ta.tv_usec = 800*1000;
	ret = send_push_msg(send_handle,spacket);
	if(ret != 0)
	{
		dbg_printf("send_push_msg is fail ! \n");
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



int  process_peer_ask(void * dev,void * arg)
{


	dbg_printf("process_peer_ask \n");
	int ret = -1;
	camera_handle_t * camera_dev = (camera_handle_t*)dev;
	if(NULL == camera_dev || NULL == camera_dev->send)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}
	net_send_handle_t * send_handle = camera_dev->send;
	socket_handle_t * handle = camera_dev->socket;
	struct sockaddr * src_addres = (struct sockaddr *)arg;
	packet_header_t * header =	(packet_header_t *)(src_addres+1); 
	peer_ask_packet_t * packet = (peer_ask_packet_t*)(header);
	if(NULL == packet)
	{
		dbg_printf("the packet is not right ! \n");
		return(-1);
	}
	send_remove_packet(send_handle,packet->head.index);

	if(0 != packet->head.ret)return(-1);
	loin_start(camera_dev,&packet->dev_addr);


	return(0);
}


int  process_loin_ask(void * dev,void * arg)
{


	dbg_printf("process_loin_ask \n");
	camera_handle_t * camera_dev = (camera_handle_t*)dev;
	if(NULL == camera_dev || NULL == camera_dev->send)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}
	net_send_handle_t * send_handle = camera_dev->send;
	
	struct sockaddr * src_addres = (struct sockaddr *)arg;
	packet_header_t * header =	(packet_header_t *)(src_addres+1); 
	loin_packet_ask_t * packet = (loin_packet_ask_t*)(header);
	if(NULL == packet)
	{
		dbg_printf("the packet is not right ! \n");
		return(-1);
	}

	if('c'+1 == packet->l)
	{
		dbg_printf("ccccccccccccccccccccccccccccc\n");
		loin_stop(camera_dev);
		if(packet->id_num == INVADE_ID_NUM)
		{
			dbg_printf("out of the number of client counts ! \n");
			return(-1);
		}
		camera_dev->id_num = packet->id_num;
		camera_dev->peeraddr = *src_addres;
		heartbeat_start(camera_dev);
		
	}
	else if('s'+1 == packet->l)
	{
		dbg_printf("sssssssssssssssssssssssssssssssss\n");

	}

	return(0);
}


int  process_pframe_packet(void * dev,void * arg)
{


	dbg_printf("process_peer_ask \n");
	int ret = -1;
	camera_handle_t * camera_dev = (camera_handle_t*)dev;
	if(NULL == camera_dev || NULL == camera_dev->send)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}
	net_send_handle_t * send_handle = camera_dev->send;
	socket_handle_t * handle = camera_dev->socket;
	struct sockaddr * src_addres = (struct sockaddr *)arg;
	packet_header_t * header =	(packet_header_t *)(src_addres+1); 
	pframe_packet_t * packet = (pframe_packet_t*)(header);

	



	return(0);
}




static handle_packet_fun_t pfun_recvsystem[] = {

	{REGISTER_PACKET,NULL},
	{REGISTER_PACKET_ASK,NULL},
	{PEER_PACKET,NULL},
	{PEER_PACKET_ASK,process_peer_ask},
	{LOIN_PACKET,NULL},
	{LOIN_PACKET_ASK,process_loin_ask},
	{BEATHEART_PACKET,NULL},
	{BEATHEART_PACKET_ASK,NULL},
	{IFRAME_PACKET,NULL},
	{PFRAME_PACKET,process_pframe_packet},
	{UNKNOW_PACKET,NULL},
		
};



handle_packet_fun_t *  get_handle_packet_fun(void)
{
	return(pfun_recvsystem);
}



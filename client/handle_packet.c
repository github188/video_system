#include "handle_packet.h"
#include "system_init.h"
#include "net_send.h"


#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"handle_packet:"


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

	#if 0

	loin_packet_t * rpacket_dev = calloc(1,sizeof(*rpacket_dev));
	if(NULL == rpacket_dev)
	{
		dbg_printf("calloc is fail ! \n");
		return(-1);
	}
	send_packet_t *spacket_dev = calloc(1,sizeof(*spacket_dev));
	if(NULL == spacket_dev)
	{
		dbg_printf("calloc is fail ! \n");
		free(rpacket_dev);
		rpacket_dev = NULL;
	}
	rpacket_dev->head.type = LOIN_PACKET;
	rpacket_dev->head.index = 0xFFFF;
	rpacket_dev->head.packet_len = sizeof(loin_packet_t);
	rpacket_dev->head.ret = 0;
	rpacket_dev->l = 'c';
	memmove(rpacket_dev->dev_name,packet->dev_name,sizeof(rpacket_dev->dev_name));

	rpacket_dev->dev_addr = packet->dev_addr;

	spacket_dev->sockfd = handle->local_socket;
	spacket_dev->data = rpacket_dev;
	spacket_dev->length = sizeof(loin_packet_t);
	spacket_dev->to = packet->dev_addr;

	spacket_dev->type = RELIABLE_PACKET;
	spacket_dev->is_resend = 0;
	spacket_dev->resend_times = 0;
	spacket_dev->index = 0xFFFF;
	spacket_dev->ta.tv_sec = 0;
	spacket_dev->ta.tv_usec = 800*1000;
	ret = send_push_msg(send_handle,spacket_dev);
	dbg_printf("999999999999999999999999999999999\n");
	if(ret != 0)
	{
		dbg_printf("netsend_push_msg is fail ! \n");
		free(rpacket_dev);
		rpacket_dev = NULL;
		free(spacket_dev);
		spacket_dev = NULL;
	}


	#endif
	
	
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
	}
	else if('s'+1 == packet->l)
	{
		dbg_printf("sssssssssssssssssssssssssssssssss\n");

	}

	
	return(0);
}


static handle_packet_fun_t pfun_recvsystem[] = {

	{REGISTER_PACKET,NULL},
	{REGISTER_PACKET_ASK,NULL},
	{PEER_PACKET,NULL},
	{PEER_PACKET_ASK,process_peer_ask},
	{LOIN_PACKET,NULL},
	{LOIN_PACKET_ASK,process_loin_ask},
	{HOLE_PACKET,NULL},
	{HOLE_PACKET_ASK,NULL},
	{ACTIVE_CHANNEL_PACKET,NULL},
	{ACTIVE_CHANNEL_ASK,NULL},
	{BEATHEART_PACKET,NULL},
	{BEATHEART_PACKET_ASK,NULL},
	{UNKNOW_PACKET,NULL},
		
};



handle_packet_fun_t *  get_handle_packet_fun(void)
{
	return(pfun_recvsystem);
}









#if 0



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
	spacket->length =sizeof(active_channle_t);
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

	system_info = socket_gethandle();
	
	return(0);
}


#endif
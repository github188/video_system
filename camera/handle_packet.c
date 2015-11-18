#include "handle_packet.h"
#include "system_init.h"
#include "net_send.h"


#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"handle_packet:"


int  send_register_packet(void * dev)
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
	rpacket->head.packet_len = sizeof(register_packet_t);
	rpacket->x = 'r';
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



int  process_register_ask(void * dev,void * arg)
{


	dbg_printf("process_register_ask \n");
	camera_handle_t * camera_dev = (camera_handle_t*)dev;
	if(NULL == camera_dev || NULL == camera_dev->send)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}

	net_send_handle_t * send_handle = camera_dev->send;
	
	struct sockaddr * src_addres = (struct sockaddr *)arg;
	packet_header_t * header =	(packet_header_t *)(src_addres+1); 
	register_ask_packet_t * packet = (register_ask_packet_t*)(header);
	if(NULL == packet)
	{
		dbg_printf("the packet is not right ! \n");
		return(-1);
	}
	send_remove_packet(send_handle,packet->head.index);
	
	return(0);
}



int  process_loin_packet(void * dev,void * arg)
{


	
	int ret = -1;
	int i = 0;
	camera_handle_t * camera_dev = (camera_handle_t*)dev;
	if(NULL == camera_dev || NULL == camera_dev->send)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}

	net_send_handle_t * send_handle = camera_dev->send;
	struct sockaddr * src_addres = (struct sockaddr *)arg;
	packet_header_t * header =	(packet_header_t *)(src_addres+1); 
	loin_packet_t * packet = (loin_packet_t*)(header);
	socket_handle_t * handle = camera_dev->socket;
	if(NULL == packet)
	{
		dbg_printf("the packet is not right ! \n");
		return(-1);
	}

	
	if('c' == packet->l)
	{

		dbg_printf("cccccccccccccccccccccccccccccccccccccccccc\n");
		int id_num = -1;
		for(i=0;i<MAX_USER; ++i)
		{
			if(0 == memcmp(&camera_dev->user[i]->addr,&packet->dev_addr,sizeof(packet->dev_addr)))	break;
	
			if(0 == camera_dev->user[i]->is_run)
			{
				id_num = i;
				camera_dev->user[i]->is_run = 1;
				camera_dev->user[i]->addr = packet->dev_addr;
				break;
			}
		}


		loin_packet_ask_t * rpacket = calloc(1,sizeof(*rpacket));
		if(NULL == rpacket)
		{
			dbg_printf("calloc is fail ! \n");
			return(-2);
		}

		send_packet_t *spacket = calloc(1,sizeof(*spacket));
		if(NULL == spacket)
		{
			dbg_printf("calloc is fail ! \n");
			free(rpacket);
			rpacket = NULL;
			return(-3);
		}

		rpacket->head.type = LOIN_PACKET_ASK;
		rpacket->head.index = packet->head.index;
		rpacket->head.packet_len = sizeof(loin_packet_ask_t);
		rpacket->l = 'c'+1;
		rpacket->id_num = id_num;
		

		spacket->sockfd = handle->local_socket;
		spacket->data = rpacket;
		spacket->length = sizeof(loin_packet_ask_t);

		spacket->to = *src_addres;
		spacket->type = UNRELIABLE_PACKET;
		ret = send_push_msg(send_handle,spacket);
		if(ret != 0)
		{
			dbg_printf("send_push_msg is fail ! \n");
			free(rpacket);
			rpacket = NULL;
			free(spacket);
			spacket = NULL;
		}	

	}
	else if('s' == packet->l)
	{

		dbg_printf("sssssssssssssssssssssssssssssssssss\n");
		loin_packet_ask_t * rpacket_server = calloc(1,sizeof(*rpacket_server));
		if(NULL == rpacket_server)
		{
			dbg_printf("calloc is fail ! \n");
			return(-2);
		}

		send_packet_t *spacket_server = calloc(1,sizeof(*spacket_server));
		if(NULL == spacket_server)
		{
			dbg_printf("calloc is fail ! \n");
			free(rpacket_server);
			rpacket_server = NULL;
			return(-3);
		}

		rpacket_server->head.type = LOIN_PACKET_ASK;
		rpacket_server->head.index = 0xFFFF;
		rpacket_server->head.packet_len = sizeof(loin_packet_ask_t);
		rpacket_server->l = 's'+1;
		rpacket_server->id_num = 0;
		

		spacket_server->sockfd = handle->local_socket;
		spacket_server->data = rpacket_server;
		spacket_server->length = sizeof(loin_packet_ask_t);

		spacket_server->to = packet->dev_addr;
		spacket_server->type = UNRELIABLE_PACKET;
		spacket_server->is_resend = 0;
		spacket_server->resend_times = 0;
		spacket_server->index = 0xFFFF;
		spacket_server->ta.tv_sec = 0;
		spacket_server->ta.tv_usec = 800*1000;
		ret = send_push_msg(send_handle,spacket_server);
		if(ret != 0)
		{
			dbg_printf("send_push_msg is fail ! \n");
			free(rpacket_server);
			rpacket_server = NULL;
			free(spacket_server);
			spacket_server = NULL;
		}	

	}



	return(0);


}


static handle_packet_fun_t pfun_recvsystem[] = {

	{REGISTER_PACKET,NULL},
	{REGISTER_PACKET_ASK,process_register_ask},
	{PEER_PACKET,NULL},
	{PEER_PACKET_ASK,NULL},
	{LOIN_PACKET,process_loin_packet},
	{LOIN_PACKET_ASK,NULL},
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
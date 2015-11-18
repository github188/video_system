#include "handle_packet.h"
#include "server.h"

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"handle_packet:"




static int  process_register_packet(void * dev_handle,void * pdata)
{


	int ret = -1;
	int flag = 0;
	if(NULL == pdata ||  NULL==dev_handle)
	{
		dbg_printf("please check the param ! \n");
		goto fail;
	}

	server_handle_t * handle = (server_handle_t *)dev_handle;
	struct sockaddr * src_addres = (struct sockaddr *)pdata;
	packet_header_t * header =	(packet_header_t *)(src_addres+1); 
	register_packet_t * packet = (register_packet_t*)(header);
	#if 1
	char * ipdev = NULL;
	int port_dev = 0;
	ipdev = socket_ntop(src_addres);
	if(NULL != ipdev)
	{
		dbg_printf("the dev_ip is %s \n",ipdev);
		free(ipdev);
		ipdev = NULL;
	}

	port_dev = socket_get_port(src_addres);
	dbg_printf("the dev port is %d \n",port_dev);
	

	#endif
	
	if('r' != packet->x || header->type != REGISTER_PACKET)
	{
		dbg_printf("this is not the right packet ! \n");
		return(-2);
	}

	char * pchar = strchr(packet->dev_name,'_');
	
	if(NULL == pchar || 0==is_digit(pchar+1))
	{
		dbg_printf("the dev name is not right ! \n");
		flag = 1;
	}
	int dev_index = atoi(pchar+1);
	if(dev_index >= DEVICE_MAX_NUM)
	{
		dbg_printf("the dev name out of the limit ! \n");
		flag = 2;
	}
	if(handle->dev[dev_index]->is_run)
	{
		dbg_printf("the dev has been register ! \n");
		flag = 3;
	}

	if(0 == flag)
	{
		handle->dev[dev_index]->is_run = 1;
		handle->dev[dev_index]->dev_addr = *src_addres;
		handle->dev[dev_index]->dev_localaddr = packet->localaddr;
		memmove(handle->dev[dev_index]->dev_name,packet->dev_name,sizeof(handle->dev[dev_index]->dev_name));
		dbg_printf("the dev name is %s \n",handle->dev[dev_index]->dev_name);
	}

	register_ask_packet_t * rpacket = calloc(1,sizeof(*rpacket));
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
	rpacket->head.type = REGISTER_PACKET_ASK;
	rpacket->head.index = packet->head.index;
	rpacket->head.packet_len = sizeof(rpacket->x);
	rpacket->head.ret = flag;
	rpacket->x = 'r'+1;
	rpacket->localaddr = packet->localaddr;
	memset(rpacket->dev_name,'\0',sizeof(rpacket->dev_name));
	memmove(rpacket->dev_name,packet->dev_name,sizeof(rpacket->dev_name));
	
	spacket->sockfd = handle->server_socket;
	spacket->data = rpacket;
	spacket->length = sizeof(register_ask_packet_t);
	spacket->to = *src_addres;

	spacket->type = UNRELIABLE_PACKET;
	ret = server_push_sendmsg(handle,spacket);
	
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

	return(-1);
	
}




#if 0






static int  handle_peer_packet(void * pdata)
{


	int ret = -1;
	int flag = 0;
	if(NULL == pdata ||  NULL==handle)
	{
		dbg_printf("please check the param ! \n");
		goto fail;
	}

	struct sockaddr * src_addres = (struct sockaddr *)pdata;
	packet_header_t * header =	(packet_header_t *)(src_addres+1); 
	peer_packet_t * packet = (peer_packet_t*)(header);
	

	if(header->type != PEER_PACKET)
	{
		dbg_printf("this is not the right packet ! \n");
		return(-2);
	}

	char * pchar = strchr(packet->dev_name,'_');
	
	if(NULL == pchar || 0==is_digit(pchar+1))
	{
		dbg_printf("the dev name is not right ! \n");
		flag = 1;
	}
	int dev_index = atoi(pchar+1);
	if(dev_index >= DEVICE_MAX_NUM)
	{
		dbg_printf("the dev name out of the limit ! \n");
		flag = 2;
	}

	if(NULL ==handle->dev[dev_index] || 0==handle->dev[dev_index]->is_run)
	{
		flag = 3;	
	}

	peer_ask_packet_t * rpacket = calloc(1,sizeof(*rpacket));
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
	rpacket->head.type = PEER_PACKET_ASK;
	rpacket->head.index = packet->head.index;
	rpacket->head.packet_len = sizeof(rpacket->dev_addr);
	rpacket->head.ret = flag;

	if(0 == flag)
	rpacket->dev_addr = handle->dev[dev_index]->dev_addr;

	
	spacket->sockfd = handle->server_socket;
	spacket->data = rpacket;
	spacket->length = sizeof(peer_ask_packet_t);
	spacket->to = *src_addres;

	spacket->type = UNRELIABLE_PACKET;
	ret = server_push_sendmsg(spacket);
	
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

	return(-1);
	
}






int send_hole_packet(struct sockaddr dev_addr,struct sockaddr to_addr)
{

	int ret = -1;
	int flag = 0;
	if(NULL==handle)
	{
		dbg_printf("please check the param ! \n");
		return(-1);
	}

	hole_packet_t * rpacket = calloc(1,sizeof(*rpacket));
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
	rpacket->head.type = HOLE_PACKET;
	rpacket->head.index = 0xFF;
	rpacket->head.packet_len = sizeof(rpacket->dev_addr);
	rpacket->head.ret = 0;
	rpacket->dev_addr = dev_addr;

	
	spacket->sockfd = handle->server_socket;
	spacket->data = rpacket;
	spacket->length = sizeof(hole_packet_t);
	spacket->to = to_addr;


	spacket->type = RELIABLE_PACKET;
	spacket->is_resend = 0;
	spacket->resend_times = 0;
	spacket->index = 0xFFFF;
	spacket->ta.tv_sec = 0;
	spacket->ta.tv_usec = 800*1000;
	ret = server_push_sendmsg(spacket);


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

	return(-1);


}



static int  handle_loin_packet(void * pdata)
{


	int ret = -1;
	int flag = 0;
	if(NULL == pdata ||  NULL==handle)
	{
		dbg_printf("please check the param ! \n");
		goto fail;
	}

	struct sockaddr * src_addres = (struct sockaddr *)pdata;
	packet_header_t * header =	(packet_header_t *)(src_addres+1); 
	loin_packet_t * packet = (loin_packet_t*)(header);

	#if 1
	char * client_ip = NULL;
	int client_dev = 0;
	client_ip = netlib_sock_ntop(src_addres);
	if(NULL != client_ip)
	{
		dbg_printf("the client_ip is %s \n",client_ip);
		free(client_ip);
		client_ip = NULL;
	}

	client_dev = netlib_sock_get_port(src_addres);
	dbg_printf("the client port is %d \n",client_dev);
	

	#endif
	
	if(header->type != LOIN_PACKET)
	{
		dbg_printf("this is not the right packet ! \n");
		return(-2);
	}

	char * pchar = strchr(packet->dev_name,'_');
	
	if(NULL == pchar || 0==is_digit(pchar+1))
	{
		dbg_printf("the dev name is not right ! \n");
		flag = 1;
		goto out;
	}
	int dev_index = atoi(pchar+1);
	if(dev_index >= DEVICE_MAX_NUM)
	{
		dbg_printf("the dev name out of the limit ! \n");
		flag = 2;
		goto out;
	}

	if(NULL ==handle->dev[dev_index])
	{
		flag = 3;
		goto out;
	}

	ret = memcmp(&((struct sockaddr_in *)&(handle->dev[dev_index]->dev_addr))->sin_addr,&((struct sockaddr_in *)&packet->dev_addr)->sin_addr,sizeof(struct in_addr));
	if(ret != 0 )
	{
		flag = 4;
		goto out;
	}

out:

	ret = 0;
	loin_packet_ask_t * rpacket = calloc(1,sizeof(*rpacket));
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
	rpacket->head.type = LOIN_PACKET_ASK;
	rpacket->head.index = packet->head.index;
	rpacket->head.packet_len = sizeof(rpacket->dev_addr);
	rpacket->head.ret = flag;

	if(0 == flag)
	rpacket->dev_addr = handle->dev[dev_index]->dev_addr;

	
	spacket->sockfd = handle->server_socket;
	spacket->data = rpacket;
	spacket->length = sizeof(loin_packet_ask_t);
	spacket->to =*src_addres;

	spacket->type = UNRELIABLE_PACKET;
	ret = server_push_sendmsg(spacket);
	
	if(ret != 0)
	{
		dbg_printf("netsend_push_msg is fail ! \n");
		goto fail;
	}

	if(0 == flag)
	{
		send_hole_packet(*src_addres,packet->dev_addr);
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

	return(-1);
	
}


int  handle_hole_ask(void * arg)
{

	struct sockaddr * src_addres = (struct sockaddr *)arg;
	packet_header_t * header =	(packet_header_t *)(src_addres+1); 
	hole_packet_ask_t * packet = (hole_packet_ask_t*)(header);
	

	if(NULL == packet)
	{
		dbg_printf("the packet is not right ! \n");
		return(-1);
	}
	netsend_remove_packet(packet->head.index);
	
	return(0);
}
#endif


static handle_packet_fun_t pfun_system[] = {

	{REGISTER_PACKET,process_register_packet},
	{REGISTER_PACKET_ASK,NULL},
	{PEER_PACKET,NULL},
	{PEER_PACKET_ASK,NULL},
	{LOIN_PACKET,NULL},
	{LOIN_PACKET_ASK,NULL},
	{HOLE_PACKET,NULL},
	{HOLE_PACKET_ASK,NULL},
	{BEATHEART_PACKET,NULL},
	{BEATHEART_PACKET_ASK,NULL},
	{UNKNOW_PACKET,NULL},
		
};



handle_packet_fun_t *  get_handle_packet_fun(void)
{

	return(pfun_system);

}
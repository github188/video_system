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
#include <sys/epoll.h>
#include <sys/time.h>
#include <time.h>
#include "data_packet.h"
#include "ring_queue.h"
#include "time_unitl.h"
#include "common.h"


#undef  DBG_ON
#undef  FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"01_udp_server:"



#define		RESEND_PACKET_MAX_NUM	(1024)
#define		RESEND_TIMES			(3)
#define		RESEND_TIME_INTERVAL	(800)  /*ms*/

#define	 SERVER_PORT	(8003u)


#define	 DEVICE_MAX_NUM			(4096u)


typedef  struct  dev_info
{
	int is_run;
	struct sockaddr dev_addr;
	char dev_name[60];
	
}dev_info_t;


typedef struct handle_fun
{
	packet_type_m type;
	int  (*handle_packet)(void * data);
}handle_fun_t;


typedef struct server_handle
{
	int server_socket;
	
	pthread_mutex_t mutex_server;
	pthread_cond_t cond_server;
	ring_queue_t server_msg_queue;
	volatile unsigned int server_msg_num;


	pthread_mutex_t mutex_send;
	pthread_cond_t cond_send;
	ring_queue_t send_msg_queue;
	volatile unsigned int send_msg_num;


	void * resend[RESEND_PACKET_MAX_NUM];
	volatile int packet_num; 
	pthread_mutex_t mutex_resend;
	pthread_cond_t cond_resend;
	volatile unsigned int resend_msg_num;

	dev_info_t * dev[DEVICE_MAX_NUM];
	
}server_handle_t;


static server_handle_t * handle = NULL;








static int server_socket_init(void * arg)
{

	int ret = -1;
	int value = 0;
	if(NULL == arg)
	{
		dbg_printf("please check the param ! \n");
		return(-1);
	}
	server_handle_t * server = (server_handle_t*)arg;

	server->server_socket = socket(AF_INET, SOCK_DGRAM, 0);

	struct sockaddr_in	servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERVER_PORT);
	int buff_size = 32 * 1024;
	setsockopt(server->server_socket, SOL_SOCKET, SO_RCVBUF, &buff_size, sizeof(buff_size));
	setsockopt(server->server_socket, SOL_SOCKET, SO_SNDBUF, (char*)&buff_size, sizeof(buff_size));

	value = fcntl(server->server_socket,F_GETFL,0);
	ret = fcntl(server->server_socket, F_SETFL, value|O_NONBLOCK);
	if(ret < 0)
	{
		dbg_printf("F_SETFL fail ! \n");
		return(-2);
	}
	fcntl(server->server_socket, F_SETFD, FD_CLOEXEC);
	ret = bind(server->server_socket, (struct sockaddr *) &servaddr, sizeof(servaddr));
	if(ret != 0 )
	{
		dbg_printf("bind is fail ! \n");
		return(-3);
	}

	return(0);	
}

	



static int server_init(void)
{

	int ret = -1;
	if(NULL != handle)
	{
		dbg_printf("handle has been init ! \n");
		return(-1);
	}
	handle = calloc(1,sizeof(1,*handle));
	if(NULL == handle)
	{
		dbg_printf("calloc is fail ! \n");
		return(-1);
	}
	ret = server_socket_init(handle);
	if(ret != 0)
	{
		dbg_printf("server_socket_init is fail ! \n");
		goto fail;
	}

	ret = ring_queue_init(&handle->server_msg_queue, 4096);
	if(ret < 0 )
	{
		dbg_printf("ring_queue_init  fail \n");
		return(-3);
	}
    pthread_mutex_init(&(handle->mutex_server), NULL);
    pthread_cond_init(&(handle->cond_server), NULL);
	handle->server_msg_num = 0;


	ret = ring_queue_init(&handle->send_msg_queue, 1024);
	if(ret < 0 )
	{
		dbg_printf("ring_queue_init  fail \n");
		return(-4);
	}
    pthread_mutex_init(&(handle->mutex_send), NULL);
    pthread_cond_init(&(handle->cond_send), NULL);
	handle->send_msg_num = 0;


	int i = 0;
	handle->packet_num = 0;
	for(i=0;i<RESEND_PACKET_MAX_NUM;++i)
		handle->resend[i] = NULL;
	 pthread_mutex_init(&(handle->mutex_resend), NULL);
	 pthread_cond_init(&(handle->cond_resend), NULL);
	 handle->resend_msg_num = 0;


	 
	
	for(i=0;i<DEVICE_MAX_NUM;++i)
	{
		handle->dev[i] = calloc(1,sizeof(dev_info_t));
		if(NULL == handle->dev[i])
		{
			dbg_printf("calloc is fail ! \n");
			goto fail;
		}
		handle->dev[i]->is_run = 0;
	}
		


	
	return(0);

fail:


	for(i=0;i<DEVICE_MAX_NUM;++i)
	{
		if(NULL != handle->dev[i])
		{
			free(handle->dev[i]);
			handle->dev[i] = NULL;
		}
	}
	
	if(NULL != handle)
	{
		free(handle);
		handle = NULL;
	}
	return(-1);
}


int  server_push_recvmsg(void * data )
{

	int ret = 1;
	int i = 0;
	if(NULL == handle)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}

	server_handle_t * server = handle;
	pthread_mutex_lock(&(server->mutex_server));

	for (i = 0; i < 1000; i++)
	{
	    ret = ring_queue_push(&server->server_msg_queue, data);
	    if (ret < 0)
	    {
	        usleep(i);
	        continue;
	    }
	    else
	    {
	        break;
	    }	
	}
    if (ret < 0)
    {
		pthread_mutex_unlock(&(server->mutex_server));
		return (-2);
    }
    else
    {
		volatile unsigned int *task_num = &server->server_msg_num;
    	fetch_and_add(task_num, 1);
    }
    pthread_mutex_unlock(&(server->mutex_server));
	pthread_cond_signal(&(server->cond_server));
	return(0);
}


int  server_push_sendmsg(void * data )
{

	int ret = 1;
	int i = 0;
	if(NULL == data || NULL == handle)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}

	server_handle_t * send = handle;

	pthread_mutex_lock(&(send->mutex_send));

	for (i = 0; i < 1000; i++)
	{
	    ret = ring_queue_push(&send->send_msg_queue, data);
	    if (ret < 0)
	    {
	        usleep(i);
	        continue;
	    }
	    else
	    {
	        break;
	    }	
	}
    if (ret < 0)
    {
		pthread_mutex_unlock(&(send->mutex_send));
		return (-2);
    }
    else
    {
		volatile unsigned int *task_num = &send->send_msg_num;
    	fetch_and_add(task_num, 1);
    }

    pthread_mutex_unlock(&(send->mutex_send));
	pthread_cond_signal(&(send->cond_send));
	return(0);
}



static int is_digit(char * str)
{
	int i = 0;
	if(NULL == str)return(0);

	for(i=0;i<strlen(str);++i)
	{
		if(str[i] >= '0' && str[i] <='9')continue;
		return(0);
	}
	return(1);
}


static int  handle_register_packet(void * pdata)
{


	int ret = -1;
	int flag = 0;
	if(NULL == pdata ||  NULL==handle)
	{
		dbg_printf("please check the param ! \n");
		goto fail;
	}

	register_packet_t * packet = (register_packet_t*)(pdata);
	packet_header_t * header =	(packet_header_t*)packet; 

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
		handle->dev[dev_index]->dev_addr = *(struct sockaddr*)(packet+1);
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
	memset(rpacket->dev_name,'\0',sizeof(rpacket->dev_name));
	memmove(rpacket->dev_name,packet->dev_name,sizeof(rpacket->dev_name));
	
	spacket->sockfd = handle->server_socket;
	spacket->data = rpacket;
	spacket->length = sizeof(register_ask_packet_t);
	spacket->to = *(struct sockaddr*)(packet+1);

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


static int  handle_peer_packet(void * pdata)
{


	int ret = -1;
	int flag = 0;
	if(NULL == pdata ||  NULL==handle)
	{
		dbg_printf("please check the param ! \n");
		goto fail;
	}

	peer_packet_t * packet = (peer_packet_t*)(pdata);
	packet_header_t * header =	(packet_header_t*)packet; 

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
	rpacket->dev_addr = handle->dev[dev_index]->dev_addr;

	
	spacket->sockfd = handle->server_socket;
	spacket->data = rpacket;
	spacket->length = sizeof(peer_ask_packet_t);
	spacket->to = *(struct sockaddr*)(packet+1);

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


static handle_fun_t pfun_system[] = {

	{REGISTER_PACKET,handle_register_packet},
	{REGISTER_PACKET_ASK,NULL},
	{PEER_PACKET,handle_peer_packet},
	{PEER_PACKET_ASK,NULL},
	{LOIN_PACKET,NULL},
	{LOIN_PACKET_ASK,NULL},
	{BEATHEART_PACKET,NULL},
	{BEATHEART_PACKET_ASK,NULL},
		
};


static void *  server_recv_pthread(void * arg)
{
	server_handle_t * server = (server_handle_t * )arg;
	if(NULL == server)
	{
		dbg_printf("please check the param ! \n");
		return(NULL);
	}

	int ret = -1;
	int is_run = 1;
	packet_header_t * header = NULL;
	int i = DUMP_PACKET;
	
	while(is_run)
	{
        pthread_mutex_lock(&(server->mutex_server));
        while (0 == server->server_msg_num)
        {
            pthread_cond_wait(&(server->cond_server), &(server->mutex_server));
        }
		ret = ring_queue_pop(&(server->server_msg_queue), (void **)&header);
		pthread_mutex_unlock(&(server->mutex_server));
		
		volatile unsigned int *handle_num = &(server->server_msg_num);
		fetch_and_sub(handle_num, 1);  
		
		if(ret != 0 || NULL == header)continue;

		for(i=0;i<sizeof(pfun_system)/sizeof(pfun_system[0]);++i)
		{
			if(header->type != pfun_system[i].type)continue;
			if(NULL != pfun_system[i].handle_packet)
			{
				pfun_system[i].handle_packet(header);
		
			}
		}

		if(NULL != header)
		{
			free(header);
			header = NULL;

		}



	}

	return(NULL);

}





static int server_disperse_packets(void * arg)
{
	if(NULL == arg)
	{
		dbg_printf("please check the param ! \n");
		return(-1);
	}
	server_handle_t * server = (server_handle_t *)arg;
	struct sockaddr_in from;
	socklen_t addr_len;
	int len = 0;
	int ret = -1;
	char buff[1500];
	packet_header_t * header = (packet_header_t*)(buff);
	len = recvfrom(server->server_socket,buff,sizeof(buff), 0 , (struct sockaddr *)&from ,&addr_len); 
	if(len <= 0 )
	{
		dbg_printf("the length is not right ! \n");
		return(-2);
	}

	if(header->type <= DUMP_PACKET || header->type >= UNKNOW_PACKET )
	{
		dbg_printf("the packet is not in the limit ! \n");
		return(-3);
	}


	void * data = calloc(1,sizeof(char)*len+sizeof(struct sockaddr_in)+1);
	if(NULL == data)
	{
		dbg_printf("calloc is fail ! \n");
		return(-4);
	}
	memmove(data,buff,len);
	memmove(data+sizeof(char)*len,&from,sizeof(struct sockaddr_in));

	ret = server_push_recvmsg(data);
	if(0 != ret)
	{
		dbg_printf("server_push_msg is fail ! \n");
		free(data);
		data = NULL;
	}
	

	return(0);
}



static void * server_send_pthread(void * arg)
{
	server_handle_t * send = (server_handle_t * )arg;
	if(NULL == send)
	{
		dbg_printf("please check the param ! \n");
		return(NULL);
	}

	int ret = -1;
	int is_run = 1;
	int count_send = 0;
	send_packet_t * packet = NULL;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	while(is_run)
	{
        pthread_mutex_lock(&(send->mutex_send));
        while (0 == send->send_msg_num)
        {
            pthread_cond_wait(&(send->cond_send), &(send->mutex_send));
        }
		ret = ring_queue_pop(&(send->send_msg_queue), (void **)&packet);
		pthread_mutex_unlock(&(send->mutex_send));
		
		volatile unsigned int *handle_num = &(send->send_msg_num);
		fetch_and_sub(handle_num, 1);  

		if(ret != 0 || NULL == packet)continue;

		count_send = 3;

		
		if(!packet->is_resend && RELIABLE_PACKET == packet->type)
		{
			packet_header_t * head = (packet_header_t*)packet->data;
			head->index = send->packet_num;
			packet->index = send->packet_num;
			
			send->packet_num += 1;
			if(send->packet_num >= RESEND_PACKET_MAX_NUM)
				send->packet_num = 0;
		}

		
		while(count_send --)
		{
			ret = sendto(packet->sockfd,packet->data,packet->length,0,(struct sockaddr *)&packet->to,addr_len);
			dbg_printf("ret =====  %d \n",ret);
			if(-1 == ret)
			{
				if(errno == EINTR || errno == EAGAIN)
				{
					usleep(1000);continue;
				}
			}
			break;
		}

		
		if(UNRELIABLE_PACKET == packet->type)
		{
			if(NULL != packet->data)
			{
				free(packet->data);
				packet->data = NULL;
			}
			free(packet);
			packet = NULL;
		}
		else
		{

			struct timeval time_pass;
			get_time(&time_pass);
			if(packet->is_resend)
			{
				dbg_printf("resend ! \n");
				pthread_mutex_lock(&(send->mutex_resend));
				if(packet->resend_times > RESEND_TIMES)
				{

					send->resend[packet->index] = NULL;

					if(NULL != packet->data)
					{
						free(packet->data);
						packet->data = NULL;
					}

					if(NULL != packet)
					{
						free(packet);
						packet = NULL;

					}

					volatile unsigned int *handle_num = &(send->resend_msg_num);
					fetch_and_sub(handle_num, 1);  
					
					dbg_printf("i free it ! \n");
				}
				else
				{
					add_time(&time_pass,&packet->ta,&packet->tp);	
				}
				pthread_mutex_unlock(&(send->mutex_resend));
				
			}
			else
			{
				dbg_printf("new realiable packet come ! \n");
				pthread_mutex_lock(&(send->mutex_resend));
				if(NULL != send->resend[packet->index])
				{

					
					send_packet_t * pre_packet = (send_packet_t*)send->resend[packet->index];
					if(NULL != pre_packet->data)
					{
						free(pre_packet->data);
						pre_packet->data = NULL;
					}

					if(NULL != pre_packet)
					{
						free(pre_packet);
						pre_packet = NULL;

					}
				}

				packet->is_resend = 1;
				packet->resend_times = 0;
				add_time(&time_pass,&packet->ta,&packet->tp);
				
				send->resend[packet->index] = packet;
				
				volatile unsigned int *task_num = &send->resend_msg_num;
				fetch_and_add(task_num, 1);
				pthread_mutex_unlock(&(send->mutex_resend));
				pthread_cond_signal(&(send->cond_resend));

			}
			

		}

	}

	return(NULL);

}


static void *  server_resend_pthread(void * arg)
{
	server_handle_t * send = (server_handle_t * )arg;
	if(NULL == send)
	{
		dbg_printf("please check the param ! \n");
		return(NULL);
	}

	int ret = -1;
	int is_run = 1;
	int i = 0;
	send_packet_t * packet = NULL;
	struct timeval time_pass;
	while(is_run)
	{
        pthread_mutex_lock(&(send->mutex_resend));
        while (0 == send->resend_msg_num)
        {
            pthread_cond_wait(&(send->cond_resend), &(send->mutex_resend));
        }
		
		ret = get_time(&time_pass);
		if(ret != 0)
		{
			dbg_printf("error !! \n");
			is_run = 0;
			break;
		}
		for(i=0;i<RESEND_PACKET_MAX_NUM;++i)
		{

			if(NULL == send->resend[i])continue;
			
			packet = (send_packet_t*)(send->resend[i]);
			if(timercmp(&time_pass,&packet->tp,>=))
			{
				packet->is_resend = 1;
				packet->resend_times += 1;
				server_push_sendmsg(send->resend[i]);
				
			}
		}
		pthread_mutex_unlock(&(send->mutex_resend));

		usleep(100*1000);

	}

	return(NULL);

}



int main(int argc,char * argv[])
{
	int ret = -1;
	ret = server_init();
	if(ret != 0)
	{
		dbg_printf("server_init is fail ! \n");
		return(-1);
	}

	int epfd = -1;
	struct epoll_event add_event;
	struct epoll_event events[4096];
	epfd=epoll_create(4096);
	add_event.data.fd = handle->server_socket;
	add_event.events = EPOLLIN | EPOLLET;
	epoll_ctl (epfd, EPOLL_CTL_ADD, handle->server_socket, &add_event);

	pthread_t process_packet_pthed;
	ret = pthread_create(&process_packet_pthed,NULL,server_recv_pthread,handle);

	pthread_t server_send_ptid;
	ret = pthread_create(&server_send_ptid,NULL,server_send_pthread,handle);
	pthread_detach(server_send_ptid);

	pthread_t server_resend_ptid;
	ret = pthread_create(&server_resend_ptid,NULL,server_resend_pthread,handle);
	pthread_detach(server_resend_ptid);
	
	int is_run = 1;
	int nevents=0;
	int i=0;
	while(is_run)
	{
		nevents= epoll_wait(epfd, events, 4096, -1);
		for(i=0;i<nevents;++i)
		{

			if(!(events[i].events & EPOLLIN))
			{
				dbg_printf("unknow events ! \n");
				continue;
			}
			if(handle->server_socket == events[i].data.fd)
			{
				server_disperse_packets(handle);

			}

		}
	}

	pthread_join(process_packet_pthed,NULL);


	return(0);
}




#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <time.h>
#include <sys/epoll.h>


#include "eventfd.h"
#include "common.h"
#include "net_recv.h"
#include "eventfd.h"
#include "ring_queue.h"
#include "data_packet.h"
#include "handle_packet.h"
#include "system_init.h"




#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"net_recv:"


typedef struct handle_recvfun
{
	packet_type_m type;
	int  (*handle_packet)(void * data);
}handle_recvfun_t;


typedef struct net_recv_handle
{
	int event_fd;
	int recv_socket;
	int servce_socket;
	pthread_mutex_t mutex_recv;
	pthread_cond_t cond_recv;
	ring_queue_t recv_msg_queue;
	volatile unsigned int recv_msg_num;

}net_recv_handle_t;



static net_recv_handle_t * recv_handle = NULL;



int net_recv_init(void)
{


	int ret = -1;
	if(NULL != recv_handle)
	{
		dbg_printf("recv_handle  has been init ! \n");
		return(-1);
	}

	recv_handle = calloc(1,sizeof(*recv_handle));
	if(NULL == recv_handle)
	{
		dbg_printf("calloc is fail ! \n");
		return(-2);
	}


	recv_handle->event_fd = eventfd_new();
	if(recv_handle->event_fd < 0)
	{
		dbg_printf("eventfd_new fail ! \n");
		goto fail;
	}


	system_handle_t * syshandle = (system_handle_t*)system_gethandle();
	if(NULL == syshandle)
	{
		dbg_printf("get syshandle is fail ! \n");
		goto fail;
	}
	recv_handle->recv_socket = syshandle->local_socket;
	recv_handle->servce_socket = syshandle->servce_socket;


	ret = ring_queue_init(&recv_handle->recv_msg_queue, 2048);
	if(ret < 0 )
	{
		dbg_printf("ring_queue_init  fail \n");
		goto fail;
	}
    pthread_mutex_init(&(recv_handle->mutex_recv), NULL);
    pthread_cond_init(&(recv_handle->cond_recv), NULL);
	recv_handle->recv_msg_num = 0;
	
	return(0);

fail:



	if(recv_handle->event_fd > 0)
	{
		close(recv_handle->event_fd);
		recv_handle->event_fd  = -1;
	}
	
	if(NULL != recv_handle)
	{
		free(recv_handle);
		recv_handle = NULL;
	}



	return(-1);
	

}



static int  netrecv_push_msg(void * data )
{

	int ret = 1;
	int i = 0;
	if(NULL == recv_handle)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}

	net_recv_handle_t * recv = recv_handle;
	pthread_mutex_lock(&(recv->mutex_recv));

	for (i = 0; i < 1000; i++)
	{
	    ret = ring_queue_push(&recv->recv_msg_queue, data);
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
		pthread_mutex_unlock(&(recv->mutex_recv));
		return (-2);
    }
    else
    {
		volatile unsigned int *task_num = &recv->recv_msg_num;
    	fetch_and_add(task_num, 1);
    }
    pthread_mutex_unlock(&(recv->mutex_recv));
	pthread_cond_signal(&(recv->cond_recv));
	return(0);
}



static int recv_socket_fun(void * arg,int socketfd)
{

	int ret = -1;
	if(NULL == arg)
	{
		dbg_printf("please check the param ! \n");
		return(-1);
	}
	net_recv_handle_t  * handle = (net_recv_handle_t*)arg;

	if(handle->recv_socket != socketfd && handle->servce_socket != socketfd )
	{
		dbg_printf("unknow socketfd ! \n");
		return(-2);
	}

	struct sockaddr_in from;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	int len = 0;
	char buff[1500];
	packet_header_t * header = (packet_header_t*)(buff);
	len = recvfrom(socketfd,buff,sizeof(buff), 0 , (struct sockaddr *)&from ,&addr_len); 

	if(len <= 0 )
	{
		dbg_printf("the length is not right ! \n");
		return(-2);
	}
	if(header->type <= DUMP_PACKET || header->type >= UNKNOW_PACKET)
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
	ret = netrecv_push_msg(data);
	if(ret < 0 )
	{
		if(NULL != data)
		{
			free(data);
			data = NULL;
		}
		return(-1);
	}
	return(0);
}







static void * recv_pthread(void * arg)
{

	net_recv_handle_t  * handle = (net_recv_handle_t*)arg;
	if(NULL == handle || handle->servce_socket<0 || handle->recv_socket<0)
	{
		dbg_printf("check the param ! \n");
		return(NULL);
	}

	int epfd = -1;
	struct epoll_event add_event;
	struct epoll_event events[32];
	epfd=epoll_create(16);

	add_event.data.fd = handle->event_fd;
	add_event.events = EPOLLIN | EPOLLET;
	epoll_ctl (epfd, EPOLL_CTL_ADD, handle->event_fd, &add_event);

	add_event.data.fd = handle->servce_socket;
	add_event.events = EPOLLIN | EPOLLET;
	epoll_ctl (epfd, EPOLL_CTL_ADD, handle->servce_socket, &add_event);


	add_event.data.fd = handle->recv_socket;
	add_event.events = EPOLLIN | EPOLLET;
	epoll_ctl (epfd, EPOLL_CTL_ADD, handle->recv_socket, &add_event);
	

	int is_run = 1;
	int nevents = 0;
	int i = 0;
	while(is_run)
	{
		nevents= epoll_wait(epfd, events, 32, -1);
		for(i=0;i<nevents;++i)
		{
			if(!(events[i].events & EPOLLIN))
			{
				dbg_printf("unknow events ! \n");
				continue;
			}
			if(handle->event_fd == events[i].data.fd)
			{
				eventfd_clean(handle->event_fd);
			}
			else
			{
				recv_socket_fun(handle,events[i].data.fd);
			}
		}

	}
}





static handle_recvfun_t pfun_recvsystem[] = {

	{REGISTER_PACKET,NULL},
	{REGISTER_PACKET_ASK,handle_register_ask},
	{LOIN_PACKET,NULL},
	{LOIN_PACKET_ASK,NULL},
	{BEATHEART_PACKET,NULL},
	{BEATHEART_PACKET_ASK,NULL},
		
};


static void *  netrecv_pthread_fun(void * arg)
{
	net_recv_handle_t * recv = (net_recv_handle_t * )arg;
	if(NULL == recv)
	{
		dbg_printf("please check the param ! \n");
		return(NULL);
	}

	int ret = -1;
	int is_run = 1;
	int i = 0;
	packet_header_t * header = NULL;
	
	while(is_run)
	{
        pthread_mutex_lock(&(recv->mutex_recv));
        while (0 == recv->recv_msg_num)
        {
            pthread_cond_wait(&(recv->cond_recv), &(recv->mutex_recv));
        }
		ret = ring_queue_pop(&(recv->recv_msg_queue), (void **)&header);
		pthread_mutex_unlock(&(recv->mutex_recv));
		
		volatile unsigned int *handle_num = &(recv->recv_msg_num);
		fetch_and_sub(handle_num, 1);  

		if(ret != 0 || NULL == header)continue;
		
		for(i=0;i<sizeof(pfun_recvsystem)/sizeof(pfun_recvsystem[0]);++i)
		{
			if(header->type != pfun_recvsystem[i].type)continue;
			if(NULL != pfun_recvsystem[i].handle_packet)
			{
				pfun_recvsystem[i].handle_packet(header);
		
			}
			
		}

		free(header);
		header = NULL;


	}

	return(NULL);

}



int netrecv_start_up(void)
{
	int ret = -1;
	ret = net_recv_init();
	if(ret != 0 )
	{
		dbg_printf("net_recv_init is fail ! \n");
		return(-1);
	}

	pthread_t netrecv_ptid;
	ret = pthread_create(&netrecv_ptid,NULL,recv_pthread,recv_handle);
	pthread_detach(netrecv_ptid);

	pthread_t netprocess_ptid;
	ret = pthread_create(&netprocess_ptid,NULL,netrecv_pthread_fun,recv_handle);
	pthread_detach(netprocess_ptid);


	return(0);
}


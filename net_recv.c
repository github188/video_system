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



#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"main"


typedef struct net_recv_handle
{
	int event_fd;
	int recv_socket;
	int servce_socket;

}net_recv_handle_t;



static net_recv_handle_t * recv_handle = NULL;



int net_recv_init(void)
{
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
		return(-3);
	}

	recv_handle->recv_socket = -1;
	recv_handle->servce_socket = -1;
	
	return(0);

fail:
	


	if(NULL != recv_handle)
	{
		free(recv_handle);
		recv_handle = NULL;
	}

	return(-1);
	

}




static int recv_socket_fun(void * arg)
{
	if(NULL == arg)
	{
		dbg_printf("please check the param ! \n");
		return(-1);
	}
	net_recv_handle_t  * handle = (net_recv_handle_t*)arg;
	/*套接字字要设置成非阻塞模式*/

	struct sockaddr_in from;
	socklen_t addr_len;
	int len = 0;
	char buff[1500];
	len = recvfrom(handle->recv_socket,buff,sizeof(buff), 0 , (struct sockaddr *)&from ,&addr_len); 
	//if(len <= 0 )continue;
	

	return(0);
}



static int servce_socket_fun(void * arg)
{
	if(NULL == arg)
	{
		dbg_printf("please check the param ! \n");
		return(-1);
	}
	net_recv_handle_t  * handle = (net_recv_handle_t*)arg;

	struct sockaddr_in from;
	socklen_t addr_len;
	int len = 0;
	char buff[1500];
	len = recvfrom(handle->servce_socket,buff,sizeof(buff), 0 , (struct sockaddr *)&from ,&addr_len); 
	//if(len <= 0 )continue;
	
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
			}

			if(handle->recv_socket == events[i].data.fd)
			{
				recv_socket_fun(handle);
			}
			else if(handle->servce_socket == events[i].data.fd)
			{
				servce_socket_fun(handle);	

			}
			else if(handle->event_fd == events[i].data.fd)
			{
				eventfd_clean(handle->event_fd);	

			}
			

		}

	}

}




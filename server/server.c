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
#include "data_packet.h"
#include "ring_queue.h"
#include "common.h"


#undef  DBG_ON
#undef  FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"01_udp_server:"




#undef MAXLINE
#define MAXLINE   1024

#define	 SERVER_PORT	(8003u)


typedef struct server_handle
{
	int server_socket;
	
	pthread_mutex_t mutex_server;
	pthread_cond_t cond_server;
	ring_queue_t server_msg_queue;
	volatile unsigned int server_msg_num;
	
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

	return(0);

fail:

	if(NULL != handle)
	{
		free(handle);
		handle = NULL;
	}
	return(-1);
}


int  server_push_msg(void * data )
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
		

		free(header);
		header = NULL;


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
	server_handle_t * server = (server_handle_t *)handle;
	struct sockaddr_in from;
	socklen_t addr_len;
	int len = 0;
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
	dbg_printf("type %d  index==%d length==%d total_length==%d\n",header->type,header->index,header->packet_len,len);

	
	

#if 0
	void * data = calloc(1,sizeof(char)*len+1);
	if(NULL == data)
	{
		dbg_printf("calloc is fail ! \n");
		return(-4);
	}
	memmove(data,buff,len);
#endif
	


	return(0);
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
				dbg_printf("the data is coming123 ! \n");
				
				#if 0
					int			n;
					socklen_t	len = sizeof(struct sockaddr);
					char		mesg[MAXLINE];
					struct sockaddr pcliaddr;
					n = recvfrom(handle->server_socket, mesg, MAXLINE, 0, &pcliaddr, &len);
				#else
				server_disperse_packets(handle);

				#endif
				

			}

		}




	}

	pthread_join(process_packet_pthed,NULL);


	return(0);
}



#if 0
void dg_echo(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen)
{
	int			n;
	socklen_t	len;
	char		mesg[MAXLINE];

	for ( ; ; ) 
	{
		len = clilen;
		n = recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);
		dbg_printf("the data is coming ! \n");

	}
}


int main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_in	servaddr, cliaddr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERVER_PORT);
	int n = 0;
	n = 220 * 1024;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n));

	bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	dg_echo(sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
}

#endif
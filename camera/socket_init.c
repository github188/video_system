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
#include "socket_init.h"
#include "link_net.h"
#include "system_init.h"



#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"socket_init"


#define	 SERVER_PORT	(8003u)
#define  SERVER_ADDRES	"209.141.43.239"

#define	 LOCAL_PORT		(8002u)



void * socket_socket_new(void)
{

	int ret = -1;
	int value = 0;
	socket_handle_t * handle = calloc(1,sizeof(*handle));
	if(NULL == handle)
	{
		dbg_printf("calloc is fail ! \n");
		return(NULL);
	}

	bzero(&handle->servaddr, sizeof(handle->servaddr));
	struct sockaddr_in * addr = (struct sockaddr_in *)&handle->servaddr;
	
	addr->sin_family = AF_INET;
	addr->sin_port = htons(SERVER_PORT);
	ret = inet_pton(AF_INET,SERVER_ADDRES,&addr->sin_addr);
	if(ret < 0)
	{
		dbg_printf("inet_pton fail ! \n");
		goto fail;
	}


	handle->local_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(handle->local_socket < 0)
	{
		dbg_printf("handle->local_socket  is fail ! \n");
		goto fail;
	}

	value = fcntl(handle->local_socket,F_GETFL,0);
	ret = fcntl(handle->local_socket, F_SETFL, value|O_NONBLOCK);
	if(ret < 0)
	{
		dbg_printf("F_SETFL fail ! \n");
		goto fail;
	}
	
	int buffer_size = 32*1024;
	setsockopt(handle->local_socket, SOL_SOCKET, SO_RCVBUF, (char*)&buffer_size, sizeof(buffer_size));
	setsockopt(handle->local_socket, SOL_SOCKET, SO_SNDBUF, (char*)&buffer_size, sizeof(buffer_size));
	int optlocal=1;
	setsockopt(handle->local_socket,SOL_SOCKET,SO_REUSEADDR,&optlocal,sizeof(optlocal));

	fcntl(handle->local_socket, F_SETFD, FD_CLOEXEC);

	struct sockaddr_in	local_addr;
	bzero(&local_addr, sizeof(local_addr));
	local_addr.sin_family      = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port        = htons(LOCAL_PORT);
	bind(handle->local_socket, (struct sockaddr *) &local_addr, sizeof(local_addr));

	ret = get_net_type();
	if(0/*0 == ret */)/*wifi*/
	{
		get_local_addr("wlan0",&handle->localaddr);	
	}
	else
	{
		get_local_addr("eth0",&handle->localaddr);
	}
	struct sockaddr_in * localaddr = (struct sockaddr_in *)&handle->localaddr;
	localaddr->sin_port = htons(LOCAL_PORT);
	

	return(handle);


fail:

	if(handle->local_socket > 0 )
	{
		close(handle->local_socket);

	}
	if(NULL != handle)
	{
		free(handle);
		handle = NULL;
	}

	return(NULL);
	
}



void socket_socket_destroy(void * arg)
{
	if(NULL == arg)
	{
		dbg_printf("free the null socket ! \n");
		return;
	}
	socket_handle_t * handle = (socket_handle_t*)arg;
	if(handle->local_socket > 0 )
	{
		close(handle->local_socket);

	}
	if(NULL != handle)
	{
		free(handle);
		handle = NULL;
	}

}



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
#include "system_init.h"



#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"system_init"


#define	 SERVER_PORT	(8003u)
#define  SERVER_ADDRES	"209.141.43.239"

#define	 LOCAL_PORT		(8002u)



static  system_handle_t * handle_system = NULL;

static int system_socket_init(void * arg)
{

	int ret = -1;
	int value = 0;
	if(NULL == arg)
	{
		dbg_printf("please check the param ! \n");
		return(-1);
	}
	system_handle_t * handle = (system_handle_t*)arg;

	bzero(&handle->servaddr, sizeof(handle->servaddr));
	struct sockaddr_in * addr = (struct sockaddr_in *)&handle->servaddr;
	
	addr->sin_family = AF_INET;
	addr->sin_port = htons(SERVER_PORT);
	ret = inet_pton(AF_INET,SERVER_ADDRES,&addr->sin_addr);
	if(ret < 0)
	{
		dbg_printf("inet_pton fail ! \n");
		return(-2);
	}
	handle->servce_socket = socket(AF_INET,SOCK_DGRAM,0);
	if(handle->servce_socket < 0)
	{
		dbg_printf("create servce_socket  fail ! \n");
		return(-3);
	}
	int optserver=1;
	setsockopt(handle->servce_socket,SOL_SOCKET,SO_REUSEADDR,&optserver,sizeof(optserver));
	
	value = fcntl(handle->servce_socket,F_GETFL,0);
	ret = fcntl(handle->servce_socket, F_SETFL, value|O_NONBLOCK);
	if(ret < 0)
	{
		dbg_printf("F_SETFL fail ! \n");
		return(-3);
	}
	fcntl(handle->servce_socket, F_SETFD, FD_CLOEXEC);

#if 1
	struct sockaddr_in	addr_in;
	bzero(&addr_in, sizeof(addr_in));
	addr_in.sin_family      = AF_INET;
	addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_in.sin_port        = htons(LOCAL_PORT);
	bind(handle->servce_socket, (struct sockaddr *) &addr_in, sizeof(addr_in));

#endif


	handle->local_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(handle->local_socket < 0)
	{
		dbg_printf("handle->local_socket  is fail ! \n");
		return(-3);
	}

	value = fcntl(handle->local_socket,F_GETFL,0);
	ret = fcntl(handle->local_socket, F_SETFL, value|O_NONBLOCK);
	if(ret < 0)
	{
		dbg_printf("F_SETFL fail ! \n");
		return(-3);
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

	return(0);

	
}




void * system_gethandle(void)
{
	if(NULL == handle_system)
	{
		dbg_printf("please init the system handle ! \n");
		return(NULL);
	}
	return(handle_system);
}




int system_init(void)
{

	int ret = -1;
	if(NULL != handle_system)
	{
		dbg_printf("the system has been init ! \n");
		return(-1);
	}

	handle_system = calloc(1,sizeof(*handle_system));
	if(NULL == handle_system)
	{
		dbg_printf("calloc is fail ! \n");
		return(-1);
	}

	ret = system_socket_init(handle_system);
	if(0 != ret)
	{
		dbg_printf("system_socket_init is fail ! \n");
		goto fail;
	}

	return(0);
	

fail:

	if(NULL != handle_system)
	{
		free(handle_system);
		handle_system = NULL;
	}
	return(-1);
}



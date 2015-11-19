#ifndef _socket_init_h
#define _socket_init_h


#include <sys/types.h>
#include <sys/socket.h>



typedef struct socket_handle
{
	int local_socket;
	struct sockaddr	servaddr;
	struct sockaddr localaddr;

}socket_handle_t;


void * socket_socket_new(void);
void socket_socket_destroy(void * arg);


#endif  /*_socket_init_h*/


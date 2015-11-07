#ifndef _system_init_h
#define _system_init_h


#include <sys/types.h>
#include <sys/socket.h>


typedef struct system_handle
{
	int local_socket;
	int servce_socket;
	struct sockaddr	servaddr;

}system_handle_t;


void * system_gethandle(void);
int system_init(void);



#endif  /*_system_init_h*/


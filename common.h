#ifndef  _COMMON_H
#define  _COMMON_H



#undef  DBG_ON
#undef  FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"common:"
#define 	dbg_printf(fmt,arg...) \
	do{if(DBG_ON)fprintf(stderr,FILE_NAME"%s(line=%d)->"fmt,__FUNCTION__,__LINE__,##arg);}while(0)


#define	 anyka_print		dbg_printf	


#define err_abort(code,text) do { \
	fprintf (stderr, "%s at \"%s\":%d: %s\n", \
	text, __FILE__, __LINE__, strerror (code)); \
	abort (); \
	} while (0)

	
#define errno_abort(text) do { \
	fprintf (stderr, "%s at \"%s\":%d: %s\n", \
	text, __FILE__, __LINE__, strerror (errno)); \
	abort (); \
	} while (0)
	

typedef void * (* pthread_fun)(void * arg);


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
#include <sys/time.h>
#include <time.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <sys/epoll.h>


char * socket_ntop(struct sockaddr *sa);
int  socket_get_port(const struct sockaddr *sa);
int get_local_addr(const char * interface,struct sockaddr * addr);
int is_digit(char * str);

#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/eventfd.h>

#include "eventfd.h"
#include "common.h"


#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"eventfd"


int eventfd_new(void)
{
	int evnet_fd = eventfd(0, EFD_NONBLOCK);
	if(evnet_fd < 0 )
	{
		dbg_printf("eventfd is fail  ! \n");
		return(-1);
	}
	return(evnet_fd);
}




int eventfd_wakeup(int fd)
{

	uint64_t value = 1;
	int nbytes = 0;
	if(fd <= 0)
	{
		dbg_printf("check the param \n");
		return(-1);
	}
	nbytes = write(fd, &value, sizeof(value));
	return(nbytes);
}




int eventfd_clean(int fd)
{

	uint64_t value = 0;
	int ret = -1;
	if(fd < 0)
	{
		dbg_printf("check the param \n");
		return(-1);
	}
	do
	{
		ret = read(fd,&value,sizeof(value));
	}while(value !=0 && ret>0);

	return(value);
}
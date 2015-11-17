
#include "common.h"


char * socket_ntop(struct sockaddr *sa)
{

	if(NULL == sa  )
	{
		dbg_printf("check the param \n");
		return(NULL);
	}
	#define  DATA_LENGTH	128
	
	char * str  = (char*)calloc(1,sizeof(char)*DATA_LENGTH);
	if(NULL == str)
	{
		dbg_printf("calloc is fail\n");
		return(NULL);
	}

	switch(sa->sa_family)
	{
		case AF_INET:
		{

			struct sockaddr_in * sin = (struct sockaddr_in*)sa;

			if(inet_ntop(AF_INET,&(sin->sin_addr),str,DATA_LENGTH) == NULL)
			{
				free(str);
				str = NULL;
				return(NULL);	
			}
			return(str);

		}
		default:
		{
			free(str);
			str = NULL;
			return(NULL);
		}


	}
			
    return (NULL);
}



int  socket_get_port(const struct sockaddr *sa)
{
	if(NULL == sa)
	{
		dbg_printf("check the param\n");
		return(-1);
	}
	switch (sa->sa_family)
	{
		case AF_INET: 
		{
			struct sockaddr_in	*sin = (struct sockaddr_in *) sa;

			return(ntohs(sin->sin_port));
		}


	}

    return(-1);
}



unsigned long socket_get_ip(const char * interface)
{
	
	if(NULL == interface)
	{
		dbg_printf("check the param ! \n");
		return(0);
	}

	int ret = -1;
    int inet_sock;
    struct ifreq ifr;
	inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
	ret = ioctl(inet_sock, SIOCGIFADDR, &ifr);
	if(ret < 0 )
	{
		dbg_printf("ioctl is fail ! \n");
		return(0);
	}
	close(inet_sock);
	return(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr.s_addr);

}



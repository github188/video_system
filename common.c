
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



int get_local_addr(const char * interface,struct sockaddr * addr)
{
	
	if(NULL == interface || NULL == addr)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}

	int ret = -1;
    int inet_sock;
    struct ifreq ifr;
	inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&ifr, sizeof(struct ifreq)); 
	strcpy(ifr.ifr_name, interface); 
	ret = ioctl(inet_sock, SIOCGIFADDR, &ifr);
	if(ret < 0 )
	{
		dbg_printf("ioctl is fail ! \n");
		return(-2);
	}
	close(inet_sock);

	*addr = ifr.ifr_addr;
	return(0);

}


int is_digit(char * str)
{
	int i = 0;
	if(NULL == str)return(0);

	for(i=0;i<strlen(str);++i)
	{
		if(str[i] >= '0' && str[i] <='9')continue;
		return(0);
	}
	return(1);
}
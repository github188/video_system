#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#include "common.h"
#include "ring_queue.h"
#include "handle_packet.h"
#include "handle_packet.h"



#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"process_loin:"




typedef struct process_loin_handle
{

	pthread_mutex_t mutex_loin;
	pthread_cond_t cond_loin;
	volatile unsigned int is_run;
	struct sockaddr addr; 

}process_loin_handle_t;



static process_loin_handle_t * loin_handle = NULL;


#if 1

char * netlib_sock_ntop(struct sockaddr *sa)
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


int  netlib_sock_get_port(const struct sockaddr *sa)
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

#endif


int  loin_init(void)
{

	int ret = 0;
	if(NULL != loin_handle)
	{
		dbg_printf("loin_handle has been init ! \n");
		return(-1);
	}

	loin_handle = (process_loin_handle_t*)calloc(1,sizeof(*loin_handle));
	if(NULL == loin_handle)
	{
		dbg_printf("calloc is fail ! \n");
		return(-2);
	}

	 pthread_mutex_init(&(loin_handle->mutex_loin), NULL);
	 pthread_cond_init(&(loin_handle->cond_loin), NULL);
	 loin_handle->is_run = 0;
	 
	return(0);
}





int loin_start_process(void * addr)
{

	process_loin_handle_t * handle = (process_loin_handle_t*)loin_handle;	
	if(NULL == handle)
	{
		dbg_printf("check the handle ! \n");
		return(-1);
	}
	pthread_mutex_lock(&(handle->mutex_loin));
	volatile unsigned int *task_num = &handle->is_run;
	compare_and_swap(task_num, 0,1);
	handle->addr = *((struct sockaddr*)addr);
	pthread_mutex_unlock(&(handle->mutex_loin));
	pthread_cond_signal(&(handle->cond_loin));

	return(0);
}



int loin_stop_process(void * addr)
{

	process_loin_handle_t * handle = (process_loin_handle_t*)loin_handle;	
	if(NULL == handle)
	{
		dbg_printf("check the handle ! \n");
		return(-1);
	}
	pthread_mutex_lock(&(handle->mutex_loin));
	volatile unsigned int *task_num = &handle->is_run;
	compare_and_swap(task_num, 1,0);
	pthread_mutex_unlock(&(handle->mutex_loin));

	return(0);
}



static void * loin_pthread(void * arg)
{
	process_loin_handle_t * handle = (process_loin_handle_t*)arg;	
	if(NULL == handle)
	{
		dbg_printf("please check  the param ! \n");
		return(NULL);
	}

	int is_run = 1;
	while(is_run)
	{
        pthread_mutex_lock(&(handle->mutex_loin));
        while (0 == handle->is_run)
        {
            pthread_cond_wait(&(handle->cond_loin), &(handle->mutex_loin));
        }
		pthread_mutex_unlock(&(handle->mutex_loin));
		send_active_channel_packet(&handle->addr);
		usleep(1000*2000);
		#if 1
		char * ipdev = NULL;
		int port_dev = 0;
		ipdev = netlib_sock_ntop(&handle->addr);
		if(NULL != ipdev)
		{
			dbg_printf("the dev_ip is %s \n",ipdev);
			free(ipdev);
			ipdev = NULL;
		}

		port_dev = netlib_sock_get_port(&handle->addr);
		dbg_printf("the dev port is %d \n",port_dev);
		

		#endif
	}



	return(NULL);
}




int loin_process_init(void)
{
	int ret = -1;
	ret = loin_init();
	if(ret != 0)
	{
		dbg_printf("loin_init is  fail ! \n");
		return(-1);
	}

	pthread_t loin_pthid;
	ret = pthread_create(&loin_pthid,NULL,loin_pthread,loin_handle);
	pthread_detach(loin_pthid);
	


	return(0);
}

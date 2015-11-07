#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <time.h>


#include "data_packet.h"
#include "ring_queue.h"
#include "net_send.h"
#include "time_unitl.h"


#include "common.h"

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"net_send"



#define		RESEND_PACKET_MAX_NUM	(1024)
#define		RESEND_TIMES			(3)
#define		RESEND_TIME_INTERVAL	(800)  /*ms*/

typedef struct net_send_handle
{
	pthread_mutex_t mutex_send;
	pthread_cond_t cond_send;
	ring_queue_t send_msg_queue;
	volatile unsigned int send_msg_num;

	
	void * resend[RESEND_PACKET_MAX_NUM];
	volatile long packet_num; 
	pthread_mutex_t mutex_resend;
	pthread_cond_t cond_resend;
	volatile unsigned int resend_msg_num;
	

}net_send_handle_t;



static  net_send_handle_t * send_handle = NULL;



static int netsend_handle_init(void)
{
	int ret = -1;
	int i = 0;
	if(NULL != send_handle)
	{
		dbg_printf("has been init ! \n");
		return(-1);
	}
	send_handle = calloc(1,sizeof(*send_handle));
	if(NULL == send_handle)
	{
		dbg_printf("calloc fail ! \n");
		return(-2);
	}
	ret = ring_queue_init(&send_handle->send_msg_queue, 1024);
	if(ret < 0 )
	{
		dbg_printf("ring_queue_init  fail \n");
		return(-3);
	}
    pthread_mutex_init(&(send_handle->mutex_send), NULL);
    pthread_cond_init(&(send_handle->cond_send), NULL);
	send_handle->send_msg_num = 0;

	
	send_handle->packet_num = 0;
	for(i=0;i<RESEND_PACKET_MAX_NUM;++i)
		send_handle->resend[i] = NULL;
	 pthread_mutex_init(&(send_handle->mutex_resend), NULL);
	 pthread_cond_init(&(send_handle->cond_resend), NULL);
	 send_handle->resend_msg_num = 0;

	return(0);
}



int  netsend_push_msg(void * data )
{

	int ret = 1;
	int i = 0;
	if(NULL == send_handle)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}

	net_send_handle_t * send = send_handle;

	pthread_mutex_lock(&(send->mutex_send));

	for (i = 0; i < 1000; i++)
	{
	    ret = ring_queue_push(&send->send_msg_queue, data);
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
		pthread_mutex_unlock(&(send->mutex_send));
		return (-2);
    }
    else
    {
		volatile unsigned int *task_num = &send->send_msg_num;
    	fetch_and_add(task_num, 1);
    }

    pthread_mutex_unlock(&(send->mutex_send));
	pthread_cond_signal(&(send->cond_send));
	return(0);
}



static void *  netsend_pthread_fun(void * arg)
{
	net_send_handle_t * send = (net_send_handle_t * )arg;
	if(NULL == send)
	{
		dbg_printf("please check the param ! \n");
		return(NULL);
	}

	int ret = -1;
	int is_run = 1;
	int count_send = 0;
	send_packet_t * packet = NULL;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	while(is_run)
	{
        pthread_mutex_lock(&(send->mutex_send));
        while (0 == send->send_msg_num)
        {
            pthread_cond_wait(&(send->cond_send), &(send->mutex_send));
        }
		ret = ring_queue_pop(&(send->send_msg_queue), (void **)&packet);
		pthread_mutex_unlock(&(send->mutex_send));
		
		volatile unsigned int *handle_num = &(send->send_msg_num);
		fetch_and_sub(handle_num, 1);  

		if(ret != 0 || NULL == packet)continue;

		count_send = 3;

		
		if(!packet->is_resend && RELIABLE_PACKET == packet->type)
		{
			packet_header_t * head = (packet_header_t*)packet->data;
			head->index = send->packet_num;
			packet->index = send->packet_num;
			
			send->packet_num += 1;
			if(send->packet_num >= RESEND_PACKET_MAX_NUM)
				send->packet_num = 0;
		}

		
		while(count_send --)
		{
			ret = sendto(packet->sockfd,packet->data,packet->length,0,(struct sockaddr *)&packet->to,addr_len);
			if(-1 == ret)
			{
				if(errno == EINTR || errno == EAGAIN)
				{
					usleep(1000);continue;
				}
			}
			break;
		}

		
		if(UNRELIABLE_PACKET == packet->type)
		{
			if(NULL != packet->data)
			{
				free(packet->data);
				packet->data = NULL;
			}
			free(packet);
			packet = NULL;
		}
		else
		{

			struct timeval time_pass;
			get_time(&time_pass);
			if(packet->is_resend)
			{
				dbg_printf("resend ! \n");
				pthread_mutex_lock(&(send->mutex_resend));
				if(packet->resend_times > RESEND_TIMES)
				{

					send->resend[packet->index] = NULL;
					if(NULL != packet->data)
					{
						free(packet->data);
						packet->data = NULL;
					}

					free(packet);
					packet = NULL;	

					
					volatile unsigned int *handle_num = &(send->resend_msg_num);
					fetch_and_sub(handle_num, 1);  
					
					dbg_printf("i free it ! \n");
				}
				else
				{
					add_time(&time_pass,&packet->ta,&packet->tp);	
				

				}
				pthread_mutex_unlock(&(send->mutex_resend));
				
			}
			else
			{
				dbg_printf("new realiable packet come ! \n");
				pthread_mutex_lock(&(send->mutex_resend));
				if(NULL != send->resend[packet->index])
				{
					send_packet_t * pre_packet = (send_packet_t*)send->resend[packet->index];
					if(NULL != pre_packet->data)
					{
						free(pre_packet->data);
						pre_packet->data = NULL;
					}
					free(pre_packet);
					pre_packet = NULL;
				}

				packet->is_resend = 1;
				packet->resend_times = 0;
				add_time(&time_pass,&packet->ta,&packet->tp);
				
				send->resend[packet->index] = packet;
				
				volatile unsigned int *task_num = &send->resend_msg_num;
				fetch_and_add(task_num, 1);
				pthread_mutex_unlock(&(send->mutex_resend));
				pthread_cond_signal(&(send->cond_resend));

			}
			

		}

	}

	return(NULL);

}




static void *  netresend_pthread_fun(void * arg)
{
	net_send_handle_t * send = (net_send_handle_t * )arg;
	if(NULL == send)
	{
		dbg_printf("please check the param ! \n");
		return(NULL);
	}

	int ret = -1;
	int is_run = 1;
	int i = 0;
	send_packet_t * packet = NULL;
	struct timeval time_pass;
	while(is_run)
	{
        pthread_mutex_lock(&(send->mutex_resend));
        while (0 == send->resend_msg_num)
        {
            pthread_cond_wait(&(send->cond_resend), &(send->mutex_resend));
        }
		
		ret = get_time(&time_pass);
		if(ret != 0)
		{
			dbg_printf("error !! \n");
			is_run = 0;
			break;
		}
		for(i=0;i<RESEND_PACKET_MAX_NUM;++i)
		{

			if(NULL == send->resend[i])continue;
			
			packet = (send_packet_t*)(send->resend[i]);
			if(timercmp(&time_pass,&packet->tp,>=))
			{
				packet->is_resend = 1;
				packet->resend_times += 1;
				netsend_push_msg(send->resend[i]);
				
			}
		}
		pthread_mutex_unlock(&(send->mutex_resend));

		usleep(100*1000);


	}

	return(NULL);

}



int  netsend_start_up(void)
{
	int ret = -1;
	ret = netsend_handle_init();
	if(0 != ret)
	{
		dbg_printf("netsend_handle_init  fail ! \n");
		return(-1);
	}

	pthread_t netsend_ptid;
	ret = pthread_create(&netsend_ptid,NULL,netsend_pthread_fun,send_handle);
	pthread_detach(netsend_ptid);

	pthread_t netresend_ptid;
	ret = pthread_create(&netresend_ptid,NULL,netresend_pthread_fun,send_handle);
	pthread_detach(netresend_ptid);
	

}

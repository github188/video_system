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

#include "data_packet.h"
#include "ring_queue.h"
#include "net_send.h"


#include "common.h"

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"net_send"



#define		RESEND_PACKET_MAX_NUM	(1024)
#define		RESEND_TIMES			(3)


typedef struct net_send_handle
{
	pthread_mutex_t mutex_send;
	pthread_cond_t cond_send;
	ring_queue_t send_msg_queue;
	volatile unsigned int send_msg_num;
	void * resend[RESEND_PACKET_MAX_NUM]; /*I帧重传也采取这种模式*/
	volatile long packet_num;

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

		
		if(!packet->is_resend)
		{
			packet->index = send->packet_num;
			send->packet_num += 1;
			if(send->packet_num >= RESEND_PACKET_MAX_NUM)
				send->packet_num = 0;
		}
		
		while(count_send -- )
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
			if(packet->is_resend)
			{
				/*检测其是否超过重发次数，如果有，直接释放，如果没有，改变其超时参数，从新推送回去*/
			}
			else
			{
				/*重新申请一个新包，推送到队列中去*/

			}

		}

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
	


}

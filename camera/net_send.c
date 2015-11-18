
#include "net_send.h"
#include "system_init.h"

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"net_send"


int send_remove_packet(net_send_handle_t *send_handle, int index)
{

	if(RESEND_PACKET_MAX_NUM < index || NULL == send_handle)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}

	net_send_handle_t * send = send_handle;
	send_packet_t * packet = NULL;
	
	pthread_mutex_lock(&(send->mutex_resend));
	if(NULL == send->resend[index])
	{
		pthread_mutex_unlock(&(send->mutex_resend));
		return(-2);
	}

	dbg_printf("netsend_remove_packet ! \n");
	packet = (send_packet_t*)send->resend[index];
	send->resend[index] = NULL;
	if(NULL != packet->data)
	{
		free(packet->data);
		packet->data = NULL;
	}

	if(NULL != packet)
	{
		free(packet);
		packet = NULL;	
	}
	volatile unsigned int *handle_num = &(send->resend_msg_num);
	fetch_and_sub(handle_num, 1);  
		
	pthread_mutex_unlock(&(send->mutex_resend));

	
	return(0);
}


int  send_push_msg(net_send_handle_t *send_handle,void * data )
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



static void *  send_pthread_fun(void * arg)
{
	camera_handle_t * camera_dev = (camera_handle_t*)arg;
	net_send_handle_t * send = (net_send_handle_t * )camera_dev->send;
	if(NULL == camera_dev || NULL == send)
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
				dbg_printf("send fail ! \n");
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




static void *  resend_pthread_fun(void * arg)
{

	camera_handle_t * camera_dev = (camera_handle_t*)arg;
	net_send_handle_t * send = (net_send_handle_t * )camera_dev->send;

	if(NULL == camera_dev  || NULL == send)
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
				send_push_msg(send,send->resend[i]);
				
			}
		}
		pthread_mutex_unlock(&(send->mutex_resend));

		usleep(100*1000);

	}

	return(NULL);

}




void * send_handle_new(void)
{
	int ret = -1;
	int i = 0;

	net_send_handle_t * send_handle = calloc(1,sizeof(*send_handle));
	if(NULL == send_handle)
	{
		dbg_printf("calloc fail ! \n");
		return(NULL);
	}
	ret = ring_queue_init(&send_handle->send_msg_queue, 1024);
	if(ret < 0 )
	{
		dbg_printf("ring_queue_init  fail \n");
		free(send_handle);
		send_handle = NULL;
		return(NULL);
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


	send_handle->send_fun = send_pthread_fun;
	send_handle->resend_fun = resend_pthread_fun;
	

	
	return(send_handle);
}



void send_handle_destroy(void * arg)
{
	if(NULL == arg)
	{
		dbg_printf("free the null socket ! \n");
		return;
	}
	net_send_handle_t * handle = (net_send_handle_t*)arg;

	if(NULL != handle)
	{
		free(handle);
		handle = NULL;
	}

}



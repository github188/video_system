#include "heart_beat.h"
#include "system_init.h"


#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"heartbeat:"


static void * heart_beat_pthread(void * arg)
{

	camera_handle_t * camera_dev = (camera_handle_t*)arg;
	socket_handle_t * handle = camera_dev->socket;
	net_send_handle_t * send_handle = camera_dev->send;
	heart_beat_handle_t * beatheart = (heart_beat_handle_t*)camera_dev->beatheart;	
	if(NULL == camera_dev || NULL== beatheart)
	{
		dbg_printf("please check  the param ! \n");
		return(NULL);
	}

	int ret = -1;
	int is_run = 1;

	while(is_run)
	{
        pthread_mutex_lock(&(beatheart->mutex_heart));
        while (0 == beatheart->is_run)
        {
            pthread_cond_wait(&(beatheart->cond_heart), &(beatheart->mutex_heart));
        }
		pthread_mutex_unlock(&(beatheart->mutex_heart));

		
		beartheart_packet_t * rpacket = calloc(1,sizeof(*rpacket));
		if(NULL == rpacket)
		{
			dbg_printf("calloc is fail ! \n");
			continue;
		}
		send_packet_t *spacket = calloc(1,sizeof(*spacket));
		if(NULL == spacket)
		{
			dbg_printf("calloc is fail ! \n");
			free(rpacket);
			rpacket = NULL;
			continue;
		}
		rpacket->head.type = BEATHEART_PACKET;
		rpacket->head.index = 0xFFFF;
		rpacket->head.packet_len = sizeof(beartheart_packet_t);
		rpacket->head.ret = 0;
		rpacket->id_num = camera_dev->id_num;


		spacket->sockfd = handle->local_socket;
		spacket->data = rpacket;
		spacket->length = sizeof(beartheart_packet_t);
		spacket->to = camera_dev->peeraddr;
		spacket->type = UNRELIABLE_PACKET;
		ret = send_push_msg(send_handle,spacket);
		if(ret != 0)
		{
			free(rpacket);
			rpacket = NULL;
			free(spacket);
			spacket = NULL;
		}
		sleep(2);

	}



	return(NULL);
}


void *  heartbeat_handle_new(void)
{

	int ret = 0;

	heart_beat_handle_t *handle = (heart_beat_handle_t*)calloc(1,sizeof(*handle));
	if(NULL == handle)
	{
		dbg_printf("calloc is fail ! \n");
		return(NULL);
	}

	 pthread_mutex_init(&(handle->mutex_heart), NULL);
	 pthread_cond_init(&(handle->cond_heart), NULL);
	 handle->is_run = 0;
	 handle->heartbeat_fun = heart_beat_pthread;

	 return(handle);
	 

}


void heartbeat_handle_destroy(void * arg)
{
	if(NULL == arg)
	{
		dbg_printf("free the null socket ! \n");
		return;
	}
	heart_beat_handle_t * handle = (heart_beat_handle_t*)arg;

	if(NULL != handle)
	{
		free(handle);
		handle = NULL;
	}

}



int heartbeat_start(void * dev)
{

	if(NULL == dev)
	{
		dbg_printf("please check the param ! \n");
		return(-1);
	}
	camera_handle_t * camera_dev = (camera_handle_t*)dev;
	heart_beat_handle_t * handle = (heart_beat_handle_t*)camera_dev->beatheart;	
	if(NULL==camera_dev  || NULL == handle)
	{
		dbg_printf("check the handle ! \n");
		return(-1);
	}
	pthread_mutex_lock(&(handle->mutex_heart));
	volatile unsigned int *task_num = &handle->is_run;
	compare_and_swap(task_num, 0,1);
	pthread_mutex_unlock(&(handle->mutex_heart));
	pthread_cond_signal(&(handle->cond_heart));

	return(0);
}



int  heartbeat_stop(void * dev)
{

	camera_handle_t * camera_dev = (camera_handle_t*)dev;
	heart_beat_handle_t * handle = (heart_beat_handle_t*)camera_dev->beatheart;	
	if(NULL==camera_dev || NULL == handle)
	{
		dbg_printf("check the handle ! \n");
		return(-1);
	}
	pthread_mutex_lock(&(handle->mutex_heart));
	volatile unsigned int *task_num = &handle->is_run;
	compare_and_swap(task_num, 1,0);
	pthread_mutex_unlock(&(handle->mutex_heart));

	return(0);
}



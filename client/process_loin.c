
#include "process_loin.h"
#include "system_init.h"


#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"process_loin:"



static void * loin_pcket_pthread(void * arg)
{

	camera_handle_t * camera_dev = (camera_handle_t*)arg;
	socket_handle_t * handle = camera_dev->socket;
	net_send_handle_t * send_handle = camera_dev->send;
	process_loin_handle_t * loin = (process_loin_handle_t*)camera_dev->loin;	
	if(NULL == handle || NULL==loin)
	{
		dbg_printf("please check  the param ! \n");
		return(NULL);
	}

	int ret = -1;
	int is_run = 1;
	while(is_run)
	{
        pthread_mutex_lock(&(loin->mutex_loin));
        while (0 == loin->is_run)
        {
            pthread_cond_wait(&(loin->cond_loin), &(loin->mutex_loin));
        }
		pthread_mutex_unlock(&(loin->mutex_loin));
		loin_packet_t * rpacket_dev = calloc(1,sizeof(*rpacket_dev));
		if(NULL == rpacket_dev)
		{
			dbg_printf("calloc is fail ! \n");
			continue;
		}
		send_packet_t *spacket_dev = calloc(1,sizeof(*spacket_dev));
		if(NULL == spacket_dev)
		{
			dbg_printf("calloc is fail ! \n");
			free(rpacket_dev);
			rpacket_dev = NULL;
		}
		rpacket_dev->head.type = LOIN_PACKET;
		rpacket_dev->head.index = 0xFFFF;
		rpacket_dev->head.packet_len = sizeof(loin_packet_t);
		rpacket_dev->head.ret = 0;
		rpacket_dev->l = 'c';

		rpacket_dev->dev_addr = loin->addr;

		spacket_dev->sockfd = handle->local_socket;
		spacket_dev->data = rpacket_dev;
		spacket_dev->length = sizeof(loin_packet_t);
		spacket_dev->to = loin->addr;

		spacket_dev->type = UNRELIABLE_PACKET;

		ret = send_push_msg(send_handle,spacket_dev);

		dbg_printf("send loin data request ! \n");
		if(ret != 0)
		{
			free(rpacket_dev);
			rpacket_dev = NULL;
			free(spacket_dev);
			spacket_dev = NULL;
		}

		usleep(1000*1000);
		
	}



	return(NULL);
}


void *  loin_handle_new(void)
{

	int ret = 0;

	process_loin_handle_t *loin_handle = (process_loin_handle_t*)calloc(1,sizeof(*loin_handle));
	if(NULL == loin_handle)
	{
		dbg_printf("calloc is fail ! \n");
		return(NULL);
	}

	 pthread_mutex_init(&(loin_handle->mutex_loin), NULL);
	 pthread_cond_init(&(loin_handle->cond_loin), NULL);
	 loin_handle->is_run = 0;
	 loin_handle->loin_fun = loin_pcket_pthread;

	 return(loin_handle);
	 

}


void loin_handle_destroy(void * arg)
{
	if(NULL == arg)
	{
		dbg_printf("free the null socket ! \n");
		return;
	}
	process_loin_handle_t * handle = (process_loin_handle_t*)arg;

	if(NULL != handle)
	{
		free(handle);
		handle = NULL;
	}

}



int loin_start(void * dev,void * addr)
{

	if(NULL == dev)
	{
		dbg_printf("please check the param ! \n");
		return(-1);
	}
	camera_handle_t * camera_dev = (camera_handle_t*)dev;
	process_loin_handle_t * handle = (process_loin_handle_t*)camera_dev->loin;	
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



int loin_stop(void * dev)
{

	camera_handle_t * camera_dev = (camera_handle_t*)dev;
	process_loin_handle_t * handle = (process_loin_handle_t*)camera_dev->loin;	
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










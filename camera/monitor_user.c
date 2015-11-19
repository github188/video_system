#include "system_init.h"
#include "monitor_user.h"



#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"monitor_user:"



static void * monitor_user_pthread(void * arg)
{

	camera_handle_t * camera_dev = (camera_handle_t*)arg;
	monitor_user_t * monitor = (monitor_user_t *)camera_dev->monitor;
	if(NULL == camera_dev || NULL==monitor)
	{
		dbg_printf("please check the param ! \n");
		return(NULL);
	}
	int i = 0;
	int is_run = 1;
	client_user_t * client = NULL;
	while(is_run)
	{
  		for(i=0;i<MAX_USER; ++i)
  		{
			client = camera_dev->user[i];
			if(NULL == client || 0==client->is_run)continue;
			if(0 == client->beatheart)
			{
				monitor->lost_count[i] += 1;
			}
			else if(1 == client->beatheart)
			{
				monitor->lost_count[i] = 0;
				client->beatheart = 0;
			}

			if(monitor->lost_count[i] > MAX_LOST_COUNT)
			{
				client->is_run = 0;
				bzero(client,sizeof(*client));
				dbg_printf(" user==%d is  offline ! \n",i);
			}
		}

		sleep(2);
	}

	return(NULL);
}


void *  monitor_handle_new(void)
{

	int ret = 0;
	monitor_user_t * handle = calloc(1,sizeof(*handle));
	if(NULL == handle)
	{
		dbg_printf("calloc is fail ! \n");
		return(NULL);
	}
	bzero(handle,sizeof(*handle));
	handle->monitor_fun = monitor_user_pthread;
	
	return(handle);
	
}


void monitor_handle_destroy(void * arg)
{
	if(NULL == arg)
	{
		dbg_printf("free the null socket ! \n");
		return;
	}
	monitor_user_t * handle = (monitor_user_t*)arg;

	if(NULL != handle)
	{
		free(handle);
		handle = NULL;
	}

}
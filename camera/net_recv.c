#include "net_recv.h"
#include "system_init.h"


#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"net_recv:"


static int  recv_push_msg(net_recv_handle_t * recv_handle, void * data )
{

	int ret = 1;
	int i = 0;
	if(NULL == recv_handle)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}
	net_recv_handle_t * recv = recv_handle;
	pthread_mutex_lock(&(recv->mutex_recv));

	for (i = 0; i < 1000; i++)
	{
	    ret = ring_queue_push(&recv->recv_msg_queue, data);
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
		pthread_mutex_unlock(&(recv->mutex_recv));
		return (-2);
    }
    else
    {
		volatile unsigned int *task_num = &recv->recv_msg_num;
    	fetch_and_add(task_num, 1);
    }
    pthread_mutex_unlock(&(recv->mutex_recv));
	pthread_cond_signal(&(recv->cond_recv));
	return(0);
}


static void * recv_data_fun(void * arg)
{

	camera_handle_t * camera_dev = (camera_handle_t*)arg;
	net_recv_handle_t  * handle = (net_recv_handle_t*)camera_dev->recv;
	if(NULL == camera_dev || camera_dev->socket->local_socket <= 0)
	{
		dbg_printf("the recv socket is not ok ! \n");
		return(NULL);
	}
	int recv_socket = camera_dev->socket->local_socket;

	
	int epfd = -1;
	struct epoll_event add_event;
	struct epoll_event events[32];
	epfd=epoll_create(256);

	add_event.data.fd = handle->event_fd;
	add_event.events = EPOLLIN;
	epoll_ctl (epfd, EPOLL_CTL_ADD, handle->event_fd, &add_event);

	add_event.data.fd = recv_socket;
	add_event.events = EPOLLIN|EPOLLHUP;
	epoll_ctl (epfd, EPOLL_CTL_ADD, recv_socket, &add_event);


	struct sockaddr from;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	int len = 0;
	char buff[1500];
	packet_header_t * header = (packet_header_t*)(buff);
	int is_run = 1;
	int nevents = 0;
	int i = 0;
	int ret = -1;
	
	while(is_run)
	{
		nevents= epoll_wait(epfd, events, 32, -1);
		for(i=0;i<nevents;++i)
		{
			if(!(events[i].events & EPOLLIN))
			{
				dbg_printf("unknow events ! \n");
				continue;
			}
			if(handle->event_fd == events[i].data.fd)
			{
				eventfd_clean(handle->event_fd);
			}
			else
			{
				len = recvfrom(events[i].data.fd,buff,sizeof(buff), 0 , (struct sockaddr *)&from ,&addr_len); 
				if(len <= 0 )
				{
					dbg_printf("the length is not right ! \n");
					continue;
				}
				if(header->type <= DUMP_PACKET || header->type >= UNKNOW_PACKET)
				{
					dbg_printf("the packet is not in the limit ! \n");
					continue;
				}

				void * data = calloc(1,sizeof(struct sockaddr)+sizeof(char)*len+1);
				if(NULL == data)
				{
					dbg_printf("calloc is fail ! \n");
					continue;
				}

				memmove(data,&from,sizeof(struct sockaddr));
				memmove(data+sizeof(struct sockaddr),buff,len);

				ret = recv_push_msg(handle,data);
				if(ret < 0 )
				{
					if(NULL != data)
					{
						free(data);
						data = NULL;
					}
					continue;
				}
			}
		}

	}
}



static void *  recv_process_fun(void * arg)
{

	camera_handle_t * camera_dev = (camera_handle_t*)arg;
	net_recv_handle_t * recv = (net_recv_handle_t * )camera_dev->recv;
	if(NULL == recv || NULL == camera_dev)
	{
		dbg_printf("please check the param ! \n");
		return(NULL);
	}
	
	handle_packet_fun_t * fun = camera_dev->fun;
	if(NULL == fun)
	{
		dbg_printf("check the fun ! \n");
		return(NULL);
	}


	int ret = -1;
	int is_run = 1;
	packet_header_t * header = NULL;
	void * pdata = NULL;
	
	while(is_run)
	{
        pthread_mutex_lock(&(recv->mutex_recv));
        while (0 == recv->recv_msg_num)
        {
            pthread_cond_wait(&(recv->cond_recv), &(recv->mutex_recv));
        }
		ret = ring_queue_pop(&(recv->recv_msg_queue), (void **)&pdata);
		pthread_mutex_unlock(&(recv->mutex_recv));
		
		volatile unsigned int *handle_num = &(recv->recv_msg_num);
		fetch_and_sub(handle_num, 1);  

		if(ret != 0 || NULL == pdata)continue;

		header = (packet_header_t *)(pdata+sizeof(struct sockaddr));
		
		for(fun = camera_dev->fun; UNKNOW_PACKET != fun->type ;fun += 1)
		{
			if(header->type != fun->type)continue;
			if(NULL != fun->handle_packet)
			{
				fun->handle_packet(camera_dev,pdata);
		
			}
			
		}
		free(pdata);
		pdata = NULL;


	}

	return(NULL);

}



void * recv_handle_new(void)
{


	int ret = -1;
	net_recv_handle_t * recv_handle = calloc(1,sizeof(*recv_handle));
	if(NULL == recv_handle)
	{
		dbg_printf("calloc is fail ! \n");
		return(NULL);
	}


	recv_handle->event_fd = eventfd_new();
	if(recv_handle->event_fd < 0)
	{
		dbg_printf("eventfd_new fail ! \n");
		goto fail;
	}

	ret = ring_queue_init(&recv_handle->recv_msg_queue, 2048);
	if(ret < 0 )
	{
		dbg_printf("ring_queue_init  fail \n");
		goto fail;
	}
    pthread_mutex_init(&(recv_handle->mutex_recv), NULL);
    pthread_cond_init(&(recv_handle->cond_recv), NULL);
	recv_handle->recv_msg_num = 0;

	recv_handle->recv_fun = recv_data_fun;
	recv_handle->process_fun = recv_process_fun; 
	return(recv_handle);

fail:



	if(recv_handle->event_fd > 0)
	{
		close(recv_handle->event_fd);
		recv_handle->event_fd  = -1;
	}
	
	if(NULL != recv_handle)
	{
		free(recv_handle);
		recv_handle = NULL;
	}

	return(NULL);
}



void recv_handle_destroy(void * arg)
{
	if(NULL == arg)
	{
		dbg_printf("free the null socket ! \n");
		return;
	}
	net_recv_handle_t * handle = (net_recv_handle_t*)arg;

	if(handle->event_fd > 0)
	{
		close(handle->event_fd);
		handle->event_fd  = -1;
	}
	
	if(NULL != handle)
	{
		free(handle);
		handle = NULL;
	}

}
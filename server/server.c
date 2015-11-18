
#include "server.h"



#undef  DBG_ON
#undef  FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"server:"







int  server_push_recvmsg(server_handle_t * handle,void * data )
{

	int ret = 1;
	int i = 0;
	if(NULL == handle)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}

	server_handle_t * server = handle;
	pthread_mutex_lock(&(server->mutex_server));

	for (i = 0; i < 1000; i++)
	{
	    ret = ring_queue_push(&server->server_msg_queue, data);
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
		pthread_mutex_unlock(&(server->mutex_server));
		return (-2);
    }
    else
    {
		volatile unsigned int *task_num = &server->server_msg_num;
    	fetch_and_add(task_num, 1);
    }
    pthread_mutex_unlock(&(server->mutex_server));
	pthread_cond_signal(&(server->cond_server));
	return(0);
}


int  server_push_sendmsg(server_handle_t * handle,void * data )
{

	int ret = 1;
	int i = 0;
	if(NULL == data || NULL == handle)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}

	server_handle_t * send = handle;

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


int remove_send_packets(server_handle_t * handle,int index)
{

	if(RESEND_PACKET_MAX_NUM < index || NULL == handle)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}

	server_handle_t * send = handle;
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



static void *  resend_pthread_fun(void * arg)
{
	server_handle_t * send = (server_handle_t * )arg;
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
				server_push_sendmsg(send,send->resend[i]);
				
			}
		}
		pthread_mutex_unlock(&(send->mutex_resend));

		usleep(100*1000);

	}

	return(NULL);

}


static void * send_pthread_fun(void * arg)
{
	server_handle_t * send = (server_handle_t * )arg;
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

					if(NULL != packet)
					{
						free(packet);
						packet = NULL;

					}

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

					if(NULL != pre_packet)
					{
						free(pre_packet);
						pre_packet = NULL;

					}
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



static void *  process_recv_fun(void * arg)
{
	server_handle_t * server = (server_handle_t * )arg;
	server->fun = get_handle_packet_fun();
	if(NULL == server || NULL==server->fun)
	{
		dbg_printf("please check the param ! \n");
		return(NULL);
	}
	handle_packet_fun_t * fun = server->fun;
	int ret = -1;
	int is_run = 1;
	packet_header_t * header = NULL;
	void * data;
	int i = DUMP_PACKET;
	
	while(is_run)
	{
        pthread_mutex_lock(&(server->mutex_server));
        while (0 == server->server_msg_num)
        {
            pthread_cond_wait(&(server->cond_server), &(server->mutex_server));
        }
		ret = ring_queue_pop(&(server->server_msg_queue), (void **)&data);
		pthread_mutex_unlock(&(server->mutex_server));
		
		volatile unsigned int *handle_num = &(server->server_msg_num);
		fetch_and_sub(handle_num, 1);  
		
		if(ret != 0 || NULL == data)continue;

		header = (packet_header_t *)(data+sizeof(struct sockaddr));
		for(fun = server->fun; UNKNOW_PACKET != fun->type ;fun += 1)
		{
			if(header->type != fun->type)continue;
			if(NULL != fun->handle_packet)
			{
				fun->handle_packet(server,data);
		
			}
			
		}

		if(NULL != data)
		{
			free(data);
			data = NULL;

		}



	}

	return(NULL);

}



int main(int argc,char * argv[])
{


	int ret = -1;
	int value = 0;
	int i = 0;
	server_handle_t * handle = calloc(1,sizeof(1,*handle));
	if(NULL == handle)
	{
		dbg_printf("calloc is fail ! \n");
		return(-1);
	}


	handle->server_socket = socket(AF_INET, SOCK_DGRAM, 0);

	struct sockaddr_in	servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERVER_PORT);
	int buff_size = 32 * 1024;
	setsockopt(handle->server_socket, SOL_SOCKET, SO_RCVBUF, &buff_size, sizeof(buff_size));
	setsockopt(handle->server_socket, SOL_SOCKET, SO_SNDBUF, (char*)&buff_size, sizeof(buff_size));

	value = fcntl(handle->server_socket,F_GETFL,0);
	ret = fcntl(handle->server_socket, F_SETFL, value|O_NONBLOCK);
	if(ret < 0)
	{
		dbg_printf("F_SETFL fail ! \n");
		return(-2);
	}
	fcntl(handle->server_socket, F_SETFD, FD_CLOEXEC);
	ret = bind(handle->server_socket, (struct sockaddr *) &servaddr, sizeof(servaddr));
	if(ret != 0 )
	{
		dbg_printf("bind is fail ! \n");
		return(-3);
	}

	ret = ring_queue_init(&handle->server_msg_queue, 4096);
	if(ret < 0 )
	{
		dbg_printf("ring_queue_init  fail \n");
		return(-3);
	}
    pthread_mutex_init(&(handle->mutex_server), NULL);
    pthread_cond_init(&(handle->cond_server), NULL);
	handle->server_msg_num = 0;


	ret = ring_queue_init(&handle->send_msg_queue, 1024);
	if(ret < 0 )
	{
		dbg_printf("ring_queue_init  fail \n");
		return(-4);
	}
    pthread_mutex_init(&(handle->mutex_send), NULL);
    pthread_cond_init(&(handle->cond_send), NULL);
	handle->send_msg_num = 0;



	handle->packet_num = 0;
	for(i=0;i<RESEND_PACKET_MAX_NUM;++i)
		handle->resend[i] = NULL;
	 pthread_mutex_init(&(handle->mutex_resend), NULL);
	 pthread_cond_init(&(handle->cond_resend), NULL);
	 handle->resend_msg_num = 0;

	for(i=0;i<DEVICE_MAX_NUM;++i)
	{
		handle->dev[i] = calloc(1,sizeof(dev_info_t));
		if(NULL == handle->dev[i])
		{
			dbg_printf("calloc is fail ! \n");
			goto fail;
		}
		handle->dev[i]->is_run = 0;
	}


	int epfd = -1;
	struct epoll_event add_event;
	struct epoll_event events[4096];
	epfd=epoll_create(4096);
	add_event.data.fd = handle->server_socket;
	add_event.events = EPOLLIN | EPOLLHUP;
	epoll_ctl (epfd, EPOLL_CTL_ADD, handle->server_socket, &add_event);

	pthread_t process_packet_pthed;
	ret = pthread_create(&process_packet_pthed,NULL,process_recv_fun,handle);

	pthread_t server_send_ptid;
	ret = pthread_create(&server_send_ptid,NULL,send_pthread_fun,handle);


	pthread_t server_resend_ptid;
	ret = pthread_create(&server_resend_ptid,NULL,resend_pthread_fun,handle);


	struct sockaddr from;
	socklen_t addr_len = sizeof(struct sockaddr);
	int len = 0;
	char buff[1500];
	packet_header_t * header = (packet_header_t*)(buff);
	int is_run = 1;
	int nevents=0;
	while(is_run)
	{
		nevents= epoll_wait(epfd, events, 4096, -1);

		for(i=0;i<nevents;++i)
		{

			if(!(events[i].events & EPOLLIN))
			{
				dbg_printf("unknow events ! \n");
				continue;
			}
			if(handle->server_socket == events[i].data.fd)
			{
			
				len = recvfrom(handle->server_socket,buff,sizeof(buff), 0 , (struct sockaddr *)&from ,&addr_len); 
				if(len <= 0 )
				{
					dbg_printf("the length is not right ! \n");
					continue;
				}

				if(header->type <= DUMP_PACKET || header->type >= UNKNOW_PACKET )
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
				memmove((char*)data+sizeof(struct sockaddr),buff,len);
				
				ret = server_push_recvmsg(handle,data);
				if(0 != ret)
				{
					dbg_printf("server_push_msg is fail ! \n");
					free(data);
					data = NULL;
				}
				

			}

		}
	}

	pthread_join(process_packet_pthed,NULL);
	pthread_join(server_send_ptid,NULL);
	pthread_join(server_resend_ptid,NULL);

	return(0);


fail:


	return(-1);

}




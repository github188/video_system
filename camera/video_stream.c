#include "video_stream.h"
#include "akuio.h"
#include "video_encode.h"
#include "system_init.h"





static int  stream_push_msg(video_stream_handle_t * handle, void * data )
{

	int ret = 1;
	int i = 0;
	if(NULL == handle || NULL== data)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}


	pthread_mutex_lock(&(handle->mutex_encode));

	for (i = 0; i < 1000; i++)
	{
	    ret = ring_queue_push(&handle->raw_msg_queue, data);
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
		pthread_mutex_unlock(&(handle->mutex_encode));
		return (-2);
    }
    else
    {
		volatile unsigned int *task_num = &handle->raw_msg_num;
    	fetch_and_add(task_num, 1);
    }
    pthread_mutex_unlock(&(handle->mutex_encode));
	pthread_cond_signal(&(handle->cond_encode));
	return(0);
}


static void * video_capture_pthread(void * arg)
{

	int ret = -1;
	camera_handle_t * camera_dev = (camera_handle_t*)arg;	
	video_stream_handle_t * video = (video_stream_handle_t*)camera_dev->video;
	if(NULL ==camera_dev ||  NULL==video)
	{
		dbg_printf("please check the param ! \n");
		return(NULL);
	}
	video->video_dev = camera_new_dev(CAMERA_DEVICE);
	if(NULL == video->video_dev)
	{
		dbg_printf("camera_new_dev is fail ! \n");
		return(NULL);
	}
	camera_start(video->video_dev);

	int is_run = 1;
	struct v4l2_buffer *frame_buf = NULL;
	while(is_run)
	{
		pthread_mutex_lock(&(video->mutex_capture));
		while (0 == video->need_capture)
		{
			pthread_cond_wait(&(video->cond_capture), &(video->mutex_capture));
		}
		pthread_mutex_unlock(&(video->mutex_capture));
		
		frame_buf = camera_read_frame(video->video_dev);
		if(NULL == frame_buf)continue;
		stream_push_msg(video,frame_buf);
	}


}



/*放置到注册程序中*/
int video_capture_start(void * dev)
{

	if(NULL == dev)
	{
		dbg_printf("please check the param ! \n");
		return(-1);
	}
	camera_handle_t * camera_dev = (camera_handle_t*)dev;
	video_stream_handle_t * video = (video_stream_handle_t*)camera_dev->video;	
	if(NULL==camera_dev  || NULL == video)
	{
		dbg_printf("check the handle ! \n");
		return(-1);
	}
	pthread_mutex_lock(&(video->mutex_capture));
	volatile unsigned int *task_num = &video->need_capture;
	compare_and_swap(task_num, 0,1);
	pthread_mutex_unlock(&(video->mutex_capture));
	pthread_cond_signal(&(video->cond_capture));

	return(0);
}


/*放置到监控程序中*/
int  video_capture_stop(void * dev)
{

	if(NULL == dev)
	{
		dbg_printf("please check the param ! \n");
		return(-1);
	}
	camera_handle_t * camera_dev = (camera_handle_t*)dev;
	video_stream_handle_t * video = (video_stream_handle_t*)camera_dev->video;	
	if(NULL==camera_dev  || NULL == video)
	{
		dbg_printf("check the handle ! \n");
		return(-1);
	}
	pthread_mutex_lock(&(video->mutex_capture));
	volatile unsigned int *task_num = &video->need_capture;
	compare_and_swap(task_num, 1,0);
	pthread_mutex_unlock(&(video->mutex_capture));

	return(0);
}


static int video_encode_buff(void * dev,client_user_t * user,void * buff)
{

	
	camera_handle_t * camera_dev = (camera_handle_t*)dev;
	socket_handle_t * handle = camera_dev->socket;
	net_send_handle_t * send_handle = camera_dev->send;
	if(NULL==user || NULL==buff || NULL==user->encode_handle)
	{
		dbg_printf("please check the param ! \n");
		return(-1);
	}

	int ret = -1;
	int i = 0;
	int count1,count2;
	int size = 0;
	void *out_buff = NULL;
	frame_type_m i_frame = UNKNOW_FRAME;
	struct v4l2_buffer *frame_buf = (struct v4l2_buffer *)buff;
	if(user->force_iframe)
	{
		i_frame = I_FRAME;		
	}
	else
	{
		i_frame = (0 == user->frame_count % user->iframe_gap) ? (I_FRAME) : (P_FRAME);
		user->frame_count = (user->frame_count + 1)%user->iframe_gap;
	}
	size = video_encode_frame(user->encode_handle,(void*)frame_buf->m.userptr,&out_buff,i_frame);
	if(size <= 0)
	{
		dbg_printf("video_encode_frame is fail ! \n");
		return(-1);
	}

	if(I_FRAME == i_frame)
	{
		




	}
	else
	{
		pframe_packet_t * rpacket = calloc(1,sizeof(*rpacket)+size+1);
		if(NULL == rpacket)
		{
			dbg_printf("calloc is fail ! \n");
			return(-1);
		}

		send_packet_t *spacket = calloc(1,sizeof(*spacket));
		if(NULL == spacket)
		{
			dbg_printf("calloc is fail ! \n");
			free(rpacket);
			rpacket = NULL;
			return(-1);
		}

		rpacket->head.type = PFRAME_PACKET;
		rpacket->head.index = 0xFFFF;
		rpacket->head.packet_len = sizeof(pframe_packet_t);
		rpacket->id_packet = user->frame_count;
		rpacket->length = size;

		spacket->sockfd = handle->local_socket;
		spacket->data = rpacket;
		spacket->length = sizeof(*rpacket)+size+1;

		spacket->to = user->addr;
		spacket->type = UNRELIABLE_PACKET;
		ret = send_push_msg(send_handle,spacket);
		if(ret != 0)
		{
			dbg_printf("send_push_msg is fail ! \n");
			free(rpacket);
			rpacket = NULL;
			free(spacket);
			spacket = NULL;
		}

	}



	return(0);
}


static void *  video_encode_pthread(void * arg)
{

	
	camera_handle_t * camera_dev = (camera_handle_t*)arg;
	video_stream_handle_t * video = (video_stream_handle_t*)camera_dev->video;
	if(NULL ==camera_dev ||  NULL==video)
	{
		dbg_printf("please check the param ! \n");
		return(NULL);
	}


	int i = 0;
	int ret = -1;
	int is_run = 1;
	struct v4l2_buffer *frame_buf = NULL;
	client_user_t * client = NULL;
	
	while(is_run)
	{
        pthread_mutex_lock(&(video->mutex_encode));
        while (0 == video->raw_msg_num)
        {
            pthread_cond_wait(&(video->cond_encode), &(video->mutex_encode));
        }
		ret = ring_queue_pop(&(video->raw_msg_queue), (void **)&frame_buf);
		pthread_mutex_unlock(&(video->mutex_encode));
		
		volatile unsigned int *handle_num = &(video->raw_msg_num);
		fetch_and_sub(handle_num, 1);  

		if(ret != 0 || NULL == frame_buf)continue;

		for(i=0;i<MAX_USER;++i)
		{
			client = camera_dev->user[i];
			if(0 == client->is_run)continue;
			video_encode_buff(camera_dev,client,frame_buf);
		}
		
		camera_free_frame(video->video_dev,frame_buf);

	
	}

	return(NULL);

}



void * video_stream_new_handle(void)
{

	int ret = -1;
	video_stream_handle_t * handle = (video_stream_handle_t*)calloc(1,sizeof(*handle));
	if(NULL == handle)
	{
		dbg_printf("calloc is fail ! \n");
		return(NULL);
	}
	
	pthread_mutex_init(&(handle->mutex_capture), NULL);
	pthread_cond_init(&(handle->cond_capture), NULL);
	handle->need_capture = 0;
	
	

	ret = ring_queue_init(&handle->raw_msg_queue, 1024);
	if(ret < 0 )
	{
		dbg_printf("ring_queue_init  fail \n");
		free(handle);
		handle = NULL;
		return(NULL);
	}
    pthread_mutex_init(&(handle->mutex_encode), NULL);
    pthread_cond_init(&(handle->cond_encode), NULL);
	handle->raw_msg_num = 0;

	handle->capature_fun = video_capture_pthread;
	handle->video_encode_fun = video_encode_pthread;



	return(handle);
}




void video_stream_handle_destroy(void * arg)
{
	if(NULL == arg)
	{
		dbg_printf("free the null socket ! \n");
		return;
	}
	video_stream_handle_t * handle = (video_stream_handle_t*)arg;
	if(NULL != handle)
	{
		free(handle);
		handle = NULL;
	}
}




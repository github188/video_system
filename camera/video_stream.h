#ifndef _video_stream_h
#define _video_stream_h

#include "common.h"
#include "data_packet.h"
#include "ring_queue.h"
#include "dev_camera.h"

typedef struct video_stream_handle
{
	pthread_mutex_t mutex_capture;
	pthread_cond_t cond_capture;
	volatile unsigned int need_capture;

	
	pthread_mutex_t mutex_encode;
	pthread_cond_t cond_encode;
	ring_queue_t raw_msg_queue;
	volatile unsigned int raw_msg_num;

	camera_dev_t * video_dev;

	pthread_t capature_ptid;
	pthread_t video_encode_ptid;
	pthread_fun capature_fun;
	pthread_fun video_encode_fun;


}video_stream_handle_t;


void * video_stream_new_handle(void);
void video_stream_handle_destroy(void * arg);
int video_capture_start(void * dev);
int  video_capture_stop(void * dev);


#endif/*_video_stream_h*/
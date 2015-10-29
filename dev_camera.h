#ifndef _dev_camera_h
#define	_dev_camera_h

#include <linux/videodev2.h>

#define VIDEO_WIDTH_720P    (1280u)
#define VIDEO_HEIGHT_720P	(720u)
#define VIDEO_WIDTH_VGA     (640u)
#define VIDEO_HEIGHT_VGA	(360u)

#define  CAMERA_DEVICE		"/dev/video0"
#define	 BUFFER_NUM			(4)


typedef struct  
{
    void * start;
    size_t length;
}buffer_t;


typedef struct camera_dev
{
	int camera_fd;
	int video_height;
	int video_width;
	void * pmem_addres;
	buffer_t data[BUFFER_NUM]; 
	
}camera_dev_t;



typedef struct  camera_handle
{
	camera_dev_t * dev;
	void* (*new_dev)(const char * dev_path);
	void* (*read_frame)(camera_dev_t * device);
	int (*free_frame)(camera_dev_t * device,void * frame_buff);
	int (*stop)(camera_dev_t * device);
	int (*start)(camera_dev_t * device);
}camera_handle_t;



void * register_camera(void);

#endif/*_dev_camera_h*/
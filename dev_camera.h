#ifndef _dev_camera_h
#define	_dev_camera_h

#include <linux/videodev2.h>

#define VIDEO_WIDTH_720P    (1280u)
#define VIDEO_HEIGHT_720P	(720u)
#define VIDEO_WIDTH_VGA     (640u)
#define VIDEO_HEIGHT_VGA	(480u)

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




int camera_stop (camera_dev_t * handle);
int camera_start (camera_dev_t * handle);
void * camera_read_frame(camera_dev_t * handle);
int camera_free_frame(camera_dev_t * handle,void * frame_buff);
void * camera_new_dev(const char * dev_path);

#endif/*_dev_camera_h*/
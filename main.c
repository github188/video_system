#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "akuio.h"
#include "dev_camera.h"
#include "common.h"

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"main"



static int lib_init(void)
{
	int ret = -1;
	ret = akuio_pmem_init();
	if(ret != 0 )
	{
		dbg_printf("akuio_pmem_init fail ! \n");
		return(-1);
	}

	return(0);
}


static camera_handle_t * camera = NULL;


int main(void)
{
	dbg_printf("this is a test ! \n");
	int ret  =-1;
	ret= lib_init();
	if(ret != 0 )
	{
		dbg_printf("lib_init  fail ! \n");
		return(-1);
	}


	camera = register_camera();
	if(NULL == camera)
	{
		dbg_printf("please check the param ! \n");
	}

	if(NULL != camera->dev)
	{
		dbg_printf("the camera has been init ! \n");
		return(-3);
	}

	camera->dev = camera->new_dev(CAMERA_DEVICE);
	if(NULL == camera->dev)
	{
		dbg_printf("the dev is not ok ! \n");
		return(-4);

	}
	camera->start(camera->dev);

	int test_fd = -1;
	test_fd = open("/mnt/mmc/yuv_file.yuv",O_RDWR|O_APPEND);
	if(test_fd < 0)
	{
		dbg_printf("open the file fail ! \n");
		return(-5);
	}

	int count = 0;

	while(1)
	{
		struct v4l2_buffer *frame_buf = camera->read_frame(camera->dev);
		if(NULL == frame_buf)continue;
		write(test_fd,(void*)frame_buf->m.userptr,frame_buf->length);
		camera->free_frame(camera->dev,frame_buf);
		count += 1;
		if(count > 60)break;
		
		
	}

	close(test_fd);
	

	
	
	


	return(0);
}




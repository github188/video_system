/*******************************************************************************
**	jweihsz@qq.com		2015			v1.0
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "akuio.h"
#include "dev_camera.h"
#include "video_encode.h"
#include "start_up.h"
#include "system_init.h"
#include "net_send.h"
#include "net_recv.h"
#include "handle_packet.h"
#include "process_loin.h"


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

	ret = video_encode_init();
	if(0 == ret)
	{
		dbg_printf("video encode lib init fail ! \n");
		return(-2);
	}

	return(0);
}



int main(void)
{
	dbg_printf("this is a test ! \n");
	int ret  =-1;
	
//	start_up();
	
	ret = system_init();
	if(ret != 0)
	{
		dbg_printf("system_init is fail ! \n");
		return(-1);
	}
	
	handle_packet_init();
	netsend_start_up();
	netrecv_start_up();
	loin_process_init();
	
	sleep(1);
	send_register_packet();
	

	while(1)
	{
		
		sleep(10);
	}
		
	
	

	


	return  0;

	
	ret= lib_init();
	if(ret != 0 )
	{
		dbg_printf("lib_init  fail ! \n");
		return(-1);
	}
	camera_dev_t * camera = camera_new_dev(CAMERA_DEVICE);
	if(NULL == camera)
	{
		dbg_printf("the dev is not ok ! \n");
		return(-4);

	}

	video_encode_handle_t * new_encode_handle = (video_encode_handle_t*)video_new_handle(500*1024);
	if(NULL == new_encode_handle)
	{
		dbg_printf("video_new_handle is fail ! \n");
		return(-5);

	}
	
	camera_start(camera);

	int test_fd = -1;
	test_fd = open("/mnt/mmc/yuv_file.264",O_RDWR|O_APPEND);
	if(test_fd < 0)
	{
		dbg_printf("open the file fail ! \n");
		return(-5);
	}

	int count = 0;
	void *out_buff;
	int size = 0;
	frame_type_m i_frame = UNKNOW_FRAME;

	while(1)
	{
		struct v4l2_buffer *frame_buf = camera_read_frame(camera);
		if(NULL == frame_buf)continue;

		if((count % 30) == 0 )i_frame = I_FRAME;
		else
			i_frame = P_FRAME;
		
		size = video_encode_frame(new_encode_handle,(void*)frame_buf->m.userptr,&out_buff,i_frame);
		if(size > 0)
		{
			write(test_fd,(void*)out_buff,size);
		}
		
		camera_free_frame(camera,frame_buf);
		count += 1;
		if(count > 500)break;
		
		
	}

	close(test_fd);
	video_encode_close(new_encode_handle);
	

	
	
	


	return(0);
}




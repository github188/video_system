

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include "akuio.h"
#include "isp_interface.h"
#include "dev_camera.h"
#include "common.h"



#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"dev_camera:"




#define RGB2Y(R,G,B)   ((306*R + 601*G + 117*B)>>10)
#define RGB2U(R,G,B)   ((-173*R - 339*G + 512*B + 131072)>>10)
#define RGB2V(R,G,B)   ((512*R - 428*G - 84*B + 131072)>>10)


static int xioctl (int camera_fd,int request,void * arg)
{
    int r = 0;
	if(camera_fd <=0 )
	{
		dbg_printf("please int the camera dev ! \n");
		return(-1);
	}
    do 
	{
		r = ioctl(camera_fd, request, arg);
	}while (-1 == r && EINTR == errno);
    return r;
}



static int camera_set_channel(int camera_fd,int width, int height, int enable)
{
	struct v4l2_streamparm parm;
	memset(&parm,0,sizeof(parm));
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	struct isp_channel2_info *p_mode = (struct isp_channel2_info *)parm.parm.raw_data;
	p_mode->type = ISP_PARM_CHANNEL2;
	p_mode->width = width;
	p_mode->height = height;
	p_mode->enable = enable;

	if ( 0 != xioctl(camera_fd, VIDIOC_S_PARM, &parm))
	{
		anyka_print("Set channel2 err \n");
	}
	else
	{
		anyka_print("Set channel2 ok \n");
	}

	return 0;
}


int camera_stop (camera_dev_t * handle)
{
	int ret = -1;
	if(NULL == handle || handle->camera_fd <=0 )
	{
		dbg_printf("please int the camera dev ! \n");
		return(-1);
	}
	
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = xioctl(handle->camera_fd, VIDIOC_STREAMOFF, &type);
	if(0 != ret)
	{
		dbg_printf("stop camera stream fail ! \n");
		return(-1);
	}
	return(0);
    
}



int camera_start (camera_dev_t * handle)
{
	int ret = -1;
	if(NULL == handle || handle->camera_fd <=0 )
	{
		dbg_printf("please int the camera dev ! \n");
		return(-1);
	}
	
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = xioctl(handle->camera_fd, VIDIOC_STREAMON, &type);
	if(0 != ret)
	{
		dbg_printf("start camera stream fail ! \n");
		return(-1);
	}
	return(0);
    
}


static int camera_mmap(camera_dev_t * handle,unsigned int buff_szie)
{

	int ret = -1;
	if(NULL == handle || handle->camera_fd <= 0 || 0 == buff_szie)
	{
		dbg_printf("please check the param ! \n");
		return(-1);
	}
	struct v4l2_requestbuffers req;
	memset(&req,0,sizeof(req));
    req.count  = BUFFER_NUM;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;
	ret = xioctl(handle->camera_fd, VIDIOC_REQBUFS, &req);
	if(0 != ret )
	{
		dbg_printf("fail !!!\n");
		return(-2);
	}

	if(NULL != handle->pmem_addres)
	{
		dbg_printf("pmem_addres has been init ! \n");
		return(-1);

	}

	handle->pmem_addres = akuio_alloc_pmem(BUFFER_NUM*buff_szie);
	if(NULL == handle->pmem_addres)
	{
		dbg_printf("akuio_alloc_pmem fail ! \n");
		return(-3);
	}

	unsigned int addres = akuio_vaddr2paddr(handle->pmem_addres) & 7;
	unsigned char * pmem_phyaddres = ((unsigned char *)handle->pmem_addres) + ((8-addres)&7);
	int i = 0;
    for (i = 0; i < BUFFER_NUM; ++i)
	{
		
		handle->data[i].length = buff_szie;
		handle->data[i].start = pmem_phyaddres + (handle->data[i].length) *i;
		if(NULL == handle->data[i].start)
		{
			dbg_printf("the addres is wrong ! \n");
			return(-4);
		}

		struct v4l2_buffer buf;
		memset(&buf,0,sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;
        buf.index = i;
        buf.m.userptr = (unsigned long)handle->data[i].start;
        buf.length = handle->data[i].length;
		ret =  xioctl(handle->camera_fd, VIDIOC_QBUF, &buf);
		if(ret != 0 )
		{
			dbg_printf("VIDIOC_QBUF  fail ! \n");
			akuio_free_pmem(handle->pmem_addres);
			return(-5);
		}
		
    }


	return(0);
}



int camera_unmmap(camera_dev_t * handle)
{
	if(NULL == handle || NULL == handle->pmem_addres)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}
	akuio_free_pmem(handle->pmem_addres);
	handle->pmem_addres = NULL;
	return(0);
}



void * camera_read_frame(camera_dev_t * handle)
{

	int ret = -1;
	int i = -1;
	if(NULL == handle || handle->camera_fd <=0 )
	{
		dbg_printf("please init the camera dev ! \n");
		return(NULL);
	}
	while(1)
	{
	    fd_set fds;
	    struct timeval tv;
	    FD_ZERO(&fds);
	    FD_SET(handle->camera_fd, &fds);
	    tv.tv_sec = 2;
	    tv.tv_usec = 0;
	    ret = select(handle->camera_fd + 1, &fds, NULL, NULL, &tv);	
		if(ret <= 0)
		{
			dbg_printf("the camera is not ready ! \n");
			return(NULL);
		}
		struct v4l2_buffer *frame_buf = calloc(1,sizeof(*frame_buf));
		if(NULL == frame_buf)
		{
			dbg_printf("calloc fail ! \n");
			return(NULL);
		}
		frame_buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		frame_buf->memory = V4L2_MEMORY_USERPTR;
		frame_buf->m.userptr = 0;

		ret = xioctl(handle->camera_fd, VIDIOC_DQBUF, frame_buf);
		if(0 != ret )
		{
			free(frame_buf);
			frame_buf = NULL;
			dbg_printf("the camera data is not ready ! \n");
			return(NULL);
		}
		for (i = 0; i < BUFFER_NUM; ++i) 
		{
		   if (frame_buf->m.userptr == (unsigned long)handle->data[i].start)
		   {
				return(frame_buf);
		   }
		}

		if(NULL != frame_buf)
		{
			free(frame_buf);
			frame_buf = NULL;
		}
		break;

	}

	return(NULL);


}




int camera_free_frame(camera_dev_t * handle,void * frame_buff)
{
	int ret = -1;

	if(NULL==handle || NULL == frame_buff || handle->camera_fd <= 0)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}
	ret = xioctl(handle->camera_fd, VIDIOC_QBUF, frame_buff);
	free(frame_buff);
	frame_buff = NULL;
	if(ret != 0 )
	{
		dbg_printf("fail ! \n");
		return(-2);
	}
	return(0);
}

void * camera_new_dev(const char * dev_path)
{

	int ret = -1;
	int min = -1;
	if(NULL == dev_path)
	{
		dbg_printf("please check the param ! \n");
		return(NULL);
	}

	struct stat st;
	ret = stat(dev_path,&st);
	if(0 != ret || !S_ISCHR (st.st_mode))
	{
		dbg_printf("please check the video device !\n");
		return(NULL);
	}

	camera_dev_t * new_camera = calloc(1,sizeof(*new_camera));
	if(NULL == new_camera)
	{
		dbg_printf("calloc fail ! \n");
		return(NULL);
	}

	new_camera->camera_fd = open (dev_path, O_RDWR| O_NONBLOCK);
	if(new_camera->camera_fd < 0 )
	{
		dbg_printf("open the camera dev fail ! \n");
		goto fail;
	}
	fcntl(new_camera->camera_fd, F_SETFD, FD_CLOEXEC);
	new_camera->video_height = /*VIDEO_HEIGHT_720P*/VIDEO_HEIGHT_VGA;
	new_camera->video_width = /*VIDEO_WIDTH_720P*/VIDEO_WIDTH_VGA;
	new_camera->pmem_addres = NULL;
	
	struct v4l2_capability cap;
	memset(&cap,0,sizeof(cap));
	ret = xioctl(new_camera->camera_fd, VIDIOC_QUERYCAP,&cap);
	if(0 != ret)
	{
		dbg_printf("get the v4l2_capability fail ! \n");
		goto fail;
	}

	dbg_printf("max_frame===%d \n",cap.reserved[1]);
	if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) || !(cap.capabilities & V4L2_CAP_STREAMING))
	{
		dbg_printf("check the device driver ! \n");
		goto fail;
	}

	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	memset(&cropcap,0,sizeof(cropcap));
	memset(&crop,0,sizeof(crop));
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = xioctl (new_camera->camera_fd, VIDIOC_CROPCAP, &cropcap);
	if(0 == ret )
	{
		dbg_printf("left==%d top==%d  width==%d  height==%d \n",cropcap.defrect.left,cropcap.defrect.top,cropcap.defrect.width,cropcap.defrect.height);
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect;
		xioctl(new_camera->camera_fd, VIDIOC_S_CROP, &crop);
	}

	struct v4l2_format fmt;
	memset(&fmt,0,sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = new_camera->video_width;  
    fmt.fmt.pix.height = new_camera->video_height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	ret  = xioctl(new_camera->camera_fd, VIDIOC_S_FMT, &fmt);
	if(0 != ret)
	{
		dbg_printf("VIDIOC_S_FMT fail ! \n");
		goto fail;
	}


#if 0
	camera_set_channel(new_camera->camera_fd,VIDEO_WIDTH_VGA,VIDEO_HEIGHT_VGA,1);
#endif

	ret  = xioctl(new_camera->camera_fd, VIDIOC_G_FMT, &fmt);
	if(0 != ret)
	{
		dbg_printf("VIDIOC_G_FMT fail ! \n");
		goto fail;
	}

	struct v4l2_streamparm parm;
	memset(&parm,0,sizeof(parm));
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	struct isp_occlusion_color *p_occ_color = (struct isp_occlusion_color *) parm.parm.raw_data;
	p_occ_color->type = ISP_PARM_OCCLUSION_COLOR;
	p_occ_color->color_type = 0;
	p_occ_color->transparency = 0;
	p_occ_color->y_component = RGB2Y(0, 0, 0);
	p_occ_color->u_component = RGB2U(0, 0, 0);
	p_occ_color->v_component = RGB2V(0, 0, 0);
	xioctl(new_camera->camera_fd, VIDIOC_S_PARM, &parm);

	
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
			fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
			fmt.fmt.pix.sizeimage = min;

#if 0
	fmt.fmt.pix.sizeimage += (new_camera->video_width*new_camera->video_height*3/2);
#endif

	camera_mmap(new_camera,fmt.fmt.pix.sizeimage);


	return(new_camera);

fail:

	if(new_camera->camera_fd > 0)
	{
		close(new_camera->camera_fd);
		new_camera->camera_fd = -1;

	}
	if(NULL != new_camera)
	{
		free(new_camera);
		new_camera = NULL;
	}
	return(NULL);
	
}

	





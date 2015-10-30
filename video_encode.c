#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include "anyka_types.h"
#include "akuio.h"
#include "dev_camera.h"
#include "video_encode.h"
#include "common.h"



#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"video_encode:"

#define	 	ENCMEM_BUFF_SIZE	(256*1024)





static T_pVOID enc_mutex_create(T_VOID)
{
	pthread_mutex_t *pMutex;
	pMutex = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(pMutex, NULL); 
	return pMutex;
}

static T_S32 enc_mutex_lock(T_pVOID pMutex, T_S32 nTimeOut)
{
	pthread_mutex_lock(pMutex);
	return 1;
}

static T_S32 enc_mutex_unlock(T_pVOID pMutex)
{
	pthread_mutex_unlock(pMutex);
	return 1;
}

static T_VOID enc_mutex_release(T_pVOID pMutex)
{
	int rc = pthread_mutex_destroy( pMutex ); 
    if ( rc  != 0 ) {                      
        pthread_mutex_unlock( pMutex );             
        pthread_mutex_destroy( pMutex );    
    } 
	free(pMutex);
}


T_BOOL ak_enc_delay(T_U32 ticks)
{
	akuio_wait_irq();
	return AK_TRUE;
}


int video_encode_init(void)
{
	T_VIDEOLIB_CB	init_cb_fun;
	int ret;

	memset(&init_cb_fun, 0, sizeof(T_VIDEOLIB_CB));
	init_cb_fun.m_FunPrintf			= NULL;
	init_cb_fun.m_FunMalloc  		= (MEDIALIB_CALLBACK_FUN_MALLOC)malloc;
	init_cb_fun.m_FunFree    		= free;
	init_cb_fun.m_FunRtcDelay       = ak_enc_delay;
				
	init_cb_fun.m_FunDMAMalloc		= (MEDIALIB_CALLBACK_FUN_DMA_MALLOC)akuio_alloc_pmem;
  	init_cb_fun.m_FunDMAFree		= (MEDIALIB_CALLBACK_FUN_DMA_FREE)akuio_free_pmem;
  	init_cb_fun.m_FunVaddrToPaddr	= (MEDIALIB_CALLBACK_FUN_VADDR_TO_PADDR)akuio_vaddr2paddr;
  	init_cb_fun.m_FunMapAddr		= (MEDIALIB_CALLBACK_FUN_MAP_ADDR)akuio_map_regs;
  	init_cb_fun.m_FunUnmapAddr		= (MEDIALIB_CALLBACK_FUN_UNMAP_ADDR)akuio_unmap_regs;
  	init_cb_fun.m_FunRegBitsWrite	= (MEDIALIB_CALLBACK_FUN_REG_BITS_WRITE)akuio_sysreg_write;
	init_cb_fun.m_FunVideoHWLock	= (MEDIALIB_CALLBACK_FUN_VIDEO_HW_LOCK)akuio_lock_timewait;
	init_cb_fun.m_FunVideoHWUnlock	= (MEDIALIB_CALLBACK_FUN_VIDEO_HW_UNLOCK)akuio_unlock;

	init_cb_fun.m_FunMutexCreate	= enc_mutex_create;
	init_cb_fun.m_FunMutexLock		= enc_mutex_lock;
	init_cb_fun.m_FunMutexUnlock	= enc_mutex_unlock;
	init_cb_fun.m_FunMutexRelease	= enc_mutex_release;
	
	ret = VideoStream_Enc_Init(&init_cb_fun);
	
	return ret;
}




void  video_encode_unint( )
{
	VideoStream_Enc_Destroy();
}



void * video_new_handle(video_type_m type)
{
	if(TYPE_VGA != type && TYPE_RECORD != type && TYPE_PIC)
	{
		dbg_printf("the type is not support ! \n");
		return(NULL);
	}
	video_encode_handle_t * new_handle = calloc(1,sizeof(*new_handle));
	if(NULL == new_handle)
	{
		dbg_printf("calloc is fail ! \n");
		return(NULL);
	}

	new_handle->handle = NULL;
	new_handle->pmem_addres = NULL;
	new_handle->pvirtual_addres = NULL;
	new_handle->pmem_addres = akuio_alloc_pmem(ENCMEM_BUFF_SIZE);
	if(NULL == new_handle->pmem_addres)
	{
		dbg_printf("akuio_alloc_pmem fail ! \n");
		goto fail;
	}
	unsigned int addres = akuio_vaddr2paddr(new_handle->pmem_addres) & 7;
	new_handle->pvirtual_addres = ((unsigned char *)new_handle->pmem_addres) + ((8-addres)&7);

	T_VIDEOLIB_ENC_OPEN_INPUT open_input;
	memset(&open_input,0,sizeof(open_input));
	open_input.encFlag = VIDEO_DRV_H264;
	open_input.encH264Par.width = VIDEO_WIDTH_VGA;		
	open_input.encH264Par.height = VIDEO_HEIGHT_VGA;			
	open_input.encH264Par.lumWidthSrc = VIDEO_WIDTH_VGA;
	open_input.encH264Par.lumHeightSrc = VIDEO_HEIGHT_VGA;
	open_input.encH264Par.horOffsetSrc = 0;
	open_input.encH264Par.verOffsetSrc = 0;
	open_input.encH264Par.rotation = ENC_ROTATE_0;		
	open_input.encH264Par.frameRateDenom = 1;
	open_input.encH264Par.frameRateNum = 15;	
	
	open_input.encH264Par.qpHdr = -1;		
  	open_input.encH264Par.streamType = 0;	
  	open_input.encH264Par.enableCabac = 1; 		
	open_input.encH264Par.transform8x8Mode = 0;
	
	open_input.encH264Par.qpMin = 10;        
	open_input.encH264Par.qpMax = 36;
	open_input.encH264Par.fixedIntraQp = 0;
    open_input.encH264Par.bitPerSecond = 400*1024/*500*1024*/;
    open_input.encH264Par.gopLen = 30; 

	new_handle->rc.qpHdr = -1;
	new_handle->rc.qpMin = open_input.encH264Par.qpMin;
	new_handle->rc.qpMax = open_input.encH264Par.qpMax;
	new_handle->rc.fixedIntraQp = open_input.encH264Par.fixedIntraQp;
	new_handle->rc.bitPerSecond =  open_input.encH264Par.bitPerSecond;
	new_handle->rc.gopLen = open_input.encH264Par.gopLen;


	new_handle->handle = VideoStream_Enc_Open(&open_input);
	if(!new_handle->handle)
	{
		dbg_printf("VideoStream_Enc_Open fail ! \n");
		goto fail ;
	}
	return(new_handle);

fail:

	if(NULL != new_handle->pmem_addres)
	{
		akuio_free_pmem(new_handle->pmem_addres);
		new_handle->pmem_addres = NULL;

	}
	if(NULL != new_handle)
	{
		free(new_handle);
		new_handle = NULL;
	}

	return(NULL);
	
}



int video_encode_frame(video_encode_handle_t * pencode_handle, void *pinbuf2, void **poutbuf2, frame_type_m  frame_type)
{

	if(NULL == pencode_handle || NULL == pencode_handle->handle ||  NULL == pencode_handle->pvirtual_addres)
	{
		dbg_printf("please check the param ! \n");
		return(-1);
		
	}
	if(I_FRAME != frame_type && P_FRAME != frame_type)
	{
		dbg_printf("check the frame type ! \n");
		return(-2);

	}
	T_VIDEOLIB_ENC_IO_PAR video_enc_io_param2;
    video_enc_io_param2.QP = 0;		
	video_enc_io_param2.mode = frame_type;
	
	video_enc_io_param2.p_curr_data = pinbuf2;		
	video_enc_io_param2.p_vlc_data = pencode_handle->pvirtual_addres;			
	video_enc_io_param2.out_stream_size = ENCMEM_BUFF_SIZE;

	VideoStream_Enc_Encode(pencode_handle->handle, NULL, &video_enc_io_param2, NULL);
	*poutbuf2 = video_enc_io_param2.p_vlc_data;

	return video_enc_io_param2.out_stream_size;
}


int video_encode_close(video_encode_handle_t * pencode_handle)
{
    
	VideoStream_Enc_Close(pencode_handle->handle);
	akuio_free_pmem(pencode_handle->pmem_addres);
    free(pencode_handle);
	return 0;
}


int video_encode_reSetRc(video_encode_handle_t * pencode_handle, int qp)
{

    pencode_handle->rc.bitPerSecond = qp*1024;
	return VideoStream_Enc_setRC(pencode_handle->handle, &pencode_handle->rc);
}


/*视频编码率900 Kbps，音频编码格式AAC，编码率161 Kbps。640*480的vga应该设置在700kbit之上*/
/*设置动态码率（VBR）和静态码率（CBR）参数*/
/*
 params.qpMin   = 1;//10;//change by chris 
            params.qpMax   = 51;//36;//change by chris 
            params.qpInit   = -1; 
 4）min qp 最小量化步长

说明：设置x264可以使用的最小量化器。量化参数越小，输出越接近输入。使用某些值时，
x264的输出可以和输入看起来完全一样，虽然其实并不是精确相同的，通常就够了没有必要使用更多比特在宏块上了。
如果开启了自适应量化器（默认开启），则不鼓励提高qpmin的值，那样可能会降低帧的平坦部分的质量。


5）max qp最大量化步长

说明：qpmin的反面，设置x264可以使用的最大量化器。默认值51是H.264标准中的最大值，质量非常低。默认值51其实相当于没有设置qpmax。如果你想控制x264输出的最低品质，也许你想要把这个值调低一点（调到30-40最低了），但一般而言不推荐调节这个值。

你好，

请问你使用的是CVR，还是VBR？

CBR下你的压缩比是和你设定的目标码率相关性的。

如果是VBR，也是和目标码流相关。如果要强行加大压缩比，一般h264算法都有设置max/min Qp的参数，设置最小Qp为较大值就可以增大压缩比，但图像质量会下降。


*/
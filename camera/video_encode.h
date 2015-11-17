#ifndef _video_encode_h
#define	_video_encode_h

#include "video_stream_lib.h"

typedef enum
{
	TYPE_VGA = 0x01,
	TYPE_RECORD,
	TYPE_PIC,
	TYPE_UNKNOW
}video_type_m;


typedef enum
{
	I_FRAME = 0x00,
	P_FRAME,
	UNKNOW_FRAME
}frame_type_m;


typedef struct video_encode_handle
{
	void * handle;
	void * pvirtual_addres;
	void * pmem_addres;
	T_VIDEOLIB_ENC_RC rc;
	
}video_encode_handle_t;


int video_encode_init(void);


void * video_new_handle(unsigned int bps);
int video_encode_frame(video_encode_handle_t * pencode_handle, void *pinbuf2, void **poutbuf2, frame_type_m  frame_type);
int video_encode_close(video_encode_handle_t * pencode_handle);
int video_encode_reSetRc(video_encode_handle_t * pencode_handle, int qp);




#endif /*_video_encode_h*/
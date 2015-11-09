#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "common.h"

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"main"





#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>

#ifdef __cplusplus
};
#endif




int main(int argc,char * argv[])
{
	dbg_printf("this is a test ! \n");
	AVCodec *pCodec;
    AVCodecContext *pCodecCtx= NULL;
	AVCodecParserContext *pCodecParserCtx=NULL;

    FILE *fp_in;
	FILE *fp_out;
    AVFrame	*pFrame;
	
	const int in_buffer_size=4096;
	unsigned char in_buffer[in_buffer_size + FF_INPUT_BUFFER_PADDING_SIZE]={0};
	unsigned char *cur_ptr;
	int cur_size;
    AVPacket packet;
	int ret, got_picture;
	int y_size;

	AVCodecID codec_id=AV_CODEC_ID_H264;
	char filepath_in[]="yuv_file.264";
	char filepath_out[]="bigbuckbunny_480x272.yuv";
	int first_time=1;

	avcodec_register_all();
    pCodec = avcodec_find_decoder(codec_id);
    if (!pCodec) 
	{
        dbg_printf("Codec not found\n");
        return -1;
    }
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx)
	{
        dbg_printf("Could not allocate video codec context\n");
        return -1;
    }

	pCodecParserCtx=av_parser_init(codec_id);
	if (!pCodecParserCtx)
	{
		dbg_printf("Could not allocate video parser context\n");
		return -1;
	}

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) 
	{
        dbg_printf("Could not open codec\n");
        return -1;
    }

    fp_in = fopen(filepath_in, "rb");
    if (!fp_in)
	{
        dbg_printf("Could not open input stream\n");
        return -1;
    }

	fp_out = fopen(filepath_out, "wb");
	if (!fp_out)
	{
		dbg_printf("Could not open output YUV file\n");
		return -1;
	}

    pFrame = av_frame_alloc();
	av_init_packet(&packet);

	while (1) {

        cur_size = fread(in_buffer, 1, in_buffer_size, fp_in);
        if (cur_size == 0)
            break;
        cur_ptr=in_buffer;

        while (cur_size>0)
		{

			int len = av_parser_parse2(   /*解析获得一个Packet,如果是流，这里完全可以省略*/
				pCodecParserCtx, pCodecCtx,
				&packet.data, &packet.size,
				cur_ptr , cur_size ,
				AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);

			cur_ptr += len;
			cur_size -= len;

			if(packet.size==0)
				continue;
			dbg_printf("[Packet]Size:%6d\t",packet.size);
			switch(pCodecParserCtx->pict_type)
			{
				case AV_PICTURE_TYPE_I: printf("Type:I\t");break;
				case AV_PICTURE_TYPE_P: printf("Type:P\t");break;
				case AV_PICTURE_TYPE_B: printf("Type:B\t");break;
				default: printf("Type:Other\t");break;
			}
			printf("Number:%4d\n",pCodecParserCtx->output_picture_number);

			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, &packet);
			if (ret < 0) {
				printf("Decode Error.\n");
				return ret;
			}
			if (got_picture)
			{
				int i=0;
				unsigned char* tempptr=NULL;
				tempptr=pFrame->data[0];
				for(i=0;i<pFrame->height;i++){
					fwrite(tempptr,1,pFrame->width,fp_out);     //Y 
					tempptr+=pFrame->linesize[0];
				}
				tempptr=pFrame->data[1];
				for(i=0;i<pFrame->height/2;i++){
					fwrite(tempptr,1,pFrame->width/2,fp_out);   //U
					tempptr+=pFrame->linesize[1];
				}
				tempptr=pFrame->data[2];
				for(i=0;i<pFrame->height/2;i++){
					fwrite(tempptr,1,pFrame->width/2,fp_out);   //V
					tempptr+=pFrame->linesize[2];
				}
				dbg_printf("Succeed to decode 1 frame!\n");
			}
		}

    }

    packet.data = NULL;
    packet.size = 0;
	while(1){
		ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, &packet);
		if (ret < 0)
		{
			dbg_printf("Decode Error.\n");
			return ret;
		}
		if (!got_picture)
			break;
		if (got_picture)
		{
			
			int i=0;
			unsigned char* tempptr=NULL;
			tempptr=pFrame->data[0];
			for(i=0;i<pFrame->height;i++){
				fwrite(tempptr,1,pFrame->width,fp_out);     //Y 
				tempptr+=pFrame->linesize[0];
			}
			tempptr=pFrame->data[1];
			for(i=0;i<pFrame->height/2;i++){
				fwrite(tempptr,1,pFrame->width/2,fp_out);   //U
				tempptr+=pFrame->linesize[1];
			}
			tempptr=pFrame->data[2];
			for(i=0;i<pFrame->height/2;i++){
				fwrite(tempptr,1,pFrame->width/2,fp_out);   //V
				tempptr+=pFrame->linesize[2];
			}

			dbg_printf("Flush Decoder: Succeed to decode 1 frame!\n");
		}
	}

    fclose(fp_in);
	fclose(fp_out);
	av_parser_close(pCodecParserCtx);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	av_free(pCodecCtx);
	return(0);
}








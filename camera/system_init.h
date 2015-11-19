

#ifndef  _system_init_h
#define _system_init_h



#include "socket_init.h"
#include "net_send.h"
#include "net_recv.h"
#include "handle_packet.h"
#include "monitor_user.h"
#include "video_stream.h"



#define  MAX_USER   (2)

typedef  struct client_user
{
	char is_run;
	char id_num;
	
	int qp;
	char iframe_gap;
	char force_iframe;
	char frame_count;
	void * encode_handle;
	struct sockaddr addr;
	int beatheart;
	
	
}client_user_t;









typedef struct camera_handle
{
	
	socket_handle_t * socket;
	net_send_handle_t * send;
	net_recv_handle_t * recv;
	handle_packet_fun_t * fun;
	monitor_user_t * monitor;
	video_stream_handle_t * video;

	int user_count;
	client_user_t *user[MAX_USER];

}camera_handle_t;



extern  camera_handle_t * camera;

int system_init(void);


#endif  /*_system_init_h*/
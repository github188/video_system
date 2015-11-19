#include "system_init.h"
#include "akuio.h"
#include "video_encode.h"



#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"system_init"



camera_handle_t * camera = NULL;



static int system_lib_init(void)
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

int system_init(void)
{

	int ret = -1;
	int i = 0;

	ret = system_lib_init();
	if(0 != ret)
	{
		dbg_printf("system_lib_init is fail ! \n");
		return(-1);

	}
	ret = inipare_init();
	if(0 != ret)
	{
		dbg_printf("inipare_init is fail ! \n");
		return(-1);
	}
	
//	ret = link_net();

	

	if(NULL != camera)
	{
		dbg_printf("camera has been init ! \n");
		return(-1);
	}

	camera = calloc(1,sizeof(*camera));
	if(NULL == camera)
	{
		dbg_printf("calloc is fail ! \n");
		return(-2);
	}


	camera->user_count = 0;
	for(i=0;i<MAX_USER; ++i)
	{
		camera->user[i] = calloc(1,sizeof(client_user_t));
		if(NULL == camera->user[i])return(-1);
		camera->user[i]->is_run = 0;
		camera->user[i]->id_num = i;
		camera->user[i]->iframe_gap = 30;
		camera->user[i]->frame_count = 0;
		camera->user[i]->encode_handle = video_new_handle(500*1024);
	}
	

	camera->socket = (socket_handle_t*)socket_socket_new();
	if(NULL == camera->socket)
	{
		dbg_printf("socket_socket_new is fail ! \n");
		goto fail;
	}


	camera->send = (net_send_handle_t*)send_handle_new();
	if(NULL == camera->send)
	{
		dbg_printf("send_handle_new is fail ! \n");
		goto fail;
	}
	if(NULL == camera->send->send_fun || NULL == camera->send->resend_fun)
	{
		dbg_printf("check the pthread fun of send and resend ! \n");
		goto fail;
	}


	camera->fun = get_handle_packet_fun();
	if(NULL == camera->fun)
	{
		dbg_printf("get_handle_packet_fun error ! \n");
		goto fail;

	}

	camera->recv = recv_handle_new();
	if(NULL == camera->recv)
	{
		dbg_printf("recv_handle_new is fail ! \n");
		goto fail;
	}
	if(NULL == camera->recv->recv_fun|| NULL == camera->recv->process_fun)
	{
		dbg_printf("check the pthread fun of recv and process_fun ! \n");
		goto fail;
	}


	camera->monitor = (monitor_user_t*)monitor_handle_new();
	if(NULL == camera->monitor)
	{
		dbg_printf("monitor_handle_new is fail ! \n");
		goto fail;
	}

	camera->video = video_stream_new_handle();
	if(NULL == camera->video)
	{
		dbg_printf("video_stream_new_handle is fail ! \n");
		goto fail;

	}

	
	ret = pthread_create(&camera->send->netsend_ptid,NULL,camera->send->send_fun,camera);
	pthread_detach(camera->send->netsend_ptid);

	ret = pthread_create(&camera->send->netresend_ptid,NULL,camera->send->resend_fun,camera);
	pthread_detach(camera->send->netresend_ptid);



	ret = pthread_create(&camera->recv->netrecv_ptid,NULL,camera->recv->recv_fun,camera);
	pthread_detach(camera->recv->netrecv_ptid);

	ret = pthread_create(&camera->recv->netprocess_ptid,NULL,camera->recv->process_fun,camera);
	pthread_detach(camera->recv->netprocess_ptid);


	ret = pthread_create(&camera->monitor->monitor_ptid,NULL,camera->monitor->monitor_fun,camera);
	pthread_detach(camera->monitor->monitor_ptid);


	ret = pthread_create(&camera->video->capature_ptid,NULL,camera->video->capature_fun,camera);
	pthread_detach(camera->video->capature_ptid);
	ret = pthread_create(&camera->video->video_encode_ptid,NULL,camera->video->video_encode_fun,camera);
	pthread_detach(camera->video->video_encode_ptid);


	

	return(0);


fail:

	if(NULL != camera->socket)
	{
		socket_socket_destroy(camera->socket);
	}

	if(NULL != camera->send)
	{
		send_handle_destroy(camera->send);
	}

	if(NULL != camera->recv)
	{
		recv_handle_destroy(camera->recv);	
	}

	if(NULL != camera->monitor)
	{
		monitor_handle_destroy(camera->monitor);	
	}

	if(NULL != camera->video)
	{
		video_stream_handle_destroy(camera->video);
		
	}

	if(NULL != camera)
	{
		free(camera);
		camera = NULL;

	}

	return(-1);


}






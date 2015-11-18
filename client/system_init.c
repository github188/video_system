#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "common.h"
#include "system_init.h"





#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"system_init"









camera_handle_t * camera = NULL;





int system_init(void)
{

	int ret = -1;


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

	camera->socket = (socket_handle_t*)socket_socket_new();
	if(NULL == camera->socket)
	{
		dbg_printf("socket_socket_new is fail ! \n");
		goto fail;
	}

#if 1
	struct sockaddr_in * localaddr = (struct sockaddr_in *)&camera->socket->localaddr;
	char * ip = socket_ntop(&camera->socket->localaddr);
	dbg_printf("ip is %s port == %d \n",ip,ntohs(localaddr->sin_port));
	free(ip);
#endif

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

	
	ret = pthread_create(&camera->send->netsend_ptid,NULL,camera->send->send_fun,camera);
	pthread_detach(camera->send->netsend_ptid);

	ret = pthread_create(&camera->send->netresend_ptid,NULL,camera->send->resend_fun,camera);
	pthread_detach(camera->send->netresend_ptid);



	ret = pthread_create(&camera->recv->netrecv_ptid,NULL,camera->recv->recv_fun,camera);
	pthread_detach(camera->recv->netrecv_ptid);

	ret = pthread_create(&camera->recv->netprocess_ptid,NULL,camera->recv->process_fun,camera);
	pthread_detach(camera->recv->netprocess_ptid);


	

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

	if(NULL != camera)
	{
		free(camera);
		camera = NULL;

	}

	return(-1);


}






#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
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



int main(int argc,char * argv[])
{

	int ret = -1;
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
	send_peer_packet();

	while(1)
	{

		
		sleep(10);
	//	dbg_printf("this is a test ! \n");
	}
	

	return(0);
}








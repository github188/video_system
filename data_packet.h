#ifndef _data_packet_h
#define _data_packet_h 

#include <sys/types.h>
#include <sys/socket.h>



typedef enum packet_reliable
{
	RELIABLE_PACKET = 0x01,
	UNRELIABLE_PACKET,
}packet_reliable_m;



typedef struct send_packet
{
	int sockfd;
	void * data;
	int length;
	struct sockaddr to;

	
	packet_reliable_m type;
	char is_resend;
	char resend_times;
	long index;
	struct timeval tp;
	struct timeval ta;
	
}send_packet_t;







#endif  /*_data_packet_h*/
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




typedef enum
{

	DUMP_PACKET = 100,
	REGISTER_PACKET,
	REGISTER_PACKET_ASK,
	LOIN_PACKET,
	LOIN_PACKET_ASK,
	BEATHEART_PACKET,
	BEATHEART_PACKET_ASK,
	UNKNOW_PACKET,
	
}packet_type_m;


typedef struct packet_header
{
	packet_type_m type;
	unsigned packet_len;
	int index;
}packet_header_t;



typedef struct register_packet
{
	packet_header_t head;
	char x;
}register_packet_t;


typedef struct register_ask_packet
{
	packet_header_t head;
	char x;
}register_ask_packet_t;








#endif  /*_data_packet_h*/
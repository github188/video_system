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
	PEER_PACKET,
	PEER_PACKET_ASK,
	LOIN_PACKET,
	LOIN_PACKET_ASK,
	BEATHEART_PACKET,
	BEATHEART_PACKET_ASK,
	IFRAME_PACKET,
	PFRAME_PACKET,
	UNKNOW_PACKET,
	
}packet_type_m;


typedef struct packet_header
{
	packet_type_m type;
	unsigned packet_len;
	int index;
	char ret;
}packet_header_t;



typedef struct register_packet
{
	packet_header_t head;
	char x;
	char dev_name[64];
	struct sockaddr localaddr;
}register_packet_t;


typedef struct register_ask_packet
{
	packet_header_t head;
	char x;
	char dev_name[64];
	struct sockaddr localaddr;
}register_ask_packet_t;




typedef struct peer_packet
{
	packet_header_t head;
	char p;
	char dev_name[64];
	struct sockaddr localaddr;
}peer_packet_t;


typedef struct peer_ask_packet
{
	packet_header_t head;
	char p;
	char dev_name[64];
	struct sockaddr dev_addr;
}peer_ask_packet_t;





typedef struct loin_packet
{
	packet_header_t head;
	char dev_name[64];
	char l;
	struct sockaddr dev_addr;

}loin_packet_t;


typedef struct loin_packet_ask
{
	packet_header_t head;
	char l;
	char id_num;
}loin_packet_ask_t;



typedef struct beartheart_packet
{
	packet_header_t head;
	char id_num;
}beartheart_packet_t;




typedef struct iframe_packet
{
	packet_header_t head;
	int  id_packet;
	char sum_packets;
	char index_packet;
	char data[1024];
}iframe_packet_t;



typedef struct pframe_packet
{
	packet_header_t head;
	int  id_packet;
	int length;
	char data[0];
}pframe_packet_t;

















#endif  /*_data_packet_h*/
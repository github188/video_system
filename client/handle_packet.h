#ifndef  _handle_packet_h
#define  _handle_packet_h



#ifdef __cplusplus
#include <iostream>
using namespace std;
extern "C"
{
#endif


int handle_packet_init(void);
int  send_peer_packet(void);
int  handle_peer_ask(void * arg);
int  handle_loin_ask(void * arg);
int send_active_channel_packet(void * dest_addr);
int  handle_active_channel_ask(void * arg);



#ifdef __cplusplus
}
#endif



#endif  /*_handle_packet_h*/
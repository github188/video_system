#ifndef  _handle_packet_h
#define  _handle_packet_h

int handle_packet_init(void);
int  send_register_packet(void);
int  handle_register_ask(void * arg);

#endif  /*_handle_packet_h*/
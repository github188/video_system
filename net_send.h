#ifndef  _net_send_h
#define  _net_send_h



int  netsend_start_up(void);
int  netsend_push_msg(void * data );
int  netsend_remove_packet(int index);


#endif  /*_net_send_h*/
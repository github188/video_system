#ifndef  _net_send_h
#define  _net_send_h

#ifdef __cplusplus
extern "C"
{
#endif

int  netsend_start_up(void);
int  netsend_push_msg(void * data );
int  netsend_remove_packet(int index);


#ifdef __cplusplus
};
#endif



#endif  /*_net_send_h*/
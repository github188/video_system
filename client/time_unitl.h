#ifndef _time_unitl_h
#define _time_unitl_h


int get_time(struct timeval *tp);
void add_time(struct timeval *a, struct timeval *b,struct timeval *res);
void sub_time(struct timeval *a, struct timeval *b,struct timeval *res);
void time_sleep(unsigned int second,unsigned int usec);




#endif  /*_time_unitl_h*/
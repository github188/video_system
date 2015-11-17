#ifndef _eventfd_h
#define _eventfd_h


int eventfd_new(void);
int eventfd_wakeup(int fd);
int eventfd_clean(int fd);




#endif  /*_eventfd_h*/
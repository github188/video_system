#ifndef _parser_inifile_h
#define	_parser_inifile_h
#include "queue.h"
#include <pthread.h>

struct ini_node
{
    char * index;
	char * value;
    TAILQ_ENTRY(ini_node) links; 
};

typedef  struct ini_handle
{
	char * file_path;
	pthread_rwlock_t rwlock;
	TAILQ_HEAD(,ini_node)ini_queue;
}ini_handle_t;



int  inipare_init(void);
int  inipare_read(char * index,char ** value);
int  inipare_write(char * index,char *value);
	


#endif  /*_parser_inifile_h*/

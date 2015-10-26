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


void*  inifile_new_handle(const char * path);
void  inifile_free_handle(void * handle);
int inifile_read_node(void * handle,char * index,char ** value);
int inifile_write_node(void * handle,char * index,char *value);
	


#endif  /*_parser_inifile_h*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "parser_inifile.h"
#include "common.h"


#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"parser_inifile:"


static int inifile_init_config(void * handle)
{
	if(NULL == handle)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}

	ini_handle_t * handle_ini = (ini_handle_t * )handle;
	
	FILE * pfile = fopen(handle_ini->file_path,"r");
	if(NULL == pfile)
	{
		dbg_printf("fopen  the file is fail ! \n");
		return(-2);
	}
	char buff[1024];
	char * presult = NULL;
	int i = 0;
	int j = 0;
	
	while(! feof(pfile) )
	{
		memset(buff,'\0',1024);
		i =0;j=0;
		presult = fgets(buff,1024,pfile);
		if(NULL == presult )continue;
		while('\0' != buff[i])
		{
			/*空白符指空格、水平制表、垂直制表、换页、回车和换行符。*/
			if(! isspace(buff[i]))
			{
				buff[j] = buff[i];
				j += 1;
			}
			i += 1;

		}
		buff[j] = '\0';

		
		if(0 == strlen(buff))continue;


		char * pstr = strstr(buff,"=");
		if(NULL == pstr)
		{
			continue;
		}
		*pstr = '\0';
		
		struct ini_node * node = calloc(1,sizeof(*node));
		if(NULL == node)
		{
			dbg_printf("calloc is  fail ! \n");
			continue;
		}
		asprintf(&node->index,"%s",buff);

		if(NULL == pstr+1)
		{
			node->value = NULL;
		}
		else
		{
			asprintf(&node->value,"%s",pstr+1);	
		}
		TAILQ_INSERT_TAIL(&handle_ini->ini_queue,node,links);
	}

	fclose(pfile);
	pfile = NULL;

	return(0);
}



static int inifile_rewrite(void * handle)
{
	if(NULL == handle)
	{
		dbg_printf("please check the param ! \n");
		return(-1);
	}
	ini_handle_t * handle_ini = (ini_handle_t * )handle;
	FILE * pfile = fopen(handle_ini->file_path,"w+");
	if(NULL == pfile)
	{
		dbg_printf("fopen  fail ! \n");
		return(-2);
	}

	struct ini_node * node = NULL;
	char buff[1024];
	int i = 0;

    TAILQ_FOREACH(node,&handle_ini->ini_queue,links)
    {
		memset(buff,'\0',1024);
		if(NULL == node->value)
		{
			snprintf(buff,1024,"%-32s=\n",node->index);
		}
		else
		{
			snprintf(buff,1024,"%-32s=\t%s\n",node->index,node->value);
		}
		fwrite(buff,1,strlen(buff),pfile);
    }
	fclose(pfile);
	pfile = NULL;
	return(0);
}


static void*  inifile_new_handle(const char * path)
{
	int ret = -1;
	if(NULL == path || access(path,F_OK) != 0)
	{
		dbg_printf("please check the param ! \n");
		return(NULL);
	}
	ini_handle_t * handle_ini = NULL;
	handle_ini = calloc(1,sizeof(*handle_ini));
	if(NULL == handle_ini)
	{
		dbg_printf("calloc fail ! \n");
		return(NULL);
	}
	ret = pthread_rwlock_init(&handle_ini->rwlock,NULL);
	if(0 != ret )
	{
		dbg_printf("pthread_rwlock_init  fail ! \n");
		if(NULL != handle_ini)
		{
			free(handle_ini);
			handle_ini = NULL;
		}
		return(NULL);
	}
	asprintf(&handle_ini->file_path,"%s",path);
	TAILQ_INIT(&handle_ini->ini_queue);
	inifile_init_config(handle_ini);
	return(handle_ini);
	
}


static void  inifile_free_handle(void * handle)
{
	int ret = -1;
	if(NULL == handle)
	{
		dbg_printf("the handle is null ! \n");
		return;
	}
	ini_handle_t * handle_ini = (ini_handle_t*)handle;
	struct ini_node * node = NULL;
	pthread_rwlock_wrlock(&handle_ini->rwlock);
	TAILQ_FOREACH(node,&handle_ini->ini_queue,links)
	{

			TAILQ_REMOVE(&handle_ini->ini_queue,node,links);
			if(NULL != node->index)
			{
				free(node->index);
				node->index = NULL;
			}
			if(NULL != node->value)
			{
				free(node->value);
				node->value = NULL;
			}
			free(node);
			node = NULL;	
	}

	if(NULL != handle_ini->file_path)
	{
		free(handle_ini->file_path);
		handle_ini->file_path = NULL;
	}
	pthread_rwlock_destroy(&handle_ini->rwlock);

	free(handle_ini);
	handle_ini = NULL;
	

	
}




static  int inifile_read_node(void * handle,char * index,char ** value)
{
	if(NULL == handle || NULL == index)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}
	ini_handle_t * handle_ini = (ini_handle_t * )handle;
	struct ini_node * node = NULL;
	pthread_rwlock_rdlock(&handle_ini->rwlock);
    TAILQ_FOREACH(node,&handle_ini->ini_queue,links)
    {
		if(0 == strcmp(node->index,index))
		{
			asprintf(value,"%s",node->value);
			pthread_rwlock_unlock(&handle_ini->rwlock);
			return(0);
		}
    }
	*value = NULL;
	pthread_rwlock_unlock(&handle_ini->rwlock);
	return(-1);
}


static int inifile_write_node(void * handle,char * index,char *value)
{
	if(NULL == handle || NULL == index)
	{
		dbg_printf("check the param ! \n");
		return(-1);
	}
	ini_handle_t * handle_ini = (ini_handle_t * )handle;
	struct ini_node * node = NULL;
	
	
	pthread_rwlock_wrlock(&handle_ini->rwlock);
    TAILQ_FOREACH(node,&handle_ini->ini_queue,links)
    {
		if(0 != strcmp(node->index,index))continue;
		if(NULL != node->value)
		{
			free(node->value);
			node->value = NULL;
		}
		asprintf(&node->value,"%s",value);
		inifile_rewrite(handle);
		pthread_rwlock_unlock(&handle_ini->rwlock);
		return(0);
    }
	pthread_rwlock_unlock(&handle_ini->rwlock);
	return(-2);
}




#define	 CONDIF_FILE		"/var/huiwei/config.ini"

static ini_handle_t * pini_handle = NULL;


int  inipare_init(void)
{
	if(NULL != pini_handle)
	{
		dbg_printf("the handle of inipare has  been init ! \n");
		return(-1);
	}
	pini_handle = inifile_new_handle(CONDIF_FILE);
	if(NULL == pini_handle)
	{
		dbg_printf("inifile_new_handle  fail ! \n");
		return(-2);
	}
	return(0);
}



int  inipare_read(char * index,char ** value)
{
	if(NULL == pini_handle)
	{
		dbg_printf("please init the inipare handle \n");
		*value = NULL;
		return(-1);
	}
	return(inifile_read_node(pini_handle,index,value));
}



int  inipare_write(char * index,char *value)
{
	if(NULL == pini_handle)
	{
		dbg_printf("please init the inipare handle \n");
		return(-1);
	}
	return(inifile_write_node(pini_handle,index,value));
}





int test_inifile(void)
{

	dbg_printf("this is null ! \n");
	ini_handle_t * handle = inifile_new_handle("/var/huiwei/test.ini");
	char * ip =NULL;
	inifile_read_node(handle,"sound_mode",&ip);
	if(NULL != ip)
	{
		dbg_printf("the mode is=%s  \n",ip);
		free(ip);
		ip = NULL;
	}
	inifile_write_node(handle,"sound_mode","2");
	inifile_read_node(handle,"sound_mode",&ip);
	if(NULL != ip)
	{
		dbg_printf("the mode is=%s  \n",ip);
		free(ip);
		ip = NULL;
	}
	
	
	return(0);
}



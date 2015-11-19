#include "link_net.h"


#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"start_up"




#define WPAFILE_FILE_PATH	"/var/huiwei/wpa_supplicant.conf"

#define  ETH_DEV_NAME	"eth0"
#define	 WIFI_DEV_NAME	"wlan0"


typedef  enum
{
	STARTUP_WIFI,
	STARTUP_NET,
	STARTUP_UNKNOW,
}net_type_m;


typedef struct wifi_info
{
	char * ssid;
	char * pswd;
	char * encrypt;
	
}wifi_info_t;



static  volatile int net_type = 3; 




void free_wifi_node(wifi_info_t * wifi)
{
	if(NULL == wifi)return;
	if(NULL != wifi->ssid )
	{
		free(wifi->ssid);
		wifi->ssid = NULL;
	}

	if(NULL != wifi->pswd)
	{
		free(wifi->pswd);
		wifi->pswd = NULL;
		
	}
	if(NULL != wifi->encrypt)
	{
		free(wifi->encrypt);
		wifi->encrypt = NULL;
	}
	free(wifi);
	wifi = NULL;

}


static net_type_m start_check_net(void)
{
	system("ifconfig eth0 up && sleep 2 ");
	FILE * pnet = fopen("/sys/class/net/eth0/carrier","r");
	if(NULL == pnet)
	{
		dbg_printf("open the file fail ! \n");
		return(STARTUP_WIFI);
	}
	char is_net;
	net_type_m type;
	fread(&is_net,1,1,pnet);
	if('1' == is_net)
	{
		type = STARTUP_NET;	
		
	}
	else
	{
		type = STARTUP_WIFI;
		
	}
	net_type = type;
	fclose(pnet);
	pnet = NULL;
	return(type);
}


static struct sockaddr_in* start_get_addres(const char * device_name) 
 { 
	int ret = 0; 
	struct ifreq req; 
	struct sockaddr_in* host = NULL; 
	if(NULL == device_name)return((struct sockaddr_in*)0);
	host = calloc(1,sizeof(struct sockaddr_in));
	if(NULL == host )return(NULL);

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
	if ( -1 == sockfd )
	{
		dbg_printf("socket fail\n");
		goto fail;
	}
	bzero(&req, sizeof(struct ifreq)); 
	strcpy(req.ifr_name, device_name); 
	if ( ioctl(sockfd, SIOCGIFADDR, &req) >= 0 ) 
	{ 
		 memmove(host,(struct sockaddr_in*)&req.ifr_addr,sizeof(*host)); 
	} 
	else 
	{ 
		goto fail;
		
	} 
	close(sockfd); 
	sockfd = -1; 
	return host; 

fail:
	if(NULL != host)
	{
		free(host);
		host = NULL;
	}
	if(sockfd > 0)
	{
		close(sockfd);
	}
	return(NULL);
} 



	
static wifi_info_t * start_get_wifi(void)
{

	wifi_info_t * wifi = calloc(1,sizeof(wifi_info_t));
	if(NULL == wifi)
	{
		dbg_printf("calloc is fail ! \n");
		return(NULL);
	}

	inipare_read("wifi_ssid",&wifi->ssid);
	inipare_read("wifi_pswd",&wifi->pswd);
	inipare_read("wifi_secy",&wifi->encrypt);
		
	return(wifi);
}




static int start_config_wifi(void)
{

	wifi_info_t * wifi = start_get_wifi();
	if(NULL == wifi)
	{
		dbg_printf("please check the param\n");
		return(-1);
	}
	const char WPAstr[]={"ap_scan=1\nnetwork={\n\tssid=\"%s\"\n\tscan_ssid=1\n\tpsk=\"%s\"\n}"};
	const char OPENstr[]={"ap_scan=1\nnetwork={\n\tssid=\"%s\"\n\tkey_mgmt=NONE\n\tscan_ssid=1\n}"};
	const char WEPstr[]={"ap_scan=1\nnetwork={\n\tssid=\"%s\"\n\tkey_mgmt=NONE\n\twep_key0=\"%s\"\n\twep_tx_keyidx=0\n\tscan_ssid=1\n}"};
	char CmdStr[1024];
	memset(CmdStr,'\0',1024);

	if(NULL !=wifi->encrypt && NULL != wifi->pswd && NULL != wifi->ssid)
	{
		if('0' == wifi->encrypt[0])
		{
			sprintf(CmdStr, OPENstr, wifi->ssid);	
		}
		else if('2' == wifi->encrypt[0])
		{
			sprintf(CmdStr, WEPstr, wifi->ssid, wifi->pswd);
		}
		else
		{
			sprintf(CmdStr, WPAstr, wifi->ssid, wifi->pswd);
		}
		FILE *fd = NULL;
		fd=fopen(WPAFILE_FILE_PATH, "w+");
		if( fd == NULL ) 
		{
			dbg_printf("file open can not create file !!! \n");
			return -4;
		}
		fprintf(fd,"%s", CmdStr);
		fclose(fd);
		//system(" insmod  /root/8189es_smartlink.ko && ifconfig wlan0 up ");
		system(" insmod  /mnt/mmc/8189es.ko  && ifconfig wlan0 up ");
		system(" wpa_supplicant -B -iwlan0 -Dwext -c /var/huiwei/wpa_supplicant.conf ");
		system(" udhcpc -i wlan0  ");
 	}
	else
	{
		dbg_printf("please check the ini file ! \n");
	}

	free_wifi_node(wifi);
	wifi = NULL;
	return 0;
	
}



static void * start_net(void * arg)
{
	FILE   *file_net = NULL;

	char cmd[128];
	net_type_m type = start_check_net();
	if(STARTUP_NET == type)
	{
		memset(cmd,'\0',128);
		snprintf(cmd,128,"ifconfig %s up && udhcpc -i %s ",ETH_DEV_NAME,ETH_DEV_NAME);
		system(cmd);
	}
	else if(STARTUP_WIFI == type)
	{
		start_config_wifi();

	}
	struct sockaddr_in * paddres = NULL;
	while(1)
	{
		paddres	= start_get_addres((type==STARTUP_WIFI ? (WIFI_DEV_NAME) : (ETH_DEV_NAME)));
		if(NULL != paddres)
		{
			free(paddres);
			paddres = NULL;
			break;
		}
		sleep(1);
	}

	dbg_printf("net is ok ! \n");
	
}






int link_net(void)
{

	int ret = -1;
	pthread_t wifi_tid_once;
	ret = pthread_create(&wifi_tid_once,NULL,start_net,NULL);

	pthread_join(wifi_tid_once,NULL);


	dbg_printf("start up finished ! \n");
	
	return(0);
}



int get_net_type(void)
{

	return(net_type);

}
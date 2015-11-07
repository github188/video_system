
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>



#undef  DBG_ON
#undef  FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"01_udp_server:"
#define 	dbg_printf(fmt,arg...) \
	do{if(DBG_ON)fprintf(stderr,FILE_NAME"%s(line=%d)->"fmt,__FUNCTION__,__LINE__,##arg);}while(0)

	

#undef MAXLINE
#define MAXLINE   1024

#define	 SERVER_PORT	(8003u)


void dg_echo(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen)
{
	int			n;
	socklen_t	len;
	char		mesg[MAXLINE];

	for ( ; ; ) 
	{
		len = clilen;
		n = recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);
		dbg_printf("the data is coming ! \n");

	}
}


int main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_in	servaddr, cliaddr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERVER_PORT);
	int n = 0;
	n = 220 * 1024;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n));

	bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	dg_echo(sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
}

struct sockaddr
{
	unsigned short sa_family;
	char sa_data[14];
};

typedef struct in_addr
{
 
	unsigned long s_addr;
 
};

struct sockaddr_in
{
 
	short sin_family;
	unsigned short sin_port;
	struct in_addr sin_addr;
	unsigned char sin_zero[8];

};


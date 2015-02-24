#include "odr_api.h"
#include "odr_protocol.h"
#include<sys/socket.h>
#include<sys/un.h>
#include<errno.h>
#include"common_functions.h"
int msg_send(int sockfd, char *dest_ip, int dest_port, char *msg, int route_disc )
{
	struct sockaddr_un servaddr;
	struct app_data appdata;
	char localhostname[100];
	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	memset(&appdata, 0 , sizeof(struct app_data));
	strncpy(appdata.remote_ip, dest_ip, IP_LEN);
	appdata.remote_port = dest_port;
	memcpy(appdata.userdata, msg, 1024);
	appdata.route_disc = route_disc;
	appdata.app_register = 0;
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path,odr_sun_path);
	gethostname(localhostname, 100);
	//printf("sending to odr layer\n");
	//printf(" sockfd = %d sun path=%s\n",sockfd, servaddr.sun_path);
	//printf(" dest_ip = %s dest_port = %d msg = %s\n route =%d ", dest_ip, dest_port, msg, route_disc);
	int i=1;
	while(i--)
	{
		if(sendto(sockfd, &appdata, sizeof(appdata), 0, (struct sockaddr*)&servaddr, sizeof(struct sockaddr_un)) <0)
		{
			printf(" send failed in api %s \n", strerror(errno));
		}
	}
	return 0;
}
int msg_recv(int sockfd, char *remote_ip, int *remote_port, char *msg)
{
	struct sockaddr_un servaddr;
	struct app_data appdata;
	int recv = 0;
	socklen_t sockun_len = sizeof(struct sockaddr_un);
	recv = recvfrom(sockfd, &appdata, sizeof(struct app_data), 0, (struct sockaddr *)&servaddr, &sockun_len);
	if(recv > 0)
	{
		strncpy(remote_ip, appdata.remote_ip, IP_LEN);
		*remote_port = appdata.remote_port;
		memcpy(msg, appdata.userdata, 1024);
	}
}
void odr_register(int sockfd)
{
	printf(" entered odr register\n");
	struct sockaddr_un servaddr;
	struct app_data appdata;
	memset(&appdata, 0, sizeof(struct app_data));
	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	appdata.app_register = 1;
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, odr_sun_path);
	if(sendto(sockfd, &appdata, sizeof(appdata), 0, (struct sockaddr*)&servaddr, sizeof(struct sockaddr_un)) < 0)
	{
		printf(" register failed %s\n",strerror(errno));
	}
	printf(" leaving odr register\n");
}
char *odr_gethostname(void)
{
	return gethostbyip(canonical_ip);
}

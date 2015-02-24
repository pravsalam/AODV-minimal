#include<stdio.h>
#include "odr_api.h"
#include<sys/socket.h>
#include<netdb.h>
#include "constants.h"
#include<string.h>
#include<sys/un.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<time.h>
#include"common_functions.h"
int main()
{
	struct sockaddr_un servaddr;
	char climsg[1024];
	char timebuff[1024];
	char remote_ip[IP_LEN];
	int sockfd;
	int remote_port;;
	time_t curTime;
	char localhostname[100];
	unlink(timeserv_sunpath);
	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, timeserv_sunpath);
	sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	bind(sockfd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr_un));
	odr_register(sockfd);
	//mesg_send(sockfd, tgt_ip, TIME_SERV_PORT, msg, 0);
	memset(climsg, 0, 1024);
	gethostname(localhostname, 100);
	//mesg_recv(sockfd, remote_ip, &remote_port, msg);
	while(1)
	{
		bzero(timebuff, sizeof(timebuff));
		msg_recv(sockfd, remote_ip, &remote_port,climsg);
		printf(" server at node %s responding to request from %s\n",localhostname, gethostbyip(remote_ip));
		time(&curTime);
		strcpy(timebuff, ctime(&curTime));
		msg_send(sockfd, remote_ip, remote_port, timebuff, 0);
	}

}

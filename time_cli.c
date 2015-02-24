#include<stdio.h>
#include "odr_api.h"
#include<sys/socket.h>
#include<netdb.h>
#include "constants.h"
#include<string.h>
#include<sys/un.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<signal.h>
#include<setjmp.h>
#include<sys/time.h>
#define TIMEOUT 30
#include<errno.h>
void sig_alarm(int signo);
static sigjmp_buf jmpbuf;
int nremaintries;
int main()
{
	struct sockaddr_un cliaddr;
	char msg[1024];
	char tgt_host[25];
	char tgt_ip[IP_LEN];
	char remote_ip[IP_LEN];
	int sockfd;
	int remote_port;
	char clisunfile[108];
	struct itimerval timer;
	struct sigaction sa;
	nremaintries = 3;
	char localhostname[25];
	sa.sa_handler = sig_alarm;
	sigemptyset(&sa.sa_mask);
        sigaction(SIGALRM,&sa,NULL);

	memset(&timer, 0, sizeof(struct itimerval));
	timer.it_value.tv_sec = TIMEOUT;
	timer.it_value.tv_usec = 0;
	if(setitimer(ITIMER_REAL,&timer, NULL)<0)
	      printf(" setitimer error %s\n",strerror(errno));
	memset(&cliaddr, 0, sizeof(struct sockaddr_un));
	//unlink("/tmp/odrcli");
	memcpy(clisunfile, tmpnam(NULL),13);
	cliaddr.sun_family = AF_UNIX;
	strcpy(cliaddr.sun_path, clisunfile);
	//strcpy(cliaddr.sun_path, tmpnam(NULL));
	//memcpy(cliaddr.sun_path, tmpnam(NULL), 108);
	//printf(" client path =%s\n", cliaddr.sun_path);
	sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	bind(sockfd, (struct sockaddr*)&cliaddr, sizeof(struct sockaddr_un));
	printf(" Enter a valid hostname vm<1,2,3...10>\n");
	scanf("%s",tgt_host);
	struct hostent *host_info = gethostbyname(tgt_host);
	struct in_addr **addr_list = (struct in_addr **)host_info->h_addr_list;
	if(addr_list[0] == NULL)
	{
		printf(" Enter valid hostname, exiting\n");
		exit(-1);
	}
	//printf(" target host ip = %s",inet_ntoa(*addr_list[0]));
	strcpy(tgt_ip, inet_ntoa(*addr_list[0]));
	strcpy(msg,"what is the time");
	gethostname(localhostname, 25);
	printf("client at node %s sending request to server at %s \n", localhostname,tgt_host);
	//nremaintries--;
	msg_send(sockfd, tgt_ip, TIME_SERV_PORT, msg, 0);
	if(sigsetjmp(jmpbuf, 1) != 0)
	{
		nremaintries= nremaintries-1;
		if(nremaintries <=0)
		{	
			printf("cliet at node %s timeout on response from  %s\n", localhostname, tgt_host);
			unlink(clisunfile);
			exit(-1);
		}
		else
		{
			printf("retries left %d \n",nremaintries);
			timer.it_value.tv_sec = TIMEOUT;
        		timer.it_value.tv_usec = 0;
        		if(setitimer(ITIMER_REAL,&timer, NULL)<0)
              			printf(" setitimer error %s\n",strerror(errno));
			msg_send(sockfd, tgt_ip, TIME_SERV_PORT, msg, 1);
		}
	}
	memset(msg, 0, 1024);
	msg_recv(sockfd, remote_ip, &remote_port, msg);
	//printf(" time received =%s\n", msg);
	printf(" client at node %s: received from %s: %s\n", localhostname, tgt_host, msg);
	timer.it_value.tv_sec = 0;
        timer.it_value.tv_usec = 0;
        if(setitimer(ITIMER_REAL,&timer, NULL)<0)
	        printf(" setitimer error %s\n",strerror(errno));
	unlink(cliaddr.sun_path);
	//unlink(clisunfile);
}
void sig_alarm(int signo)
{
 	printf(" alarm triggered\n");
        siglongjmp(jmpbuf, 1);
}

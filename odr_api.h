#ifndef __ODRAPI_INCLUDED__
#define __ODRAPI_INCLUDED__ 1
/* structure to interpret unix domain communication between time services and ODR*/
#define TIME_SERV_PORT 5800
static char *timeserv_sunpath="/tmp/timeserv";
int msg_send(int sockfd, char *dest_ip, int dest_port,char *buff, int route_disc);
int msg_recv(int sockfd, char *remote_ip,int *dest_port, char *buff);
void odr_register(int sockfd);
char *odr_gethostname(void);
#endif


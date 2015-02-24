#ifndef __ODRPROTO_INCLUDED__
#define __ODRPROTO_INCLUDED__ 1
#include "odr.h"
#include "hw_addrs.h"
#include "constants.h"
#define MAX_INF 10
#include"routing_table.h"
#include<unistd.h>
#define ODR_PROTO 0x8890
static int broadcast_id;
static char *odr_sun_path="/tmp/odr";
//char *timeserv_sunpath="/tmp/timeserv";
struct route_info *routel_head;
struct sfileportmap *port_thead;
int unixdsockfd;
extern int purge_timeout;
char canonical_ip[IP_LEN];
struct infbind_info{
	int sd;
	struct hwa_info hw_info;
};
struct app_data{
	char remote_ip[IP_LEN];
	int remote_port;
	char userdata[1024];
	int route_disc;
	int app_register;
};
struct infbind_info inf_info[MAX_INF];
int total_bind_ifs;
void *receiver_thread(void *);
void *userdata_thread(void *);
void send_rrep(int sockfd, char *dest_ip, char *about_ip, time_t );
void send_rreq(char *, char *, int ,int , time_t, int );
void forward_packet(void *);
int belong_toself(char *);
#endif

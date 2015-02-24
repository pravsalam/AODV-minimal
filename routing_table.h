/* interface to talk to routing tables */
#ifndef __ROUTINGAPI_INCLUDED__
#define __ROUTINGAPI_INCLUDED__
#include "constants.h"
#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<string.h>
struct route_info{
	char target_ip[IP_LEN];
	char nexthop_hwaddr[ETH_ALEN];
	int bcast_id;
	int inf_index;
	int hop_count;
	time_t time_stamp;
	struct route_info *route_next;
};
struct route_info* route_insert(struct route_info *, char *, char *, int, int, int);
struct route_info* route_clean(struct route_info *);
struct route_info* route_search(struct route_info *, char *);
struct route_info* route_exact_search(struct route_info *, char *, int);
void route_print(struct route_info *);

#endif 

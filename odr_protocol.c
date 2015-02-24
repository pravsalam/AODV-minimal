/* odr protocol will have interfaces for others, many global static variables
   shared among multiple functions, and a thread keeps running and based on mesage received it calls other functions */
#include<stdio.h>
#include<string.h>
#include "odr_protocol.h"
#include<pthread.h>
#include<errno.h>
#include<sys/socket.h>
//#include<netpacket/packet.h>
#include<net/ethernet.h>
#include<sys/select.h>
#include<linux/if_arp.h>
#include<arpa/inet.h>
#include "unp.h"
#include "odr_api.h"
#include "port_table.h"
#include<sys/un.h>
#include<sys/time.h>
#include "common_functions.h"
int purge_timeout;
struct thread_data{
	char src_ip[IP_LEN];
	int src_port;
	char dest_ip[IP_LEN];
	int dest_port;
	char userdata[1024];
	int route_disc;
};
int find_max(struct infbind_info [], int );
time_t  get_current_time(void);
void print_mac(char *);
void forward_data(char *src_ip, char *dest_ip, int src_port, int dest_port, int bcast_id, int hop_count, time_t time_stamp, char *msg, int discover);
static pthread_mutex_t mutx;
static pthread_mutex_t ptmutx;
void* process_userdata(void*);
void* process_serverdata(void *);
void print_packhdr(char *, char *, char *, int);
int main(int argc, char **argv)
{
	if(argc != 2)
	{
		printf(" correct usage odr_palam staleness\n");
		return;
	}
	purge_timeout = atoi(argv[1]);
	int i;
	int sockd;
	struct hwa_info *infi;
	struct hwa_info *infhead;
	unsigned char src_mac[ETH_ALEN];
	unsigned char dest_mac[ETH_ALEN];
	struct sockaddr_ll sll_addr;
	infhead = get_hw_addrs();
	for(i=0;i<MAX_INF;i++)
	{
		// filling with invalid index number
		inf_info[i].sd = -1;
	}
	routel_head = NULL;
	port_thead = NULL;
	/*iterate over each interface and bind */
	for(infi = infhead,i=0; infi!= NULL;infi = infi->hwa_next)
	{
		//printf("before if name =%s alias =%d\n ", infi->if_name, infi->ip_alias);
		// we will not bind to eth0 and lo and aliases
		if((strcmp(infi->if_name, "eth0")) && (strcmp(infi->if_name, "lo")) && !infi->ip_alias)
		{
			//printf(" what has crossed this %s ", infi->if_name);
			//print_mac(infi->if_haddr);
			if((sockd = socket(PF_PACKET, SOCK_RAW, htons(ODR_PROTO))) < 0)
			{
				printf("root previlage are required to create sock_raw socket\n");
				return -1;
			}
			//printf(" what is sockd = %d\n", sockd);
			inf_info[i].sd = sockd;
			inf_info[i].hw_info = *infi;
			/* bind the socket descriptor to an interface */
			memset(&sll_addr, 0, sizeof(struct sockaddr_ll));
			memset(&src_mac, 0, ETH_ALEN);
			memset(&dest_mac, 0, ETH_ALEN);
			sll_addr.sll_family = PF_PACKET;
			sll_addr.sll_ifindex = infi->if_index;
			//printf("sockd =%d\n", sockd);
			if(bind(sockd, (struct sockaddr *)&sll_addr, sizeof(struct sockaddr_ll)) < 0)
			{
				printf("sockd =%d unable to bind raw socket to interface %d error:%s\n",sockd, infi->if_index,strerror(errno));
			}	
			i++;
		}
		else if( strcmp(infi->if_name, "eth0") == 0)
		{
			/* get canonical IP address */
			strcpy(canonical_ip, Sock_ntop_host(infi->ip_addr, sizeof(struct sockaddr)));
			printf("canonical IP of the node = %s\n", canonical_ip);
		}
	}
	total_bind_ifs = i;
	broadcast_id = 0;
	/* initialize unix domain socket communication */
	struct sockaddr_un servaddr;
	memset(&servaddr, 0 , sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path,odr_sun_path);
	unixdsockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	unlink(servaddr.sun_path);
	if(bind(unixdsockfd, (struct sockaddr*) &servaddr, sizeof(struct sockaddr_un)) < 0)
		printf("bind failed\n");
	
	pthread_t recvrt_id;
	pthread_t userdatat_id;
	pthread_mutex_init(&mutx, NULL);
	pthread_mutex_init(&ptmutx, NULL);
	pthread_create(&recvrt_id, NULL, &receiver_thread,NULL);
	pthread_create(&userdatat_id, NULL, &userdata_thread, NULL); 
	pthread_join(recvrt_id, NULL);
	pthread_join(userdatat_id, NULL);
	pthread_mutex_destroy(&ptmutx);
	pthread_mutex_destroy(&mutx);
	return 0;
}

void *receiver_thread(void *arg)
{
	printf(" receiver thread entered\n");
	/* this thread reads packets on a loop and take actions based on msgType*/

	int i;
	int listenfd;
	fd_set descset;
	int maxfd = find_max(inf_info, total_bind_ifs);
	FD_ZERO(&descset);
	/* we will start monitoring each interface for incoming traffic */
	for(i =0; i<total_bind_ifs;i++)
		FD_SET(inf_info[i].sd, &descset);
	while(1)
	{
		for(i =0; i<total_bind_ifs;i++)
                	FD_SET(inf_info[i].sd, &descset);
		//printf(" waiting for connection\n");
		if(select(maxfd+1, &descset, NULL, NULL, NULL) == -1)
		{
			if(errno == EINTR)
				continue;
			else
				exit(-1);
		}
		//printf(" got out of select\n");
		/* check on each interface for packet */
		for(i = 0;i < total_bind_ifs; i ++)
		{
			if(FD_ISSET(inf_info[i].sd , &descset))
			{
				/* received a packet on interface */ 
				listenfd = inf_info[i].sd;
				//allocate buffer here and free at the end of loop
				pthread_t servert_id;
				pthread_create(&servert_id, NULL, &process_serverdata, (void *)&listenfd);
			}
		}
	}
	return NULL;
}
void send_rrep(int sockfd, char *dest_ip, char *about_ip, time_t timestamp)
{
	/* if about_ip is self, hop_count is not increased if intermediate node is replying, hop count to dest_ip and about_ip are added*/
	printf(" entering send_rrep \n");
	int i;
	int temp;
	void *buffer;
	char dest_mac[ETH_ALEN];
	buffer = malloc(ODR_PACK_LEN);
	struct sockaddr_ll sock_addr;
	memset(buffer, 0, ODR_PACK_LEN);
	route_print(routel_head);
	struct route_info *route_todest = route_search(routel_head, dest_ip);
	struct ethhdr *eth_hdr = (struct ethhdr *)buffer;
	/* build ethernet header */
	for(i =0;i < total_bind_ifs; i++)
	{
		if(inf_info[i].hw_info.if_index == route_todest->inf_index)
		{
			memcpy(eth_hdr->h_source, inf_info[i].hw_info.if_haddr, ETH_ALEN);
			temp = inf_info[i].sd;
		}
	}
	//printf(" arg sock =%d temp =%d\n",sockfd, temp);
	//printf(" send rrep source mac =");
	//print_mac(eth_hdr->h_source);

	memcpy(eth_hdr->h_dest, route_todest->nexthop_hwaddr,ETH_ALEN);
	//memcpy(eth_hdr->h_source, inf_info[i].hw_info.if_haddr,ETH_ALEN);
	eth_hdr->h_proto = htons(ODR_PROTO);
	
	/* build odr header */
	struct odrhdr *odr_hdr = (struct odrhdr *)(buffer+ETH_HDR_LEN);
	odr_hdr->bcast_id = route_todest->bcast_id;
	odr_hdr->msg_type = RREP;
	strncpy(odr_hdr->src_ip, about_ip, IP_LEN);
	strncpy(odr_hdr->dest_ip, dest_ip, IP_LEN);
	odr_hdr->time_stamp = timestamp;
	sock_addr.sll_family = PF_PACKET;
	//sock_addr.sll_hatype = ARPHRD_ETHER;
	//sock_addr.sll_pkttype = PACKET_OTHERHOST;
	sock_addr.sll_halen = ETH_ALEN;
	memcpy(&sock_addr.sll_addr, route_todest->nexthop_hwaddr, ETH_ALEN);
	char broadcast_mac[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	//memcpy(&sock_addr.sll_addr, broadcast_mac, ETH_ALEN);
	//memcpy(eth_hdr->h_dest, broadcast_mac, ETH_ALEN);
	/*int k;
	for(k=0;k<ETH_ALEN;k++)
		printf(" %x:", route_todest->nexthop_hwaddr[k]);
	printf("\n");*/
	sock_addr.sll_addr[6] = 0x00;
	sock_addr.sll_addr[7] = 0x00;
	for(i= 0;i<total_bind_ifs;i++)
	{
		/* find the interface the packet came in*/
		if (inf_info[i].sd == sockfd)
		{
			sock_addr.sll_ifindex = inf_info[i].hw_info.if_index;
			printf(" sending rrep on interface =%d\n", sock_addr.sll_ifindex);
			/* is about_ip same as own*/
			if(belong_toself(about_ip))
			{
				/* own ip address */
				printf(" rreq was for self\n");
				odr_hdr->hop_count = route_todest->hop_count;

			}
			else
			{
				/*intermediate node is replying about destination*/
				/* retrieve details of the destination and reply to the source*/
				printf(" rreq was for somebody else\n");
				struct route_info *route_somebody = route_search(routel_head, about_ip);
				if(route_somebody != NULL)
				{
					odr_hdr->hop_count = 1 + route_somebody->hop_count;
				}
			}
		}
	}
	print_packhdr(about_ip, dest_ip, route_todest->nexthop_hwaddr, 1);		
	if(sendto(sockfd, buffer, ODR_PACK_LEN, 0, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr_ll)) < 0)
	{
		printf("unable to reply with RREP packet\n");
	}
	printf(" leaving send rrep \n");
}
void send_rreq(char *src_ip, char *dest_ip, int hop_count, int bcast_id, time_t time_stamp,int except_inf)
{
	printf(" entered send rreq src %s dest %s hop %d bcast %d time %d einf %d\n", src_ip, dest_ip, hop_count, bcast_id, time_stamp, except_inf);
	int i;
	struct sockaddr_ll sock_addr;
	void* buffer; 
	buffer = malloc(ODR_PACK_LEN);
	memset(buffer,0, ODR_PACK_LEN);
	struct ethhdr *eth_hdr = (struct ethhdr *)buffer;
	struct odrhdr *odr_hdr = (struct odrhdr *)(buffer + ETH_HDR_LEN);
	char dest_mac[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	/* fill eth hdr except src_mac */
	memcpy(eth_hdr->h_dest, dest_mac, ETH_ALEN);
	eth_hdr->h_proto = htons(ODR_PROTO);
	
	/* fill odr headr */
	odr_hdr->msg_type = RREQ;
	memcpy(odr_hdr->src_ip, src_ip, IP_LEN);
	memcpy(odr_hdr->dest_ip, dest_ip, IP_LEN);
	odr_hdr->bcast_id = bcast_id;
	odr_hdr->hop_count = hop_count;
	odr_hdr->time_stamp= time_stamp;
	
	/* fill sockaddr_ll structure */
	sock_addr.sll_family = PF_PACKET;
	//sock_addr.sll_hatype = ARPHRD_ETHER;
	//sock_addr.sll_pkttype = PACKET_OTHERHOST;
	sock_addr.sll_halen = ETH_ALEN;
	memcpy(&sock_addr.sll_addr, dest_mac, ETH_ALEN);
	sock_addr.sll_addr[6] = 0x00;
	sock_addr.sll_addr[7] = 0x00;
	for(i = 0; i<total_bind_ifs; i++)
	{
		/* we will not send RREQ on inf on which an RREQ was received. if except_inf is -1, we will broadcast*/
		if( inf_info[i].hw_info.if_index != except_inf)
		{
			memcpy(eth_hdr->h_source, inf_info[i].hw_info.if_haddr,ETH_ALEN);
			sock_addr.sll_ifindex = inf_info[i].hw_info.if_index;
			//sendto(inf_info[i].sd, buffer, ODR_PACK_LEN, 0, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr_ll));
			/* send the packet on interface */
			print_packhdr(src_ip, dest_ip, dest_mac, 0);
			if(sendto(inf_info[i].sd, buffer, ODR_PACK_LEN, 0, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr_ll)) < 0)
			{
				printf("unable to broadcast RREQ packets\n");
			}
		}
	}
	printf(" leaving rreq\n");
}
int find_max( struct infbind_info infinfo[], int n)
{
	int i;
	int maxfd = 0;
	printf("number of interfaces it seems = %d\n", n);
	for(i = 0;i< n;i++)
	{
		if (infinfo[i].sd > maxfd)
			maxfd = infinfo[i].sd;
		
	}
	printf("max fd = %d\n", maxfd);
	return maxfd;
}
int belong_toself(char *ip)
{
	int i;
	if(strcmp(canonical_ip, ip) ==0)
		return 1;
	return 0;
}
void forward_packet(void *buffer)
{
	printf("entered forward\n");
	struct ethhdr *eth_hdr = (struct ethhdr *)buffer;
	int i;
	struct odrhdr *odr_hdr = (struct odrhdr*)(buffer+ ETH_HDR_LEN);
	struct sockaddr_ll sock_addr;
	memset(&sock_addr, 0, sizeof(struct sockaddr_ll));
	struct route_info *route_todest = route_exact_search(routel_head, odr_hdr->dest_ip, odr_hdr->bcast_id);
	if(route_todest != NULL)
	{
		/* forward the packet */
		for(i=0;i<total_bind_ifs;i++)
		{
			if(inf_info[i].hw_info.if_index == route_todest->inf_index)
			{
				printf(" forwarding on interface %d\n", inf_info[i].hw_info.if_index);
				memcpy(eth_hdr->h_source, inf_info[i].hw_info.if_haddr,ETH_ALEN);
				memcpy(eth_hdr->h_dest, route_todest->nexthop_hwaddr, ETH_ALEN);
				/*printf(" source =");
				print_mac(eth_hdr->h_source);
				printf(" dest =");
				print_mac(eth_hdr->h_dest);*/
				eth_hdr->h_proto= htons(ODR_PROTO);
				sock_addr.sll_family = PF_PACKET;
				//sock_addr.sll_hatype = ARPHRD_ETHER;
				//sock_addr.sll_pkttype = PACKET_OTHERHOST;
				sock_addr.sll_halen = ETH_ALEN;
				memcpy(&sock_addr.sll_addr, route_todest->nexthop_hwaddr, ETH_ALEN);
				/*printf("addr copied to sll =");
				print_mac(sock_addr.sll_addr);*/
				sock_addr.sll_addr[6] = 0x00;
				sock_addr.sll_addr[7] = 0x00;
				sock_addr.sll_ifindex = inf_info[i].hw_info.if_index;
				int pack_size = sizeof(buffer);
				printf(" in forward odr type  = %d sd = %d  \n", odr_hdr->msg_type, inf_info[i].sd);
				fwrite(buffer, ODR_PACK_LEN, 1, stdout);
				if(sendto(inf_info[i].sd, buffer,pack_size, 0, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr_ll)) <0)
					printf(" unable to forward packet\n");
			}
		}
	}
	else
		printf(" no route was found\n");
 
	printf(" leaving forward\n");
}
void *userdata_thread(void *args)
{
	int i;
	struct  app_data appdata;
	struct sockaddr_un cliaddr;
	memset(&cliaddr, 0 , sizeof(struct sockaddr_un));
	memset(&appdata, 0, sizeof(struct app_data));
	socklen_t unixsocklen = sizeof(struct sockaddr_un);
	struct sfileportmap *ptable_entry;
	int source_port = 0;
	void *buffer = malloc(ODR_PACK_LEN);
	struct sockaddr_ll sock_addr;
	int sendfd;
	while(1)
	{
		//printf(" checking for incoming app data unixd = %d\n", unixdsockfd);
		memset(&appdata, 0, sizeof(appdata));
		memset(&cliaddr, 0, sizeof(cliaddr));
		int recvd = recvfrom(unixdsockfd, &appdata, sizeof(appdata), 0, (struct sockaddr*)&cliaddr,&unixsocklen);
		//printf(" recvd = %d\n", recvd);
		if(recvd > 0)
		{
			printf(" received data from application\n");
			if((ptable_entry = port_table_search_sfile(port_thead,cliaddr.sun_path)) != NULL)
			{
				/* there is an entry for the application */
				source_port = ptable_entry->port;
			}
			else
			{
				//printf(" no entry was found\n");
				/*  keep the information of the application in port table*/
				if( strcmp(cliaddr.sun_path,timeserv_sunpath) ==0)
				{
					/*it is the server, port is predefined  as TIME_SERV_PORT*/
					//printf(" it is server port \n");
					source_port = TIME_SERV_PORT;
					pthread_mutex_lock(&ptmutx);
					port_thead = port_table_insert(port_thead, cliaddr.sun_path, TIME_SERV_PORT);
					pthread_mutex_unlock(&ptmutx);
				}
				else
				{
					/* some client */
					//printf(" it is client\n");
					int randval = rand();
					//printf(" rand value = %d\n", randval);
					source_port = randval%PORT_MAX;
					pthread_mutex_lock(&ptmutx);
					//printf(" client port = %d path = %s\n", source_port, cliaddr.sun_path);
					port_thead = port_table_insert(port_thead, cliaddr.sun_path, source_port);
					pthread_mutex_unlock(&ptmutx);
					//printf(" done client insertion\n");
				}
			}
			//port_table_print(port_thead);
			if(appdata.app_register != 1)
			{
				struct thread_data tdata;
				strcpy(tdata.src_ip, canonical_ip);
				tdata.src_port = source_port;
				strcpy(tdata.dest_ip, appdata.remote_ip);
				tdata.dest_port = appdata.remote_port;
				tdata.route_disc = appdata.route_disc;
				memcpy(tdata.userdata, appdata.userdata,1024);
				pthread_t thread_id;
				
				if(belong_toself(appdata.remote_ip))
				{
					/* Deliver it to user  over unix socket*/
					printf(" data came for self\n");
					//port_table_print(port_thead);
					struct sfileportmap *app_info = port_table_search_port(port_thead, appdata.remote_port);
					struct sockaddr_un appcliaddr;
					struct app_data newappdata;
					memset(&appcliaddr, 0, sizeof(struct sockaddr_un));
					if(app_info != NULL)
					{
						printf(" found route to application\n");
						appcliaddr.sun_family = AF_UNIX;
						//printf(" application at %s\n", app_info->sfile);
						strcpy(appcliaddr.sun_path, app_info->sfile);
						strcpy(newappdata.remote_ip, canonical_ip);
						newappdata.remote_port = source_port;
						strcpy(newappdata.userdata, appdata.userdata);
						socklen_t unixsocklen = sizeof(struct sockaddr_un);
						if(sendto(unixdsockfd, &newappdata, sizeof(struct app_data), 0, (struct sockaddr*)&appcliaddr, unixsocklen) <0)
						{
							printf("ODR couldn't deliver to application %s\n",strerror(errno));
						}
					}
				}
				else
				{
					pthread_create(&thread_id, 0,&process_userdata, (void *)&tdata); 
				}
			}

		}
	}

}
time_t get_current_time(void)
{
	struct timeval ts;
	gettimeofday(&ts, NULL);
	return ts.tv_sec;
}
void print_mac(char *mac)
{
	int i;
	//printf(" new print mac\n");
	for(i =0;i<ETH_ALEN; i++)
		printf("%.2x%s",((unsigned char )mac[i]& 0xff),(i < ETH_ALEN -1)?":" : " ");
	printf("\n");
}
void forward_data(char *src_ip, char *dest_ip, int src_port, int dest_port, int bcast_id, int hop_count, time_t time_stamp, char *msg, int discover)
{
	//printf(" entered forward data\n");
	void *buffer;
	int i;
	int sendfd;
	buffer = malloc(ODR_PACK_LEN);
	struct ethhdr *eth_hdr  = (struct ethhdr*)buffer;
	struct odrhdr *odr_hdr = (struct odrhdr *)(buffer + ETH_HDR_LEN);
	char *userdata = (char *) (buffer + ETH_HDR_LEN+ ODR_HDR_LEN);
	//printf(" dest ip =%s bcast = %d\n", dest_ip, bcast_id);
	struct route_info* routeto_dest = route_search(routel_head, dest_ip);
	if(routeto_dest == NULL || discover == 1)
	{
		printf(" no route to destination\n");
		time_t time_stamp = get_current_time();
		if(bcast_id == -1)
		{
			/* initiate route for self*/
			broadcast_id++;
			bcast_id = broadcast_id;
		}
		send_rreq(src_ip, dest_ip, hop_count, bcast_id, time_stamp,-1);
	}
	while((routeto_dest = route_search(routel_head, dest_ip)) == NULL);
	struct sockaddr_ll sock_addr;
	memset(&sock_addr, 0, sizeof(struct sockaddr_ll));
	for(i = 0;i <total_bind_ifs;i++)
	{
		if(inf_info[i].hw_info.if_index == routeto_dest->inf_index)
                {
                        memcpy(eth_hdr->h_source, inf_info[i].hw_info.if_haddr, ETH_ALEN);
			memcpy(eth_hdr->h_dest, routeto_dest->nexthop_hwaddr, ETH_ALEN);
                        sendfd = inf_info[i].sd;
                        sock_addr.sll_ifindex = inf_info[i].hw_info.if_index;
                }
	}
	eth_hdr->h_proto = htons(ODR_PROTO);
	odr_hdr->msg_type = ODR_DATA;
	strcpy(odr_hdr->src_ip, src_ip);
	strcpy(odr_hdr->dest_ip, dest_ip);
	odr_hdr->hop_count = hop_count;
	odr_hdr->src_port = src_port;
	odr_hdr->dest_port = dest_port;
	odr_hdr->time_stamp = time_stamp;
	odr_hdr->bcast_id = bcast_id;
	memcpy(userdata, msg, (ODR_PACK_LEN - ETH_HDR_LEN - ODR_HDR_LEN));
	sock_addr.sll_family = PF_PACKET;
	sock_addr.sll_halen = ETH_ALEN;

	memcpy(&sock_addr.sll_addr,routeto_dest->nexthop_hwaddr, ETH_ALEN );
        sock_addr.sll_addr[6] = 0x00;
        sock_addr.sll_addr[7] = 0x00;
        //fwrite(buffer, ODR_PACK_LEN, 1, stdout);
	print_packhdr(src_ip, dest_ip, routeto_dest->nexthop_hwaddr,2);
        if(sendto(sendfd,buffer, ODR_PACK_LEN, 0, (struct sockaddr *)&sock_addr, sizeof(struct sockaddr_ll)) < 0)
        {
                printf(" sendto failed in odr\n");
        }
	//printf(" leaving forward data\n");
}
void *process_userdata(void *args)
{
	/* unbox and call forward_data*/
	struct thread_data *tdata = (struct thread_data *)args;
	forward_data(tdata->src_ip, tdata->dest_ip, tdata->src_port, tdata->dest_port, -1,0, get_current_time(), tdata->userdata,tdata->route_disc);
}

void *process_serverdata(void *args)
{
	int i,j;
	struct sockaddr_ll sll_addr;
	socklen_t sockll_len = sizeof(struct sockaddr_ll);
	void* buffer; 
	unsigned char src_mac[ETH_ALEN];
	unsigned char dest_mac[ETH_ALEN];
	int rcvd_inf;
	int listenfd = *((int *)args);
	buffer = malloc(ODR_PACK_LEN);
	memset(&sll_addr, 0, sizeof(struct sockaddr_ll));
	int recvd = recvfrom(listenfd, buffer, ODR_PACK_LEN, 0,(struct sockaddr*)&sll_addr,&sockll_len); 
	if(recvd > 0)
	{
		/* process the packet to find type*/
		struct ethhdr *eth_hdr = (struct ethhdr*)buffer;
		memcpy(src_mac,eth_hdr->h_source, ETH_ALEN);
		for(j=0;j<ETH_ALEN;j++)
			printf("%x:", src_mac[j]);
		printf("\n");
		printf("interface =%d\n",sll_addr.sll_ifindex );
		rcvd_inf  = sll_addr.sll_ifindex;
		struct odrhdr *odr_hdr = (buffer+ETH_HDR_LEN); 
		char *data = (buffer + ETH_HDR_LEN + ODR_HDR_LEN);
		/* check the message type and take necessary action */
		printf("packet type =%d\n", odr_hdr->msg_type);
		if(!belong_toself(odr_hdr->src_ip))
		{
			if( odr_hdr->msg_type == RREQ)
			{
				printf(" ODR recieved RREQ \n");
				/*learn the source_ip and add to routing entry*/
				pthread_mutex_lock(&mutx);
				routel_head = route_insert(routel_head, odr_hdr->src_ip,src_mac,odr_hdr->bcast_id, rcvd_inf, odr_hdr->hop_count);
				pthread_mutex_unlock(&mutx);
				route_print(routel_head);
				if(belong_toself(odr_hdr->dest_ip))
				{
					printf(" got rreq for self\n");
					printf(" fd =%d src_ip =%s dest =%s \n", listenfd, odr_hdr->src_ip, odr_hdr->dest_ip);
					send_rrep(listenfd, odr_hdr->src_ip, odr_hdr->dest_ip, odr_hdr->time_stamp);
				}
				else
				{
					/* search if node has route to dest_ip*/
					struct route_info *route_dest = route_search(routel_head, odr_hdr->dest_ip);
					if(route_dest != NULL)
					{
						/*current node has route to destination ip, make a new entry into routing table with new broadcast id*/
						route_dest->bcast_id = odr_hdr->bcast_id; 
						pthread_mutex_lock(&mutx);
						routel_head =route_insert(routel_head,
										route_dest->target_ip,
										route_dest->nexthop_hwaddr,
										route_dest->bcast_id, 
										route_dest->inf_index, 
										route_dest->hop_count);
						pthread_mutex_unlock(&mutx);
						/*send RREP with the route information */
						send_rrep(listenfd, odr_hdr->src_ip,odr_hdr->dest_ip,odr_hdr->time_stamp);
					}
					else
					{
						/* there is no route to dest_ip broadcast RREQ on other interfaces*/
						send_rreq(odr_hdr->src_ip, 
										odr_hdr->dest_ip,
										odr_hdr->hop_count+1, 
										odr_hdr->bcast_id, 
										odr_hdr->time_stamp, 
										rcvd_inf);
					}
				}
			}
			else if( odr_hdr->msg_type == RREP)
			{
				printf(" ODR recieved RREP\n");
				/*check the RREP is for self*/ 
				printf(" got rrep for ip %s\n",odr_hdr->dest_ip );
				if(belong_toself(odr_hdr->dest_ip))
				{
					/*RREP for self, add route to destination*/
					printf("received RREP for self\n");
					//print_mac(eth_hdr->h_source);
					pthread_mutex_lock(&mutx);
					routel_head = route_insert(routel_head, 
													odr_hdr->src_ip,
													eth_hdr->h_source, 
													odr_hdr->bcast_id,
													rcvd_inf,
													odr_hdr->hop_count);
					pthread_mutex_unlock(&mutx);					
					route_print(routel_head);
				}
				else
				{
					/* RREP is for somebody else, send RREP back to the source that initiated */
					struct route_info *route_original_req  = route_exact_search(routel_head, odr_hdr->dest_ip, odr_hdr->bcast_id);
					if(route_original_req != NULL)
					{
						/* found entry to node that initiated the request, send RREP*/
						/* find the sockid through which to send*/ 
						int k;
						int send_socket;
						for(k=0;k<total_bind_ifs;k++)
						{
							if(inf_info[k].hw_info.if_index == route_original_req->inf_index)
								send_socket = inf_info[k].sd;
						}
						pthread_mutex_lock(&mutx);
						routel_head = route_insert(routel_head,
														odr_hdr->src_ip,
														src_mac,
														odr_hdr->bcast_id,
														rcvd_inf, 
														odr_hdr->hop_count);
						pthread_mutex_unlock(&mutx);
						send_rrep(send_socket, odr_hdr->dest_ip, odr_hdr->src_ip, odr_hdr->time_stamp);
					}
				}
				
			}
			else if (odr_hdr->msg_type == ODR_DATA)
			{
				printf(" received data\n");
				/*check message is for self*/
				if(belong_toself(odr_hdr->dest_ip))
				{	
					/* Deliver it to user  over unix socket*/
					printf(" data came for self\n");
					//port_table_print(port_thead);
					struct sfileportmap *app_info = port_table_search_port(port_thead, odr_hdr->dest_port);
					struct sockaddr_un appcliaddr;
					struct app_data appdata;
					memset(&appcliaddr, 0, sizeof(struct sockaddr_un));
					if(app_info != NULL)
					{
						printf(" found route to application\n");
						appcliaddr.sun_family = AF_UNIX;
						//printf(" application at %s\n", app_info->sfile);
						strcpy(appcliaddr.sun_path, app_info->sfile);
						strcpy(appdata.remote_ip, odr_hdr->src_ip);
						appdata.remote_port = odr_hdr->src_port;
						strcpy(appdata.userdata, data);
						socklen_t unixsocklen = sizeof(struct sockaddr_un);
						sendto(unixdsockfd, &appdata, sizeof(struct app_data), 0, (struct sockaddr*)&appcliaddr, unixsocklen);
					}
				}
				else
				{
					/* packet belongs to somebody else forward*/
					//forward_packet(buffer);
					char *msg = (char *)(buffer +ETH_HDR_LEN+ODR_HDR_LEN);
					forward_data(odr_hdr->src_ip,odr_hdr->dest_ip, odr_hdr->src_port, odr_hdr->dest_port, odr_hdr->bcast_id, odr_hdr->hop_count, odr_hdr->time_stamp, msg,0);
				}
			}
		}	
	}
	free(buffer);

}
void print_packhdr(char *src_ip, char *dest_ip, char *dest_mac, int odr_type)
{
	char localhostname[50];
	char srcNode[50];
	char destNode[50];
	gethostname(localhostname, 50);
	printf("ODR sending frame src %s dest mac ",localhostname);
	print_mac(dest_mac);
	strcpy(srcNode, gethostbyip(src_ip));
	strcpy(destNode, gethostbyip(dest_ip));
	printf("ODR msg type %d src %s dest %s\n",odr_type, srcNode,destNode);
}

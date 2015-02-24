#include "routing_table.h"
#include<sys/time.h>
#include "odr_protocol.h"
time_t get_cur_time(void);
struct route_info* route_insert(struct route_info *head, char *targetip, char *targetmac, int bcastid, int infindex, int hopcount)
{
	//printf(" entering route_insert %d \n",purge_timeout);
	head = route_clean(head);
	struct route_info *entry = NULL;
	struct route_info *pred =NULL;
	struct timeval ts;
	gettimeofday(&ts,NULL);
	if(head !=NULL)
		entry = head;
	else
		head = entry;
	while(entry !=NULL)
	{
		if((!strcmp(entry->target_ip, targetip)) && (entry->bcast_id == bcastid))
		{
			/* there is an entry already present, update time_stamp*/
			entry->time_stamp = ts.tv_sec;
			if(hopcount<entry->hop_count)
			{
				/* we have a shorter route, update target mac*/
				//memset(entry->nexthop_hwaddr,0, ETH_ALEN);
				memcpy(entry->nexthop_hwaddr, targetmac, ETH_ALEN);
				entry->inf_index = infindex;
			}
			return head;

		}
		pred = entry;
		entry = entry->route_next;
	}
	/* no match was found, reached end of link list */
	entry = (struct route_info *)malloc(sizeof(struct route_info));
	strncpy(entry->target_ip, targetip, IP_LEN);
	memcpy(entry->nexthop_hwaddr, targetmac, ETH_ALEN);
	entry->bcast_id = bcastid;
	entry->inf_index = infindex;
	entry->hop_count = hopcount;
	entry->time_stamp = ts.tv_sec;
	if(pred !=NULL)
		pred->route_next = entry;
	if(head == NULL)
		head = entry;
	entry->route_next = NULL;
	//printf(" leaving route_insert\n");
	return head;
}
struct route_info* route_clean(struct route_info *head)
{
	/* look for the old entries which are older than PURGE_TIMEOUT and sort the linklist*/
	//printf(" entering route clean\n");
	struct route_info *entry = head;
	struct route_info *pred = entry;
	while( entry != NULL)
	{
		struct timeval ts;
		gettimeofday(&ts,NULL);
		if(entry->time_stamp - ts.tv_sec > purge_timeout)
		{
			/* purge the entry*/
			if(entry == head)
			{
				head = entry->route_next;
				free(entry);
				entry = head;
			}
			else
			{
				pred->route_next = entry->route_next;
				struct route_info *temp = entry->route_next;
				free(entry);
				entry = temp->route_next;
			}
		}
		else
		{	
			pred = entry;
			entry = entry->route_next;
		}
	}
	//printf(" leaving route clean\n");
	return head;
}
struct route_info* route_search(struct route_info *head,char *ipstr)
{
	struct route_info *entry = head;
	while(entry != NULL)
	{
		if(strcmp(entry->target_ip, ipstr) == 0)
		{
			/* a match is found*/
			entry->time_stamp = get_cur_time();
			return entry;
		}
		else
			entry = entry->route_next;
	}
	return NULL;
}
struct route_info* route_exact_search(struct route_info *head, char *ipstr, int bcast_id)
{
	struct route_info *entry =head;
	while(entry != NULL)
	{ 
		if((strcmp(entry->target_ip, ipstr) ==0) && (entry->bcast_id == bcast_id))
		{
			entry->time_stamp = get_cur_time();
			return entry;
		}
		else
		{
			entry = entry->route_next; 
		}
	}
	return NULL;
}
void route_print(struct route_info *head)
{
	struct route_info *entry = head;
	printf("\n\n");
	printf("================== ROUTING TABLE ===================\n");
	while(entry != NULL)
	{
		int j=0;
		printf(" target ip = %s bcstid = %d hw =", entry->target_ip, entry->bcast_id);
		for(j=0;j<ETH_ALEN; j++)
			printf("%.2x%s", ((unsigned char)entry->nexthop_hwaddr[j]&0xff),(j< ETH_ALEN-1) ? ":" : " ");
		printf(" \n");
		entry= entry->route_next;
	}
	printf("================ END OF TABLE ======================\n");
}
time_t get_cur_time(void)
{
	struct timeval ts;
	gettimeofday(&ts, NULL);
	return ts.tv_sec;
}

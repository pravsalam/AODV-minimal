#include "port_table.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
struct sfileportmap* port_table_insert(struct sfileportmap* head,  char* sfile, int port)
{
	//printf(" Entering port table insert\n");
	head = port_table_clean(head);
	struct sfileportmap *port_entry = NULL; 
	struct sfileportmap *pred = NULL;
	port_entry= (struct sfileportmap *)head;
	pred = port_entry;
	while(port_entry != NULL)
	{	
		if(port_entry->port == port)
		{
			printf(" port already in use\n");
			return NULL;
		}
		pred = port_entry;
		port_entry = port_entry->next;
	}
	port_entry = (struct sfileportmap*)malloc(sizeof(struct sfileportmap));
	strcpy(port_entry->sfile, sfile);
	port_entry->port = port;
	port_entry->time_stamp = current_time();
	if( pred != NULL)
		pred->next = port_entry;
	if(head == NULL)
		head = port_entry;
	port_entry->next = NULL;
	//printf(" Leaving port table insert\n");
	return head;
}
struct sfileportmap* port_table_clean(struct sfileportmap* head)
{
	//printf(" Entering port table clean\n");
	struct sfileportmap *port_entry;
	port_entry = head;
	struct sfileportmap *pred = port_entry;
	while(port_entry != NULL)
	{
		int cur_time = current_time();
		if(((cur_time - port_entry->time_stamp) > PORT_ENTRY_TIMEOUT) || (access(port_entry->sfile, F_OK) < 0))
		{
			if(head == port_entry)
			{
				struct sfileportmap *temp;
				temp = port_entry;
				unlink(temp->sfile);
				/*first entry timed out*/
				head = port_entry->next;
				port_entry = head;
				pred = head;
				free(temp);
			}
			else
			{
				/* some intermediate node */
				struct sfileportmap *temp;
				pred->next = port_entry->next;
				temp = port_entry;
				port_entry = port_entry->next;
				unlink(temp->sfile);
				free(temp);
			}
		}
		else
		{
			pred = port_entry;
			port_entry = port_entry->next;
		}
	}
	//printf(" Leaving port table clean\n");
	return head;
}
struct sfileportmap* port_table_search_port(struct sfileportmap* head, int port)
{
	struct sfileportmap* port_entry = head;
	while(port_entry != NULL)
	{
		if(port_entry->port == port)
		{
			port_entry->time_stamp = current_time();
			return port_entry;
		}
		else
		{
			port_entry = port_entry->next;
		}
	}
	return NULL;

}

struct sfileportmap* port_table_search_sfile(struct sfileportmap* head,  char* sfile)
{
	struct sfileportmap* port_entry = head;
	while(port_entry != NULL)
	{
		if(strcmp(port_entry->sfile, sfile) ==0)
		{
			port_entry->time_stamp =  current_time();
			return port_entry;
		}
		else
		{
			port_entry = port_entry->next;
		}
	}
	return NULL;
}

int current_time()
{
	struct timeval ts;
	gettimeofday(&ts,NULL);
	return ts.tv_sec;
}
void port_table_print(struct sfileportmap* head)
{
	//printf(" entering port table print\n");
	struct sfileportmap* port_entry = head;
	while(port_entry != NULL)
	{
		printf(" port = %d sun = %s\n", port_entry->port, port_entry->sfile);
		port_entry = port_entry->next;
	}
	//printf(" leaving port table print\n");
}

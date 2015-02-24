#include<sys/types.h>
#include "constants.h"
struct sfileportmap{
	char sfile[108];
	time_t time_stamp;
	int port;
	struct sfileportmap *next;
};
struct sfileportmap* port_table_insert(struct sfileportmap*, char *, int);
struct sfileportmap* port_table_clean(struct sfileportmap*);
struct sfileportmap* port_table_search_port(struct sfileportmap*, int );
struct sfileportmap* port_table_search_sfile(struct sfileportmap *, char *);
void port_table_print(struct sfileportmap*);
int current_time();

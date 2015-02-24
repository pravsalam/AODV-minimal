/* header file for odr protocol*/
#include<sys/types.h>
#define ODR_PACK_LEN 1500
#define RREQ 0
#define RREP 1
#define ODR_DATA 2
struct odrhdr{
	int msg_type;
	int bcast_id;
	char src_ip[16];
	char dest_ip[16];
	int hop_count;
	int src_port;
	int dest_port;
	time_t time_stamp;
};

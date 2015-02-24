#include<stdio.h>
#include<netdb.h>
char *gethostbyip(char *ip)
{
	struct hostent *he;
	struct in_addr ipaddr;
	inet_pton(AF_INET, ip, &ipaddr);
	he = gethostbyaddr(&ipaddr, sizeof(ipaddr), AF_INET );
	//printf("ip %s hostname = %s\n", ip, he->h_name);
	return he->h_name;
}
/*void main()
{
	char *ip = "130.245.156.21";
	hostbyip(ip);
}*/

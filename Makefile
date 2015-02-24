#include ../Make.defines
include /users/cse533/Stevens/unpv13e/Make.defines
CC = gcc
#LIBS = /home/praveen/Documents/np-3/unp/libunp.a
LIBS = /users/cse533/Stevens/unpv13e/libunp.a
PROGS = odr_palam client_palam server_palam prhw
all: ${PROGS}
FLAGS = -g -O2
PFLAGS = -I /users/cse533/Stevens/unpv13e/lib/ -g -O2 -D_REENTRANT -Wall
odr_palam : odr_protocol.o get_hw_addrs.o routing_table.o odr_api.o port_table.o common_functions.o
	${CC} ${CFLAGS} -o $@ odr_protocol.o get_hw_addrs.o routing_table.o port_table.o common_functions.o -lpthread ${LIBS}
prhw : prhwaddrs.o get_hw_addrs.o
	${CC} ${FLAGS} -o $@ prhwaddrs.o get_hw_addrs.o ${LIBS}
prhwaddrs.o : prhwaddrs.c
	${CC} ${FLAGS} -c prhwaddrs.c
odr_protocol.o : odr_protocol.c
	${CC} ${PFLAGS} -c odr_protocol.c
get_hw_addrs.o : get_hw_addrs.c
	${CC} ${FLAGS} -c get_hw_addrs.c 
routing_table.o : routing_table.c
	${CC} ${FLAGS} -c routing_table.c
odr_api.o: odr_api.c 
	${CC} ${FLAGS} -c odr_api.c
port_table.o: port_table.c
	${CC} ${FLAGS} -c port_table.c
server_palam: time_serv.o odr_api.o common_functions.o
	${CC} ${FLAGS} -o $@ time_serv.o odr_api.o common_functions.o
time_serv.o: time_serv.c
	${CC} ${FLAGS} -c time_serv.c
client_palam: time_cli.o odr_api.o common_functions.o
	${CC} ${FLAGS} -o $@ time_cli.o odr_api.o common_functions.o
time_cli.o: time_cli.c
	${CC} ${FLAGS} -c time_cli.c
common_functions.o: common_functions.c
	${CC} ${FLAGS} -c common_functions.c
clean: 
	rm odr_palam client_palam server_palam prhw *.o

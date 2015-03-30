
CFLAGS += -std=gnu99
LDFLAGS += -lrt

target-y = \
	udp_msg_client \
	udp_msg_server

udp_msg_client_files-y = \
	common.c \
	udp_msg_client.c

udp_msg_server_files-y = \
	common.c \
	udp_msg_server.c

udp_msg_server_ldflags-y = \
	-lpthread

include Makefile.lib

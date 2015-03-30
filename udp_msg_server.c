#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>
#include <getopt.h>
#include "common.h"
#include "debug.h"


#define	DEFAULT_MSG_SIZE	64
#define DEFAULT_PORT 		5000

#define MAGIC_START 0xABCDEFAA
#define MAGIC_END	0xABCDEFBB
#define MAGIC_DATA	0xABCDEFCC

#define	ETH_HEADER	14
#define IP_HEADER	20
#define UDP_HEADER	8

enum method {
	USE_SELECT,
	USE_EPOLL,
	USE_THREAD
};

unsigned int packet_counter;
unsigned int waiting_packet;
uint64_t byte_received;
struct timespec before;
struct timespec after;
static int exit_flag = 0;

/* Command Line Arguments */
typedef struct program_opts {
	unsigned short port;
	const char *scheduler;
	int priority;
	int receive_buffer_size;
	int method;
} opts_t;
opts_t options = { DEFAULT_PORT, "OTHER", 0, 0, USE_SELECT };

const char *opt_string = "p:S:m:P:r:h?";

const struct option long_opts[] = {
		{ "port", required_argument, NULL, 'p' },
		{ "scheduler", required_argument, NULL, 'S' },
		{ "priority", required_argument, NULL, 'P' },
		{ "receive-buffer-size", required_argument, NULL, 'r' },
		{ "method", required_argument, NULL, 'm' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, no_argument, NULL, 0 }
};

void display_usage (const char *program)
{
	printf("Usage: %s [ OPTIONS ]\n"
			"Options:\n"
			"[-p, --port LISTEN_PORT]\n"
			"[-S, --scheduler] Scheduler Algorithm (OTHER, BATCH, IDLE, FIFO, RR)\n"
			"[-P, --priority NUMBER] Realtime priority between 0.99 for realtime schedulers\n"
			"[-r, --receive-buffer-size SIZE]\n"
			"[-m, --method SELECT|EPOLL|THREAD]\n\n", program);
	printf("Scheduler Types can be: OTHER (default), BATCH, IDLE, FIFO, RR\n");
	printf("FIFO and RR are realtime schedulers\n");
	exit(1);
}

void parse_cmdline (int argc, char *argv[])
{
	int opt = 0;
	int long_index = 0;

	opt = getopt_long(argc, argv, opt_string, long_opts, &long_index);
	while (opt != -1) {
		switch (opt) {
		case 'S':
			options.scheduler = optarg;
			break;
		case 'P':
			options.priority = atoi(optarg);
			break;
		case 'p':
			options.port = (unsigned short)atoi(optarg);
			break;
		case 'r':
			options.receive_buffer_size = atoi(optarg);
			break;
		case 'm':
			if (strcasecmp(optarg, "SELECT") == 0) {
				options.method = USE_SELECT;
			} else if (strcasecmp(optarg, "EPOLL") == 0) {
				options.method = USE_EPOLL;
			} else if (strcasecmp(optarg, "THREAD") == 0) {
				options.method = USE_THREAD;
			} else {
				errorf("Unknown method: %s, SELECT method will be used", optarg);
			}
			break;
		case 'h':
		case '?':
			display_usage(argv[0]);
			break;

		default:
			break;
		}
		opt = getopt_long(argc, argv, opt_string, long_opts, &long_index);
	}
}

void parse_data (char *buf, int size)
{
	unsigned int type;
	memcpy(&type, buf, 4);

	if (type == MAGIC_START) {
		packet_counter	= 0;
		byte_received	= 0;
		memcpy(&waiting_packet, buf + 4, 4);
	} else if (type == MAGIC_END) {
		struct timespec result;
		clock_gettime(CLOCK_MONOTONIC, &after);
	    result = timespec_diff(before, after);
	    double loss = (waiting_packet - packet_counter) * 100.0 / waiting_packet;
	    infof("%d packet received within %li.%03li seconds", packet_counter, result.tv_sec, result.tv_nsec / 1000000);
	    infof("Total Byte Received: %" PRIu64, byte_received);
	    double spent_time = result.tv_sec + result.tv_nsec / (1000 * 1000 * 1000.0);
	    debugf("Bandwith: DATA: %.2f Mbps, WIRE: %.2f Mbps, Data/Header Ratio: %.2f, Packet Loss: %%%.6f",
	    		(byte_received / spent_time) * 8 / 1000000,
				((byte_received + (ETH_HEADER + IP_HEADER + UDP_HEADER) * packet_counter) / spent_time) * 8 / 1000000,
				byte_received / ((ETH_HEADER + IP_HEADER + UDP_HEADER) * (float)packet_counter), loss);
	    packet_counter = 0;
	} else if (type == MAGIC_DATA) {
		if (packet_counter == 0) {
			clock_gettime(CLOCK_MONOTONIC, &before);
		}
		packet_counter++;
		byte_received += size;
	}
}

void use_select (int sock)
{
	char buf[4096];
	struct sockaddr_in other;
    socklen_t socklen = sizeof(struct sockaddr_in);
    fd_set readfs;
    fd_set masterfs;
    struct timeval tout;
    int ret;

    FD_ZERO(&masterfs);
	FD_SET(sock, &masterfs);

	for (;;) {
		tout.tv_sec = 1;
		tout.tv_usec = 0;

select_again:

		readfs = masterfs;

		ret = select(sock + 1, &readfs, NULL, NULL, &tout);
		if (ret < 0 && errno != EINTR) {
			errorf("Unexpected error");
			return;
		} else if (ret > 0) {
			if (FD_ISSET(sock, &readfs)) {
				if ((ret = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *) &other, &socklen)) < 0) {
					errorf("recvfrom failed");
					return;
				}
				parse_data(buf, ret);
			}
			goto select_again;
		} else if (ret == 0) {
			/* timeout */
			if (packet_counter)
				debugf("%u packets received", packet_counter);
		}
	}
}

void use_epoll (int sock)
{
	(void) sock;
	errorf("Not implemented yet!");
	exit(1);
}

void * use_thread (void *args)
{
	int sock = *(int*)args;
	int ret;
	char buf[4096];
    struct sockaddr_in other;
    socklen_t socklen = sizeof(struct sockaddr_in);

	while (!exit_flag) {
		if ((ret = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *) &other, &socklen)) < 0) {
			if (errno != EINTR) {
				errorf("recvfrom failed: %s", strerror(errno));
				return NULL;
			}
			continue;
		}
		parse_data(buf, ret);
	}

	return NULL;
}

int main (int argc, char *argv[])
{
    struct sockaddr_in self;
    int s;

    parse_cmdline(argc, argv);

    if (select_scheduler(options.scheduler, options.priority) < 0) {
    	errorf("Scheduler selection failed");
    	exit(1);
    }

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("socket");
        return 1;
    }

    if (options.receive_buffer_size > 0) {
    	if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&options.receive_buffer_size, sizeof(int)) < 0) {
    		errorf("Coudn't set socket receive buffer size to: %d [%s]", options.receive_buffer_size, strerror(errno));
    	} else {
    		socklen_t size = sizeof(int);
    		int current_buffer_size;
    		getsockopt(s, SOL_SOCKET, SO_RCVBUF, &current_buffer_size, &size);
    		if (current_buffer_size < options.receive_buffer_size) {
    			errorf("Socket buffer size setting rejected!");
    			errorf("Please check %srmem_max%s", COLOR_RED_BOLD, COLOR_DEFAULT);
    			infof("%ssudo sysctl -w net.core.rmem_max=%d%s", COLOR_BLUE_BOLD, options.receive_buffer_size, COLOR_DEFAULT);
    		}
    	}
    }

    memset((char *) &self, 0, sizeof(struct sockaddr_in));
    self.sin_family = AF_INET;
    self.sin_port = htons(options.port);
    self.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s, (struct sockaddr *) &self, sizeof(self)) == -1) {
        perror("bind");
        return 1;
    }

    if (options.method == USE_SELECT) {
    	use_select(s);
    } else if (options.method == USE_EPOLL) {
    	use_epoll(s);
    } else {
		pthread_t worker_thread;
		if (pthread_create(&worker_thread, NULL, use_thread, &s) < 0) {
			perror("thread create");
		}
		pthread_join(worker_thread, NULL);
    }

    close(s);
    return 0;
}

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sched.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include "common.h"
#include "debug.h"

#define MSG_SIZE_HEADER			4
#define DEFAULT_PORT 			5000
#define DEFAULT_MSG_SIZE		64
#define DEFAULT_TOTAL_SEND		100000

#define MAGIC_START 0xABCDEFAA
#define MAGIC_END	0xABCDEFBB
#define MAGIC_DATA	0xABCDEFCC

/* Command Line Arguments */
typedef struct program_opts {
	char* server;
	unsigned short port;
	int payload;
	int total_send;
	int connected_mode;
	const char *scheduler;
	int priority;
} opts_t;
opts_t options = { "127.0.0.1", DEFAULT_PORT, DEFAULT_MSG_SIZE, DEFAULT_TOTAL_SEND, 0, "OTHER", 0 };

const char *opt_string = "s:p:S:P:m:t:ch?";

const struct option long_opts[] = {
		{ "server", required_argument, NULL, 's' },
		{ "port", required_argument, NULL, 'p' },
		{ "msg-payload", required_argument, NULL, 'm' },
		{ "total", required_argument, NULL, 't' },
		{ "connected-mode", no_argument, NULL, 'c' },
		{ "scheduler", required_argument, NULL, 'S' },
		{ "priority", required_argument, NULL, 'P' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, no_argument, NULL, 0 }
};

void display_usage (const char *program)
{
	printf("Usage: %s [ OPTIONS ]\n"
			"Options:\n"
			"[-s, --server SERVER_IP] Remote Server Ip\n"
			"[-p, --port PORT] Remote Server Port\n"
			"[-m, --msg-payload PAYLOAD] Between 0..1500\n"
			"[-t, --total TOTAL_SEND] Number of packets\n"
			"[-S, --scheduler] Scheduler Algorithm (OTHER, BATCH, IDLE, FIFO, RR)\n"
			"[-P, --priority NUMBER] Realtime priority between 0.99 for realtime schedulers\n"
			"[-c] Use connected udp mode\n"
			"[-h, --help]\n\n", program);
	exit(1);
}

void parse_cmdline(int argc, char *argv[]) {
	int opt = 0;
	int long_index = 0;

	opt = getopt_long(argc, argv, opt_string, long_opts, &long_index);
	while (opt != -1) {
		switch (opt) {
		case 'c':
			options.connected_mode = 1;
			break;
		case 's':
			options.server = strdup(optarg);
			break;
		case 'p':
			options.port = (unsigned short)atoi(optarg);
			break;
		case 'm':
			options.payload = atoi(optarg);
			if (options.payload > 1500) {
				errorf("Payload set to 1500");
				options.payload = 1500;
			}
			break;
		case 't':
			options.total_send = atoi(optarg);
			break;
		case 'S':
			options.scheduler = strdup(optarg);
			break;
		case 'P':
			options.priority = atoi(optarg);
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

int main(int argc, char *argv[])
{
    struct sockaddr_in server;
    char randomdata[1500];
    char buf[1500];
    int s;
    int fd;
    int total;
    struct timespec before, after, result;
    (void) buf;

    parse_cmdline(argc, argv);

    if (select_scheduler(options.scheduler, options.priority) < 0) {
    	errorf("Scheduler selection failed");
    	exit(1);
    }

    if ( (fd = open("/dev/urandom", O_RDONLY)) < 0) {
    	errorf("unable to open /dev/urandom");
    	return 1;
    }

    if (read(fd, randomdata, sizeof(randomdata)) < (int)sizeof(randomdata)) {
    	perror("read");
    	exit(1);
    }
    close(fd);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("socket");
        return 1;
    }

    memset((char *) &server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(options.port);
    inet_pton(AF_INET, options.server, &server.sin_addr.s_addr);

	if (options.connected_mode) {
		if (connect(s, (struct sockaddr *) &server, sizeof(struct sockaddr_in)) < 0) {
			perror("connect");
			exit(1);
		}
	} 
    /* send start transaction request and msg types */
    unsigned int msg_code = MAGIC_START;
    memcpy(buf, &msg_code, 4);
    memcpy(buf + 4, &options.total_send, 4);
	if (options.connected_mode) {
		send(s, buf, 8, 0);
	} else {
		sendto(s, buf, 8, 0, (struct sockaddr *) &server,
				sizeof(struct sockaddr_in));
	}
    infof("Packet sending started");

    total = 0;
    msg_code = MAGIC_DATA;
    memcpy(randomdata, &msg_code, 4);
    clock_gettime(CLOCK_MONOTONIC, &before);
    while (total < options.total_send) {
		if (options.connected_mode) {
			if (send(s, randomdata, options.payload, 0) < 0) {
				perror("send");
				return 1;
			}
		} else {
			if (sendto(s, randomdata, options.payload, 0,
					(struct sockaddr *) &server, sizeof(struct sockaddr_in)) < 0) {
				perror("sendto()");
				return 1;
			}
		}
		total++;
	}

    /* send transaction finish packet */
	msg_code = MAGIC_END;
	memcpy(buf, &msg_code, 4);
	if (options.connected_mode) {
		send(s, buf, 4, 0);
	} else {
		sendto(s, buf, 4, 0, (struct sockaddr *) &server, sizeof(struct sockaddr_in));
	}

    clock_gettime(CLOCK_MONOTONIC, &after);
    result = timespec_diff(before, after);
    debugf("%d packet sent within %li.%03li seconds", total, result.tv_sec, result.tv_nsec / 1000000);

    close(s);
    return 0;
}

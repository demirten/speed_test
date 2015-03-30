#define _GNU_SOURCE

#include <string.h>
#include <sched.h>
#include <errno.h>
#include "common.h"
#include "debug.h"

int select_scheduler (const char *scheduler, int priority)
{
    struct sched_param sparam;
    int sched_type = SCHED_OTHER;
    if (strcasecmp(scheduler, "FIFO") == 0)		sched_type = SCHED_FIFO;
    else if (strcasecmp(scheduler, "RR") == 0)		sched_type = SCHED_RR;
    else if (strcasecmp(scheduler, "OTHER") == 0)	sched_type = SCHED_OTHER;
    else if (strcasecmp(scheduler, "BATCH") == 0)	sched_type = SCHED_BATCH;
    else if (strcasecmp(scheduler, "IDLE") == 0)	sched_type = SCHED_IDLE;
    else {
    	errorf("Unknown scheduler type: %s", scheduler);
    	return -1;
    }
    if (sched_type == SCHED_FIFO || sched_type == SCHED_RR) {
    	int min = sched_get_priority_min(sched_type);
    	int max = sched_get_priority_max(sched_type);
    	if (priority < min) priority = min;
    	else if (priority > max) priority = max;
    	sparam.sched_priority = priority;
    } else {
    	sparam.sched_priority = 0;
    }
    if (sched_setscheduler(0, sched_type, &sparam) < 0) {
    	errorf("Couldn't set scheduler to %s: %s", scheduler, strerror(errno));
    	return -1;
    }
    return 0;
}

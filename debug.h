#include <stdio.h>

#define COLOR_DEFAULT		"\033[0m"
#define COLOR_BLACK			"\033[0;30m"
#define COLOR_BLACK_BOLD	"\033[1;30m"
#define COLOR_RED			"\033[0;31m"
#define COLOR_RED_BOLD		"\033[1;31m"
#define COLOR_GREEN			"\033[0;32m"
#define COLOR_GREEN_BOLD	"\033[1;32m"
#define COLOR_BROWN			"\033[0;33m"
#define COLOR_BROWN_BOLD	"\033[1;33m"
#define COLOR_BLUE			"\033[0;34m"
#define COLOR_BLUE_BOLD		"\033[1:34m"
#define COLOR_MAGENTA		"\033[0;35m"
#define COLOR_MAGENTA_BOLD	"\033[1;35m"
#define COLOR_CYAN			"\033[0;36m"
#define COLOR_CYAN_BOLD		"\033[1;36m"
#define COLOR_LIGHTGRAY		"\033[0;37m"
#define COLOR_LIGHTGRAY_BOLD	"\033[1;37m"

#define errorf(a...) { \
        fprintf(stderr, "%serror:%s ", COLOR_RED_BOLD, COLOR_DEFAULT); \
        fprintf(stderr, a); \
        fprintf(stderr, " %s(%s %s:%d)%s\n", COLOR_BLACK_BOLD, __FUNCTION__, __FILE__, __LINE__, COLOR_DEFAULT); \
        fflush(stderr); \
}

#define infof(a...) { \
        fprintf(stderr, "%sinfo:%s ", COLOR_CYAN, COLOR_DEFAULT); \
        fprintf(stderr, a); \
        fprintf(stderr, " %s(%s %s:%d)%s\n", COLOR_BLACK_BOLD, __FUNCTION__, __FILE__, __LINE__, COLOR_DEFAULT); \
        fflush(stderr); \
}

#define debugf(a...) { \
        fprintf(stderr, "%sdebug:%s ", COLOR_MAGENTA, COLOR_DEFAULT); \
        fprintf(stderr, a); \
        fprintf(stderr, " %s(%s %s:%d)%s\n", COLOR_BLACK_BOLD, __FUNCTION__, __FILE__, __LINE__, COLOR_DEFAULT); \
        fflush(stderr); \
}

inline struct timespec timespec_diff (struct timespec before, struct timespec after)
{
	struct timespec res;
	if ((after.tv_nsec - before.tv_nsec) < 0) {
		res.tv_sec	= after.tv_sec - before.tv_sec - 1;
		res.tv_nsec	= 1000000000 + after.tv_nsec - before.tv_nsec;
	} else {
		res.tv_sec	= after.tv_sec - before.tv_sec;
		res.tv_nsec	= after.tv_nsec - before.tv_nsec;
	}
	return res;
}

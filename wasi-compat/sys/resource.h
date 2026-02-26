/* sys/resource.h stub for WASI. */
#ifndef _WASI_COMPAT_SYS_RESOURCE_H
#define _WASI_COMPAT_SYS_RESOURCE_H

#include <sys/types.h>

#define RLIMIT_CPU     0
#define RLIMIT_FSIZE   1
#define RLIMIT_DATA    2
#define RLIMIT_STACK   3
#define RLIMIT_CORE    4
#define RLIMIT_NOFILE  7

#define RLIM_INFINITY  (~0UL)

typedef unsigned long rlim_t;

struct rlimit {
	rlim_t rlim_cur;
	rlim_t rlim_max;
};

struct rusage {
	struct { long tv_sec; long tv_usec; } ru_utime;
	struct { long tv_sec; long tv_usec; } ru_stime;
};

#define RUSAGE_SELF     0
#define RUSAGE_CHILDREN (-1)

static inline int getrlimit(int resource, struct rlimit *rlp) {
	(void)resource;
	if (rlp) {
		rlp->rlim_cur = RLIM_INFINITY;
		rlp->rlim_max = RLIM_INFINITY;
	}
	return 0;
}

static inline int getrusage(int who, struct rusage *usage) {
	(void)who;
	if (usage) {
		usage->ru_utime.tv_sec = 0;
		usage->ru_utime.tv_usec = 0;
		usage->ru_stime.tv_sec = 0;
		usage->ru_stime.tv_usec = 0;
	}
	return 0;
}

#endif

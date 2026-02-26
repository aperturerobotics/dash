/* sys/wait.h stub for WASI. */
#ifndef _WASI_COMPAT_SYS_WAIT_H
#define _WASI_COMPAT_SYS_WAIT_H

#include <sys/types.h>

/* Wait status macros. */
#define WIFEXITED(s)    (((s) & 0x7f) == 0)
#define WEXITSTATUS(s)  (((s) & 0xff00) >> 8)
#define WIFSIGNALED(s)  (((signed char)(((s) & 0x7f) + 1) >> 1) > 0)
#define WTERMSIG(s)     ((s) & 0x7f)
#define WIFSTOPPED(s)   (((s) & 0xff) == 0x7f)
#define WSTOPSIG(s)     WEXITSTATUS(s)
#define WCOREDUMP(s)    ((s) & 0x80)

#define WNOHANG    1
#define WUNTRACED  2
#define WCONTINUED 8

static inline pid_t waitpid(pid_t pid, int *status, int options) {
	(void)pid; (void)status; (void)options;
	return -1;
}

static inline pid_t wait(int *status) {
	(void)status;
	return -1;
}

static inline pid_t wait3(int *status, int options, void *rusage) {
	(void)status; (void)options; (void)rusage;
	return -1;
}

#endif

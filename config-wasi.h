/* config-wasi.h -- configuration for WASI target.
 *
 * Hand-written config for cross-compiling dash to WASI.
 * Replaces autoconf-generated config.h.
 *
 * This header is force-included (-include config-wasi.h) before all
 * source files. Compiler -D flags (_WASI_EMULATED_SIGNAL, etc.) are
 * already active when this file is processed.
 */

#ifndef DASH_CONFIG_WASI_H
#define DASH_CONFIG_WASI_H

/* Package info */
#define PACKAGE_NAME "dash"
#define PACKAGE_TARNAME "dash"
#define PACKAGE_VERSION "0.5.13.1"
#define PACKAGE_STRING "dash 0.5.13.1"

/* SMALL mode: no libedit, no history */
#define SMALL 1

/* No LINENO tracking */
/* #undef WITH_LINENO */

/* WASI has no __attribute__((__alias__())) */
/* #undef HAVE_ALIAS_ATTRIBUTE */

/* Standard headers */
/* #undef HAVE_ALLOCA_H */
/* #undef HAVE_PATHS_H */

/* Path defaults for WASI */
#define _PATH_BSHELL "/bin/sh"
#define _PATH_DEVNULL "/dev/null"
#define _PATH_TTY "/dev/tty"

/* WASI libc has isblank */
#define HAVE_DECL_ISBLANK 1

/* intmax_t format */
#define SIZEOF_INTMAX_T 8
#define SIZEOF_LONG_LONG_INT 8
/* #undef PRIdMAX */
#include <inttypes.h>

/* Available functions in WASI libc */
#define HAVE_BSEARCH 1
#define HAVE_ISALPHA 1
/* #undef HAVE_MEMPCPY */
#define HAVE_STPCPY 1
/* #undef HAVE_STRCHRNUL */
#define HAVE_STRTOD 1
#define HAVE_STRTOIMAX 1
#define HAVE_STRTOUMAX 1
/* #undef HAVE_SYSCONF */
/* #undef HAVE_GETPWNAM */
/* #undef HAVE_GETRLIMIT */
/* #undef HAVE_KILLPG */
/* #undef HAVE_SIGSETMASK */
/* Provide strsignal — WASI has no sys_siglist so dash's fallback
 * won't compile. Define HAVE_STRSIGNAL and provide a stub. */
#define HAVE_STRSIGNAL 1
/* #undef HAVE_FACCESSAT */

/* WASI has no F_DUPFD_CLOEXEC */
#define HAVE_F_DUPFD_CLOEXEC 0

/* WASI has no separate 64-bit file ops */
#define fstat64 fstat
#define lstat64 lstat
#define stat64 stat
#define open64 open
#define readdir64 readdir
#define dirent64 dirent
/* #undef HAVE_GLOB */
#define glob64_t glob_t
#define glob64 glob
#define globfree64 globfree

/* WASI has no fnmatch */
/* #undef HAVE_FNMATCH */

/* Use wait3 stub from wasi-compat/sys/wait.h (avoids broken 4-arg waitpid fallback) */
#define HAVE_WAIT3 1

/* WASI emulated signal provides signal()/raise() only. */
#define HAVE_SIGNAL 1

/* No st_mtim on WASI */
/* #undef HAVE_ST_MTIM */

/* Shell defines */
#define BSD 1
#define SHELL 1

/* Disable job control for WASI */
#define JOBS 0

/*
 * POSIX signal stubs for WASI.
 *
 * WASI's emulated signal library provides signal()/raise()/sig_atomic_t
 * and signal number defines. Everything else (sigprocmask, sigemptyset,
 * kill, sigset_t, SIG_SETMASK, etc.) is behind __wasilibc_unmodified_upstream
 * guards and unavailable. Dash references these in system.h and trap.c but
 * with JOBS=0 they are effectively dead code. Provide no-op stubs.
 *
 * WASI defines sigset_t as unsigned char via __typedef_sigset_t.h.
 * We use that type as-is.
 */
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

#ifndef SIG_BLOCK
#define SIG_BLOCK     0
#define SIG_UNBLOCK   1
#define SIG_SETMASK   2
#endif

static inline int sigemptyset(sigset_t *set) {
	if (set) *set = 0;
	return 0;
}
static inline int sigfillset(sigset_t *set) {
	if (set) *set = (sigset_t)~0;
	return 0;
}
static inline int sigaddset(sigset_t *set, int sig) {
	(void)set; (void)sig;
	return 0;
}
static inline int sigdelset(sigset_t *set, int sig) {
	(void)set; (void)sig;
	return 0;
}
static inline int sigismember(const sigset_t *set, int sig) {
	(void)set; (void)sig;
	return 0;
}
static inline int sigprocmask(int how, const sigset_t *set,
			      sigset_t *old) {
	(void)how; (void)set; (void)old;
	return 0;
}
static inline int kill(pid_t pid, int sig) {
	(void)pid; (void)sig;
	return -1;
}
static inline int sigsuspend(const sigset_t *mask) {
	(void)mask;
	errno = EINTR;
	return -1;
}

/* sigaction stub. WASI's signal.h declares struct sigaction and sigaction()
 * behind __wasilibc_unmodified_upstream. Dash uses these in trap.c. */
#define SA_RESTART  0x10000000

struct sigaction {
	void (*sa_handler)(int);
	sigset_t sa_mask;
	int sa_flags;
};

static inline int sigaction(int sig, const struct sigaction *act,
			    struct sigaction *oact) {
	(void)sig;
	if (oact) {
		oact->sa_handler = SIG_DFL;
		oact->sa_mask = 0;
		oact->sa_flags = 0;
	}
	if (act && act->sa_handler != SIG_ERR)
		signal(sig, act->sa_handler);
	return 0;
}

/*
 * Host command execution for WASI.
 *
 * External commands are dispatched to the host via an imported function.
 * The host (Go/wazero) implements cat, mkdir, ls, etc. natively.
 * argc/argv follow the standard C convention (argv[0] is the command name).
 * Returns: exit status (0-255), or 127 if the command is not found.
 */
__attribute__((__import_module__("env"), __import_name__("__exec_command")))
int __wasi_host_exec(int argc, char **argv);

/*
 * POSIX process stubs for WASI.
 *
 * WASI has no fork/exec/pipe/dup. These are referenced throughout
 * dash but with JOBS=0 and reactor mode, process creation paths are
 * dead code. Provide stubs that return errors.
 */
#include <unistd.h>

static inline pid_t fork(void) {
	errno = ENOSYS;
	return -1;
}

static inline pid_t vfork(void) {
	errno = ENOSYS;
	return -1;
}

static inline int execve(const char *path, char *const argv[],
			 char *const envp[]) {
	(void)path; (void)argv; (void)envp;
	errno = ENOSYS;
	return -1;
}

static inline int pipe(int pipefd[2]) {
	(void)pipefd;
	errno = ENOSYS;
	return -1;
}

static inline int dup(int oldfd) {
	(void)oldfd;
	errno = ENOSYS;
	return -1;
}

static inline int dup2(int oldfd, int newfd) {
	(void)oldfd; (void)newfd;
	errno = ENOSYS;
	return -1;
}

/* fcntl constants. WASI provides F_GETFD/F_SETFD/F_GETFL/F_SETFL but
 * not F_DUPFD. Include fcntl.h first, then add the missing one. */
#include <fcntl.h>
#ifndef F_DUPFD
#define F_DUPFD         0
#endif

static inline pid_t getpgrp(void) {
	return 0;
}

static inline int setpgid(pid_t pid, pid_t pgid) {
	(void)pid; (void)pgid;
	return 0;
}

static inline pid_t setsid(void) {
	return 0;
}

static inline pid_t tcgetpgrp(int fd) {
	(void)fd;
	return 0;
}

static inline int tcsetpgrp(int fd, pid_t pgrp) {
	(void)fd; (void)pgrp;
	return 0;
}

static inline mode_t umask(mode_t mask) {
	(void)mask;
	return 022;
}

static inline uid_t getuid(void) { return 0; }
static inline uid_t geteuid(void) { return 0; }
static inline gid_t getgid(void) { return 0; }
static inline gid_t getegid(void) { return 0; }
static inline int getgroups(int size, gid_t list[]) {
	(void)size; (void)list;
	return 0;
}
static inline pid_t getppid(void) { return 1; }

#endif /* DASH_CONFIG_WASI_H */

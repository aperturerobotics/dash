/* termios.h stub for WASI -- no terminal support. */

#ifndef _WASI_COMPAT_TERMIOS_H
#define _WASI_COMPAT_TERMIOS_H

typedef unsigned int tcflag_t;
typedef unsigned int cc_t;
typedef unsigned int speed_t;

#define NCCS 32

struct termios {
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	cc_t c_cc[NCCS];
	speed_t c_ispeed;
	speed_t c_ospeed;
};

#define ICANON 0x0002

static inline int tcgetattr(int fd, struct termios *t) {
	(void)fd;
	(void)t;
	return -1;
}

static inline int tcsetattr(int fd, int actions, const struct termios *t) {
	(void)fd;
	(void)actions;
	(void)t;
	return -1;
}

#define TCSANOW 0

#endif

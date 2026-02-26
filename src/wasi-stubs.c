/*
 * WASI stub implementations for symbols referenced by the pre-generated
 * builtins.c but conditionally compiled out in WASI mode (JOBS=0, no
 * HAVE_GETRLIMIT).
 */

#include "shell.h"
#include "output.h"
#include "error.h"

#if !JOBS
int
fgcmd(int argc, char **argv)
{
	(void)argc; (void)argv;
	sh_error("no job control in this shell");
	return 1;
}

int
bgcmd(int argc, char **argv)
{
	(void)argc; (void)argv;
	sh_error("no job control in this shell");
	return 1;
}
#endif

#ifndef HAVE_GETRLIMIT
int
ulimitcmd(int argc, char **argv)
{
	(void)argc; (void)argv;
	sh_error("ulimit not supported");
	return 1;
}
#endif

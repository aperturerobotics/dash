/*
 * Dash WASI Reactor Mode
 *
 * In reactor mode, dash exports functions that can be called repeatedly
 * by the host instead of running main() once. This enables re-entrant
 * shell execution: init once, eval many times, state persists in WASM
 * linear memory between calls.
 *
 * Based on the QuickJS WASI reactor pattern.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "shell.h"
#include "main.h"
#include "options.h"
#include "var.h"
#include "output.h"
#include "eval.h"
#include "input.h"
#include "error.h"
#include "init.h"
#include "exec.h"
#include "jobs.h"
#include "memalloc.h"
#include "parser.h"
#include "trap.h"
#include "nodes.h"

static int reactor_initialized = 0;

extern struct jmploc main_handler;

/*
 * Initialize the dash shell runtime.
 *
 * This performs the same initialization as main() up to but not including
 * cmdloop(). Shell state is set up and persists in WASM linear memory
 * across subsequent calls to dash_eval.
 *
 * Returns 0 on success, -1 on error.
 */
__attribute__((export_name("dash_init")))
int
dash_init(int argc, char **argv)
{
	volatile int state;
	struct stackmark smark;

	if (reactor_initialized)
		return -1;

	state = 0;
	if (setjmp(main_handler.loc)) {
		int e;

		exitreset();
		e = exception;

		if (e == EXEND || e == EXEXIT)
			return -1;

		reset();
		popstackmark(&smark);
		FORCEINTON;

		if (state < 3)
			return -1;

		/* Initialization completed enough to be usable. */
		goto done;
	}

	handler = &main_handler;
	rootpid = getpid();
	init();
	setstackmark(&smark);
	procargs(argc, argv);

	state = 3;

	/* Skip profile reading for reactor mode. The host controls
	 * initialization via dash_eval for any startup scripts. */

	popstackmark(&smark);

done:
	reactor_initialized = 1;
	return 0;
}

/*
 * Evaluate a command string.
 *
 * This is the primary re-entrant entry point. The host calls this
 * repeatedly with command strings. Shell state (variables, functions,
 * aliases, exit status) persists between calls.
 *
 * Returns the exit status of the last command.
 */
__attribute__((export_name("dash_eval")))
int
dash_eval(const char *cmd, int len)
{
	char *s;
	int status;

	if (!reactor_initialized)
		return -1;

	if (!cmd || len <= 0)
		return 0;

	/* Copy the command string since evalstring may modify it. */
	s = malloc(len + 1);
	if (!s)
		return -1;
	memcpy(s, cmd, len);
	s[len] = '\0';

	/* evalstring handles its own setjmp for error recovery. */
	status = evalstring(s, 0);

	free(s);
	return status;
}

/*
 * Get the current exit status.
 *
 * Returns the exit status of the last command executed.
 */
__attribute__((export_name("dash_get_exitstatus")))
int
dash_get_exitstatus(void)
{
	return exitstatus;
}

/*
 * Get a shell variable value.
 *
 * Returns a pointer to the value string in WASM memory, or NULL
 * if the variable is not set. The pointer is valid until the next
 * call to dash_eval or dash_setvar.
 */
__attribute__((export_name("dash_getvar")))
const char *
dash_getvar(const char *name)
{
	if (!reactor_initialized)
		return NULL;
	return lookupvar(name);
}

/*
 * Set a shell variable.
 *
 * Returns 0 on success, -1 on error.
 */
__attribute__((export_name("dash_setvar")))
int
dash_setvar(const char *name, const char *value)
{
	if (!reactor_initialized)
		return -1;
	setvar(name, value, 0);
	return 0;
}

/*
 * Destroy the dash runtime and free all resources.
 */
__attribute__((export_name("dash_destroy")))
void
dash_destroy(void)
{
	if (!reactor_initialized)
		return;
	reactor_initialized = 0;
	/* exitshell() would call exit(). In reactor mode we just
	 * mark as uninitialized. The WASM instance will be
	 * discarded by the host. */
}

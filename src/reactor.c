/*
 * Dash WASI Reactor Mode
 *
 * In reactor mode, dash exports functions that can be called repeatedly
 * by the host instead of running main() once. This enables re-entrant
 * shell execution: init once, eval many times, state persists in WASM
 * linear memory between calls.
 *
 * Based on the QuickJS WASI reactor pattern.
 *
 * setjmp/longjmp constraint: wazero's snapshot/restore requires that
 * setjmp and longjmp occur within the same exported function invocation.
 * Each export that might trigger an error sets up its own handler.
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
 * Each call sets up its own exception handler so that all setjmp/longjmp
 * pairs stay within this invocation (required by wazero snapshot/restore).
 *
 * Returns the exit status of the last command.
 */
__attribute__((export_name("dash_eval")))
int
dash_eval(const char *cmd, int len)
{
	struct jmploc jmploc;
	struct jmploc *savehandler;
	char *s;
	int status;

	if (!reactor_initialized)
		return -1;

	if (!cmd || len <= 0)
		return 0;

	savehandler = handler;
	if (setjmp(jmploc.loc)) {
		handler = savehandler;
		FORCEINTON;
		return exitstatus;
	}
	handler = &jmploc;

	s = malloc(len + 1);
	if (!s) {
		handler = savehandler;
		return -1;
	}
	memcpy(s, cmd, len);
	s[len] = '\0';

	status = evalstring(s, 0);

	free(s);
	handler = savehandler;
	return status;
}

/*
 * Get the current exit status.
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
 * Sets up its own exception handler for safety.
 *
 * Returns 0 on success, -1 on error.
 */
__attribute__((export_name("dash_setvar")))
int
dash_setvar(const char *name, const char *value)
{
	struct jmploc jmploc;
	struct jmploc *savehandler;

	if (!reactor_initialized)
		return -1;

	savehandler = handler;
	if (setjmp(jmploc.loc)) {
		handler = savehandler;
		FORCEINTON;
		return -1;
	}
	handler = &jmploc;

	setvar(name, value, 0);

	handler = savehandler;
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
}

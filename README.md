# dash

> Fork of the [dash](https://git.kernel.org/pub/scm/utils/dash/dash.git) POSIX shell with WASI reactor support and host exec dispatch.

## About Dash

[Dash](https://en.wikipedia.org/wiki/Almquist_shell#dash) is a POSIX-compliant shell derived from the Almquist shell (ash). It is small (~15K lines of C), fast, and widely used as `/bin/sh` on Debian and Ubuntu systems.

## What This Fork Adds

This fork adds three capabilities on top of upstream dash:

### 1. WASI Reactor Build (`CMakeLists.txt`, `config-wasi.h`, `src/reactor.c`)

Compiles dash to WebAssembly using wasi-sdk with a **reactor model** entry point. Instead of blocking in `main()`, the WASM binary exports functions for re-entrant use:

- `dash_init(argc, argv)` - Initialize the shell runtime
- `dash_eval(cmd, len)` - Evaluate a command string
- `dash_get_exitstatus()` - Get exit status of last command
- `dash_getvar(name)` - Get a shell variable
- `dash_setvar(name, value)` - Set a shell variable
- `dash_destroy()` - Tear down the runtime

Shell state (variables, functions, aliases, exit status) persists in WASM linear memory between calls.

### 2. Snapshot/Restore for setjmp/longjmp (`config-wasi.h`, `wasi-compat/setjmp.h`)

Dash uses `setjmp`/`longjmp` for error recovery. Standard wasi-sdk compiles these using WASM Exception Handling, which not all runtimes support. This fork declares `__setjmp`/`__longjmp` as host imports (`env.__setjmp`, `env.__longjmp`), allowing the host runtime to implement them via snapshot/restore or any other mechanism.

### 3. Host Exec Dispatch (`src/eval.c`, `config-wasi.h`)

In WASI builds, `fork`/`exec` are stubbed (ENOSYS). Instead of failing on external commands, this fork calls a host-provided `env.__exec_command` import. The host decides how to handle the command — Go implementations, delegation to native binaries, or anything else.

Changes to `eval.c`:
- `CMDUNKNOWN` case calls host exec instead of returning 127
- `CMDNORMAL` (default) case calls host exec instead of fork+exec
- `DO_ERR` suppressed in `find_command` for WASI (host decides if command exists)

## Files Added/Modified

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | CMake build for WASI reactor target |
| `config-wasi.h` | WASI-specific config + host import declarations |
| `src/reactor.c` | Reactor entry points (init, eval, getvar, setvar, destroy) |
| `src/eval.c` | Host exec dispatch for CMDUNKNOWN and CMDNORMAL |
| `src/wasi-stubs.c` | Stubs for unavailable POSIX functions (fork, exec, pipe, etc.) |
| `wasi-compat/` | Compatibility headers (setjmp.h, sys/wait.h, sys/ioctl.h, sys/resource.h) |

## Building

Requires [wasi-sdk](https://github.com/WebAssembly/wasi-sdk).

```bash
# Native build first (generates parser tables, builtins, etc.)
./autogen.sh && ./configure && make

# WASI reactor build
mkdir -p build-wasi && cd build-wasi
cmake .. \
  -DCMAKE_SYSTEM_NAME=WASI \
  -DCMAKE_C_COMPILER=/path/to/wasi-sdk/bin/clang \
  -DCMAKE_SYSROOT=/path/to/wasi-sdk/share/wasi-sysroot \
  -DDASH_WASI_REACTOR=ON
cmake --build .
```

Output: `build-wasi/dash.wasm`

## Related Projects

- [go-dash-wasi-reactor](https://github.com/aperturerobotics/go-dash-wasi-reactor) - Go module embedding the WASM binary with wazero wrapper

## License

BSD 3-Clause (same as upstream dash). See [COPYING](./COPYING).

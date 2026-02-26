/*
 * Custom setjmp/longjmp for WASI reactor mode.
 *
 * Standard wasi-sdk setjmp requires WASM Exception Handling (EH), which
 * wazero does not support. This header provides setjmp/longjmp via host
 * imports that use wazero's snapshot/restore mechanism.
 *
 * jmp_buf holds a 64-bit checkpoint index managed by the host.
 */

#ifndef _SETJMP_H
#define _SETJMP_H

typedef long long __jmp_buf_tag;
typedef __jmp_buf_tag jmp_buf[1];

/* Host-provided setjmp: snapshots execution state, returns 0.
 * When longjmp restores this buf, setjmp "returns" the longjmp value. */
__attribute__((__import_module__("env"), __import_name__("__setjmp")))
int __host_setjmp(void *buf);

/* Host-provided longjmp: restores snapshot, making the corresponding
 * setjmp return val. Does not return. */
__attribute__((__import_module__("env"), __import_name__("__longjmp")))
__attribute__((__noreturn__))
void __host_longjmp(void *buf, int val);

#define setjmp(env)       __host_setjmp(env)
#define longjmp(env, val) __host_longjmp(env, val)

#endif /* _SETJMP_H */

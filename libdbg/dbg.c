/*
 * Phoenix-RTOS
 *
 * libdbg - in-process debug/backtrace facility
 *
 * Phoenix delivers no ucontext to signal handlers, but the libphoenix aarch64 signal trampoline
 * stashes the interrupted cpu_context_t* into the global _dbg_signal_ctx (and the leaf/fault PC
 * into _dbg_signal_pc) on every signal. This facility reads that context - the interrupted pc and
 * the x29 frame pointer - and walks the frame-pointer chain, so a crash OR a watchdog-interrupted
 * hang prints a real backtrace naming the actual code, over UART, with the board kept booted.
 *
 * Build the program with -fno-omit-frame-pointer so the x29 chain is valid. Symbolize host-side:
 *   aarch64-phoenix-addr2line -f -e <elf> <printed return addresses>
 *
 * Copyright 2026 Phoenix Systems
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>


#if defined(__aarch64__)

/* Set by libphoenix's aarch64 _signal_trampoline to the interrupted cpu_context_t* and the
 * interrupted PC (leaf/fault site; signalCtx->pc itself is clobbered with the handler address). */
extern void *_dbg_signal_ctx;
extern unsigned long _dbg_signal_pc;


/* Mirror of the kernel aarch64 cpu_context_t layout (hal/aarch64/arch/cpu.h). MUST match the
 * toolchain float ABI (same __SOFTFP__ as the kernel build => same offsets). */
typedef struct {
	unsigned long savesp;
	unsigned long cpacr;
#ifndef __SOFTFP__
	unsigned long fpcr;
	unsigned long fpsr;
	unsigned long freg[2 * 32];
#endif
	unsigned long psr;
	unsigned long pc;
	unsigned long x[31]; /* x[29] = frame pointer, x[30] = link register */
	unsigned long sp;
} dbg_ctx_t;


static void dbg_walkFp(unsigned long fp)
{
	int i;

	for (i = 0; (i < 40) && (fp != 0); i++) {
		unsigned long next = ((unsigned long *)fp)[0];
		unsigned long ret = ((unsigned long *)fp)[1];

		if (ret == 0) {
			break;
		}
		printf("dbg:   #%02d 0x%lx\n", i, ret);

		/* Stop on a non-ascending or implausibly distant frame (corrupt/leaf end). */
		if ((next <= fp) || ((next - fp) > 0x400000UL)) {
			break;
		}
		fp = next;
	}
	fflush(stdout);
}


void dbg_backtrace(const char *tag)
{
	dbg_ctx_t *c = (dbg_ctx_t *)_dbg_signal_ctx;

	printf("dbg: ===== backtrace [%s] =====\n", (tag != NULL) ? tag : "");
	if (c != NULL) {
		printf("dbg:   pc=0x%lx sp=0x%lx fp(x29)=0x%lx lr(x30)=0x%lx\n",
			_dbg_signal_pc, c->sp, c->x[29], c->x[30]);
		printf("dbg:   #LEAF 0x%lx\n", _dbg_signal_pc);
		dbg_walkFp(c->x[29]);
	}
	else {
		printf("dbg:   (no signal context; unwinding caller)\n");
		dbg_walkFp((unsigned long)__builtin_frame_address(0));
	}
	printf("dbg: ===== end backtrace =====\n");
	fflush(stdout);
}

#else /* !__aarch64__ */

void dbg_backtrace(const char *tag)
{
	printf("dbg: backtrace [%s] unsupported on this architecture\n", (tag != NULL) ? tag : "");
	fflush(stdout);
}

#endif


static void dbg_faultHandler(int sig)
{
	printf("\ndbg: *** FAULT signal=%d ***\n", sig);
	fflush(stdout);
	dbg_backtrace("fault");
	_exit(128 + sig);
}


static unsigned int dbg_watchdogSecs = 0;
static volatile unsigned int dbg_watchdogTicks = 0;


static void dbg_watchdogHandler(int sig)
{
	(void)sig;

	dbg_watchdogTicks++;
	printf("\ndbg: *** WATCHDOG tick #%u (possible hang) ***\n", dbg_watchdogTicks);
	fflush(stdout);
	dbg_backtrace("watchdog");

	if (dbg_watchdogSecs != 0) {
		alarm(dbg_watchdogSecs); /* re-arm */
	}
}


static void dbg_install(int sig, void (*fn)(int))
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = fn;
	(void)sigaction(sig, &sa, NULL);
}


void dbg_init(void)
{
	dbg_install(SIGSEGV, dbg_faultHandler);
	dbg_install(SIGILL, dbg_faultHandler);
	dbg_install(SIGBUS, dbg_faultHandler);
	dbg_install(SIGFPE, dbg_faultHandler);
	dbg_install(SIGABRT, dbg_faultHandler);
	printf("dbg: fault handlers installed (SEGV/ILL/BUS/FPE/ABRT)\n");
	fflush(stdout);
}


void dbg_arm_watchdog(unsigned int secs)
{
	dbg_watchdogSecs = secs;
	dbg_install(SIGALRM, dbg_watchdogHandler);
	alarm(secs);
	printf("dbg: watchdog armed (%u s)\n", secs);
	fflush(stdout);
}

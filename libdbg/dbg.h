/*
 * Phoenix-RTOS
 *
 * libdbg - in-process debug/backtrace facility
 *
 * Copyright 2026 Phoenix Systems
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _LIBDBG_H_
#define _LIBDBG_H_


/* Install fault handlers (SIGSEGV/SIGILL/SIGBUS/SIGFPE/SIGABRT). On a crash the faulting PC and a
 * frame-pointer backtrace (return addresses) are printed over stdout/UART, then the process exits
 * with 128+signo. Build the program with -fno-omit-frame-pointer so the x29 chain is valid. */
extern void dbg_init(void);


/* Print a backtrace now. When called from a signal handler it unwinds the INTERRUPTED code (via the
 * cpu_context_t the libphoenix aarch64 signal trampoline stashes into _dbg_signal_ctx); otherwise
 * it unwinds the caller. Symbolize host-side with:
 *   aarch64-phoenix-addr2line -f -e <elf> <printed return addresses> */
extern void dbg_backtrace(const char *tag);


/* Arm a SIGALRM watchdog: after `secs` of wall-clock the handler prints a backtrace of wherever the
 * process currently is (locating a HANG), then re-arms. Call before entering a region that may hang;
 * pass 0 later to leave it disarmed on the next fire. */
extern void dbg_arm_watchdog(unsigned int secs);


#endif /* _LIBDBG_H_ */

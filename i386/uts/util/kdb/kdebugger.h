/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_KDB_KDEBUGGER_H	/* wrapper symbol for kernel use */
#define _UTIL_KDB_KDEBUGGER_H	/* subject to change without notice */

#ident	"@(#)uts-x86:util/kdb/kdebugger.h	1.8"
#ident	"$Header: $"

/*	Used by kernel debuggers */

#ifdef _KERNEL

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h> /* COMPATIBILITY */
#endif

#ifndef _PROC_REGSET_H
#include <proc/regset.h> /* REQUIRED */
#endif

#ifndef _IO_CONF_H
#include <io/conf.h> /* COMPATIBILITY */
#endif

#elif	defined(_KERNEL)

#include <sys/types.h>	/* COMPATIBILITY */
#include <sys/regset.h>	/* REQUIRED */
#include <sys/conf.h>	/* COMPATIBILITY */

#endif	/* _KERNEL_HEADERS */

extern void (*cdebugger)();

struct kdebugger {
	void	(*kdb_entry)();
	int	kdb_reserved[5];
	struct kdebugger *kdb_next;
	struct kdebugger *kdb_prev;
};
extern struct kdebugger *debuggers;

extern boolean_t kdb_installed;

/*
 * Each entry in the dbg_io array is a pointer to a structure of
 * type conssw (which contains putchar routine, dev number, and
 * getchar routine).  Generally, the first entry will point to the
 * kernel variable, conssw, so we use whatever is the standard console
 * device.
 */

extern struct conssw *dbg_io[], **cdbg_io;
extern int ndbg_io;
extern unsigned dbg_putc_count;

#define DBG_GETCHAR()	(((*cdbg_io)->ci) ((*cdbg_io)->co_dev))
#define DBG_PUTCHAR(c)	(void) ((*(*cdbg_io)->co) (c, (*cdbg_io)->co_dev), \
				++dbg_putc_count)

/*
 * As a security feature, the kdb_security flag (set by the KDBSECURITY
 * tuneable) is provided.  If it is non-zero, the debugger should ignore
 * attempts to enter from a console key sequence.
 */
extern int kdb_security;

/*
 * Symbol table info loaded into the kernel by the unixsyms command.
 */
extern char *symtable;
extern int symtab_size;

extern char *debugger_init;	/* Optional initial command string */

extern int debugger_early;	/* Flag: too early for some features
				   (e.g. single-step; see startup.c) */

#define NREGSET	8
extern gregset_t *regset[NREGSET];

extern volatile unsigned long db_st_startpc;
extern volatile unsigned long db_st_startsp;
extern volatile unsigned long db_st_startfp;
extern unsigned long db_st_offset;
extern int *db_st_r0ptr;

/* The stack pointer as saved in a register frame is offset from the actual
   value it had at the time of the trap, since the trap pushed several words
   onto the stack.  This symbol provides the proper adjustment factor. */

#define ESP_OFFSET	0x14

/* Some useful 386 opcodes */

#define OPC_INT3	0xCC
#define OPC_PUSHFL	0x9C
#define OPC_CALL_REL	0xE8
#define OPC_CALL_DIR	0x9A
#define OPC_CALL_IND	0xFF
#define OPC_PUSH_EBP	0x55
#define OPC_MOV_RTOM	0x89
#define OPC_ESP_EBP	0xE5
#define OPC_MOV_MTOR	0x8B
#define OPC_EBP_ESP	0xEC

/* This inline function gets the register values needed to start
   a stack trace.  Must be called from the same routine which calls
   db_stacktrace(). */
asm void
db_get_stack()
{
%lab	push_eip
	call	push_eip
push_eip:
	popl	db_st_startpc
	movl	%esp, db_st_startsp
	movl	%ebp, db_st_startfp
}


extern void	db_frameregs(ulong_t, uint_t, void (*)());
extern void	db_stacktrace(void(*)(), ulong_t, int);

#endif /* _KERNEL */

#endif /* _UTIL_KDB_KDEBUGGER_H */

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_MACHINE_H
#define _MACHINE_H
#ident	"@(#)debugger:inc/i386/Machine.h	1.10.1.2"

/* Machine dependent manifest constants. */
#include <sys/signal.h>
#include <sys/fault.h>
#include <sys/procfs.h>
#include <sys/elf.h>
#include <sys/syscall.h>
#include "filehdr.h"

#define TEXT_BYTE_SWAP  

#define GENERIC_CHAR_SIGNED  	1
#define COFFMAGIC	 	I386MAGIC
#ifdef PTRACE
#define EXECCNT			4
#else
#define EXECCNT			2
#endif

#define BKPTSIZE		1
#define BKPTSIG			SIGTRAP
#define BKPTFAULT		FLTBPT
#define BKPTTEXT		"\314"

#define TRACEFAULT		FLTBPT

#if PTRACE
#define STOP_TRACE		BKPTSIG
#define STOP_BKPT		BKPTSIG
#define STOP_TYPE		PR_SIGNALLED
#else
#define STOP_TRACE		TRACEFAULT
#define STOP_BKPT		BKPTFAULT
#define STOP_TYPE		PR_FAULTED
#endif

/* signal that informs us of process state change */
#if FOLLOWER_PROC
#define SIG_INFORM		SIGUSR1 
#endif

/* special signals that shouldn't be ignored */
#if PTRACE
#define SIG_SPECIAL(I) ((I) == BKPTSIG)
#else
#define SIG_SPECIAL(I) (0)
#endif

#define ERRBIT			0x1
#define EXEC_FAILED()		(goal2 == sg_run && getreg(REG_EAX) != 0)
#define FORK_FAILED()		(getreg(REG_EFLAGS) & ERRBIT)
#define SYS_RETURN_VAL()	(getreg(REG_EAX))

#define MACHINE		EM_386

#define Elf_Ehdr	Elf32_Ehdr
#define Elf_Phdr	Elf32_Phdr
#define Elf_Shdr	Elf32_Shdr
#define Elf_Sym		Elf32_Sym
#define Elf_Dyn		Elf32_Dyn
#define Elf_Addr	Elf32_Addr
#define Elf_Half	Elf32_Half

/* constants for use in TYPE */
#define	CHAR_SIZE	1
#define	SHORT_SIZE	sizeof(short)
#define	INT_SIZE	sizeof(int)
#define	LONG_SIZE	sizeof(long)
#define	PTR_SIZE	sizeof(long)
#define	SFLOAT_SIZE	sizeof(float)
#define	LFLOAT_SIZE	sizeof(double)
/* 386/387 use 10 bytes for extended floats, even though they
 * are stored in memory as 12 bytes - the final bytes are ignored
 */
#define	XFLOAT_SIZE	12
#define EXTENDED_SIZE	10

#define SIZEOF_TYPE	ft_sint
#define PTRDIFF_TYPE	ft_int
#define PTRDIFF		int

#define ROUND_TO_WORD(x)	(((x) + sizeof(int) - 1) & ~(sizeof(int)-1))
/* for COFF core files */
#ifndef	UVUBLK
#define UVUBLK	0xe0000000
#endif

/* last valid syscall number - NOTE: OS release dependent */

#if defined(SYS_lwpcontinue)   /* ES/MP */
#define lastone SYS_lwpcontinue
#elif defined(SYS_getksym)	/* ES */
#define lastone SYS_getksym
#else	/* 4.0 */
#define lastone SYS_seteuid
#endif

/* Number of digits in string representation of a number */
#define	MAX_INT_DIGITS	10
#define	MAX_LONG_DIGITS	10

/* Number of lines in the register pane in the Disassembly window */
#define REG_DISP_LINE	6

#endif /* Machine.h */

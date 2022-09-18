/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _Proctypes_h
#define _Proctypes_h

#ident	"@(#)debugger:inc/common/Proctypes.h	1.2"

#include <sys/types.h>
#include <signal.h>
#include <sys/regset.h>
#include <sys/procfs.h>
#include <sys/fault.h>
#include "Proc.Mach.h"

// structure definitions for functions that write sets of data:
// gregs, fpregs, dbregs, sigset, sysset and fltset.  These
// structures have room at the beginning for a control word,
// to make access to the new /proc more efficient.  The control
// word is unused for ptrace or old /proc
//

typedef prmap_t map_ctl;

struct	greg_ctl {
#if !PTRACE && !OLD_PROC
	int		ctl;
#endif
	gregset_t	gregs;
};

struct	fpreg_ctl {
#if !PTRACE && !OLD_PROC
	int		ctl;
#endif
	fpregset_t	fpregs;
};

struct	sys_ctl {
#if !PTRACE && !OLD_PROC
	int		ctl;
#endif
	sysset_t	scalls;
};

struct	sig_ctl {
#if !PTRACE && !OLD_PROC
	int		ctl;
#endif
	sigset_t	signals;
};

struct	flt_ctl {
#if !PTRACE && !OLD_PROC
	int		ctl;
#endif
	fltset_t	faults;
};

#endif

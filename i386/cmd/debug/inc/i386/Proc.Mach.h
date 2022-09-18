/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _Proc_Mach_h
#define _Proc_Mach_h

#ident	"@(#)debugger:inc/i386/Proc.Mach.h	1.1"

#include <sys/regset.h>

// Machine dependent definitions for process control
// Not all machines have debug registers

struct	dbreg_ctl {
#if !PTRACE && !OLD_PROC
	int		ctl;
#endif
	dbregset_t	dbregs;
};


#endif

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:sys/hrtsys.c	1.3"

#ifndef DSHLIB
#ifdef __STDC__
	#pragma weak hrtcntl = _hrtcntl
	#pragma weak hrtalarm = _hrtalarm
#endif
#endif
#include	"synonyms.h"
#include	"sys/types.h"
#include	"sys/hrtcntl.h"

#define	HRTSYS		109

#define	HRTCNTL		0
#define HRTALARM	1

hrtcntl(cmd, clk, intp, hrtp)
int cmd, clk;
interval_t *intp;
hrtime_t *hrtp;
{
	return(syscall(HRTSYS, HRTCNTL, cmd, clk, intp, hrtp));
}

#ifndef DSHLIB

hrtalarm(cmdp, cmds)
hrtcmd_t *cmdp;
int cmds;
{
	return(syscall(HRTSYS, HRTALARM, cmdp, cmds));
}

#endif

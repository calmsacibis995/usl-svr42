/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:util/util.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h>	/* to collect tunable parameters */
#include <sys/sysinfo.h>
#include <sys/param.h>

char		putbuf[PUTBUFSZ];
int		putbufsz = PUTBUFSZ;
int		sanity_clk = SANITYCLK;

struct shlbinfo shlbinfo = {SHLBMAX, 0, 0};

/*
**	Switch to turn on/off 386 B1 stepping workarounds.
**	The variables do386b1, do386b1_387, and do386b1_x87 are all controlled
**	by the DO386B1 tuneable.
**	The do386b1_387 variable will be turned off if there is no 80387 present,
**	in order to disable workarounds which are only needed with a 387.
**	Similarly, do386b1_x87 will be turned off if there is neither an 80287
**	nor an 80387.
**	The DO386B1 tuneable takes 3 values:
**		0: disable workarounds
**		1: always enable workarounds
**		2: auto-detect B1 stepping; enable workarounds if needed
*/
int	do386b1 = DO386B1;
int	do386b1_387;	/* copied from do386b1 if 387 present */
int	do386b1_x87;	/* copied from do386b1 if 287 or 387 present */
/*
**	DO387CR3 enables the workaround for the 386 B1 stepping errata #21.
**	Like do386b1_x87, it will be turned off if do386b1 is off or there is
**	no math chip.  It needs to be separate since some hardware can't
**	support this workaround.
*/
int	do387cr3 = DO387CR3;


int	Hz = HZ;	/* From util/param.h */

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:proc/proc.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h>
#include <sys/sysmacros.h>

/* Scheduler initialization */
#define INITCLASS 	"TS"

char	*initclass = INITCLASS;

int 	exec_ncargs = ARG_MAX;

int	ngroups_max = NGROUPS_MAX;

/*	The following describe the minimum conditions required
**	for putting the stack below data and starting it in
**	the data region's page table.
*/
int minhidustk = ctob(MINHIDUSTK);
int minustkgap = ctob(stoc(MINUSTKGAP));

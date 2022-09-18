/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:lib/get_ngm.c	1.2.11.2"
#ident  "$Header: get_ngm.c 2.0 91/07/13 $"

#include	<sys/param.h>
#include	<unistd.h>

static int ngm = -1;

/*
	read the value of NGROUPS_MAX from the kernel 
*/
int
get_ngm()
{
	if (ngm != -1)
		return ngm;

	ngm = sysconf(_SC_NGROUPS_MAX);

	return ngm;
}

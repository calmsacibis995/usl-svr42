/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/hba/mcst.cf/Stubs.c	1.2"
#ident	"$Header: $"

#include	<sys/errno.h>

int
mcst_bdinit()	{ return(0); }

void
mcst_drvinit()	{}

int
mcst_cmd()	{ return(ENOSYS); }

void	*
mcst_int()	{ return((void *)0); }

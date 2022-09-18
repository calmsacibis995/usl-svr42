/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/lockd/setbuffer.c	1.1.4.4"
#ident	"$Header: $"

#include <stdio.h>
#include "stdlib.h"

/*
 * set line buffering
 */
setlinebuf(iop)
	register FILE *iop;
{
	register unsigned char *buf;

	fflush(iop);
		setvbuf(iop, (char *)NULL, _IOLBF|_IOMYBUF, 128);
}


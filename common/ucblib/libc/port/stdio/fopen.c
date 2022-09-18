/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ucb:common/ucblib/libc/port/stdio/fopen.c	1.3"
#ident	"$Header: $"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*LINTLIBRARY*/
#include <stdio.h>
#include <fcntl.h>

extern int fclose();
extern FILE *_findiop(), *_endopen();
extern FILE *_realfopen(), *_realfreopen();

FILE *
fopen(file, mode)
const char	*file, *mode;
{
	FILE *iop;

	iop = _realfopen(file, mode);
	if (mode[0] == 'a')   {
		if ((fseek(iop,0L,SEEK_END)) < 0)  {
			(void) fclose(iop);
			return NULL;
		}
	}
	return (iop);
}

FILE *
freopen(file, mode, fptr)
const char	*file, *mode;
FILE *fptr;
{
	FILE *iop;
	iop =  _realfreopen(file, mode, fptr);

	if (mode[0] == 'a')   {
		if ((fseek(iop,0L,SEEK_END)) < 0)  {
			(void) fclose(iop);
			return NULL;
		}
	}
	return (iop);
}

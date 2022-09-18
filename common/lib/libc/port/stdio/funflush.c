/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/funflush.c	1.1"
/*LINTLIBRARY*/

#include "synonyms.h"
#include "shlib.h"
#include <stdio.h>
#include "stdiom.h"

#ifdef STDIO_NEW
#include "stdlock.h"
#endif

#ifdef __STDC__
	#pragma weak funflush = _funflush
#endif

void
#ifdef __STDC__
funflush(register FILE *fp)	/* toss any pending buffered data */
#else
funflush(fp)register FILE *fp;
#endif
{
#ifdef STDIO_NEW
	BFILE *bp;
#endif

	if (fp == 0)	/* funflush all streams */
	{
#ifdef STDIO_NEW
		bp = STDIN;
		do
		{
			funflush((FILE *)bp->file._base);
		} while ((bp = bp->next) != 0);
#else
		register int i;
		for (i = 0; i < _NFILE; i++)
		{
			if( (__iob[i])._base != 0 )
				funflush(&__iob[i]);
		}
#endif
		return;
	}
#ifdef STDIO_NEW
	bp = (BFILE *)fp->_base;
	STDLOCK(&bp->lock);
	fp->_ptr = bp->begptr;
#else
	fp->_ptr = fp->_base;
#endif
	fp->_cnt = 0;	/* seek back if _IOREAD?? */
	if (fp->_flag & _IORW)
		fp->_flag &= ~(_IOREAD | _IOWRT);
#ifdef STDIO_NEW
	STDUNLOCK(&bp->lock);
#endif
}

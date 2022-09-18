/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/data.c	2.15.1.1"
/*LINTLIBRARY*/
#ifdef __STDC__
	#pragma weak _iob = __iob
#endif
#include "synonyms.h"
#include <stdio.h>
#include "stdiom.h"

/*
 * Ptrs to start of preallocated buffers for stdin, stdout.
 * Some slop is allowed at the end of the buffers in case an upset in
 * the synchronization of _cnt and _ptr (caused by an interrupt or other
 * signal) is not immediately detected.
 */
Uchar _sibuf[BUFSIZ + _SMBFSZ], _sobuf[BUFSIZ + _SMBFSZ];
Uchar _smbuf[_NFILE + 1][_SMBFSZ] = {0};  /* shared library compatibility */

/*
 * Ptrs to end of read/write buffers for first _NFILE devices.
 * There is an extra bufend pointer which corresponds to the dummy
 * file number _NFILE, which is used by sscanf and sprintf.
 */
Uchar *_bufendtab[_NFILE+1] = { NULL, NULL, _smbuf[2] + _SBFSIZ, };

FILE _iob[_NFILE] = {
	{ 0, NULL, NULL, _IOREAD, 0},
	{ 0, NULL, NULL, _IOWRT, 1},
	{ 0, _smbuf[2], _smbuf[2], _IOWRT+_IONBF, 2},
};

/*
 * Ptr to end of io control blocks
 */
FILE *_lastbuf = &_iob[_NFILE];

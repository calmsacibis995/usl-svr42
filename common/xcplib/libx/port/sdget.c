/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)xcplibx:common/xcplib/libx/port/sdget.c	1.6"
#ident  "$Header: sdget.c 1.1 91/07/04 $"

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#include "sys/sd.h"
#include <errno.h>
#include <stdarg.h>

extern	int	errno;

extern	char	*_sdget();

/***	sdget -- create/attach to XENIX shared data segment
 *
 *	char *addr;
 *	addr = sdget( path, flags[, size, mode] );
 */

char *
sdget( char *path, int flags, ...)
{
	va_list	ap ;
	long size ;
	int mode ;

	va_start ( ap, ) ;
	size = va_arg ( ap, long ) ;
	mode = va_arg ( ap, int ) ;
	va_end( ap ) ;
	if ( ((flags & SD_CREAT) == SD_CREAT) && (size <= 0) ) {
	    	errno = EINVAL;
	    	return( (char *) -1 );
	}

	/* limit == size - 1 */
	return( _sdget( path, flags, (unsigned long) size - 1, mode ) );
}

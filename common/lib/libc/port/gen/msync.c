/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/msync.c	1.4"
#ifdef __STDC__
	#pragma weak msync = _msync
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <sys/mman.h>

msync(addr, len, flags)
caddr_t	addr;
size_t	len;
int flags;
{
	return (memcntl(addr, len, MC_SYNC, (caddr_t)flags, 0, 0));
}

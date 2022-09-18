/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libelf:i386/svr4.c	1.1"


#include "syn.h"
#include <sys/utsname.h>
#include "libelf.h"
#include "decl.h"


int
_elf_svr4()
{
	struct utsname	u;
	static int	vers = -1;

	if (vers == -1)
	{
		if (uname(&u) > 0)
			vers = 1;
		else
			vers = 0;
	}
	return vers;
}

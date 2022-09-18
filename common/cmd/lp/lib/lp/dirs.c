/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:lib/lp/dirs.c	1.10.1.3"
#ident	"$Header: $"

#include "sys/types.h"
#include "errno.h"

#include "lp.h"
#include "mac.h"

#include <priv.h>
#include <sys/secsys.h>

/**
 ** mkdir_lpdir()
 **/

int
#if	defined(__STDC__)
mkdir_lpdir (
	char *			path,
	int			mode
)
#else
mkdir_lpdir (path, mode)
	char			*path;
	int			mode;
#endif
{
	int			old_umask = umask(0);
	int			ret;
	int			save_errno;


	ret = Mkdir(path, mode);
	if (ret != -1)
		ret = chown_lppath(path);
	save_errno = errno;
	if (old_umask)
		(void)umask (old_umask);
	errno = save_errno;

	/* moved change level to calling routines as needed. abs s19 */

	return (ret);
}

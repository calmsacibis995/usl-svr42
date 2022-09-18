/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/procprivl.c	1.9"

#ifdef __STDC__
	#pragma	weak	procprivc	= _procprivc
	#pragma	weak	procprivl	= _procprivl
#endif

/*LINTLIBRARY*/
#include	"synonyms.h"
#include	<stdarg.h>
#include	<values.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<priv.h>
#include	<sys/secsys.h>

/*
 *	procprivl(cmd, prid)
 *
 *	where prid is a vector prid[0], ... prid[x], NULL
 *	last vector element must be NULL
 */
int
#ifdef	__STDC__
procprivl(int cmd, ...)
#else
procprivl(cmd, va_alist)
	int	cmd;
	va_dcl
#endif
{
	register int	cnt = 0;
	static int	priv = 1;
	va_list		ap, sap;

	switch (cmd) {
	case CLRPRV:
	case CNTPRV:
	case PUTPRV:
	case SETPRV:
		break;
	default:
		errno = EINVAL;
		return -1;
	}	/* end of "cmd" switch */

	if (priv) {	/* do this only if the process is privileged */
#ifdef	__STDC__
		va_start(ap,);
#else
		va_start(ap);
#endif
		sap = ap;
		while (va_arg(ap, priv_t))
			++cnt;
		va_end(ap);

		priv = procpriv(cmd, sap, cnt);

		return priv;
	}
	else {	/* the process is not privileged */
		return 0;
	}
}


/*
 *	procprivc(cmd, prid)
 *
 *	where prid is a vector prid[0], ... prid[x], NULL
 *	last vector element must be NULL
*/
int
#ifdef	__STDC__
procprivc(int cmd, ...)
#else
procprivc(cmd, va_alist)
	int	cmd;
	va_dcl
#endif
{
	register int	cnt = 0;
	static int	priv = 1;
	static int	in_effect = -1;
	ulong		pm_flg = 0;
	va_list		ap, sap;

	switch (cmd) {
	case CLRPRV:
	case CNTPRV:
	case PUTPRV:
	case SETPRV:
		break;
	default:
		errno = EINVAL;
		return -1;
	}	/* end of "cmd" switch */

	if (in_effect < 0) {		/* do this first time into routine */
		(void) secsys(ES_PRVINFO, (caddr_t)&pm_flg);
		/*
		 * The variable "in_effect" is set iff the privilege
		 * mechanism supports fine-grained privilege bracketing
		 * in the binary.
		 */
		in_effect = (pm_flg & PM_PRVMODE);
	}
	if (priv && in_effect) {
#ifdef	__STDC__
		va_start(ap,);
#else
		va_start(ap);
#endif
		sap = ap;
		while (va_arg(ap, priv_t))
			++cnt;
		va_end(ap);

		priv = procpriv(cmd, sap, cnt);

		return priv;
	}
	else {
		/*
		 * Either the process isn't privileged or
		 * fine-grained bracketing is not in effect.
		 */
		return 0;
	}
}

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:lib/lp/audit.c	1.2.1.3"
#ident	"$Header: $"

#include <sys/param.h>
#include <audit.h>

#ifdef	__STDC__
void
CutAuditRec (int type, int status, int size , char *msgp)
#else
void
CutAuditRec (type, status, size, msgp)

int	type;	/* event type */
int	status;	/* event exit status */
int	size;	/* size of msgp */
char *	msgp;	/* message */
#endif
{
        arec_t	rec;		/* auditdmp(2) structure */

	rec.rtype	= type;
	rec.rstatus	= status;
	rec.rsize	= size;
	rec.argp	= msgp;

	auditdmp (&rec, sizeof (arec_t));

        return;
}

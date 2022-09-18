/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)head.usr:acl.h	1.1.2.2"
#ident  "$Header: acl.h 1.3 91/06/21 $"

#ifndef _ACL_H
#define	_ACL_H


#include <sys/acl.h>

#if defined (__STDC__)

extern int acl(char *, int, int, struct acl *);
extern int aclipc(int, int, int, int, struct acl *);
extern int aclsort(int, int, struct acl *);

#else	/* !defined (__STDC__) */

extern int acl();
extern int aclipc();
extern int aclsort();

#endif	/* defined (__STDC__) */

#endif	/* _SYS_ACL_H */

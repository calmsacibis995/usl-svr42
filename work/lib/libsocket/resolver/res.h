/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libsocket:common/lib/libsocket/resolver/res.h	1.1.1.3"
#ident "$Header: 1.1 91/02/28 $"

#ifdef SYSV
#define	rindex		strrchr
#define	index		strchr
#define	bcmp(a,b,c)	memcmp((a),(b),(c))
#define	bzero(a,b)	memset((a), 0, (b))
#define	bcopy(a,b,c)	memcpy((b),(a),(c))
#endif /* SYSV */

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_SCCSID_H	/* wrapper symbol for kernel use */
#define _UTIL_SCCSID_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:util/sccsid.h	1.2"
#ident	"$Header: $"
#ifdef lint
#define VERSION(x);
#define HVERSION(x);
#else
#define VERSION(x) static char sccsid[]="x";
#define HVERSION(n,x) static char n[]="x";
#endif

#endif	/* _UTIL_SCCSID_H */

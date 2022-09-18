/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)sh:common/cmd/sh/mac.h	1.8.5.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/sh/mac.h,v 1.1 91/02/28 20:08:40 ccs Exp $"
/*
 *	UNIX shell
 */

#define TRUE	(-1)
#define FALSE	0
#define LOBYTE	0377
#define QUOTE	0200

#ifndef EOF
#define EOF	0
#endif
#define NL	'\n'
#define SP	' '
#define LQ	'`'
#define RQ	'\''
#define MINUS	'-'
#define COLON	':'
#define TAB	'\t'


#define MAX(a,b)	((a)>(b)?(a):(b))

#define blank()		prc(SP)
#define	tab()		prc(TAB)
#define newline()	prc(NL)


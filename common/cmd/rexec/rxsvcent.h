/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rexec:rxsvcent.h	1.2.2.2"
#ident  "$Header: rxsvcent.h 1.2 91/06/27 $"


#define	RX_SVCFILE	"/etc/rexec/services"
#define	RX_TMPSVCFILE	"/etc/rexec/tmp"

typedef struct {
	char	name[RX_MAXSVCSZ + 1];
	char	descr[RX_MAXSVCDESCR + 1];
	char	utmp[RX_MAXUTMP + 1];
	char	def[RX_MAXSVCDEF + 1];
} RX_SERVICE;

#define	RX_TOK_OK_LAST		3
#define	RX_TOK_OK		2
#define	RX_TOK_TRUNC_LAST	1
#define	RX_TOK_TRUNC		0
#define	RX_TOK_ERROR		(-1)

#define	EOF			(-1)
#define	EOLN			'\n'
#define	TAB			'\t'
#define	QUOTE1			'\''
#define	QUOTE2			'"'
#define	BACKSLASH		'\\'
#define	PERCENT			'%'

#define	RX_SCAN_ERROR		0x01
#define	RX_SCAN_EOF		0x02
#define	RX_SCAN_LAST		0x04

#define	RXF_TRUNC_NAME		0x01
#define	RXF_TRUNC_DESCR		0x02
#define	RXF_TRUNC_UTMP		0x04
#define	RXF_TRUNC_DEF		0x08

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)localedef:common/cmd/localedef/colltbl/colltbl.h	1.1.5.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/localedef/colltbl/colltbl.h,v 1.1 91/02/28 17:44:34 ccs Exp $"

/* Diagnostic mnemonics */
#define WARNING		0
#define ERROR		1
#define ABORT		2

enum errtype {
	GEN_ERR,
	DUPLICATE,
	EXPECTED,
	ILLEGAL,
	TOO_LONG,
	INSERTED,
	NOT_FOUND,
	NOT_DEFINED,
	TOO_MANY,
	INVALID	,
	BAD_OPEN,
	NO_SPACE,
	NEWLINE,
	REGERR,
	CWARN,
	YYERR,
	PRERR
};

/* Diagnostics Functions and Subroutines */
void	error();
void	regerr();
void	usage();

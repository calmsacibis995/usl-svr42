/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/bkregmsgs.c	1.2.5.2"
#ident  "$Header: bkregmsgs.c 1.2 91/06/21 $"

char *errmsgs[] = {
	"Option \"%c\" is invalid.\n",
	"Tag %s does not exist in table %s (return code %d).\n",
	"Could not allocate memory for a table entry.\n",
	"Could not read table entry number %d.\n",
	"Could not open temporary file %s.\n",
	"Cannot open table %s (%d).\n",
};
int	lasterrmsg = sizeof( errmsgs )/sizeof( char * );

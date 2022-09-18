/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkgadd/msgdefs.h	1.1.7.3"
#ident  "$Header: $"

#define ERR_NOPKGS \
"selected package <%s> not found on media <%s>"

#define ERR_CHDIR \
"unable to change directory to <%s>"

#define ERR_DSINIT \
"could not process datastream from <%s>"

#define ERR_STREAMDIR \
"unable to make temporary directory to unpack datastream"

#define MSG_SUSPEND \
"Installation of <%s> has been suspended."

#define MSG_1MORETODO \
"\nThere is 1 more package to be %s."

#define MSG_MORETODO \
"\nThere are %d more packages to be %s."

#define ASK_CONTINUE \
"Do you want to continue with installation"


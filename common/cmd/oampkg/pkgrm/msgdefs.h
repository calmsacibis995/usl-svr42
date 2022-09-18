/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkgrm/msgdefs.h	1.2.5.2"
#ident  "$Header: msgdefs.h 1.2 91/06/27 $"

#define ASK_CONTINUE \
"Do you want to continue with package removal"

#define ERR_NOPKGS \
"no packages were found in <%s>"

#define ERR_CHDIR \
"unable to change directory to <%s>"

#define MSG_SUSPEND \
"Removals of <%s> has been suspended."

#define MSG_1MORETODO \
"\nThere is 1 more package to be removed."

#define MSG_MORETODO \
"\nThere are %d more packages to be removed."

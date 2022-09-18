/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libns:common/lib/libns/ns_errlist.c	1.5.4.3"
#ident  "$Header: ns_errlist.c 1.2 91/06/26 $"

char	*ns_errlist[] = {
	"No error",
	"Format error",
	"Name server failure",
	"Resource name not found",
	"Request type not implemented",
	"Permission for operation denied",
	"Resource name not unique",
	"System call failure in name server",
	"Error in accessing password file on primary name server",
	"Password does not match that on primary name server",
	"No password entry for this machine",
	"Setup error for name server message",
	"No reply from domain name server",
	"Unknown domain",
	"Name Server in recovery, try again",
	"Unknown failure"
};
int	ns_err = { sizeof(ns_errlist)/sizeof(ns_errlist[0]) };

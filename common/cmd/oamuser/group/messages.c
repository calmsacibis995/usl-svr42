/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:group/messages.c	1.2.11.2"
#ident  "$Header: messages.c 2.0 91/07/13 $"

char *errmsgs[] = {
	":0:gid %ld is reserved.\n",
	":0:invalid syntax.\nusage:  groupadd [-g gid [-o]] group\n",
	":0:invalid syntax.\nusage:  groupdel group\n",
	":0:invalid syntax.\nusage:  groupmod -g gid [-o] | -n name group\n",
	":0:Cannot update system files - group cannot be %s.\n",
	":0:%s is not a valid group id.  Choose another.\n",
	":0:%s is already in use.  Choose another.\n",
	":0:%s is not a valid group name.  Choose another.\n",
	":0:%s does not exist.\n",
	":0:Group id %ld is too big.  Choose another.\n"
};

int lasterrmsg = sizeof( errmsgs ) / sizeof( char * );

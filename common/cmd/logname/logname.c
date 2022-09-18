/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)logname:logname.c	1.4.1.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/logname/logname.c,v 1.1 91/02/28 17:45:21 ccs Exp $"

#include <stdio.h>
main() {
	char *name, *cuserid();

	name = cuserid((char *)NULL);
	if (name == NULL)
		return (1);
	(void) puts (name);
	return (0);
}

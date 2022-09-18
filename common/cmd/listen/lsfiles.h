/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)listen:lsfiles.h	1.3.5.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/listen/lsfiles.h,v 1.1 91/02/28 17:43:47 ccs Exp $"

/*
 *	lsfiles.h:	listener files stuff
 */


/*
 * initialization constants for file checking/usage
 */

#define LOGOFLAG	(O_WRONLY | O_APPEND | O_CREAT)
#define LOGMODE		(0666)

#define PIDOFLAG	(O_WRONLY | O_CREAT)
#define PIDMODE		(0644)

#define NETOFLAG	(O_RDWR)


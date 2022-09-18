/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libsocket:common/lib/libsocket/socket/callselect.c	1.1.2.3"
#ident "$Header: 1.1 91/02/28 $"

/*
 * dummy routine to force select() to be linked from archive libc
 */
callselect()
{
	select(0, 0, 0, 0, 0);
}

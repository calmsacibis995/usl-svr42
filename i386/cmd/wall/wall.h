/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _WALL_H
#define _WALL_H

#ident	"@(#)wall:i386/cmd/wall/wall.h	1.1.1.2"
#ident  "$Header: wall.h 1.1 91/03/13 $"

/*
 * describes tty specific behavior for i386 machines
 */

#define insert_cr(ptr)	if (*ptr == '\r')   *(ptr++) = '\r'

#define BRCMSGTG ":582:\r\n\07\07\07Broadcast Message from %s (%s) on %s %19.19s to group %s...\r\n"
#define BRCMSG   ":583:\r\n\07\07\07Broadcast Message from %s (%s) on %s %19.19s...\r\n"
#define DEBUGMSG "DEBUG: To %.8s on %s\r\n"

#endif /* _WALL_H */

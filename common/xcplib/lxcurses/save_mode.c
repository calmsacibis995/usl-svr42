/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)xcplxcurses:save_mode.c	1.1.2.2"
#ident  "$Header: save_mode.c 1.1 91/07/09 $"

/*
 *	$Header: save_mode.c 1.1 91/07/09 $
 */

/*
 * save and restore tty modes for curses
 */
#include <termio.h>

static struct termio save_tty;


/*
 * save mode, called from initscr()
 */
save_mode()
{
	ioctl(0, TCGETA, &save_tty);
}
	
/*
 * reset mode, called from endwin()
 */
rest_mode()
{
	ioctl(0, TCSETA, &save_tty);
}

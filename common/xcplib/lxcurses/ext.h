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

#ident	"@(#)xcplxcurses:ext.h	1.1.2.2"
#ident  "$Header: ext.h 1.1 91/07/09 $"

/*
 *	$Header: ext.h 1.1 91/07/09 $
 */
/*	curses.ext	1.3	83/07/02	*/

/*
 * External variables for the curses library
 */

/* LINTLIBRARY */

# include	"xcurses.h"

extern bool	_echoit, _rawmode, My_term, _endwin;

extern char	ttytype[], *_unctrl[];

extern int	_tty_ch, LINES, COLS;

extern SGTTY	_tty;

char		_putchar();

#ifdef DEBUG
# define	outf	_outf

FILE		*outf;
#endif

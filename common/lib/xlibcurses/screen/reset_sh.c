/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/reset_sh.c	1.5.2.2"
#ident  "$Header: reset_sh.c 1.2 91/06/26 $"
#include	"curses_inc.h"

reset_shell_mode()
{
#ifdef	DIOCSETT
    /*
     * Restore any virtual terminal setting.  This must be done
     * before the TIOCSETN because DIOCSETT will clobber flags like xtabs.
     */
    cur_term -> old.st_flgs |= TM_SET;
    (void) ioctl(cur_term->Filedes, DIOCSETT, &cur_term -> old);
#endif	/* DIOCSETT */
    if (_BR(SHELLTTY))
    {
	(void) ioctl(cur_term -> Filedes,
#ifdef	SYSV
	    TCSETAW,
#else	/* SYSV */
	    TIOCSETN,
#endif	/* SYSV */
		    &SHELLTTY);
#ifdef	LTILDE
	if (cur_term -> newlmode != cur_term -> oldlmode)
	    (void) ioctl(cur_term -> Filedes, TIOCLSET, &cur_term -> oldlmode);
#endif	/* LTILDE */
    }
    return (OK);
}

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/resetty.c	1.5.2.2"
#ident  "$Header: resetty.c 1.2 91/06/26 $"

#include	"curses_inc.h"

resetty()
{
    if ((_BR(SP->save_tty_buf)) != 0)
    {
	PROGTTY = SP->save_tty_buf;
#ifdef	DEBUG
	if (outf)
#ifdef	SYSV
	    fprintf(outf, "resetty(), file %x, SP %x, flags %x, %x, %x, %x\n",
		cur_term->Filedes, SP, PROGTTY.c_iflag, PROGTTY.c_oflag,
		PROGTTY.c_cflag, PROGTTY.c_lflag);
#else	/* SYSV */
	    fprintf(outf, "resetty(), file %x, SP %x, flags %x\n",
		cur_term->Filedes, SP, PROGTTY.sg_flags);
#endif	/* SYSV */
#endif	/* DEBUG */
	reset_prog_mode();
    }
    return (OK);
}

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/ws/ws_ansi.c	1.3"
#ident	"$Header: $"


#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>
#include <svc/errno.h>
#include <util/inline.h>
#include <io/tty.h>
#include <io/termio.h>
#include <util/cmn_err.h>
#include <io/ws/vt.h>
#include <io/ansi/at_ansi.h>
#include <io/ascii.h>
#include <io/kd/kd.h>
#include <io/stream.h>
#include <io/strtty.h>
#include <io/stropts.h>
#include <proc/proc.h>
#include <io/xque/xque.h>
#include <io/ws/ws.h>

/*
 *
 */

wsansi_parse(wsp, chp, addr, cnt)
wstation_t	*wsp;
channel_t	*chp;
unchar	*addr;
int	cnt;
{
	termstate_t	*tsp = &chp->ch_tstate;
	register ushort	ch; 
	register int	i;

	while (cnt--) {
		ch = *addr++ & 0xFF; 
		if (ch == A_ESC || ch == A_CSI || (ch < ' ' && tsp->t_font == ANSI_FONT0))
			wsansi_cntl(wsp, chp, tsp, ch);
		else
			tcl_norm(wsp, chp, tsp, ch);
	}  /* while (cnt--) */
}

/*
 *
 */

wsansi_cntl(wsp, chp, tsp, ch)
wstation_t	*wsp;
channel_t	*chp;
termstate_t	*tsp;
ushort	ch;
{
	switch (ch) {
	case A_BEL:
		(*wsp->w_bell)(wsp, chp);
		break;
	case A_BS:
		tcl_bs(wsp, chp, tsp);
		break;
	case A_HT:
		tcl_ht(wsp, chp, tsp);
		break;
	case A_NL:
	case A_VT:
		if (tsp->t_row == tsp->t_rows - 1) {
			tsp->t_ppar[0] = 1;
			tcl_scrlup(wsp, chp, tsp);
		} else {
			tsp->t_row++;
			tsp->t_cursor += tsp->t_cols;
		}
		(*wsp->w_setcursor)(chp, tsp);
		break;
	case A_FF:
		tcl_reset(wsp, chp, tsp);
		break;
	case A_CR:
		tsp->t_cursor -= tsp->t_col;
		tsp->t_col = 0;
		(*wsp->w_setcursor)(chp, tsp);
		break;
	case A_GS:
		tcl_bht(wsp, chp, tsp);
		break;
	default:
		break;
	}
}

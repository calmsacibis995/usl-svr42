#ident	"@(#)xpr:devices/terminfo/ti_motion.c	1.3"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#ifdef __STDC__
#include <errno.h>	/* Stop STDC from giving empty translation */
#endif			/* unit warning */
#if	defined(DO_MOTION)

#include "xpr_term.h"

extern char		*tparm();

static int		left_margin	= -1,
			right_margin	= -1,
			last_hpos	= -1,
			last_vpos	= -1;


/*
 * NOTE: THESE ROUTINES EXPECT ARGUMENTS IN DOTS, NOT COLUMNS, LINES,
 * OR MICRO STEPS.
 *
 * These routines aren't used often, so there is very little optimization.
 */

static int		device_scale	= 1;

/**
 ** set_motion_scale() - RECORD SIZE OF DOTS
 ** dot_to_col() - CONVERT DOTS TO COLUMNS
 ** dot_to_mcol() - CONVERT DOTS TO MICRO STEPS (HORIZONTAL)
 ** dot_to_row() - CONVERT DOTS TO ROWS
 ** dot_to_mrow() - CONVERT DOTS TO MICRO STEPS (VERTICAL)
 ** col_to_dot() - CONVERT COLUMNS TO DOTS
 ** row_to_dot() - CONVERT ROWS TO DOTS
 **/

void			set_motion_scale (scale)
	int			scale;
{
	/*
	 * "scale" is the number of PIN steps per dot, both
	 * horizontally and vertically. Depending on the device,
	 * these MAY OR MAY NOT be the same as the micro steps,
	 * and very likely aren't columns and rows.
	 */
	device_scale = scale;
	return;
}

/*
 * The conversion routines return type "double", to allow proper
 * truncation or rounding, depending on the need.
 */

#define dot_to_col(X)	((double)(X * device_scale * orhi)/(orc * spinh))
#define dot_to_row(Y)	((double)(Y * device_scale * orvi)/(orl * SPINV))
#define dot_to_mcol(X)	((double)(X * device_scale * orhi)/spinh)
#define dot_to_mrow(Y)	((double)(Y * device_scale * orvi)/SPINV)
#define col_to_dot(C)	((double)(C * orc * spinh)/(device_scale * orhi))
#define row_to_dot(R)	((double)(R * orl * SPINV)/(device_scale * orvi))

/**
 ** set_lrmargins()
 **/

int			set_lrmargins (left, right)
	int			left,
				right;
{
	int			left_col,
				right_col;

	char			*mgc,
				*smglp,
				*smgrp,
				*set_both_margins,
				*smgl,
				*smgr;


	/*
	 * Convert the arguments to columns.
	 */
	left_col = floor(dot_to_col(left));
	right_col = ceil(dot_to_col(right));

	/*
	 * Clear the current margins.
	 */
	tidbit ((char *)0, "mgc", &mgc);
	if (OKAY(mgc))
		putp (mgc);

	/*
	 * Try the parameterized capabilities.
	 */

	tidbit ((char *)0, "smglp", &smglp);
	tidbit ((char *)0, "smgrp", &smgrp);

	if (OKAY(smglp) && !OKAY(smgrp))
		set_both_margins = smglp;
	else if (OKAY(smgrp) && !OKAY(smglp))
		set_both_margins = smgrp;
	else
		set_both_margins = 0;

	if (set_both_margins) {
		putp (tparm(set_both_margins, left_col, right_col));
		goto CalcMargins;
	}
	if (OKAY(smglp) && OKAY(smgrp)) {
		putp (tparm(smglp, left_col));
		putp (tparm(smgrp, right_col));

CalcMargins:	/*
		 * We remember where the margins are, measured in dots.
		 * Since the column spacing probably didn't land on
		 * a dot, these aren't necessarily "left" and "right".
		 */
		left_margin = col_to_dot(left_col);
		right_margin = col_to_dot(right_col);
		return (0);
	}

	/*
	 * Try the ``margin here'' capabilities.
	 *
	 * WARNING: We (may) need a new Terminfo capability (how does
	 * ``margin here'' deal with being between column/row boundaries)
	 * that wasn't present at the time of this coding. If the cap is
	 * added, remove the conditional directives. Note: I don't know
	 * if this cap is REALLY needed, but it comes to mind.
	 */

	tidbit ((char *)0, "smgl", &smgl);
	tidbit ((char *)0, "smgr", &smgr);

	if (OKAY(smgl) && OKAY(smgr)) {
#if	defined(HAVE_MARGIN_HERE_BEHAVIOR)
		short			mgrnd;


		tidbit ((char *)0, "mgrnd", &mgrnd);
		switch (mgrnd) {

		case -1:
		case 0:	/* margin stays here */
#endif
			hmove (left);
			putp (smgl);
			left_margin = last_hpos;
			hmove (right);
			putp (smgr);
			right_margin = last_hpos;
#if	defined(HAVE_MARGIN_HERE_BEHAVIOR)
			break;

#define	dot_to_dotcol(X)	col_to_dot((int)dot_to_col(X));

		case 1: /* margin floats to next column */
			hmove (left - (col_to_dot(1) - 1));
			putp (smgl);
			left_margin = dot_to_dotcol(last_hpos);
			hmove (right);
			putp (smgr);
			right_margin = dot_to_dotcol(last_hpos);
			break;

		case 2:	/* margin floats to previous column */
			hmove (left);
			putp (smgl);
			left_margin = dot_to_dotcol(last_hpos);
			hmove (right + (col_to_dot(1) - 1));
			putp (smgr);
			right_margin = dot_to_dotcol(last_hpos);
			break;
		}
#endif
		return (0);
	}

	/*
	 * Getting here means that we can't set the margins:
	 * Let the caller know.
	 */
	return (-1);

}

/**
 ** reset_hmove() - RESET KNOWLEDGE OF CARRIAGE POSITION
 ** hmove() - MOVE TO HORIZONTAL POSITION
 **/

void			reset_hmove (new_pos)
	int			new_pos;
{
	last_hpos = new_pos;
	return;
}

void			hmove (x)
	int			x;
{
	static char		*cr,
				*cuf,
				*cub,
				*cuf1,
				*cub1,
				*hpa,
				*rep;

	char			*abs	= 0,
				*rel	= 0,
				*step	= 0;

	int			col,
				delta;


/*
 * MORE: Include micro motion caps to get fine-grain motion.
 */

	if (!OKAY(cr)) {
		tidbit ((char *)0, "cr", &cr);
		if (!OKAY(cr))
			cr = "\r";
		tidbit((char *)0, "cuf", &cuf);
		tidbit((char *)0, "cub", &cub);
		tidbit((char *)0, "cuf1", &cuf1);
		tidbit((char *)0, "cub1", &cub1);
		tidbit((char *)0, "hpa", &hpa);
		tidbit((char *)0, "rep", &rep);
		left_margin = 0;
		right_margin = col_to_dot(cols) - 1;
	}

	if (x == last_hpos)
		return;

	if (x < left_margin || right_margin < x)
		return;

	if (x == left_margin) {
		putp (cr);
		return;
	}

	col = dot_to_col(x);

	if ((delta = col - dot_to_col(last_hpos)) < 0) {
		delta = -delta;
		if (OKAY(cub))
			rel = tparm(cub, delta);
		step = cub1;
	} else {
		if (OKAY(cuf))
			rel = tparm(cuf, delta);
		if (!OKAY(rel) && OKAY(rep))
			rel = tparm(rep, ' ', delta);
		if (!OKAY(step = cuf1))
			step = " ";
	}

	if (OKAY(rel))
		putp (rel);
	else if ((abs = tparm(hpa, col)) && *abs)
		putp (abs);
	else if (OKAY(step)) {
		int			i;

		for (i = 0; i < delta; i++)
			putp (step);
	}

	last_hpos = col_to_dot(col);
	return;
}

/**
 ** reset_vmove() - RESET KNOWLEDGE OF CARRIAGE POSITION (VERTICAL)
 ** vmove() - MOVE TO VERTICAL POSITION
 **/

void			reset_vmove (new_pos)
	int			new_pos;
{
	last_vpos = new_pos;
	return;
}

void			vmove (y)
	int			y;
{
	static char		*cud,
				*cuu,
				*cud1,
				*cuu1,
				*vpa;

	static int		have_done_tidbit	= 0;

	char			*abs	= 0,
				*rel	= 0,
				*step	= 0;

	int			row,
				delta;


	if (!have_done_tidbit) {
		tidbit((char *)0, "cud", &cud);
		tidbit((char *)0, "cuu", &cuu);
		tidbit((char *)0, "cud1", &cud1);
		tidbit((char *)0, "cuu1", &cuu1);
		tidbit((char *)0, "vpa", &vpa);
		have_done_tidbit = 1;
	}

	if (y == last_vpos)
		return;

	if (y < 0 || row_to_dot(lines) <= y)
		return;

	row = dot_to_row(y);

	if ((delta = row - dot_to_row(last_vpos)) < 0) {
		delta = -delta;
		if (OKAY(cuu))
			rel = tparm(cuu, delta);
		step = cuu1;
	} else {
		if (OKAY(cud))
			rel = tparm(cud, delta);
		step = cud1;
	}

	if (OKAY(rel))
		putp (rel);
	else if ((abs = tparm(vpa, row)) && *abs)
		putp (abs);
	else if (OKAY(step)) {
		int			i;

		for (i = 0; i < delta; i++)
			putp (step);
	}

	last_vpos = row_to_dot(row);
	return;
}

#endif	/* DO_MOTION */

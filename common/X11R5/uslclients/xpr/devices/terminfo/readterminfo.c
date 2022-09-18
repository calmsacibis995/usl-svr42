#ident	"@(#)xpr:devices/terminfo/readterminfo.c	1.4"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "stdio.h"

#include "xpr_term.h"

short			cols,
			lines,
			orc,
			orhi,
			orl,
			orvi,
			npins,
			spinh,
			spinv,	/* pin density */
			SPINV,	/* dot density (spinv * bitwin) */
			bitwin,
			bitype,
			colors,
			hls;

char			*ff,
			*porder,
			*sbim,
			*rbim,
			*defbi,
			*endbi,
			*birep,
			*binel,
			*bicr,
			*initc,
			*setcolor,
			*colornm;

/**
 ** read_terminfo_database()
 **/

void			read_terminfo_database (TERM)
	char			*TERM;
{
	tidbit (TERM, "cols", &cols);
	tidbit (TERM, "lines", &lines);
	tidbit (TERM, "ff", &ff);

	tidbit (TERM, "orc", &orc);
	tidbit (TERM, "orhi", &orhi);
	tidbit (TERM, "orl", &orl);
	tidbit (TERM, "orvi", &orvi);

	tidbit (TERM, "npins", &npins);
	tidbit (TERM, "spinv", &spinv);
	tidbit (TERM, "spinh", &spinh);
	tidbit (TERM, "sbim", &sbim);
	tidbit (TERM, "rbim", &rbim);
	tidbit (TERM, "porder", &porder);

	tidbit (TERM, "colors", &colors);

	/*
	 * Some of the Terminfo capabilities have not been defined
	 * at the time of this coding. Thus, we first check to see
	 * if Terminfo knows about one of them. If it doesn't, then
	 * look in the ``user-reserved'' strings for the values.
	 */
	if (
		/*
		 * First, do *we* know about the new caps?
		 */
		tidbit(TERM, "bitype", (short *)0) == 2
		/*
		 * Second, does the database have a reasonable value
		 * for a required cap?
		 */
	     && (bitype = tidbit_number) != -1
	) {
		
		tidbit (TERM, "bitwin", &bitwin);
/*		tidbit (TERM, "bitype", &bitype);	*/
		tidbit (TERM, "birep", &birep);
		tidbit (TERM, "binel", &binel);
		tidbit (TERM, "defbi", &defbi);
		tidbit (TERM, "endbi", &endbi);
		tidbit (TERM, "bicr", &bicr);
		tidbit (TERM, "initc", &initc);
		tidbit (TERM, "hls", &hls);
		tidbit (TERM, "setcolor", &setcolor);
		tidbit (TERM, "colornm", &colornm);
	} else {
		tidbit (TERM, "u1", (char *)0);
		bitwin = atoi(tidbit_string);

		tidbit (TERM, "u2", (char *)0);
		bitype = atoi(tidbit_string);

		tidbit (TERM, "u4", &birep);
		tidbit (TERM, "u5", &binel);
		tidbit (TERM, "u6", &defbi);
		tidbit (TERM, "u7", &endbi);

		tidbit (TERM, "u3", &bicr);
		tidbit (TERM, "initc", &initc);
		tidbit (TERM, "hls", &hls);
		setcolor = initc;
		tidbit (TERM, "u8", &colornm);
	}


	if (
		!OKAY(porder)
	     || !OKAY(defbi) && !OKAY(sbim)
	     || spinv == -1
	     || spinh == -1
	     || npins == -1
	) {
		fprintf (
			stderr,
	"xpr: Error: The device \"%s\" doesn't have a graphics capability.\n",
			TERM
		);
		exit (1);
	}

#if	!defined(DO_MOTION)
	if (!OKAY(defbi)) {
		fprintf (
			stderr,
"\
xpr: Error: This version can't handle the device \"%s\":\n\
            there is no \"defbi\" Terminfo capability for it.\n\
",
			TERM
		);
		exit (1);
	}
#endif

	if (colors > MAX_COLORS) {
		fprintf (
			stderr,
    "xpr: Warning: Using only %d of the %d printer colors available.\n",
			MAX_COLORS,
			colors
		);
		colors = MAX_COLORS;
	}
	if (colors < 0)
		colors = 0;

	if (bitwin < 1)
		bitwin = 1;

	/*
	 * The "spinv" capability is the number of PINS per inch
	 * in the print head. We need the dot density instead.
	 */
	SPINV = spinv * bitwin;

	return;
}

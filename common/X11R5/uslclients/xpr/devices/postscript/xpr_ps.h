#ident	"@(#)xpr:devices/postscript/xpr_ps.h	1.7"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */


/*
 * Page size, in inches.
 */
#define PS_PAGE_WIDTH	8.0
#define PS_PAGE_HEIGHT	11.0

/*
 * Dots per inch.
 */
#define PS_DPI		300

/*
 * Units per inch in the default user coordinate system.
 * The Red Book says one unit is ~ 1 point, or 1/72 inch.
 */
#define PS_USER_SPACE_UPI	72

/*
 * Total space to leave above/below image, for header/trailer,
 * in inches.
 */
#define PS_TEXT_ADJUST	(TEXT_GAP + TEXT_PT / (double)72)

/*
 * Font used for header and trailer.
 */
#define PS_TEXT_FONT	"Times-Roman"

/*
 * Various PostScript code fragments. These are used as follows:
 *
 *	PS_PAGE_START
 *	PS_SET_ORIGIN
 *	PS_TRANSFER_START
 *	PS_TRANSFER_ITEM
 *	PS_TRANSFER_END
 *	PS_TRANSFER_INVERT
 *	PS_PIXDUMP
 *	<data>
 *	PS_PAGE_END
 *	PS_PAGE_START
 *	.
 *	.
 *	.
 *	PS_PAGE_END
 *
 */

#define PS_SET_ORIGIN		"%d %d translate\n"

#define PS_TRANSFER_START	"\n{ [\n"
#define PS_TRANSFER_ITEM	"    %5.3f\n"
#define PS_TRANSFER_END		"  ] exch .001 sub %d mul cvi get %s} settransfer\n"
#define PS_TRANSFER_INVERT	"1 exch sub "

#define PS_PAGE_START		"\n%%%%Page ? %d\nsave\n72 300 div dup scale\n"
#define PS_PAGE_END		"showpage\nrestore\n"

#define PS_PIXDUMP		"\n%d %d %d %d pixdump\n"

/*
 * PostScript epilogue and trailer.
 */
#define PS_EPILOGUE	PS_PAGE_END
#define	PS_TRAILER	"%%Trailer\n"
#define PS_BOUNDINGBOX	"%%BoundingBox:"
#define PS_BB_FORMAT	"%s %ld %ld %ld %ld\n"
#define PS_PAGES	"%%Pages:"
#define PS_PAGES_FORMAT	"%s %d\n"

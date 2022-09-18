#ident	"@(#)xpr:devices/postscript/prologue.c	1.3"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "signal.h"
#include "stdio.h"
#include "termio.h"
#include "pwd.h"
#include "sys/types.h"

#include "xpr.h"

#include "xpr_ps.h"

static long		bb_ll_x = PS_PAGE_WIDTH * PS_USER_SPACE_UPI;
static long		bb_ll_y = PS_PAGE_HEIGHT * PS_USER_SPACE_UPI;
static long		bb_ur_x = 0;
static long		bb_ur_y = 0;

static char		prologue_docfont[] = "%%%%DocumentFonts: %s\n";
static char		prologue_title[]   = "%%%%Title: %s\n";
static char		prologue_creator[] = "%%%%Creator: %s\n";
static char		prologue_date[]	   = "%%%%CreationDate: %s";

static char *		prologue_ps[] = {
"%!PS-Adobe-\n",
prologue_docfont,
prologue_title,
prologue_creator,
prologue_date,
"%%Pages: (atend)\n",
"%%BoundingBox: (atend)\n",
"%%EndComments\n",
"\n",
"/getcnt\n",
"{\n",
"	/hex2 2 string def\n",
"	currentfile hex2 readhexstring pop pop\n",
"	hex2 0 get 256 mul hex2 1 get add\n",
"} def\n",
"\n",
"/pixgen\n",
"{\n",
"	% The data is packed into fields:\n",
"	%	rHHHHXX		% repeat byte XX HHHH times\n",
"	%	dHHHHXXXX...XX	% data bytes XXXX...XX (HHHH in length)\n",
"	% where HHHH is a 4-byte hex value, and XX is a 2-byte hex datum.\n",
"\n",
"	/firstchar 1 string def\n",
"	{\n",
"		currentfile firstchar readstring not { exit } if\n",
"		(rd) exch search { pop pop pop exit } if\n",
"		pop\n",
"	} loop\n",
"\n",
"	firstchar (r) eq {\n",
"		/cnt getcnt def\n",
"		/hex1 1 string def\n",
"		currentfile hex1 readhexstring pop pop\n",
"		0 1 cnt 1 sub {\n",
"			picstr exch hex1 putinterval\n",
"		} for\n",
"		picstr 0 cnt getinterval\n",
"	} if\n",
"\n",
"	firstchar (d) eq {\n",
"		currentfile getcnt string readhexstring pop\n",
"	} if\n",
"} def\n",
"\n",
"/pixdump\n",
"{\n",
"	% The stack should contain: width, height, iscale, bits-per-pixel\n",
"	/bpp exch def\n",
"	/iscale exch def\n",
"	/height exch def\n",
"	/width exch def\n",
"\n",
"	width iscale mul height iscale mul scale\n",
"\n",
"	% this string length is overkill for bits-per-sample < 8,\n",
"	% but no harm is done.\n",
"	/picstr width string def\n",
"\n",
"	width height bpp [width 0 0 height neg 0 height] { pixgen } image\n",
"} def\n",
"\n",
"%%EndProlog\n",
0
};

/**
 ** ps_prologue() - PUT OUT PROLOGUE (GENERAL INITIALIZATION SEQUENCES)
 **/

void			ps_prologue (fp)
	FILE			*fp;
{
	char **			line;


	for (line = prologue_ps; *line; line++) {
		if (*line == prologue_docfont)
			fprintf (fp, *line, PS_TEXT_FONT);
		else if (*line == prologue_title)
			fprintf (fp, *line, input_filename);
		else if (*line == prologue_creator) {
			struct passwd		*pe = getpwuid(getuid());

			fprintf (fp, *line, (pe? pe->pw_name : "(unknown)"));
		} else if (*line == prologue_date) {
			time_t			now = time((long *)0);

			fprintf (fp, *line, ctime(&now));
		} else
			fputs (*line, fp);
	}
	return;
}

/**
 ** ps_epilogue() - PUT OUT EPILOGUE (CLOSING CONTROL SEQUENCES)
 **/

void			ps_epilogue (fp, x, y, width, height, scale, pages)
	FILE			*fp;
	int			x;
	int			y;
	int			width;
	int			height;
	int			scale;
	int			pages;
{
	long			ll_x;
	long			ll_y;
	long			ur_x;
	long			ur_y;
	long			page_height;


	/*
	 * The coordinates--x,y,width,height--are in ``image'' units.
	 * The scale parameter gives the size of an image unit in
	 * dots-per-inch on the PostScript printer; PS_DPI is the
	 * macro that gives the expected PostScript printer resolution.
	 *
	 * The x,y values are also measured from an upper-left
	 * origin; the bounding box has a lower-left origin. The height
	 * of a PostScript page is assumed to be PS_PAGE_HEIGHT inches.
	 *
	 * The bounding box information must be in ``default user
	 * coordinates'' which has a unit of 1/72 inch (or ~ 1 point).
	 * (The macro PS_USER_SPACE_UPI defines the number of these
	 * units per inch, just in case it needs to change.)
	 *
	 * Using the conversion of x as an example and a value for PS_DPI
	 * of 300 (typical):
	 *
	 *   x * scale   is a number in 1/300's (of an inch),
	 *               we need it in 1/72's (of an inch).
	 *
	 *    72 1/72's
	 *   ----------- = 1 (unit-less)
	 *   300 1/300's
	 *
	 * Therefore,
	 *
	 *                72 1/72's
	 *   x * scale * ----------- = (x * scale * 72) / 300
	 *               300 1/300's
	 *
	 * This result will be in 1/72's units.
	 */
#define CONVERT(V,SCALE) (((V) * SCALE * PS_USER_SPACE_UPI) / PS_DPI)
	ll_x = CONVERT(x, scale);
	ur_x = CONVERT(x + width, scale);
	page_height = PS_PAGE_HEIGHT * PS_USER_SPACE_UPI;
	ll_y = page_height - CONVERT(y + height, scale);
	ur_y = page_height - CONVERT(y, scale);
#undef	CONVERT

	if (ll_x < bb_ll_x)
		bb_ll_x = ll_x;
	if (ll_y < bb_ll_y)
		bb_ll_y = ll_y;
	if (ur_x > bb_ur_x)
		bb_ur_x = ur_x;
	if (ur_y > bb_ur_y)
		bb_ur_y = ur_y;

	fprintf (fp, "%s", PS_EPILOGUE);
	fprintf (fp, "%s", PS_TRAILER);
	fprintf (
		fp,
		PS_BB_FORMAT,
		PS_BOUNDINGBOX,
		bb_ll_x,
		bb_ll_y,
		bb_ur_x,
		bb_ur_y
	);
	fprintf (fp, PS_PAGES_FORMAT, PS_PAGES, pages);

	return;
}

/**
 ** ps_backup_epilogue() - BACKUP OVER PREVIOUS EPILOGUE IN A FILE
 **/

void			ps_backup_epilogue (fp, ppages, noff)
	FILE *			fp;
	int *			ppages;
	int			noff;
{
	int			len;
	int			new_loc;

	char *			last_bytes;

	char			line[BUFSIZ];


	/*
	 * If the ``file'' isn't seekable, then it isn't capable of
	 * being changed.
	 */
	if (fseek(fp, 0L, 2) != 0)
		return;

	/*
	 * Search for the trailer, then back up over the
	 * ``epilogue''.
	 */
	rewind (fp);
 	while ((new_loc = ftell(fp), fgets(line, BUFSIZ, fp)))
		if (STREQU(line, PS_TRAILER))
			break;
	if (feof(fp) || ferror(fp))
		return;

	/*
	 * Now search for the bounding box line, and parse the
	 * values from it.
	 */
	len = strlen(PS_BOUNDINGBOX);
	do
		if (STRNEQU(line, PS_BOUNDINGBOX, len))
			break;
	while (fgets(line, BUFSIZ, fp));
	if (!feof(fp) && !ferror(fp)) {
		char			*toss	= PS_BOUNDINGBOX;

		sscanf (
			line,
			PS_BB_FORMAT,
			&toss,
			&bb_ll_x,
			&bb_ll_y,
			&bb_ur_x,
			&bb_ur_y
		);
	} else {
		bb_ll_x = PS_PAGE_WIDTH;
		bb_ll_y = PS_PAGE_HEIGHT;
		bb_ur_x = 0;
		bb_ur_y = 0;
	}

	/*
	 * Now search for the number of pages and get its value.
	 */
	len = strlen(PS_PAGES);
	do
		if (STRNEQU(line, PS_PAGES, len))
			break;
	while (fgets(line, BUFSIZ, fp));
	if (!feof(fp) && !ferror(fp)) {
		char			*toss	= PS_PAGES;

		sscanf (
			line,
			PS_PAGES_FORMAT,
			&toss,
			ppages
		);
		if (noff)
			(*ppages)--;
	} else
		*ppages = 0;

	if (!noff)
		fseek (fp, new_loc, 0);
	else {
		len = strlen(PS_EPILOGUE);
		new_loc -= len;
		if (new_loc >= 0) {
			last_bytes = Malloc(len);
			fseek (fp, new_loc, 0);
			if (
				fread(last_bytes, len, 1, fp) == 1
			     && memcmp(last_bytes, PS_EPILOGUE, len) == 0
			)
				fseek (fp, new_loc, 0);
			/* else
				the "fread()" moved pointer back to eof */
		}
	}

	return;
}

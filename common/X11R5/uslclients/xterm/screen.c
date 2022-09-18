/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:screen.c	1.2.1.41"
#endif

/*
 screen.c (C source file)
	Acc: 601052383 Tue Jan 17 09:59:43 1989
	Mod: 601054125 Tue Jan 17 10:28:45 1989
	Sta: 601054125 Tue Jan 17 10:28:45 1989
	Owner: 7007
	Group: 1985
	Permissions: 666
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/

/* Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts. */

#include <X11/copyright.h>

#ifndef lint
static char rcs_id[] = "$Header: screen.c,v 1.1 88/02/10 13:08:12 jim Exp $";
#endif	/* lint */

#include <X11/Xlib.h>
#include <X11/Xatom.h>		/* SS-hints */
#include <stdio.h>
#include <signal.h>
#include "ptyx.h"
#include "error.h"

#include <sys/termio.h>		/* for TIOCSWINSZ */
#ifndef SVR4
#include <sys/stream.h>
#include <sys/ptem.h>
#endif /* SVR4 */

/* SS-color */
#include "data.h"
#include "xterm.h"
/* SS-color-end */
#include <Xol/OpenLook.h>

#include "Strings.h"
#include "messages.h"

extern Widget toplevel;

#ifndef MEMUTIL
extern char *calloc();
extern char *malloc();
extern char *realloc();
#endif
extern void bcopy();
#ifndef MEMUTIL
extern void free();
#endif

/* SS-color */
Char PixelIndex();
/* SS-color-end */

/* Allocate nrow lines.  Each line is formed by 4 of Char arrays.	*/
/* The 1st array contains the actual characters;			*/
/* The 2nd array contains video attributes information			*/
/* The 3rd array contains color information (indexes into Pixels array) */
/* The 4th array contains I18N information: EUC codeset and Multy_byte  */

ScrnBuf Allocate (nrow, ncol)
register int nrow, ncol;
{
	register ScrnBuf base;
	register Char *tmp;
	register int i;

	if ((base = (ScrnBuf) calloc ((unsigned)(nrow *= 4), sizeof (char *))) == 0)
		SysError (ERROR_SCALLOC);

	if ((tmp = (Char *) calloc ((unsigned) (nrow * ncol),
				    sizeof(Char))) == 0)
		SysError (ERROR_SCALLOC2);

	/* *addr = tmp; */
	for (i = 0; i < nrow; i++, tmp += ncol)
		base[i] = tmp;
	return (base);
}


/*
 *  This is called when the screen is resized. Not complex if you do
 *  things in the right order...
 */
static void
Reallocate(sbuf, nrow, ncol, oldrow, oldcol, fg, bg)
ScrnBuf *sbuf;
int nrow, ncol, oldrow, oldcol;
Pixel fg, bg;
{
	register ScrnBuf base;
	register Char *tmp;
	register int i, minrows, mincols;
	Char *oldbuf;
	
	if (sbuf == NULL || *sbuf == NULL)
		return;

	oldbuf = (*sbuf)[0];
	oldrow *= 4;

	/* 
	 *  realloc sbuf; we don't care about losing the lower lines if the
	 *  screen shrinks. It might be cleaner to readjust the screen so
	 *  that the UPPER lines vanish when the screen shrinks but that's
	 *  more work...
	 */

	nrow *= 4;
	*sbuf = (ScrnBuf) realloc ((char *) (*sbuf),
				   (unsigned) (nrow * sizeof(char *)));
	if (*sbuf == 0)
		SysError(ERROR_RESIZE);
	base = *sbuf;

	/* 
	 *  create the new buffer space and copy old buffer contents there
	 *  line by line, updating the pointers in sbuf as we go; then free
	 *  the old buffer
	 */
	if ((tmp = (Char *) calloc((unsigned) (nrow * ncol), sizeof(Char))) == 0)
		SysError(ERROR_SREALLOC);
	/* *sbufaddr = tmp; */
	minrows = (oldrow < nrow) ? oldrow : nrow;
	mincols = (oldcol < ncol) ? oldcol : ncol;
	for(i = 0; i < minrows; i++, tmp += ncol) {
		bcopy(base[i], tmp, mincols);
		base[i] = tmp;
	}
	if (oldrow < nrow) {
		for (i = minrows; i < nrow; i++, tmp += ncol)
			base[i] = tmp;
	}

	/* Now free the old buffer - simple, see... */
	free(oldbuf);
}


/* SS-color */

/* added fg and bg */
ScreenWrite (screen, str, flags, textfg, textbg, length)
/* SS-color-end */
/*
   Writes str into buf at row row and column col.  Characters are set to match
   flags.
 */
TScreen *screen;
Char *str;
register unsigned flags;
/* SS-color */
register Pixel textfg, textbg;
/* SS-color-end */
int length;		/* length of string */
{
	register Char *col, *attr;
	register int  i, avail  = screen->max_col - screen->cur_col + 1;
/* SS-inter */
	OlStrSegment  *segment = &(screen->segment);
	register Char *euc;
	OlFontList    *fontl = term->primitive.font_list;
	Boolean	      stop_parsing = FALSE;
	register int  len, code_set, code_width, total_len = 0;
/* SS-inter-end */

	if (length <= 0)
		return;

	col  = screen->buf[i = 4 * screen->cur_row] + screen->cur_col;
	attr = screen->buf[i + 1] + screen->cur_col;
	euc  = screen->buf[i + 3] + screen->cur_col;
	flags &= ATTRIBUTES;

/* SS-inter */

	/* if font list is not specified, we can simply copy the string */
	/* into the screen buffer, and not worry about the euc field	*/

	if (!fontl) {
	    bcopy(str, col, length);
	    total_len = length;
	}

	/* examine every byte, to determine the codeset and save codeset */
	/* information in the euc field					 */

	else while (length > 0 && !stop_parsing) {

	   /* the following call will advance the "str" pointer and  	*/
	   /* decrement the "length"					*/

	   if (OlGetNextStrSegment (fontl, segment, &str, &length) == -1) {
#if !defined(I18N)
	       printf ("ERROR");
	       exit(-1);
#else
		OlVaDisplayErrorMsg(screen->display, OleNolGetnextstrsegment,
			OleTbadOlgetnextstrsegment,
			OleCOlClientXtermMsgs,
			OleMolGetnextstrsegment_badOlgetnextstrsegment, NULL);
#endif
	   }
	   len        = segment->len;
	   code_set   = segment->code_set;
	   code_width = fontl->cswidth[code_set];
		
	   /* truncate the segment if it won't fit on the screen	*/

	   if (len > avail) {
		len = avail;
		stop_parsing = TRUE;

		/* make sure multy-byte chars won't get split	*/

		if (code_width > 1) /*  && (i=len%code_width) != 0) */
		    len -= (len%code_width);
	    }
	    avail -= len;
	    total_len += len;

	    /* copy the segment and EUC information into Screen structure */

	    bcopy(segment->str, col, len);
            memset (euc,  code_set, len);
	    col += len;

	    /* if the codeset is multy-byte, mark the 1st and 2nd bytes */
	    /* MORE: this assumes only 2 byte codes			    */

	    if (code_width > 1) {
		while (len > 0) {
		       *euc++ |= FIRST_BYTE;
		       *euc++ |= SECOND_BYTE;
		       len -= 2;
		}
	    }
	    else
	        euc  += len;
	}
	memset (attr, flags, total_len);
/* SS-inter-end */

/* SS-color */
	if (Using_colors && (flags & (USE_BG_COLOR | USE_FG_COLOR))) {
	    register Char *color =
			screen->buf[4*screen->cur_row+2] + screen->cur_col;
	    Char fgbg = 0;

	    if (flags & USE_FG_COLOR)
		fgbg = SetFGColor(fgbg, PixelIndex(textfg));
            if (flags & USE_BG_COLOR)
		fgbg = SetBGColor(fgbg, PixelIndex(textbg));
            memset (color, fgbg, total_len);
	}
/* SS-color-end */

/* SS-inter */
	/* make sure there are no 1/2 characters left on the line	*/

	if (avail > 0 && (*euc & SECOND_BYTE)) {

		/* draw a blank on the screen	*/
		
		int cx = CursorX(screen, screen->cur_col + total_len);
		int cy = CursorY(screen,
				 screen->cur_row)+screen->fnt_norm[0]->ascent;

 		XDrawImageString(screen->display, TextWindow(screen),
			 	 screen->normalGC[0], cx, cy, " ", 1);

		/* put a blank into the screen structure 	*/

		attr += total_len;
		*col  = ' ';
		*attr = 0;
		*euc  = 0;
	}
/* SS-inter-end */
}


/* SS-color : changed the first argument to screen */
ScrnInsertLine (screen, last, where, n, size)
/*
   Inserts n blank lines at sb + where, treating last as a bottom margin.
   Size is the size of each entry in sb.
   Requires: 0 <= where < where + (2 * n) <= last
   	     n <= MAX_ROWS
 */
register TScreen *screen;
/* SS-color-end */
int last;
register int where, n, size;
{
	register int i;
/* SS-color */
	register ScrnBuf sb = screen->buf;
/* SS-color-end */
	char *save [4 * MAX_ROWS];

	/* save n lines at bottom */
	bcopy ((char *) &sb [4 * (last -= n - 1)], (char *) save,
		4 * sizeof (char *) * n);
	
	/* clear contents of old rows */
	for (i = 4 * n - 1; i >= 0; i--)
		bzero ((char *) save [i], size);

	/* move down lines */
	bcopy ((char *) &sb [4 * where], (char *) &sb [4 * (where + n)],
		/* 2 * sizeof (char *) * (last - where)); */
		4 * sizeof (char *) * (last - where - n + 1));

	/* reuse storage for new lines at where */
	bcopy ((char *)save, (char *) &sb[4 * where], 4 * sizeof(char *) * n);
}


/* SS-color : changed the first argument to screen and added last argument */
ScrnDeleteLine (screen, last, where, n, size, allbuf, del_line)
/*
   Deletes n lines at sb + where, treating last as a bottom margin.
   Size is the size of each entry in sb.
   Requires 0 <= where < where + n < = last
   	    n <= MAX_ROWS
 */
register TScreen *screen;
Boolean allbuf, del_line;
/* SS-color-end */
register int n, last, size;
int where;
{
	register int i;
	char *save [4 * MAX_ROWS];
/* SS-color */
	register ScrnBuf sb;

/* SS-cut: where is modified to accomodate CUT operation */
	if (allbuf) {
	    sb = screen->allbuf;
	    if (del_line)
	        where = screen->savelines + Topline;
	}
	else {
	    sb = screen->buf;
	    if (del_line)
	        where += Topline;
	}
/* SS-color-end */

	/* save n lines at where */
	bcopy ((char *) &sb[4 * where], (char *)save, 4 * sizeof(char *) * n);

	/* clear contents of old rows */
	for (i = 4 * n - 1 ; i >= 0 ; i--)
		bzero ((char *) save [i], size);

	/* move up lines */
	bcopy ((char *) &sb[4 * (where + n)], (char *) &sb[4 * where],
		4 * sizeof (char *) * ((last -= n - 1) - where));

	/* reuse storage for new bottom lines */
	bcopy ((char *)save, (char *) &sb[4 * last],
		4 * sizeof(char *) * n);
}


/* SS-color : changed the first argument to screen */
ScrnInsertChar (screen, row, col, n, size)
/*
   Inserts n blanks in sb at row, col.  Size is the size of each row.
 */
TScreen *screen;
/* SS-color-end */
int row, size;
register int col, n;
{
/* SS-color */
	ScrnBuf sb = screen->buf;
/* SS-color-end */
	register int i, j = 4 * row;

	register Char *ptr   = sb [j];
	register Char *attr  = sb [j + 1];
	register Char *color = sb [j + 2];
	register Char *euc   = sb [j + 3];

	for (i = size - 1; i >= col + n; i--) {
		ptr[i]   = ptr[j = i - n];
		attr[i]  = attr[j];
		euc [i]  = euc [j];
		color[i] = color[j];
	}

	bzero (ptr + col, n);
	bzero (attr + col, n);
	bzero (euc + col, n);
	bzero (color + col, n);
}


/* SS-color : changed the first argument to screen */
ScrnDeleteChar (screen, row, col, n, size)
/*
   Deletes n characters in sb at row, col. Size is the size of each row.
 */
TScreen *screen;
/* SS-color-end */
register int row, size;
register int n, col;
{
/* SS-color */
	ScrnBuf sb = screen->buf;
/* SS-color-end */

	register int j;
	register Char *ptr, *attr, *euc, *color;

/* SS-cut */
	row += Topline;
/* SS-cut-end */

	j = 4 * row;
	ptr   = sb [j];
	attr  = sb [j + 1];
	color = sb [j + 2];
	euc   = sb [j + 3];
	
	j = (size - n - col);

/* SS-bug : actually this is optimization, not a bug */
	if (j > 0)
	{
	    bcopy (ptr   + col + n, ptr   + col, j);
	    bcopy (attr  + col + n, attr  + col, j);
	    bcopy (euc   + col + n, euc   + col, j);
	    bcopy (color + col + n, color + col, j);
	}
/* SS-bug-end */
	bzero (ptr   + size - n, n);
	bzero (attr  + size - n, n);
	bzero (euc   + size - n, n);
	bzero (color + size - n, n);
}


ScrnRefresh (screen, toprow, leftcol, nrows, ncols, hilite)
/*
   Repaints the area enclosed by the parameters.
   Requires: (toprow, leftcol), (toprow + nrows, leftcol + ncols) are
   	     coordinates of characters in screen;
	     nrows and ncols positive.
   If hilite is TRUE, always use this routine.  Otherwise, if colors
   are used, go to ColorScrnRefresh().
*/
register TScreen *screen;
int toprow, leftcol, nrows, ncols;
Boolean     hilite;
{
	int y = toprow * FontHeight(screen) + screen->border +
		(screen->fnt_norm[0])->ascent;
	register int row;
	register int topline = screen->topline;
	int maxrow = toprow + nrows - 1;
	int scrollamt = screen->scroll_amt;
	int max = screen->max_row;
	
	if (Using_colors && !hilite)
	    return(ColorScrnRefresh (screen, toprow, leftcol, nrows, ncols));

	if(screen->cursor_col >= leftcol && screen->cursor_col <=
	 (leftcol + ncols - 1) && screen->cursor_row >= toprow + topline &&
	 screen->cursor_row <= maxrow + topline)
		screen->cursor_state = OFF;
	for (row = toprow; row <= maxrow; y += FontHeight(screen), row++) {
	   register Char *chars;
	   register Char *attr;
/* SS-inter */
	   register Char *euc;
/* SS-inter-end */
	   register int col = leftcol;
	   int maxcol = leftcol + ncols - 1;
	   int lastind;
	   int flags, code_set, multy_byte;
	   int x, n;
	   GC gc;

	   lastind = row - scrollamt + Topline;
	   if (lastind < 0 || lastind > max)
	   	continue;
	   chars = screen->buf [4 * (lastind + topline)];
	   attr  = screen->buf [4 * (lastind + topline) + 1];
	   euc   = screen->buf [4 * (lastind + topline) + 3];

			/* FLH i18n -- skip spaces */
	   while (col <= maxcol && (attr[col] & ~BOLD) == 0 &&
	    (chars[col] & ~040) == 0)
		col++;
/* SS-inter */
	   /* if we are on the second byte of a 2-byte char, decrement */
	   /* the col.  Just in case, make sure that col>0 (this       */
	   /* should never happen)				       */

	   if ((euc[col] & SECOND_BYTE) && col > 0)
	       col--;
/* SS-inter-end */

	   while (col <= maxcol && (attr[maxcol] & ~BOLD) == 0 &&
	    (chars[maxcol] & ~040) == 0)
		maxcol--;

/* SS-inter */
	   /* if we are on the first  byte of a 2-byte char, increment */
	   /* the maxcol.					       */

	   if (euc[maxcol] & FIRST_BYTE)
	       maxcol++;
/* SS-inter-end */

	   if (col > maxcol) continue;

	   flags = attr[col];
	   code_set = Code_set (euc[col]);
	   multy_byte = Multy_byte (euc[col]);

	   /* select the GC.  It depends on the EUC being used, and */
	   /* video attributes turned on			    */

	   if (!(flags & HILITED)) {
	       if ((flags & INVERSE) != 0)
	           if (flags & BOLD) gc = screen->reverseboldGC[code_set];
	           else gc = screen->reverseGC[code_set];
	       else 
	           if (flags & BOLD) gc = screen->normalboldGC[code_set];
	           else gc = screen->normalGC[code_set];
	   }
	   else {
	       if ((flags & INVERSE) != 0)
	           if (flags & BOLD) gc = screen->normalboldGC[code_set];
	           else gc = screen->normalGC[code_set];
	       else 
	           if (flags & BOLD) gc = screen->reverseboldGC[code_set];
	           else gc = screen->reverseGC[code_set];
	   }

	   x = CursorX(screen, col);
	   lastind = col;

	   for (; col <= maxcol; col++) {
		if ((attr[col] != flags) ||
		    ((euc[col] & EUC_MASK) != code_set)) {
		   if (multy_byte) {
		       XDrawImageString16(screen->display, TextWindow(screen), 
		        		gc, x, y, (XChar2b *) &chars[lastind],
					n = (col - lastind)/2);
		       if ((flags & BOLD) && screen->enbolden)
		 	    XDrawString16(screen->display, TextWindow(screen), 
			 	   gc, x+1, y, (XChar2b *) &chars[lastind], n);
		       n += n;
		   }
		   else {
		       XDrawImageString(screen->display, TextWindow(screen), 
		        		gc, x, y, (char *) &chars[lastind],
					n = col - lastind);
		       if ((flags & BOLD) && screen->enbolden)
		 	    XDrawString(screen->display, TextWindow(screen), 
			 	       gc, x+1, y, (char *) &chars[lastind], n);
		   }
		   if (flags & UNDERLINE) 
		       XDrawLine(screen->display, TextWindow(screen), 
		 	         gc, x, y+1, x+n*FontWidth(screen), y+1);

		   /*x += (col - lastind) * FontWidth(screen);*/
		   x += n * FontWidth(screen);

		   lastind = col;

		   flags = attr[col];
	   	   code_set = Code_set (euc[col]);
	   	   multy_byte = Multy_byte (euc[col]);

	   	   if (!(flags & HILITED)) {
	   	       if ((flags & INVERSE) != 0)
	       		    if (flags & BOLD) gc = screen->reverseboldGC[code_set];
	       		    else gc = screen->reverseGC[code_set];
	  	       else 
	      		    if (flags & BOLD) gc = screen->normalboldGC[code_set];
	      		    else gc = screen->normalGC[code_set];
	   	   }
	   	   else {
	       	       if ((flags & INVERSE) != 0)
	           	   if (flags & BOLD) gc = screen->normalboldGC[code_set];
	           	   else gc = screen->normalGC[code_set];
	       	       else 
	           	   if (flags & BOLD) gc = screen->reverseboldGC[code_set];
	           	   else gc = screen->reverseGC[code_set];
	   	   }
		}

		if(chars[col] == 0)
			chars[col] = ' ';
	   }

	   if (multy_byte) {
	       XDrawImageString16(screen->display, TextWindow(screen), gc, 
	         	          x, y, (XChar2b *) &chars[lastind],
				  n = (col-lastind)/2);
	       if ((flags & BOLD) && screen->enbolden)
		    XDrawString16(screen->display, TextWindow(screen), gc,
				  x + 1, y, (XChar2b *) &chars[lastind], n);
	       n += n;
	   }
	   else {
	       XDrawImageString(screen->display, TextWindow(screen), gc, 
	         	        x, y, (char *) &chars[lastind],
				n = col-lastind);
	       if ((flags & BOLD) && screen->enbolden)
		    XDrawString(screen->display, TextWindow(screen), gc,
				  x + 1, y, (char *) &chars[lastind], n);
	   }

	   if(flags & UNDERLINE) 
		XDrawLine(screen->display, TextWindow(screen), gc, 
		 	  x, y+1, x + n * FontWidth(screen), y+1);
	}
}



/* SS-color : same as ScrnRefresh, but it also examines colors */
/*	      I made it a separate routine in order not to     */
/*	      penalize most of the programs with at least 2    */
/* 	      extra "ifs" per character			       */
/* Places different from ScrnRefresh are highlited    	       */
ColorScrnRefresh (screen, toprow, leftcol, nrows, ncols)
/*
   Repaints the area enclosed by the parameters.
   Requires: (toprow, leftcol), (toprow + nrows, leftcol + ncols) are
   	     coordinates of characters in screen;
	     nrows and ncols positive.
 */
register TScreen *screen;
int toprow, leftcol, nrows, ncols;
{
	int y = toprow * FontHeight(screen) + screen->border +
		(screen->fnt_norm[0])->ascent;
	register int row;
	register int topline = screen->topline;
	int maxrow = toprow + nrows - 1;
	int scrollamt = screen->scroll_amt;
	int max = screen->max_row;
	

	if(screen->cursor_col >= leftcol && screen->cursor_col <=
	 (leftcol + ncols - 1) && screen->cursor_row >= toprow + topline &&
	 screen->cursor_row <= maxrow + topline)
		screen->cursor_state = OFF;
	for (row = toprow; row <= maxrow; y += FontHeight(screen), row++) {
	   register Char *chars;
	   register Char *attr;
	   register Char *euc;
	   register Char *color;
/* SS-color */
	   register Pixel sfg, sbg, savefg, savebg;
	   Char	    fg, bg;
	   register Boolean savedcolor = FALSE;
/* SS-color-end */
	   register int col = leftcol;
	   int maxcol = leftcol + ncols - 1;
	   int lastind;
	   int flags, code_set, multy_byte;
	   int x, n;
	   GC gc;

	   lastind = row - scrollamt + Topline;
	   if (lastind < 0 || lastind > max)
	   	continue;
	   chars = screen->buf [4 * (lastind + topline)];
	   attr  = screen->buf [4 * (lastind + topline) + 1];
	   color = screen->buf [4 * (lastind + topline) + 2];
	   euc   = screen->buf [4 * (lastind + topline) + 3];
/* SS-color */
	   sfg = screen->foreground;
	   sbg = screen->background;

	   /* skip colorless blanks on both ends of the line */

	   while (col <= maxcol && (attr[col] & ~BOLD) == 0 &&
	    	 (chars[col] & ~040) == 0)
		col++;
/* SS-inter */
	   /* if we are on the second byte of a 2-byte char, decrement */
	   /* the col.  Just in case, make sure that col>0 (this       */
	   /* should never happen)				       */

	   if ((euc[col] & SECOND_BYTE) && col > 0)
	       col--;
/* SS-inter-end */

	   while (col <= maxcol && (attr[maxcol] & ~BOLD) == 0 &&
	    	 (chars[maxcol] & ~040) == 0)
		maxcol--;
/* SS-inter */
	   /* if we are on the first  byte of a 2-byte char, increment */
	   /* the maxcol.					       */

	   if (euc[maxcol] & FIRST_BYTE)
	       maxcol++;
/* SS-inter-end */

	   if (col > maxcol) continue;

	   flags = attr[col];
	   code_set = Code_set(euc[col]);
	   multy_byte = Multy_byte(euc[col]);
	   fg = FGColor(color[col]);
	   bg = BGColor(color[col]);

	   if (!(flags & HILITED)) {
	       if ((flags & INVERSE) != 0)
	           if (flags & BOLD) gc = screen->reverseboldGC[code_set];
	           else gc = screen->reverseGC[code_set];
	       else 
	           if (flags & BOLD) gc = screen->normalboldGC[code_set];
	           else gc = screen->normalGC[code_set];
	   }
	   else {
	       if ((flags & INVERSE) != 0)
	           if (flags & BOLD) gc = screen->normalboldGC[code_set];
	           else gc = screen->normalGC[code_set];
	       else 
	           if (flags & BOLD) gc = screen->reverseboldGC[code_set];
	           else gc = screen->reverseGC[code_set];
	   }

	   if (((flags & (USE_FG_COLOR | USE_BG_COLOR))) &&
	       ((flags & HILITED) == 0))
	   {
	       /* we may change the foreground or background of GC, so  */
	       /* save the original ones				*/

	       savefg = GC_Foreground (gc);
	       savebg = GC_Background (gc);
	       savedcolor = TRUE;

	       if ((PixelValue(fg) != sfg) && (flags & USE_FG_COLOR))
	   	    if (flags & INVERSE)
		        XSetBackground (screen->display, gc, PixelValue(fg));
		    else
		        XSetForeground (screen->display, gc, PixelValue(fg));

	       if ((PixelValue(bg) != sbg) && (flags & USE_BG_COLOR))
	   	    if (flags & INVERSE)
		        XSetForeground (screen->display, gc, PixelValue(bg));
		    else
		        XSetBackground (screen->display, gc, PixelValue(bg));
	   }
    
	   x = CursorX(screen, col);
	   lastind = col;

	   for (; col <= maxcol; col++) {
		Char fgc = FGColor(color[col]);
		Char bgc = BGColor(color[col]);
		if (attr[col] != flags ||
		    Code_set(euc[col]) != code_set ||
		    (((attr[col] & USE_FG_COLOR) && (fgc != fg)) ||
		    ((attr[col] & USE_BG_COLOR) && (bgc != bg))) &&
	            ((flags & HILITED) == 0))
		{
		   if (multy_byte) {
		       XDrawImageString16(screen->display, TextWindow(screen), 
		        		  gc, x, y, (XChar2b *) &chars[lastind],
					  n = (col - lastind)/2);
		       if ((flags & BOLD) && screen->enbolden)
		 	    XDrawString16(screen->display, TextWindow(screen), 
			 	   gc, x+1, y, (XChar2b *) &chars[lastind], n);
		       n += n;
		   }
		   else {
		       XDrawImageString(screen->display, TextWindow(screen), 
		        		gc, x, y, (char *) &chars[lastind],
					n = col - lastind);
		       if ((flags & BOLD) && screen->enbolden)
		 	    XDrawString(screen->display, TextWindow(screen), 
			 	       gc, x+1, y, (char *) &chars[lastind], n);
		   }
		   if (flags & UNDERLINE) 
		       XDrawLine (screen->display, TextWindow(screen), 
			 	  gc, x, y+1, x+n*FontWidth(screen), y+1);

		   /* x += (col - lastind) * FontWidth(screen); */
		   x += n * FontWidth(screen);

		   lastind = col;

		   if (attr[col] != flags ||
		       Code_set(euc[col]) != code_set)
		   {
		       /* we are going to use the new gc, so restore the */
		       /* old gc and save the colors from the new one    */

		       if (savedcolor)
		       {
           	           XSetForeground (screen->display, gc, savefg); 
           	           XSetBackground (screen->display, gc, savebg);
			   savedcolor = FALSE;
		       }

		       flags = attr[col];
		       code_set = Code_set(euc[col]);
		       multy_byte = Multy_byte(euc[col]);
	   	       if (!(flags & HILITED)) {
	       	           if ((flags & INVERSE) != 0)
	           	       if (flags & BOLD) gc = screen->reverseboldGC[code_set];
	           	       else gc = screen->reverseGC[code_set];
	       	           else 
	           	       if (flags & BOLD) gc = screen->normalboldGC[code_set];
	           	       else gc = screen->normalGC[code_set];
	   	       }
	   	       else {
	       	           if ((flags & INVERSE) != 0)
	           	       if (flags & BOLD) gc = screen->normalboldGC[code_set];
	           	       else gc = screen->normalGC[code_set];
	       	           else 
	           	       if (flags & BOLD) gc = screen->reverseboldGC[code_set];
	           	       else gc = screen->reverseGC[code_set];
	   	       }

	   	       if (((flags & (USE_FG_COLOR | USE_BG_COLOR))) &&
	       	           ((flags & HILITED) == 0))
		       {
	   	           savefg = GC_Foreground (gc);
	   	           savebg = GC_Background (gc);
			   savedcolor = TRUE;

		           /* the gc has definitelly changed, so we must  */
		           /* set foreground and background		      */
    
	   		    fg = fgc;
	   		    bg = bgc;
	       		    if ((PixelValue(fg) != sfg) &&
				(flags & USE_FG_COLOR)) {
	   		       if (flags & INVERSE)
		    		    XSetBackground (screen->display, gc, PixelValue(fg));
			       else
		    		    XSetForeground (screen->display, gc, PixelValue(fg));
		            }

	       		    if ((PixelValue(bg) != sbg) &&
				(flags & USE_BG_COLOR)) {
	   		       if (flags & INVERSE)
		    		    XSetForeground (screen->display, gc, PixelValue(bg));
			       else
		    		    XSetBackground (screen->display, gc, PixelValue(bg));
		            }
			}
		   }
		   else
		   {
		       /* the gc has not changed, so we need to set only */
		       /* the color that has changed			 */

	   		if (fg != fgc)
			{
			   fg = fgc;
	   		   if (flags & INVERSE)
		    		XSetBackground (screen->display, gc, PixelValue(fg));
			   else
		    		XSetForeground (screen->display, gc, PixelValue(fg));
			}

	   		if (bg != bgc)
			{
			   bg = bgc;
	   		   if (flags & INVERSE)
		    		XSetForeground (screen->display, gc, PixelValue(bg));
			   else
		    		XSetBackground (screen->display, gc, PixelValue(bg));
			}
		   }
		}
/* SS-color-end */

		if(chars[col] == 0)
			chars[col] = ' ';
	   }
	   if (multy_byte) {
	       XDrawImageString16(screen->display, TextWindow(screen), gc, 
	         	          x, y, (XChar2b *) &chars[lastind],
				  n = (col - lastind)/2);
	       if ((flags & BOLD) && screen->enbolden)
		    XDrawString16(screen->display, TextWindow(screen), gc,
		    		  x + 1, y, (XChar2b *) &chars[lastind], n);
	       n += n;
	   }
	   else {
	       XDrawImageString(screen->display, TextWindow(screen), gc, 
	         	        x, y, (char *) &chars[lastind],
				n = col - lastind);
	       if ((flags & BOLD) && screen->enbolden)
		    XDrawString(screen->display, TextWindow(screen), gc,
		    		x + 1, y, (char *) &chars[lastind], n);
	   }
	   if(flags & UNDERLINE) 
		XDrawLine(screen->display, TextWindow(screen), gc, 
		 x, y+1, x + n * FontWidth(screen), y+1);

	   if (savedcolor)
	   {
	       XSetForeground (screen->display, gc, savefg); 
	       XSetBackground (screen->display, gc, savebg);
	       savedcolor = FALSE;
	   }
	}
}
/* SS-color-end */



ClearBufRows (screen, first, last)
/*
   Sets the rows first though last of the buffer of screen to spaces.
   Requires first <= last; first, last are rows of screen->buf.
 */
register TScreen *screen;
register int first, last;
{
	first *= 4;
	last = 4 * last + 1;
	for (; first <= last; first += 4) {
	     bzero (screen->buf [first], (screen->max_col + 1));
	     bzero (screen->buf [first+1], (screen->max_col + 1));
	     bzero (screen->buf [first+3], (screen->max_col + 1));
	}
}

ScreenResize (screen, width, height, flags)
/*
	width and height are measurements of xterm widget, without border

   Resizes screen:
   1. If overall size didn't change, then return.

   3. If number of lines or columns has changed, and screen->buf
		has been allocated, enlarges screen->buf
      if necessary  (new space is appended to the bottom and to the right),
      or reduces  screen->buf if necessary (old space is removed from the
      bottom and from the right).  (buf is not allocated until
		*after* realization, but resize will occur during creation
		of scrollbar, which occurs *during* realization.)
   4. Cursor is positioned as closely to its former position as possible
   5. Sets screen->max_row and screen->max_col to reflect new size
   6. Maintains the inner border.
   7. Clears origin mode and sets scrolling region to be entire screen.
   8. Returns 0
 */
register TScreen *screen;
int width, height;
unsigned *flags;
{
	register int rows, cols;
	register int index;
	register int savelines;
	register ScrnBuf sb = screen->allbuf;
	register ScrnBuf ab = screen->altbuf;
/* SS-color */
	register Pixel fg = screen->foreground;
	register Pixel bg = screen->background;
/* SS-color-end */
	register int x, xx;
	register int border = 2 * screen->border;
	register int i, j, k;
#ifdef sun
#ifdef TIOCSSIZE
	struct ttysize ts;
#endif	/* TIOCSSIZE */
#else	/* sun */
#ifdef TIOCSWINSZ
	struct winsize ws;
#endif	/* TIOCSWINSZ */
#endif	/* sun */

/* SS-copy */
	extern Boolean	Have_hilite;
	extern int      TrackText();
/* SS-copy-end */

	/* nothing has changed, simply return	*/

	if (FullHeight(screen) == height && FullWidth(screen) == width)
	 	return(0);

/* SS-copy */
	if (Have_hilite) {
	    TrackText (0, 0, 0, 0);
	    Have_hilite = FALSE;
	}
/* SS-copy-end */

#ifdef ROUND_SIZE 
	/* round so that it is unlikely the screen will change size on  */
	/* small mouse movements.					*/
	/* This has been taken out because it causes an additional row
	 * which cannot be fully displayed in the xterm window when the 
	 * window is resized by an amount other than an integral multiple
	 * of the font width/height.
	 */
	   
/* SS-menu */
	rows = (height + FontHeight(screen) / 2 - border) / FontHeight(screen);
/* SS-menu-end */
	cols = (width + FontWidth(screen) / 2 - border) /
	 FontWidth(screen);
#endif

	rows = (height - border) / FontHeight(screen);
	cols = (width - border) / FontWidth(screen);

	if (rows < 1) rows = 1;
	if (cols < 1) cols = 1;

	/* change buffers if the screen has changed size */
	if (screen->max_row != rows - 1 || screen->max_col != cols - 1) {

	    int r_gap;	/* distance betweeen last column and window edge */
	    int b_gap;  /* distance between last row and window edge */
	    
	    if(screen->cursor_state)
		HideCursor();
	    savelines = screen->scrollWidget ? screen->savelines : 0;
	    
	    if (screen->altbuf) 
		Reallocate(&screen->altbuf, rows, cols,
			   screen->max_row + 1, screen->max_col + 1,
			   screen->foreground, screen->background);
	    Reallocate(&screen->allbuf, rows + savelines, cols,
		       screen->max_row+1 + savelines, screen->max_col + 1,
		       screen->foreground, screen->background);
	    screen->buf = &screen->allbuf[4 * savelines];
	    
	    if ((screen->segment.str = (Char *) realloc ((char *) screen->segment.str,
							 (unsigned) 3*cols)) == NULL)
		SysError (ERROR_SREALLOC);
	    
	    /* Clear the right and bottom internal border.
	     * The window manager may not have honored our window
	     * size increments.  In that case, we will have extra
	     * space (at most one character-width along the right
	     * edge and at most one character-height along the bottom
	     * edge) *in*addition*to* the internal border.  We must
	     * clear all space to the right of the last column and
	     * below the last row.
	     */
	    
	    r_gap = width - (screen-> border + cols * FontWidth(screen));
	    b_gap = height - (screen-> border + rows * FontHeight(screen));
	    
	    XClearArea (screen->display, TextWindow (screen),
			width - r_gap, 0,
			r_gap, height, False);

	    XClearArea (screen->display, TextWindow (screen),
			0, height - b_gap,
			width, b_gap, False);
	    
	    screen->max_row = rows - 1;
	    screen->max_col = cols - 1;
	    
	    /* adjust scrolling region */
	    screen->top_marg = 0;
	    screen->bot_marg = screen->max_row;
	    *flags &= ~ORIGIN;
	    
	    if (screen->cur_row > screen->max_row)
		screen->cur_row = screen->max_row;
	    if (screen->cur_col > screen->max_col)
		screen->cur_col = screen->max_col;
	    
	    screen->fullVwin.height = height - border;
	    screen->fullVwin.width = width - border;
	}

	if(screen->scrollWidget)
	{
	    Arg args[2];

	    XtSetArg(args[0], XtNsliderMax, (XtArgVal)
			      (screen->savelines + screen->max_row + 1));
	    XtSetArg(args[1], XtNproportionLength, (XtArgVal) rows);
						   
	    XtSetValues (screen->scrollWidget, args, 2);

	    /* in creating scrollbar, leave 1 pixel between the scrollbar */
	    /* and window manager border.			          */
		
			/* update the position of the scrollbar "thumb" */
		ScrollBarDrawThumb(screen->scrollWidget);
	}

	screen->fullVwin.fullheight = height;
	screen->fullVwin.fullwidth = width;
#ifdef sun
#ifdef TIOCSSIZE
	/* Set tty's idea of window size */
	ts.ts_lines = rows;
	ts.ts_cols = cols;
	ioctl (screen->respond, TIOCSSIZE, &ts);
#ifdef SIGWINCH
	if(screen->pid > 1)
		killpg(xgetpgrp(screen->pid), SIGWINCH);
#endif	/* SIGWINCH */
#endif	/* TIOCSSIZE */
#else	/* sun */
#ifdef TIOCSWINSZ
	/* Set tty's idea of window size */
	ws.ws_row = (unsigned short) rows;
	ws.ws_col = (unsigned short) cols;
	ws.ws_xpixel = (unsigned short) width;
	ws.ws_ypixel = (unsigned short) height;
	ioctl (screen->respond, TIOCSWINSZ, (char *)&ws);
#ifdef SIGWINCH
	if(screen->pid > 1)
		killpg(xgetpgrp((int)screen->pid), SIGWINCH);
#endif	/* SIGWINCH */
#endif	/* TIOCSWINSZ */
#endif	/* sun */
	return (0);
}

/* SS-color */

Char
PixelIndex (n)
Pixel n;
{
	register Char i;

	for (i=0; i<8; i++)
	     if (Pixels[i] == n)
		 return i;

#if !defined(I18N)
	fprintf (stderr, "No index for Pixel 0x%x\n", n);
#else
	OlVaDisplayErrorMsg(XtDisplay(toplevel), OleNpixel,
		OleTbadPixel,
		OleCOlClientXtermMsgs,
		OleMpixel_badPixel, n);
#endif
	return 5;
}

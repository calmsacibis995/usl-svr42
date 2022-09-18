/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:MotifIPos.c	1.5"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This file contains icon related procedures for Motif style.
 *
 ******************************file*header********************************
 */

#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <Xol/DynamicP.h>
#include <wm.h>
#include <Xol/VendorI.h>
#include <WMStepP.h>
#include <Extern.h>
#include <limits.h>	/* Has WORD_BIT, the number of bits per int */

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */
static int	num_horz_icons,
		num_vert_icons;
static int	num_horz_words;

/* icon_map: a map of where the icons are on the screens - will be a "flat"
 * 2 dimensional array.
 */
static unsigned int	*icon_map;

/* To get to row i, col. j in this icon_map, consider num_horz_icons ==
 * the number of columns;  An array of 3 rows by 2 cols, looking like:
 *	[R0C0 R0C1 R0C2]
 *	[R1C0 R1C1 R1C2]
 *	[R2C0 R2C1 R2C2]
 *
 * will look like:	[R0C0 R0C1 R2C2 R1C0 R1C1 R1C2 R2C0 R2C1 R2C2]
 * Access as icon_map[sub(i,j)] (instead of icon_map[i][j])
 */
#define sub(i,j)	(i * num_horz_words + j)

/* Motif icons: No other explanation exists besides there must be some
 * forced minimum space between motif icons - whatever number I define
 * seemed to work best after trial and error.
 */
#define MIN_ICON_SPACING	4

/* The max possible iconPlacementMargin is about 128 from trial and error */
#define MAX_ICON_PLACE_MARGIN	128

/*
 *************************************************************************
 *
 * Procedures
 *
 ***************************private*procedures****************************
 */

extern void	ConfirmIconPosition OL_ARGS((WMStepWidget, WMGeometry **));
extern void	ConsumeIconPosition OL_ARGS((WMStepWidget, WMGeometry **));
extern void	DetermineXYFromMapPos OL_ARGS((Screen *, int, int,
							int *, int *));
extern Boolean	DetermineMapPosFromXY OL_ARGS((Screen *, int, int,
							int *, int *));
extern void	InitIconPlacementStrategy OL_ARGS((Screen *));
extern void	MoveMapPosition OL_ARGS((WMStepWidget, int, int));

extern void	ReleaseMapPosition OL_ARGS((int, int));
static void	ScreenReduction OL_ARGS((Screen *, int *,int * ,int * ,int * ));

void
InitIconPlacementStrategy OLARGLIST((screen))
OLGRA(Screen *, screen)
{
int vert_remdr, horiz_remdr;
int	wid = mot_icon_iwidth,
	ht = mot_icon_iheight + mot_icon_label_height;
int	scrwid = WidthOfScreen(screen);
int	scrht = HeightOfScreen(screen);
int	numicons;
int	space_per_icon;
int	remdr_per_icon;
int	numhorzwords;
int	numvertwords;
int	wordstomalloc;
int	marginstep;

	/* How many icons per row and column?  That will determine how
	 * we set up the array of "bit indicators".  iconPlacementMargin
	 * tells me how much space to leave from each edge;  the minimum
	 * space between icons is MIN_ICON_SPACING, defined at the top of
	 * this file.
	 */
	if (motwmrcs->iconPlacementMargin < 0 ||
		motwmrcs->iconPlacementMargin > 0 &&
			motwmrcs->iconPlacementMargin < MAX_ICON_PLACE_MARGIN)
		marginstep = motwmrcs->iconPlacementMargin;
	else
		if (motwmrcs->iconPlacementMargin > MAX_ICON_PLACE_MARGIN)
			marginstep = motwmrcs->iconPlacementMargin =
					MAX_ICON_PLACE_MARGIN;
		
	/* Subtract the icon margins from the allowable room on the screen
	 * for icons
	 */
	scrwid -= 2 * (motwmrcs->iconPlacementMargin < 0 ? 0 : motwmrcs->iconPlacementMargin);
	scrht -= 2 * (motwmrcs->iconPlacementMargin < 0 ? 0 : motwmrcs->iconPlacementMargin);

	/* remdr is wasted space, so divide it up over all icons */
	num_horz_icons = scrwid / wid;
	for (; num_horz_icons > 1;num_horz_icons--) {
		horiz_remdr = scrwid - num_horz_icons * wid;
		remdr_per_icon = horiz_remdr / num_horz_icons;
		/* There are num_horz_icons-1 spaces, each must be 
	 	 * >= MIN_ICON_SPACING pixels.
	 	 */
		/*
		space_per_icon = (num_horz_icons  - 1) / remdr_per_icon;
		if (space_per_icon <  MIN_ICON_SPACING)
		 */
		if (remdr_per_icon <  MIN_ICON_SPACING)
			continue;
		break;
	}
	space_per_icon = scrwid / num_horz_icons;
	/* XIconIncrement is the amount of space to be used by each icon in the
	 * vertical direction - an icon is always centered in this space
	 * such that an equal amount of free space exists on each side of it.
	 * Be careful with some calculations, because regardless of this,
	 * there may exist a non-zero iconPlacementMargin - calculate this
	 * separately - remember, the amount  of space for icon use on
	 * the screen is reduced by 2 * iconPlacementMargin.  Same goes for
	 * YIncrement.
	 */
	XIconIncrement = space_per_icon;
	/* Now, num_horz_icons = the number of icons per row on the grid.
	 * Do the same for vertical grid.
	 */
	num_vert_icons = scrht / ht;

	for (; num_vert_icons > 1;num_vert_icons--) {
		vert_remdr = scrht - num_vert_icons * ht;
		remdr_per_icon = vert_remdr / num_vert_icons;
		/* There are num_horz_icons-1 spaces, each must be 
	 	 * >= MIN_ICON_SPACING pixels.
	 	 */
		/*
		space_per_icon = (num_vert_icons  - 1) / remdr_per_icon;
		if (space_per_icon <  MIN_ICON_SPACING)
		 */
		if (remdr_per_icon <  MIN_ICON_SPACING)
			continue;
		break;
	}

	space_per_icon = scrht / num_vert_icons;
	/* YIncrement is the amount of space to be used by each icon in the
	 * vertical direction - an icon is always centered in this space
	 * such that an equal amount of free space exists on each side of it.
	 */
	YIconIncrement = space_per_icon;
	/* We assume WORD_BIT bits per int */
	num_horz_words = num_horz_icons / WORD_BIT +
				( (num_horz_icons % WORD_BIT  ) ? 1 : 0);
	numvertwords = num_vert_icons;
	wordstomalloc = num_horz_words * numvertwords;
	icon_map = (unsigned int *)calloc(wordstomalloc, sizeof(unsigned int));

	/* "icon_map" is a flattened 2-dim array of m x n ints,
	 * where m = num_vert_icons, and the sum of all the bits in n
	 * >= num_horz_icons. That's m rows to represent icons (the n's).
	 * If a bit is on, then the position on the screen represented by
	 * it is taken by an icon.  If a bit is off, then that position
	 * it represents is free.  This is a very strict interpretation -
	 * what you see is what you get (WYSIWHG).  There may be
	 * lot of shifts used here.
	 */
}

/* ConsumeIconPosition - find the first available position, based on the
 * gravity.
 */
extern void
ConsumeIconPosition OLARGLIST((wm, p))
OLARG(WMStepWidget, wm)
OLGRA(WMGeometry **, p)
{
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
Screen 	*screen = XtScreen((Widget)wm);
int	x, y;
int	i, j, k, step;
unsigned int	currentword,
		result_bits;
int	icon_count;
Boolean	go = 0;
int	maprow, mapcol;
int	retx, rety;
WMGeometry	*q = *p;

/* for now, assume p is malloc'd in IconPosition()...
	if (*p != NULL)
		return(*p);
	*p = wmstep->icon = (WMGeometry *) XtMalloc(sizeof(WMGeometry));
	if ( (wmstep->hints & WMHints) &&
			   (wmstep->xwmhints->flags & IconPositionHint) ) {
		*p->x = wmstep->xwmhints->icon_x;
		*p->y = wmstep->xwmhints->icon_y;
		return;
	}
 */
	switch(wmrcs->iconGravity) {
		case WMNorthGravity:
			/* North: left to right across the top */
			/* i = row, j = column */
			for (i = 0; i < num_vert_icons; i++) {
				for (j=0, icon_count = num_horz_icons;
						j < num_horz_words; j++,
						icon_count -= WORD_BIT) {
					currentword = icon_map[sub(i,j)];
/*
fprintf(stderr,"sub(i,j) = %d\n", sub(i,j));
fprintf(stderr,"currentword = %d\n", currentword);
 */
					step = WORD_BIT - 1;
					/* Any empty spaces in the word?*/
					for (k=0;k < icon_count && k< WORD_BIT;
								step--,k++){
						if (!(1<<step & currentword)){
							/* turn it on */
							result_bits =
						(unsigned int)	(1 << step);
/*
fprintf(stderr,"sub(i,j) = %d\n", sub(i,j));
fprintf(stderr,"result_bits= %d\n", result_bits);
 */
							icon_map[sub(i,j)] |=
								result_bits;
							/* row i, col j */
							maprow = i;
							mapcol = j* (WORD_BIT-1)
									+ k;
							go++; break;
						}
					} /* for k */
					if (go)
						break;
				} /* for j */
				if (go)
					break;
			} /* for i */
			if (i >= num_vert_icons)  {
				/* mlp-out of room - make new icon map for
				 * the next layer of icons!  For now,
				 * forget it.
				 */
				q->x = NO_ICON_ROOM;
				return;
			}
			/* Now, where is this position?? We need an x,y
			 * for the position of the icon, x = col, y = row.
			 */
			DetermineXYFromMapPos(screen, maprow, mapcol,
							&retx, &rety);
			q->x = retx;
			q->y = rety;
			q->width = icon_width;
			q->height = icon_height;
			wmstep->icon_map_pos_row = maprow;
			wmstep->icon_map_pos_col = mapcol;
			break;
		case WMSouthGravity:
			/* i = row, j = column */
			for (i = num_vert_icons - 1; i >=0; i--) {
				for (j=0, icon_count = num_horz_icons;
						j < num_horz_words; j++,
						icon_count -= WORD_BIT) {
					currentword = icon_map[sub(i,j)];
/*
fprintf(stderr,"sub(i,j) = %d\n", sub(i,j));
fprintf(stderr,"currentword = %d\n", currentword);
 */
					step = WORD_BIT - 1;
					/* Any empty spaces in the word?*/
					for (k=0;k < icon_count && k< WORD_BIT;
								step--,k++){
						if (!(1<<step & currentword)){
							/* turn it on */
							result_bits =
						(unsigned int)	(1 << step);
/*
fprintf(stderr,"sub(i,j) = %d\n", sub(i,j));
fprintf(stderr,"result_bits= %d\n", result_bits);
 */
							icon_map[sub(i,j)] |=
								result_bits;
							/* row i, col j */
							maprow = i;
							mapcol = j* (WORD_BIT-1)
									+ k;
							go++; break;
						}
					} /* for k */
					if (go)
						break;
				} /* for j */
				if (go)
					break;
			} /* for i */
					
			if (i < 0)  {
				/* mlp-out of room - make new icon map for
				 * the next layer of icons!  For now,
				 * forget it.
				 */
				q->x = NO_ICON_ROOM;
				return;
			}
			/* Now, where is this position?? We need an x,y
			 * for the position of the icon, x = col, y = row.
			 */
			DetermineXYFromMapPos(screen, maprow, mapcol,
							&retx, &rety);
			q->x = retx;
			q->y = rety;
			q->width = icon_width;
			q->height = icon_height;
			wmstep->icon_map_pos_row = maprow;
			wmstep->icon_map_pos_col = mapcol;
			break;
		case WMEastGravity:
			/* East : Start at upper right, go south; then offset
			 * from the right for additional columns.
			 */
			/* i = row, j = column */
			icon_count = num_horz_icons;
			/* for i - start at right column */
			for (i = num_horz_words - 1; i >= 0; i--) {
				/* for step: start at rightmost bit */
				for (step = WORD_BIT - (icon_count %  WORD_BIT);
						step < WORD_BIT; step++,
							icon_count--) {
					for (j = 0; j < num_vert_icons; j++) {
					currentword = icon_map[sub(i,j)];
/*
fprintf(stderr,"sub(i,j) = %d\n", sub(i,j));
fprintf(stderr,"currentword = %d\n", currentword);
 */
					/* Any empty spaces in the word?*/
						if (!(1<<step & currentword)){
							/* turn it on */
							result_bits =
						(unsigned int)	(1 << step);
/*
fprintf(stderr,"sub(i,j) = %d\n", sub(i,j));
fprintf(stderr,"result_bits= %d\n", result_bits);
 */
							icon_map[sub(i,j)] |=
								result_bits;
							/* row i, col j */
							mapcol = i * WORD_BIT +
								WORD_BIT - step
								- 1;
							maprow = j;
							go++; break;
						} /* if */
					} /* for j */
					if (go)
						break;
				} /* for step */
				if (go)
					break;
			} /* for i */
			if (i < 0)  {
				/* mlp-out of room - make new icon map for
				 * the next layer of icons!  For now,
				 * forget it.
				 */
				q->x = NO_ICON_ROOM;
				return;
			}
			/* Now, where is this position?? We need an x,y
			 * for the position of the icon, x = col, y = row.
			 */
			DetermineXYFromMapPos(screen, maprow, mapcol,
							&retx, &rety);
			q->x = retx;
			q->y = rety;
			q->width = icon_width;
			q->height = icon_height;
			wmstep->icon_map_pos_row = maprow;
			wmstep->icon_map_pos_col = mapcol;
			break;
		case WMWestGravity:
			/* Opposite of east, right?? Use similar concepts */
			/* i = row, j = column */
			icon_count = num_horz_icons;
			/* for i - start at right column */
			for (i = 0; i < num_horz_words; i++) {
				/* for step: start at rightmost bit */
				for (step = WORD_BIT - 1;
						step > 0; step--,
							icon_count--) {
					for (j = 0; j < num_vert_icons; j++) {
					currentword = icon_map[sub(i,j)];
/*
fprintf(stderr,"sub(i,j) = %d\n", sub(i,j));
fprintf(stderr,"currentword = %d\n", currentword);
 */
					/* Any empty spaces in the word?*/
						if (!(1<<step & currentword)){
							/* turn it on */
							result_bits =
						(unsigned int)	(1 << step);
/*
fprintf(stderr,"sub(i,j) = %d\n", sub(i,j));
fprintf(stderr,"result_bits= %d\n", result_bits);
 */
							icon_map[sub(i,j)] |=
								result_bits;
							/* row i, col j */
							mapcol = i * WORD_BIT +
								WORD_BIT - step
								- 1;
							maprow = j;
							go++; break;
						} /* if */
					} /* for j */
					if (go)
						break;
				} /* for step */
				if (go)
					break;
			} /* for i */
			if (i >= num_horz_words)  {
				/* mlp-out of room - make new icon map for
				 * the next layer of icons!  For now,
				 * forget it.
				 */
				q->x = NO_ICON_ROOM;
				return;
			}
			/* Now, where is this position?? We need an x,y
			 * for the position of the icon, x = col, y = row.
			 */
			DetermineXYFromMapPos(screen, maprow, mapcol,
							&retx, &rety);
			q->x = retx;
			q->y = rety;
			q->width = icon_width;
			q->height = icon_height;
			wmstep->icon_map_pos_row = maprow;
			wmstep->icon_map_pos_col = mapcol;
			break;
	} /* switch */
} /* ConsumeIconPosition */

/*
 * ConfirmIconPosition: A widget already had a position for it's icon -
 * is the position still available? If so, then nothing to do - just
 * consume the position right here; if not, then call ConsumeIconPosition()
 * to consume a brand new one.
 */
extern void
ConfirmIconPosition OLARGLIST((wm, p))
OLARG(WMStepWidget, wm)
OLGRA(WMGeometry **, p)
{
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
int	oldrow = wmstep->icon_map_pos_row;
int	oldcol = wmstep->icon_map_pos_col;
int	rowword, colword;
unsigned int	result_bits,
		confirm_word;
		rowword = oldrow;
		colword = oldcol / WORD_BIT;
		confirm_word = sub(rowword, colword);
		result_bits = (unsigned int) (1 << (WORD_BIT - (oldcol + 1)));
		if (!(icon_map[confirm_word] & result_bits)) {
			/* Good news: the spot hasn't been taken,
			 * so consume it.
			 */
			icon_map[confirm_word] |= result_bits;
			/* Nothing else to do because icon_geometry stays
			 * the same.
			 */
			return;
		}
		else {
			/* Our old position has already been taken - darn.
			 * Just get me a new spot in the map.
			 */
			ConsumeIconPosition(wm, p);
		}

} /* ConfirmIconPosition */



/* DetermineXYFromMapPos.
 *	Input:
 *		maprow = row, mapcol = col.  These are relative positions
 *	on the map, where (0,0) is the upper left.
 *
 *		retx, rety
 *	are return values, actual pixel values on the screen where the
 *	icon belongs.
 */
void
DetermineXYFromMapPos OLARGLIST((screen, maprow, mapcol, retx, rety))
OLARG(Screen *, screen)
OLARG(int, maprow)
OLARG(int, mapcol)
OLARG(int *, retx)
OLGRA(int *, rety)
{
int	screenwidth = WidthOfScreen(screen),
	screenheight = HeightOfScreen(screen);
int	screenstartx, screenendx,
	screenstarty, screenendy;
int	x, y;

	/* First reduce the screen size by the icon margin - it may be
	 * 0, but we can generalize it.
	 */
	ScreenReduction(screen, &screenstartx, &screenendx, &screenstarty,
			&screenendy);

	/* To determine the screen position, we must consider the
	 * iconPlacementMargin resource, XIconIncrement, YIconIncrement,
	 * and combine them with maprow, mapcol, and the gravity (of course).
	 * Where to start:
	 * 	maprow = the row on the screen.
	 *	mapcol = the column.
	 * The position will be affected by the gravity and by the margin.
	 * Consider each area of the screen to contain an icon a "box"
	 * with width = XIconIncrement, ht = YIconIncrement.
	 * RULES:
	 * icon iconmargin != -1, then it must be followed strictly;
	 * otherwise, we can center the icon in the place provided.
	 */
	switch(wmrcs->iconGravity) {
		case WMNorthGravity:
			if (motwmrcs->iconPlacementMargin != -1) {
				/* remember - mapx = the row, mapy = col. */
				*retx = screenstartx + mapcol*XIconIncrement;
				*rety = screenstarty +
					maprow * YIconIncrement;
			}
			else { /* center it */
				*retx = screenstartx + mapcol*XIconIncrement +
					(XIconIncrement - icon_width) / 2;
				*rety = screenstarty +
					maprow * YIconIncrement +
					(YIconIncrement - icon_height)/2;
			}
			break;
		case WMSouthGravity:
			if (motwmrcs->iconPlacementMargin != -1) {
				/* remember - mapx = the row, mapy = col. */
				*retx = screenstartx + mapcol*XIconIncrement;
				*rety = screenstarty -
				   (num_vert_icons - maprow)*YIconIncrement +
					YIconIncrement - icon_height;
			}
			else { /* center it */
				*retx = screenstartx + mapcol*XIconIncrement +
					(XIconIncrement - icon_width) / 2;
				*rety = screenstarty -
					(num_vert_icons - maprow)*
							YIconIncrement +
					(YIconIncrement - icon_height)/2;
			}
			break;
		case WMEastGravity:
			if (motwmrcs->iconPlacementMargin != -1) {
				*retx = screenstartx -
				  (num_horz_icons - mapcol) * XIconIncrement +
				  XIconIncrement - icon_width;
				*rety = screenstarty + maprow * YIconIncrement;
			}
			else { /* center it */
				*retx = screenstartx -
				  (num_horz_icons - mapcol) * XIconIncrement +
				  (XIconIncrement - icon_width)/2;
				*rety = screenstarty + maprow * YIconIncrement
					+ (YIconIncrement - icon_height) / 2;
			}
			break;
		case WMWestGravity:
			if (motwmrcs->iconPlacementMargin != -1) {
				*retx = screenstartx + mapcol * XIconIncrement;
				*rety = screenstarty + maprow * YIconIncrement;
			}
			else { /* center it */
				*retx = screenstartx + mapcol*XIconIncrement +
				  (XIconIncrement - icon_width)/2;
				*rety = screenstarty + maprow * YIconIncrement
					+ (YIconIncrement - icon_height) / 2;
			}
			break;
	} /* switch */
} /* DetermineXYFromMapPos */

/* DetermineMapPosFromXY.
 * Parameters:
 *	Input: (x, y), the screen position.
 *	Return parameters: (retrow, retcol), the row and column on the
 *	map where the item belongs.
 *	Return value:
 *		True, if the space is taken already.
 *		False otherwise.  Some functions, such as a move function
 *		looking for a place to move an icon, may just beep on
 *		return of False.
 */
Boolean
DetermineMapPosFromXY OLARGLIST((screen, x, y, maprow, mapcol))
OLARG(Screen *, screen)
OLARG(int, x)
OLARG(int, y)
OLARG(int *, maprow)
OLGRA(int *, mapcol)
{
int	screenwidth = WidthOfScreen(screen),
	screenheight = HeightOfScreen(screen);
int	screenstartx, screenendx,
	screenstarty, screenendy;
int	halfx = XIconIncrement / 2,
	halfy = YIconIncrement / 2;
int	row, col;
int	colbyte, step;
unsigned int currentbyte;
int	remdr;

	/* What do we have to work with?? 
	 * space_per_icon, both horizontally and vertically: this is
	 * XIconIncrement and YIconIncrement.
	 */
	ScreenReduction(screen, &screenstartx, &screenendx, &screenstarty,
			&screenendy);

	*maprow = *mapcol = -1;
	/* What row?? */
	switch(wmrcs->iconGravity) {
		default:
		case WMSouthGravity:
			/* column? Use 50% tolerance. */
			/* check boundaries */
			if (x < (screenstartx - halfx) ||
					x >(screenendx + halfx) ||
					y > (screenstarty + halfy)  ||
					y < (screenendy - halfy) )
				return(False);
		remdr = x % XIconIncrement;
		col = (x - (x % XIconIncrement) + screenstartx) /
						XIconIncrement;
		/*fprintf(stderr,"Col=%d, remdr = %d\n", col, remdr);*/
		if(remdr > halfx)
			col++;
		if (col >= num_horz_icons)
			return(False);
		remdr =  y % YIconIncrement;
		row = (screenstarty - (screenstarty - y)) / YIconIncrement;
/*
		row = (y - screenstarty - y % YIconIncrement +
				num_vert_icons * YIconIncrement) /
							YIconIncrement;
 */
		/*fprintf(stderr, "row = %d remdr = %d\n", row, remdr);*/
		if (remdr >halfy)
			row++;
		if (row >= num_vert_icons)
			return(False);

			/* Check to see if the position is taken */
			colbyte = col / WORD_BIT;
			step = col % WORD_BIT;
			currentbyte = icon_map[sub(row, colbyte)];
			*maprow = row;
			*mapcol = col;
			if (currentbyte & (1 << (WORD_BIT - col - 1)))
				/* taken */
				return(True);
			else
				return(False);
			break;
		case WMNorthGravity:
		case WMWestGravity:
			/* Both North and west have the same starting
			 * and ending row and col points, therefore
			 * the have the same startx, endx, starty, and
			 * endy positions - only difference is the initial
			 * placement and the direction to follow!
			 */
			if (x < (screenstartx - halfx) ||
					x >(screenendx + halfx) ||
					y < (screenstarty - halfy)  ||
					y > (screenendy + halfy) )
				return(False);
		remdr = x % XIconIncrement;
		col = (x - (x % XIconIncrement) + screenstartx) /
						XIconIncrement;
		if(x > 0 && remdr > halfx)
			col++;
		if (col >= num_horz_icons)
			return(False);
		remdr = y % YIconIncrement;
		/*row = (y - screenstarty - remdr) / YIconIncrement;*/
		row = (y - screenstarty) / YIconIncrement;
		if (y > 0 && remdr > halfy)
			row++;
		if (row >= num_vert_icons)
			return(False);
		if (col >= num_horz_icons)
			return(False);
		colbyte = col / WORD_BIT;
		step = col % WORD_BIT;
		currentbyte = icon_map[sub(row, colbyte)];
		*maprow = row;
		*mapcol = col;
		if (currentbyte & (1 << (WORD_BIT - col - 1)))
			/* taken */
			return(True);
		else
			return(False);
		break;
		case WMEastGravity:
			/* East is the only one that has a unique
			 * startx point - from the right; it's starty
			 * point is similar to both North and West,
			 * so that (the row) can be copied over.
			 */
			if (x > (screenstartx + halfx) ||
					x <(screenendx - halfx) ||
					y < (screenstarty - halfy)  ||
					y > (screenendy + halfy) )
				return(False);
			/* row */
			remdr = y % YIconIncrement;
			row = (y - screenstarty ) / YIconIncrement;
			if (y > 0 && y < screenheight && remdr > halfy)
				row++;
			if (row >= num_vert_icons)
				return(False);
			/* col */
			remdr = x % XIconIncrement;
			/* This is EXTREMELY dangerous - like being heavily
			 * on margin the day of a crash... consider a
			 * screen VGA, 640x480;  XIconIncrement = 71, so
			 * num_horz_icons (e.g., 9) * XIconIncrement = 630,
			 * NOT 640.  The algebraic equation calls for
			 * num_horz_icons * XIconIncrement, but the 1 pixel
			 * we leave out, if there is no margin, forces us
			 * to completely miss a map column on the screen!
			 * Instead of getting column = 4, we'll get 3
			 * always 1 less, because on this one pixel).  The
			 * solution is to use the screenwidth in the equation
			 * if no margin is provided;  this can get complicated
			 * because if there is a margin, then most likely
			 * the increments will get either require some
			 * modification between them (not all icons will
			 * have the same increment), or we may run into
			 * the same situation, because the margin requirement
			 * is a strict one on both sides of the screen in
			 * general.
			 */

			col = (x - screenstartx - remdr +
			  ((motwmrcs->iconPlacementMargin == -1) ?
			   screenwidth : num_horz_icons * XIconIncrement)) /
						XIconIncrement;
			if (x < screenwidth && remdr > halfx)
				col++;
			if (col >= num_horz_icons)
				return(False);
			colbyte = col / WORD_BIT;
			step = col % WORD_BIT;
			currentbyte = icon_map[sub(row, colbyte)];
			*maprow = row;
			*mapcol = col;
			if (currentbyte & (1 << (WORD_BIT - col - 1)))
				/* taken */
				return(True);
			else
				return(False);
			break;
	} /* switch (iconGravity) */
} /* DetermineMapPosFromXY() */	


/*
 * ReleaseMapPosition() - given map position (mapx, mapy), free it in the
 * map by setting it to 0. [(mapx, mapy) is equivalent to (col, row)]
 * 	Row = maprow,
 *	Column = mapcol.
 *	(0,0) is the upper left corner of the screen.
 */
void
ReleaseMapPosition OLARGLIST((maprow, mapcol))
OLARG(int, maprow)
OLGRA(int, mapcol)
{
int	rowword,
	colword;
int	release;
unsigned int	result_bits;

	rowword = maprow;
	colword = mapcol / WORD_BIT;
	release = sub(rowword, colword);
/*fprintf(stderr,"Releasing from map array pos = %d\n", release);*/
	result_bits = (unsigned int) (1 << (WORD_BIT - (mapcol + 1)));
	icon_map[sub(rowword, colword)] &=  ~result_bits;
}

/* ScreenReduction - utility to reduce the screen by the user specified
 * margin.
 */
static void
ScreenReduction OLARGLIST((screen, screenstartx, screenendx,
					screenstarty, screenendy))
OLARG(Screen *, screen)
OLARG(int *, screenstartx)
OLARG(int *, screenendx)
OLARG(int *, screenstarty)
OLGRA(int *, screenendy)
{
int	screenwidth = WidthOfScreen(screen);
int	screenheight = HeightOfScreen(screen);
/*
int	marginstep = (motwmrcs->iconPlacementMargin < 0) ? 0 :
				motwmrcs->iconPlacementMargin;
 */
int	marginstepx;
int	marginstepy;
int	spacex,
	spacey;

	if (motwmrcs->iconPlacementMargin < 0) {
		int remdr;

		remdr = screenwidth - num_horz_icons * XIconIncrement;
		marginstepx = remdr / 2;
		remdr = screenheight - num_vert_icons * YIconIncrement;
		marginstepy = remdr / 2;
	}
	else
		marginstepx = marginstepy = motwmrcs->iconPlacementMargin;
	switch(wmrcs->iconGravity) {
		case WMNorthGravity:
			*screenstartx = marginstepx;
			*screenendx = screenwidth - marginstepx;
			*screenstarty = marginstepy;
			*screenendy = screenheight - marginstepy;
			break;
		case WMSouthGravity:
		default:
			*screenstartx = marginstepx;
			*screenendx = screenwidth - marginstepx;
			*screenstarty = screenheight - marginstepy;
			*screenendy = marginstepy;
			break;
		case WMEastGravity:
			*screenstartx = screenwidth - marginstepx;
			*screenendx = marginstepx;
			*screenstarty = marginstepy;
			*screenendy = screenheight - marginstepy;
			break;
		case WMWestGravity:
			*screenstartx = marginstepx;
			*screenendx = screenwidth - marginstepx;
			*screenstarty = marginstepy;
			*screenendy = screenheight - marginstepy;
			break;
	} /* switch */
} /* ScreenReduction */

extern void
MoveMapPosition OLARGLIST((wm, maprow, mapcol))
OLARG(WMStepWidget, wm)
OLARG(int,	maprow)
OLGRA(int,	mapcol)
{
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
int oldrow = wmstep->icon_map_pos_row;
int oldcol = wmstep->icon_map_pos_col;
int rowword, colword, release;
unsigned int	result_bits;

	if (wmstep->icon_widget) {
		if (oldrow >= 0 && oldrow < num_vert_icons &&
				oldcol >= 0 && oldcol < num_horz_icons)
			ReleaseMapPosition(oldrow, oldcol);
		rowword = maprow;
		colword = mapcol / WORD_BIT;
		release = sub(rowword, colword);
/*fprintf(stderr,"Taking from map array pos = %d\n", release);*/
		result_bits = (unsigned int) (1 << (WORD_BIT - (mapcol + 1)));
		icon_map[sub(rowword, colword)] |= result_bits;
		wmstep->icon_map_pos_row = maprow;
		wmstep->icon_map_pos_col = mapcol;
	} /* wmstep->icon_widget */
} /* MoveMapPosition */

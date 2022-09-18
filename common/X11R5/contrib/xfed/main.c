#ident	"@(#)r4xfed:main.c	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright 1988 by Siemens
 *		All Rights Reserved
 * 
 * please feel free to add any bells and neats to this program -
 * 
 * written 16-17 Mar 1988 by claus gittinger
 *
 * Email: ...!decvax!unido!athen!claus
 *
 * a little font editor by claus
 * usage: xfed [-fg color] [-bg color] 
 *             [-bd color] [-bw number] [-nogrid] [-psize number] fontname.bdf
 */

#include "defs.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <errno.h>

#include "icon.bit"
#include "next.bit"
#include "prev.bit"
#include "save.bit"
#include "quit.bit"

Window XCreateWindow();
Window mainWin, charinfoWin, infoWin, editWin, nextWin, prevWin, quitWin,
			    saveWin, imageWin;
Display *dpy;
Colormap cmap;
GC gc;
int screen;
int winW, winH;
char *borderColor = "black";
char *foregroundColor = "black";
char *backgroundColor = "white";
int borderWidth = 1;
long foregroundPixel, backgroundPixel, borderPixel;
char *fontfilename = NULL;
Pixmap saveButton, nextButton, prevButton, quitButton;
static struct character *charp;
int charIndex = 0;
int firstSave = 1;
int minx, miny, maxx, maxy;

#define SPACE		5
int PIXSIZE = 16;
int grid=1;

#define info_height	20
#define charinfo_height	20

int info_x, info_y;
int info_width;
int charinfo_x, charinfo_y;
int charinfo_width;
int edit_x, edit_y;
int edit_width, edit_height;
int image_x, image_y;
int image_x0, image_nc;
int image_width, image_height;
int next_x, next_y;
int prev_x, prev_y;
int quit_x, quit_y;
int save_x, save_y;
Atom		kill_atom;
Atom		protocol_atom;

#define SETBIT		0
#define CLEARBIT	1
#define COMBIT		2

main(argc, argv)
char *argv[];
{
	char **ap;
	char *cp;

	for (ap=argv; *ap != NULL; ap++) {
	    cp = *ap;
	    if (strcmp(cp, "-bd") == 0)
		borderColor = *++ap;
	    else if (strcmp(cp, "-nogrid") == 0)
		grid = 0;
	    else if (strcmp(cp, "-fg") == 0)
		foregroundColor = *++ap;
	    else if (strcmp(cp, "-bg") == 0)
		backgroundColor = *++ap;
	    else if (strcmp(cp, "-psize") == 0) 
		sscanf(*++ap, "%d", &PIXSIZE);
	    else if (strcmp(cp, "-bw") == 0) 
		sscanf(*++ap, "%d", &borderWidth);
	    else
		fontfilename = *ap;
	}


	if (fontfilename == NULL) usage(argc, argv);

	if (freopen(fontfilename, "r", stdin) == NULL) {
	    fprintf(stderr, "cannot open %s\n", fontfilename);
	    exit(1);
	}
	minx = maxx = miny = maxy = 0;
	yyparse();
#ifdef DEBUG
	printf("Bounds = %d, %d to %d, %d\n", minx, miny, maxx, maxy);
#endif
	setupWindow(argc, argv);
	charp = font.characters; charIndex = 0;
	doEvents();
}

usage(argc, argv) 
char *argv[];
{
	fprintf(stderr, "usage: %s [-fg color] [-bg color] [-bw n] [-bd color]\n", argv[0]);
	fprintf(stderr, "          [-nogrid] [-psize number] file\n");
	exit(1);
}

setupWindow(argc, argv) 
char *argv[];
{
    int winx, winy;
    XGCValues xgcv;
    XSetWindowAttributes xswa;
    char *display;
    Visual visual;
    XSizeHints szhint;
    XColor def, exact;
    extern char *getenv();

	XWMHints 		*wm_hints ;
	XClassHint 		*cl_hints ;
	XTextProperty 	window_name , icon_name ;
	char 			*list1[1] ;

    display = "";
    if (!(dpy = XOpenDisplay(display)))
    {
	perror("Cannot open display\n");
	exit(-1);
    }

    screen = DefaultScreen(dpy);
    cmap = DefaultColormap(dpy, screen);

    XAllocNamedColor(dpy, cmap, borderColor, &def, &exact);
    borderPixel = def.pixel;
    XAllocNamedColor(dpy, cmap, foregroundColor, &def, &exact);
    foregroundPixel = def.pixel;
    XAllocNamedColor(dpy, cmap, backgroundColor, &def, &exact);
    backgroundPixel = def.pixel;

    info_x = info_y = 0;
    charinfo_x = 0;
    charinfo_y = info_y + SPACE + info_height;
    edit_x = 0;
    edit_y = charinfo_y + SPACE + charinfo_height;
    edit_width = (PIXSIZE+grid)*(maxx - minx);
    edit_height = (PIXSIZE+grid)*(maxy - miny);
    prev_x = edit_width + SPACE;
    prev_y = edit_y;
    next_x = prev_x;
    next_y = prev_y + SPACE + prev_height;
    save_x = prev_x;
    save_y = next_y + SPACE + next_height;
    quit_x = prev_x;
    quit_y = save_y + SPACE + save_height;

    image_x = 0;
    image_y = info_height + SPACE + charinfo_height + SPACE + edit_height + SPACE;
    if (image_y < (quit_y+quit_height+SPACE))
	image_y = quit_y + quit_height + SPACE;
    image_width = edit_width + SPACE + prev_width + SPACE;
    image_height = SPACE + font.maxbbx.h + SPACE;
    image_x0 = (image_width - font.maxbbx.w)/2;
    image_nc = (image_x0 - SPACE)/font.maxbbx.w;

    winx = 0;
    winy = 0;
    winW = edit_width + SPACE + prev_width + SPACE;
    winH = image_y + SPACE + font.maxbbx.h + SPACE;

    info_width = winW;
    charinfo_width = winW;
    image_width = winW;

    szhint.flags = PPosition | PSize;
    szhint.x = 0;
    szhint.y = 0;
    szhint.width = winW;
    szhint.height = winH;

    xswa.backing_store = NotUseful;
    xswa.event_mask = ExposureMask;
    xswa.background_pixel = backgroundPixel;
    xswa.border_pixel = borderPixel;
    visual.visualid = CopyFromParent;
    mainWin = XCreateWindow(dpy,
		RootWindow(dpy, DefaultScreen(dpy)),
		winx, winy, szhint.width, szhint.height, borderWidth,
		DefaultDepth(dpy, 0), InputOutput,
		&visual, 
	        CWEventMask | CWBackingStore | CWBorderPixel | CWBackPixel, 
		&xswa);

/*
 *  Setting the standard properties for the communication with the window 
 *  manager according to the new R5 standards . WM_DELETE_WINDOW support has
 *  also been added .
 */
	protocol_atom = XInternAtom(dpy, "WM_PROTOCOLS", False);
	kill_atom = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, mainWin, &kill_atom, 1);

		list1[0] = "Xfed" ;
		XStringListToTextProperty(list1,1,&window_name);
		list1[0] = "Xfed" ;
		XStringListToTextProperty(list1,1,&icon_name);

		if((wm_hints = XAllocWMHints() ) == NULL
				| (cl_hints = XAllocClassHint()) == NULL )
			{
			printf("OOps! Insufficient memory\n");
			return(-1);
			}

		wm_hints->icon_pixmap = XCreateBitmapFromData(dpy, mainWin,
						     	(char*)icon_bits, icon_width, icon_height),
		wm_hints->flags = IconPixmapHint ;
	
	cl_hints->res_name = argv[0] ;
	cl_hints->res_class = "XFed" ;

	XSetWMProperties(dpy, mainWin, &window_name , &icon_name ,
		argv, argc, &szhint,wm_hints, cl_hints);
		XFree(wm_hints);
		XFree(cl_hints);

/*****
    XSetStandardProperties(dpy, mainWin, "fed", "fed",
				XCreateBitmapFromData(dpy, mainWin,
						     (char*)icon_bits,
						     icon_width, icon_height),
				argv, argc, &szhint);
*****/

    XSelectInput(dpy, mainWin, ButtonPressMask|ExposureMask);
    XMapWindow(dpy, mainWin);

    gc = XCreateGC(dpy, mainWin, 0L, &xgcv);
    XSetForeground(dpy, gc, foregroundPixel);
    XSetBackground(dpy, gc, backgroundPixel);

    infoWin = XCreateSimpleWindow(dpy, mainWin,
			    info_x, info_y, info_width, info_height,
			    1, borderPixel, backgroundPixel);
    XSelectInput(dpy, infoWin, ExposureMask);

    charinfoWin = XCreateSimpleWindow(dpy, mainWin,
			    charinfo_x, charinfo_y, charinfo_width, charinfo_height,
			    1, borderPixel, backgroundPixel);
    XSelectInput(dpy, charinfoWin, ExposureMask);

    editWin = XCreateSimpleWindow(dpy, mainWin,
			    edit_x, edit_y, edit_width, edit_height,
			    1, borderPixel, backgroundPixel);
    XSelectInput(dpy, editWin, ExposureMask | ButtonPressMask |
			       Button1MotionMask | Button2MotionMask |
			       Button3MotionMask);
    xswa.cursor = XCreateFontCursor(dpy, XC_cross);
    XChangeWindowAttributes(dpy, editWin, CWCursor, &xswa);

    prevWin = XCreateSimpleWindow(dpy, mainWin,
			    prev_x, prev_y, prev_width, prev_height,
			    1, borderPixel, backgroundPixel);
    XSelectInput(dpy, prevWin, ExposureMask | ButtonPressMask);
    prevButton = XCreateBitmapFromData(dpy, mainWin,
			       (char *)prev_bits, prev_width, prev_height);

    nextWin = XCreateSimpleWindow(dpy, mainWin,
			    next_x, next_y, next_width, next_height,
			    1, borderPixel, backgroundPixel);
    XSelectInput(dpy, nextWin, ExposureMask | ButtonPressMask);
    nextButton = XCreateBitmapFromData(dpy, mainWin,
			       (char*)next_bits, next_width, next_height);

    saveWin = XCreateSimpleWindow(dpy, mainWin,
			    save_x, save_y, save_width, save_height,
			    1, borderPixel, backgroundPixel);
    XSelectInput(dpy, saveWin, ExposureMask | ButtonPressMask);
    saveButton = XCreateBitmapFromData(dpy, mainWin,
			       (char*)save_bits, save_width, save_height);

    quitWin = XCreateSimpleWindow(dpy, mainWin,
			    quit_x, quit_y, quit_width, quit_height,
			    1, borderPixel, backgroundPixel);
    XSelectInput(dpy, quitWin, ExposureMask | ButtonPressMask);
    quitButton = XCreateBitmapFromData(dpy, mainWin,
			       (char*)quit_bits, quit_width, quit_height);

    imageWin = XCreateSimpleWindow(dpy, mainWin,
			    image_x, image_y, image_width, image_height,
			    1, borderPixel, backgroundPixel);
    XSelectInput(dpy, imageWin, ExposureMask | ButtonPressMask);
    xswa.cursor = XCreateFontCursor(dpy, XC_arrow);
    XChangeWindowAttributes(dpy, imageWin, CWCursor, &xswa);

    XMapWindow(dpy, infoWin);
    XMapWindow(dpy, charinfoWin);
    XMapWindow(dpy, editWin);
    XMapWindow(dpy, prevWin);
    XMapWindow(dpy, nextWin);
    XMapWindow(dpy, saveWin);
    XMapWindow(dpy, quitWin);
    XMapWindow(dpy, imageWin);
}

doEvents() 
{
    XEvent pe;
    XExposeEvent *ee = (XExposeEvent *) &pe;
    XConfigureEvent *ce = (XConfigureEvent *)&pe;
    XButtonPressedEvent *be = (XButtonPressedEvent *)&pe;
    XMotionEvent *me = (XMotionEvent *)&pe;
    int last_row, last_col;
    int this_row, this_col;

    while(1) {
	XNextEvent(dpy, &pe);       /* this should get first exposure event */
	switch (pe.type) {
		case ClientMessage:
		    {
		     XClientMessageEvent	*ev = (XClientMessageEvent *)&pe;

		     if (ev->message_type == protocol_atom && 
					ev->data.l[0] == kill_atom)
			 {
				XFreeGC (dpy, gc);
				XDestroyWindow (dpy, mainWin);
				XCloseDisplay (dpy);
				exit(0);
			 }
		    }
		    break;
	    case Expose:
		if (ee->window == editWin)
		    redrawEditWindow();
		else if (ee->window == prevWin)
		    redrawPrevWindow();
		else if (ee->window == nextWin)
		    redrawNextWindow();
		else if (ee->window == infoWin)
		    redrawInfoWindow();
		else if (ee->window == charinfoWin)
		    redrawCharinfoWindow();
		else if (ee->window == quitWin)
		    redrawQuitWindow();
		else if (ee->window == saveWin)
		    redrawSaveWindow();
		else if (ee->window == imageWin)
		    redrawImageWindow();
		break;

	    case ButtonPress:
		if (be->window == editWin) {
		    if (whatSquare(be->x, be->y, &last_row, &last_col) == -1)
			break;
		    switch (be->button) {
			case Button1:	/* left button */ 
		            bitOp(SETBIT, last_row, last_col);
			    break;
			case Button2:	/* middle button */ 
		            bitOp(COMBIT, last_row, last_col);
			    break;
			case Button3:	/* right button */ 
		            bitOp(CLEARBIT, last_row, last_col);
			    break;
		    }
		} else if (be->window == imageWin)
		    changeCharacter(be->x);
		else if (be->window == prevWin)
		    prevCharacter();
		else if (be->window == nextWin)
		    nextCharacter();
		else if (be->window == saveWin) {
		    char cmdbuff[132];

		    if (firstSave) {
			firstSave = 0;
		        sprintf(cmdbuff, "cp %s %s~", fontfilename, fontfilename);
		        system(cmdbuff);
		    }
		    output(&font, fontfilename);
		} else if (be->window == quitWin)
		  {

/*
 *  Quitting decently.
 */
				XFreeGC (dpy, gc);
				XDestroyWindow (dpy, mainWin);
				XCloseDisplay (dpy);

				return 0;
		  }
		break;

    	    case MotionNotify:
		if (me->window != editWin) break;
      		if (whatSquare (me->x, me->y, &this_row, &this_col))
        	    break;  
      		if ((this_row != last_row) || (this_col != last_col)) {
       	  	    switch (me->state) {
	    		case Button1Mask:	/* left button down */
	      		    bitOp(SETBIT, this_row, this_col);
	      		    break;
	    		case Button2Mask:	/* middle button down */
	      		    bitOp(COMBIT, this_row, this_col);
	      		    break;
	    		case Button3Mask:	/* right button down */
	      		    bitOp(CLEARBIT, this_row, this_col);
	      		    break;
		    }
      		    last_row = this_row;
      		    last_col = this_col;
		}
      		break;
  
	    default:
		break;
	}
    }
}

/* 
 * !! This bounding check makes xfed useful only for fine editing of
 * fonts, since you just cannot extend the font bbox on two sides. We
 * should fix bitOp to move things around so it can be made to grow in
 * the negative direction as well, and update the bbx appropriately
 */
/*
 * convert mouse x/y to row/col and check for pixel in bounding box
 * return -1 if illegal.
 */
whatSquare(x, y, prow, pcol)
int *prow, *pcol;
{
	int row, col;

	row = (edit_height - y) / (PIXSIZE+grid);
	*prow = charp->bbx.y - miny + charp->bbx.h - row - 1;
	col = x / (PIXSIZE+grid);
	*pcol = col - charp->bbx.x + minx;
	/* can't handle box growing in the negative direction. sigh */
	if (*prow < 0 || *pcol < 0) return -1;
	/* extending the rows is confusing enough */
#ifndef EXTENDROWS
	if (*prow >= charp->nrows) return -1;
#endif
	if (*prow >= charp->bbx.h) charp->bbx.h = *prow + 1;
	if (*pcol >= charp->bbx.w) charp->bbx.w = *pcol + 1;
	return 0;
}

/*
 * set/clear/complement a pixel in current character - row/col must be checked
 * elsewhere
 */
int bitmask[] = { 8, 4, 2, 1 };

bitOp(what, row, col) {
	char *rowbits;
	int bits;
	char fourbits[2];

#ifdef EXTENDROWS
	/* extend rows if needed */
	if (row >= charp->nrows) {
	    int i, j;
	    unsigned int clen = strlen(charp->rows[0]);

	    charp->rows = (char **) realloc((char *) charp->rows,
			  (unsigned)((row + 1) * (sizeof(char *))));
	    for(i = charp->nrows; i < (row + 1); i++) {
		charp->rows[i] = malloc(clen + 1);
		for(j = 0; j < clen; j++)
		    charp->rows[i][j] = '0';
		charp->rows[i][clen] = '\0';
	    }
	    charp->nrows = row + 1;
	};
#endif
	rowbits = charp->rows[row];
	/* must we extend bitmap-line ? */
	if (col >= (int)strlen((char *)rowbits)*4) {
	    int j;

	    charp->rows[row] = (char *)realloc(rowbits, (unsigned) (col/4 + 2));
	    rowbits = charp->rows[row];
	    for(j = strlen(rowbits); j < (col / 4 + 1); j++)
		rowbits[j] = '0';
	    rowbits[col/4 + 1] = '\0';
	};
	rowbits += col/4;
	fourbits[0] = *rowbits;
	fourbits[1] = '\0';
	sscanf(fourbits, "%x", &bits);
	switch (what) {
	    case SETBIT:
		bits |= bitmask[col % 4];
		break;
	    case CLEARBIT:
		bits &= ~bitmask[col % 4];
		break;
	    case COMBIT:
		bits ^= bitmask[col % 4];
		break;
	}
	sprintf(fourbits, "%x", bits);
	*rowbits = fourbits[0];
	if (bits & bitmask[col % 4]) {
	    SetSquare(row, col);
	    XFillRectangle(dpy, imageWin, gc,
			col + image_x0, row + SPACE, 1, 1);
	} else {
	    ClearSquare(row, col);
	    XClearArea(dpy, imageWin,
			col + image_x0, row + SPACE, 1, 1, 0);
	}

	/*
	 * redraw bounding box marker if scratched
	 */
	if (row == charp->bbx.h || col == charp->bbx.w || row == 0 || col == 0)
	    DrawBBox();
	DrawOrig();
}

SetSquare(row, col)
{
    XFillRectangle(dpy, editWin, gc, 
	(charp->bbx.x - minx + col)*(PIXSIZE+grid), 
	edit_height - (charp->bbx.y - miny + charp->bbx.h - row)*(PIXSIZE+grid), 
	PIXSIZE, PIXSIZE);
}


ClearSquare(row, col)
{
    XClearArea(dpy, editWin,
	(charp->bbx.x - minx + col)*(PIXSIZE+grid), 
	edit_height - (charp->bbx.y - miny + charp->bbx.h - row)*(PIXSIZE+grid), 
	PIXSIZE, PIXSIZE, 0);
}


DrawBBox()
{
    int x1, y1;

    x1 = charp->bbx.x - minx;
    y1 = charp->bbx.y - miny + charp->bbx.h;
    XSetForeground(dpy, gc, borderPixel);
    XDrawRectangle(dpy, editWin, gc,
        x1 * (PIXSIZE+grid), edit_height - y1 * (PIXSIZE+grid), 
	charp->bbx.w * (PIXSIZE+grid), charp->bbx.h * (PIXSIZE+grid));
    XSetForeground(dpy, gc, foregroundPixel);
}


DrawOrig()
{
    XSetFunction(dpy, gc, GXinvert);
    XDrawArc(dpy, editWin, gc, 
    	(-minx) * (PIXSIZE+grid) - PIXSIZE / 2, 
	edit_height + miny * (PIXSIZE + grid) - PIXSIZE / 2,
	(unsigned) PIXSIZE, (unsigned) PIXSIZE, 0, 360 * 64);
    XSetFunction(dpy, gc, GXcopy);
}


changeCharacter(x) {
	int delta, index;

	delta = (x - SPACE)/font.maxbbx.w - image_nc;
	index = charIndex + delta;
	if (index < 0)
	    index = 0;
	if (index >= font.nchars)
	    index = font.nchars - 1;
	if ((delta = index - charIndex) == 0)
		return;
	charIndex += delta;
	charp += delta;
	redrawEditWindow();
	redrawCharinfoWindow();
	redrawImageWindow();
}

nextCharacter() {
	if ((charIndex+1) < font.nchars) {
	    charp++;
	    charIndex++;
	    redrawEditWindow();
	    redrawCharinfoWindow();
	    redrawImageWindow();
	}
}

prevCharacter() {
	if (charIndex > 0) {
	    charp--;
	    charIndex--;
	    redrawEditWindow();
	    redrawCharinfoWindow();
	    redrawImageWindow();
	}
}

redrawEditWindow() {
	int row, col;
	char *rowbits;
	char fourbits[2];
	int bits;
	int bound;

	XClearWindow(dpy, editWin);

#ifdef DEBUG
	printf("%s: encoding = %d, x, y = %d, %d, w, h = %d, %d\n",
	 charp->charId, charp->encoding, charp->bbx.x, charp->bbx.y,
	 charp->bbx.w, charp->bbx.h);
#endif
	if (grid) {
	    XSetForeground(dpy, gc, borderPixel);
	    bound = maxy - miny;
	    for (row=1; row<bound; row++) {
	        XDrawLine(dpy, editWin, gc,
			    0, row*(PIXSIZE+grid)-1, 
			    edit_width, row*(PIXSIZE+grid)-1);
	    }

	    bound = maxx - minx;
	    for (col=1; col<bound; col++) {
	        XDrawLine(dpy, editWin, gc,
			    col*(PIXSIZE+grid)-1, 0,
			    col*(PIXSIZE+grid)-1, edit_height);
	    }
	    XSetForeground(dpy, gc, foregroundPixel);
	}

	fourbits[1] = '\0';
	for (row=0; row<charp->nrows; row++) {
	    rowbits = charp->rows[row];
	    for (col=0; col < charp->bbx.w; col++) {
		if (col % 4 == 0) {
	            fourbits[0] = rowbits[0];
	            sscanf(fourbits, "%x", &bits);
		    rowbits++;
		}
		if (bits & bitmask[col % 4])
		    SetSquare(row, col);
	    }
	}
	DrawBBox();
	DrawOrig();
}

redrawImageWindow() {
	int row, col;
	int x, cIndex;
	char *rowbits;
	int bits;
	struct character *cp;

	XClearWindow(dpy, imageWin);
	cp = charp - image_nc;
	cIndex = charIndex - image_nc;
	for (x = image_x0 - image_nc*font.maxbbx.w;
			    x<(winW - SPACE) && cIndex<font.nchars;
			    x += font.maxbbx.w, cp++, cIndex++) {
	    if (cIndex < 0) continue;
	    for (row = 0; row<cp->nrows; row++) {
		rowbits = cp->rows[row];
		for (col = 0; *rowbits; rowbits++) {
		    if ((bits = *rowbits - '0') > 9 || bits < 0) {
			if (*rowbits >= 'A' && *rowbits <= 'F')
			    bits = *rowbits - 'A' + 10;
			else if (*rowbits >= 'a' && *rowbits <= 'f')
			    bits = *rowbits - 'a' + 10;
		    }
		    if ((bits & bitmask[0]) && col < cp->bbx.w)
			XDrawPoint(dpy, imageWin, gc, x + col, row + SPACE);
		    col++;
		    if ((bits & bitmask[1]) && col < cp->bbx.w)
			XDrawPoint(dpy, imageWin, gc, x + col, row + SPACE);
		    col++;
		    if ((bits & bitmask[2]) && col < cp->bbx.w)
			XDrawPoint(dpy, imageWin, gc, x + col, row + SPACE);
		    col++;
		    if ((bits & bitmask[3]) && col < cp->bbx.w)
			XDrawPoint(dpy, imageWin, gc, x + col, row + SPACE);
		    col++;
		}
	    }
	}
}

redrawPrevWindow() {
	redrawButtonInWindow(prevButton, prevWin, prev_width, prev_height);
}

redrawNextWindow() {
	redrawButtonInWindow(nextButton, nextWin, next_width, next_height);
}

redrawSaveWindow() {
	redrawButtonInWindow(saveButton, saveWin, save_width, save_height);
}

redrawQuitWindow() {
	redrawButtonInWindow(quitButton, quitWin, quit_width, quit_height);
}

redrawButtonInWindow(pixmap, win, w, h)
Pixmap pixmap;
Window win;
{
	XClearWindow(dpy, win);
	XCopyPlane(dpy, pixmap, win, gc, 0, 0, w, h, 0, 0, 1);
}

redrawInfoWindow() {
	char buffer[200];

	sprintf(buffer, "File: %s Name: '%s' %d Characters                       ", 
					fontfilename, font.fontname, font.nchars);
	XDrawImageString(dpy, infoWin, gc,
			  5, 13,
			  buffer, strlen(buffer));

}

redrawCharinfoWindow() {
	char buffer[200];

	sprintf(buffer, "Character: '%s' Code: %x                        ",
			charp->charId, charp->encoding);
	XDrawImageString(dpy, charinfoWin, gc,
			  5, 13,
			  buffer, strlen(buffer));

}


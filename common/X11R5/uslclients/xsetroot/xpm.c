/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)r4xsetroot:xpm.c	1.1"
#endif

/* Copyright 1989 GROUPE BULL -- See licence conditions in file COPYRIGHT */
/*****************************************************************************
 ****  Read/Write package for XPM file format (X PixMap) 
 ****
 ****  Pixmap XCreatePixmapFromData(dpy, d, cmap, w, h, depth, n, c, col, pix)
 ****  int    XReadPixmapFile(dpy, d, cmap, filename, w_return, h_return,
 ****                                                     depth, pixmap_return)
 ****  int    XWritePixmapFile(dpy, cmap, filename, pixmap, w, h)
 ****
 ****  Daniel Dardailler - Bull RC (89/02/22) e-mail: daniel@mirsa.inria.fr
 *****************************************************************************
 ****  Version 1.1:  extended chars_per_pixel support... [ Read, Create ]
 ****
 ****  Richard Hess - Consilium    (89/11/06) e-mail: ..!uunet!cimshop!rhess
 *****************************************************************************
 ****  Version 1.2:  improved file and storage mgmt, support for writing 1 cpp
 ****
 ****  James Bash - AT&T Bell Labs (89/11/25) e-mail: jmb@attunix.att.com
 *****************************************************************************
 ****  Version "2.0":  Read support for NAME_mono[] option...
 ****
 ****  Richard Hess - Consilium    (89/11/30) e-mail: ..!uunet!cimshop!rhess
 *****************************************************************************/

/*****************************************************************************
 **** Look of XPM file : .. something like X11 'C includable' format

#define drunk_format 1
#define drunk_width 18
#define drunk_height 21
#define drunk_ncolors 4
#define drunk_chars_per_pixel 2
static  char * drunk_colors[] = {
"  " , "#FFFFFFFFFFFF",
". " , "#A800A800A800",
"X " , "White",
"o " , "#540054005400"  
} ;
static char * drunk_pixels[] = {
"                                    ",
"                                    ",
"            . . . . . . .           ",
"          X         . . . .         ",
"        X     X       . . . .       ",
"      o         X       . . .       ",
"    o o     X           . . . .     ",
"  o o o               . . . . .     ",
"o o o               . . . . . .     ",
"o o o                   . . . .     ",
"  X                 X   . . .       ",
"  X   X               . . . .       ",
"    X               . . . .         ",
"    X                 . .           ",
"      X                   X X X     ",
"        X X X               X   X   ",
"              X           X X       ",
"            X X X       X X         ",
"          X       X X X             ",
"      X X                           ",
"                                    " 
} ;

******************************************************************************
*  Version 1.1 can handle either 1 or 2 chars per pixel
*  - for each different color : n chars can represent the pixel value 
*    and is associed with red, green and blue intensity or colorname.
******************************************************************************/

/*
 * From arpa!mirsa.inria.fr!Daniel.Dardailler Wed Nov 15 16:27:25 +0100 1989
 * Date: Wed, 15 Nov 89 16:27:25 +0100
 * From: Daniel Dardailler <Daniel.Dardailler@mirsa.inria.fr>
 * To: jmb@attunix.att.com
 * Subject: XPM
 * 
 * What is really meaned by this copyright is that:
 * 
 * Like the MIT distribution of the X Window System,it is publicly available,
 * but is NOT in the public domain (my fault).
 * The difference is that copyrights granting rights
 * for unrestricted use and redistribution have been placed on all of the
 * software to identify its authors.
 * You are allowed and encouraged to take
 * this software and build commercial products.
 * 
 * GROUPE BULL will let you use XPM as long as you do not pretend to have
 * written it.
 * 
 * You are also encouraged to re-distribute it freely with the same behaviour.
 * bye
 * 
 *    Daniel Dardailler                   |      Email : daniel@mirsa.inria.fr
 *    BULL  Centre de Sophia Antipolis    |      Phone : (33) 93 65 77 71
 *          2004, Route des Lucioles      |      Telex :      97 00 50 F
 *          06565 Valbonne CEDEX  France  |      Fax   : (33) 93 65 77 66
 */

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>

#include "xpm.h"      /* PixmapOpenFailed, PixmapSuccess .. MAXPRINTABLE */

/* Unused routine from the pristine xpm.c removed here... */

#define MAX_LINE_LENGTH		2048

static char * getline();


/* Unused routine from the pristine xpm.c removed here... */


/**[ XReadPixmapFile ]********************************************************
 *
 *  Read a Pixmap file in a X Pixmap with specified depth and colormap...
 * (THIS VERSION SUPPORTS : '_mono[]' [ optional ] )
 * (THIS VERSION SUPPORTS : '_chars_per_pixel 1', '_chars_per_pixel 2')
 *****************************************************************************/

int XReadPixmapFile (display,d,
		     colormap,filename,width,height,depth,pixmap)
     Display *display;
     Drawable d;
     Colormap colormap ;
     char *filename;
     unsigned int *width, *height;       /* RETURNED */
     unsigned int depth ;   
     Pixmap *pixmap;                     /* RETURNED */
{
  GC Gc = NULL;
  XGCValues xgcv;

  FILE *fstream = NULL;			/* handle on file  */
  char linebuf[MAX_LINE_LENGTH] ;
  char namebuf[80];
  char name[80];
  char type[40];

  int ncolors ;

  CmapEntry * cmap = NULL;
  int * Tpixel = NULL;

  char c1, c2, c;
  int red, green, blue ;
  XColor xcolor ;
  int i,j,p,n;
  int cpp;
  int mono = 0;
  

  /* cleanup and return macro */
#undef RETURN
#define RETURN(code, flag) \
	{ if (cmap) free (cmap); if (Tpixel) free (Tpixel); \
		if (Gc) XFreeGC(display, Gc); \
		if (flag && *pixmap) { XFreePixmap(display, *pixmap); \
							*pixmap = NULL; } \
		if (fstream) fclose (fstream); return (code); }

  if ((fstream = fopen(filename, "r")) == NULL) {
    RETURN (PixmapOpenFailed, 0);
  }

  getline(linebuf,fstream);
  if ((sscanf(linebuf, "#define %[^_]%s %d", namebuf, type, &p) != 3) 
      || ((strcmp("_format",type)) && (strcmp("_paxformat",type)))
      || (p != XPM_FORMAT)) {
    RETURN (PixmapFileInvalid, 0);    /* be silent about it at first */
  } else
    strcpy(name,namebuf);

  getline(linebuf,fstream);
  if ((sscanf(linebuf, "#define %[^_]%s %d", namebuf, type, width) != 3)
      || (strcmp(name,namebuf)) 
      || (strcmp("_width",type))) 
	RETURN (fatal("bad '#define NAME_width n'"), 0);

  getline(linebuf,fstream);
  if ((sscanf(linebuf, "#define %[^_]%s %d", namebuf, type, height) != 3)
      || (strcmp(name,namebuf)) 
      || (strcmp("_height",type))) 
	RETURN (fatal("bad '#define NAME_height n'"), 0);

  getline(linebuf,fstream);
  if ((sscanf(linebuf, "#define %[^_]%s %d", namebuf, type, &ncolors) != 3)
      || (strcmp(name,namebuf)) 
      || (strcmp("_ncolors",type))) 
	RETURN (fatal("bad '#define NAME_ncolors n'"), 0);

  if (ncolors > (MAXPRINTABLE*MAXPRINTABLE)) 
    RETURN (fatal("Too many different colors, version 1"), 0);

  getline(linebuf,fstream);
  if ((sscanf(linebuf, "#define %[^_]%s %d", namebuf, type, &cpp) != 3)
      || (strcmp(name,namebuf)) || (cpp < 1) || (cpp > 2)
      || (strcmp("_chars_per_pixel",type)))
	RETURN (fatal("bad '#define NAME_chars_per_pixel n' [1][2]"), 0);

  for (n=0; n<2 ; n++) {
    if (n == 0) {
      getline(linebuf,fstream);
      if ((sscanf(linebuf, "static char * %[^_]%s = {",namebuf,type) != 2)
	  || (strcmp(name,namebuf))
	  || (strcmp("_mono[]",type))) continue;
      if (depth != 1) {
	for (i=0; i<=ncolors ; i++) {
	  getline(linebuf,fstream);
	}
	if (strncmp(linebuf, "} ;",3))
	  RETURN (fatal("missing '} ;' in NAME_mono[]"), 0);
	getline(linebuf,fstream);
	continue;
      }
      mono = 1;
    }
    else {
      if (mono) getline(linebuf, fstream);
      if ((sscanf(linebuf, "static char * %[^_]%s = {",namebuf,type) != 2)
	  || (strcmp(name,namebuf))
	  || (strcmp("_colors[]",type))) 
	RETURN (fatal("bad 'static char * NAME_colors[] = {'"), 0);
      if (mono) {
	for (i=0; i<=ncolors ; i++) {
	  getline(linebuf,fstream);
	}
	if (strncmp(linebuf, "} ;",3))
	  RETURN (fatal("missing '} ;'"), 0);
	continue;
      }
    }
    cmap = (CmapEntry *) malloc(ncolors*sizeof(CmapEntry)) ;
    Tpixel = (int *) malloc(ncolors*sizeof(int)) ;
    if ((cmap == NULL) || (Tpixel == NULL))
      RETURN (PixmapNoMemory, 0) ;

    getline(linebuf,fstream);
    for (i=0; i<ncolors ; i++) {
      switch (cpp)
	{
	case 1:
	  if (sscanf(linebuf, "\"%c\" , \"%[^\"]%s",
		     &c1,namebuf,type) != 3)
	    RETURN
	    (fatal("bad colormap entry : must be '\"c\" , \"colordef\",'"), 0);
	  if (index(printable,c1)) {
	    cmap[i].cixel.c1 = c1 ;
	  } else
	    RETURN (fatal("bad cixel value : must be printable"), 0);
	  break;
	case 2:
	  if (sscanf(linebuf, "\"%c%c\" , \"%[^\"]%s",
		     &c1,&c2,
		     namebuf,type) != 4)
	    RETURN
	    (fatal("bad colormap entry : must be '\"cC\" , \"colordef\",'"), 0);
	  if ((index(printable,c1)) &&
	      (index(printable,c2))) {
	    cmap[i].cixel.c1 = c1 ;
	    cmap[i].cixel.c2 = c2 ;
	  } else
	    RETURN (fatal("bad cixel value : must be printable"), 0);
	  break;
	}
      if (!XParseColor(display,colormap,namebuf,&xcolor))
	RETURN (fatal("bad colordef specification : #RGB or colorname"), 0);
      XAllocColor(display,colormap,&xcolor);
      Tpixel[i] = xcolor.pixel ;
      getline(linebuf,fstream);
    }
    if (strncmp(linebuf, "} ;",3))
      RETURN (fatal("missing '} ;'"), 0);
  }

  getline(linebuf,fstream);
  if ((sscanf(linebuf, "static char * %[^_]%s = {",namebuf,type) != 2)
      || (strcmp(name,namebuf))
      || (strcmp("_pixels[]",type))) 
	RETURN (fatal("bad 'static char * NAME_pixels[] = {'"), 0);

  *pixmap = XCreatePixmap(display,d,*width,*height,depth);
  Gc = XCreateGC(display,*pixmap,0,&xgcv);
  
  getline(linebuf,fstream);
  j = 0 ;
  while((j < *height) && strncmp(linebuf, "} ;",3))
    {  
      if (strlen(linebuf) < (cpp*(*width)+2)) 
	RETURN (fatal("pixmap line length %d exceeds maximum of %d",
					cpp*(*width)+2, strlen(linebuf)), 1);
      switch (cpp)
	{
	case 1:
	  for (i=1; i<=(*width) ; i++)
	    {
	      c1 = linebuf[i] ;
	      for (p = 0 ; p < ncolors ; p++)
		if (cmap[p].cixel.c1 == c1) break ;
	      if (p != ncolors)
		XSetForeground(display,Gc,Tpixel[p]);
	      else 
		RETURN (fatal("cixel \"%c\" not in previous colormap",c1), 1);
	      XDrawPoint(display,*pixmap,Gc,i-1,j) ;
	    }
	  break;
	case 2:
	  for (i=1; i<(2*(*width)) ; i+=2)
	    {
	      c1 = linebuf[i] ;
	      c2 = linebuf[i+1] ;
	      for (p = 0 ; p < ncolors ; p++)
		if ((cmap[p].cixel.c1 == c1)&&(cmap[p].cixel.c2 == c2)) break ;
	      if (p != ncolors)
		XSetForeground(display,Gc,Tpixel[p]);
	      else 
		RETURN
		  (fatal("cixel \"%c%c\" not in previous colormap",c1,c2), 1);
	      XDrawPoint(display,*pixmap,Gc,i/2,j) ;
	    }
	  break;
	}
      j++ ;
      getline(linebuf,fstream);    
    }

  if (strncmp(linebuf, "} ;",3))
    RETURN (fatal("missing '} ;'"), 1);

  if (j != *height)
    RETURN (fatal("%d too few pixmap lines", *height - j), 1);

  RETURN (PixmapSuccess, 0) ;
}


/* Unused routine from the pristine xpm.c removed here... */


/****[ UTILITIES ]************************************************************
 * following routines are used in XReadPixmapFile() function 
 *****************************************************************************/

/*
 * read the next line and jump blank lines 
 */
static char *
getline(s,pF)
     char * s ;
     FILE * pF ;
{
    s = fgets(s,MAX_LINE_LENGTH,pF);

    while (s) {
	int len = strlen(s);
	if (len && s[len-1] == '\015')
	    s[--len] = '\0';
	if (len==0) s = fgets(s,MAX_LINE_LENGTH,pF);
	else break;
    }
    return(s);
}
	    

/*
 * fatal message : return code, no exit 
 */
static int fatal(msg, p1, p2, p3, p4)
    char *msg;
{
    fprintf(stderr,"\n");
    fprintf(stderr, msg, p1, p2, p3, p4);
    fprintf(stderr,"\n");
    return PixmapFileInvalid ;
}


/* Unused routines from the pristine xpm.c removed here... */


/****<eof>********************************************************************/

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:xpixmap.c	1.1"
/* xpixmap.c:
 *
 * XPixMap format file read and identify routines.  these can handle any
 * "format 1" XPixmap file with up to BUFSIZ - 1 chars per pixel.  it's
 * not nearly as picky as it might be.
 *
 * unlike most image loading routines, this is X specific since it
 * requires X color name parsing.  to handle this we have global X
 * variables for display and screen.  it's ugly but it keeps the rest
 * of the image routines clean.
 *
 * Copyright 1989 Jim Frost.  See included file "copyright.h" for complete
 * copyright information.
 */

#include "copyright.h"
#include "xloadimage.h"

char *rindex();

extern Display *Disp; /* X display, null if in "identify" mode */
extern int      Scrn; /* X screen number */

#define XPM_FORMAT 1

static void corrupted(fullname, zf)
     char  *fullname;
     ZFILE *zf;
{
  zclose(zf);
  printf("%s: X Pixmap file is corrupted\n", fullname);
  exit(1);
}

Image *xpixmapLoad(fullname, name, verbose)
     char         *fullname, *name;
     unsigned int  verbose;
{ ZFILE         *zf;
  char           buf[BUFSIZ];
  char           what[BUFSIZ];
  char          *p;
  char          *imagetitle;
  unsigned int   value;
  unsigned int   format;  /* image format */
  unsigned int   w, h;    /* image dimensions */
  unsigned int   cpp;     /* chars per pixel */
  unsigned int   ncolors; /* number of colors */
  unsigned int   depth;   /* depth of image */
  char         **ctable;  /* color table */
  Image         *image;
  XColor         xcolor;
  unsigned int   a, b, x, y;
  int            c;
  byte          *dptr;

  if (! (zf= zopen(fullname)))
    return(NULL);

  /* read #defines until we have all that are necessary or until we
   * get an error
   */

  format= w= h= ncolors= 0;
  for (;;) {
    if (! zgets(buf, BUFSIZ - 1, zf)) {
      zclose(zf);
      return(NULL);
    }
    if (!strncmp(buf, "#define", 7)) {
      if (sscanf(buf, "#define %s %d", what, &value) != 2) {
	zclose(zf);
	return(NULL);
      }
      if (! (p= rindex(what, '_')))
	p= what;
      else
	p++;
      if (!strcmp(p, "format"))
	format= value;
      else if (!strcmp(p, "width"))
	w= value;
      else if (!strcmp(p, "height"))
	h= value;
      else if (!strcmp(p, "ncolors"))
	ncolors= value;

      /* this one is ugly
       */

      else if (!strcmp(p, "pixel")) { /* this isn't pretty but it works */
	if (p == what)
	  continue;
	*(--p)= '\0';
	if (!(p= rindex(what, '_')) || (p == what) || strcmp(++p, "per"))
	  continue;
	*(--p)= '\0';
	if (!(p= rindex(what, '_')))
	  p= what;
	if (strcmp(++p, "chars"))
	  continue;
	cpp= value;
      }
    }
    else if ((sscanf(buf, "static char * %s", what) == 1) &&
	     (p= rindex(what, '_')) && !strcmp(++p, "colors[]"))
      break;
  }

  if ((format != XPM_FORMAT) || !w || !h || !ncolors || !cpp) {
    zclose(zf);
    return(NULL);
  }

  if (p= rindex(what, '_')) {     /* get the name in the image if there is */
    *p= '\0';                     /* one */
    imagetitle= dupString(what);
  }
  else {
    p= what;
    imagetitle= dupString(name);
  }

  if (verbose)
    printf("%s is a %dx%d X Pixmap image with %d colors titled '%s'\n",
	   name, w, h, ncolors, imagetitle);

  for (depth= 1, value= 2; value < ncolors; value <<= 1, depth++)
    ;
  image= newRGBImage(w, h, depth);
  image->rgb.used= ncolors;
  image->title= dupString(imagetitle);

  /* read the colors array and build the image colormap
   */

  ctable= (char **)lmalloc(sizeof(char *) * ncolors);
  xcolor.flags= DoRed | DoGreen | DoBlue;
  for (a= 0; a < ncolors; a++) {
 
    /* read pixel value
     */

    *(ctable + a)= (char *)lmalloc(cpp);
    while (((c= zgetc(zf)) != EOF) && (c != '"'))
      ;
    if (c == EOF)
      corrupted(fullname, zf);
    for (b= 0; b < cpp; b++) {
      if ((c= zgetc(zf)) == '\\')
	c= zgetc(zf);
     if (c == EOF)
	corrupted(fullname, zf);
      *(*(ctable + a) + b)= (char)c;
    }
    if (((c= zgetc(zf)) == EOF) || (c != '"'))
      corrupted(fullname, zf);

    /* read color definition and parse it
     */

    while (((c= zgetc(zf)) != EOF) && (c != '"'))
      ;
    if (c == EOF)
      corrupted(fullname, zf);
    for (b= 0; ((c= zgetc(zf)) != EOF) && (c != '"'); b++) {
      if (c == '\\')
	c= zgetc(zf);
      if (c == EOF)
	corrupted(fullname, zf);
      buf[b]= (char)c;
    }
    buf[b]= '\0';

    if (Disp) {
      if (! XParseColor(Disp, DefaultColormap(Disp, Scrn), buf, &xcolor)) {
	printf("%s: %s: Bad color name\n", fullname, buf);
	exit(1);
      }
      *(image->rgb.red + a)= xcolor.red;
      *(image->rgb.green + a)= xcolor.green;
      *(image->rgb.blue + a)= xcolor.blue;
    }
  }

  for (;;) {
    if (! zgets(buf, BUFSIZ - 1, zf))
      corrupted(fullname, zf);
    if (sscanf(buf, "static char * %s", what) == 1)
      break;
  }

  if (p= rindex(what, '_'))
    p++;
  else
    p= what;
  if (strcmp(p, "pixels[]"))
    corrupted(fullname, zf);

  /* read in image data
   */

  dptr= image->data;
  for (y= 0; y < h; y++) {
    while (((c= zgetc(zf)) != EOF) && (c != '"'))
      ;
    for (x= 0; x < w; x++) {
      for (a= 0; a < cpp; a++) {
	if ((c= zgetc(zf)) == '\\')
	  c= zgetc(zf);
	if (c == EOF)
	  corrupted(fullname, zf);
	buf[a]= (char)c;
      }
      for (a= 0; a < ncolors; a++)
	if (!strncmp(*(ctable + a), buf, cpp))
	  break;
      if (a == ncolors) { /* major uncool */
	zclose(zf);
	printf("%s: Pixel data doesn't match color data\n", fullname);
	exit(1);
      }
      valToMem((unsigned long)a, dptr, image->pixlen);
      dptr += image->pixlen;
    }
    if ((c= zgetc(zf)) != '"')
      corrupted(fullname, zf);
  }
  return(image);
}

int xpixmapIdent(fullname, name)
     char *fullname, *name;
{ Image *image;

  if (image= xpixmapLoad(fullname, name, (unsigned int)1)) {
    freeImage(image);
    return(1);
  }
  return(0);
}

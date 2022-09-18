/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:xbitmap.c	1.1"
/* xbitmap.c:
 *
 * at one time this was XRdBitF.c.  it bears very little resemblence to it
 * now.  that was ugly code.  this is cleaner, faster, and more reliable
 * in most cases.
 *
 * jim frost 10.06.89
 *
 * Copyright, 1987, Massachusetts Institute of Technology
 *
 * Copyright 1989 Jim Frost.  See included file "copyright.h" for complete
 * copyright information.
 */

#include "mit.cpyrght"
#include "copyright.h"
#include "xloadimage.h"
#include <ctype.h>

char *rindex();

#define MAX_SIZE 255

static short        HexTable[256];  /* conversion value */
static unsigned int Initialized= 0; /* easier to fill in at run time */

#define b0000 0 /* things make more sense if you see them by bit */
#define b0001 1
#define b0010 2
#define b0011 3
#define b0100 4
#define b0101 5
#define b0110 6
#define b0111 7
#define b1000 8
#define b1001 9
#define b1010 10
#define b1011 11
#define b1100 12
#define b1101 13
#define b1110 14
#define b1111 15

#define HEXSTART -1
#define HEXDELIM -2
#define HEXBAD   -3

/* build a hex digit value table with the bits inverted
 */

static void initHexTable()
{ int a;

  for (a= 0; a < 256; a++)
    HexTable[a]= HEXBAD;

  HexTable['0']= b0000;
  HexTable['1']= b1000;
  HexTable['2']= b0100;
  HexTable['3']= b1100;
  HexTable['4']= b0010;
  HexTable['5']= b1010;
  HexTable['6']= b0110;
  HexTable['7']= b1110;
  HexTable['8']= b0001;
  HexTable['9']= b1001;
  HexTable['A']= b0101; HexTable['a']= HexTable['A'];
  HexTable['B']= b1101; HexTable['b']= HexTable['B'];
  HexTable['C']= b0011; HexTable['c']= HexTable['C'];
  HexTable['D']= b1011; HexTable['d']= HexTable['D'];
  HexTable['E']= b0111; HexTable['e']= HexTable['E'];
  HexTable['F']= b1111; HexTable['f']= HexTable['F'];
  HexTable['x']= HEXSTART;
  HexTable['\r']= HEXDELIM;
  HexTable['\n']= HEXDELIM;
  HexTable['\t']= HEXDELIM;
  HexTable[' ']= HEXDELIM;
  HexTable[',']= HEXDELIM;
  HexTable['}']= HEXDELIM;

  Initialized = 1;
}

/* read a hex value and return its value
 */

static int nextInt(zf)
     ZFILE *zf;
{ int c;
  int value= 0;
  int shift= 0;
    
  for (;;) {
    c= zgetc(zf);
    if (c == EOF)
      return(-1);
    else {
      c= HexTable[c & 0xff];
      switch(c) {
      case HEXSTART:
	shift= 0; /* reset shift counter */
	break;
      case HEXDELIM:
	if (shift)
	  return(value);
	break;
      case HEXBAD:
	return(-1);
      default:
	value += (c << shift);
	shift += 4;
      }
    }
  }
}

static void badFile(name)
     char *name;
{
  printf("%s: bad X bitmap file\n", name);
  exit(1);
}

Image *xbitmapLoad(fullname, name, verbose)
     char         *fullname, *name;
     unsigned int  verbose;
{ ZFILE        *zf;
  Image        *image;
  char          line[MAX_SIZE];
  char          name_and_type[MAX_SIZE];
  char         *type;
  int           value;
  int           v10p;
  unsigned int  linelen, dlinelen;
  unsigned int  x, y;
  unsigned int  w, h;
  byte         *dataptr;

  if (!Initialized)
    initHexTable();

  if (! (zf= zopen(fullname)))
    return(NULL);

  /* get width/height values */

  while (zgets(line, MAX_SIZE, zf)) {
    if (strlen(line) == MAX_SIZE-1) {
      zclose(zf);
      return(NULL);
    }

    /* width/height/hot_x/hot_y scanning
     */

    if (sscanf(line,"#define %s %d", name_and_type, &value) == 2) {
      if (!(type = rindex(name_and_type, '_')))
	type = name_and_type;
      else
	type++;

      if (!strcmp("width", type))
	w= (unsigned int)value;
      if (!strcmp("height", type))
	h= (unsigned int)value;
    }

    /* if start of data, determine if it's X10 or X11 data and break
     */

    if (sscanf(line, "static short %s = {", name_and_type) == 1) {
      v10p = 1;
      break;
    }
    if ((sscanf(line,"static unsigned char %s = {", name_and_type) == 1) ||
	(sscanf(line, "static char %s = {", name_and_type) == 1)) {
      v10p = 0;
      break;
    }
  }

  if (!w || !h)
    return(NULL);
  image= newBitImage(w, h);

  /* get title of bitmap if any
   */

  if ((type = rindex(name_and_type, '_')) && !strcmp("bits[]", type + 1)) {
    *type= '\0';
    image->title= dupString(name_and_type);
  }
    
  /* read bitmap data
   */

  linelen= (w / 8) + (w % 8 ? 1 : 0); /* internal line length */
  if (v10p) {
    dlinelen= (w / 8) + (w % 16 ? 2 : 0);
    dataptr= image->data;
    for (y= 0; y < h; y++) {
      for (x= 0; x < dlinelen; x++) {
	if ((value= nextInt(zf)) < 0) {
	  freeImage(image);
	  return(NULL);
	}
	*(dataptr++)= value >> 8;
	if (++x < linelen)
	  *(dataptr++)= value & 0xff;
      }
    }
  }
  else {
    dataptr= image->data;
    for (y= 0; y < h; y++)
      for (x= 0; x < linelen; x++) {
	if ((value= nextInt(zf)) < 0)
	  badFile(name);
	*(dataptr++)= value;
      }
  }

  if (verbose) {
    printf("%s is a %dx%d X", name, image->width, image->height);
    if (v10p)
      printf("10");
    else
      printf("11");
    if (image->title)
      printf(" bitmap file titled '%s'", image->title);
    printf("\n");
  }
  return(image);
}

/* this is the easiest way to do this.  it's not likely we'll have mondo
 * x bitmaps anyway given their size
 */

int xbitmapIdent(fullname, name)
     char         *fullname, *name;
{ Image *image;

  if (image= xbitmapLoad(fullname, name, (unsigned int)1)) {
    freeImage(image);
    return(1);
  }
  return(0);
}

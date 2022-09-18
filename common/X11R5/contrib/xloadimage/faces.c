/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:faces.c	1.1"
/* faces.c:
 *
 * faces format image loader
 *
 * jim frost 07.06.89
 *
 * Copyright 1989 Jim Frost.  See included file "copyright.h" for complete
 * copyright information.
 */

#include "copyright.h"
#include "xloadimage.h"

static short        HexTable[256];  /* conversion value */
static unsigned int Initialized= 0; /* easier to fill in at run time */

#define HEXIGNORE -1
#define HEXBAD    -2

/* build a hex digit value table with the bits inverted
 */

static void initHexTable()
{ int a;

  for (a= 0; a < 256; a++)
    HexTable[a]= HEXBAD;

  HexTable['0']= 0x0;
  HexTable['1']= 0x1;
  HexTable['2']= 0x2;
  HexTable['3']= 0x3;
  HexTable['4']= 0x4;
  HexTable['5']= 0x5;
  HexTable['6']= 0x6;
  HexTable['7']= 0x7;
  HexTable['8']= 0x8;
  HexTable['9']= 0x9;
  HexTable['A']= 0xa; HexTable['a']= HexTable['A'];
  HexTable['B']= 0xb; HexTable['b']= HexTable['B'];
  HexTable['C']= 0xc; HexTable['c']= HexTable['C'];
  HexTable['D']= 0xd; HexTable['d']= HexTable['D'];
  HexTable['E']= 0xe; HexTable['e']= HexTable['E'];
  HexTable['F']= 0xf; HexTable['f']= HexTable['F'];
  HexTable['\r']= HEXIGNORE;
  HexTable['\n']= HEXIGNORE;
  HexTable['\t']= HEXIGNORE;
  HexTable[' ']= HEXIGNORE;

  Initialized = 1;
}

/* read a hex value and return its value
 */

static int nextInt(zf, len)
     ZFILE        *zf;
     unsigned int  len;
{ int c;
  int value= 0;
  int count;

  len <<= 1;
  for (count= 0; count < len;) {
    c= zgetc(zf);
    if (c == EOF)
      return(-1);
    else {
      c= HexTable[c & 0xff];
      switch(c) {
      case HEXIGNORE:
	break;
      case HEXBAD:
	return(-1);
      default:
	value= (value << 4) + c;
	count++;
      }
    }
  }
  return(value);
}

Image *facesLoad(fullname, name, verbose)
     char *fullname, *name;
{ ZFILE        *zf;
  Image        *image;
  char          fname[BUFSIZ];
  char          lname[BUFSIZ];
  char          buf[BUFSIZ];
  unsigned int  w, h, d, iw, ih, id;
  unsigned int  x, y;
  int           value;
  unsigned int  linelen;
  byte         *lineptr, *dataptr;

  if (!Initialized)
    initHexTable();

  if (! (zf= zopen(fullname)))
    return(NULL);

  w= h= d= 0;
  fname[0]= lname[0]= '\0';
  while (zgets(buf, BUFSIZ - 1, zf)) {
    if (! strcmp(buf, "\n"))
      break;
    if (!strncmp(buf, "FirstName:", 10))
      strcpy(fname, buf + 11);
    else if (!strncmp(buf, "LastName:", 9))
      strcpy(lname, buf + 10);
    else if (!strncmp(buf, "Image:", 6)) {
      if (sscanf(buf + 7, "%d%d%d", &iw, &ih, &id) != 3) {
	printf("%s: Bad Faces Project image\n", fullname);
	exit(1);
      }
    }
    else if (!strncmp(buf, "PicData:", 8)) {
      if (sscanf(buf + 9, "%d%d%d", &w, &h, &d) != 3) {
	printf("%s: Bad Faces Project image\n", fullname);
	exit(1);
      }
    }
  }
  if (!w || !h || !d) {
    zclose(zf);
    return(NULL);
  }

  if (verbose)
    printf("%s is a  %dx%d %d-bit grayscale Faces Project image\n",
	   name, w, h, d);

  image= newRGBImage(w, h, d);
  fname[strlen(fname) - 1]= ' ';
  strcat(fname, lname);
  fname[strlen(fname) - 1]= '\0';
  image->title= dupString(fname);

  /* image is greyscale; build RGB map accordingly
   */

  for (x= 0; x < image->rgb.size; x++)
    *(image->rgb.red + x)= *(image->rgb.green + x)= *(image->rgb.blue + x)=
      (65536 / image->rgb.size) * x;
  image->rgb.used= image->rgb.size;

  /* read in image data
   */

  linelen= w * image->pixlen;
  lineptr= image->data + (h * linelen);
  for (y= 0; y < h; y++) {
    lineptr -= linelen;
    dataptr= lineptr;
    for (x= 0; x < w; x++) {
      if ((value= nextInt(zf, image->pixlen)) < 0) {
	printf("%s: Bad Faces Project image data\n", fullname);
	exit(1);
      }
      *(dataptr++)= value;
    }
  }
  zclose(zf);
  return(image);
}

int facesIdent(fullname, name)
     char *fullname, *name;
{ Image *image;

  if (image= facesLoad(name, fullname, 1)) {
    freeImage(image);
    return(1);
  }
  return(0);
}

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:pbm.c	1.1"
/* pbm.c:
 *
 * portable bit map (pbm) format images
 *
 * jim frost 09.27.89
 *
 * patched by W. David Higgins (wdh@mkt.csd.harris.com) to support
 * raw-format PBM files.
 */

#include "xloadimage.h"

static int          IntTable[256];
static unsigned int Initialized= 0;

#define NOTINT  -1
#define COMMENT -2
#define SPACE   -3
#define NEWLINE -4

#define BADREAD    0 /* read error */
#define NOTPBM     1 /* not a pbm file */
#define PBMNORMAL  2 /* pbm normal type file */
#define PBMCOMPACT 3 /* pbm compacty type file */
#define PBMRAWBITS 4 /* pbm raw bits type file */

static void initializeTable()
{ unsigned int a;

  for (a= 0; a < 256; a++)
    IntTable[a]= NOTINT;
  IntTable['#']= COMMENT;
  IntTable['\n']= NEWLINE;
  IntTable['\r']= IntTable['\t']= IntTable[' ']= SPACE;
  IntTable['0']= 0;
  IntTable['1']= 1;
  IntTable['2']= 2;
  IntTable['3']= 3;
  IntTable['4']= 4;
  IntTable['5']= 5;
  IntTable['6']= 6;
  IntTable['7']= 7;
  IntTable['8']= 8;
  IntTable['9']= 9;
  Initialized= 1;
}

static int pbmReadChar(zf)
     ZFILE *zf;
{ int c;

  if ((c= zgetc(zf)) == EOF) {
    zclose(zf);
    return(-1);
  }
  if (IntTable[c] == COMMENT)
    do {
      if ((c= zgetc(zf)) == EOF)
	return(-1);
    } while (IntTable[c] != NEWLINE);
  return(c);
}

static int pbmReadInt(zf)
     ZFILE *zf;
{ int c, value;

  for (;;) {
    c= pbmReadChar(zf);
    if (c < 0)
      return(-1);
    if (IntTable[c] >= 0)
      break;
  };

  value= IntTable[c];
  for (;;) {
    c= pbmReadChar(zf);
    if (c < 0)
      return(-1);
    if (IntTable[c] < 0)
      return(value);
    value= (value * 10) + IntTable[c];
  }
}

static int isPBM(zf, name, width, height, verbose)
     ZFILE        *zf;
     char         *name;
     unsigned int *width, *height;
     unsigned int  verbose;
{ unsigned char buf[4];

  if (! Initialized)
    initializeTable();

  if (zread(zf, buf, 2) != 2)
    return(NOTPBM);
  if (memToVal(buf, 2) == memToVal("P1", 2)) {
    if (((*width= pbmReadInt(zf)) < 0) || ((*height= pbmReadInt(zf)) < 0))
      return(NOTPBM);
    if (verbose)
      printf("%s is a %dx%d PBM image\n", name, *width, *height);
    return(PBMNORMAL);
  }
  if (memToVal(buf, 2) == memToVal("P4", 2)) {
    if (((*width= pbmReadInt(zf)) < 0) || ((*height= pbmReadInt(zf)) < 0))
      return(NOTPBM);
    if (verbose)
      printf("%s is a %dx%d RawBits PBM image\n", name, *width, *height);
    return(PBMRAWBITS);
  }
  if (memToVal(buf, 2) == 0x2a17) {
    if (zread(zf, buf, 4) != 4)
      return(NOTPBM);
    *width= memToVal(buf, 2);
    *height= memToVal(buf + 2, 2);
    if (verbose)
      printf("%s is a %dx%d Compact PBM image\n", name, *width, *height);
    return(PBMCOMPACT);
  }
  return(NOTPBM);
}

int pbmIdent(fullname, name)
     char *fullname, *name;
{ ZFILE        *zf;
  unsigned int  width, height, ret;

  if (! (zf= zopen(fullname, name)))
    return(0);

  ret= isPBM(zf, name, &width, &height, (unsigned int)1);
  zclose(zf);
  return(ret != NOTPBM);
}

Image *pbmLoad(fullname, name, verbose)
     char         *fullname, *name;
     unsigned int  verbose;
{ ZFILE        *zf;
  Image        *image;
  unsigned int  x, y;
  unsigned int  width, height;
  unsigned int  linelen;
  byte          srcmask, destmask;
  byte         *destptr, *destline;
  int           src;
  unsigned int  numbytes, numread;

  if (! (zf= zopen(fullname)))
    return(NULL);

  switch (isPBM(zf, name, &width, &height, verbose)) {
  case NOTPBM:
    zclose(zf);
    return(NULL);
    
  case PBMNORMAL:
    image= newBitImage(width, height);
    linelen= (width / 8) + (width % 8 ? 1 : 0);
    destline= image->data;
    for (y= 0; y < height; y++) {
      destptr= destline;
      destmask= 0x80;
      for (x= 0; x < width; x++) {
	do {
	  if ((src= pbmReadChar(zf)) < 0) {
	    printf("%s: Short image\n", fullname);
	    zclose(zf);
	    exit(1);
	  }
	  if (IntTable[src] == NOTINT) {
	    printf("%s: Bad image data\n", fullname);
	    zclose(zf);
	    exit(1);
	  }
	} while (IntTable[src] < 0);
	
	switch (IntTable[src]) {
	case 1:
	  *destptr |= destmask;
	case 0:
	  if (! (destmask >>= 1)) {
	    destmask= 0x80;
	    destptr++;
	  }
	  break;
	default:
	  printf("%s: Bad image data\n", fullname);
	  zclose(zf);
	  exit(1);
	}
      }
      destline += linelen;
    }
    break;

  case PBMRAWBITS:
    image= newBitImage(width, height);
    destline= image->data;
    linelen= (width + 7) / 8;
    numbytes= linelen * height;
    srcmask= 0;		/* force initial read */
    numread= 0;
    for (y= 0; y < height; y++) {
      destptr= destline;
      destmask= 0x80;
      if (srcmask != 0x80) {
        srcmask= 0x80;
	if ((numread < numbytes) && ((src= zgetc(zf)) == EOF)) {
	   printf("%s: Short image\n", fullname);
	   zclose(zf);
	   exit(1);
	}
	numread++;
      }
      for (x= 0; x < width; x++) {
	if (src & srcmask)
	  *destptr |= destmask;
	if (! (destmask >>= 1)) {
	  destmask= 0x80;
	  destptr++;
	}
	if (! (srcmask >>= 1)) {
	  srcmask= 0x80;
	  if ((numread < numbytes) && ((src= zgetc(zf)) == EOF)) {
	    printf("%s: Short image\n", fullname);
	    zclose(zf);
	    exit(1);
	  }
	  numread++;
	}
      }
      destline += linelen;
    }
    break;
 
  case PBMCOMPACT:
    image= newBitImage(width, height);
    destline= image->data;
    linelen= (width / 8) + (width % 8 ? 1 : 0);
    srcmask= 0x80;
    destmask= 0x80;
    if ((src= zgetc(zf)) == EOF) {
      printf("%s: Short image\n", fullname);
      zclose(zf);
      exit(1);
    }
    numread= 1;
    numbytes= width * height;
    numbytes= (numbytes / 8) + (numbytes % 8 ? 1 : 0);
    for (y= 0; y < height; y++) {
      destptr= destline;
      destmask= 0x80;
      for (x= 0; x < width; x++) {
	if (src & srcmask)
	  *destptr |= destmask;
	if (! (destmask >>= 1)) {
	  destmask= 0x80;
	  destptr++;
	}
	if (! (srcmask >>= 1)) {
	  srcmask= 0x80;
	  if ((numread < numbytes) && ((src= zgetc(zf)) == EOF)) {
	    printf("%s: Short image\n", fullname);
	    zclose(zf);
	    exit(1);
	  }
	  numread++;
	}
      }
      destline += linelen;
    }
    break;
  }
  image->title= dupString(name);
  return(image);
}

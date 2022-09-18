/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:sunraster.c	1.1"
/* sunraster.c:
 *
 * sun rasterfile image type
 *
 * jim frost 09.27.89
 *
 * Copyright 1989 Jim Frost.  See included file "copyright.h" for complete
 * copyright information.
 */

#include "copyright.h"
#include "xloadimage.h"
#include "sunraster.h"

static void babble(name, header)
     char           *name;
     struct rheader *header;
{
  printf("%s is a", name);
  switch (memToVal(header->type, 4)) {
  case ROLD:
    printf("n old-style");
    break;
  case RSTANDARD:
    printf(" standard");
    break;
  case RRLENCODED:
    printf(" run-length encoded");
    break;
  default:
    printf(" unknown-type");
  }
  printf(" %dx%d ", memToVal(header->width, 4), memToVal(header->height, 4));
  if (memToVal(header->depth, 4) > 1)
    printf("%d plane %s",
	       memToVal(header->depth, 4),
	       (memToVal(header->maplen, 4) > 0 ? "color" : "greyscale")
	       );
  else
    printf("monochrome");
  printf(" Sun rasterfile\n");
}

int sunRasterIdent(fullname, name)
     char *fullname, *name;
{ ZFILE          *zf;
  struct rheader  header;
  int             r;

  if (! (zf= zopen(fullname))) {
    perror("sunRasterIdent");
    return(0);
  }
  switch (zread(zf, &header, sizeof(struct rheader))) {
  case -1:
    perror("sunRasterIdent");
    r= 0;
    break;

  case sizeof(struct rheader):
    if (memToVal(header.magic, 4) != RMAGICNUMBER) {
      r= 0;
      break;
    }
    babble(name, &header);
    r= 1;
    break;

  default:
    r= 0;
    break;
  }
  zclose(zf);
  return(r);
}

/* read either rl-encoded or normal image data
 */

static void sunread(zf, buf, len, enc)
     ZFILE        *zf;
     byte         *buf;
     unsigned int  len;
     unsigned int  enc;  /* true if encoded file */
{ static byte repchar, remaining= 0;

  /* rl-encoded read
   */

  if (enc) {
    while (len--)
      if (remaining) {
	remaining--;
	*(buf++)= repchar;
      }
      else {
	if (zread(zf, &repchar, 1) != 1) {
	  printf("sunRasterLoad: Bad read on image data\n");
	  exit(1);
	}
	if (repchar == RESC) {
	  if (zread(zf, &remaining, 1) != 1) {
	    printf("sunRasterLoad: Bad read on image data\n");
	    exit(1);
	  }
	  if (remaining == 0)
	    *(buf++)= RESC;
	  else {
	    if (zread(zf, &repchar, 1) != 1) {
	      printf("sunRasterLoad: Bad read on image data\n");
	      exit(1);
	    }
	    *(buf++)= repchar;
	  }
	}
	else
	  *(buf++)= repchar;
      }
  }

  /* normal read
   */

  else {
    if (zread(zf, buf, len) < len) {
      printf("sunRasterLoad: Bad read on image data\n");
      exit(1);
    }
  }
}

Image *sunRasterLoad(fullname, name, verbose)
     char         *fullname, *name;
     unsigned int  verbose;
{ ZFILE          *zf;
  struct rheader  header;
  unsigned int    mapsize;
  byte           *map;
  byte           *mapred, *mapgreen, *mapblue;
  unsigned int    depth;
  unsigned int    linelen;   /* length of raster line in bytes */
  unsigned int    fill;      /* # of fill bytes per raster line */
  unsigned int    enc;
  byte            fillchar;
  Image          *image;
  byte           *lineptr;
  unsigned int    y;

  if (! (zf= zopen(fullname))) {
    perror("sunRasterLoad");
    return(NULL);
  }
  switch (zread(zf, &header, sizeof(struct rheader))) {
  case -1:
    perror("sunRasterLoad");
    zclose(zf);
    exit(1);

  case sizeof(struct rheader):
    if (memToVal(header.magic, 4) != RMAGICNUMBER) {
      zclose(zf);
      return(NULL);
    }
    if (verbose)
      babble(name, &header);
    break;

  default:
    zclose(zf);
    return(NULL);
  }

  /* get an image to put the data in
   */

  depth= memToVal(header.depth, 4);
  if (depth == 1)
    image= newBitImage(memToVal(header.width, 4),
		       memToVal(header.height, 4));
  else
    image= newRGBImage(memToVal(header.width, 4),
		       memToVal(header.height, 4),
		       memToVal(header.depth, 4));

  /* set up the colormap
   */

  if (depth == 1)
    linelen= (image->width / 8) + (image->width % 8 ? 1 : 0);
  else
    linelen= image->width * image->pixlen;
  fill= (linelen % 2 ? 1 : 0);
  /*
   *  Handle color...
   */
  if (mapsize= memToVal(header.maplen, 4)) {
    map= lmalloc(mapsize);
    if (zread(zf, map, mapsize) < mapsize) {
      printf("sunRasterLoad: Bad read on colormap\n");
      exit(1);
    }
    mapsize /= 3;
    mapred= map;
    mapgreen= mapred + mapsize;
    mapblue= mapgreen + mapsize;
    for (y= 0; y < mapsize; y++) {
      *(image->rgb.red + y)= (*(mapred++) << 8);
      *(image->rgb.green + y)= (*(mapgreen++) << 8);
      *(image->rgb.blue + y)= (*(mapblue++) << 8);
    }
    lfree(map);
    image->rgb.used= mapsize;
  }

  /*
   *  Handle 8-bit greyscale via a simple ramp function...
   */
  else if (depth > 1) {
    mapsize = 256*3;
    map= lmalloc(mapsize);
    for (y = 0; y < 256; y += 1) {
      map[y] = map[256+y] = map[2*256+y] = y;
    }
    mapsize /= 3;
    mapred= map;
    mapgreen= mapred + mapsize;
    mapblue= mapgreen + mapsize;
    for (y= 0; y < mapsize; y++) {
      *(image->rgb.red + y)= (*(mapred++) << 8);
      *(image->rgb.green + y)= (*(mapgreen++) << 8);
      *(image->rgb.blue + y)= (*(mapblue++) << 8);
    }
    lfree(map);
    image->rgb.used= mapsize;
  }
    

  enc= (memToVal(header.type, 4) == RRLENCODED);
  lineptr= image->data;
  for (y= 0; y < image->height; y++) {
    sunread(zf, lineptr, linelen, enc);
    lineptr += linelen;
    if (fill)
      sunread(zf, &fillchar, fill, enc);
  }
  zclose(zf);
  image->title= dupString(name);
  return(image);
}

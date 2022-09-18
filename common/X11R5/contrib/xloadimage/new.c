/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:new.c	1.1"
/* new.c:
 *
 * functions to allocate and deallocate structures and structure data
 *
 * jim frost 09.29.89
 *
 * Copyright 1989 Jim Frost.  See included file "copyright.h" for complete
 * copyright information.
 */

#include "copyright.h"
#include "image.h"

char *dupString(s)
     char *s;
{ char *d;

  if (!s)
    return(NULL);
  d= (char *)lmalloc(strlen(s) + 1);
  strcpy(d, s);
  return(d);
}

void newRGBMapData(rgb, size)
     RGBMap       *rgb;
     unsigned int  size;
{
  rgb->used= 0;
  rgb->size= size;
  rgb->red= (Intensity *)lmalloc(sizeof(Intensity) * size);
  rgb->green= (Intensity *)lmalloc(sizeof(Intensity) * size);
  rgb->blue= (Intensity *)lmalloc(sizeof(Intensity) * size);
}

void freeRGBMapData(rgb)
     RGBMap *rgb;
{
  lfree(rgb->red);
  lfree(rgb->green);
  lfree(rgb->blue);
}

Image *newBitImage(width, height)
     unsigned int width, height;
{ Image        *image;
  unsigned int  linelen;

  image= (Image *)lmalloc(sizeof(Image));
  image->type= IBITMAP;
  image->title= NULL;
  newRGBMapData(&(image->rgb), (unsigned int)2);
  *(image->rgb.red)= *(image->rgb.green)= *(image->rgb.blue)= 65535;
  *(image->rgb.red + 1)= *(image->rgb.green + 1)= *(image->rgb.blue + 1)= 0;
  image->rgb.used= 2;
  image->width= width;
  image->height= height;
  image->depth= 1;
  linelen= (width / 8) + (width % 8 ? 1 : 0); /* thanx johnh@amcc.com */
  image->data= (unsigned char *)lcalloc(linelen * height);
  return(image);
}

Image *newRGBImage(width, height, depth)
     unsigned int width, height, depth;
{ Image        *image;
  unsigned int  pixlen, numcolors, a;

  pixlen= (depth / 8) + (depth % 8 ? 1 : 0);
  for (numcolors= 2, a= depth - 1; a; a--)
    numcolors *= 2;
  image= (Image *)lmalloc(sizeof(Image));
  image->type= IRGB;
  image->title= NULL;
  newRGBMapData(&(image->rgb), numcolors);
  image->width= width;
  image->height= height;
  image->depth= depth;
  image->pixlen= pixlen;
  image->data= (unsigned char *)lmalloc(width * height * pixlen);
  return(image);
}

void freeImageData(image)
     Image *image;
{
  if (image->title) {
    lfree(image->title);
    image->title= NULL;
  }
  freeRGBMapData(&(image->rgb));
  lfree(image->data);
}

void freeImage(image)
     Image *image;
{
  freeImageData(image);
  lfree(image);
}

byte *lmalloc(size)
     unsigned int size;
{ byte *area;

  if (!(area= (byte *)malloc(size))) {
    perror("malloc");
    exit(1);
  }
  return(area);
}

byte *lcalloc(size)
     unsigned int size;
{ byte *area;

  if (!(area= (byte *)calloc(1, size))) {
    perror("calloc");
    exit(1);
  }
  return(area);
}

void lfree(area)
     byte *area;
{
  free(area);
}

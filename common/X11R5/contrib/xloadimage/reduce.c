/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:reduce.c	1.1"
/* reduce.c:
 *
 * reduce an image's colormap usage to a set number of colors.
 *
 * jim frost 07.06.89
 *
 * Copyright 1989, 1990 Jim Frost.  See included file "copyright.h" for
 * complete copyright information.
 */

#include "copyright.h"
#include "image.h"

#define DIST(A, B) ((A) < (B) ? (B) - (A) : (A) - (B))

/* find the distance between two colors.  we loose some accuracy here because
 * a triple squared short may not fit in a long.  we use a table lookup
 * to help speed this up; it's an O(exp(n,2)) algorithm.
 */

static unsigned int  squareInit= 0;
static unsigned long squareTable[32768];

static void initSquareTable()
{ unsigned long a;

  for (a= 0; a < 32768; a++)
    squareTable[a]= a * a;
  squareInit= 1;
}

static unsigned long colorDistance(rgb, a, b)
     RGBMap *rgb;
     Pixel   a, b;
{
  return(squareTable[(int)DIST(*(rgb->red + a), *(rgb->red + b)) >> 1] +
	 squareTable[(int)DIST(*(rgb->green + a), *(rgb->green + b)) >> 1] +
	 squareTable[(int)DIST(*(rgb->blue + a), *(rgb->blue + b)) >> 1]);
}

Pixel bestColor(rgb, color, rdist)
     RGBMap        *rgb;
     Pixel          color;
     unsigned long *rdist;
{ Pixel         qcolor, bcolor;
  unsigned long qdist, bdist;

  bdist= 0xffffffff;
  bcolor= 0;
  for (qcolor= color + 1; qcolor < rgb->used; qcolor++)
    if ((qdist= colorDistance(rgb, color, qcolor)) && (qdist < bdist)) {
      bdist= qdist;
      bcolor= qcolor;
    }
  *rdist= bdist;
  return(bcolor);
}

void reduceRGBMap(rgb, n, verbose)
     RGBMap       *rgb;
     unsigned int  n;
     unsigned int  verbose;
{ unsigned int   numcolors;
  Pixel          a, b;
  Pixel          lowextreme, highextreme; /* intensity extremes */
  unsigned long  lowintensity, highintensity, myintensity;
  Pixel          bcolor1; /* closest colors */
  Pixel          bcolor2;
  unsigned long  bdist;
  Pixel         *best;    /* array holding best match for each color */
  unsigned long *dists;   /* array holding distances of best matches */
  Pixel         *same;    /* array holding identical pixel lists */
  Intensity      newred, newgreen, newblue;

  if (!squareInit)     /* only do multiplies once */
    initSquareTable();

  if (verbose) {
    printf("  Reducing colormap to %d colors...", n);
    fflush(stdout);
  }

  best= (Pixel *)lmalloc(sizeof(Pixel) * rgb->used);
  same= (Pixel *)lmalloc(sizeof(Pixel) * rgb->used);
  dists= (unsigned long *)lmalloc(sizeof(unsigned long) * rgb->used);

  /* find out how many unique colors we have by subtracting identical ones
   * and build table of identicals.  find extreme intensities while we're
   * at it.
   */

  lowextreme= highextreme= rgb->used - 1;
  lowintensity= highintensity= *(rgb->red + lowextreme) +
     *(rgb->green + lowextreme) + *(rgb->blue + lowextreme);
  for (a= 0; a < rgb->used; a++)
    *(same + a)= a;
  for (numcolors= rgb->used, a= 0; a < rgb->used - 1; a++) {
    if (*(same + a) == a) {
      myintensity= *(rgb->red + a) + *(rgb->green + a) + *(rgb->blue + a);
      if (myintensity < lowintensity) {
	lowintensity= myintensity;
	lowextreme= a;
      }
      if (myintensity > highintensity) {
	highintensity= myintensity;
	highextreme= a;
      }
      for (b= a + 1; b < rgb->used; b++) {
	if ((*(rgb->red + a) == *(rgb->red + b)) &&
	    (*(rgb->green + a) == *(rgb->green + b)) &&
	    (*(rgb->blue + a) == *(rgb->blue + b))) {
	  numcolors--;
	  *(same + b)= a;
	}
      }
    }
  }

  for (a= 0; a < rgb->used - 1; a++) /* build table of "bests" */
    *(best + a)= bestColor(rgb, a, dists + a);

  /* find the two closest colors in the colormap and average them, thus
   * reducing the size of the colormap by one.  continue until we fit.
   * this is simplistic but effective.
   */

  while (numcolors-- > n) {
    bdist= 0xffffffff; /* a really big number */
    for (a= 0; a < rgb->used - 1; a++)
      if ((*(same + a) == a) && (*(dists + a) < bdist)) {
	bdist= *(dists + a);
	bcolor1= a;
      }

    bcolor2= *(same + *(best + bcolor1));

    /* calculate new rgb values.  we average the colors unless one of them
     * is an extreme.
     */

    if ((bcolor1 == lowextreme) || (bcolor1 == highextreme)) {
      newred= *(rgb->red + bcolor1);
      newgreen= *(rgb->green + bcolor1);
      newblue= *(rgb->blue + bcolor1);
    }
    else if ((bcolor2 == lowextreme) || (bcolor2 == highextreme)) {
      newred= *(rgb->red + bcolor2);
      newgreen= *(rgb->green + bcolor2);
      newblue= *(rgb->blue + bcolor2);
    }
    else {
      newred= ((unsigned int)(*(rgb->red + bcolor1)) +
	       (unsigned int)(*(rgb->red + bcolor2))) >> 1;
      newgreen= ((unsigned int)(*(rgb->green + bcolor1)) +
		 (unsigned int)(*(rgb->green + bcolor2))) >> 1;
      newblue= ((unsigned int)(*(rgb->blue + bcolor1)) +
		(unsigned int)(*(rgb->blue + bcolor2))) >> 1;
    }

    for (a= 0; a < rgb->used; a++)
      if ((*(same + a) == bcolor1) || (*(same + a) == bcolor2)) {
	*(same + a)= bcolor1;
        *(rgb->red + a)= newred;
        *(rgb->green + a)= newgreen;
        *(rgb->blue + a)= newblue;
      }
	
    for (a= 0; a < rgb->used - 1; a++)
      if ((*(best + a) == bcolor1) || (*(same + a) == bcolor1) ||
	  (*(same + *(best + a)) == bcolor1))
	*(best + a)= bestColor(rgb, a, dists + a);
  }

  lfree((byte *)best);
  lfree((byte *)dists);
  lfree((byte *)same);

  if (verbose)
    printf("done\n");
}

void reduce(image, n, verbose)
     Image        *image;
     unsigned int  n, verbose;
{ char buf[BUFSIZ];

  goodImage(image, "reduce");
  if (! RGBP(image)) /* we're AT&T */
    return;
  compress(image, verbose);
  reduceRGBMap(&(image->rgb), n, verbose);
  compress(image, verbose);
  sprintf(buf, "%s (%d colors)", image->title, image->rgb.used);
  if (image->title)
    lfree((byte *)image->title);
  image->title= dupString(buf);
}

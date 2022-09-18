/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:imagetypes.c	1.1"
/* imagetypes.c:
 *
 * this contains things which reference the global ImageTypes array
 *
 * jim frost 09.27.89
 *
 * Copyright 1989 Jim Frost.  See included file "copyright.h" for complete
 * copyright information.
 */

#include "copyright.h"
#include "image.h"
#include "imagetypes.h"
#include <errno.h>

extern int errno;

/* load a named image
 */

Image *loadImage(name, verbose)
     char         *name;
     unsigned int  verbose;
{ char   fullname[BUFSIZ];
  Image *image;
  int    a;

  if (findImage(name, fullname) < 0) {
    if (errno == ENOENT)
      printf("%s: image not found\n", name);
    else
      perror(fullname);
    return(NULL);
  }
  for (a= 0; ImageTypes[a].loader; a++)
    if (image= ImageTypes[a].loader(fullname, name, verbose))
      return(image);
  printf("%s: unknown or unsupported image type\n", fullname);
  return(NULL);
}

/* identify what kind of image a named image is
 */

void identifyImage(name)
     char *name;
{ char fullname[BUFSIZ];
  int  a;

  if (findImage(name, fullname) < 0) {
    if (errno == ENOENT)
      printf("%s: image not found\n", name);
    else
      perror(fullname);
    return;
  }
  for (a= 0; ImageTypes[a].identifier; a++)
    if (ImageTypes[a].identifier(fullname, name))
      return;
  printf("%s: unknown or unsupported image type\n", fullname);
}

/* tell user what image types we support
 */

void supportedImageTypes()
{ int a;

  printf("Image types supported:\n");
  for (a= 0; ImageTypes[a].name; a++)
    printf("  %s\n", ImageTypes[a].name);
}

void goodImage(image, func)
     Image *image;
     char  *func;
{
  if (!image) {
    printf("%s: nil image\n", func);
    exit(0);
  }
  switch (image->type) {
  case IBITMAP:
  case IRGB:
    break;
  default:
    printf("%s: bad destination image\n", func);
    exit(0);
  }
}

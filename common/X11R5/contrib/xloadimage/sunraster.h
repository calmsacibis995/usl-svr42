/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:sunraster.h	1.1"
/* sunraster.h
 *
 * this describes the header for Sun rasterfiles.  if you have SunOS, a
 * better description is in /usr/include/rasterfile.h.  this is used
 * instead to improve portability and to avoid distribution problems.
 *
 * Copyright 1989 Jim Frost.  See included file "copyright.h" for complete
 * copyright information.
 */

#include "copyright.h"

struct rheader {
  unsigned char magic[4];   /* magic number */
  unsigned char width[4];   /* width of image in pixels */
  unsigned char height[4];  /* height of image in pixels */
  unsigned char depth[4];   /* depth of each pixel */
  unsigned char length[4];  /* length of the image in bytes */
  unsigned char type[4];    /* format of file */
  unsigned char maptype[4]; /* type of colormap */
  unsigned char maplen[4];  /* length of colormap in bytes */
};

/* following the header is the colormap (unless maplen is zero) then
 * the image.  each row of the image is rounded to 2 bytes.
 */

#define RMAGICNUMBER 0x59a66a95 /* magic number of this file type */

/* these are the possible file formats
 */

#define ROLD       0 /* old format, see /usr/include/rasterfile.h */
#define RSTANDARD  1 /* standard format */
#define RRLENCODED 2 /* run length encoding to compress the image */

/* these are the possible colormap types.  if it's in RGB format,
 * the map is made up of three byte arrays (red, green, then blue)
 * that are each 1/3 of the colormap length.
 */

#define RNOMAP  0 /* no colormap follows the header */
#define RRGBMAP 1 /* rgb colormap */
#define RRAWMAP 2 /* raw colormap; good luck */

#define RESC 128 /* run-length encoding escape character */


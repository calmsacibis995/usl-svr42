/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xwd:xwd.c	1.2"
#endif
/*
 xwd.c (C source file)
	Acc: 574010310 Thu Mar 10 10:18:30 1988
	Mod: 572850945 Fri Feb 26 00:15:45 1988
	Sta: 573774662 Mon Mar  7 16:51:02 1988
	Owner: 2011
	Group: 1985
	Permissions: 444
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
#include <X11/copyright.h>

/* Copyright 1987 Massachusetts Institute of Technology */

/*
 * xwd.c MIT Project Athena, X Window system window raster image dumper.
 *
 * This program will dump a raster image of the contents of a window into a 
 * file for output on graphics printers or for other uses.
 *
 *  Author:	Tony Della Fera, DEC
 *		17-Jun-85
 * 
 *  Modification history:
 *
 *  11/14/86 Bill Wyatt, Smithsonian Astrophysical Observatory
 *    - Removed Z format option, changing it to an XY option. Monochrome 
 *      windows will always dump in XY format. Color windows will dump
 *      in Z format by default, but can be dumped in XY format with the
 *      -xy option.
 *
 *  11/18/86 Bill Wyatt
 *    - VERSION 6 is same as version 5 for monchrome. For colors, the 
 *      appropriate number of Color structs are dumped after the header,
 *      which has the number of colors (=0 for monochrome) in place of the
 *      V5 padding at the end. Up to 16-bit displays are supported. I
 *      don't yet know how 24- to 32-bit displays will be handled under
 *      the Version 11 protocol.
 *
 *  6/15/87 David Krikorian, MIT Project Athena
 *    - VERSION 7 runs under the X Version 11 servers, while the previous
 *      versions of xwd were are for X Version 10.  This version is based
 *      on xwd version 6, and should eventually have the same color
 *      abilities. (Xwd V7 has yet to be tested on a color machine, so
 *      all color-related code is commented out until color support
 *      becomes practical.)
 */

#ifndef lint
static char *rcsid_xwd_c = "$Header: xwd.c,v 1.34 88/02/12 13:24:59 jim Exp $";
#endif

/*%
 *%    This is the format for commenting out color-related code until
 *%  color can be supported.
%*/

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>

#define FEEP_VOLUME 0

/* Include routines to do parsing */
#include "dsimple.h"

/* Setable Options */

int format = ZPixmap;
Bool nobdrs = False;
Bool standard_out = True;
Bool debug = False;

extern int (*_XErrorFunction)();
extern int _XDefaultError();

main(argc, argv)
    int argc;
    char **argv;
{
    register i;
    Window target_win;
    FILE *out_file = stdout;

    INIT_NAME;

    Setup_Display_And_Screen(&argc, argv);

    /* Get window select on command line, if any */
    target_win = Select_Window_Args(&argc, argv);

    for (i = 1; i < argc; i++) {
	if (!strcmp(argv[i], "-nobdrs")) {
	    nobdrs = True;
	    continue;
	}
	if (!strcmp(argv[i], "-debug")) {
	    debug = True;
	    continue;
	}
	if (!strcmp(argv[i], "-help"))
	  usage();
	if (!strcmp(argv[i], "-out")) {
	    if (++i >= argc) usage();
	    if (!(out_file = fopen(argv[i], "w")))
	      Error("Can't open output file as specified.");
	    standard_out = False;
	    continue;
	}
	if (!strcmp(argv[i], "-xy")) {
	    format = XYPixmap;
	    continue;
	}
	usage();
    }
    
    /*
     * Let the user select the target window.
     */
    if (!target_win)
      target_win = Select_Window(dpy);

    /*
     * Dump it!
     */
    Window_Dump(target_win, out_file);

    fclose(out_file);
}


/*
 * Window_Dump: dump a window to a file which must already be open for
 *              writting.
 */

#ifndef MEMUTIL
char *calloc();
#endif /* MEMUTIL */

#include "X11/XWDFile.h"

Window_Dump(window, out)
     Window window;
     FILE *out;
{
    unsigned long swaptest = 1;
    XColor *colors;
    unsigned buffer_size;
    int win_name_size;
    int header_size;
    int ncolors, i;
    char *win_name;
    XWindowAttributes win_info;
    XImage *image;

    XWDFileHeader header;

    
    /*
     * Inform the user not to alter the screen.
     */
    Beep();

    /*
     * Get the parameters of the window being dumped.
     */
    if (debug) outl("xwd: Getting target window information.\n");
    if(!XGetWindowAttributes(dpy, window, &win_info)) 
      Fatal_Error("Can't get target window attributes.");

    XFetchName(dpy, window, &win_name);
    if (!win_name || !win_name[0])
      win_name = "xwdump";

    /* sizeof(char) is included for the null string terminator. */
    win_name_size = strlen(win_name) + sizeof(char);

    /*
     * Snarf the pixmap with XGetImage.
     */

    if (nobdrs) {
      	if (debug) outl("xwd: Image without borders selected.\n");
	image = XGetImage ( dpy, window, 0, 0, win_info.width,
			   win_info.height, ~0, format); 
      }
    else {
	if (debug) outl("xwd: Image with borders selected.\n");
	image = XGetImage ( dpy, window,
			   -win_info.border_width, -win_info.border_width, 
			   win_info.width + (win_info.border_width << 1),
			   win_info.height + (win_info.border_width << 1),
			   ~0, format); 
      }
    if (debug) outl("xwd: Getting pixmap.\n");

    /*
     * Determine the pixmap size.
     */
    buffer_size = Image_Size(image);

    if (debug) outl("xwd: Getting Colors.\n");

    ncolors = Get_XColors(&win_info, &colors);

    /*
     * Inform the user that the image has been retrieved.
     */
    XBell(dpy, FEEP_VOLUME);
    XBell(dpy, FEEP_VOLUME);
    XFlush(dpy);

    /*
     * Calculate header size.
     */
    if (debug) outl("xwd: Calculating header size.\n");
    header_size = sizeof(header) + win_name_size;

    /*
     * Write out header information.
     */
    if (debug) outl("xwd: Constructing and dumping file header.\n");
    header.header_size = (xwdval) header_size;
    header.file_version = (xwdval) XWD_FILE_VERSION;
    header.pixmap_format = (xwdval) format;
    header.pixmap_depth = (xwdval) image->depth;
    header.pixmap_width = (xwdval) image->width;
    header.pixmap_height = (xwdval) image->height;
    header.xoffset = (xwdval) image->xoffset;
    header.byte_order = (xwdval) image->byte_order;
    header.bitmap_unit = (xwdval) image->bitmap_unit;
    header.bitmap_bit_order = (xwdval) image->bitmap_bit_order;
    header.bitmap_pad = (xwdval) image->bitmap_pad;
    header.bits_per_pixel = (xwdval) image->bits_per_pixel;
    header.bytes_per_line = (xwdval) image->bytes_per_line;
    header.visual_class = (xwdval) win_info.visual->class;
    header.red_mask = (xwdval) win_info.visual->red_mask;
    header.green_mask = (xwdval) win_info.visual->green_mask;
    header.blue_mask = (xwdval) win_info.visual->blue_mask;
    header.bits_per_rgb = (xwdval) win_info.visual->bits_per_rgb;
    header.colormap_entries = (xwdval) win_info.visual->map_entries;
    header.ncolors = ncolors;
    header.window_width = (xwdval) win_info.width;
    header.window_height = (xwdval) win_info.height;
    if (nobdrs) {
      header.window_x = (xwdval) (win_info.x + win_info.border_width);
      header.window_y = (xwdval) (win_info.y + win_info.border_width);
    } else {
      header.window_x = (xwdval) win_info.x;
      header.window_y = (xwdval) win_info.y;
    }
    header.window_bdrwidth = (xwdval) win_info.border_width;

    if (*(char *) &swaptest) {
	_swaplong((char *) &header, sizeof(header));
	for (i = 0; i < ncolors; i++) {
	    _swaplong((char *) &colors[i].pixel, sizeof(long));
	    _swapshort((char *) &colors[i].red, 3 * sizeof(short));
	}
    }

    (void) fwrite((char *)&header, sizeof(header), 1, out);
    (void) fwrite(win_name, win_name_size, 1, out);

    /*
     * Write out the color maps, if any
     */

    if (debug) outl("xwd: Dumping %d colors.\n", ncolors);
    (void) fwrite((char *) colors, sizeof(XColor), ncolors, out);

    /*
     * Write out the buffer.
     */
    if (debug) outl("xwd: Dumping pixmap.  bufsize=%d\n",buffer_size);

    /*
     *    This copying of the bit stream (data) to a file is to be replaced
     *  by an Xlib call which hasn't been written yet.  It is not clear
     *  what other functions of xwd will be taken over by this (as yet)
     *  non-existant X function.
     */
    (void) fwrite(image->data, (int) buffer_size, 1, out);

    /*
     * free the color buffer.
     */

    if(debug && ncolors > 0) outl("xwd: Freeing colors.\n");
    if(ncolors > 0) free(colors);

    /*
     * Free window name string.
     */
    if (debug) outl("xwd: Freeing window name string.\n");
    free(win_name);

    /*
     * Free image
     */
    XDestroyImage(image);
}

/*
 * Report the syntax for calling xwd.
 */
usage()
{
    fprintf (stderr,
"usage: %s [-display host:dpy] [-debug] [-help] %s [-nobdrs] [-out <file>]",
	   program_name, SELECT_USAGE);
    fprintf (stderr, " [-xy]\n");
    exit(1);
}


/*
 * Error - Fatal xwd error.
 */
extern int errno;

Error(string)
	char *string;	/* Error description string. */
{
	outl("\nxwd: Error => %s\n", string);
	if (errno != 0) {
		perror("xwd");
		outl("\n");
	}

	exit(1);
}


/*
 * Determine the pixmap size.
 */

int Image_Size(image)
     XImage *image;
{
    if (format != ZPixmap)
      return(image->bytes_per_line * image->height * image->depth);

    return(image->bytes_per_line * image->height);
}


/*
 * Get the XColors of all pixels in image - returns # of colors
 */
int Get_XColors(win_info, colors)
     XWindowAttributes *win_info;
     XColor **colors;
{
    int i, ncolors;

    if (!win_info->colormap)
	return(0);

    if (win_info->visual->class == TrueColor ||
	win_info->visual->class == DirectColor)
	return(0);    /* XXX punt for now */

    ncolors = win_info->visual->map_entries;
    if (!(*colors = (XColor *) malloc (sizeof(XColor) * ncolors)))
      Fatal_Error("Out of memory!");

    for (i=0; i<ncolors; i++)
      (*colors)[i].pixel = i;

    XQueryColors(dpy, win_info->colormap, *colors, ncolors);
    
    return(ncolors);
}

_swapshort (bp, n)
    register char *bp;
    register unsigned n;
{
    register char c;
    register char *ep = bp + n;

    while (bp < ep) {
	c = *bp;
	*bp = *(bp + 1);
	bp++;
	*bp++ = c;
    }
}

_swaplong (bp, n)
    register char *bp;
    register unsigned n;
{
    register char c;
    register char *ep = bp + n;
    register char *sp;

    while (bp < ep) {
	sp = bp + 3;
	c = *sp;
	*sp = *bp;
	*bp++ = c;
	sp = bp + 1;
	c = *sp;
	*sp = *bp;
	*bp++ = c;
	bp += 2;
    }
}

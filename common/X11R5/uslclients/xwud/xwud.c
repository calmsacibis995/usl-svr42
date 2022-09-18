/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xwud:xwud.c	1.2"
#endif
/*
 xwud.c (C source file)
	Acc: 574010316 Thu Mar 10 10:18:36 1988
	Mod: 572851032 Fri Feb 26 00:17:12 1988
	Sta: 573774703 Mon Mar  7 16:51:43 1988
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
/* 
 * $Locker:  $ 
 */ 
static char	*rcsid = "$Header: xwud.c,v 1.18 88/02/09 12:07:17 jim Exp $";
#include <X11/copyright.h>

/* Copyright 1985, 1986, Massachusetts Institute of Technology */

/*
 * xwud.c - MIT Project Athena, X Window system window raster image
 *	    undumper.
 *
 * This program will read a raster image of a window from stdin or a file
 * and display it on an X display.
 *
 *  Author:	Tony Della Fera, DEC
 *
 *  Modified 11/14/86 by William F. Wyatt,
 *                        Smithsonian Astrophysical Observatory
 *    allows writing of monochrome XYFormat window dump files on a color
 *    display, using default WhitePixel for 1's and BlackPixel for 0's.
 *
 *  Modified 11/20/86 WFW
 *    VERSION 6 - same as V5 for monochrome, but expects color map info
 *    in the file for color images. Checks to see if the requested
 *    colors are already in the display's map (e.g. if the window dump
 *    and undump are contemporaneous to the same display). If so,
 *    undump immediately. If not, request new colors, alter the 
 *    pixels to the new values, then write the pixmap. Note that
 *    multi-plane XY format undumps don't work if the pixel values
 *    corresponding to the requested colors have to be changed.
 */

#ifndef lint
static char *rcsid_xwud_c = "$Header: xwud.c,v 1.18 88/02/09 12:07:17 jim Exp $";
#endif

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#ifndef MEMUTIL
extern char *calloc();
#endif /* MEMUTIL */
#include "dsimple.h"

#include <X11/XWDFile.h>

extern int errno;

usage()
{
    outl("usage: %s [display host:dpy] [-help] [-debug] [-inverse] [-in <file>]\n",
	 program_name);
    exit(1);
}

main(argc, argv)
    int argc;
    char **argv;
{
    register int i;
    XImage image;
    XSetWindowAttributes attributes;
    XVisualInfo vinfo, *vinfos;
    Visual *visual = NULL;
    register char *buffer;

    unsigned long swaptest = 1;
    int j, status;
    unsigned buffer_size;
    int win_name_size;
    int ncolors;
    char *str_index;
    char *file_name;
    char *win_name;
    Bool standard_in = True;
    Bool debug = False, inverse = False;

    XColor *colors;
    Window image_win;
    int win_depth;
    Colormap colormap;
    XEvent event;
    register XExposeEvent *expose = (XExposeEvent *)&event;

    GC gc;
    XGCValues gc_val;

    XWDFileHeader header;

    FILE *in_file = stdin;

    INIT_NAME;

    Setup_Display_And_Screen(&argc, argv);

    for (i = 1; i < argc; i++) {
	str_index = (char *) index (argv [i], '-');
	if (str_index == NULL) usage();
	if (strncmp(argv[i], "-help", 5) == 0) {
	    usage();
	}
	if (strncmp(argv[i], "-in", 4) == 0) {
	    if (++i >= argc) usage();
	    file_name = argv[i];
	    standard_in = False;
	    continue;
	}
	if(strcmp(argv[i], "-inverse") == 0) {
	    inverse = True;
	    continue;
	}
	if(strcmp(argv[i], "-debug") == 0) {
	    debug = True;
	    continue;
	}
	usage();
    }
    
    if (!standard_in) {
	/*
	 * Open the output file.
	 */
	in_file = fopen(file_name, "r");
	if (in_file == NULL) {
	    Error("Can't open output file as specified.");
	}
    }
    
    /*
     * Read in header information.
     */
    if(fread((char *)&header, sizeof(header), 1, in_file) != 1)
      Error("Unable to read dump file header.");

    if (*(char *) &swaptest)
	_swaplong((char *) &header, sizeof(header));

    /*
     * check to see if the dump file is in the proper format.
     */
    if (header.file_version != XWD_FILE_VERSION) {
	fprintf(stderr,"xwud: XWD file format version missmatch.");
	Error("exiting.");
    }
    if (header.header_size < sizeof(header)) {
	fprintf(stderr,"xwud: XWD header size is too small.");
	Error("exiting.");
    }

    /*
     * Calloc window name.
     */
    win_name_size = (header.header_size - sizeof(header));
    if((win_name = calloc((unsigned) win_name_size, sizeof(char))) == NULL)
      Error("Can't calloc window name storage.");

    /*
     * Read in window name.
     */
    if(fread(win_name, sizeof(char), win_name_size, in_file) != win_name_size)
      Error("Unable to read window name from dump file.");

    image.width = (int) header.pixmap_width;
    image.height = (int) header.pixmap_height;
    image.xoffset = (int) header.xoffset;
    image.format = (int) header.pixmap_format;
    image.byte_order = (int) header.byte_order;
    image.bitmap_unit = (int) header.bitmap_unit;
    image.bitmap_bit_order = (int) header.bitmap_bit_order;
    image.bitmap_pad = (int) header.bitmap_pad;
    image.depth = (int) header.pixmap_depth;
    image.bits_per_pixel = (int) header.bits_per_pixel;
    image.bytes_per_line = (int) header.bytes_per_line;
    image.red_mask = header.red_mask;
    image.green_mask = header.green_mask;
    image.blue_mask = header.blue_mask;
    image.obdata = NULL;
    _XInitImageFuncPtrs(&image);


    /* Calloc the color map buffer.
     * Read it in, copy it and use the copy to query for the
     * existing colors at those pixel values.
     */
    if(ncolors = header.ncolors) {
	colors = (XColor *)calloc(ncolors,sizeof(XColor));
	if(fread((char *) colors, sizeof(XColor), ncolors, in_file) != ncolors)
	  Error("Unable to read color map from dump file.");
	if(debug)
	  fprintf(stderr,"Read %d colors\n", ncolors);
	if (*(char *) &swaptest) {
	    for (i = 0; i < ncolors; i++) {
		_swaplong((char *) &colors[i].pixel, sizeof(long));
		_swapshort((char *) &colors[i].red, 3 * sizeof(short));
	    }
	}
    }

    /*
     * Calloc the pixel buffer.
     */
    buffer_size = Image_Size(&image);
    if((buffer = calloc(buffer_size, 1)) == NULL)
      Error("Can't calloc data buffer.");
    image.data = buffer;

    /*
     * Read in the pixmap buffer.
     */
    if((status = fread(buffer, sizeof(char), (int)buffer_size, in_file))
       != buffer_size){
	/*  Add elaboration on error here. %%*/
        Error("Unable to read pixmap from dump file.");
    }
    /*
     * Close the input file.
     */
    (void) fclose(in_file);

    vinfo.screen = screen;
    vinfo.depth = (int) header.pixmap_depth;
    vinfo.class = (int) header.visual_class;
    vinfo.red_mask = header.red_mask;
    vinfo.green_mask = header.green_mask;
    vinfo.blue_mask = header.blue_mask;
    vinfo.colormap_size = (int) header.colormap_entries;
    vinfo.bits_per_rgb = (int) header.bits_per_rgb;

    vinfos = (XVisualInfo *)
	     XGetVisualInfo(dpy,
			    /* XXX ignoring rgb mask differences */
			    VisualScreenMask|VisualDepthMask|VisualClassMask|
			    VisualColormapSizeMask|VisualBitsPerRGBMask,
			    &vinfo,
			    &j);
    if (j > 0) {
        visual = vinfos[0].visual;
	win_depth = vinfo.depth;
    } else if (header.pixmap_depth == 1) {
        visual = DefaultVisual(dpy, screen);
	win_depth = DefaultDepth(dpy, screen);
	image.format = XYBitmap;
    } else {
	fprintf(stderr, "xwud: could not find matching visual.\n");
	Error("exiting.");
    }
    
    /* XXX */
    if (visual == DefaultVisual(dpy, screen))
        colormap = DefaultColormap(dpy, screen);
	/* XXX */
#ifdef notdef
	colormap = ModifyColors(image, visual, colormap, colors, ncolors);
#endif
    else {
	colormap = XCreateColormap(dpy, RootWindow(dpy, screen), visual,
				   visual->class & 1);
	if (visual->class & 1)
	    XStoreColors(dpy, colormap, colors, ncolors);
	/* XXX colors may not be accurate for static maps */
    }


    if (colormap != DefaultColormap(dpy, screen))
	XInstallColormap(dpy, colormap); /* XXX */

    /*
     * Create the image window.
     */

    attributes.override_redirect = True;
    attributes.background_pixel = BlackPixel(dpy, screen); /* XXX */
    attributes.colormap = colormap;

    image_win = XCreateWindow(dpy,
	RootWindow(dpy, screen),
	header.window_x, header.window_y,
	header.pixmap_width, header.pixmap_height,
	0, win_depth, InputOutput, visual,
	CWOverrideRedirect|CWBackPixel|CWColormap, &attributes);

    if (!image_win) Error("Can't create image window.");

    /*
     * Select mouse ButtonPressed on the window, this is how we determine
     * when to stop displaying the window.
     */
    XSelectInput(dpy,image_win, (ButtonPressMask | ExposureMask));
     
    /*
     * Store the window name string.
     */
    XStoreName(dpy, image_win, win_name);
    
    /*
     * Map the image window.
     */
    XMapWindow(dpy, image_win);

    /* XXX */
    if (inverse) {
	gc_val.foreground = (unsigned long) WhitePixel (dpy, screen); 
	gc_val.background = (unsigned long) BlackPixel (dpy, screen); 
    } else {
	gc_val.foreground = (unsigned long) BlackPixel (dpy, screen);
	gc_val.background = (unsigned long) WhitePixel (dpy, screen); 
    }
    gc = XCreateGC (dpy, image_win, GCForeground|GCBackground, &gc_val);

    /*
     * Set up a while loop to maintain the image.
     */

    while (True) {
	/*
	 * Wait on mouse input event to terminate.
	 */
	XNextEvent(dpy, &event);
	if (event.type == ButtonPress) break;

	switch((int)event.type) {
	  case Expose:
	      if (expose->x < image.width &&
		  expose->y < image.height) {
		  if ((image.width - expose->x) < expose->width)
		      expose->width = image.width - expose->x;
		  if ((image.height - expose->y) < expose->height)
		      expose->height = image.height - expose->y;
		  XPutImage(dpy, image_win, gc, &image,
			    expose->x, expose->y, expose->x, expose->y,
			    expose->width, expose->height);
	      }
	}
    }

    /*
     * Destroy the image window.
     */
    XDestroyWindow(dpy, image_win);
    
    /*
     * Free the pixmap buffer.
     */
    free(buffer);

    /*
     * Free window name string.
     */
    free(win_name);
    exit(0);
}

#ifdef notdef
Colormap
ModifyColors(image, visual, colormap, colors, ncolors)
	XImage *image;
	Visual *visual;
	Colormap colormap;
	XColor *colors;
	int ncolors;
{
    unsigned long *cplanes, *cpixels;
    XColor *copycolors;
    register int *histbuffer;
    register u_short *wbuffer;
	
    /*
     * If necessary, get and store the new colors, convert the pixels to the
     * new colors appropriately.
     */
    if(ncolors) {
	copycolors = (XColor *)calloc(ncolors,sizeof(XColor));
	bcopy(colors, copycolors, sizeof(XColor)*ncolors);
	if(XQueryColors(dpy, colormap, copycolors, ncolors) == 0)
	  Error("Can't query the color map?");
	for(i=0; i<ncolors; i++)
	  if(!ColorEqual(&colors[i], &copycolors[i])) {
	      copycolors = True;
	      break;
	  }
	if(debug) {
	    if(copycolors)  fprintf(stderr,"New colors needed\n");
	    else fprintf(stderr,"Old colors match!\n");
	}
	cpixels = (unsigned long *)calloc(ncolors+1,sizeof(int));
	if(XAllocColorCells(dpy, colormap, 0, cplanes, 0, cpixels, 
	     (unsigned int) ncolors) == 0)

/*  Old arguments (for XGetColorCells() in X10) were: 
               0, ncolors, 0, &cplanes, cpixels %%*/

	  Error("Can't allocate colors.");
	for(i=0; i<ncolors; i++) {
	    copycolors[i].pixel = cpixels[i];
	    copycolors[i].red   = colors[i].red;
	    copycolors[i].green = colors[i].green;
	    copycolors[i].blue  = colors[i].blue;
	    if(debug) 
	      fprintf(stderr,"Pixel %4d, r = %5d  g = %5d  b = %5d\n",
		      copycolors[i].pixel, copycolors[i].red,
		      copycolors[i].green, copycolors[i].blue);
	}
	XStoreColors(ncolors, copycolors);

	/* now, make a lookup table to convert old pixels into the new ones*/
	if(header.pixmap_format == ZPixmap) {
	    if(header.display_planes < 9) {
		histbuffer = (int *)calloc(256, sizeof(int));
		bzero(histbuffer, 256*sizeof(int));
		for(i=0; i<ncolors; i++)
		  histbuffer[colors[i].pixel] = copycolors[i].pixel;
		for(i=0; i<buffer_size; i++)
		  buffer[i] = histbuffer[buffer[i]];
	    }
	    else if(header.display_planes < 17) {
		histbuffer = (int *)calloc(65536, sizeof(int));
		bzero(histbuffer, 65536*sizeof(int));
		for(i=0; i<ncolors; i++)
		  histbuffer[colors[i].pixel] = copycolors[i].pixel;
		wbuffer = (u_short *)buffer;
		for(i=0; i<(buffer_size/sizeof(u_short)); i++)
		  wbuffer[i] = histbuffer[wbuffer[i]];
	    } 
	    else if(header.display_planes > 16) {
		Error("Unable to handle more than 16 planes at this time");
	    }
	    free(histbuffer);
	}
	free(cpixels);
	bcopy(copycolors, colors, sizeof(XColor)*ncolors);
	free(copycolors);
    }
    return colormap;
}

/*
 * test two color map entries for equality
 */
ColorEqual(color1, color2)
     register XColor *color1, *color2;
{
    return(color1->pixel == color2->pixel &&
	   color1->red   == color2->red &&
	   color1->green == color2->green &&
	   color1->blue  == color2->blue);
}

#endif

int Image_Size(image)
     XImage *image;
{
    if (image->format != ZPixmap)
      return(image->bytes_per_line * image->height * image->depth);

    return(image->bytes_per_line * image->height);

}

/*
 * Error - Fatal xwud error.
 */
Error(string)
	char *string;	/* Error description string. */
{
	fprintf(stderr, "xwud: Error => %s\n", string);

	if (errno != 0) {
		perror("xwud");
		fprintf(stderr, "\n");
	}

	exit(1);
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

/* End of xwud.c */

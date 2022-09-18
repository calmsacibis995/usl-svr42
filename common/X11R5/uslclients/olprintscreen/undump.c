/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olps:undump.c	1.14"
#endif


#include <X11/copyright.h>
/* Copyright 1987 Massachusetts Institute of Technology */

#include "main.h"
#include "externs.h"
#include "error.h"

/*
 * undump.c - based on xwud.c code
 * routines to read image from saved file, 
 * create scrolled window and stub,
 * create image for stub,
 * reset size of scrolled window (and hence olprintscreen)
         for each new image read in
 */

/*
#ifndef MEMUTIL
extern char *calloc();
#endif
*/
#include <stdlib.h>

extern int xlocal;	/* for local shm	*/

static Pixmap oldpix;

Boolean	PSTriggerNotify OL_ARGS((Widget, Window, Position,
					 Position, Atom, Time,
					 OlDnDDropSiteID,
					 OlDnDTriggerOperation, Boolean,
					 Boolean, XtPointer));
static void	PSSelectionCB OL_ARGS((Widget, XtPointer, Atom *, Atom *,
				       XtPointer, unsigned long *, int *));

int
FileToImage(name,image)
char * name;
XImage	*image;  /* pass in image_struct */
{
    register int i;
    int status;
    Bool standard_in = True;
    Bool debug1 = False;


    /* For testing out file to see if XWD file */
    XWDFileHeader     header_test;
    char            * win_name_test;
    int		      win_n_size_test;
    XColor          * colors_test;
    int		      ncolors_test;

    /* open */
    if (CanOpenFile(name,"r",&rw_file) == -1)
            return (-1);

    /* First test if it is in dump file format. */
    if(fread((char *)&header_test, sizeof(header_test), 1, rw_file) != 1) {
      strcpy(message,name);
      strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                    OleNfooterMsg, OleTnotDumpFile,
                                    OleCOlClientOlpsMsgs,
                                    OleMfooterMsg_notDumpFile,
                                    (XrmDatabase)NULL));
      SetFooterText(footer_text, message);
      (void) fclose(rw_file);
      return(-1);
    }

    if (*(char *) &swaptest)
        _swaplong((char *) &header_test, sizeof(header_test));

    if (header_test.file_version != XWD_FILE_VERSION) {
      strcpy(message,name);
      strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                    OleNfooterMsg, OleTnotDumpFile,
                                    OleCOlClientOlpsMsgs,
                                    OleMfooterMsg_notDumpFile,
                                    (XrmDatabase)NULL));
      SetFooterText(footer_text, message);
      (void) fclose(rw_file);
      return(-1);
    }
    if (header_test.header_size < sizeof(header_test)) {
      strcpy(message,name);
      strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                    OleNfooterMsg, OleTnotDumpFile,
                                    OleCOlClientOlpsMsgs,
                                    OleMfooterMsg_notDumpFile,
                                    (XrmDatabase)NULL));
      SetFooterText(footer_text, message);
      (void) fclose(rw_file);
      return(-1);
    }

    win_n_size_test = (header_test.header_size - sizeof(header_test));
    win_name_test = calloc((unsigned) win_n_size_test, sizeof(char));
    if(fread(win_name_test, sizeof(char),
        win_n_size_test, rw_file) != win_n_size_test){
      strcpy(message,name);
      strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                    OleNfooterMsg, OleTnotDumpFile,
                                    OleCOlClientOlpsMsgs,
                                    OleMfooterMsg_notDumpFile,
                                    (XrmDatabase)NULL));
      SetFooterText(footer_text, message);
      Free1(win_name_test, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                           OleNfooterMsg, OleTwinNameTest,
                           OleCOlClientOlpsMsgs,
                           OleMfooterMsg_winNameTest,
                          (XrmDatabase)NULL));
      (void) fclose(rw_file);
      return(-1);
    }
    if (ncolors_test = header_test.ncolors) {
        colors_test = (XColor *)calloc(ncolors_test,sizeof(XColor));
        if(fread((char *) colors_test, sizeof(XColor),
            ncolors_test, rw_file) != ncolors_test) {
                strcpy(message,name);
      strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                    OleNfooterMsg, OleTnotDumpFile,
                                    OleCOlClientOlpsMsgs,
                                    OleMfooterMsg_notDumpFile,
                                    (XrmDatabase)NULL));
                SetFooterText(footer_text, message);
      Free1(win_name_test, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                           OleNfooterMsg, OleTwinNameTest,
                           OleCOlClientOlpsMsgs,
                           OleMfooterMsg_winNameTest,
                          (XrmDatabase)NULL));
      Free1(colors_test, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                           OleNfooterMsg, OleTcolorsTest,
                           OleCOlClientOlpsMsgs,
                           OleMfooterMsg_colorsTest,
                          (XrmDatabase)NULL));
      		(void) fclose(rw_file);
                return(-1);
        }
    }
      Free1(win_name_test, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                           OleNfooterMsg, OleTwinNameTest,
                           OleCOlClientOlpsMsgs,
                           OleMfooterMsg_winNameTest,
                          (XrmDatabase)NULL));

      Free1(colors_test, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                           OleNfooterMsg, OleTcolorsTest,
                           OleCOlClientOlpsMsgs,
                           OleMfooterMsg_colorsTest,
                          (XrmDatabase)NULL));
    rewind(rw_file);	

    /*
     * Read in header information.
     */
    if((fread((char *)&header, sizeof(header), 1, rw_file)) != 1) {
	  OlVaDisplayErrorMsg(XtDisplay(toplevel),
			      OleNerrorMsg,
			      OleTcantReadHeader,
			      OleCOlClientOlpsMsgs,
			      OleMerrorMsg_cantReadHeader);
    }

    if (*(char *) &swaptest)
	_swaplong((char *) &header, sizeof(header));

    /*
     * check to see if the dump file is in the proper format.
     */
    if (header.file_version != XWD_FILE_VERSION) {
      fprintf(stderr, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                           OleNerrorMsg, OleTformatMissMatch,
                           OleCOlClientOlpsMsgs,
                           OleMerrorMsg_formatMissMatch,
                          (XrmDatabase)NULL));

	  OlVaDisplayErrorMsg(XtDisplay(toplevel),
			      OleNerrorMsg,
			      OleTexiting,
			      OleCOlClientOlpsMsgs,
			      OleMerrorMsg_exiting);
    }
    if (header.header_size < sizeof(header)) {
      fprintf(stderr, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                           OleNerrorMsg, OleTheaderTooSmall,
                           OleCOlClientOlpsMsgs,
                           OleMerrorMsg_headerTooSmall,
                          (XrmDatabase)NULL));

	  OlVaDisplayErrorMsg(XtDisplay(toplevel),
			      OleNerrorMsg,
			      OleTexiting,
			      OleCOlClientOlpsMsgs,
			      OleMerrorMsg_exiting);
    }

    /*
     * Calloc window name.
     */
      Free1(win_name, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                           OleNfooterMsg, OleTwinName,
                           OleCOlClientOlpsMsgs,
                           OleMfooterMsg_winName,
                          (XrmDatabase)NULL));

    win_name_size = (header.header_size - sizeof(header));

    if((win_name = calloc((unsigned) win_name_size, sizeof(char))) == NULL) {
	  OlVaDisplayErrorMsg(XtDisplay(toplevel),
			      OleNerrorMsg,
			      OleTcantCallocName,
			      OleCOlClientOlpsMsgs,
			      OleMerrorMsg_cantCallocName);
    }

    /*
     * Read in window name.
     */
    if(fread(win_name, sizeof(char), 
	win_name_size, rw_file) != win_name_size){
	  OlVaDisplayErrorMsg(XtDisplay(toplevel),
			      OleNerrorMsg,
			      OleTcantReadName,
			      OleCOlClientOlpsMsgs,
			      OleMerrorMsg_cantReadName);
    }

    image->width = (int) header.pixmap_width;
    image->height = (int) header.pixmap_height;
    image->xoffset = (int) header.xoffset;
    image->format = (int) header.pixmap_format;
    image->byte_order = (int) header.byte_order;
    image->bitmap_unit = (int) header.bitmap_unit;
    image->bitmap_bit_order = (int) header.bitmap_bit_order;
    image->bitmap_pad = (int) header.bitmap_pad;
    image->depth = (int) header.pixmap_depth;
    image->bits_per_pixel = (int) header.bits_per_pixel;
    image->bytes_per_line = (int) header.bytes_per_line;
    image->red_mask = header.red_mask;
    image->green_mask = header.green_mask;
    image->blue_mask = header.blue_mask;

/* CAUTION: do the following only if not shm, shm has already been taken care of
	else where - Xlib	*/

    if (!xlocal)	 {
	image->obdata = NULL;
        _XInitImageFuncPtrs(image);
    }


    /* Calloc the color map buffer.
     * Read it in, copy it and use the copy to query for the
     * existing colors at those pixel values.
     */
    if(ncolors = header.ncolors) {

      Free1(colors, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                           OleNfooterMsg, OleTcolors,
                           OleCOlClientOlpsMsgs,
                           OleMfooterMsg_colors,
                          (XrmDatabase)NULL));
	colors = (XColor *)calloc(ncolors,sizeof(XColor));

	if(fread((char *) colors, sizeof(XColor), 
	    ncolors, rw_file) != ncolors) {
	  OlVaDisplayErrorMsg(XtDisplay(toplevel),
			      OleNerrorMsg,
			      OleTcantReadMap,
			      OleCOlClientOlpsMsgs,
			      OleMerrorMsg_cantReadMap);
	}
	if (*(char *) &swaptest) {
	    for (i = 0; i < ncolors; i++) {
		_swaplong((char *) &colors[i].pixel, sizeof(long));
		_swapshort((char *) &colors[i].red, 3 * sizeof(short));
	    }
	}
    }

    /*
     * Calloc the pixel buffer.
	if (f_contents == True)
     */
      Free1(buffer, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                           OleNfooterMsg, OleTbuffer,
                           OleCOlClientOlpsMsgs,
                           OleMfooterMsg_buffer,
                          (XrmDatabase)NULL));

    buffer_size = Image_Size(image);

    /*  do the following only for non-shm	*/


    if (!xlocal)	{
	    if((buffer = calloc(buffer_size, 1)) == NULL)
	  OlVaDisplayErrorMsg(XtDisplay(toplevel),
			      OleNerrorMsg,
			      OleTcantCallocBuffer,
			      OleCOlClientOlpsMsgs,
			      OleMerrorMsg_cantCallocBuffer);
	    image->data = buffer;
    }

    /*
     * Read in the pixmap buffer.
     */
    if((status = fread(image->data, sizeof(char), (int)buffer_size, rw_file))
       != buffer_size){

        /*  Add elaboration on error here. %%*/
	  OlVaDisplayErrorMsg(XtDisplay(toplevel),
			      OleNerrorMsg,
			      OleTcantReadPixmap,
			      OleCOlClientOlpsMsgs,
			      OleMerrorMsg_cantReadPixmap);
    }

    /*
     * Close the input file.
     */
    (void) fclose(rw_file);

    return(0);
}



void
ImageToPane(image)
XImage * image;
{
    XVisualInfo vinfo, *vinfos;
	int j;
    Visual *visual = NULL;
    int win_depth;
    XSetWindowAttributes attributes;
    Bool inverse = False;
    Colormap colormap;

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
	image->format = XYBitmap;
    } else {
      fprintf(stderr, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                           OleNerrorMsg, OleTcantFindVisual,
                           OleCOlClientOlpsMsgs,
                           OleMerrorMsg_cantFindVisual,
                          (XrmDatabase)NULL));

	  OlVaDisplayErrorMsg(XtDisplay(toplevel),
			      OleNerrorMsg,
			      OleTexiting,
			      OleCOlClientOlpsMsgs,
			      OleMerrorMsg_exiting);
    }
    
    /* XXX */
    if (visual == DefaultVisual(dpy, screen)) {
        colormap = DefaultColormap(dpy, screen);
	/* XXX */
#ifdef notdef
	colormap = ModifyColors(image, visual, colormap, 
			colors, ncolors);
#endif
    } else {
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

    XFlush(dpy);
    /* If something in pane from previous, save its name for later freeing */
    if (f_contents == True) 
	oldpix = pix;

    /* Proper order -
	free former image
	blast new image onto pixmap
	(clear stub window)
	resize stub
	*set size of scrollw (first time only)
	copy in new pixmap
    */

    if (f_contents == False){

    	/* resize stub to size of image */
    	cnt = 0;
    	XtSetArg(args[cnt], XtNheight, image->height);         cnt++;
    	XtSetArg(args[cnt], XtNwidth, image->width);           cnt++;
    	XtSetValues(stub, args, cnt);

	if (image->height <= (int)olps_height_limit) {
        	cnt = 0;
    		XtSetArg(args[cnt], XtNheight, image->height+20);   cnt++;
    		XtSetValues(scrollw, args, cnt);
	}
	else {
        	cnt = 0;
    		XtSetArg(args[cnt], XtNheight, olps_height_limit);  cnt++;
    		XtSetValues(scrollw, args, cnt);
	}
    }
    else {
    	XClearWindow(dpy,stub_window);

    	/* resize stub to size of image */
    	cnt = 0;
    	XtSetArg(args[cnt], XtNheight, image->height);         cnt++;
    	XtSetArg(args[cnt], XtNwidth, image->width);           cnt++;
    	XtSetValues(stub, args, cnt);

	if (f_fromopenfile == False) {
	   if (f_toggle == False) {
		f_toggle = True;
        	cnt = 0;
        	XtSetArg(args[cnt], XtNwidth,  form_save_width+1);         cnt++;
        	XtSetArg(args[cnt], XtNheight, form_save_height+1);         cnt++;
        	XtSetValues(form, args, cnt);
	   } else {
		f_toggle = False;
        	cnt = 0;
        	XtSetArg(args[cnt], XtNwidth,  form_save_width-1);         cnt++;
        	XtSetArg(args[cnt], XtNheight, form_save_height-1);         cnt++;
        	XtSetValues(form, args, cnt);
	   }
    	}
    }

    pix = XCreatePixmap(dpy, stub_window, image->width, 
			image->height, image->depth);


    /* XXX */
    if (inverse) {
	gc_val.foreground = (unsigned long) WhitePixel (dpy, screen); 
	gc_val.background = (unsigned long) BlackPixel (dpy, screen); 
    } else {
	gc_val.foreground = (unsigned long) BlackPixel (dpy, screen);
	gc_val.background = (unsigned long) WhitePixel (dpy, screen); 
    }
    gc = XCreateGC (dpy, pix, GCForeground|GCBackground, &gc_val);
    /* free up old pixmap */
    if (f_contents == True)
    	XFreePixmap(dpy,oldpix);
    

    /* blast image onto pixmap */
#ifdef MITSHM    
    if(xlocal)	
	XShmPutImage(dpy, pix, gc, image, 0,0, 0,0,
                                 image->width, image->height, True);
    else	{
#endif      
	XPutImage(dpy, pix, gc, image, 0,0, 0,0,
              image->width, image->height);
#ifdef MITSHM	
    }
#endif    
    XCopyArea( dpy, pix, stub_window, gc, 0, 0, image->width, 
		image->height, 0, 0);

    f_contents = True;
    f_contentssaved = False;
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
	  OlVaDisplayErrorMsg(XtDisplay(toplevel),
			      OleNerrorMsg,
			      OleTcantQueryMap,
			      OleCOlClientOlpsMsgs,
			      OleMerrorMsg_cantQueryMap);
	for(i=0; i<ncolors; i++)
	  if(!ColorEqual(&colors[i], &copycolors[i])) {
	      copycolors = True;
	      break;
	  }
	if(debug1) {
     if(copycolors) fprintf(stderr, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                           OleNerrorMsg, OleToldColorsNeeded,
                           OleCOlClientOlpsMsgs,
                           OleMerrorMsg_oldColorsNeeded,
                          (XrmDatabase)NULL));
            else fprintf(stderr, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                           OleNerrorMsg, OleToldColorsMatch,
                           OleCOlClientOlpsMsgs,
                           OleMerrorMsg_oldColorsMatch,
                          (XrmDatabase)NULL));
	}
	cpixels = (unsigned long *)calloc(ncolors+1,sizeof(int));
	if(XAllocColorCells(dpy, colormap, 0, cplanes, 0, cpixels, 
	     (unsigned int) ncolors) == 0)

/*  Old arguments (for XGetColorCells() in X10) were: 
               0, ncolors, 0, &cplanes, cpixels %%*/

	  OlVaDisplayErrorMsg(XtDisplay(toplevel),
			      OleNerrorMsg,
			      OleTcantAllocateColors,
			      OleCOlClientOlpsMsgs,
			      OleMerrorMsg_cantAllocateColors);
	for(i=0; i<ncolors; i++) {
	    copycolors[i].pixel = cpixels[i];
	    copycolors[i].red   = colors[i].red;
	    copycolors[i].green = colors[i].green;
	    copycolors[i].blue  = colors[i].blue;
	    if(debug1) 
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
	  OlVaDisplayErrorMsg(XtDisplay(toplevel),
			      OleNerrorMsg,
			      OleTtooManyPlanes,
			      OleCOlClientOlpsMsgs,
			      OleMerrorMsg_tooManyPlanes);
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

/*
 * Determine the pixmap size.
 */

int Image_Size(image)
     XImage *image;
{
    if (image->format != ZPixmap)
      return(image->bytes_per_line * image->height * image->depth);

    return(image->bytes_per_line * image->height);

}

/*
 * Error1 - Fatal error.
 */
Error1(string)
	char *string;	/* Error1 description string. */
{
	fprintf(stderr, "Error1 => %s\n", string);

	if (errno != 0) {
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

Boolean
PSTriggerNotify OLARGLIST((w, win, x, y, selection, timestamp,
			    drop_site_id, op, send_done, forwarded, closure))
  OLARG( Widget,		w)
  OLARG( Window,		win)
  OLARG( Position,		x)
  OLARG( Position,		y)
  OLARG( Atom,			selection)
  OLARG( Time,			timestamp)
  OLARG( OlDnDDropSiteID,	drop_site_id)
  OLARG( OlDnDTriggerOperation,	op)
  OLARG( Boolean,		send_done)
  OLARG( Boolean,		forwarded)	/* not used */
  OLGRA( XtPointer,		closure)
{
	XtGetSelectionValue(
		w, selection, OL_XA_FILE_NAME(XtDisplay(w)),
		PSSelectionCB, send_done, timestamp
	);

	return(True);
} /* end of PSTriggerNotify */

static void
PSSelectionCB OLARGLIST ((w, client_data, selection, type, value, length,
			   format))
  OLARG( Widget,		w)
  OLARG( XtPointer,		client_data)
  OLARG( Atom *,		selection)
  OLARG( Atom *,		type)
  OLARG( XtPointer,		value)
  OLARG( unsigned long *,	length)
  OLGRA( int *,			format)
{
  Boolean send_done = (Boolean)client_data;
  String fullname;

  /* Since only OL_XA_FILE_NAME(dpy) is passed in, we know we have
     a valid type.
   */

  fullname = (String) value;

  Open_File(fullname);

  /* Errors handled by Open_File.  We don't care if there was an error.  */
  /* The transaction is done regardless. */

  XtFree(value);

  if (send_done == True) {
    OlDnDDragNDropDone(w, *selection, CurrentTime, NULL, NULL);
  }
} /* end of PSSelectionCB */



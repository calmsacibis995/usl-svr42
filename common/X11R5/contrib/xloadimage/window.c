#ident	"@(#)r4xloadimage:window.c	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* window.c:
 *
 * display an image in a window
 *
 * jim frost 10.03.89
 *
 * Copyright 1989, 1990 Jim Frost.  See included file "copyright.h" for
 * complete copyright information.
 */

#include "copyright.h"
#include "xloadimage.h"
#include <ctype.h>
#include <X11/cursorfont.h>

static Window ImageWindow= 0;
XClientMessageEvent *client_event  ;

Atom   wm_atom;		/**	To set the WM_PROTOCOLS **/
static void setCursor(disp, window, iw, ih, ww, wh, cursor)
     Display      *disp;
     Window        window;
     unsigned int  iw, ih;
     unsigned int  ww, wh;
     Cursor       *cursor;
{ XSetWindowAttributes swa;

  if ((ww >= iw) && (wh >= ih))
    swa.cursor= XCreateFontCursor(disp, XC_icon);
  else if ((ww < iw) && (wh >= ih))
    swa.cursor= XCreateFontCursor(disp, XC_sb_h_double_arrow);
  else if ((ww >= iw) && (wh < ih))
    swa.cursor= XCreateFontCursor(disp, XC_sb_v_double_arrow);
  else
    swa.cursor= XCreateFontCursor(disp, XC_fleur);
  XChangeWindowAttributes(disp, window, CWCursor, &swa);
  XFreeCursor(disp, *cursor);
  *cursor= swa.cursor;
}

/* place an image
 */

static void placeImage(width, height, winwidth, winheight, rx, ry)
     int width, height, winwidth, winheight;
     int *rx, *ry; /* supplied and returned */
{ int pixx, pixy;

  pixx= *rx;
  pixy= *ry;

  if (winwidth > width)
    pixx= (winwidth - width) / 2;
  else {
    if ((pixx < 0) && (pixx + width < winwidth))
      pixx= winwidth - width;
    if (pixx > 0)
      pixx= 0;
  }
  if (winheight > height)
    pixy= (winheight - height) / 2;
  else {
    if ((pixy < 0) && (pixy + height < winheight))
      pixy= winheight - height;
    if (pixy > 0)
      pixy= 0;
  }
  *rx= pixx;
  *ry= pixy;
}

/* blit an image
 */

static void blitImage(disp, pixmap, window, gc, pixx, pixy, width, height,
		      winwidth, winheight, x, y, w, h)
     Display *disp;
     Pixmap   pixmap;
     Window   window;
     GC       gc;
     int      pixx, pixy, width, height, winwidth, winheight, x, y, w, h;
{
  if (x + w > winwidth)
    w= winwidth - x;
  if (y + h > winheight)
    h= winheight - y;
  if (x < pixx) {
    XClearArea(disp, window, x, y, pixx - x, y + h, False);
    w -= (pixx - x);
    x= pixx;
  }
  if (y < pixy) {
    XClearArea(disp, window, x, y, w, pixy - y, False);
    h -= (pixy - y);
    y= pixy;
  }
  if (x + w > pixx + width) {
    XClearArea(disp, window, pixx + width, y, w - width, h, False);
    w= width;
  }
  if (y + h > pixy + height) {
    XClearArea(disp, window, x, pixy + height, w, h - height, False);
    h= height;
  }
  XCopyArea(disp, pixmap, ImageWindow, gc, x - pixx, y - pixy, w, h,
	    x, y);
}

/* clean up static window if we're through with it
 */

void cleanUpWindow(disp)
     Display *disp;
{
  if (ImageWindow)
    XDestroyWindow(disp, ImageWindow);
  ImageWindow= 0;
}

char imageInWindow(disp, scrn, image, winx, winy, winwidth, winheight,
		   fullscreen, install, slideshow, verbose)
     Display      *disp;
     int           scrn;
     Image        *image;
     unsigned int  winx, winy, winwidth, winheight;
     unsigned int  fullscreen;
     unsigned int  install;
     unsigned int  slideshow;
     unsigned int  verbose;
{ Pixmap               pixmap;
  Colormap             xcmap;
  XSetWindowAttributes swa;
  XSizeHints           sh;
  XWMHints             wmh;
  XGCValues            gcv;
  GC                   gc;
  int                  pixx, pixy;
  int                  lastx, lasty, mousex, mousey;
  int                  paint;
  union {
    XEvent              event;
    XAnyEvent           any;
    XButtonEvent        button;
    XKeyEvent           key;
    XConfigureEvent     configure;
    XExposeEvent        expose;
    XMotionEvent        motion;
    XResizeRequestEvent resize;
  } event;

  /* figure out the window size.  unless specifically requested to do so,
   * we will not exceed 90% of display real estate.
   */

  if (fullscreen) {
    winwidth= DisplayWidth(disp, scrn);
    winheight= DisplayHeight(disp, scrn);
  }
  else {
    lastx= (winwidth || winheight); /* user set size flag */
    if (!winwidth) {
      winwidth= image->width;
      if (winwidth > DisplayWidth(disp, scrn) * 0.9)
	winwidth= DisplayWidth(disp, scrn) * 0.9;
    }
    if (!winheight) {
      winheight= image->height;
      if (winheight > DisplayHeight(disp, scrn) * 0.9)
	winheight= DisplayHeight(disp, scrn) * 0.9;
    }
  }

  if (! sendImageToX(disp, scrn, DefaultVisual(disp, scrn),
		     image, &pixmap, &xcmap, verbose))
    exit(1);
  swa.background_pixel= 0;
  swa.backing_store= NotUseful;
  swa.bit_gravity= NorthWestGravity;
  swa.cursor= XCreateFontCursor(disp, XC_watch);
  swa.colormap= xcmap;
  swa.event_mask= ButtonPressMask | Button1MotionMask | KeyPressMask |
    ExposureMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask;
  swa.save_under= False;
  swa.override_redirect= (fullscreen ? True : False);
/*
 *  USPosition is always ignored if Zeroes are passed to XCreateWindow
 *  winx and winy are now passed to XCreateWindow which will be Zero 
 *  if geometry is not specified .
 */

  if (!ImageWindow) {
    ImageWindow= XCreateWindow(disp, RootWindow(disp, scrn), winx, winy,
			       winwidth, winheight, 0,
			       DefaultDepth(disp, scrn),
			       InputOutput, CopyFromParent,
			       CWBackPixel | CWBackingStore |
			       CWBitGravity | CWCursor | CWColormap |
			       CWEventMask | CWSaveUnder, &swa);
    paint= 0;
  }
  else {
    XResizeWindow(disp, ImageWindow, winwidth, winheight);
	XSetWindowColormap(disp,ImageWindow,xcmap);
    paint= 1;
  }
/*
 *	WIPRO : vivek t.
 *	CHANGE # UNKNOWN
 *	FILE # window.c
 * 	XSetColormap is been done in case the window already existed. The Colormap
 *	would now get installed through the window manager for all the images
 *	in -slideshow option.
 *	ENDCHANGE # UNKNOWN
 */
  XStoreName(disp, ImageWindow, image->title);
  XSetIconName(disp, ImageWindow, image->title);

  sh.width= winwidth;
  sh.height= winheight;
  if (fullscreen) {
    sh.min_width= sh.max_width= winwidth;
    sh.min_height= sh.max_height= winheight;
  }
  else {
    sh.min_width= 1;
    sh.min_height= 1;
    sh.max_width= image->width;
    sh.max_height= image->height;
  }
  sh.width_inc= 1;
  sh.height_inc= 1;
  sh.flags= PMinSize | PMaxSize | PResizeInc;
  if (lastx || fullscreen)
    sh.flags |= USSize;
  else
    sh.flags |= PSize;
  if (fullscreen) {
    sh.x= sh.y= 0;
    sh.flags |= USPosition;
  }
  else if (winx || winy) {
    sh.x= winx;
    sh.y= winy;
    sh.flags |= USPosition;
  }
  XSetNormalHints(disp, ImageWindow, &sh);
  wmh.input= True;
  wmh.flags= InputHint;
  XSetWMHints(disp, ImageWindow, &wmh);

  gcv.function= GXcopy;
  gcv.foreground= 0;
  gc= XCreateGC(disp, ImageWindow, GCFunction | GCForeground, &gcv);

/*
 *	WIPRO : Neeti   
 *	CHANGE # UNKNOWN
 *	FILE # window.c
 *	Set the WM_DELETE_WINDOW window manager protocol. 
 *	ENDCHANGE # UNKNOWN
 */
	wm_atom = XInternAtom(disp, "WM_DELETE_WINDOW", False);
	if ( XSetWMProtocols (disp, ImageWindow, &wm_atom, 1) == 0)
	  fprintf (stderr, "xloadimage: Not able to set WM_DELETE_WINDOW protocol.\n");


  XMapWindow(disp, ImageWindow);
  placeImage(image->width, image->height, winwidth, winheight, &pixx, &pixy);
  if (paint)
    blitImage(disp, pixmap, ImageWindow, gc,
	      pixx, pixy, image->width, image->height, winwidth, winheight,
	      0, 0, winwidth, winheight);
  setCursor(disp, ImageWindow, image->width, image->height,
	    winwidth, winheight, &(swa.cursor));

  lastx= lasty= -1;
  for (;;) {
    XNextEvent(disp, (XEvent*)&event);
    switch (event.any.type) {


		 /***	Check if WM_DELETE_WINDOW message has been sent ***/
		 case ClientMessage:
				client_event = (XClientMessageEvent *)(&event);
				if (client_event->data.l[0] == wm_atom)
					exit(0);
				break;

    case ButtonPress:
      if (event.button.button == 1) {
	lastx= event.button.x;
	lasty= event.button.y;
	break;
      }
      break;

    case KeyPress: {
      char buf[128];
      KeySym ks;
      XComposeStatus status;
      char ret;
      Cursor cursor;

      XLookupString(&event.key,buf,128,&ks,&status);
      ret= buf[0];
      if (isupper(ret))
	ret= tolower(ret);
      switch (ret) {
      case 'n':
      case 'p':
	cursor= swa.cursor;
	swa.cursor= XCreateFontCursor(disp, XC_watch);
	XChangeWindowAttributes(disp, ImageWindow, CWCursor, &swa);
	XFreeCursor(disp, cursor);
	XFlush(disp);
	/* FALLTHRU */
      case '\003': /* ^C */
      case 'q':
	XFreeCursor(disp, swa.cursor);
	XFreePixmap(disp, pixmap);
	if (xcmap != DefaultColormap(disp, scrn))
          XFreeColormap(disp, xcmap);
	return(ret);
      }
      break;
    }

    case MotionNotify:
      if ((image->width <= winwidth) && (image->height <= winheight))
	break; /* we're AT&T */
      mousex= event.button.x;
      mousey= event.button.y;
      while (XCheckTypedEvent(disp, MotionNotify, (XEvent*)&event) == True) {
	mousex= event.button.x;
	mousey= event.button.y;
      }
      pixx -= (lastx - mousex);
      pixy -= (lasty - mousey);
      lastx= mousex;
      lasty= mousey;
      placeImage(image->width, image->height, winwidth, winheight,
		 &pixx, &pixy);
      blitImage(disp, pixmap, ImageWindow, gc,
		pixx, pixy, image->width, image->height, winwidth, winheight,
		0, 0, winwidth, winheight);
      break;

    case ConfigureNotify:
      winwidth= event.configure.width;
      winheight= event.configure.height;

      placeImage(image->width, image->height, winwidth, winheight,
		 &pixx, &pixy);

      /* configure the cursor to indicate which directions we can drag
       */

      setCursor(disp, ImageWindow, image->width, image->height,
		winwidth, winheight, &(swa.cursor));

      /* repaint 
       */

      blitImage(disp, pixmap, ImageWindow, gc,
		pixx, pixy, image->width, image->height, winwidth, winheight,
		0, 0, winwidth, winheight);
      break;

    case DestroyNotify:
      XFreeCursor(disp, swa.cursor);
      XFreePixmap(disp, pixmap);
      if (xcmap != DefaultColormap(disp, scrn))
	XFreeColormap(disp, xcmap);
      return('\0');

    case Expose:
      blitImage(disp, pixmap, ImageWindow, gc,
		pixx, pixy, image->width, image->height, winwidth, winheight,
		event.expose.x, event.expose.y,
		event.expose.width, event.expose.height);
      break;

    case EnterNotify:
      if (install)
	XInstallColormap(disp, xcmap);
      break;

    case LeaveNotify:
      if (install)
	XUninstallColormap(disp, xcmap);
    }
  }
}

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:button/unit_test3.c	1.9"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/ControlAre.h>
#include <Xol/OpenLook.h>
#include <Xol/RectButton.h>
#include <Xol/OblongButt.h>

XImage *t_colorimg();
XImage *pushpin_image();
static XImage *CreateImage();

Widget imageButton;
Widget pushpinButton;
Widget tileImageButton;
Widget busyTileImageButton;
Widget busyImageButton;
Widget setImageButton;
Widget oblongImageButton;
Widget oblongTileImageButton;

#define pushpin_width 17
#define pushpin_height 11
static char pushpin_bits[] = {
   0x00, 0x03, 0x00, 0x80, 0x03, 0x00, 0x83, 0x03, 0x00, 0xff, 0x03, 0x00,
   0xff, 0x03, 0x00, 0xff, 0xff, 0x01, 0xff, 0x03, 0x00, 0xff, 0x03, 0x00,
   0x83, 0x03, 0x00, 0x80, 0x03, 0x00, 0x00, 0x03, 0x00};
/*
#define pushpin_width 32
#define pushpin_height 12
static char pushpin_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x80, 0x43, 0x00,
   0x00, 0x40, 0x81, 0x00, 0x00, 0x20, 0x81, 0x00, 0x00, 0x20, 0x43, 0x00,
   0x00, 0x20, 0x7e, 0x00, 0x00, 0x60, 0x40, 0x00, 0x00, 0xc0, 0x30, 0x00,
   0x00, 0x60, 0x0f, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
*/

void Quitcallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	exit(0);
}


int main(argc, argv)
unsigned int argc;
char **argv;
{

	Widget toplevel, box, quitButton;
	Arg arg[20];
	unsigned int n;

	toplevel = OlInitialize("toplevel",
		"Toplevel",
		NULL,
		0,
		&argc,
		argv);

	n=0;
	XtSetArg(arg[n], XtNlayoutType, OL_FIXEDCOLS); n++;
	XtSetArg(arg[n], XtNmeasure, 3); n++;
	box = XtCreateManagedWidget("box",
		controlAreaWidgetClass,
		toplevel,
		arg,
		2);


	n = 0;
	quitButton = XtCreateManagedWidget("Quit",
		oblongButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(quitButton, XtNselect, Quitcallback, NULL);	


	n = 0;
	XtSetArg(arg[n], XtNlabelType, OL_IMAGE);		n++;
	XtSetArg(arg[n], XtNlabelImage,
		t_colorimg(XtDisplay(toplevel), 32, 32));	n++;
	imageButton = XtCreateManagedWidget("image button",
		rectButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNlabelType, OL_IMAGE);		n++;
	XtSetArg(arg[n], XtNset, TRUE);				n++;
	XtSetArg(arg[n], XtNlabelImage,
		t_colorimg(XtDisplay(toplevel), 32, 32));	n++;
	setImageButton = XtCreateManagedWidget("set image button",
		rectButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNlabelType, OL_IMAGE);		n++;
/*
	XtSetArg(arg[n], XtNscale, EXTRA_LARGE_SCALE);			n++;
*/
	XtSetArg(arg[n], XtNlabelImage,
		t_colorimg(XtDisplay(toplevel), 16, 16));	n++;
	oblongImageButton = XtCreateManagedWidget("set image button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNlabelType, OL_IMAGE);		n++;
	XtSetArg(arg[n], XtNlabelTile, TRUE);			n++;
	XtSetArg(arg[n], XtNlabelImage,
		t_colorimg(XtDisplay(toplevel), 32, 32));	n++;
	tileImageButton = XtCreateManagedWidget("image button",
		rectButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNlabelType, OL_IMAGE);		n++;
	XtSetArg(arg[n], XtNlabelTile, TRUE);			n++;
/*
	XtSetArg(arg[n], XtNscale, EXTRA_LARGE_SCALE);			n++;
*/
	XtSetArg(arg[n], XtNlabelImage,
		t_colorimg(XtDisplay(toplevel), 16, 16));	n++;
	oblongTileImageButton = XtCreateManagedWidget("set image button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNlabelType, OL_IMAGE);		n++;
	XtSetArg(arg[n], XtNlabelImage,
		pushpin_image(XtDisplay(toplevel)));		n++;
	pushpinButton = XtCreateManagedWidget("image button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNlabelType, OL_IMAGE);		n++;
	XtSetArg(arg[n], XtNlabelImage,
		CreateImage(XtScreen(box),
			pushpin_bits,
			pushpin_width,
			pushpin_height));			n++;
	pushpinButton = XtCreateManagedWidget("image button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNlabelType, OL_IMAGE);		n++;
	XtSetArg(arg[n], XtNlabelTile, TRUE);			n++;
	XtSetArg(arg[n], XtNlabelImage,
		CreateImage(XtScreen(box),
			pushpin_bits,
			pushpin_width,
			pushpin_height));			n++;
	pushpinButton = XtCreateManagedWidget("image button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNlabelType, OL_IMAGE);		n++;
	XtSetArg(arg[n], XtNbusy, TRUE);			n++;
	XtSetArg(arg[n], XtNlabelImage,
		CreateImage(XtScreen(box),
			pushpin_bits,
			pushpin_width,
			pushpin_height));			n++;
	pushpinButton = XtCreateManagedWidget("image button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNlabelType, OL_IMAGE);		n++;
	XtSetArg(arg[n], XtNlabelTile, TRUE);			n++;
	XtSetArg(arg[n], XtNbusy, TRUE);			n++;
	XtSetArg(arg[n], XtNlabelImage,
		CreateImage(XtScreen(box),
			pushpin_bits,
			pushpin_width,
			pushpin_height));			n++;
	pushpinButton = XtCreateManagedWidget("image button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNlabelType, OL_IMAGE);		n++;
	pushpinButton = XtCreateManagedWidget("default label",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	XtRealizeWidget(toplevel);
	XtMainLoop();
}


#include <stdio.h>
#include "X11/Xlib.h"

#define INTERVAL	3
#define tWHITEONBLACK	1
#define tBLACKONWHITE	0

XImage *t_colorimg (dpy, w, h)
Display *dpy;
int	w, h;	/* width and height define the size of pixmap */
{
  static GC	 gc = NULL;
  XImage *tmp, *t_monoimg ();
  int	 screen = XDefaultScreen (dpy);
  Window root = XDefaultRootWindow (dpy);
  int    depth = XDefaultDepth (dpy, screen);
  Colormap cmap = XDefaultColormap (dpy, screen);
  Pixmap pid;
  int	 len;
  XColor dummy, r, g, b, cyan;
  int	 forg;
  int    op;

  op = tWHITEONBLACK;	/* force it ??? */
  if (XAllocNamedColor (dpy, cmap, "red", &dummy, &r) == False)
    return (t_monoimg (dpy, w, h));
  if (XAllocNamedColor (dpy, cmap, "green", &dummy, &g) == False)
    return (t_monoimg (dpy, w, h));
  if (XAllocNamedColor (dpy, cmap, "blue", &dummy, &b) == False)
    return (t_monoimg (dpy, w, h));
  if (XAllocNamedColor (dpy, cmap, "cyan", &dummy, &cyan) == False)
    return (t_monoimg (dpy, w, h));

  if (gc == NULL)
    gc = XCreateGC (dpy, root, 0, NULL);
  pid = XCreatePixmap (dpy, root, w, h, depth);
  len = (w > h) ? h : w;
  if (len <= INTERVAL)
  {
    XSetForeground (dpy, gc, r.pixel);
    XSetFillStyle (dpy, gc, FillSolid);
    XFillRectangle (dpy, pid, gc, 0, 0, w, h);
    XDrawRectangle (dpy, pid, gc, 0, 0, w, h);
  }
  else
  {
    int i = 0, pos, ww, hh, color [4];

    color [0] = r.pixel;
    color [1] = g.pixel;
    color [2] = b.pixel;
    color [3] = cyan.pixel;
    forg = (op != tBLACKONWHITE) ? XWhitePixel (dpy, screen)
			         : XBlackPixel (dpy, screen);
    XSetForeground (dpy, gc, forg);
    XSetFillStyle (dpy, gc, FillSolid);
    XFillRectangle (dpy, pid, gc, 0, 0, w, h);
    XDrawRectangle (dpy, pid, gc, 0, 0, w, h);
/*
XSync (dpy, 0);
fflush (stdout); fflush (stdin);
getchar ();
*/
    while (1)
    {
      pos = (i+1) * INTERVAL;
      ww = w - (i+1) * 2 * INTERVAL;
      hh = h - (i+1) * 2 * INTERVAL;
      if (ww <= 0 || hh <= 0)
        break;
/*
printf ("pos: %d, ww: %d, hh: %d\n", pos, ww, hh);
*/
      XSetForeground (dpy, gc, color [i % 4]);
      XDrawRectangle (dpy, pid, gc, pos, pos, ww, hh);
      i++;
/*
XSync (dpy, 0);
fflush (stdout); fflush (stdin);
getchar ();
*/
    }
  }
/*
printf ("w: %d, h: %d\n", w, h);
*/
  tmp = XGetImage (dpy, pid, 0, 0, w, h, XAllPlanes (), ZPixmap);
  XFreePixmap (dpy, pid);
  return (tmp);
}

XImage *t_monoimg (dpy, w, h)
Display *dpy;
int	w, h;	/* width and height define the size of pixmap */
{
  static GC	 gc = NULL;
  int	 screen = XDefaultScreen (dpy);
  XImage *tmp;
  Window root = XDefaultRootWindow (dpy);
  int    depth = XDefaultDepth (dpy, screen);
  Pixmap pid;
  int	 backg, forg;
  int	 len;
  int    op;

  op = tWHITEONBLACK;
  if (gc == NULL)
    gc = XCreateGC (dpy, root, 0, NULL);
  pid = XCreatePixmap (dpy, root, w, h, depth);
  backg = (op == tBLACKONWHITE) ? XWhitePixel (dpy, screen)
				: XBlackPixel (dpy, screen);
  forg = (op != tBLACKONWHITE) ? XWhitePixel (dpy, screen)
			       : XBlackPixel (dpy, screen);
  XSetForeground (dpy, gc, forg);
  XSetFillStyle (dpy, gc, FillSolid);
  XFillRectangle (dpy, pid, gc, 0, 0, w, h);
  XDrawRectangle (dpy, pid, gc, 0, 0, w, h);
  len = (w > h) ? h : w;
  if (len > INTERVAL)
  {
    int i = 0, pos, ww, hh;

    XSetForeground (dpy, gc, backg);
    while (1)
    {
      pos = (i+1) * INTERVAL;
      ww = w - (i+1) * 2 * INTERVAL;
      hh = h - (i+1) * 2 * INTERVAL;
      if (ww <= 0 || hh <= 0)
        break;
      XDrawRectangle (dpy, pid, gc, pos, pos, ww, hh);
      i++;
    }
  }
  tmp = XGetImage (dpy, pid, 0, 0, w, h, XAllPlanes (), ZPixmap);
  XFreePixmap (dpy, pid);
  return (tmp);
}

t_privimg (dpy, w, h, pimage)
Display *dpy;
int     w, h;
XImage  *pimage;
{
  XImage *tmp, *t_colorimg ();

  tmp = t_colorimg (dpy, w, h);
/*
printf ("size: %d\n", sizeof (XImage));
*/
  memcpy (pimage, tmp, sizeof (XImage));
}

XImage *pushpin_image (dpy)
Display *dpy;
{
	int	 screen = XDefaultScreen (dpy);
	XImage *tmp;
	Window root = XDefaultRootWindow (dpy);
	int    depth = XDefaultDepth (dpy, screen);
	Pixmap pushpin_pixmap;
	int	 backg, forg;
	int    op;

	op = tWHITEONBLACK;

	backg = (op == tBLACKONWHITE) ? XWhitePixel (dpy, screen)
				: XBlackPixel (dpy, screen);
	forg = (op != tBLACKONWHITE) ? XWhitePixel (dpy, screen)
			       : XBlackPixel (dpy, screen);
	pushpin_pixmap = XCreatePixmapFromBitmapData(dpy,
		root,
		pushpin_bits,
		pushpin_width,
		pushpin_height,
		forg,		/* foreground always off */
		backg,		/* background always on  */
		(unsigned) depth);
	tmp = XGetImage (dpy,
		pushpin_pixmap,
		0,
		0,
		pushpin_width,
		pushpin_height,
		XAllPlanes(),
		XYPixmap);
	XFreePixmap (dpy, pushpin_pixmap);
	return (tmp);
}

#define BYTES_PER_LINE(w)       ((w & 7L) != 0 ? (w >>3) + 1 : (w >>3))


/*************************************************************************
 * CreateImage - creates an XImage out of bitmap data
 ****************************function*header*****************************/
static XImage *
CreateImage(screen, data, width, height)
        Screen          *screen;        /* Intended screen              */
        char            *data;          /* Bitmap bits                  */
        unsigned int     width;         /* Bitmap width                 */
        unsigned int     height;        /* Bitmap height                */
{
        register Display *dpy = DisplayOfScreen(screen);
        register XImage  *image;

        image = XCreateImage(dpy,                       /* Display      */
                        DefaultVisualOfScreen(screen),  /* Visual       */
                        (unsigned int) 1,               /* Depth        */
                        XYBitmap,                       /* Format       */
                        0,                              /* Offset       */
                        data,                           /* Image data   */
                        width,                          /* Image width  */
                        height,                         /* Image height */
                        BitmapPad(dpy),                 /* Bitmap pad   */
                        BYTES_PER_LINE(width));         /* Bytes/line   */

                                /* Now customize for the Bitmap file
                                 * format                               */

        image->byte_order       = MSBFirst;
        image->bitmap_unit      = 8;
        image->bitmap_bit_order = LSBFirst;

        return(image);

} /* END OF CreateImage() */


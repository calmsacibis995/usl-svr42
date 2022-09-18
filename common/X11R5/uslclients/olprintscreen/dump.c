/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olps:dump.c	1.13"
#endif

#include <X11/copyright.h>
/* Copyright 1987 Massachusetts Institute of Technology */

/*
 * dump.c - based on xwd.c code: routines
 * related to taking dump of screen/window/area,
 * also to save dump into a file
 */

#include "main.h"
#include "externs.h"
#include "error.h"

static	int format = ZPixmap;
extern int xlocal;
extern XImage *shmimage;
extern int noflag;
extern char pixmapformat[];

#define ROUNDUP(nbytes, pad) ((((nbytes) + ((pad) - 1)) / (pad)) * (pad))
#define PROP_ZFORMAT	"ZPixmap"
#define PROP_XYFORMAT	"XYPixmap"


int
PaneToFile(	name,	    image,
header_p, 
win_name_p, win_name_size_p,
colors_p,   ncolors_p, 
buffer_size_p)

char 		*  name;
XImage 		*  image;
XWDFileHeader 	*  header_p;
char 		*  win_name_p;
int		   win_name_size_p;
XColor		*  colors_p;
int		   ncolors_p;
unsigned	   buffer_size_p;

{
	int i;
	int test = False;

	if (CanOpenFile(name,"w",&rw_file) == -1)
		return (-1);

#ifdef MITSHM	
	if ((noflag) && (xlocal))	{
		XShmGetImage(dpy, pix, image, 0, 0,~0);
		noflag = 0;
	}
#endif	

	if ((f_bypass == False)
	    && (f_contentssaved == False)
	    && (*(char *) &swaptest))
		test = True;
	else if ((f_bypass == True)
	    && (*(char *) &swaptest))
		test = True;
	if (test == True) {
		_swaplong((char *) header_p, sizeof(*header_p));
		for (i = 0; i < ncolors_p; i++) {
			_swaplong((char *) &colors_p[i].pixel, sizeof(long));
			_swapshort((char *) &colors_p[i].red, 3 * sizeof(short));
		}
	}

	(void) fwrite((char *)header_p, sizeof(*header_p), 1, rw_file);
	(void) fwrite(win_name_p, win_name_size_p, 1, rw_file);
	(void) fwrite((char *) colors_p, sizeof(XColor), ncolors_p, rw_file);
	(void) fwrite(image->data, (int) buffer_size_p, 1, rw_file);
	(void) fclose(rw_file);
	if (f_bypass == False)
		f_contentssaved = True;

	return (0);
}

static Window target_win;

void
WindowToImage(	image, 
header_p,   header_size_p, 
win_name_p, win_name_size_p,
colors_p,   ncolors_p, 
buffer_size_p)

XImage 		** image;
XWDFileHeader 	*  header_p;
int		*  header_size_p;
char 		** win_name_p;
int		*  win_name_size_p;
XColor		** colors_p;
int		*  ncolors_p;
unsigned	*  buffer_size_p;

{
	if (f_screen == False)
		target_win = Select_Window(dpy);
	else{
		target_win = rootwindow;
		sleep(3);
	}

	/*
     	 * Dump it!
     	 */
	Window_Dump(target_win,
	    image, 
	    header_p,   header_size_p, 
	    win_name_p, win_name_size_p,
	    colors_p,   ncolors_p, 
	    buffer_size_p,
	    0,0,0,0);

	if (f_bypass == False)
		f_contentssaved = False;
}


static XColor          	ForegroundColor;
static XColor          	BackgroundColor;
static XtResource resources[] =
{
	{ "foreground", "Foreground", XtRPixel, sizeof(Pixel),
	(Cardinal) &ForegroundColor, XtRString, "Black"},
	{ "background", "Background", XtRPixel, sizeof(Pixel),
	(Cardinal) &BackgroundColor, XtRString, "White"},
};
static XGCValues       	gcv;
static GC  		invertGC;
static XPoint	  	pt[5];
static f_lines_exist 	= False;
#define OUTLINE		1


DrawOutline(x1, y1, x2, y2)
int x1, y1, x2, y2;
{

	if (f_lines_exist == True) {
		XDrawLines(dpy, rootwindow, invertGC, pt, 5, CoordModeOrigin);
		f_lines_exist = False;
	}

	/* since they are points, don't have to check for directions */
	pt[0].x 	= x1;
	pt[0].y 	= y1;

	pt[1].x 	= x2;
	pt[1].y 	= y1;

	pt[2].x 	= x2;
	pt[2].y 	= y2;

	pt[3].x 	= x1;
	pt[3].y 	= y2;

	pt[4].x 	= x1;
	pt[4].y 	= y1;

	XDrawLines(dpy, rootwindow, invertGC, pt, 5, CoordModeOrigin);
	f_lines_exist = True;
}


void
WindowToImage2(	image, 
header_p,   header_size_p, 
win_name_p, win_name_size_p,
colors_p,   ncolors_p, 
buffer_size_p)

XImage 		** image;
XWDFileHeader 	*  header_p;
int		*  header_size_p;
char 		** win_name_p;
int		*  win_name_size_p;
XColor		** colors_p;
int		*  ncolors_p;
unsigned	*  buffer_size_p;

{

	XEvent event;
	Window target_win = None;

	int f_pressed = False;
	int f_released = False;
	Window  	window_dummy;
	int     	root_x_return1, root_y_return1;
	int     	root_x_return2, root_y_return2;
	int     	root_x, root_y;
	int     	last_root_x, last_root_y;
	int		moving_root_x, moving_root_y;
	int		int_dummy;
	unsigned int    mask_return;

	OlVirtualEventRec	ve;


	target_win = rootwindow;

	XtGetApplicationResources(toplevel, NULL, resources,
	    XtNumber(resources), NULL, 0);
	gcv.graphics_exposures = False;
	gcv.function = GXinvert;
	gcv.foreground = ForegroundColor.pixel;
	gcv.background = BackgroundColor.pixel;
	gcv.subwindow_mode = IncludeInferiors;
	gcv.line_width = OUTLINE;
	gcv.line_style = LineSolid;
	gcv.cap_style = CapProjecting;
	gcv.join_style = JoinMiter;
	gcv.fill_style = FillSolid;

	invertGC = XCreateGC(dpy, rootwindow, GCFunction | GCSubwindowMode |
	    GCForeground | GCBackground | GCGraphicsExposures, &gcv);

	/* Grab the pointer using target cursor, letting it roam all over */
	while (XGrabPointer(dpy, RootWindow(dpy, screen), False,
	    ButtonPressMask|ButtonReleaseMask, GrabModeAsync,
	    GrabModeAsync, None, sa_cursor, CurrentTime)
	    != GrabSuccess);

        /* Grab the Keyboard to allow for mouseless operation */
	if(XGrabKeyboard(dpy, RootWindow(dpy, screen), False, GrabModeAsync,
		GrabModeAsync, CurrentTime) != GrabSuccess) {
		fprintf(stderr, "Cannot capture Keyboard\n");
	}

	f_lines_exist = False;
	/* Let the user select 2 points (i.e., an area) ... */
	while (f_released == False) {

		if (XCheckMaskEvent(dpy, ButtonPressMask|ButtonReleaseMask|KeyPressMask,
		    &event) == True)
		{
				/* get the virtual binding */
			OlLookupInputEvent(toplevel, &event, &ve,
					   OL_DEFAULT_IE);
			switch (event.type) {
			case ButtonPress:
					/* make sure it's select button
			   			and not pressed already */
				if (ve.virtual_name == OL_SELECT
				    && (f_pressed == False)) {
					last_root_x = root_x_return1 = event.xbutton.x_root;
					last_root_y = root_y_return1 = event.xbutton.y_root;
					f_pressed = True;
				}
				break;
			case ButtonRelease:
					/* make sure it's select button
				   	   and pressed already */
				if (ve.virtual_name == OL_SELECT
				    && (f_pressed == True)) {
					root_x_return2 = event.xbutton.x_root;
					root_y_return2 = event.xbutton.y_root;
					f_released = True;
				}
				break;

			case KeyPress:
				if (ve.virtual_name == OL_SELECTKEY
				    && (f_pressed == False)) {
					moving_root_x = last_root_x = root_x_return1 = event.xbutton.x_root;
					moving_root_y = last_root_y = root_y_return1 = event.xbutton.y_root;
					f_pressed = True;
				}
				else 
					if (ve.virtual_name == OL_SELECTKEY
				    	&& (f_pressed == True)) {
						root_x_return2 = event.xbutton.x_root;
						root_y_return2 = event.xbutton.y_root;
						f_released = True;
					}
                                if (!(ve.virtual_name == OL_SELECT)) {
					moving_root_x = last_root_x = event.xbutton.x_root;
					moving_root_y = last_root_y = event.xbutton.y_root;
					HandleCursor(ve.virtual_name, 
						      &moving_root_x, &moving_root_y);
					XWarpPointer(dpy, None, 
							RootWindow(dpy, screen),
							0,0, (unsigned int) 0, 
							(unsigned int) 0,
							moving_root_x,
							moving_root_y);
					if (f_pressed == True) {
						(void) XQueryPointer(dpy, rootwindow, 
				    		&window_dummy, &window_dummy, 
				    		&root_x, &root_y,
				    		&int_dummy, &int_dummy,
				    		&mask_return);
						if ((last_root_x != root_x)
				    		|| (last_root_y != root_y)) {
							last_root_x = root_x;
							last_root_y = root_y;
							DrawOutline(root_x_return1, root_y_return1,	
					    		root_x, root_y);
						}
					}
				}
							
			}
		}
		else {
			if (f_pressed == True) {
				(void) XQueryPointer(dpy, rootwindow, 
				    &window_dummy, &window_dummy, 
				    &root_x, &root_y,
				    &int_dummy, &int_dummy,
				    &mask_return);
				if ((last_root_x != root_x)
				    || (last_root_y != root_y)) {
					last_root_x = root_x;
					last_root_y = root_y;
					DrawOutline(root_x_return1, root_y_return1,	
					    root_x, root_y);
				}
			}
		}
	}

	/* get rid of last outlines drawn, only if any were drawn */
	if ((root_x_return1 != root_x_return2)
	    || (root_y_return1 != root_y_return2))
		XDrawLines(dpy, rootwindow, invertGC, pt, 5, CoordModeOrigin);
	XUngrabPointer(dpy, CurrentTime);      /* Done with pointer */
	XUngrabKeyboard(dpy, CurrentTime);      /* Done with Keyboard */
	target_win = rootwindow;
	Window_Dump(target_win,
	    image, 
	    header_p,   header_size_p, 
	    win_name_p, win_name_size_p,
	    colors_p,   ncolors_p, 
	    buffer_size_p,
	    root_x_return1, root_y_return1,
	    root_x_return2, root_y_return2);

	if (f_bypass == False)
		f_contentssaved = False;

}



/*
 * Window_Dump: dump a window to a file which must already be open for
 *              writting.
 */

/*
#ifndef MEMUTIL
char *calloc();
#endif
*/
#include <stdlib.h>

Window_Dump(  	window,     image,
header_p,   header_size_p,
win_name_p, win_name_size_p,
colors_p,   ncolors_p,
buffer_s_p,
x1, y1, x2, y2)

Window 		   window;
XImage          ** image;
XWDFileHeader   *  header_p;
int             *  header_size_p;
char            ** win_name_p;
int             *  win_name_size_p;
XColor          ** colors_p;
int             *  ncolors_p;
unsigned        *  buffer_s_p;
int		   x1, y1, x2, y2;

{
	int dump_x, dump_width, xtemp2;
	int dump_y, dump_height, ytemp2;

	if (strcmp(pixmapformat, PROP_XYFORMAT) == 0)
		format = XYPixmap;
	else
		format = ZPixmap;

	if (*image != NULL){

		/*
     		 * free the color buffer.
     		 */

		if(*ncolors_p > 0){
			Free1(*colors_p,"dump : *colors_p");
			*colors_p = (XColor *)calloc(*ncolors_p,sizeof(XColor));
		}

		/*
     		 * Free window name string.
     		 */

		Free1(*win_name_p,"dump : win_name_p");

		/*
     		 * Free image
     		 */

		if(!xlocal)
			XDestroyImage(*image);

	}

	_OlBeepDisplay(toplevel, 1);

	/*
     	 * Get the parameters of the window being dumped.
      	 */
	if(!XGetWindowAttributes(dpy, window, &win_info))
	  OlVaDisplayErrorMsg(XtDisplay(toplevel),
			      OleNerrorMsg,
			      OleTnoWinAttrs,
			      OleCOlClientOlpsMsgs,
			      OleMerrorMsg_noWinAttrs);

	XFetchName(dpy, window, win_name_p);
	if (!(*win_name_p) || !((*win_name_p)[0])){
		*win_name_p = (char *) malloc (sizeof(char) * strlen("xwdump"));
		strcpy(*win_name_p,"xwdump");
	}

	/* sizeof(char) is included for the null string terminator. */

	*win_name_size_p = strlen(*win_name_p) + sizeof(char);

	/*
     	 * Snarf the pixmap with XGetImage.
     	 */

	if (f_area == True) {
		if (x1 < x2) {
			dump_x = x1;
			dump_width = x2 - x1;
		}
		else {
			dump_x = x2;
			dump_width = x1 - x2;
		}
		if (y1 < y2) {
			dump_y = y1;
			dump_height = y2 - y1;
		}
		else {
			dump_y = y2;
			dump_height = y1 - y2;
		}
		if (dump_width <= 0)
			dump_width = 1;
		if (dump_height <= 0)
			dump_height= 1;
	}
	else {
		if (win_info.x < 0){
			dump_x = - win_info.x;
			xtemp2 = win_info.width - dump_x;
			if (xtemp2 <= rootwidth)
				dump_width = xtemp2;
			else
				dump_width = rootwidth;
			dump_x = 0; /**/
		} else {
			xtemp2 = win_info.x + win_info.width;
			if (xtemp2 <= rootwidth)
				dump_width = win_info.width;
			else
				dump_width = rootwidth - win_info.x;
			dump_x = win_info.x;/*0;*/
		}
		if (win_info.y < 0){
			dump_y = - win_info.y;
			ytemp2 = win_info.height - dump_y;
			if (ytemp2 <= rootheight)
				dump_height = ytemp2;
			else
				dump_height = rootheight;
			dump_y = 0; /**/
		} else {
			ytemp2 = win_info.y + win_info.height;
			if (ytemp2 <= rootheight)
				dump_height = win_info.height;
			else
				dump_height = rootheight - win_info.y;
			dump_y = win_info.y;/*0;*/
		}
	}

#ifdef MITSHM
	if (xlocal)	{

	    (*image)->width = dump_width;
	    (*image)->height = dump_height;
	    (*image)->format = format;
	    (*image)->depth = win_info.depth;

   	    if ((*image)->format == ZPixmap)
	       (*image)->bits_per_pixel=
			 _XGetBitsPerPixel(dpy,(int)(*image)->depth);
 	    else
	       (*image)->bits_per_pixel = 1;
            (*image)->bytes_per_line=
			ROUNDUP(((*image)->bits_per_pixel*(*image)->width),
                                   (*image)->bitmap_pad) >> 3;

	    XShmGetImage(dpy, rootwindow, *image, dump_x, dump_y, ~0);
	}
	else	{
#endif /* MITSHM */
	    *image = XGetImage ( dpy, rootwindow, dump_x, dump_y, dump_width,
	    			dump_height, ~0, format);
#ifdef MITSHM	    
	}
#endif	

	*buffer_s_p = Image_Size(*image);
	*ncolors_p = Get_XColors(&win_info, colors_p);

	_OlBeepDisplay(toplevel, 2);
	XFlush(dpy);

	/*
     	 * Calculate header size.
     	 */
	*header_size_p = sizeof(header) + *win_name_size_p;

	/*
     	 * Write out header information.
     	*/
	header_p->header_size = (xwdval) *header_size_p;
	header_p->file_version = (xwdval) XWD_FILE_VERSION;
	header_p->pixmap_format = (xwdval) format;
	header_p->pixmap_depth = (xwdval) (*image)->depth;
	header_p->pixmap_width = (xwdval) (*image)->width;
	header_p->pixmap_height = (xwdval) (*image)->height;
        header_p->xoffset = (xwdval) (*image)->xoffset;
	header_p->byte_order = (xwdval) (*image)->byte_order;
	header_p->bitmap_unit = (xwdval) (*image)->bitmap_unit;
	header_p->bitmap_bit_order = (xwdval) (*image)->bitmap_bit_order;
	header_p->bitmap_pad = (xwdval) (*image)->bitmap_pad;
	header_p->bits_per_pixel = (xwdval) (*image)->bits_per_pixel;
	header_p->bytes_per_line = (xwdval) (*image)->bytes_per_line;
	header_p->visual_class = (xwdval) win_info.visual->class;
	header_p->red_mask = (xwdval) win_info.visual->red_mask;
	header_p->green_mask = (xwdval) win_info.visual->green_mask;
	header_p->blue_mask = (xwdval) win_info.visual->blue_mask;
	header_p->bits_per_rgb = (xwdval) win_info.visual->bits_per_rgb;
	header_p->colormap_entries = (xwdval) win_info.visual->map_entries;
	header_p->ncolors = *ncolors_p;
	header_p->window_width = (xwdval) win_info.width;
	header_p->window_height = (xwdval) win_info.height;
	/* following useful only for xwud application, which does XCreateWindow
	based on offset of root window. Olprintscreen does not need this
	since it uses XCreatePixmap instead, which does not use offsets */
	header_p->window_x = dump_x;
	header_p->window_y = dump_y;

	header_p->window_bdrwidth = (xwdval) win_info.border_width;
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
	  OlVaDisplayErrorMsg(XtDisplay(toplevel),
			      OleNerrorMsg,
			      OleTnoMemory,
			      OleCOlClientOlpsMsgs,
			      OleMerrorMsg_noMemory);

	for (i=0; i<ncolors; i++)
		(*colors)[i].pixel = i;

	XQueryColors(dpy, win_info->colormap, *colors, ncolors);

	return(ncolors);
}


/* Code Below is taken from olwm (window manager: HandleMoveCmd) */
HandleCursor(request, horiz_ret, vert_ret)
OlVirtualName request;
int * horiz_ret;
int * vert_ret;
{
#define MOVEINC         5       /* in pixels    */

        typedef struct {
                OlVirtualName   name;
                int             sign;
                Boolean         use_horiz;
                Boolean         get_count;
        } MoveCmd;
        static OLconst MoveCmd  moveCmds[] = {
                { OL_MOVERIGHT,  1,     True,   False   },
                { OL_MOVELEFT,  -1,     True,   False   },
                { OL_MOVEDOWN,   1,     False,  False   },
                { OL_MOVEUP,    -1,     False,  False   },
                { OL_MULTIRIGHT, 1,     True,   True    },
                { OL_MULTILEFT, -1,     True,   True    },
                { OL_MULTIDOWN,  1,     False,  True    },
                { OL_MULTIUP,   -1,     False,  True    }
        };
        OLconst MoveCmd *       cmds = moveCmds;
        Cardinal                i;

        for (i=0; i < XtNumber(moveCmds); ++cmds, ++i)
        {
                if (request == cmds->name)

                {
                        int *   to_change = (cmds->use_horiz == True ?
                                                horiz_ret : vert_ret);

                        *to_change += MOVEINC * cmds->sign *
                                        (cmds->get_count == True ?
                                        2 : 1);
                        break;
                }
        }
} /* END OF HandleMoveCmd() */

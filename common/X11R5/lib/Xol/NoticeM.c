/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef	NOIDENT
#ident	"@(#)notice:NoticeM.c	1.1"
#endif

/*******************************file*header*******************************
    Description: NoticeM.c - Motif specific Notice Widget code
*/

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/ControlAre.h>
#include <Xol/Caption.h>
#include <Xol/EventObjP.h>	/* for gadgets */
#include <Xol/NoticeP.h>
#include <Xol/StaticText.h>
#include <Xol/VendorI.h>
#include <RectObj.h>
#include <Xol/RubberTile.h>

#include <Xol/Pixmap.h>


/**************************forward*declarations***************************

    Forward function definitions listed by category:
		1. Private functions
		2. Class   functions
		3. Action  functions
		4. Public  functions
 */
						/* private procedures */

extern void _MolNoticeCheckSetValues OL_ARGS((NoticeShellWidget,
					NoticeShellWidget));
extern void _MolNoticeRemoveEventHandlers OL_ARGS((Widget));
extern void _MolNoticeSetupFrame OL_ARGS((NoticeShellWidget, ArgList,
						 Cardinal *));
static void SizeMessageGlyph OL_ARGS((NoticeShellWidget, char **,
					unsigned int * ,unsigned int *));

						/* class procedures */
						/* action procedures */
						/* public procedures */

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

#define MAX_PICTURE_WIDTH	22
#define MAX_PICTURE_HEIGHT	24

#define NPART(w)		( &((NoticeShellWidget)(w))->notice_shell )

/* XtNnoticeType bitmaps */

static OLconst unsigned char NoticeTypeErrorPic[] = {
   0x00, 0x00, 0x00, 0xc0, 0x0f, 0x00, 0xf0, 0x3a, 0x00, 0x58, 0x55, 0x00,
   0x2c, 0xa0, 0x00, 0x56, 0x40, 0x01, 0xaa, 0x80, 0x02, 0x46, 0x81, 0x01,
   0x8a, 0x82, 0x02, 0x06, 0x85, 0x01, 0x0a, 0x8a, 0x02, 0x06, 0x94, 0x01,
   0x0a, 0xe8, 0x02, 0x14, 0x50, 0x01, 0x28, 0xb0, 0x00, 0xd0, 0x5f, 0x00,
   0xa0, 0x2a, 0x00, 0x40, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static OLconst unsigned char NoticeTypeInfoPic[] = {
   0x00, 0x00, 0x78, 0x00, 0x54, 0x00, 0x2c, 0x00, 0x54, 0x00, 0x28, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x2a, 0x00, 0x5c, 0x00, 0x28, 0x00,
   0x58, 0x00, 0x28, 0x00, 0x58, 0x00, 0x28, 0x00, 0x58, 0x00, 0x28, 0x00,
   0x58, 0x00, 0xae, 0x01, 0x56, 0x01, 0xaa, 0x00, 0x00, 0x00, 0x00, 0x00
};

static OLconst  unsigned char NoticeTypeQuestionPic[] = {
   0xf0, 0x3f, 0x00, 0x58, 0x55, 0x00, 0xac, 0xaa, 0x00, 0xd6, 0x5f, 0x01,
   0xea, 0xbf, 0x02, 0xf6, 0x7f, 0x01, 0xea, 0xba, 0x02, 0xf6, 0x7d, 0x05,
   0xea, 0xba, 0x0a, 0x56, 0x7d, 0x15, 0xaa, 0xbe, 0x1e, 0x56, 0x5f, 0x01,
   0xac, 0xaf, 0x02, 0x58, 0x57, 0x01, 0xb0, 0xaf, 0x00, 0x60, 0x55, 0x01,
   0xa0, 0xaa, 0x00, 0x60, 0x17, 0x00, 0xa0, 0x2f, 0x00, 0x60, 0x17, 0x00,
   0xb0, 0x2a, 0x00, 0x50, 0x55, 0x00
};

static OLconst unsigned char NoticeTypeWarningPic[] = {
   0x00, 0x00, 0x18, 0x00, 0x2c, 0x00, 0x56, 0x00, 0x2a, 0x00, 0x56, 0x00,
   0x2a, 0x00, 0x56, 0x00, 0x2c, 0x00, 0x14, 0x00, 0x2c, 0x00, 0x14, 0x00,
   0x2c, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x14, 0x00,
   0x2c, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00
};

static OLconst unsigned char NoticeTypeWorkingPic[] = {
   0x00, 0x00, 0x00, 0xfe, 0xff, 0x0f, 0xaa, 0xaa, 0x0a, 0x44, 0x55, 0x06,
   0xcc, 0x2a, 0x02, 0x44, 0x55, 0x06, 0xcc, 0x2a, 0x02, 0x84, 0x15, 0x06,
   0x8c, 0x2a, 0x02, 0x04, 0x15, 0x06, 0x0c, 0x0a, 0x02, 0x04, 0x06, 0x06,
   0x0c, 0x0b, 0x02, 0x84, 0x15, 0x06, 0xcc, 0x2a, 0x02, 0x44, 0x55, 0x06,
   0xcc, 0x2a, 0x02, 0x44, 0x55, 0x06, 0xcc, 0x2a, 0x02, 0x44, 0x55, 0x06,
   0xfe, 0xff, 0x0f, 0x56, 0x55, 0x05, 0x00, 0x00, 0x00
};


/******************************function*header****************************
 * SetupNoticeFrame() - GUI specific.
 */

/* ARGSUSED */
extern void
_MolNoticeSetupFrame(new, args , num_args)
    NoticeShellWidget	new;
    ArgList		args;
    Cardinal *		num_args;
{
    NoticeShellPart *	nPart = &(new->notice_shell);
    Cardinal		m;
    MaskArg		mArgs[20];
    ArgList		mergedArgs;
    Cardinal		mergedCnt;
    Widget		pane;
    Widget		pane_child;
    Widget		new_composite;
    int			control_area_height;
			
   Widget		rectobj_top_margin, 
			rectobj_left_margin,
			rectobj_right_margin;

/*
    static Arg		pane_args[] = {
				{ XtNcenter, (XtArgVal) True },
				{ XtNlayoutType, (XtArgVal) OL_FIXEDCOLS },
				{ XtNsameSize, (XtArgVal) OL_NONE },
				{ XtNvSpace, (XtArgVal) 0 },
				};
*/


#define NOTICE_WIDTH		200
#define PICTURE_OFFSET		8
#define PIC_MSG_SEPARATION	15
#define CONTROL_AREA_HEIGHT	100
#define TOP_MARGIN		5



	new->notice_shell.warp_pointer = FALSE;
	/* create the sole child widget of this notice/shell widget */
	pane = XtVaCreateManagedWidget("pane", rubberTileWidgetClass,
				(Widget) new,
				XtNorientation, OL_VERTICAL, 
				XtNwidth, 1,
				XtNheight, 1,
				(char *)NULL);

	{
		unsigned int picture_width, picture_height;
		char *picture_data;
		Pixmap	fancy_picture = (Pixmap)NULL;

	rectobj_top_margin = XtVaCreateManagedWidget("top_mrgn", 
				rectObjClass, pane,
				XtNweight, 1,
				XtNwidth, 1,
				XtNheight, TOP_MARGIN,
				(char *)0);
	pane_child = XtVaCreateManagedWidget("panechild",
				rubberTileWidgetClass,
				(Widget) pane,
				XtNorientation, OL_HORIZONTAL, 
				XtNwidth, 1,
				XtNheight, 1,
				XtNweight, 1,
				XtNborderWidth, 0,
				(char *)NULL);
	rectobj_left_margin = XtVaCreateManagedWidget("top_mrgn", 
				rectObjClass, pane_child,
				XtNweight, 1,
				XtNwidth, PICTURE_OFFSET,
				XtNheight, 1,
				XtNborderWidth, 0,
				(char *)0);
	new_composite = XtVaCreateManagedWidget("caption",
			captionWidgetClass, pane_child,
			XtNweight, 10,
			XtNposition, OL_RIGHT,
			XtNalignment, OL_CENTER,
			XtNheight, MAX_PICTURE_HEIGHT,
			XtNborderWidth, 0,
			/* PIC_MSG_SEPARATION pixels between the glyph and
			 * the label.
			 */
			XtNspace, PIC_MSG_SEPARATION,
			XtNlabel, "Message",
			(char *)0);

	/* Motif mode: text is really a caption */
	nPart->text = new_composite;
	SizeMessageGlyph(new, &picture_data, &picture_width, &picture_height);

	/* first child of "rubbertwo" rubberTile */
	new->notice_shell.glyph = XtVaCreateManagedWidget("picture", 
		pixmapGadgetClass, new_composite,
		(char *)0);
	rectobj_right_margin = XtVaCreateManagedWidget("top_mrgn", 
				rectObjClass, pane_child,
				XtNweight, 1,
				XtNwidth, PICTURE_OFFSET,
				XtNheight, 1,
				(char *)0);
        fancy_picture = XCreatePixmapFromBitmapData(XtDisplay(new),
		RootWindowOfScreen(XtScreen(new)), picture_data, picture_width,
		picture_height, BlackPixelOfScreen(XtScreen(new)),
		new->core.background_pixel,
		DefaultDepthOfScreen(XtScreen(new)) );
	XtVaSetValues(new->notice_shell.glyph, XtNpixmap, fancy_picture,
							 (char *)0);
	}
    /*
     * create control area: will contain application's controls
     */


	control_area_height = OlScreenPointToPixel(OL_VERTICAL,
				CONTROL_AREA_HEIGHT, XtScreen(new));	
	m = 0;
	_OlSetMaskArg(mArgs[m], XtNhPad, 0, OL_DEFAULT_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNhSpace, 0, OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNsameSize, 0, OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNvPad, 0, OL_DEFAULT_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNvSpace, 0, OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNheight, control_area_height,
						 OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNlayoutType, OL_FIXEDCOLS,
						 OL_DEFAULT_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNwidth, NOTICE_WIDTH,
						 OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNheight, 1,
						 OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNcenter, TRUE, OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNweight, 1, OL_SOURCE_PAIR); m++;
    
	_OlComposeArgList(args, *num_args, mArgs, m, &mergedArgs, &mergedCnt);
	
	nPart->control = XtCreateManagedWidget("controlarea",
					   controlAreaWidgetClass,
					   pane, mergedArgs, mergedCnt);
    XtFree(mergedArgs);
}

/******************************function*header****************************
    SizeMessageGlyph.
*/

#if OlNeedFunctionPrototypes
static void
SizeMessageGlyph(NoticeShellWidget w, char **data,
		unsigned int *width, unsigned int *height)
#else
static void
SizeMessageGlyph(w, data, width, height)
NoticeShellWidget w;
char **data;
unsigned int *width;
unsigned int *height;
#endif
{
	switch(w->notice_shell.noticeType) {
		case OL_INFORMATION:
			*data = (char *)NoticeTypeInfoPic;
			*width = (unsigned)11;
			*height = (unsigned)24;
			break;
		case OL_QUESTION:
			*data = (char *)NoticeTypeQuestionPic;
			*width = (unsigned)22;
			*height = (unsigned)22;
			break;
		case OL_WARNING:
			*data = (char *)NoticeTypeWarningPic;
			*width = (unsigned)9;
			*height = (unsigned)22;
			break;
		case OL_WORKING:
			*data = (char *)NoticeTypeWorkingPic;
			*width = (unsigned)21;
			*height = (unsigned)23;
			break;
		default:
			w->notice_shell.noticeType = OL_ERROR;
			/* fall through */
		case OL_ERROR:
			*data = (char *)NoticeTypeErrorPic;
			*width = (unsigned)20;
			*height = (unsigned)20;
			break;
	} /* switch */
}

#if OlNeedFunctionPrototypes
extern void
_MolNoticeCheckSetValues(NoticeShellWidget current, NoticeShellWidget new)
#else
extern void
_MolNoticeCheckSetValues(current,new)
NoticeShellWidget current, new;
#endif
{
	new->notice_shell.warp_pointer = FALSE;
	if ((NPART(new)->noticeType!= NPART(current)->noticeType)) {
		char *picture_data;
		unsigned int picture_width, picture_height;

		SizeMessageGlyph(new, &picture_data, &picture_width,
						&picture_height);

		/* Extra check to see if the default kicked in */
		if ((NPART(new)->noticeType != NPART(current)->noticeType)) {
			PixmapGadget glyph = (PixmapGadget)NPART(new)->glyph;
			Pixel fg, bg;
			Pixmap old_pixmap = (Pixmap)NULL;;

			XtVaGetValues((Widget)glyph,XtNpixmap, &old_pixmap,
				(char *)0);
			if (old_pixmap)
				XFreePixmap(XtDisplay(new), old_pixmap);
			XtVaSetValues((Widget)glyph, XtNpixmap,
		   		XCreatePixmapFromBitmapData(XtDisplay(new),
					RootWindowOfScreen(XtScreen(new)),
					picture_data, picture_width,
					picture_height, fg, bg,
					DefaultDepthOfScreen(XtScreen(new)) ),
					(char *)0 );
		}
	} /* if new noticeType != current noticeType */
}

extern void
_MolNoticeRemoveEventHandlers(w)
Widget w;
{ /* Nothing to remove */
}

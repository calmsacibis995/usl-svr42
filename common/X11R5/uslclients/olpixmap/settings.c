/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olpixmap:settings.c	1.25"
#endif

#include "pixmap.h"
#include "error.h"
#include <Xol/Error.h>  
#include <X11/Xos.h>
#include <PopupWindo.h>
#include <FButtons.h>
#include <StaticText.h>
#include <Caption.h>
#include <ControlAre.h>
#include <Slider.h>
#include <TextField.h>
#include <OlCursors.h>

#define GET_MNEM(a,b) \
		(XtArgVal) *(OlGetMessage(DISPLAY, \
				NULL, 0, \
				OleNmnemonic, \
				a, \
				OleCOlClientOlpixmapMsgs, \
				b, \
				(XrmDatabase)NULL))


typedef struct {
	XtArgVal	label;
	XtArgVal	is_set;
} FlatExclusives;

Widget			PropertiesPopup;


static int		LocalFunction;
static Bool		LocalDrawFilled;
static int		LocalLineWidth;
static int		LocalLineStyle;
static Bool		LocalShowGrid;
static Dimension	LocalPixmapWidth;
static Dimension	LocalPixmapHeight;

static String		exc_fields[] = { XtNlabel, XtNset };

static Bool		Popdown;
static Widget		ObjectFillExclusives;
static Widget		Slider;
static Widget		LineWidthText;
static Widget		LineStyleExclusives;
static Widget		GridExclusives;
static Widget		PixmapWidthField;
static Widget		PixmapHeightField;

static void	SetOperation();
static void	SetObjectFill();
static void	SetLineWidth();
static void	SetLineStyle();
static void	SetGrid();
static void	SetWidthAndHeight();

static void	ObjectFillCallback();
static void	SliderMovedCallback();
static void	LineStyleCallback();
static void	GridCallback();

static void	PropApplyCallback();
static void	PropResetCallback();
static void	PropVerifyCallback();




static char *Labels[13];

Cardinal n = 0;


#define GETMESS(a,b)	 OlGetMessage(dsp, NULL, 0, \
		     OleNfixedString, \
		     a,  \
		     OleCOlClientOlpixmapMsgs, \
		     b, \
		     (XrmDatabase)NULL)



SetPropLabels(dsp)
  Display *dsp;
{
Labels[n++] =
	GETMESS(OleTsprops,OleMfixedString_sprops);
Labels[n++] =
	GETMESS(OleTfilled,OleMfixedString_filled);
Labels[n++] =
	GETMESS(OleThollow,OleMfixedString_hollow);
Labels[n++] =
	GETMESS(OleTinteriors,OleMfixedString_interiors);
Labels[n++] =
	GETMESS(OleTlinewidth,OleMfixedString_linewidth);
Labels[n++] =
	GETMESS(OleTsolid,OleMfixedString_solid);
Labels[n++] =
	GETMESS(OleTdashed,OleMfixedString_dashed);
Labels[n++] =
	GETMESS(OleTlinestyle,OleMfixedString_linestyle);
Labels[n++] =
	GETMESS(OleTon,OleMfixedString_on);
Labels[n++] =
	GETMESS(OleToff,OleMfixedString_off);
Labels[n++] =
	GETMESS(OleTgrid,OleMfixedString_grid);
Labels[n++] =
	GETMESS(OleTpixwidth,OleMfixedString_pixwidth);
Labels[n++] =
	GETMESS(OleTpixheight,OleMfixedString_pixheight);

  n = 0;
}

void
SetProperties(parent, button)
Widget parent;
Widget button;
{
	Widget			control_area;
	static XtCallbackRec	apply_callback[] = {
					{PropApplyCallback, (XtPointer)0},
					{(XtCallbackProc)0, (XtPointer)0},
				};
	static XtCallbackRec	reset_callback[] = {
					{PropResetCallback, (XtPointer)0},
					{(XtCallbackProc)0, (XtPointer)0},
				};
	static XtCallbackRec	verify_callback[] = {
					{PropVerifyCallback, (XtPointer)0},
					{(XtCallbackProc)0, (XtPointer)0},
				};
	char			buf[BUFSIZ];

	SetPropLabels(XtDisplay(parent));

	INIT_ARGS();
	SET_ARGS(XtNapply, apply_callback);
	SET_ARGS(XtNreset, reset_callback);
	SET_ARGS(XtNverify, verify_callback);
	sprintf(buf, Labels[n++], ApplicationName);
	SET_ARGS(XtNtitle, buf);
	PropertiesPopup = CREATE_POPUP("PropertiesPopup",
					popupWindowShellWidgetClass, parent);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNupperControlArea, &control_area);
	GET_VALUES(PropertiesPopup);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNlayoutType, OL_FIXEDCOLS);
	SET_ARGS(XtNmeasure, 1);
	SET_ARGS(XtNborderWidth, 1);
	SET_ARGS(XtNtraversalManager, True);
	SET_VALUES(control_area);
	END_ARGS();

	SetOperation(control_area);
	SetObjectFill(control_area);
	SetLineWidth(control_area);
	SetLineStyle(control_area);
	SetGrid(control_area);
	SetWidthAndHeight(control_area);
}


static void
SetOperation(parent)
Widget parent;
{
}


static void
SetObjectFill(parent)
Widget parent;
{
	Widget			caption;
	static FlatExclusives	exc_items[] = {
		{ (XtArgVal)"Filled" },
		{ (XtArgVal)"Hollow" }
	};
	XtArgVal		caption_mnem;
	
	caption_mnem = GET_MNEM(OleTinteriors, OleMmnemonic_interiors);

 	exc_items[0].label = (XtArgVal)Labels[n++];
	exc_items[1].label = (XtArgVal)Labels[n++];

	
	INIT_ARGS();
	SET_ARGS(XtNborderWidth, 0);
	SET_ARGS(XtNlabel, Labels[n++]);
	SET_ARGS(XtNmnemonic, caption_mnem);
	caption = CREATE_MANAGED("caption", captionWidgetClass, parent);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNlayoutType, OL_FIXEDROWS);
	SET_ARGS(XtNbuttonType, OL_RECT_BTN);
	SET_ARGS(XtNexclusives, True);
	SET_ARGS(XtNmeasure, 1);
	SET_ARGS(XtNlabelJustify, OL_LEFT);
	SET_ARGS(XtNselectProc, ObjectFillCallback);
	SET_ARGS(XtNitems, exc_items);
	SET_ARGS(XtNnumItems, XtNumber(exc_items));
	SET_ARGS(XtNitemFields, exc_fields);
	SET_ARGS(XtNnumItemFields, XtNumber(exc_fields));
	ObjectFillExclusives = CREATE_MANAGED("InteriorsExclusives",
					flatButtonsWidgetClass, caption);
	END_ARGS();
}


static void
SetLineWidth(parent)
Widget parent;
{
	Widget	caption,
		control_area;
	char	buf[BUFSIZ];
	int	i,
		len;
	XtArgVal		caption_mnem;
	
	caption_mnem = GET_MNEM(OleTlinewidth, OleMmnemonic_linewidth);

	INIT_ARGS();
	SET_ARGS(XtNborderWidth, 0);
	SET_ARGS(XtNlabel, Labels[n++]);
	SET_ARGS(XtNmnemonic, caption_mnem);
	caption = CREATE_MANAGED("caption", captionWidgetClass, parent);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNlayoutType, OL_FIXEDROWS);
	SET_ARGS(XtNmeasure, 1);
	SET_ARGS(XtNborderWidth, 0);
	SET_ARGS(XtNshadowThickness, 0);
	control_area = CREATE_MANAGED("control_area",
					controlAreaWidgetClass, caption);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNorientation, OL_HORIZONTAL);
	SET_ARGS(XtNsliderMin, 0);
	SET_ARGS(XtNsliderMax, MAX_LINEWIDTH);
	SET_ARGS(XtNsliderValue, CurrentLineWidth);
	SET_ARGS(XtNgranularity, 1);
	SET_ARGS(XtNwidth, OlMMToPixel(OL_HORIZONTAL, 28));
	SET_ARGS(XtNborderWidth, 0);
	SET_ARGS(XtNshowValue, False);
	Slider = CREATE_MANAGED("LineWidthSlider", sliderWidgetClass,
								control_area);
	END_ARGS();

	sprintf(buf, "%d", MAX_LINEWIDTH);
	len = strlen(buf);
	strcpy(buf, "");
	for (i = 0 ; i < len ; i++)
		strcat(buf, "X");

	INIT_ARGS();
	SET_ARGS(XtNstring, buf);
	SET_ARGS(XtNrecomputeSize, False);
	SET_ARGS(XtNgravity, EastGravity);
	SET_ARGS(XtNborderWidth, 0);
	LineWidthText = CREATE_MANAGED("LineWidthText",
					staticTextWidgetClass, control_area);
	END_ARGS();

	XtAddCallback(Slider, XtNsliderMoved, SliderMovedCallback,
							LineWidthText);
	SliderMovedCallback(Slider, LineWidthText, &CurrentLineWidth);
}


static void
SetLineStyle(parent)
Widget parent;
{
	Widget			caption;
	static FlatExclusives	exc_items[] = {
		{ (XtArgVal)"Solid" },
		{ (XtArgVal)"Dashed" }
	};
	XtArgVal		caption_mnem;
	
	caption_mnem = GET_MNEM(OleTlinestyle, OleMmnemonic_linestyle);

 	exc_items[0].label = (XtArgVal)Labels[n++];
	exc_items[1].label = (XtArgVal)Labels[n++];
	
	INIT_ARGS();
	SET_ARGS(XtNborderWidth, 0);
	SET_ARGS(XtNlabel, Labels[n++]);
	SET_ARGS(XtNmnemonic, caption_mnem);
	caption = CREATE_MANAGED("caption", captionWidgetClass, parent);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNlayoutType, OL_FIXEDROWS);
	SET_ARGS(XtNbuttonType, OL_RECT_BTN);
	SET_ARGS(XtNexclusives, True);
	SET_ARGS(XtNmeasure, 1);
	SET_ARGS(XtNlabelJustify, OL_LEFT);
	SET_ARGS(XtNselectProc, LineStyleCallback);
	SET_ARGS(XtNitems, exc_items);
	SET_ARGS(XtNnumItems, XtNumber(exc_items));
	SET_ARGS(XtNitemFields, exc_fields);
	SET_ARGS(XtNnumItemFields, XtNumber(exc_fields));
	LineStyleExclusives = CREATE_MANAGED("StyleExclusives",
					flatButtonsWidgetClass, caption);
	END_ARGS();
}


static void
SetGrid(parent)
Widget parent;
{
	Widget			caption;
	static FlatExclusives	exc_items[] = {
		{ (XtArgVal)"On" },
		{ (XtArgVal)"Off" }
	};
	XtArgVal		caption_mnem;
	
	caption_mnem = GET_MNEM(OleTgrid, OleMmnemonic_grid);

	exc_items[0].label = (XtArgVal)Labels[n++];
	exc_items[1].label = (XtArgVal)Labels[n++];
	
	INIT_ARGS();
	SET_ARGS(XtNborderWidth, 0);
	SET_ARGS(XtNlabel, Labels[n++]);
	SET_ARGS(XtNmnemonic, caption_mnem);
	caption = CREATE_MANAGED("caption", captionWidgetClass, parent);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNlayoutType, OL_FIXEDROWS);
	SET_ARGS(XtNbuttonType, OL_RECT_BTN);
	SET_ARGS(XtNexclusives, True);
	SET_ARGS(XtNmeasure, 1);
	SET_ARGS(XtNlabelJustify, OL_LEFT);
	SET_ARGS(XtNselectProc, GridCallback);
	SET_ARGS(XtNitems, exc_items);
	SET_ARGS(XtNnumItems, XtNumber(exc_items));
	SET_ARGS(XtNitemFields, exc_fields);
	SET_ARGS(XtNnumItemFields, XtNumber(exc_fields));
	GridExclusives = CREATE_MANAGED("GridExclusives",
					flatButtonsWidgetClass, caption);
	END_ARGS();
}


static void
SetWidthAndHeight(parent)
Widget parent;
{
	Widget	caption;
	XtArgVal		caption_mnem;
	
	caption_mnem = GET_MNEM(OleTpixwidth, OleMmnemonic_pixwidth);

	INIT_ARGS();
	SET_ARGS(XtNborderWidth, 0);
	SET_ARGS(XtNlabel, Labels[n++]);
	SET_ARGS(XtNmnemonic, caption_mnem);
	caption = CREATE_MANAGED("caption", captionWidgetClass, parent);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNmaximumSize, 4);
	PixmapWidthField = CREATE_MANAGED("PixmapWidth",
					textFieldWidgetClass, caption);
	END_ARGS();

	caption_mnem = GET_MNEM(OleTpixheight, OleMmnemonic_pixheight);

	INIT_ARGS();
	SET_ARGS(XtNborderWidth, 0);
	SET_ARGS(XtNlabel, Labels[n++]);
	SET_ARGS(XtNmnemonic, caption_mnem);
	caption = CREATE_MANAGED("caption", captionWidgetClass, parent);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNmaximumSize, 4);
	PixmapHeightField = CREATE_MANAGED("PixmapHeight",
					textFieldWidgetClass, caption);
	END_ARGS();
}


void
SetDimensionFields(width, height)
Dimension width;
Dimension height;
{
	char	buf[BUFSIZ];

	sprintf(buf, "%d", width);
	INIT_ARGS();
	SET_ARGS(XtNstring, buf);
	SET_VALUES(PixmapWidthField);
	END_ARGS();

	sprintf(buf, "%d", height);
	INIT_ARGS();
	SET_ARGS(XtNstring, buf);
	SET_VALUES(PixmapHeightField);
	END_ARGS();
}


void
PropertiesPopupCallback(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	LocalFunction = CurrentFunction;
	LocalDrawFilled = DrawFilled;
	LocalLineWidth = CurrentLineWidth;
	LocalLineStyle = CurrentLineStyle;
	LocalShowGrid = ShowGrid;
	LocalPixmapWidth = PixmapWidth;
	LocalPixmapHeight = PixmapHeight;

	XtPopup(PropertiesPopup, XtGrabNone);
	XDefineCursor(DISPLAY, XtWindow(PropertiesPopup),
		      OlGetStandardCursor(PropertiesPopup));
	XRaiseWindow(DISPLAY, XtWindow(PropertiesPopup));
}


static void
ObjectFillCallback(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
 	OlFlatCallData *	fd = (OlFlatCallData *)call_data;

	LocalDrawFilled = !(fd->item_index);
}


static void
SliderMovedCallback(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	Widget	static_text = (Widget)client_data;
	char	buf[BUFSIZ];

	LocalLineWidth = *(int *)call_data;

	INIT_ARGS();
	sprintf(buf, "%d", LocalLineWidth);
	SET_ARGS(XtNstring, buf);
	SET_VALUES(static_text);
	END_ARGS();
}


static void
LineStyleCallback(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
 	OlFlatCallData *	fd = (OlFlatCallData *)call_data;

	LocalLineStyle = (fd->item_index) ? LineOnOffDash : LineSolid;
}


static void
GridCallback(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
 	OlFlatCallData *	fd = (OlFlatCallData *)call_data;

	LocalShowGrid = !(fd->item_index);
}


static void
PropApplyCallback(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	char *	Msg = NULL;
	char *	string;

	INIT_ARGS();
	SET_ARGS(XtNstring, &string);
	GET_VALUES(PixmapWidthField);
	END_ARGS();
	LocalPixmapWidth = (Dimension) ConvertToPosInt(string);
	XtFree(string);

	INIT_ARGS();
	SET_ARGS(XtNstring, &string);
	GET_VALUES(PixmapHeightField);
	END_ARGS();
	LocalPixmapHeight = (Dimension) ConvertToPosInt(string);
	XtFree(string);

	if (LocalPixmapWidth <= 0 || LocalPixmapHeight <= 0)
	  Msg = OlGetMessage(XtDisplay(wid), NULL, 0,
			     OleNfooterMsg,
			     OleTbadHW,
			     OleCOlClientOlpixmapMsgs,
			     OleMfooterMsg_badHW,
			     (XrmDatabase)NULL);

	if (Msg) {
		Popdown = False;
	} else {
		CurrentFunction = LocalFunction;
		CurrentLineWidth = LocalLineWidth;
		CurrentLineStyle = LocalLineStyle;
		DrawFilled = LocalDrawFilled;
		ResetDrawGC();

		if (ShowGrid != LocalShowGrid)
		{
			Dimension	width,
					height;

			INIT_ARGS();
			SET_ARGS(XtNwidth, &width);
			SET_ARGS(XtNheight, &height);
			GET_VALUES(Canvas);
			END_ARGS();

			ShowGrid = True;	/* force the grid to draw */
			DrawGrid(DISPLAY, XtWindow(Canvas), 0, 0,
				(unsigned int)width, (unsigned int)height);
			ShowGrid = LocalShowGrid;
		}

		if (PixmapWidth != LocalPixmapWidth ||
					PixmapHeight != LocalPixmapHeight) {
			Pixmap	new_pixmap;

			new_pixmap = XCreatePixmap(DISPLAY, ROOT,
					LocalPixmapWidth, LocalPixmapHeight,
					PixmapDepth);
			if (LocalPixmapWidth > PixmapWidth ||
					LocalPixmapHeight > PixmapHeight) {
				XSetForeground(DISPLAY, DrawGC,
							CurrentBackground);
				XFillRectangle(DISPLAY, new_pixmap, DrawGC,
					0, 0, LocalPixmapWidth,
							LocalPixmapHeight);
				XSetForeground(DISPLAY, DrawGC,
							CurrentForeground);
			}

			XCopyArea(DISPLAY, RealPixmap, new_pixmap, DrawGC,
				0, 0, MIN(PixmapWidth, LocalPixmapWidth),
				MIN(PixmapHeight, LocalPixmapHeight), 0, 0);

			Changed = True;
			ResetAllVisuals(new_pixmap, LocalPixmapWidth,
							LocalPixmapHeight);
		}
		Popdown = True;
	}
	FooterMessage(Msg, (Msg != NULL));
}


static void
PropResetCallback(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	LocalFunction = CurrentFunction;
	LocalDrawFilled = DrawFilled;
	LocalLineWidth = CurrentLineWidth;
	LocalLineStyle = CurrentLineStyle;
	LocalShowGrid = ShowGrid;
	LocalPixmapWidth = PixmapWidth;
	LocalPixmapHeight = PixmapHeight;

	INIT_ARGS();
	SET_ARGS(XtNset, True);
	FSET_VALUES(ObjectFillExclusives, (LocalDrawFilled) ? 0 : 1);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNsliderValue, LocalLineWidth);
	SET_VALUES(Slider);
	END_ARGS();
	SliderMovedCallback(Slider, LineWidthText, &LocalLineWidth);

	INIT_ARGS();
	SET_ARGS(XtNset, True);
	FSET_VALUES(LineStyleExclusives, (LocalLineStyle == LineSolid) ? 0 : 1);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNset, True);
	FSET_VALUES(GridExclusives, (LocalShowGrid) ? 0 : 1);
	END_ARGS();

	SetDimensionFields(LocalPixmapWidth, LocalPixmapHeight);

	FooterMessage(NULL, False);
	Popdown = False;
}


static void
PropVerifyCallback(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	*((Boolean *) call_data) &= Popdown;
}

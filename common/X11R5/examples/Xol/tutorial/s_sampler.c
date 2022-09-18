/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olexamples:tutorial/s_sampler.c	1.35"
#endif

/************************************************************************/
/*	OPEN LOOK WIDGET SAMPLER : Toolkit prototyping and use of form	*/
/*									*/
/*	Copyright (c) 1989 AT&T						*/
/*	Copyright (c) 1988 Hewlett-Packard Company 			*/
/*	Copyright (c) 1988 Massachusetts Institute of Technology	*/
/************************************************************************/

#include <stdio.h>

/*
 * Headers required for all OPEN LOOK applications.
 */

#include <X11/Intrinsic.h>	
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

/*
 * Headers for internationalization.
 */

#include <stdarg.h>
#include "msgs.h"

/*
 * Headers required for creating widget instances.
 */

#include <Xol/AbbrevMenu.h>
#include <Xol/BulletinBo.h>
#include <Xol/Button.h>
#include <Xol/Caption.h>
#include <Xol/CheckBox.h>
#include <Xol/ControlAre.h>
#include <Xol/Exclusives.h>
#include <Xol/FooterPane.h>
#include <Xol/Form.h>
#include <Xol/Gauge.h>
#include <Xol/MenuButton.h>
#include <Xol/Nonexclusi.h>
#include <Xol/Notice.h>
#include <Xol/OblongButt.h>
#include <Xol/PopupWindo.h>
#include <Xol/RectButton.h>
#include <Xol/Scrollbar.h>
#include <Xol/ScrolledWi.h>
#include <Xol/ScrollingL.h>
#include <X11/Shell.h>
#include <Xol/Slider.h>
#include <Xol/StaticText.h>
#include <Xol/Stub.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>

/*
 * Headers required for creating flat widget instances.
 */

#include <Xol/FExclusive.h>
#include <Xol/FNonexclus.h>
#include <Xol/FCheckBox.h>

/*
 * icon.xpm is the pixmap for the application's icon
 * Xutil.h is needed for XIconSize struct - see SetProgramIcon()
 * Shell.h is needed but included above.
 */

#include "icon.xpm"
#include <X11/Xutil.h>

/*
 * Defines.
 */

#define MAXTEXT 1000 
#define MAXBUF  100 

/*
 * Ol_PointToPixel() scales to the current screen resolution.
 */

#define N1_H_PIXEL Ol_PointToPixel(OL_HORIZONTAL,1) 
#define N1_V_PIXEL Ol_PointToPixel(OL_VERTICAL,1)
#define N10_H_PIXELS Ol_PointToPixel(OL_HORIZONTAL,8) 
#define N10_V_PIXELS Ol_PointToPixel(OL_VERTICAL,10)
#define N50_V_PIXELS Ol_PointToPixel(OL_VERTICAL,50)
#define N100_H_PIXELS Ol_PointToPixel(OL_HORIZONTAL,100)
#define N100_V_PIXELS Ol_PointToPixel(OL_VERTICAL,100)
#define N150_H_PIXELS Ol_PointToPixel(OL_HORIZONTAL,150)
#define N150_V_PIXELS Ol_PointToPixel(OL_VERTICAL,150)
#define N200_H_PIXELS Ol_PointToPixel(OL_HORIZONTAL,200)

/*
 * Global variables.
 */

static Pixel	red_pixel,blue_pixel,purple_pixel,green_pixel, 
		yellow_pixel,orange_pixel,skyblue_pixel;

static int	i;

static Boolean 	rainbow = FALSE;

/*
 * Note: In many cases, a widget is labelled by making it the child of a 
 * caption, which itself is a child of the parent form.
 */

static Widget	toplevel,
		controlarea,cabutton,
		form1,			/* top child of footer */
		form2,			/* contains most widgets */
		form3,			/* contains caption and stub */
		gauge,
		abbmenu_caption,	/* caption for abbreviatedmenubutton */
		bb_caption,		/* caption for bulletinboard */
		ca_caption,		/* caption for controlarea */
		caption,		/* simple caption */
		cb_caption,		/* caption for checkbox */
		f_caption,		/* caption for flats */
		fm_caption,		/* caption for form1 */
		footer_text,		/* statictext widget for messages */
		gd_caption,		/* caption for gadgets */
		g_caption,		/* caption for gauge */
		slist_caption,		/* caption for scrollinglist */
		slider_caption,		/* caption for slider */
		te_caption,		/* caption for textedit */
		sw_caption,		/* caption for scrolledwindow */
		nonexclusives,nebutton2,nebutton3,
		noticeshell,noticebox,
		popupshell0,
		popupshell1,
		popupshell2,
		scrollbar,
		stub,
		footer_text;

/* 
 * Form constraint resources used for most widgets in application.
 */

static Arg genericARGS[] = {

	{ XtNxRefName, NULL },
	{ XtNyRefName, NULL },
	{ XtNxOffset,(XtArgVal) 0 },	/* to be initialized below */
	{ XtNyOffset,(XtArgVal) 0 },	/* to be initialized below */
	{ XtNxAddWidth,(XtArgVal) TRUE },
	{ XtNyAddHeight,(XtArgVal) TRUE },
};

/* 
 * FUNCTIONS
 */

/*
 * Function to vary value displayed in gauge widget.
 */

static void 
ChangeGauge(widget)
	Widget widget;
{
	Arg arg;
	int current_value;

	XtSetArg(arg,XtNsliderValue,&current_value);
	XtGetValues(widget,&arg,1);

	current_value+=10;
	if(current_value>100)
		current_value=0;

/*
 * The following convenience routine is faster than a SetValues().
 */
	OlSetGaugeValue(gauge,current_value);
}

/* 
 * Xt and Xlib examples to give functionality to the stub widget.
 */

static void 
DrawAndPrint(widget,xevent,region)
	Widget widget;
	XEvent *xevent; /* not used */
	Region region;  /* not used */
{
	Display *display; 
	static Window window; 
	static Boolean done1 = FALSE, done2 = FALSE;
	static GC gc[6], font_gc;
	XGCValues values;
	int i, ix5, ix10, coord, axis, finish = 180*64;
	static int hpixel,vpixel;
	XtGCMask mask;
	static OlFontList *fontlist;
	static XFontStruct *xfs;
	static char *widget_label;
	Arg arg;

	display = XtDisplay(widget);
	window = XtWindow(widget);

	if(!done1) {
		done1 = TRUE;

		hpixel = (int) N1_H_PIXEL;
		vpixel = (int) N1_V_PIXEL;
		if(hpixel==0)
			hpixel=1;
		if(vpixel==0)
			vpixel=1;

		widget_label = OlGetMessage(	XtDisplay(widget),
						NULL,
						0,
						OleNstub,
						OleTlabel,
						OleCOlSamplerMsgs,
						OleMstub_label,
						(XrmDatabase) NULL);

		XtSetArg(arg, XtNfontGroup, (XtArgVal) &fontlist);
		XtGetValues(widget, &arg, 1);

		XtSetArg(arg, XtNfont, (XtArgVal) &xfs);
		XtGetValues(widget, &arg, 1);

		if(xfs != (XFontStruct *) NULL && xfs->fid != (Font) 0) {
			values.font = xfs->fid;
			font_gc = XtGetGC(widget, (XtGCMask)GCFont, &values);
		}
	}

	if(rainbow) {
	
		values.line_width = 5*hpixel;

		if(!done2) {
			mask = (XtGCMask) (GCForeground | GCLineWidth);

			values.foreground = red_pixel;
			gc[0]= XtGetGC(widget,mask, &values);
			values.foreground = orange_pixel;
			gc[1]= XtGetGC(widget,mask, &values);
			values.foreground = yellow_pixel;
			gc[2]= XtGetGC(widget,mask, &values);
			values.foreground = green_pixel;
			gc[3]= XtGetGC(widget,mask, &values);
			values.foreground = blue_pixel;
			gc[4]= XtGetGC(widget,mask, &values);
			values.foreground = purple_pixel;
			gc[5]= XtGetGC(widget,mask, &values);
		}

		for(i=0;i<6;i++) {

			ix5 = i*5;
			ix10 = i*10;

			axis = 95 - ix10;
			coord = ix5 + 5;	

			XDrawArc(	display, window, gc[i],
					coord*hpixel,
					coord*vpixel,
					(unsigned int) (axis*hpixel),
					(unsigned int) (axis*vpixel),
					0,finish);
		}
	}
	else
		XClearWindow(display,window);

	/*
	 * If the fontlist is NULL, strings have single byte
	 * characters and use XDrawString(); otherwise strings
	 * have multibyte characters and OlDrawString() must be used.
	 */

	if(fontlist ==  (OlFontList *) NULL 
		&& xfs != (XFontStruct *) NULL
		&& xfs->fid != (Font)0) 

		XDrawString(	display, 
				window, 
				font_gc, 
				5*hpixel, 
				70*vpixel, 
				widget_label,
				strlen(widget_label));

	else if(fontlist !=  NULL
		&& xfs !=  NULL
		&& xfs->fid != (Font) 0)

		OlDrawString(	display,
				window,
				fontlist,
				font_gc,
				5*hpixel, 
				70*vpixel, 
				(unsigned char *) widget_label,
				strlen(widget_label));
				
/*
 * This function merely varies the value displayed in the gauge.
 */

	ChangeGauge(gauge);
}

/* 
 * Function for changing the footer message.
 */

static void 
FooterMessage(	char *msg_name,
		char *msg_type,
		char *default_msg,
		Boolean args_passed, 
		...)
{
	static char msg[BUFSIZ], buf[BUFSIZ], *footer_msg;
	Arg arg;
	int i;
	va_list vargs;

/* 
 * Get the "raw" message.
 */

	footer_msg = OlGetMessage(	XtDisplay(toplevel),
					msg,
					BUFSIZ,
					msg_name,
					msg_type,
					OleCOlSamplerMsgs,
					default_msg,
					(XrmDatabase)NULL);
	
/* 
 * If there is an argument list, make the substitutions in string.
 */

	if(args_passed) {
		va_start(vargs,msg);
		vsprintf(buf,msg,vargs);
		footer_msg = buf;
	}

	XtSetArg(arg,XtNstring,footer_msg);
	XtSetValues(footer_text,&arg,1);
	if(args_passed)
		va_end(vargs);
}

/* 
 * Function for getting color values for the display.
 */

static void 
GetColors()	
{
	XrmValue fromValue, toValue;

	static char *colors[] = {
		"purple","blue","green","yellow","orange","red","skyblue"
	};

	int ncolors = 7;

	for(i=0;i<7;i++) {
		fromValue.size = sizeof(colors[i]);
		fromValue.addr = colors[i];
		XtConvert(toplevel,XtRString,&fromValue,XtRPixel,&toValue);
		switch(i) {
			case 0: 
				purple_pixel = *((Pixel *) toValue.addr);
				break;
			case 1: 
				blue_pixel = *((Pixel *) toValue.addr);
				break;
			case 2: 
				green_pixel = *((Pixel *) toValue.addr);
				break;
			case 3: 
				yellow_pixel = *((Pixel *) toValue.addr);
				break;
			case 4: 
				orange_pixel = *((Pixel *) toValue.addr);
				break;
			case 5: 
				red_pixel = *((Pixel *) toValue.addr);
				break;
			case 6: 
				skyblue_pixel = *((Pixel *) toValue.addr);
				break;
		}
	}
}

/*
 * Function to calculate icon dimensions based upon
 * current window manager's preference(s).
 */

static void
GetIconWindowDimensions(size_list_return, count_return, 
				old_height, old_width, new_height, new_width)
	XIconSize *size_list_return;
	int count_return;
	int old_height, old_width, *new_height, *new_width;
{
	XIconSize *first = size_list_return;

	if((old_height >= first->min_height) 
			&& (old_height <= first->max_height))
		*new_height = old_height;
	else if(old_height < first->min_height)
		*new_height = first->min_height;
	else if(old_height > first->max_height)
		*new_height = first->max_height;
	if((old_width >= first->min_width) 
			&& (old_width <= first->max_width))
		*new_width = old_width;
	else if(old_width < first->min_width)
		*new_width = first->min_width;
	else if(old_width > first->max_width)
		*new_width = first->max_width;
}

/*
 * Function for positioning widgets on form2.
 *
 * Note: XtNx[y]RefName are used instead of XtNx[y]RefWidget so
 * that resources for each widget can be set in an .Xdefaults 
 * file.  In addition, this also allows specifying the placement 
 * of any widget on the form without first having to place the 
 * corresponding reference widget(s) on the form.  (This would 
 * NOT be the case with the XtNx[y]RefWidget resource).
 */

static void 
SetPosition(widget, xwidget, ywidget)
Widget widget;
char *xwidget, *ywidget;
{
	static int nargs;

	if(nargs == 0)
		nargs = XtNumber(genericARGS);

	genericARGS[0].value = (XtArgVal) xwidget; 
	genericARGS[1].value = (XtArgVal) ywidget; 
	XtSetValues(widget,genericARGS,nargs);
}

/* 
 * Function for creating a custom icon for the application.
 */

static void 
SetProgramIcon(toplevel)
	Widget toplevel;
{
	Pixmap icon_pixmap;
	Arg arg[2];
	int i;
	Screen *screen = XtScreen(toplevel);
	Display *display = XtDisplay(toplevel);
	Window root_window = XRootWindowOfScreen(screen);
	int depth = XDefaultDepthOfScreen(screen);
	Colormap colormap = XDefaultColormapOfScreen(screen);
	Window icon_window;
	XSetWindowAttributes xswa;
	Status status;
	int new_icon_height, new_icon_width;
	XIconSize *size_list_return = 0, *first;
	int count_return = 0;

/* 
 * Create the icon with the "icon_" values in icon.xpm,
 * generated by olpixmap. XCreatePixmapData() is found in libolc.a
 */
	
	icon_pixmap = XCreatePixmapFromData( display,
				root_window,
				colormap,
				icon_width,icon_height,
				depth,
				icon_ncolors,
				icon_chars_per_pixel,
				icon_colors, 
				icon_pixels); 

/*
 * Get the window manager's preferred icon window dimensions 
 * and calculate optimal icon window dimensions using this data.
 */

	status = XGetIconSizes(	display,
				root_window,
				&size_list_return,
				&count_return);

	if(status==(Status) 0 || count_return==0) {
		new_icon_height = icon_height;
		new_icon_width = icon_width;
	}
	else
		GetIconWindowDimensions(	size_list_return,
						count_return,
						icon_height,
						icon_width,
						&new_icon_height, 
						&new_icon_width);

/*
 * Create the icon window with the pixmap as the window background.
 */

	xswa.background_pixmap = icon_pixmap;
	xswa.colormap = colormap;
	icon_window = XCreateWindow(XtDisplay(toplevel),
				root_window,
				0,
				0, 
				(unsigned int) new_icon_width,
				(unsigned int) new_icon_height,
				(unsigned int) 0,
				depth,
				InputOutput,
				CopyFromParent,
				(unsigned long) (CWBackPixmap | CWColormap ),
				&xswa);

	i=0;
	XtSetArg(arg[i],XtNiconWindow,(XtArgVal) icon_window); i++;
	XtSetArg(arg[i],XtNiconName,(XtArgVal) "s_sampler"); i++;
	XtSetValues(toplevel,arg,i);

	XtFree((char *) size_list_return);
}

/* 
 * Function for creating a special cursor for the stub widget.
 */

static void 
SetStubCursor(widget)
	Widget widget;
{
	static Cursor cursor;

/*
 * See OlCursor.c for other cursor possibilities.
 */

	cursor = GetOlQuestionCursor(XtScreen(widget));
	XDefineCursor(XtDisplay(widget),XtWindow(widget),cursor);
}

/* 
 *  Event handler example to give functionality to the stub widget.
 */

static void 
StubEventHandler(widget,clientData,event)
	Widget widget;
	XtPointer clientData;
	XEvent *event;
{
	XCrossingEvent *xce;

/*
 * The xce pointer allows referencing to event specifics - see Xlib.h
 */

	if(event->type==EnterNotify || event->type==LeaveNotify)
		xce = (XCrossingEvent *) &(event->xcrossing);
	else 
		return;

	if(event->type ==EnterNotify)
		FooterMessage(	OleNfooterMessage, 
				OleTpointerIn,
				OleMfooterMessage_pointerIn,
				FALSE);
	else
		FooterMessage(	OleNfooterMessage, 
				OleTpointerOut,
				OleMfooterMessage_pointerOut,
				FALSE);
}

/* 
 * CALLBACKS FOR WIDGETS
 */

/* 
 * With this callback, each widget passes its index as 
 * clientData and thus maps to its own footerpanel message. 
 */

static void 
genericCB(widget,clientData,callData)
	Widget widget;
	XtPointer clientData,callData;
{
	int n = (int) clientData;

	switch(n) {

		case 1:
		case 2:
		case 3:
		FooterMessage(	OleNfooterMessage, 
				OleTnonExclusivesCB,
				OleMfooterMessage_nonExclusivesCB,	
				TRUE,
				n);
		break;

		case 4:

		FooterMessage(	OleNfooterMessage, 
				OleTpopupApplyCB,
				OleMfooterMessage_popupApplyCB,
				FALSE);
		break;

		case 5:

		FooterMessage(	OleNfooterMessage, 
				OleTpopupSetDefaultsCB,
				OleMfooterMessage_popupSetDefaultsCB,
				FALSE);
		break;

		case 6:

		FooterMessage(	OleNfooterMessage, 
				OleTpopupResetCB,
				OleMfooterMessage_popupResetCB,
				FALSE);
		break;

		case 7:

		FooterMessage(	OleNfooterMessage, 
				OleTpopupResetFactoryCB,
				OleMfooterMessage_popupResetFactoryCB,
				FALSE);
		break;

		case 8:

		FooterMessage(	OleNfooterMessage, 
				OleToblongGadgetCB,
				OleMfooterMessage_oblongGadgetCB,
				FALSE);
		break;

		case 9:

		FooterMessage(	OleNfooterMessage, 
				OleTflatSelectCB,
				OleMfooterMessage_flatSelectCB,
				FALSE);
		break;
	}
}

static void 
checkboxCB(widget,clientData,callData)
	Widget widget;
	XtPointer clientData, callData;
{
	Arg arg;
	OlDefine position;
/* 
 * Get the current value.
 */
	XtSetArg(arg,XtNposition,&position);
	XtGetValues(widget,&arg,1);

/* 
 * Toggle it.
 */
	if(position == (OlDefine) OL_LEFT)
		XtSetArg(arg,XtNposition,OL_RIGHT);
	else 
		XtSetArg(arg,XtNposition,OL_LEFT);

	XtSetValues(widget,&arg,1);
}

/* 
 * The MenuSelectCB callback is used by the pulldown menu
 * popup menu, and abbreviatedmenubutton widget and gadget. 
 */

static void 
menuSelectCB(widget,clientData,callData)
	Widget widget;
	XtPointer clientData,callData;
{
	int n = (int) clientData;
		
	FooterMessage(	OleNfooterMessage, 
			OleTbuttonSelected,
			OleMfooterMessage_buttonSelected,
			TRUE,
			n);
}

static void 
nonexclusivesCB(widget,clientData,callData)
	Widget widget;
	XtPointer clientData,callData;
{
	Widget parent = (Widget) clientData;
	Arg arg;

/* 
 * If only "More" button, create "Fewer" button.
 */
	if(widget == nebutton2 && nebutton3 == (Widget) 0) {
		XtSetArg(arg, XtNlabel,OlGetMessage(	XtDisplay(toplevel),	
							(char *)NULL,
							0,
							OleNexclusives,
							OleTlabel,
							OleCOlSamplerMsgs,
							OleMexclusives_label,
							(XrmDatabase)NULL));

		nebutton3 = XtCreateManagedWidget
			("Fewer",rectButtonWidgetClass,parent,&arg,1);
		XtAddCallback(nebutton3,XtNselect,
				nonexclusivesCB,(XtPointer) nonexclusives);
		XtAddCallback(nebutton3,XtNselect,
				genericCB,(XtPointer) 3);

	}

/* 
 * If all three buttons, delete "Fewer" button.
 */

	else if(widget == nebutton3 && nebutton3 != (Widget)0) {
		XtDestroyWidget(nebutton3);
		nebutton3 = (Widget) 0;
	}
}

static void 
noticeCB1(widget,callData,clientData) 
	Widget widget;	/* w = emanating button: where notice does popup */
	XtPointer callData, clientData;
{
	Arg arg;

	XtSetArg(arg, XtNemanateWidget, (XtArgVal)widget);
	XtSetValues(noticebox, &arg, 1);
	XtPopup(noticeshell, XtGrabExclusive); 
}

static void 
noticeCB2(widget,clientData,callData)
	Widget widget;
	XtPointer clientData,callData;
{
	FooterMessage(	OleNfooterMessage, 
			OleTgoodBye,
			OleMfooterMessage_goodBye,
			FALSE);
	exit(0);
}

static void 
popupCB(widget,clientData,callData)
	Widget widget;
	XtPointer clientData,callData;
{
	XtPopup(clientData,XtGrabNone);
}

/*
 * The rainbowCB callback is for the RAINBOW button.
 */

static void 
rainbowCB(widget,clientData,callData)
	Widget widget;
	XtPointer clientData,callData;
{
	if(rainbow)
		rainbow=FALSE;
	else 
		rainbow=TRUE;

	DrawAndPrint(stub,(XEvent *)0,(Region)0);
}

/* 
 * This callback is used by all three scrollbars.
 */

static void 
scrollbarCB(widget,clientData,callData)
    Widget widget;
    XtPointer clientData,callData;
{
	OlScrollbarVerify *sbv = (OlScrollbarVerify *) callData;
	int n = (int) clientData;

	sbv->ok = TRUE;

	switch(n) {
		case 0:

		FooterMessage(	OleNfooterMessage, 
				OleTscrollbarMoved,
				OleMfooterMessage_scrollbarMoved,
				TRUE,
				sbv->new_location);
		break;

		case 1:

		FooterMessage(	OleNfooterMessage, 
				OleTscrolledWindowVSB,
				OleMfooterMessage_scrolledWindowVSB,
				FALSE);
		break;

		case 2:

		FooterMessage(	OleNfooterMessage, 
				OleTscrolledWindowHSB,
				OleMfooterMessage_scrolledWindowHSB,
				FALSE);
		break;
	}

/*
 * Update form1's scrollbar page indicator.
 */
	if(widget==scrollbar)
		sbv->new_page = (int) (sbv->new_location/10) + 1;
}

static void 
scrollinglistCB(widget,clientData,callData)
	Widget widget;
	XtPointer clientData,callData;
{
	OlListToken token = (OlListToken) callData;
	OlListItem *selected_listitem;

/* 
 * This macro identifies the item selected.
 */

	selected_listitem = OlListItemPointer(token);
		
	FooterMessage(	OleNfooterMessage, 
			OleTscrollingListMsg,
			OleMfooterMessage_scrollingListMsg,
			TRUE,
			selected_listitem->label,
			selected_listitem->user_data);
}

static void 
sliderCB(widget,clientData,callData)
	Widget widget;
	XtPointer clientData,callData;
{
	OlSliderVerify *sv = (OlSliderVerify *) callData;
	Arg arg;

	arg.value = (XtArgVal) sv->new_location;
	XtSetArg(arg,XtNbackground,arg.value);
	XtSetValues(stub,&arg,1);
}

static void 
textfieldCB(widget,clientData,callData)
	Widget widget;
	XtPointer clientData,callData;
{
	OlTextFieldVerify *tfv = (OlTextFieldVerify *) callData;
	Arg arg;

	FooterMessage(	OleNfooterMessage, 
			OleTtextfieldMsg,
			OleMfooterMessage_textfieldMsg,
			TRUE,
			tfv->string);

	XtSetArg(arg,XtNstring,(XtArgVal) "");
	XtSetValues(widget,&arg,1);
}

/*
 * THE WIDGET TREE: 
 *
 * 				Toplevel 
 *				   |
 * 				Footerpanel
 *				/     \
 * 			     Form     Statictext
 *			      /
 * 			Remaining OPEN LOOK widgets, gadgets, and flats
 */

/*
 * MAIN
 */

int main(argc,argv)
	int argc;
	char *argv[];
{

	Arg arg[10];

/*
 * Initialize the environment.
 */

{

	toplevel = OlInitialize(	"s_sampler",
					"s_sampler", 
					(XrmOptionDescRec *) NULL, 
					(Cardinal) 0, 
					&argc, 
					argv);

	i=0;
	XtSetArg(arg[i],XtNtitle, OlGetMessage(	XtDisplay(toplevel),
						(char *)NULL,
						0,
						OleNtopLevel,
						OleTtitle,
						OleCOlSamplerMsgs,
						OleMtopLevel_title,
						(XrmDatabase)NULL)); i++;
	XtSetValues(toplevel,arg,i);

/*
 * Get colors to use later.
 */
	GetColors();		

/* 
 * Set the pixmap for program icon window 
 */

	SetProgramIcon(toplevel);

/*
 * Set FORM resources for the specific screen.
 */

	genericARGS[2].value = (XtArgVal) N10_H_PIXELS; 
	genericARGS[3].value = (XtArgVal) N10_V_PIXELS; 
}

/* 
 * Make all the widgets and then do placement on the forms last.
 */

/* 
 * FOOTERPANEL: This will be the child of toplevel, with
 * FORM1 as the topchild and STATICTEXT as the footer child.
 */

{
	Widget footerpanel;

	footerpanel = XtCreateManagedWidget("footerpanel",
		footerPanelWidgetClass,toplevel,NULL,0);

/* 
 * FORM: Form1's caption is the top child of the footerpanel.
 */

	i = 0;
	XtSetArg(arg[i],XtNposition,(XtArgVal) OL_TOP); i++;
	XtSetArg(arg[i],XtNalignment,(XtArgVal) OL_CENTER); i++;
	fm_caption = XtCreateManagedWidget("fm_caption",
		captionWidgetClass,footerpanel,arg,i);

	form1 = XtCreateManagedWidget("form1",
			formWidgetClass,fm_caption,NULL,0);

/*
 * Register help for the application, using the file "tlevel.help"
 */

	OlRegisterHelp(	OL_WIDGET_HELP,
			(XtPointer) toplevel,

			(String) OlGetMessage(	XtDisplay(toplevel),
						(char *)NULL,
						0,
						OleNhelp,
 						OleTtitle,
						OleCOlSamplerMsgs,
						OleMhelp_title,
						(XrmDatabase)NULL),
			OL_DISK_SOURCE,
			OlFindHelpFile(toplevel,"tlevel.help"));

/*
 * Make STATICTEXT as the footer child of the footerpanel.
 */

	footer_text = XtCreateManagedWidget("statictext1",
		staticTextWidgetClass,footerpanel,NULL,0);
}

/*
 * The controlarea, the scrollbar, and form2 will be on form1.
 * The remaining widgets will be on form2.  This was necessary
 * for resolution independence.  Otherwise, at times the scrollbar
 * was to the right of the scrolled window and at other times,
 * to the right of the bulletin board containing the flat widgets.
 */

/*
 * CAPTION: to label the controlarea, latter to be created next.
 */

	i = 0;
	XtSetArg(arg[i],XtNxResizable,(XtArgVal) TRUE); i++;
	XtSetArg(arg[i],XtNxAttachRight,(XtArgVal) TRUE); i++;
	ca_caption = XtCreateManagedWidget("ca_caption",
		captionWidgetClass,form1,arg,i);

/*
 * CONTROLAREA: the control area contains a README oblongbutton
 * with a popup statictext message, a popup command window, 
 * a popup property window, and an EXIT button with a notice 
 * widget which pops up when the button is selected.
 */

{
	Widget widget,popupca01;
	static char stext[MAXTEXT];
	FILE *fp = (FILE *) NULL;

	controlarea = XtCreateManagedWidget("controlarea",
		controlAreaWidgetClass,ca_caption,NULL,0);

	widget = XtCreateManagedWidget("readme",
		oblongButtonWidgetClass,controlarea,NULL,0);

	popupshell0 = XtCreatePopupShell("popupshell0",
		popupWindowShellWidgetClass,widget,NULL,0);

	XtAddCallback(widget,XtNselect,popupCB,popupshell0);

	i = 0;
	XtSetArg(arg[i], XtNupperControlArea, &popupca01); i++;
	XtGetValues (popupshell0, arg, i);

/* 
 * STATICTEXT: This is used as a README for the application.
 */
	if((fp=fopen(OlFindHelpFile(toplevel,"stext.text"),"r"))) {
		fread(stext,sizeof(char),MAXTEXT,fp); 
		fclose(fp);
	}
	else 
		strcpy(	stext,
			OlGetMessage(	XtDisplay(toplevel),
					(char *)NULL,
					0,
					OleNstatictext,
 					OleTmessage,
					OleCOlSamplerMsgs,
					OleMstatictext_message,
					(XrmDatabase)NULL));

	i = 0;
	XtSetArg(arg[i], XtNstring, stext); i++;
	XtCreateManagedWidget("statictext2",
		staticTextWidgetClass,popupca01,arg,i);
}

/* 
 * POPUP: Used for Command Window.
 */

{
	Widget widget,popupca11,popupca12,popupfooter1,popupbutton1;

	popupbutton1 = XtCreateManagedWidget("cw_button",
		oblongButtonWidgetClass,controlarea,NULL,0);

/* 
 * Make the popup shell first.
 */

	popupshell1 = XtCreatePopupShell("popupshell1", 
		popupWindowShellWidgetClass,popupbutton1,NULL,0);

/* 
 * Add callback to popup button now that we have popupshell widget ID. 
 */

	XtAddCallback(popupbutton1, XtNselect, popupCB, popupshell1);

/*
 * The popup window automatically makes three children: upper and
 * lower control areas and footer: get widget IDs to populate them.
 */

	i = 0;
	XtSetArg(arg[i], XtNupperControlArea, &popupca11); i++;
	XtSetArg(arg[i], XtNlowerControlArea, &popupca12); i++;
	XtSetArg(arg[i], XtNfooterPanel, &popupfooter1); i++;
	XtGetValues (popupshell1, arg, i);

/* 
 * Populate popup upper control area.
 */

/* 
 * Make a caption as a label/prompt for the TEXTFIELD.
 */

	widget = XtCreateManagedWidget("tf_caption",
		captionWidgetClass,popupca11,NULL,0);

	widget = XtCreateManagedWidget("textfield",
		textFieldWidgetClass,widget,NULL,0);

/* 
 * Callback to "read" user input when <return> typed. 
 */

	XtAddCallback(widget,XtNverification,textfieldCB,NULL);

/* 
 * Populate popup lower controlarea with buttons: one must be a default.
 */

	XtCreateManagedWidget("option1",
		oblongButtonWidgetClass,popupca12,NULL,0);

	XtCreateManagedWidget("option2",
		oblongButtonWidgetClass,popupca12,NULL,0);


/* 
 * Make a statictext widget for the popup footer.
 */

	i = 0;
	XtSetArg(arg[i], XtNborderWidth, 0); i++;
	XtCreateManagedWidget("footer1",
		staticTextWidgetClass,popupfooter1,arg,i);
}

/*
 * End of POPUP: Command Window.
 */

/*
 * POPUP: Used for Property Window.
 */

{
	Widget exclusives,widget,popupca21,popupfooter2,popupbutton2;

/*
 * XtCallbackRec used for widget SetValues().
 */

	static XtCallbackRec popup_applyCBR[] = {
		{ genericCB, (XtPointer) 4 },
		{ (XtCallbackProc) NULL, (XtPointer) NULL },
	};

	static XtCallbackRec popup_setdefaultsCBR[] = {
		{ genericCB, (XtPointer) 5 },
		{ (XtCallbackProc) NULL, (XtPointer) NULL },
	};

	static XtCallbackRec popup_resetCBR[] = {
		{ genericCB, (XtPointer) 6 },
		{ (XtCallbackProc) NULL, (XtPointer) NULL },
	};

	static XtCallbackRec popup_resetfactoryCBR[] = {
		{ genericCB, (XtPointer) 7 },
		{ (XtCallbackProc) NULL, (XtPointer) NULL },
	};

	popupbutton2 = XtCreateManagedWidget("pw_button",
		oblongButtonWidgetClass,controlarea,NULL,0);

/* 
 * Make the popup shell first:
 * NOTE: callbacks must be set for creation of the automatic buttons.
 * In this example, mapping is to one function but does not have to be. 
 */

	i = 0;
	XtSetArg(arg[i],XtNreset,(XtArgVal) popup_resetCBR); i++;
	XtSetArg(arg[i],XtNapply,(XtArgVal) popup_applyCBR); i++;
	XtSetArg(arg[i],XtNresetFactory,(XtArgVal) popup_resetfactoryCBR);i++;
	XtSetArg(arg[i],XtNsetDefaults, (XtArgVal) popup_setdefaultsCBR);i++;
	popupshell2 = XtCreatePopupShell("popupshell2", 
		popupWindowShellWidgetClass,popupbutton2,arg,i);

/* 
 * Add callback to popup button now that we have popupshell widget ID. 
 */
	XtAddCallback(popupbutton2, XtNselect, popupCB, popupshell2);

/*
 * Get widget IDs of popup children needed.  
 * Note that lower control area ID is not needed 
 * since automatic buttons are created by code above.
 */

	i = 0;
	XtSetArg(arg[i], XtNupperControlArea, &popupca21); i++;
	XtSetArg(arg[i], XtNfooterPanel, &popupfooter2); i++;
	XtGetValues (popupshell2, arg, i);

/* 
 * Populate popup upper control area with EXCLUSIVES and NONEXCLUSIVES.
 * Note that there is no need to populate popup lower controlarea.
 */
	i=0;
	XtSetArg(arg[i],XtNlayoutType,OL_FIXEDCOLS);i++;
	XtSetValues(popupca21,arg,i);

	widget =XtCreateManagedWidget("ex_caption",
		captionWidgetClass,popupca21,NULL,0);

	i = 0;
	XtSetArg(arg[i],XtNlayoutType,(XtArgVal) OL_FIXEDROWS); i++;
	XtSetArg(arg[i],XtNmeasure,(XtArgVal) 1); i++;
	exclusives = XtCreateManagedWidget("exclusives",
		exclusivesWidgetClass,widget,arg,i);

	widget = XtCreateManagedWidget("rbutton1",
		rectButtonWidgetClass,exclusives,NULL,0);
	XtAddCallback(widget,XtNselect,genericCB,(XtPointer) 1);

	widget = XtCreateManagedWidget("rbutton2",
		rectButtonWidgetClass,exclusives,NULL,0);
	XtAddCallback(widget,XtNselect,genericCB,(XtPointer) 2);

	widget = XtCreateManagedWidget("rbutton3",
		rectButtonWidgetClass,exclusives,NULL,0);
	XtAddCallback(widget,XtNselect,genericCB,(XtPointer) 3);

	widget = XtCreateManagedWidget("nex_caption",
		captionWidgetClass,popupca21,NULL,0);

	i = 0;
	XtSetArg(arg[i],XtNlayoutType,(XtArgVal) OL_FIXEDROWS); i++;
	XtSetArg(arg[i],XtNmeasure,(XtArgVal) 1); i++;
	nonexclusives = XtCreateManagedWidget("nonexclusives",
		nonexclusivesWidgetClass,widget,arg,i);

	widget = XtCreateManagedWidget("rbutton4",
		rectButtonWidgetClass,nonexclusives,NULL,0);
	XtAddCallback(widget,XtNselect,genericCB,(XtPointer) 1);

	nebutton2 = XtCreateManagedWidget("rbutton5",
		rectButtonWidgetClass,nonexclusives,NULL,0);
	XtAddCallback(nebutton2,XtNselect,nonexclusivesCB,nonexclusives);
	XtAddCallback(nebutton2,XtNselect,genericCB,(XtPointer)2);

/* 
 * Add text to the popup footer.
 */
	i = 0;
	XtSetArg(arg[i], XtNborderWidth, 0); i++;
	XtCreateManagedWidget("footer2",
		staticTextWidgetClass,popupfooter2,arg,i);
}

/*
 * End of POPUP: Property Sheet.
 */

/*
 * TO MAKE A MENU:
 *
 * Widget Tree for creating menu:
 *
 *		( parent widget )
 *			|
 *		menubutton ( visible/mouse sensitive symbol for menu )
 *			|
 *		pane ( used for placement of menu button set )
 *			|
 *		       / \
 *		     /	   \
 *	        button1   button2
 */

/*
 * Create a menubutton and menu with a pushpin.
 */

{
	static Widget menubutton,menupane,widget;

	i = 0;
	XtSetArg(arg[i],XtNpushpin,(XtArgVal) OL_OUT); i++;
	XtSetArg(arg[i],XtNrecomputeSize,(XtArgVal) TRUE); i++;
	menubutton = XtCreateManagedWidget("menubutton",
		menuButtonWidgetClass,controlarea,arg,i);

/* 
 * Get the Widget id of the menupane of the menubutton. 
 */
	i = 0;
	XtSetArg(arg[i], XtNmenuPane,(XtArgVal) &menupane); i++;
	XtGetValues(menubutton, arg, i);

/* 
 * Make two oblongbuttons on the menupane, with select callbacks.
 */

	widget = XtCreateManagedWidget("mbutton1",
		oblongButtonWidgetClass,menupane,NULL,0);
	XtAddCallback(widget,XtNselect,menuSelectCB,(XtPointer) 1);

	widget = XtCreateManagedWidget("mbutton2",
		oblongButtonWidgetClass,menupane,NULL,0);
	XtAddCallback(widget,XtNselect,menuSelectCB,(XtPointer) 2);
}

/*
 * End of MENU
 */

/* 
 * NOTICE: attach to the EXIT button in the control area.
 */

{

	static Widget noticetext;
	Widget widget;

	static Arg noticeARGS[] = {
		{XtNtextArea, (XtArgVal) &noticetext},
		{XtNcontrolArea, (XtArgVal) &noticebox},
	};

	cabutton = XtCreateManagedWidget("Exit",
		oblongButtonWidgetClass,controlarea,NULL,0);

/* 
 * Attach notice popup callback to "EXIT" controlarea button. 
 */

	XtAddCallback(cabutton, XtNselect, noticeCB1,NULL);

/* 
 * Create notice popup shell. 
 */

	noticeshell = XtCreatePopupShell("notice",
		noticeShellWidgetClass,cabutton,NULL,0);

/* 
 * Get the widget IDs of the notice's textarea and controlarea.
 */

	XtGetValues(noticeshell, noticeARGS, XtNumber(noticeARGS));

/* 
 * Add text to text area of noticebox: since the widget name
 * of the noticetext widget is unknown, OlGetMessage() is used to
 * set the string instead of using the app-defaults file.
 */
 
	i = 0;
	XtSetArg(	arg[i],
			XtNstring,
			(XtArgVal) OlGetMessage(	XtDisplay(toplevel),
							(char *)NULL,
							0,
							OleNnotice,
 							OleTmessage,
							OleCOlSamplerMsgs,
							OleMnotice_message,
							(XrmDatabase)NULL));
	i++;
	XtSetValues(noticetext,arg,i);

/* 
 * Add two buttons to noticebox: first has an exit() callback while
 * second is a no-op that pops down notice widget without exiting. 
 */

	widget = XtCreateManagedWidget("Okay", 
		oblongButtonWidgetClass,noticebox,NULL,0);
	XtAddCallback(widget, XtNselect,noticeCB2,noticebox);

	XtCreateManagedWidget("Cancel",
		oblongButtonWidgetClass,noticebox,NULL,0);
}

/*
 * End of NOTICE
 */

/*
 * The following widgets are all children or other descendants of form2.
 */

	form2 = XtCreateManagedWidget("form2",
		formWidgetClass,form1,NULL,0);

/*
 * Form3 contains the caption and stub widget for resolution independence.
 */

	form3 = XtCreateManagedWidget("form3",
		formWidgetClass,form2,NULL,0);
/* 
 * CAPTION
 */

{
	static Widget widget; 

	i = 0;
	XtSetArg(arg[i],XtNposition,(XtArgVal) OL_TOP); i++;
	XtSetArg(arg[i],XtNalignment,(XtArgVal) OL_CENTER); i++;
	caption = XtCreateManagedWidget("c_caption",
		captionWidgetClass,form3,arg,i);

	widget = XtCreateManagedWidget("Rainbow",
		oblongButtonWidgetClass,caption,NULL,0);
	XtAddCallback(widget, XtNselect, rainbowCB, NULL);
}

/* 
 * STUB WIDGET: this will be used as a drawing canvas.
 */

/* 
 * First two arguments scale widget to resolution of screen.
 */

	i = 0;
	XtSetArg(arg[i],XtNheight,(XtArgVal) N100_V_PIXELS); i++;
	XtSetArg(arg[i],XtNwidth,(XtArgVal) N100_H_PIXELS); i++;
	XtSetArg(arg[i],XtNbackground, skyblue_pixel); i++;
/*
 * DrawAndPrint() will be called with an Expose event;
 */
	XtSetArg(arg[i],XtNexpose, DrawAndPrint); i++; 
	stub = XtCreateManagedWidget("stub",stubWidgetClass,form3,arg,i);

/*
 * Add an eventhandler to track when the pointer 
 * enters and leaves the stub widget window.
 */

	XtAddEventHandler(stub,EnterWindowMask | LeaveWindowMask,
		FALSE, StubEventHandler, (XtPointer)NULL);

/* 
 * Set the special cursor for stub widget window 
 * below once the widget tree realized. 
 */

/* 
 * BULLETINBOARD: with ABBREVIATEDMENUBUTTON, CHECKBOX, 
 * and TEXTEDIT widgets in it, each within a caption 
 * labelling the widget.
 */

{
	Widget widget,bulletinboard,abbmenubutton,abbmenupane,checkbox;
	Dimension	height,yvalue,
			xpad = (Dimension) N10_H_PIXELS,
			ypad = (Dimension) N10_V_PIXELS;

/* 
 * BULLETINBOARD.
 */

	i = 0;
	XtSetArg(arg[i],XtNposition,(XtArgVal) OL_TOP); i++;
	XtSetArg(arg[i],XtNalignment,(XtArgVal) OL_CENTER); i++;
	bb_caption = XtCreateManagedWidget("bb_caption",
		captionWidgetClass,form2,arg,i);

	bulletinboard = XtCreateManagedWidget("bulletinboard",
	bulletinBoardWidgetClass,bb_caption,NULL,0);

/*
 * Set some resources for the bulletinboard.
 */

	i = 0;
	XtSetArg(arg[i],XtNborderWidth, (XtArgVal) 2); i++;
	XtSetArg(arg[i],XtNborderColor, (XtArgVal) orange_pixel); i++;
	XtSetValues(bulletinboard,arg,i);

/*
 * ABBREVIATEDMENUBUTTON.
 */

	yvalue = ypad;

	i = 0;
	XtSetArg(arg[i],XtNx,(XtArgVal) xpad); i++;
	XtSetArg(arg[i],XtNy,(XtArgVal) yvalue); i++;
	abbmenu_caption = XtCreateManagedWidget("abbmenu_caption",
		captionWidgetClass,bulletinboard,arg,i);

	abbmenubutton = XtCreateManagedWidget("abbmenubutton",
		abbrevMenuButtonWidgetClass,abbmenu_caption,arg,i);

/* 
 * Get the Widget ID of the menupane of the abbreviated menu button.
 */

	i = 0;
	XtSetArg(arg[i], XtNmenuPane,(XtArgVal) &abbmenupane); i++;
	XtGetValues(abbmenubutton, arg, i);

	widget = XtCreateManagedWidget("abm_button1",
		oblongButtonWidgetClass,abbmenupane,NULL,0);
 	XtAddCallback(widget,XtNselect,menuSelectCB,(XtPointer) 1);

	widget = XtCreateManagedWidget("abm_button2",
		oblongButtonWidgetClass,abbmenupane,NULL,0);
 	XtAddCallback(widget,XtNselect,menuSelectCB,(XtPointer) 2);

/*
 * CHECKBOX.
 */

	i=0;
	XtSetArg(arg[i],XtNheight, &height); i++;
	XtGetValues(abbmenubutton,arg,1);

	yvalue = yvalue + height + ypad;
	
	i = 0;
	XtSetArg(arg[i], XtNx,xpad); i++;
	XtSetArg(arg[i],XtNy, yvalue); i++;
	cb_caption = XtCreateManagedWidget("cb_caption",
		captionWidgetClass,bulletinboard,arg,i);

	checkbox = XtCreateManagedWidget("checkbox" ,
		checkBoxWidgetClass,cb_caption,NULL,0);
	XtAddCallback(checkbox,XtNselect,checkboxCB,NULL);

/* 
 * TEXTEDIT WIDGET.
 */

	i=0;
	XtSetArg(arg[i],XtNheight, &height); i++;
	XtGetValues(checkbox,arg,1);

	yvalue = yvalue + height + ypad;
	
	i = 0;
	XtSetArg(arg[i],XtNx,(XtArgVal) xpad); i++;
	XtSetArg(arg[i],XtNy, (XtArgVal) yvalue); i++;
	XtSetArg(arg[i],XtNalignment, (XtArgVal) OL_TOP); i++;
	te_caption = XtCreateManagedWidget("te_caption",
		captionWidgetClass,bulletinboard,arg,i);
				
	i = 0;
	XtSetArg(arg[i],XtNheight, (XtArgVal) N50_V_PIXELS); i++;
	XtSetArg(arg[i],XtNwidth, (XtArgVal) N150_H_PIXELS); i++;
	XtSetArg(arg[i],XtNsourceType, (XtArgVal) OL_STRING_SOURCE); i++;
	XtCreateManagedWidget("textedit",textEditWidgetClass,te_caption,arg,i);
}

/*
 * SCROLLINGLIST
 */

{
	static Widget scrollinglist;
	static OlListToken  (*scrollinglistADDfn)();
	static OlListItem sl_item[10];
	int natoi;
	char *label_format;

/*
 * Use a caption to label the scrollinglist.
 */

	i = 0;
	XtSetArg(arg[i],XtNposition,(XtArgVal) OL_TOP); i++;
	XtSetArg(arg[i],XtNalignment,(XtArgVal) OL_CENTER); i++;
	slist_caption = XtCreateManagedWidget("slist_caption",
		captionWidgetClass,form2,arg,i);

	i = 0;
	XtSetArg(arg[i], XtNviewHeight, (XtArgVal) 7); i++;
	scrollinglist = XtCreateManagedWidget("scrollinglist",
		scrollingListWidgetClass,slist_caption,arg,i);

/*
 * To add items, get the pointer to the scrollinglist function.
 */

	i = 0;
	XtSetArg(arg[i], XtNapplAddItem, (XtArgVal) &scrollinglistADDfn); 
	i++;
	XtGetValues(scrollinglist,arg,i);

	natoi = (int) 'A';
	label_format = OlGetMessage(	XtDisplay(toplevel),
					(char *)NULL,
					0,
					OleNscrollingList,
 					OleTitemLabel,
					OleCOlSamplerMsgs,
					OleMscrollingList_itemLabel,
					(XrmDatabase)NULL);
	
	for(i=0; i<10; i++) {
		sl_item[i].label_type = (OlDefine) OL_STRING;
		sl_item[i].label = XtMalloc(BUFSIZ*sizeof(char));
		sprintf(sl_item[i].label,label_format,(char *)natoi);

		sl_item[i].mnemonic = tolower(natoi);
/*
 * This field is for storing any type of data desired.
 */
		sl_item[i].user_data = (XtPointer) i ;

		(*scrollinglistADDfn) (scrollinglist,NULL,NULL,sl_item[i]); 
		natoi++;
	}

/* 
 * Callback to be invoked when user selects an item. 
 */

	XtAddCallback(scrollinglist,XtNuserMakeCurrent,scrollinglistCB,NULL);
}

/*
 * SCROLLEDWINDOW
 */

{
static Widget scrolledwindow;
static char	*swstring; 

	swstring = OlGetMessage(	XtDisplay(toplevel),
					(char *)NULL,
					0,
					OleNscrolledWindow,
					OleTstring,
					OleCOlSamplerMsgs,
					OleMscrolledWindow_string,
					(XrmDatabase) NULL);

	i = 0;
	XtSetArg(arg[i],XtNposition,(XtArgVal) OL_TOP); i++;
	XtSetArg(arg[i],XtNalignment,(XtArgVal) OL_CENTER); i++;
	sw_caption = XtCreateManagedWidget("sw_caption",
		captionWidgetClass,form2,arg,i); 

	i = 0;
	XtSetArg(arg[i], XtNheight,N150_V_PIXELS); i++;
	XtSetArg(arg[i], XtNwidth, N100_H_PIXELS); i++;
	XtSetArg(arg[i], XtNvStepSize, 20); i++;
	XtSetArg(arg[i], XtNhStepSize, 20); i++;
	scrolledwindow = XtCreateManagedWidget("scrolledwindow",
		scrolledWindowWidgetClass,sw_caption,arg,i);
	XtAddCallback(scrolledwindow,
			XtNvSliderMoved,scrollbarCB,(XtPointer)1);
	XtAddCallback(scrolledwindow,
			XtNhSliderMoved,scrollbarCB,(XtPointer)2);

/* 
 * Make a statictext widget in the scrolled window
 * to scroll with the scrollbars.
 */

	i = 0;
	XtSetArg(arg[i], XtNheight,N150_V_PIXELS); i++;
	XtSetArg(arg[i], XtNstring, swstring); i++;
	XtCreateManagedWidget("statictext3",
		staticTextWidgetClass,scrolledwindow,arg,i);
}

/*
 * SLIDER 
 */

/*
 * The slider will be used to change the background of the stub widget: 
 * calculate the number of colors for the screen (N) and set the slider 
 * range from 0 to (N-1) with a granularity of 1; see the sliderCB 
 * function for the rest of the code/functionality.
 */

{
	Display *display = XtDisplay(toplevel);
	int screen = XDefaultScreen(display);
	int n, ncolors=2;
	Widget w;

	n= XDefaultDepth(display,screen);

	for(i=1; i<n; i++) {
		ncolors= ncolors*2;
	}

	ncolors = ncolors - 1 ;

	i = 0;
	XtSetArg(arg[i],XtNposition,(XtArgVal) OL_TOP); i++;
	XtSetArg(arg[i],XtNalignment,(XtArgVal) OL_CENTER); i++;
	slider_caption = XtCreateManagedWidget("slider_caption",
		captionWidgetClass,form2,arg,i); 

	i = 0; 
	XtSetArg(arg[i],XtNwidth,(XtArgVal) N200_H_PIXELS); i++; 
	XtSetArg(arg[i],XtNorientation, (XtArgVal) OL_HORIZONTAL); i++;
	XtSetArg(arg[i],XtNsliderMax, (XtArgVal) ncolors); i++;
	XtSetArg(arg[i],XtNgranularity, (XtArgVal) 1); i++;
	XtSetArg(arg[i], XtNticks, (XtArgVal) 1); i++; 
	XtSetArg(arg[i], XtNtickUnit, (XtArgVal) OL_SLIDERVALUE); i++; 
	XtSetArg(arg[i], XtNdragCBType, (XtArgVal) OL_RELEASE); i++; 

	w = XtCreateManagedWidget("slider",
		sliderWidgetClass,slider_caption,arg,i);
	XtAddCallback(w,XtNsliderMoved,sliderCB,NULL);
}

/*
 * GAUGE
 */

	i = 0;
	XtSetArg(arg[i],XtNposition,(XtArgVal) OL_TOP); i++;
	XtSetArg(arg[i],XtNalignment,(XtArgVal) OL_CENTER); i++;
	g_caption = XtCreateManagedWidget("g_caption",
		captionWidgetClass,form2,arg,i); 

	i = 0; 
	gauge = XtVaCreateManagedWidget("gauge",
			gaugeWidgetClass,
			g_caption,
			XtNwidth,(XtArgVal) (N200_H_PIXELS),
			XtNorientation, (XtArgVal) OL_HORIZONTAL,
			XtNsliderMax, (XtArgVal) 100,
			XtNgranularity, (XtArgVal) 10,
			XtNticks, (XtArgVal) 10,
			XtNtickUnit, (XtArgVal) OL_SLIDERVALUE,
			(String) 0 );

/* 
 * GADGETS: the oblongbutton gadget and abbreviatedmenubutton
 * gadget are displayed in a controlarea, the latter in a 
 * caption entitled, "Gadgets."
 */

{
	Widget carea,button,menubutton,menupane;

	i=0;
	XtSetArg(arg[i],XtNposition,(XtArgVal) OL_TOP); i++;
	XtSetArg(arg[i],XtNalignment,(XtArgVal) OL_CENTER); i++;
	gd_caption = XtCreateManagedWidget("gd_caption",
		captionWidgetClass,form2,arg,i);

	i=0;
	XtSetArg(arg[i],XtNborderWidth, (XtArgVal) 2); i++;
	XtSetArg(arg[i],XtNborderColor, (XtArgVal) orange_pixel); i++;
	carea = XtCreateManagedWidget("controlarea",
		controlAreaWidgetClass,gd_caption,arg,i);

	i=0;
	XtSetArg(arg[i],XtNx,(XtArgVal) N10_H_PIXELS); i++;
	XtSetArg(arg[i], XtNy,(XtArgVal) N10_V_PIXELS); i++;
	button = XtCreateManagedWidget("buttongadget",
		oblongButtonGadgetClass,carea,arg,i);
 	XtAddCallback(button,XtNselect,genericCB,(XtPointer) 8);

	i = 0;
	XtSetArg(arg[i],XtNx,(XtArgVal) N100_H_PIXELS);
	XtSetArg(arg[i],XtNy,(XtArgVal) N10_V_PIXELS); i++;
	menubutton = XtCreateManagedWidget("menubuttongadget",
		menuButtonGadgetClass,carea,arg,i);
	i = 0;
	XtSetArg(arg[i], XtNmenuPane,(XtArgVal) &menupane); i++;
	XtGetValues(menubutton,arg,i);

	button = XtCreateManagedWidget("gbutton1",
		oblongButtonGadgetClass,menupane,NULL,0);
 	XtAddCallback(button,XtNselect,menuSelectCB,(XtPointer) 1);

	button = XtCreateManagedWidget("gbutton2",
		oblongButtonGadgetClass,menupane,NULL,0);
 	XtAddCallback(button,XtNselect,menuSelectCB,(XtPointer) 2);
}

/* 
 * FLAT WIDGETS: the flatexclusives, flatnonexclusives, and
 * flatcheckbox are displayed in a controlarea, the latter
 * in a caption entitled, "Flat Widgets."
 */

{
	Widget widget,carea;
	WidgetClass class;
	int num_subobjects = 3;
	char *checkbox_msg,*checkbox_label;
	Dimension hspace = N10_H_PIXELS,
	          vspace = N10_V_PIXELS;

/*
 * For flatexclusives and flatnonexclusives: note "customized" fields.
 */

	static String fields1[] = {
		XtNbackground, XtNlabel, XtNselectProc, XtNclientData
	};

	typedef struct {
		XtArgVal	background;	/* item's background */
		XtArgVal	label;		/* item's label */
		XtArgVal	select;		/* item's select callback */
		XtArgVal	clientData;	/* clientData for callback */
	} FlatData1;

	static FlatData1 *items1;

/*
 * For flatcheckbox: note "customized" fields.
 */

	static String fields2[] = {
		XtNbackground, XtNlabel, XtNset, XtNselectProc, XtNclientData
	};

	typedef struct {
		XtArgVal	background;	/* item's background */
		XtArgVal	label;		/* item's label */
		XtArgVal	set;		/* item's set status */
		XtArgVal	select;		/* item's select callback */
		XtArgVal	clientData;	/* clientData for callback */
	} FlatData2;

	static FlatData2 *items2;

	items1 = (FlatData1 *)XtMalloc((Cardinal)
		(num_subobjects*sizeof(FlatData1)));

	items2 = (FlatData2 *)XtMalloc((Cardinal)
		(num_subobjects*sizeof(FlatData2)));

	i=0;
	XtSetArg(arg[i],XtNposition,(XtArgVal) OL_TOP); i++;
	XtSetArg(arg[i],XtNalignment,(XtArgVal) OL_CENTER); i++;
	f_caption = XtCreateManagedWidget("f_caption",
		captionWidgetClass,form2,arg,i);

	i=0;
	XtSetArg(arg[i],XtNborderWidth,(XtArgVal) 2); i++;
	XtSetArg(arg[i],XtNborderColor,(XtArgVal) orange_pixel); i++;
	XtSetArg(arg[i],XtNhSpace,hspace); i++;
	XtSetArg(arg[i],XtNvSpace,vspace); i++;
	XtSetArg(arg[i],XtNhPad,hspace); i++;
	XtSetArg(arg[i],XtNvPad,vspace); i++;

	carea = XtCreateManagedWidget("controlarea",
		controlAreaWidgetClass,f_caption,arg,i);

	widget =XtCreateManagedWidget("e_caption",
		captionWidgetClass,carea,NULL,0);

	items1[0].background = (XtArgVal) red_pixel;
	items1[1].background = (XtArgVal) green_pixel;
	items1[2].background = (XtArgVal) blue_pixel;

	items1[0].label = items1[1].label = items1[2].label = (XtArgVal) "  ";

	items1[0].select = items1[1].select = items1[2].select
			= (XtArgVal) genericCB;

	items1[0].clientData = 
		items1[1].clientData = items1[2].clientData = (XtArgVal) 9;

	class = flatExclusivesWidgetClass;
	i=0;
	XtSetArg(arg[i],XtNitems,items1);i++;
	XtSetArg(arg[i],XtNnumItems,num_subobjects);i++;
	XtSetArg(arg[i],XtNitemFields,fields1); i++;
	XtSetArg(arg[i],XtNnumItemFields,XtNumber(fields1)); i++;
	XtCreateManagedWidget("flatexclusives",class,widget,arg,i);

	widget =XtCreateManagedWidget("ne_caption",
		captionWidgetClass,carea,NULL,0);

	class = flatNonexclusivesWidgetClass;
	XtCreateManagedWidget("flatnonexclusives",class,widget,arg,i);

	items2[0].background = (XtArgVal) red_pixel;
	items2[1].background = (XtArgVal) green_pixel;
	items2[2].background = (XtArgVal) blue_pixel;

	checkbox_msg = OlGetMessage(	XtDisplay(widget),
					NULL,
					0,
					OleNflatCheckbox,
					OleTlabel,
					OleCOlSamplerMsgs,
					OleMflatCheckbox_label,
					(XrmDatabase) NULL);

	for(i=0; i<3; i++) {
		checkbox_label = XtMalloc(BUFSIZ*sizeof(char));
		sprintf(checkbox_label,checkbox_msg,i+1);
		items2[i].label = (XtArgVal) checkbox_label;
	}

	items2[0].set = items2[1].set = items2[2].set = (XtArgVal) TRUE;

	items2[0].select = items2[1].select = items2[2].select
			= (XtArgVal) genericCB;
	items2[0].clientData = 
		items2[1].clientData = items2[2].clientData = (XtArgVal) 9;

	class = flatCheckBoxWidgetClass;

	i=0;
	XtSetArg(arg[i],XtNitems,items2);i++;
	XtSetArg(arg[i],XtNnumItems,num_subobjects);i++;
	XtSetArg(arg[i],XtNitemFields,fields2); i++;
	XtSetArg(arg[i],XtNnumItemFields,XtNumber(fields2)); i++;

	XtCreateManagedWidget("flatcheckbox",class,carea,arg,i);
}

/* 
 * SCROLLBAR: attach to the right & bottom of form1.
 */

{

static Arg scrollbarARGS[] = {

/* 
 * Widget resources. 
 */

	{ XtNproportionLength, 10 },
	{ XtNshowPage, OL_LEFT },

/* 
 * Form resources. 
 */

	{ XtNyResizable,(XtArgVal) TRUE },
	{ XtNyAttachBottom,(XtArgVal)TRUE },
	{ XtNxAttachRight,(XtArgVal) TRUE },
	{ XtNxVaryOffset,(XtArgVal) TRUE },
};

	scrollbar = XtCreateManagedWidget("scrollbar",
		scrollbarWidgetClass,form1,scrollbarARGS,
		XtNumber(scrollbarARGS));
	XtAddCallback(scrollbar,XtNsliderMoved,scrollbarCB,(XtPointer) 0);
}

/*
 * Position all the widgets relatively.  Note that reference names are used.
 * Note that 10 horizontal and vertical pixels are used in most cases; in a  
 * few cases special values are used.
 */

/*
 * First place the top controlarea caption, form2, and scrollbar on form1.
 */

	genericARGS[2].value = (XtArgVal) 0;
	genericARGS[3].value = (XtArgVal) 0;
	SetPosition(ca_caption,"form1","form1");
	genericARGS[2].value = (XtArgVal) N10_H_PIXELS;
	genericARGS[3].value = (XtArgVal) N10_V_PIXELS;

	SetPosition(form2,"form1","ca_caption");

	genericARGS[3].value = (XtArgVal) 0;
	SetPosition(scrollbar,"form2","ca_caption");
	genericARGS[3].value = (XtArgVal) N10_V_PIXELS;

/*
 * Then position the caption and stub widgets on form3.
 */

	SetPosition(caption,"form3","form3");
	SetPosition(stub,"form3","c_caption");

/*
 * Now postion form 3 and the remaining widgets on form2.
 */

	SetPosition(form3,"form2","form2");
	SetPosition(bb_caption,"form3","form2");
	SetPosition(slider_caption,"form2","form3");
	SetPosition(g_caption,"form2","slider_caption");
	SetPosition(f_caption,"form2","gd_caption");
	SetPosition(slist_caption,"bb_caption","form2");
	SetPosition(sw_caption,"slist_caption","form2");

	genericARGS[2].value = (XtArgVal) (7 * N10_H_PIXELS);
	SetPosition(gd_caption,"slider_caption","slist_caption");
	genericARGS[2].value = (XtArgVal) N10_H_PIXELS;

/*
 * Realize the widget tree.
 */

	XtRealizeWidget(toplevel); 

/* 
 * The special cursor for the stub widget can be set now
 * that it has been realized as part of the widget tree. 
 */

	SetStubCursor(stub);

/* 
 * Turn control over to the Xt intrinsics and OPEN LOOK.
 */

	XtMainLoop();

} /* MAIN */

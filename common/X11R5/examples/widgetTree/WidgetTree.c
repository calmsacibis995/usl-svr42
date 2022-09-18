/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:WidgetTree.c	1.2"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Panes.h>
#include <Xol/StaticText.h>
#include <Xol/OblongButt.h>
#include <Xol/RectButton.h>
#include <Xol/ScrolledWi.h>
#include <Xol/RubberTile.h>
#include <Xmu/Editres.h>
#include "WidgetTree.h"

extern char * item_fields[] = {XtNlabel, XtNselectProc};
extern int num_item_fields = XtNumber(item_fields);

static struct widget_tree WidgetTree[] = {
        /* class                reference               position   select M? */
        { "transientShell",     NULL,                   OL_BOTTOM, NULL, 1 },
        { "core",               "transientShell",       OL_BOTTOM, NULL, 1 },
        { "rectObject",         "core",                 OL_BOTTOM, NULL, 1 },
        { "menuShell",          "transientShell",       OL_RIGHT, MenuCB, 0 },
        { "modalShell",         "menuShell",            OL_BOTTOM, ModalCB, 1 },
        { "popupMenuShell",     "modalShell",           OL_BOTTOM, PopupMenuCB, 1 },
        { "popupWindowShell",   "popupMenuShell",       OL_BOTTOM, PopupWindowCB, 1 },
        { "noticeShell",        "modalShell",           OL_RIGHT, NoticeCB, 1 },
        { "manager",            "core",                 OL_RIGHT, NULL, 1 },
        { "primitive",          "manager",              OL_BOTTOM, NULL, 1 },
        { "bulletinBoard",      "manager",              OL_RIGHT, BulletinBoardCB, 1 },
        { "caption",            "bulletinBoard",        OL_BOTTOM, CaptionCB, 1 },
        { "category",           "caption",              OL_BOTTOM, CategoryCB, 1 },
        { "controlArea",        "category",             OL_BOTTOM, ControlAreaCB, 1 },
        { "exclusives",         "controlArea",          OL_BOTTOM, ExclusivesCB, 0 },
        { "form",               "exclusives",          OL_BOTTOM, FormCB, 1 },
        { "nonexclusives",      "form",                 OL_BOTTOM, NonexclusivesCB, 0 },
        { "panes",              "nonexclusives",        OL_BOTTOM, PanesCB, 1 },
        { "scrolledWindow",     "panes",           OL_BOTTOM, ScrolledWindowCB, 1 },
        { "scrollingList",      "form",                 OL_RIGHT, ScrollingListCB, 0 },
        { "abbreviatedButton",  "primitive",            OL_RIGHT, AbbreviatedButtonCB, 1 },
        { "abbreviatedMenuButton", "abbreviatedButton", OL_BOTTOM, AbbreviatedMenuCB, 0 },
        { "checkbox",           "abbreviatedMenuButton", OL_BOTTOM, CheckboxCB, 1 },
        { "flat",               "checkbox",             OL_BOTTOM, NULL, 1 },
        { "footer",             "flat",                 OL_BOTTOM, FooterCB, 1 },
        { "gauge",              "footer",               OL_BOTTOM, GaugeCB, 1 },
        { "menuButton",         "gauge",             OL_BOTTOM, MenuButtonWidgetCB, 0 },
        { "oblongButton",       "menuButton",           OL_BOTTOM, OblongButtonWidgetCB, 0 },
        { "rectButton",         "oblongButton",         OL_BOTTOM, RectButtonWidgetCB, 0 },
        { "scrollbar",          "rectButton",           OL_BOTTOM, ScrollbarCB, 1 },
        { "slider",             "scrollbar",            OL_BOTTOM, SliderCB, 1 },
        { "staticText",         "slider",               OL_BOTTOM, StaticTextCB, 1 },
        { "stub",               "staticText",           OL_BOTTOM, StubCB, 1 },
        { "text",               "stub",                 OL_BOTTOM, TextCB, 0 },
        { "textEdit",           "text",                 OL_BOTTOM, TextEditCB, 1 },
        { "rubberTile",         "panes",                OL_RIGHT, RubberTileCB, 1 },
        { "footerPanel",        "rubberTile",           OL_RIGHT, FooterPanelCB, 1 },
        { "help",               "footerPanel",          OL_BOTTOM, HelpCB, 1 },
        { "flatRowColumn",      "flat",                 OL_RIGHT, NULL, 1 },
        { "flatButtons",        "flatRowColumn",        OL_RIGHT, FButtonsCB, 1 },
        { "flatList",           "flatButtons",          OL_BOTTOM, FListCB, 1 },
        { "flatCheckBox",     "flatButtons",          OL_RIGHT, FCheckBoxCB, 1 },
        { "flatExclusives",     "flatCheckBox",          OL_BOTTOM, FExclusivesCB, 1 },
        { "flatNonexclusives",  "flatExclusives",       OL_BOTTOM, FNonexclusivesCB, 1 },
        { "textField",          "textEdit",             OL_RIGHT, TextFieldCB, 1 },
        { "integerField",       "textField",            OL_RIGHT, IntegerFieldCB, 1 },
        { "stepField",          "integerField",         OL_BOTTOM, StepFieldCB, 1 },
        { "eventObject",        "rectObject",           OL_RIGHT, NULL, 1 },
        { "oblongButtonGadget", "eventObject",          OL_RIGHT, OblongButtonGadgetCB, 0 },
        { "menuButtonGadget",   "oblongButtonGadget",   OL_BOTTOM, MenuButtonGadgetCB, 0 },
};

static void BusyCB OL_ARGS((Widget, XtPointer, XtPointer));

main ( argc, argv)
        int argc;
        char ** argv;
{
        Cardinal i, n;
        Widget top, rubber, sw, pane, w;
	Dimension height;

        /*  Initialize the toolkit and the intrinsics  */
        OlToolkitInitialize(&argc, argv, NULL); 
        top = XtInitialize("widgetTree", "WidgetTree", NULL, 0, &argc, argv);

        /*  Enable the editres application. */
        XtAddEventHandler(top, (EventMask) 0, True,
                                _XEditResCheckMessages, NULL);

        rubber = XtVaCreateManagedWidget("rubber", rubberTileWidgetClass, top, 
                (String) 0);

        XtVaCreateManagedWidget("title", staticTextWidgetClass, rubber,
                XtNstring, "MooLIT Widget Class Hierarchy",
                XtNfont, (XFontStruct *) _OlGetDefaultFont(rubber, OlDefaultNoticeFont),
                XtNweight, 0, 
                (String)0);

        sw = XtVaCreateManagedWidget("scrolledWindow",
                scrolledWindowWidgetClass, rubber,
		XtNviewWidth, 375,
                (String)0);

        pane = XtVaCreateManagedWidget("pane", panesWidgetClass, sw,
		XtNshadowThickness, 0,
		(String) 0);

        /*  Create the widget tree using the static definition above. */
        n = XtNumber(WidgetTree);
        for (i = 0; i < n; i++)  {
		if (WidgetTree[i].callback != NULL)  {
                	w = XtVaCreateManagedWidget(WidgetTree[i].class,
                        	oblongButtonWidgetClass, pane,
                        	XtNrefName, WidgetTree[i].ref,
                        	XtNrefPosition, WidgetTree[i].pos,
                        	XtNwidth, 150,
				XtNsensitive, (OlGetGui() == OL_MOTIF_GUI &&
					!WidgetTree[i].motif) ? False : True,
                        	(String) 0);
                        XtAddCallback(w, XtNselect, WidgetTree[i].callback,
                               	WidgetTree[i].class);
                        XtAddCallback(w, XtNselect, BusyCB, NULL);
		}
		else  {
                	w = XtVaCreateManagedWidget(WidgetTree[i].class,
                        	rectButtonWidgetClass, pane,
                        	XtNrefName, WidgetTree[i].ref,
                        	XtNrefPosition, WidgetTree[i].pos,
                        	XtNwidth, 150,
				XtNfont, (XFontStruct *) _OlGetDefaultFont(pane, OlDefaultItalicFont),
				XtNsensitive, (OlGetGui() == OL_MOTIF_GUI &&
					!WidgetTree[i].motif) ? False : True,
                        	(String) 0);
		}
        }

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) top, NULL,
                        OL_DISK_SOURCE, "README");

	/*  Set the granularity on the scrolled window to move a "widget"
	    at a time.  */
	XtVaGetValues(w, XtNheight, &height, 0);
	XtVaSetValues(sw, XtNvStepSize, height, 0);

        XtRealizeWidget(top);

        XtMainLoop();

}  /* end of main() */

/*ARGSUSED*/
static void
BusyCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
/*
	XtVaSetValues(w, XtNbusy, True, NULL);
*/
}  /* end of BusyCB() */

/*ARGSUSED*/
void
DestroyCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
	XtDestroyWidget(w);
	XtVaSetValues((Widget)client_data, XtNbusy, False, NULL);
}  /* end of DestroyCB() */

/*ARGSUSED*/
void
PopdownCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
	XtPopdown((Widget) client_data);
}  /* end of PopdownCB() */


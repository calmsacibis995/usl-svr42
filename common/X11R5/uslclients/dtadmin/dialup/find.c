/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/find.c	1.16"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <FButtons.h>
#include <PopupWindo.h>
#include <Caption.h>
#include <Footer.h>
#include <Gizmos.h>
#include <locale.h>
#include "uucp.h"
#include "error.h"

extern char *		ApplicationName;
extern void		BringDownPopup();
extern Widget		AddMenu();
extern void		UnselectSelect();
extern void		DisallowPopdown();
extern void		HelpCB();

static void	Find();
static void	Cancel();
static Widget	footer;

Arg arg[50];

static HelpText AppHelp = {
    title_find, HELP_FILE, help_find,
};

static Items findItems[] = {
	{Find, NULL, (XA)TRUE},
	{Cancel, NULL, (XA)TRUE},
	{HelpCB, NULL, (XA)TRUE, NULL, NULL, (XA)&AppHelp},
};

static Menus findMenu = {
	"find",
	findItems,
	XtNumber (findItems),
	False,
	OL_FIXEDROWS,
	OL_NONE
};

static void
Cancel(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	XtPopdown(sf->findPopup);
}

void
FindPopupCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	XtPopup(sf->findPopup, XtGrabNone);
	SetValue (footer, XtNleftFoot, "");
	XRaiseWindow(DISPLAY, XtWindow(sf->findPopup));
}

void
Find(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	char f_name[BUFSIZ];
	static char *regx = NULL;
	static char *string = NULL;
	int i, j, k;
	char *oloc;

	SetValue (footer, XtNleftFoot, "");
	if (sf->numFlatItems == 0 || sf->numFlatItems == 1) {
		PUTMSG(GGT(string_noItem));
		BringDownPopup(sf->findPopup);
		return;
	}
	if (string != NULL)
		XtFree (string);
	XtVaGetValues (
		sf->findTextField,
		XtNstring, &string,
		(String)0
	);
#ifdef debug
	printf ("String = %s\n", string);
#endif
	oloc = setlocale(LC_CTYPE, (char *) NULL);
	if (setlocale(LC_CTYPE, "C") != 0) {
		if (regx == NULL) {
			regx = (char *)regcmp (
				"([A-Za-z][A-Za-z0-9]*)$0",
				0
			);
		}
		if ((void *)regex (regx, string, f_name) == NULL) {
			(void) setlocale(LC_CTYPE, oloc);
			return;
		}
		(void) setlocale(LC_CTYPE, oloc);
	}
	if ((j = strlen (f_name)) == 0)
		return;
	for (i=sf->currentItem+1; i<sf->numFlatItems; i++) {
		k = strlen ((char *)sf->flatItems[i].pField->f_name);
		if (j > k) continue;
		if (strncmp (
			f_name,
			(char *)sf->flatItems[i].pField->f_name,
			j
		    ) == 0) {
			sf->currentItem = i;
			UnselectSelect ();
			BringDownPopup(sf->findPopup);
			return;
		}
	}
	for (i=0; i<= sf->currentItem; i++) {
		k = strlen ((char *)sf->flatItems[i].pField->f_name);
		if (j > k) continue;
		if (strncmp (
			f_name,
			(char *)sf->flatItems[i].pField->f_name,
			j
		    ) == 0) {
			sf->currentItem = i;
			UnselectSelect ();
			BringDownPopup(sf->findPopup);
			return;
		}
	}
	SetValue (footer, XtNleftFoot, GGT(string_notFound));
}/* Find */
 
void
GetFindPopup(parent)
Widget parent;
{
	Widget	button_area;
	Widget	prompt_area;
	Widget	footer_area;
	Widget	caption;
	char	buf[128];

	SET_HELP(AppHelp);
	sprintf(buf, "%s: %s", ApplicationName, GGT(label_find));
	XtSetArg (arg[0], XtNtitle, buf);
	sf->findPopup = XtCreatePopupShell(
		"FindPopup",
		popupWindowShellWidgetClass,
		parent,
		arg, 1
	);

	XtAddCallback (
                sf->findPopup,
                XtNverify,
                DisallowPopdown,
                (XtPointer)0
        );

        XtVaGetValues (sf->findPopup,
                XtNlowerControlArea,    (XtArgVal) &button_area,
                XtNupperControlArea,    (XtArgVal) &prompt_area,
                XtNfooterPanel,		(XtArgVal) &footer_area,
                0);

	caption = XtVaCreateManagedWidget(
		"caption",
		captionWidgetClass,
		prompt_area,
		XtNlabel, GGT(label_system),
		XtNborderWidth, 0,
		(String) 0
	);

	sf->findTextField = XtVaCreateManagedWidget(
		"input",
		textFieldWidgetClass,
		caption,
		XtNstring, "",
		XtNborderWidth, 0,
		XtNcharsVisible, 20,
		XtNmaximumSize, 20,
		(String) 0
	);

	footer = XtVaCreateManagedWidget(
		"find_footer",
		footerWidgetClass,
		footer_area,
		XtNweight,		0,
		XtNleftFoot,		"",
		XtNleftWeight,		100,
		XtNrightFoot,		"",
		XtNrightWeight,		0,
		(String)0
	);

	SET_LABEL (findItems, 0, find);
	SET_LABEL (findItems, 1, cancel);
	SET_LABEL (findItems, 2, help);

	AddMenu (button_area, &findMenu, False);
} /* GetFindPopup */

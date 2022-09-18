/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olpixmap:pixmap.c	1.30"
#endif

#include "pixmap.h"
#include "error.h"
#include <Xol/Error.h>


extern Widget	InitializeAllVisuals();


Widget		Toplevel;
char *		ApplicationName = APP_NAME;
char *		Argv_0;

Dimension	PixmapWidth,				/* in pixels */
		PixmapHeight;				/* in pixels */
Cardinal	PixmapDepth;				/* in planes */
Colormap	PixmapColormap;

AppResStruct	AppResources;


static XrmOptionDescRec		Options[] =
{
	{ "-iconmenu",	"*iconMenu",	XrmoptionNoArg,	(XtPointer) "True" },
};

static XtResource		Resources[] =
{
#define offset(field) XtOffset(AppResStruct *, field)
	{ "iconMenu", "IconMenu", XtRBoolean, sizeof(Boolean),
		offset(icon_menu), XtRImmediate, (XtPointer) False },
	{ "warnings", "Warnings", XtRBoolean, sizeof(Boolean),
		offset(warnings), XtRImmediate, (XtPointer) False },
#undef offset
};

static Widget		Footer;

static void	IgnoreWarnings();
static void	Usage();



void
main(argc, argv)
int argc;
char *argv[];
{
#ifdef UseXtApp
	XtAppContext	app_con;
#endif

	Argv_0 = argv[0];

#ifdef MEMUTIL
	InitializeMemutil();
#endif

	OlToolkitInitialize(&argc, argv, NULL);

#ifdef UseXtApp
	Toplevel = XtAppInitialize(
			&app_con,		/* app_context_return	*/
			"olpixmap",		/* application_class	*/
			Options,		/* options		*/
			XtNumber(Options),	/* num_options		*/
			&argc,			/* argc_in_out		*/
			argv,			/* argv_in_out		*/
			(String)NULL,		/* fallback_resources	*/
			(ArgList)NULL,		/* args			*/
			(Cardinal)0		/* num_args		*/
	);
#else
	Toplevel = XtInitialize("olpixmap", "olpixmap",
			Options, XtNumber(Options), &argc, argv);
#endif

        ApplicationName = OlGetMessage(DISPLAY, NULL, 0,
				       OleNfixedString,
				       OleTappName,
				       OleCOlClientOlpixmapMsgs,
				       OleMfixedString_appName,
				       (XrmDatabase)NULL);
	
	_OlSetApplicationTitle(ApplicationName);
	OlGetApplicationResources(Toplevel, (XtPointer) &AppResources,
				Resources, XtNumber(Resources), NULL, 0);
	if (AppResources.warnings == False) {
		XtSetWarningHandler(IgnoreWarnings);
		OlSetWarningHandler(IgnoreWarnings);
	}
	if (argc > 2)
		Usage(Toplevel);

	DtInitialize(Toplevel);

	XtSetMappedWhenManaged(Toplevel, False);
	Footer = InitializeAllVisuals(Toplevel);
	FooterMessage(NULL, False);
	SetStatus(Normal);

	if (!OpenFile((argc == 2) ? argv[1] :
		      OlGetMessage(DISPLAY, NULL, BUFSIZ,
				   OleNfixedString,
				   OleTuntitled,
				   OleCOlClientOlpixmapMsgs,
				   UNNAMED_FILE,
				   (XrmDatabase)NULL)))
		ResetAllVisuals((Pixmap)0, PixmapWidth, PixmapHeight);

	XtMapWidget(Toplevel);
	ShowPixmap(Toplevel, (XtPointer) 0, (XtPointer) 0);

#ifdef UseXtApp
	XtAppMainLoop(app_con);
#else
	XtMainLoop();
#endif
}


static void
IgnoreWarnings(message)
char * message;
{
	/* do nothing */
}


static void
Usage(w)
  Widget w;
{
  OlVaDisplayErrorMsg(XtDisplay(w),
		      OleNbadCommandLine,
		      OleTusage,
		      OleCOlClientOlpixmapMsgs,
		      OleMbadCommandLine_usage,
		      Argv_0);

}


void
FooterMessage(message, important)
char * message;
Bool important;
{
	char *	saved_msg = message;

 	if (message) {
		if (important)
			_OlBeepDisplay(Footer, 1);
	} else {
		message = " ";
	}

	if (saved_msg != NULL)
	{
			/* inform this routine so that next SELECT
			 * button press will clear the status area...
			 */
		PixelEventHandler(
			(Widget)NULL, (XtPointer)NULL,
			(XEvent *)NULL, (Boolean *)NULL
		);
	}

	INIT_ARGS();
	SET_ARGS(XtNstring, message);
	SET_VALUES(Footer);
	END_ARGS();
}

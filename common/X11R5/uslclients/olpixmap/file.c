/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*copyright     "%c%"*/

#ifndef NOIDENT
#ident	"@(#)olpixmap:file.c	1.43"
#endif

/*
 *	XWDFormat by itself is not the answer, since (as documented in
 *	XWDFile.h) it is "not portable between machines of differing
 *	word sizes".  Also, XWDFormat is not #include'able, so images
 *	in that format would have to be read at runtime.
 */

#include <IntrinsicP.h>
#include "pixmap.h"
#include "Gizmo/xpm.h"
#include "error.h"
#include "Xol/Error.h"
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <Dt/Desktop.h>
#include <Dt/DtMsg.h>
#include <OblongButt.h>
#include <TextField.h>
#include <Caption.h>
#include <PopupWindo.h>
#include <Notice.h>
#include <CoreP.h>
#include <OlCursors.h>

#include <errno.h>
#include <sys/stat.h>
#include <FMcomm.h>


extern int	errno;
extern char	*sys_errlist[];

extern char *	Argv_0;
extern Widget	AddMenu ();


Widget		FilePopup;
Widget		FileNotice = (Widget) NULL;
Bool		Changed = False;


static char *	Filename = NULL;		/* name of input file */
static enum	IO_error {e_invalid, e_cannot_open,
					e_cannot_write, e_no_memory};

static Widget	PromptTextField;
static Widget	OpenButton;
static Bool	PopdownOK;

static void	VerifyPopdown();
static void	OpenAndClear();
static void	SaveFull();
static void	HandleCurrentContents();
static void	RequestDesktopManager();
static void	FilePopdown();
void FilePopupCallback();

#define NUL (XtPointer)0

static char *Labels[2];

static MenuItem file_items[] = {
  {(XtArgVal)NULL, (XtArgVal)FilePopupCallback, (XtArgVal)True, (XtArgVal)True, 
				(XtArgVal)True, (XtArgVal)NUL},
  {(XtArgVal)NULL, (XtArgVal)RequestDesktopManager,(XtArgVal)NUL,
			 (XtArgVal)True, (XtArgVal)True,(XtArgVal) NUL},
  {(XtArgVal)NULL,   (XtArgVal)SaveFull,
		(XtArgVal)NUL, (XtArgVal)True, (XtArgVal)True,(XtArgVal) NUL},
  {(XtArgVal)NULL,   (XtArgVal)FilePopupCallback,
		(XtArgVal)False, (XtArgVal)True, (XtArgVal)True,(XtArgVal) NUL},
};

static MenuItem current_items[] = {
  {(XtArgVal)NULL, (XtArgVal)OpenAndClear, (XtArgVal)NUL, (XtArgVal)True, 
     (XtArgVal)True, (XtArgVal)NUL},
  {(XtArgVal)NULL, (XtArgVal)FilePopdown, (XtArgVal)NUL, (XtArgVal)True,
     (XtArgVal)True, (XtArgVal)NUL},
};
static MenuItem open_save_items[] = {
  {(XtArgVal)NULL, (XtArgVal)OpenAndClear, (XtArgVal)NUL, (XtArgVal)True, 
     (XtArgVal)True, (XtArgVal)NUL},
  {(XtArgVal)NULL,   (XtArgVal)SaveFull, (XtArgVal)NUL, (XtArgVal)True,
     (XtArgVal)True, (XtArgVal)NUL},
};

static Menu menu = {
  "File",
  file_items,
  XtNumber(file_items),
  True,
  OL_FIXEDCOLS,
  OL_NONE
};
  

static Menu file_menu = {
	"file",
	current_items,
	XtNumber(current_items),
	False,
	OL_FIXEDROWS,
	OL_NONE
};


#define GETMESS(a,b)	 OlGetMessage(dsp, NULL, 0, \
		     OleNfixedString, \
		     a,  \
		     OleCOlClientOlpixmapMsgs, \
		     b, \
		     (XrmDatabase)NULL)

void
SetFileLabels(dsp)
  Display *dsp;
{
  Cardinal n = 0;
Labels[n++] =
	GETMESS(OleTsfile,OleMfixedString_sfile);
Labels[n++] =
	GETMESS(OleTfileName,OleMfixedString_fileName);
}


#define GETMNEM(a,b)	 OlGetMessage(dsp, NULL, 0, \
		     OleNmnemonic, \
		     a,  \
		     OleCOlClientOlpixmapMsgs, \
		     b, \
		     (XrmDatabase)NULL)

Widget
SetFile(parent, button)
Widget parent;
Widget button;
{
	Widget	prompt_area;
	Widget	button_area;
	Widget	caption;
	Widget	browse;
	Widget	btn_menu;
	char	buf[BUFSIZ];
	Display *dsp = XtDisplay(parent);

	SetFileLabels(dsp);
	
	/* GetLabels for the control area buttons (open, browse, save) */
	file_items[0].label = 
			(XtArgVal)GETMESS(OleTopen,OleMfixedString_open);
	file_items[0].mnemonic = 
			 (XtArgVal)*(GETMNEM(OleTopen,OleMmnemonic_open));

	file_items[1].label = 
			(XtArgVal)GETMESS(OleTbrowse,OleMfixedString_browse);
	file_items[1].mnemonic = 
			 (XtArgVal)*(GETMNEM(OleTbrowse,OleMmnemonic_browse));

	file_items[2].label = 
			(XtArgVal)GETMESS(OleTsave,OleMfixedString_save);
	file_items[2].mnemonic = 
			 (XtArgVal)*(GETMNEM(OleTsave,OleMmnemonic_save));
	file_items[3].label = 
			(XtArgVal)GETMESS(OleTsaveAs,OleMfixedString_saveAs);
	file_items[3].mnemonic = 
			 (XtArgVal)*(GETMNEM(OleTsaveAs,OleMmnemonic_saveAs));
	btn_menu = AddMenu(parent, &menu);

	sprintf(buf, Labels[0], ApplicationName);
	INIT_ARGS();
	SET_ARGS(XtNtitle, buf);
	FilePopup = CREATE_POPUP("FilePopup", popupWindowShellWidgetClass,
								btn_menu);
	END_ARGS();
	XtAddCallback(FilePopup, XtNverify, VerifyPopdown, (XtPointer) 0);

	INIT_ARGS();
	SET_ARGS(XtNupperControlArea, &prompt_area);
	SET_ARGS(XtNlowerControlArea, &button_area);
	GET_VALUES(FilePopup);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNlabel, Labels[1]);
	SET_ARGS(XtNborderWidth, 0);
	caption = CREATE_MANAGED("caption", captionWidgetClass, prompt_area);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNstring, "");
	SET_ARGS(XtNborderWidth, 0);
	SET_ARGS(XtNwidth, OlMMToPixel(OL_HORIZONTAL, 55));
	PromptTextField = CREATE_MANAGED("input", textFieldWidgetClass,
								caption);
	END_ARGS();

	{
	  String tmp;
	  tmp = strdup(GETMESS(OleTopen,OleMfixedString_open));
	  /* to remove ... from the Open... string */
	  tmp[strlen(tmp)-3] = '\0';
	  current_items[0].label = (XtArgVal) strdup(tmp);
	  open_save_items[0].label = (XtArgVal) tmp;
	}

	current_items[1].label =
	  (XtArgVal)GETMESS(OleTcancel,OleMfixedString_cancel);
	current_items[1].mnemonic =
	  (XtArgVal)*(GETMNEM(OleTcancel,OleMmnemonic_cancel));
	current_items[0].mnemonic =
	open_save_items[0].mnemonic = 
			 (XtArgVal)*(GETMNEM(OleTopen,OleMmnemonic_open));
	open_save_items[1].label = 
			(XtArgVal)GETMESS(OleTsave,OleMfixedString_save);
	open_save_items[1].mnemonic = 
			 (XtArgVal)*(GETMNEM(OleTsave,OleMmnemonic_save));
	OpenButton = AddMenu (button_area, &file_menu);

	/*
	 *	functionality not there yet...

	if (!OlIsFMRunning(DISPLAY, SCREEN))
		XtSetSensitive(browse, False); 

	*/
	return btn_menu;
}

void
FilePopupCallback(wid, client_data, call_data)
Widget wid;
Boolean client_data;
XtPointer call_data;
{
  static Boolean lastbutton = True;

  if (client_data != lastbutton) {
    if (client_data == True) {
      OlVaFlatSetValues(OpenButton, 0,
			XtNlabel, (XtArgVal) open_save_items[0].label,
			XtNmnemonic, (XtArgVal)open_save_items[0].mnemonic,
			XtNselectProc, (XtArgVal) open_save_items[0].p,
			(String) 0);
    }
    else {
      OlVaFlatSetValues(OpenButton, 0,
			XtNlabel, (XtArgVal) open_save_items[1].label,
			XtNmnemonic, (XtArgVal)open_save_items[1].mnemonic,
			XtNselectProc, (XtArgVal) open_save_items[1].p,
			(String) 0);
    }
    lastbutton = client_data;
  }
  XtPopup(FilePopup, XtGrabNone);
  XDefineCursor(DISPLAY, XtWindow(FilePopup), OlGetStandardCursor(FilePopup));
  XRaiseWindow(DISPLAY, XtWindow(FilePopup));
}


static void
VerifyPopdown(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	*((Boolean *) call_data) &= PopdownOK;
}


static void
SetCommand()
{
	char *	command[2];

	command[0] = Argv_0;
	command[1] = Filename;

	XSetCommand(DISPLAY, XtWindow(Toplevel), command, 2);
	XFlush(DISPLAY);
}


static void
Exit()
{
	XtUnmapWidget(Toplevel);
	XCloseDisplay(DISPLAY);
	exit(0);
}


static void
HandleIOError(e)
enum IO_error e;
{
	char buf[BUFSIZ];
	char *msg;

	if (e == e_invalid)
		sprintf(buf,
			msg = OlGetMessage(DISPLAY, NULL, 0,
				     OleNfooterMsg,
				     OleTbadFormat,
				     OleCOlClientOlpixmapMsgs,
				     OleMfooterMsg_badFormat,
				     (XrmDatabase)NULL),
			Filename);
	else if (e == e_cannot_open)
		sprintf(buf,
			msg = OlGetMessage(DISPLAY, NULL, 0,
				     OleNfooterMsg,
				     OleTnoFile,
				     OleCOlClientOlpixmapMsgs,
				     OleMfooterMsg_noFile,
				     (XrmDatabase)NULL),
			Filename, sys_errlist[errno]);
	else if (e == e_cannot_write)
		sprintf(buf,
			msg = OlGetMessage(DISPLAY, NULL, 0,
				     OleNfooterMsg,
				     OleTnoPermission,
				     OleCOlClientOlpixmapMsgs,
				     OleMfooterMsg_noPermission,
				     (XrmDatabase)NULL),
			Filename, sys_errlist[errno]);
	else
		sprintf(buf,
			msg = OlGetMessage(DISPLAY, NULL, 0,
				     OleNfooterMsg,
				     OleTnoMemory,
				     OleCOlClientOlpixmapMsgs,
				     OleMfooterMsg_noMemory,
				     (XrmDatabase)NULL));
	FooterMessage(buf, True);
}


/*
 *	WriteOutput returns True if output successfully written, False if not
 */
static Bool
WriteOutput()
{
	int		condition;
	Bool		retval = False;
	char		*msg;

	SetStatus(Busy);
	condition = XWritePixmapFile(DISPLAY, PixmapColormap, Filename,
					RealPixmap, PixmapWidth, PixmapHeight);
	SetStatus(Normal);

	switch (condition) {

	case PixmapOpenFailed:
	case PixmapFileInvalid:
		HandleIOError(e_cannot_write);
		break;

	case PixmapNoMemory:
		HandleIOError(e_no_memory);
		break;

	case PixmapSuccess:
		msg = OlGetMessage(DISPLAY, NULL, 0,
			     OleNfooterMsg,
			     OleTsave,
			     OleCOlClientOlpixmapMsgs,
			     OleMfooterMsg_save,
			     (XrmDatabase)NULL);
		FooterMessage(msg, False);
		retval = True;
		break;

	default:
		msg = OlGetMessage(DISPLAY, NULL, 0,
			     OleNfooterMsg,
			     OleTerror,
			     OleCOlClientOlpixmapMsgs,
			     OleMfooterMsg_error,
			     (XrmDatabase)NULL);
		FooterMessage(msg, True);
		break;
	}

	if (retval)
		Changed = False;
	return (retval);
}


static void
SaveFull(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	if (Filename)
		XtFree(Filename);
	INIT_ARGS();
	SET_ARGS(XtNstring, &Filename);
	GET_VALUES(PromptTextField);
	END_ARGS();

	PopdownOK = WriteOutput();
}


static Bool
ReadPixmapFile(name)
char * name;
{
	int		condition;
	unsigned int	width, height;
	Pixmap		new_pixmap;

	condition = XReadPixmapFile(DISPLAY, ROOT, PixmapColormap, name,
			&width, &height, PixmapDepth, &new_pixmap);
	if (condition != PixmapSuccess) {
#ifndef NO_XMU
		int	hot;
		Pixmap	bitmap;

		if (XReadBitmapFile(DISPLAY, ROOT, name, &width, &height,
				&bitmap, &hot, &hot) == BitmapSuccess)
		{
			Pixel fg, bg;

			if (CurrentForeground != CurrentBackground) {
				fg = CurrentForeground;
				bg = CurrentBackground;
			} else {
				fg = BlackPixelOfScreen(SCREEN);
				bg = WhitePixelOfScreen(SCREEN);
			}
			new_pixmap = XmuCreatePixmapFromBitmap(DISPLAY, ROOT,
				bitmap, width, height, PixmapDepth, fg, bg);
			XFreePixmap(DISPLAY, bitmap);
			condition = PixmapSuccess;
		}
		else
#endif /* NO_XMU */
		{
			return condition;
		}
	}

	ResetAllVisuals(new_pixmap, (Dimension)width, (Dimension)height);
	return PixmapSuccess;
}


/*
 *	ReadInput returns True if input successfully read, False if not
 */
static Bool
ReadInput()
{
	int		condition;
	Bool		retval = False;
	struct stat	statbuf;
	char		*msg;

	SetStatus(Busy);
	condition = ReadPixmapFile(Filename);
	SetStatus(Normal);

	switch (condition) {

	case PixmapFileInvalid:
		HandleIOError(e_invalid);
		break;

	case PixmapNoMemory:
		HandleIOError(e_no_memory);
		break;

	case PixmapOpenFailed:
		if (stat(Filename, &statbuf) == 0) {
			/*
			 *	File exists, but we can't read it.
			 */
			HandleIOError(e_cannot_open);
		} else {
			ResetAllVisuals((Pixmap)0, DEFAULT_WIDTH,
							DEFAULT_HEIGHT);
			msg = OlGetMessage(DISPLAY, NULL, 0,
				     OleNfooterMsg,
				     OleTnew,
				     OleCOlClientOlpixmapMsgs,
				     OleMfooterMsg_new,
				     (XrmDatabase)NULL);
			FooterMessage(msg, False);
			retval = True;
		}
		break;

	case PixmapSuccess:
		msg = OlGetMessage(DISPLAY, NULL, 0,
			     OleNfooterMsg,
			     OleTopen,
			     OleCOlClientOlpixmapMsgs,
			     OleMfooterMsg_open,
			     (XrmDatabase)NULL);
		FooterMessage(msg, False);
		retval = True;
		break;

	default:
		msg = OlGetMessage(DISPLAY, NULL, 0,
			     OleNfooterMsg,
			     OleTerror,
			     OleCOlClientOlpixmapMsgs,
			     OleMfooterMsg_error,
			     (XrmDatabase)NULL);
		FooterMessage(msg, True);
		break;
	}

	if (retval) {
		Changed = False;
		SetCommand();
	}
	return (retval);
}


static void
OpenAndClear(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	if (Filename && Changed) {
		HandleCurrentContents(OpenButton, True);
		PopdownOK = False;
	} else {
		if (Filename)
			XtFree(Filename);
		INIT_ARGS();
		SET_ARGS(XtNstring, &Filename);
		GET_VALUES(PromptTextField);
		END_ARGS();


		PopdownOK = ReadInput();
	}
}


Bool
OpenFile(name)
char * name;
{
	INIT_ARGS();
	SET_ARGS(XtNstring, name);
	SET_VALUES(PromptTextField);
	END_ARGS();

	OpenAndClear((Widget) NULL, (XtPointer) 0, (XtPointer) 0);
	return (PopdownOK);
}


static void
SaveOld(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	Bool	do_open = (Bool) client_data;

	if (WriteOutput() && do_open) {
		OpenAndClear((Widget) NULL, (XtPointer) 0, (XtPointer) 0);
		if (PopdownOK)
			BringDownPopup(FilePopup);
	}

	if (!do_open)
		Exit();
}


static void
DiscardOld(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	Bool	do_open = (Bool) client_data;

	Changed = False;		/* discard changes */
	if (do_open) {
		OpenAndClear((Widget) NULL, (XtPointer) 0, (XtPointer) 0);
		if (PopdownOK)
			BringDownPopup(FilePopup);
	} else {
		Exit();
	}
}


static void
CancelOpen(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	char *msg;
	/*
	 *	Do absolutely nothing - useful in case the user has
	 *	a change of heart or has hit Open instead of Save.
	 *
	 *	Existing work remains, and no files are read or written.
	 */
  msg = OlGetMessage(XtDisplay(Toplevel), NULL, 0,
	       OleNfooterMsg,
	       OleTcancelOpen,
	       OleCOlClientOlpixmapMsgs,
	       OleMfooterMsg_cancelOpen,
	       (XrmDatabase)NULL);
  FooterMessage(msg, False);
}


static MenuItem cancel_items [] = {
  {(XtArgVal)NULL,    (XtArgVal)SaveOld,    (XtArgVal)NUL,
		(XtArgVal)True, (XtArgVal)True, (XtArgVal)NUL, (XtArgVal)'S'},
  {(XtArgVal)NULL, (XtArgVal)DiscardOld, (XtArgVal)NUL,
		(XtArgVal)True, (XtArgVal)True, (XtArgVal)NUL, (XtArgVal)'D'},
  {(XtArgVal)NULL,  (XtArgVal)CancelOpen, (XtArgVal)NUL,
		 (XtArgVal)True, (XtArgVal)True, (XtArgVal)NUL, (XtArgVal)'C'},
};

static Menu cancel_menu = {
	"cancel",
	cancel_items,
	XtNumber(cancel_items),
	False,
	OL_FIXEDROWS,
	OL_NONE
};

#define GETMESS(a,b)	 OlGetMessage(dsp, NULL, 0, \
		     OleNfixedString, \
		     a,  \
		     OleCOlClientOlpixmapMsgs, \
		     b, \
		     (XrmDatabase)NULL)

#define GETMNEM(a,b)	 OlGetMessage(dsp, NULL, 0, \
		     OleNmnemonic, \
		     a,  \
		     OleCOlClientOlpixmapMsgs, \
		     b, \
		     (XrmDatabase)NULL)

static void
HandleCurrentContents(emanate, do_open)
Widget emanate;
Bool do_open;
{
	static Widget	cancel_buttons;
	char *msg;

	if (FileNotice == (Widget) NULL) {
		Widget	text_area,
			control_area;
		Display *dsp = XtDisplay(emanate);

		INIT_ARGS();
		FileNotice = CREATE_POPUP("FileNotice", noticeShellWidgetClass,
								Toplevel);
		END_ARGS();

		INIT_ARGS();
		SET_ARGS(XtNtextArea, &text_area);
		SET_ARGS(XtNcontrolArea, &control_area);
		GET_VALUES(FileNotice);
		END_ARGS();

		INIT_ARGS();
		msg = OlGetMessage(XtDisplay(Toplevel), NULL, 0,
						 OleNfooterMsg,
						 OleTnoSave,
						 OleCOlClientOlpixmapMsgs,
						 OleMfooterMsg_noSave,
						 (XrmDatabase) NULL);
		SET_ARGS(XtNstring, msg);
		SET_ARGS(XtNborderWidth, 0);
		SET_VALUES(text_area);
		END_ARGS();
	/* Get labels for  cancel control area  - save, discard, cancel */
		cancel_items[0].label = 
			(XtArgVal)GETMESS(OleTsave,OleMfixedString_save);
		cancel_items[0].mnemonic =
			 (XtArgVal)*(GETMNEM(OleTsave,OleMmnemonic_save));

		cancel_items[1].label = 
			(XtArgVal)GETMESS(OleTdiscard,OleMfixedString_discard);
		cancel_items[1].mnemonic = 
			 (XtArgVal)*(GETMNEM(OleTdiscard,OleMmnemonic_discard));

		cancel_items[2].label = 
			(XtArgVal)GETMESS(OleTcancel,OleMfixedString_cancel);
		cancel_items[2].mnemonic = 
			 (XtArgVal)*(GETMNEM(OleTcancel,OleMmnemonic_cancel));

		cancel_buttons = AddMenu (control_area, &cancel_menu);
	}

	cancel_items[0].data =
	cancel_items[1].data =
	cancel_items[2].data = (XtArgVal)do_open;
	XtVaSetValues (
		cancel_buttons,
		XtNnumItems,	 (XtArgVal)do_open ? 3 : 2,
		XtNitemsTouched, (XtArgVal)True,
		(String)0
	);
	INIT_ARGS();
	SET_ARGS(XtNemanateWidget, emanate);
	SET_VALUES(FileNotice);
	END_ARGS();

	XtPopup(FileNotice, XtGrabExclusive);
	XDefineCursor(DISPLAY, XtWindow(FileNotice),
						OlGetStandardCursor(FileNotice));
}


void
WindowManagerEventHandler(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlWMProtocolVerify *	p = (OlWMProtocolVerify *)call_data;

	switch (p->msgtype) {
	case OL_WM_DELETE_WINDOW:
		if (Filename && Changed) {
			HandleCurrentContents(Toplevel, False);
		} else {
			Exit();
		}
		break;

	case OL_WM_SAVE_YOURSELF:
		/*
		 *	Do nothing for now; just respond.
		 */
		SetCommand();
		break;

	default:
		OlWMProtocolAction(wid, p, OL_DEFAULTACTION);
		break;
	}
}

#define OPEN	request.open_folder

static void
RequestDesktopManager()
{
	DtRequest request;
	char buf[BUFSIZ], *tmpenv;

	tmpenv = getenv("XWINHOME");

	sprintf(buf, "%s%s", tmpenv, PIXMAPLOC);
	if (access(buf, 04) != 0) {
		tmpenv = getenv("HOME");
		sprintf(buf, "%s", tmpenv);
	}

	memset(&request, 0, sizeof(request));
	OPEN.rqtype	  = DT_OPEN_FOLDER;
	
	OPEN.title	   = buf;
	OPEN.options	   = DT_ICONIC_VIEW;
	OPEN.path	   = buf;
	OPEN.pattern	   = PATTERN;

	(void) DtEnqueueRequest(XtScreen(Canvas), _DT_QUEUE(XtDisplay(Canvas)),
				_DT_QUEUE(XtDisplay(Canvas)),
				XtWindow(Canvas), &request); 
	PopdownOK = False;
}

#undef OPEN

static void
FilePopdown(w, cli, call)
  Widget w;
  XtPointer cli, call;
{
  XtPopdown((Widget)_OlGetShellOfWidget(w));
}

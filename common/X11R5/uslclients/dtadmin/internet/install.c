/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:internet/install.c	1.9"
#endif

/* Popup Controls for installing network use icons */
#include <pwd.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <PopupWindo.h>
#include <Caption.h>
#include <TextField.h>
#include <FButtons.h>
#include <MenuShell.h>
#include "inet.h"
#include "error.h"


extern void		NotifyUser ();
extern void             DisallowPopdown();
extern void             HelpCB();
extern void		InstallCB();

static void		MakeSoftLink(Widget, char *, char *);
static void		InstallFriendNode();
static void		Cancel();
static char		*user_home=NULL;

extern char *		ApplicationName;


static Widget           PopupWindow;
static Widget           installTextField;

static HelpText AppHelp = {
    title_hinstall, HELP_FILE, help_install,
};

static Items installItems [] = {
    { InstallFriendNode, NULL, (XA)TRUE}, /* Install */
    { Cancel, NULL, (XA)TRUE},                 /* Cancel */
    { HelpCB, NULL, (XA)TRUE, NULL, NULL, (XA)&AppHelp},
};

static Menu installMenu = {
	"install",
	installItems,
	XtNumber (installItems),
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
	XtPopdown(PopupWindow);
}

void
InstallCB (widget, client_data, call_data)
Widget widget;
XtPointer client_data;
XtPointer call_data;
{
	Widget      button_area;
	Widget      prompt_area;
	Widget      caption;
	static char buf[128];

	/* If first time, create the popup and its controls */
	if (!PopupWindow) {
		SET_HELP(AppHelp);
		sprintf(buf, "%s: %s", ApplicationName, GGT(title_install));
		PopupWindow = XtVaCreatePopupShell ("install",
			popupWindowShellWidgetClass, widget,
			XtNpushpin,             (XtArgVal) OL_NONE,
			XtNtitle,		buf,
			0);

		XtVaGetValues (PopupWindow,
			XtNlowerControlArea,    (XtArgVal) &button_area,
			XtNupperControlArea,    (XtArgVal) &prompt_area,
			0);

		XtAddCallback (
			PopupWindow,
			XtNverify,
			DisallowPopdown,
			(XtPointer)0
		);

		caption = XtVaCreateManagedWidget(
			"caption",
			captionWidgetClass,
			prompt_area,
			XtNlabel, GGT(label_location),
			XtNborderWidth, 0,
			(String) 0
		);

		sprintf(buf, "%s", hf->userHome);
		installTextField = XtVaCreateManagedWidget(
			"input",
			textFieldWidgetClass,
			caption,
			XtNstring, buf,
			XtNborderWidth, 0,
			XtNcharsVisible, 20,
			(String) 0
		);

		SET_LABEL (installItems,0,minstall);
		SET_LABEL (installItems,1,cancel);
		SET_LABEL (installItems,2,help);
		AddMenu (button_area, &installMenu, False);
	}

	XtPopup (PopupWindow, XtGrabExclusive);
} /* InstallCB */

/*
 *
 * Install Network use icons (friend nodes) in the control room
 */
static void
InstallFriendNode (widget, client_data, call_data)
Widget widget;
XtPointer client_data;
XtPointer call_data;
{
	FILE 			*attrp;
	HostData		*dp = NULL;
	static char 		node_directory[PATH_MAX];
	char 			target[PATH_MAX];
	static Boolean		first_time = True;
	struct stat		stat_buf;
	int			index, num;

	/* Nothing to operate on, just return */
	if (hf->numFlatItems == 0) { /* nothing to operate */
		PUTMSG(GGT(string_noItem));
		return;
	}
	/* Get the select host */
	dp = hf->flatItems[hf->currentItem].pField;

	if (dp == NULL) { 
		PUTMSG(GGT(string_noSelect));
		return;
	}

	/* check to see if the local machine has been selected */
	if (strcmp(dp->f_name, hf->nodeName) == 0) {
		PUTMSG(GGT(string_sameNode));
		return;
	}
	if (first_time) {
		first_time = False;
		sprintf (node_directory, "%s/.node", hf->userHome);
	}
	/* check the node directory is there or not. */
	/* if not, then create it			*/

	if (!DIRECTORY(node_directory, stat_buf) ) {
		if (mkdir(node_directory, DMODE) == -1) {
			PUTMSG(GGT(string_noAccessNodeDir));
			return;
		}
		if (chown(node_directory, getuid(), getgid()) == -1) {
			PUTMSG(GGT(string_noAccessNodeDir));
			return;
		}
	} else
	    if (access(node_directory, W_OK) < 0) {
		    PUTMSG(GGT(string_noAccessNodeDir));
		    return;
	    }

#ifdef debug
	fprintf(stderr,"the MODE is: %o\n", DMODE);
	fprintf(stderr,"the mode for %s is: %o\n", node_directory,
						stat_buf.st_mode);
#endif
	/* Get the select host */
	sprintf (target, "%s/%s", node_directory, dp->f_name);

	attrp = fopen( target, "w");
	if (attrp == (FILE *) 0) {
		PUTMSG(GGT(string_noAccessNodeDir));
		return;
	}

	/* put the node's properties here */
	fprintf( attrp, "DUSER=%s\nDPATH=\n",
		hf->userName);

	(void) fflush(attrp);
	fclose( attrp );

	if (chmod( target, MODE) == -1) {
		PUTMSG(GGT(string_noAccessNodeDir));
		return;
	}
	if (chown( target, getuid(), getgid()) == -1) {
		PUTMSG(GGT(string_noAccessNodeDir));
		return;
	}

	MakeSoftLink(widget, dp->f_name, target);
	XtPopdown(PopupWindow);

} /* InstallFriendNode() */

static void
MakeSoftLink(Widget wid, char *fname, char *target)
{
	DtRequest	request;
	char	 	msg[BUFSIZ];
	char	 	link[PATH_MAX];
	char		*toolbox, *tp;
	int		len;

	XtVaGetValues (
		installTextField,
		XtNstring, &toolbox,
		(String)0
	);
#ifdef debug
	printf ("Toolbox = %s\n", toolbox);
#endif

	/* strip off leading spaces if any */
	tp = toolbox;
	while (*tp == ' ') tp++;
	len = strlen(tp);
	if (len == 0 || (*tp) != '/') /* relative assume */
		sprintf (link, "%s/%s/%s", hf->userHome, tp, fname);
	else
		sprintf (link, "%s/%s", tp, fname);
	XtFree(toolbox);

	errno = 0;
	if(symlink( target, link) == -1) {
		switch (errno) {
		case EACCES:
		case ENOENT:
			sprintf(msg, GGT(string_pathDeny),
				link);
			break;
		case EEXIST:
			sprintf(msg, GGT(string_pathExist),
				fname);
			break;
		case ENAMETOOLONG:
			sprintf(msg, GGT(string_longFilename),
				fname);
			break;
		default:
			sprintf(msg, GGT(string_installFailed),
				link);
			break;
		}
		NotifyUser(hf->toplevel, msg);
		return;
	} else {
		if (lchown( link, getuid(), getgid()) == -1) {
			PUTMSG(GGT(string_noAccessNodeDir));
			return;
		}
		sprintf(msg, GGT(string_installDone),
			fname);
		PUTMSG(msg);
	}

	memset (&request, 0, sizeof (request));
	request.sync_folder.rqtype= DT_SYNC_FOLDER;
	request.sync_folder.path = link;
	(void) DtEnqueueRequest (XtScreen (wid),
			_DT_QUEUE(XtDisplay(wid)),
			_DT_QUEUE(XtDisplay(wid)),
			XtWindow (wid), &request);

} /* MakeSoftLink */

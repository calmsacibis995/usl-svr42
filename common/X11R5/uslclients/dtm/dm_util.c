/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:dm_util.c	1.71"

/******************************file*header********************************

    Description:
	This file contains the source code for utility-type functions which are
	shared between the different components of dtm (Folders, Wastebasket,
	etc).
*/
						/* #includes go here	*/
#include <fmtmsg.h>
#include <limits.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLookP.h>		/* The "P" for _OlBeepDisplay */
#include <Xol/OlCursors.h>
#include <Xol/Footer.h>
#include <Gizmo/Gizmos.h>		/* for GetGizmoText */
#include <Gizmo/PopupGizmo.h>		/* for SetPopupMessage */
#include <Gizmo/BaseWGizmo.h>		/* for SetBaseWindowStatus */
#include <Gizmo/MenuGizmo.h>
#include <memutil.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/

static void	VDisplayMsg(DmWinPtr, int, char *, va_list);

					/* public procedures		*/

void		DmBusyWindow(Widget, Boolean, XtCallbackProc, XtPointer);
int		DmDropCommand(DmWinPtr, DmObjectPtr, char *);
void		DmDropObject(DmWinPtr, Cardinal, DmWinPtr, DmObjectPtr);
int		DmExecCommand(DmWinPtr, DmObjectPtr, char *, char *);
int		DmExecuteShellCmd(DmWinPtr, DmObjectPtr, char *, Boolean);
char *		DmMakeFolderTitle(DmFolderWindow window);
int		DmObjectToIndex(DmWinPtr wp, DmObjectPtr op);
DmItemPtr	DmObjectToItem(DmWinPtr wp, DmObjectPtr op);
DmItemPtr	DmObjNameToItem(DmWinPtr win, register char * name);
void		DmOpenObject(DmWinPtr wp, DmObjectPtr op);
DmFolderWindow	DmQueryFolderWindow(char * path);
int		DmSameOrDescendant(char * path1, char * path2, int path1_len);
void		DmTouchIconBox(DmWinPtr window, ArgList, Cardinal);
void		DmUpdateFolderTitle(DmFolderWindow);
void		DmVaDisplayState(DmWinPtr window, char * msg, ...);
void		DmVaDisplayStatus(DmWinPtr window, int type, char * msg, ...);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/
static const char DFLT_OPEN_CMD[] = "##DROP(dtedit) || exec dtedit %F &";

#define FOLDERNAME "name"

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    VDisplayMsg- display message 'msg' in footer of 'window'

	'msg_type' is currently only True/False to indicate whether the
	message belongs in the "Status/Error" (left) part of the footer
	(True) or whether it belongs in the "State" (right) part of the
	footer (False).

	NOTE:  Since there is currently	no generic way to put a meesage in
	a footer, the type of gizmo can be determined by window->attrs.  If
	the window is a Command window, use SetPopupMessage, etc.
*/
static void
VDisplayMsg(DmWinPtr window, int msg_type, char * msg, va_list ap)
{
    const char	*lmsg;		/* localized msg */
    char	buffer[1024];

    if (msg == NULL)			/* assume NULL msg means clear */
    {
	if ( !(window->attrs | DM_B_SHOWN_MSG) )
	    return;			/* already clear */

	window->attrs &= ~DM_B_SHOWN_MSG;
	buffer[0] = '\0';		/* make message a NULL string */

    } else
    {
	window->attrs |= DM_B_SHOWN_MSG;

	lmsg = Dm__gettxt(msg);		/* first I18N'ize string */
	vsprintf(buffer, lmsg, ap);
    }

    if (window->attrs & DM_B_BASE_WINDOW)
    {
	if (msg_type == False)
		SetBaseWindowStatus(window->gizmo_shell, buffer);
	else
		SetBaseWindowMessage(window->gizmo_shell, buffer);
    } else
	SetPopupMessage(window->gizmo_shell, buffer);

}				/* end of VDisplayMsg */


/***************************private*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    DmBusyWindow-
*/
void
DmBusyWindow(Widget w, Boolean state, XtCallbackProc stop_proc,
	     XtPointer stop_data)
{
    Display *dpy = XtDisplay(w);
    Window   win = XtWindow(w);

    /* set busy state on window title */
    XtSetArg(Dm__arg[0], XtNbusy, state);
    XtSetValues(w, Dm__arg, 1);

    if (state == True)
    {
	XGrabButton(dpy, AnyButton, AnyModifier, win, False,
		    ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
		    PointerMotionMask | EnterWindowMask | LeaveWindowMask,
		    GrabModeAsync, GrabModeAsync, (Window)None,
		    (Cursor)OlGetBusyCursor(w));

	XGrabKey(dpy, AnyKey, AnyModifier, win, False,
		 GrabModeAsync,GrabModeAsync);

	if (stop_proc != NULL)
	    OlAddCallback(w, XtNconsumeEvent, stop_proc, stop_data);
    } else
    {
	XUngrabButton(dpy, AnyButton, AnyModifier, win);
	XUngrabKey(dpy, AnyKey, AnyModifier, win);

	if (stop_proc != NULL)
	    OlRemoveCallback(w, XtNconsumeEvent, stop_proc, stop_data);

    }
}				/* End of DmBusyWindow */

/****************************procedure*header*****************************
    DmDropCommand-
*/
int
DmDropCommand(DmWinPtr wp, DmObjectPtr op, char * name)
{
	Display *dpy = XtDisplay(wp->box);
	Atom app_id = XInternAtom(dpy, name, False);
	Window app_win;

	if ((app_id != None) &&
	    ((app_win = DtGetAppId(dpy, app_id)) != None)) {
		XWindowAttributes win_attrs;
		OlDnDDragDropInfo root_info;

		/*
		 * We have to get the root_window id. Sigh.
		 */
		XGetWindowAttributes(dpy, app_win, &win_attrs);

		root_info.root_window = win_attrs.root;
		root_info.root_x      = 0;
		root_info.root_y      = 0;
		root_info.drop_timestamp = CurrentTime;

	    	if (DmDnDNewTransaction((DmWinPtr)wp,
			(DmItemPtr *)DmOneItemList(DmObjectToItem(wp, op)),
			DM_B_SEND_EVENT,
			&root_info,
			app_win,
			OlDnDTriggerCopyOp,
			DmConvertSelectionProc,
			DmTransactionStateProc))
			return(1);
	}

	return(0);
}				/* end of DmDropCommand */

/****************************procedure*header*****************************
    DmDropObject-
*/
void
DmDropObject(DmWinPtr dst_win, Cardinal dst_indx,
	     DmWinPtr src_wp, DmObjectPtr src_op)
{
    char *	p;
    DmObjectPtr	dst_op;

    if (ITEM_BUSY(DM_WIN_ITEM(dst_win, dst_indx)))
	return;				/* Return now if dst item is busy */

    /* busy the dst item */
    XtSetArg(Dm__arg[0], XtNbusy, True);
    OlFlatSetValues(dst_win->box, dst_indx, Dm__arg, 1);
    OlUpdateDisplay(dst_win->box);	/* make it look busy immediately */

    /* execute drop command (if any) */
    dst_op = ITEM_OBJ(DM_WIN_ITEM(dst_win, dst_indx));
    if ( (p = DmGetObjProperty(dst_op, DROPCMD, NULL)) != NULL )
    {
	DmSetSrcWindow(src_wp);
	DmSetSrcObject(src_op);
	p = Dm__expand_sh(p, DmDropObjProp, (XtPointer)dst_op);
        if (getenv("DTDEBUG"))
		printf("dropcmd=%s\n", p);

	/*
	 * set force_chdir to True only if the source window is a
	 * folder window.
	 */
	if (src_wp->attrs & DM_B_FOLDER_WIN)
		(void)DmExecuteShellCmd(src_wp, src_op, p, True);
	else
		(void)DmExecuteShellCmd(src_wp, src_op, p, False);

	FREE(p);

    } else
    {
	/* put the default behavior here */
    }

    /* un-busy the dst item */
    XtSetArg(Dm__arg[0], XtNbusy, False);
    OlFlatSetValues(dst_win->box, dst_indx, Dm__arg, 1);

}				/* end of DmDropObject */

/****************************procedure*header*****************************
    DmExecCommand-
*/
int
DmExecCommand(DmWinPtr wp, DmObjectPtr op, char * name, char * str)
{
    if (getenv("DTDEBUG"))
	printf("exec cmd: name=:%s: str=:%s:\n", name, str);
    return(DmDispatchRequest(wp->box,
			     XInternAtom(XtDisplay(wp->box), name, False),
			     str));

}				/* end of DmExecuteCommand */

/****************************procedure*header*****************************
    DmExecuteShellCmd-
*/
int
DmExecuteShellCmd(DmWinPtr wp, DmObjectPtr op, char * cmdstr,
		  Boolean force_chdir)
{

#define FINDCHAR(P,C)	while (*(P) != C) (P)++
#define EATSPACE(P,C)	while (*(P) == C) (P)++

#define CMD_PREFIX	"##"
#define CMD_PREFIX_LEN	2

	char *p2;
	char *name;
	int namelen;
	char *cmd;
	char *cmd_end;
	char *selection;
	int ret = 0;
	int found;
	int done;

loop:
	done = 0;
	EATSPACE(cmdstr, ' ');
	if (*cmdstr == '\0')
		return(ret);

	if (!strncmp(cmdstr, CMD_PREFIX, CMD_PREFIX_LEN)) {
		/* special command */
		name = cmdstr + CMD_PREFIX_LEN;

		/* get name */
		for (p2=name; *p2 && (*p2 != '('); p2++);
		namelen = p2 - name;
		p2++; /* skip '(' */

		/* get end of special command */
		for (cmd=p2; *p2 && (*p2 != ')'); p2++) {
			/* handle matching quotes inside parenthesis */
			if (*p2 == '"') {
				FINDCHAR(p2, '"');
			}
			if (*p2 == '\'') {
				FINDCHAR(p2, '\'');
			}
		}
		cmd_end = p2;

		name = (char *)strndup(name, namelen);
		if (!strcmp(name, "DROP")) {
			if (wp) {
				selection = (char *)strndup(cmd, cmd_end - cmd);
				ret = DmDropCommand(wp, op, selection);
				FREE(selection);
			}
			else {
printf("##DROP and wp==NULL\n");
				FREE(name);
				return(-1);
			}
		}
		else if (!strcmp(name, "DELETE")) {
			if (wp)
				DmMoveToWBProc2(DmObjPath(op),NULL,NULL,NULL);
			else
				DmMoveToWBProc2((char *)op,NULL,NULL,NULL);
			ret = 1;
		}
		else if (!strcmp(name, "COMMAND")) {
			if (wp) {
				char *comma = cmd;
				char *str;

				/* find the comma */
				while ((comma < cmd_end) &&
					(*comma != ',')) comma++;
				if (*comma != ',') {
					Dm__VaPrintMsg(TXT_MISSING_COMMA, cmd);
					return(-1);
				}

				selection = (char *)strndup(cmd, comma - cmd);
				str = (char *)strndup(comma+1, cmd_end-comma-1);
				ret = DmExecCommand(wp, op, selection, str);
				FREE(str);
				FREE(selection);
			}
			else {
printf("##COMMAND and wp==NULL\n");
				FREE(name);
				return(-1);
			}
		}
		else {
			FREE(name);
			Dm__VaPrintMsg(TXT_BAD_NAME, name);
			return(-1);
		}
		FREE(name);

		cmdstr = cmd_end;
		if (cmdstr)
			cmdstr++; /* skip ')' */
		EATSPACE(cmdstr, ' ');
		done = 1;
	} /* !strncmp(cmdstr, CMD_PREFIX, CMD_PREFIX_LEN))  */

	/* find end of command ";##", "|| ##", or "&& ##" */
	for (p2=cmdstr, found=0; *p2 && !found; p2++) {
		switch(*p2) {
		case '"':
			FINDCHAR(p2, '"');
			break;
		case '\'':
			FINDCHAR(p2, '\'');
			break;
		case ';':
			p2++;
			cmd_end = p2;
			EATSPACE(p2, ' ');
			if ((*p2 == '#') && (*(p2 + 1) == '#')) {
				found++;
				break;
			}
			break;
		case '|':
		case '&':
			if (*(p2 + 1) == *p2) {
				cmd_end = p2;
				p2++;
				p2++;
				EATSPACE(p2, ' ');

				if (done) {
					found++;
					break;
				}

				if ((*p2 == '#') && (*(p2 + 1) == '#')){
					found++;
					break;
				}
			}
		}
	}

	if (!found)
		cmd_end = p2;

	if (!done) {
		cmd = (char *)strndup(cmdstr, cmd_end - cmdstr);
		if (getenv("DTDEBUG"))
			printf("system(%s)\n", cmd);
		if (force_chdir || (op->ftype == DM_FTYPE_DATA))
			chdir(op->container->path);
		switch (fork()) {
		case 0:
			signal(SIGCLD,SIG_DFL);
			signal(SIGHUP,SIG_DFL);
			signal(SIGTERM,SIG_DFL);
			signal(SIGINT,SIG_DFL);
			(void) execl ("/sbin/sh", "sh","-c",cmd, (String)0);
			exit(errno);
		case -1:
			ret = errno;
			break;
		default:
			/* For now, always assume success */
			ret = 1;
			/* (void) wait(&ret); */
		}
		FREE(cmd);
		if (force_chdir || (op->ftype == DM_FTYPE_DATA))
			chdir(DESKTOP_CWD(Desktop));
	}

	if (found) {
		cmdstr = cmd_end;
		switch(*cmd_end) {
		case ';':
			cmdstr++;
			break;
		case '|':
			if (ret)
				return(ret);
			else {
				/* skip "||" */
				cmdstr++;
				cmdstr++;
			}
			break;
		case '&':
			if (ret) {
				/* skip "&&" */
				cmdstr++;
				cmdstr++;
			}
			else
				return(ret);
			break;
		case '\0':
			/* done */
			return(ret);
		default:
			Dm__VaPrintMsg(TXT_SHELL_SYNTAX, cmdstr);
			return(-1);
		}

		goto loop;
	}

	return(ret);
}					/* end of DmExecuteShellCmd */

/****************************procedure*header*****************************
    Dm__gettxt-
*/
char *
Dm__gettxt(char * msg)
{
	static char msgid[6 + 10] = "dtmgr:";

	strcpy(msgid + 6, msg);
	return(gettxt(msgid, msg + strlen(msg) + 1));
}

char *
DmGetFolderName(DmFolderWindow window)
{
    char *name = DtGetProperty(&(window->cp->plist), FOLDERNAME, NULL);

    if (name)
	name = GetGizmoText(name);
    else
	name = DM_WIN_PATH(window);
    return(name);
}

/****************************procedure*header*****************************
    DmMakeFolderTitle- makes folder title given 'path'.
	If "first" folder, give it special product name title.
	Otherwise, title is full path or basename depending on current option.
	Note: result is "constant" string or returned in Dm__buffer.
*/
char *
DmMakeFolderTitle(DmFolderWindow window)
{
    static char *	prefix = NULL;
    char *name = DtGetProperty(&(window->cp->plist), FOLDERNAME, NULL);
    char *path = DM_WIN_PATH(window);

    if (!prefix)
    {
	char *logname;
	char *prodname;
	char *buffer;

        /* First Time: also assume this is the main window. */
	prefix = Dm__gettxt(TXT_FOLDER_PREFIX);
	prodname = Dm__gettxt(TXT_PRODUCT_NAME);
	if ((logname = cuserid(NULL)) == NULL)
		logname = "???";

	/* 4 is " - " + NULL char */
	buffer = malloc(strlen(logname) + strlen(prodname) + 4);
	sprintf(buffer, "%s - %s", prodname, logname);

	/* NOTE: buffer is never freed */
	return(buffer);
    }

    strcpy(Dm__buffer, prefix);		/* Copy in prefix */

    /* Now add path part. Note: "/" is special since basename("/") "fails". */
    if (!name)
	name = (SHOW_FULL_PATH(Desktop) || ROOT_DIR(path)) ?
		  path : basename(path);
    else
	name = GetGizmoText(name);
	
    strcat(Dm__buffer, name);

    return(Dm__buffer);
}					/* end of DmMakeFolderTitle */

/****************************procedure*header*****************************
    DmObjectToIndex-
*/
int
DmObjectToIndex(DmWinPtr wp, DmObjectPtr op)
{
    int			num_items = wp->nitems;
    register DmItemPtr	ip;
    register int	i;

    for (i = num_items, ip = wp->itp; i ; i--, ip++) {
	if ((ITEM_MANAGED(ip) != False) && (ITEM_OBJ(ip) == op))
	    return(i);
    }
    return(OL_NO_ITEM);
}				/* End of DmObjectToIndex */

/****************************procedure*header*****************************
    DmObjectToItem-
*/
DmItemPtr
DmObjectToItem(DmWinPtr wp, DmObjectPtr op)
{
    int			num_items = wp->nitems;
    register DmItemPtr	ip;
    register int	i;

    for (i = num_items, ip = wp->itp; i ; i--, ip++) {
	if ((ITEM_MANAGED(ip) != False) && (ITEM_OBJ(ip) == op))
	    return(ip);
    }
    return(NULL);
}				/* End of DmObjectToItem */

/****************************procedure*header*****************************
    DmObjNameToItem-
*/
DmItemPtr
DmObjNameToItem(DmWinPtr win, register char * name)
{
    register DmItemPtr item;

    for (item = win->itp; item < win->itp + win->nitems; item++)
	if (ITEM_MANAGED(item) && (strcmp(ITEM_OBJ_NAME(item), name) == 0) )
	    return(item);

    return(NULL);
}				/* End of DmObjectToIndex */

/****************************procedure*header*****************************
    DmOpenObject-
*/
void
DmOpenObject(DmWinPtr wp, DmObjectPtr op)
{
    char *p = NULL;

    if ((p = DmGetObjProperty(op, OPENCMD, NULL)) == NULL) {
	/* kai, try not to make this an exception */
	/* Use the default Open command depending on file type */
	if (OBJ_IS_DIR(op))
	{
	    p = strdup(DmObjPath(op));
	    if (DmOpenFolderWindow(p, 0, NULL, False) == NULL)
		DmVaDisplayStatus(wp, True, TXT_OpenErr, p);
	    FREE(p);
	}

	/* ultimate OPEN command default for data files */
	if (op->ftype == DM_FTYPE_DATA) {
		p = (char *)DFLT_OPEN_CMD;
	}
	else
		/* No default for others */
		return;
    }

    p = Dm__expand_sh(p, DmObjProp, (XtPointer)op);
    DmExecuteShellCmd(wp, op, p, False);
    FREE(p);
} /* end of DmOpenObject */

/****************************procedure*header*****************************
    DmQueryFolderWindow-
*/
DmFolderWinPtr
DmQueryFolderWindow(register char * path)
{
    register DmFolderWinPtr folder;
    char *real_path = realpath(path, Dm__buffer);

    for (folder = DESKTOP_FOLDERS(Desktop);
	 folder != NULL; folder = folder->next)
	if (strcmp(real_path, folder->cp->path) == 0)
	    break;

    return(folder);
}				/* end of DmQueryFolderWindow */

/****************************procedure*header*****************************
    DmSameOrDescendant-
		< 0 if path2 is the same as path1,
		> 0 if path2 is a descendant of path1
		= 0 otherwise.

	The caller can pass 'len' to optimize (for loop processing, for
	instance), otherwise, 'len' is calculated.
*/
int
DmSameOrDescendant(char * path1, char * path2, int path1_len)
{
    if (path1_len <= 0)
	path1_len = strlen(path1);

    return((strncmp(path1, path2, path1_len) != 0) ? 0 :
	   (path2[path1_len] == '\0') ? -1 :
	   (path2[path1_len] == '/') ? 1 : 0);

}					/* end of DmSameOrDescendant */

/****************************procedure*header*****************************
    DmTouchIconBox- append "items touched" args to 'args_in' and do
	XtSetValues.  (Assumes args_in is large enough.  Typically, Dm__arg
	is used.)
*/
void
DmTouchIconBox(DmWinPtr window, ArgList args_in, Cardinal num_args)
{
    ArgList args;

    if (args_in == NULL)
    {
	args = Dm__arg;
	num_args = 0;

    } else
	args = args_in;

    XtSetArg(args[num_args], XtNitems,		window->itp);	num_args++;
    XtSetArg(args[num_args], XtNnumItems,	window->nitems);num_args++;
    XtSetArg(args[num_args], XtNitemsTouched,	True);		num_args++;
    XtSetValues(window->box, args, num_args);

}					/* End of DmTouchIconBox */

/****************************procedure*header*****************************
    DmUpdateFolderTitle- generate new folder title and display it.
	Note: at the time of this writing, it is not necessary to update
	the gizmo ('base->title').
*/
void
DmUpdateFolderTitle(DmFolderWindow folder)
{
    BaseWindowGizmo *	base = DM_WIN_BASE_GIZMO(folder);
    XTextProperty	text_prop;
    char *		list[2];

    XtFree(base->title);
    base->title = strdup(DmMakeFolderTitle(folder));

    list[0] = strdup(base->title);
    XStringListToTextProperty(list, 1, &text_prop);
    XSetWMName(XtDisplay(folder->shell), XtWindow(folder->shell), &text_prop);
    XtFree(list[0]);
}

/****************************procedure*header*****************************
    DmVaDisplayState- display message 'msg' in "state/mode" part of
	'window' footer.
*/
void
DmVaDisplayState(DmWinPtr window, char * msg, ... )
{
    va_list ap;

    va_start (ap, msg);
    VDisplayMsg(window, False, msg, ap);
    va_end(ap);
}

/****************************procedure*header*****************************
    DmVaDisplayStatus-
*/
void
DmVaDisplayStatus(DmWinPtr window, int type, char * msg, ... )
{
    va_list ap;

    if (type)
	_OlBeepDisplay(window->shell, 1);

    va_start (ap, msg);
    VDisplayMsg(window, True, msg, ap);
    va_end(ap);
}

/****************************procedure*header*****************************
    Dm__VaPrintMsg-
*/
void
Dm__VaPrintMsg(char *format, ... )
{
	va_list	ap;
	char buffer[1024];
	const char *lmsg;

	/* format message using var args */
	va_start(ap, format);
	lmsg = Dm__gettxt(format);
	vsprintf(buffer, lmsg, ap);
	va_end(ap);

	/* should use fmtmsg() */
	fprintf(stderr, "UX: dtm: %s\n", buffer);
}

void
DmAddWindow(list, newp)
DmWinPtr *list;
DmWinPtr newp;
{
	newp->next = NULL;

	if (*list == NULL)
		*list = newp;
	else {
		register DmWinPtr wp;

		for (wp=*list; wp->next; wp=wp->next);

		wp->next = newp;
	}
}

void
DmDisplayStatus(window)
DmWinPtr window;
{
	int numselected = 0;
	int nitems = 0;
	int i;
	Widget status;
	DmItemPtr ip;
	char lbuff[64];
	char rbuff[64];

	/* figure out the # of items and # of selected items */
	for (ip=window->itp, i=window->nitems; i; i--, ip++)
		if (ITEM_MANAGED(ip)) {
			nitems++;
			if (ITEM_SELECT(ip))
				numselected++;
		}


	/*
	 * Update selectCount at this point.
	 * Because some d&d operations didn't update this correctly.
	 */
	XtSetArg(Dm__arg[0], XtNselectCount, numselected);
	XtSetValues(window->box, Dm__arg, 1);

	/*
	 * Depending on window type, do something different here.
	 */
	sprintf(lbuff, Dm__gettxt(TXT_NUM_SELECTED_ITEMS), numselected);
	sprintf(rbuff, Dm__gettxt(TXT_NUM_ITEMS), nitems);

	XtSetArg(Dm__arg[0], XtNleftFoot, lbuff);
	XtSetArg(Dm__arg[1], XtNrightFoot, rbuff);

	status = (Widget)QueryGizmo(BaseWindowGizmoClass, window->gizmo_shell,
				GetGizmoWidget, "status");

	XtSetValues(status, Dm__arg, 2);
}

void
DmRetypeObj(DmObjectPtr op)
{
	DmSetFileClass(op);
	DmInitObjType(DESKTOP_SHELL(Desktop), op);
}

/*
 * Note that there is no need to handle wastebasket window and helpdesk
 * window, because they only used their own special file classes.
 */
void
DmSyncWindows(new_fnkp, del_fnkp)
DmFnameKeyPtr new_fnkp;
DmFnameKeyPtr del_fnkp;
{
	register DmObjectPtr op;
	DmFnameKeyPtr fnkp;
	DmWinPtr wp;
	int refresh;
	int i;
	int done;

	XtSetArg(Dm__arg[0], XtNitemsTouched, True);

	/* folder windows */
	wp = (DmWinPtr)DESKTOP_FOLDERS(Desktop);

	/* loop through all windows */
	for (; wp; wp=wp->next) {
		refresh = 0;
		for (op=wp->cp->op; op; op=op->next) {
			done = 0;

			/* check for deleted classes */
			for (fnkp=del_fnkp; fnkp; fnkp=fnkp->next) {
				if (op->fcp->key == fnkp) {
					DmFclassPtr save_fcp = op->fcp;

					DmRetypeObj(op);
					if (op->fcp != save_fcp) {
						refresh++;
						done++;
						break;
					}
				}
			}

			if (done)
				continue;

			/* check for new or changed classes */
			for (fnkp=new_fnkp; fnkp; fnkp=fnkp->next) {
				if (fnkp->attrs & DM_B_REPLACED) {
					DmFclassPtr save_fcp = op->fcp;

					DmRetypeObj(op);
					if (op->fcp != save_fcp) {
						refresh++;
						done++;
						break;
					}
				}
				else if (op->fcp->key == fnkp)
					break;
			}
		}

		if (refresh)
			XtSetValues(wp->box, Dm__arg, 1);
	} /* for each window */
}

/*
 * This function searches for the specified file name in the class database.
 * If found, a ptr to the file class header is returned.
 */
DmFclassFilePtr
DmCheckFileClassDB(filename)
char *filename;
{
	register DmFclassFilePtr fcfp=(DmFclassFilePtr)DESKTOP_FNKP(Desktop);

	for (; fcfp; fcfp=fcfp->next_file)
		if (!strcmp(fcfp->name, filename))
			break;

	return(fcfp);
}

/*
 * This function looks at the item. If it is currently selected, then it
 * returns an array of selected items. If it is not selected, then it returns
 * an array with only the item in it.
 */
void **
DmGetItemList(window, item_index)
DmWinPtr window;
int item_index;		/* item being operated on */
{
	void **list;

	if ((item_index != OL_NO_ITEM) &&
	    !ITEM_SELECT(DM_WIN_ITEM(window, item_index))
		&& !ITEM_BUSY(DM_WIN_ITEM(window, item_index))) {
		/* return one entry */
		return(DmOneItemList(DM_WIN_ITEM(window, item_index)));
	}
	else {
		/* return an array of selected items */
		int i;
		DmItemPtr ip;
		void **lp;

		/* get the # of selected items */
		XtSetArg(Dm__arg[0], XtNselectCount, &i);
		XtGetValues(window->box, Dm__arg, 1);

		if (list = (void **)MALLOC(sizeof(void *) * ++i)) {
			for (ip=window->itp, i=window->nitems, lp=list;
			     i; i--,ip++)
				if (ITEM_MANAGED(ip) && ITEM_SELECT(ip) && !ITEM_BUSY(ip))
					*lp++ = ip;
			*lp = NULL; /* a NULL terminated list */
		}
	}

	return(list);
}

/*
 * This function builds a one entry item list.
 */
void **
DmOneItemList(DmItemPtr ip)
{
	void **ilist = (void **)MALLOC(sizeof(void *) * 2);

	if (ilist) {
		ilist[0] = ip;
		ilist[1] = NULL;
	}

	return(ilist);
}

/*
 * This function converts a list of item ptrs to a list of filenames & a count.
 * The purpose of this convertion is to accommodate current interface of the
 * file operation code during a transition period.
 */
char **
DmItemListToSrcList(ilist, count)
void **ilist;		/* NULL terminated item list */
int *count;
{
	DmItemPtr *ipp;
	char **src_list;

	/* count the item list first */
	for (*count=0, ipp=(DmItemPtr *)ilist; *ipp; ipp++, (*count)++) ;

	if (src_list = (char **)MALLOC(sizeof(char *) * *count)) {
		char **sp;

		for (sp=src_list, ipp=(DmItemPtr *)ilist; *ipp; ipp++)
    			*sp++ = strdup(ITEM_OBJ(*ipp)->name);
	}

	return(src_list);
}

char *
DmClassName(fcp)
DmFclassPtr fcp;
{
	char *classname;

	if (classname = DtGetProperty(&(fcp->plist), CLASS_NAME, NULL))
		return(classname);
	else
		return(((DmFnameKeyPtr)(fcp->key))->name);
}

char *
DmObjClassName(op)
DmObjectPtr op;
{
	return(DmClassName(op->fcp));
}

/*
 * DmMapWindow() maps and raises the window, and then set focus to the icon
 * box widget.
 */
void
DmMapWindow(window)
DmWinPtr window;
{
	XWMHints *wmh;
	Display *dpy = XtDisplay(window->shell);
	Window   win = XtWindow(window->shell);

	wmh = XGetWMHints(dpy, win);
	if (wmh) {
		if (wmh->initial_state != NormalState) {
			wmh->initial_state = NormalState;
			XSetWMHints(dpy, win, wmh);
		}
		FREE((void *)wmh);
	}

	XtMapWidget(window->shell);
	if (OlCanAcceptFocus(window->box, CurrentTime) != False) {
		XRaiseWindow(dpy, win);
		OlSetInputFocus(window->box, RevertToNone, CurrentTime);
	}
}

void
DmUnmapWindow(window)
DmWinPtr window;
{
	Widget w = window->shell;
	XUnmapEvent xunmap;

	XtUnmapWidget(w);

	/* Send a synthetic unmap notify event as described in ICCCM */
	xunmap.type		= UnmapNotify;
	xunmap.serial		= (unsigned long)0;
	xunmap.send_event	= True;
	xunmap.display 		= XtDisplay(w);
	xunmap.event 		= RootWindowOfScreen(XtScreen(w));
	xunmap.window		= XtWindow(w);
	xunmap.from_configure	= False;
	XSendEvent(XtDisplay(w), RootWindowOfScreen(XtScreen(w)), False,
		   (SubstructureRedirectMask | SubstructureNotifyMask),
		   (XEvent *)&xunmap);
}

char *
DmHasSystemFiles(void **ipp)
{
	char *ret;

	if (!DmGetDTProperty(IGNORE_SYSTEM, NULL)) {
	    for (;*ipp; ipp++) {
		if ((ret = DmGetObjProperty(ITEM_OBJ((DmItemPtr)*ipp), SYSTEM,
		    NULL)) && ((*ret == 'y') || (*ret == 'Y')))
			return(ITEM_LABEL((DmItemPtr)*ipp));
	    }
	}

	return(NULL);
}


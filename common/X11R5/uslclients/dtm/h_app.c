/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:h_app.c	1.53"

/******************************file*header********************************

    Description:
     This file contains the source code for adding and removing an
	application to the list of applications which have registered
	for help.
*/
                              /* #includes go here     */

#include <errno.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Private Procedures
          2. Public  Procedures
*/

/**************************private*procedures***************************

	Private Procedures.
*/
static void GetAppName(char *old, char **new);
static void GetAppTitle(char *old, char **new);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/*
 * This is used to generate an application id.
 * The value 0 is reserved.
 */
static int  ticket = 1;
struct stat hstat;
static char *dflt_iconfile = "exec.icon";

static char *app_names[][2] = {
	TXT_INTERNETSETUP, TXT_INTERNETMGR,
	TXT_INTERNET_SETUP, TXT_INTERNETMGR,
	TXT_DIALUP_SETUP, TXT_DIALUPMGR,
	TXT_FONT_SETUP, TXT_FONTMGR,
	TXT_FONTSETUP, TXT_FONTMGR,
	TXT_TEXT_EDITOR, TXT_DTEDIT,
	TXT_TEXTEDITOR, TXT_DTEDIT,
	TXT_ICON_SETUP, TXT_DESKTOP_MGR,
	TXT_ICONSETUP, TXT_DESKTOP_MGR,
	TXT_APP_SETUP, TXT_PACKAGEMGR,
	TXT_APPSETUP, TXT_PACKAGEMGR,
	TXT_PRINT_SETUP, TXT_PRTSETUP,
	TXT_PRINTSETUP, TXT_PRTSETUP,
	TXT_PRINTER, TXT_PRTSETUP,
	TXT_USER_SETUP, TXT_LOGINMGR,
	TXT_USERSETUP, TXT_LOGINMGR,
	TXT_FILE_SHARING, TXT_DTNFS,
	TXT_FILESHARING, TXT_DTNFS,
	TXT_SYSTEM_STATUS, TXT_DASHBOARD,
	TXT_SYSTEMSTATUS, TXT_DASHBOARD,
	TXT_PASSWORD_SETUP, TXT_LOGINMGR,
	TXT_PASSWDSETUP, TXT_LOGINMGR,
	TXT_TASK_SCHEDULER, TXT_DTSCHED,
	TXT_TASKSCHEDULER, TXT_DTSCHED,
	TXT_DISKETTE, TXT_MEDIAMGR,
	TXT_TAPE, TXT_MEDIAMGR,
	TXT_BACKUP, TXT_MEDIAMGR,
	TXT_HELPDESK_LINK, TXT_FOLDER_TITLE,
	TXT_DATAFILE, TXT_DESKTOP_MGR,
	TXT_HD_FOLDERMAP, TXT_DESKTOP_MGR,
	TXT_DTM, TXT_DESKTOP_MGR,
	TXT_CLOCK, TXT_DTCLOCK,
	TXT_MAIL, TXT_DTMAIL,
};
static char *(*app_name)[2];

static char *app_titles[][2] = {
	TXT_INTERNETMGR, TXT_INTERNET_SETUP,
	TXT_INTERNET_SETUP, TXT_INTERNET_SETUP,
	TXT_DIALUPSETUP, TXT_DIALUP_SETUP,
	TXT_DIALUPMGR, TXT_DIALUP_SETUP,
	TXT_FONTSETUP, TXT_FONT_SETUP,
	TXT_FONTMGR, TXT_FONT_SETUP,
	TXT_TEXTEDITOR, TXT_TEXT_EDITOR,
	TXT_ICONSETUP, TXT_ICON_SETUP,
	TXT_APPSETUP, TXT_APP_SETUP,
	TXT_PACKAGEMGR,TXT_APP_SETUP,
	TXT_PRINTSETUP, TXT_PRINT_SETUP,
	TXT_PRINTER, TXT_PRINT_SETUP,
	TXT_PRTSETUP, TXT_PRINT_SETUP,
	TXT_USERSETUP, TXT_USER_SETUP,
	TXT_LOGINMGR, TXT_USER_SETUP,
	TXT_FILE_SHARING, TXT_FILE_SHARING,
	TXT_SYSTEMSTATUS, TXT_SYSTEM_STATUS,
	TXT_DASHBOARD, TXT_SYSTEM_STATUS,
	TXT_PASSWDSETUP, TXT_PASSWORD_SETUP,
	TXT_TASKSCHEDULER, TXT_TASK_SCHEDULER,
	TXT_DISKETTE, TXT_MEDIAMGR,
	TXT_TAPE, TXT_MEDIAMGR,
	TXT_BACKUP, TXT_MEDIAMGR,
	TXT_HELPDESK_LINK, TXT_FOLDER_TITLE,
	TXT_DATAFILE, TXT_FOLDER_TITLE,
	TXT_HD_FOLDERMAP, TXT_FOLDERMAP,
	TXT_XTERM, TXT_TERMINAL,
};
static char *(*app_title)[2];

/****************************procedure*header*****************************
 * This routine will allocate a new application structure and
 * returns the associated app_id.
 */
DmHelpAppPtr
DmNewHelpAppID(scrn, win, app_name, app_title, node, help_dir, icon_file)
Screen *scrn;
Window win;
char   *app_name;
char   *app_title;
char   *node;
char   *help_dir;
char   *icon_file;
{
	char *begp;
	char *endp;
	char *name;
	char *class;
	char *app;
	char *napp_name;
	char *napp_title;
	char buf[PATH_MAX];
	char lang[64];
	int  len;
	DmMapfilePtr   mp;
	DmHelpBmarkPtr bmp;
	DmHelpAppPtr   hap = NULL;
	DmGlyphPtr     gp = NULL;
	register DmHelpAppPtr thap;

	/* There is at least one graphical admin. app which is still
	 * passing in "none" as its app_name.  Locate its app_name
	 * using its app_title, if specified.  If app_title is NULL,
	 * simply return NULL.
	 */
	if (strcmp(app_name, "none") == 0)
		if (app_title == NULL)
			return(NULL);
		else
			app_name = app_title;

	/* convert application name */
	GetAppName(app_name, &napp_name);
	if (app_title != NULL && strcmp(app_title, ""))
		/* convert application title */
		GetAppTitle(app_title, &napp_title);
	else
		GetAppTitle(napp_name, &napp_title);

 	/* check if app is already in list */
	for (thap = DESKTOP_HELP_INFO(Desktop); thap; thap = thap->next) {
		if (strcmp(thap->name, napp_name) == 0) {
			if (napp_title) {
				if (thap->title && strcmp(napp_title, thap->title) == 0)
					return(thap);
			} else {
				if (strcmp(thap->title, napp_name) == 0)
					return(thap);
			}
		}
	}

	if ((hap = (DmHelpAppPtr)CALLOC(1, sizeof(DmHelpAppRec))) == NULL)
		return(NULL);

	hap->scrn        = scrn;
	hap->app_win     = win;
	hap->name        = strdup(napp_name);
	hap->title       = strdup(napp_title);

	hap->hlp_win.sp    = -1;
	hap->hlp_win.hsp   = NULL;
	hap->hlp_win.shell = NULL;
	hap->bmp           = NULL;
	hap->num_bmark     = 0;

	if (help_dir)
		hap->help_dir = strdup(help_dir);
	else
		hap->help_dir = NULL;

	/* use a default executable icon if no icon_file is specified */
	gp = DmGetPixmap(scrn, (icon_file ? icon_file : dflt_iconfile));
	if (gp != NULL)
		hap->icon_pixmap = gp->pix;
	else
		hap->icon_pixmap = NULL;

	/* takes care of wraparound */
	if (ticket < 0)
		ticket = 1;

	hap->app_id = ticket++;

	/* add it to the list */
	hap->next = DESKTOP_HELP_INFO(Desktop);
	DESKTOP_HELP_INFO(Desktop) = hap;

	len = sprintf(lang, "%s", getenv("LANG"));

	if (len == 0) {
		strcpy(lang, "C");
	} else {
		/* remove '\n' from lang */
		if (strchr(lang, '\n'))
			lang[len - 1] = '\0';
	}

	if (strcmp(hap->name, Dm__gettxt(TXT_DESKTOP_MGR)) == 0)
		app = hap->title;
	else
		app = hap->name;

	sprintf(buf, "%s/.dthelp/.%s/.%s/.bookmark", DESKTOP_DIR(Desktop),
		lang, app);

	errno = 0;
	if ((stat(buf, &hstat)) != 0) {
		if (errno == ENOENT) {
			return(hap);
		}
	}
	if (hstat.st_size == 0) { /* bookmark file is empty */
			return(hap);
	}

	/* create list of bookmarks if exist */
	if (!(mp = Dm__mapfile(buf, PROT_READ, MAP_SHARED)))
		Dm__VaPrintMsg(TXT_CANT_ACCESS_BMARK_FILE, buf);

	else {
		/* start reading in bookmarks */
		while (MF_NOT_EOF(mp)) {
			bmp = (DmHelpBmarkPtr)CALLOC(1, sizeof(DmHelpBmarkRec));
			bmp->blp = (DmBmarkLabelPtr)CALLOC(1, sizeof(DmBmarkLabelRec));

			begp = MF_GETPTR(mp);
			if (!(endp = Dm__findchar(mp, '^')))
				goto bye;
			bmp->blp->app_name = (XtPointer)
					(strndup(begp, endp - begp));

			MF_NEXTC(mp);
			begp = MF_GETPTR(mp);
			if (!(endp = Dm__findchar(mp, '^'))) {
				FREE(bmp->blp->app_name);
				FREE((void *)(bmp->blp));
				FREE((void *)bmp);
				goto bye;
			}
			bmp->blp->file_name = (XtPointer)
					(strndup(begp, endp - begp));

			MF_NEXTC(mp);
			begp = MF_GETPTR(mp);
			if (!(endp = Dm__findchar(mp, '^'))) {
				FREE(bmp->blp->app_name);
				FREE(bmp->blp->file_name);
				FREE((void *)(bmp->blp));
				FREE((void *)bmp);
				goto bye;
			}
			bmp->blp->sect_tag = (XtPointer)
					(strndup(begp, endp - begp));

			MF_NEXTC(mp);
			begp = MF_GETPTR(mp);
			if (!(endp = Dm__findchar(mp, '^'))) {
				FREE(bmp->blp->app_name);
				FREE(bmp->blp->file_name);
				FREE(bmp->blp->sect_tag);
				FREE((void *)(bmp->blp));
				FREE((void *)bmp);
				goto bye;
			}
			bmp->blp->sect_name = (XtPointer)
					(strndup(begp, endp - begp));

			MF_NEXTC(mp); /* skip '^' */
			MF_NEXTC(mp); /* skip '\n' */

			bmp->next = hap->bmp;
			hap->bmp = bmp;
			hap->num_bmark++;
		}
	}
bye:
	Dm__unmapfile(mp);
	return(hap);

} /* end of DmNewHelpAppID */

/****************************procedure*header*****************************
 * This routine will return the DmHelpAppPtr given the associated app_id.
 */
DmHelpAppPtr
DmGetHelpApp(app_id)
int app_id;
{
	register DmHelpAppPtr hap;

	for (hap=DESKTOP_HELP_INFO(Desktop); hap; hap=hap->next)
		if (app_id == hap->app_id)
			return(hap);
	return(NULL);

} /* end of DmGetHelpAppID */

/****************************procedure*header*****************************
 * This routine will free all the resources associated with the app_id.
 */
void
DmFreeHelpAppID(app_id)
int app_id;
{
	DmHelpAppPtr hap;

	if (hap = DmGetHelpApp(app_id)) {
		/* remove it from the list */
		if (hap->name)
			FREE(hap->name);

		if (hap->title)
			FREE(hap->title);

		if (hap->help_dir)
			FREE(hap->help_dir);

		DmRemoveAppFromList(hap);
		DmCloseHelpWindow(&(hap->hlp_win));
		FREE((void *)hap);
	}
} /* end of DmFreeHelpAppID */

/****************************procedure*header*****************************
 * Removes an application from the list of applications which are
 * registered for help.
 */
void
DmRemoveAppFromList(hap)
DmHelpAppPtr hap;
{
	register DmHelpAppPtr thap = DESKTOP_HELP_INFO(Desktop);

	if (thap == hap) {
		DESKTOP_HELP_INFO(Desktop) = hap->next;
	} else {
		for (; thap->next; thap = thap->next) {
			if (thap->next == hap) {
				thap->next = hap->next;
				return;
			}
		}
	}
} /* end of DmRemoveAppFromList */

/****************************procedure*header*****************************
 * Returns a "new" application name given an "old" application name.
 */
void
GetAppName(old, new)
char *old;
char **new;
{
	for(app_name = app_names;
		app_name < app_names + XtNumber(app_names); app_name++) {

		if (strcmp(old, Dm__gettxt((*app_name)[0])) == 0) {
			*new = Dm__gettxt((*app_name)[1]);
			return;
		}
	}
	*new = old;
} /* end of GetAppName */

/****************************procedure*header*****************************
 * Returns a "new" application title given an "old" application title.
 */
void
GetAppTitle(old, new)
char *old;
char **new;
{

	for(app_title = app_titles;
		app_title < app_titles + XtNumber(app_titles); app_title++) {

		if (strcmp(old, Dm__gettxt((*app_title)[0])) == 0) {
			*new = Dm__gettxt((*app_title)[1]);
			return;
		}
	}
	*new = old;
} /* end of GetAppTitle */

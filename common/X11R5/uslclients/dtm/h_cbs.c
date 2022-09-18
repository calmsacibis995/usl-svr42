/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:h_cbs.c	1.110"

/******************************file*header********************************

    Description:
     This file contains the source code for the callbacks for buttons
	in the help window.  It also contains code for the Bookmark, Notes,
	Search, Definition and Glossary windows.
*/
                              /* #includes go here     */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/StaticText.h>
#include <Xol/TextField.h>
#include <Xol/Caption.h>
#include <Xol/ControlAre.h>
#include <Xol/ScrolledWi.h>
#include <Xol/FList.h>
#include <Xol/FButtons.h>
#include <Xol/TextEdit.h>

#include <Gizmos.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Public Procedures
          2. Private  Procedures
*/
                         /* private procedures         */


static void	GlossCancelCB(Widget, XtPointer, XtPointer);
static void	GlossPopdnCB(Widget, XtPointer, XtPointer);
static void	DefPopdnCB(Widget, XtPointer, XtPointer);
static void	DefCancelCB(Widget, XtPointer, XtPointer);
static void	BmarkPopdnCB(Widget, XtPointer, XtPointer);
static void	BmarkCancelCB(Widget, XtPointer, XtPointer);
static void	SearchPopdnCB(Widget, XtPointer, XtPointer);
static void	SearchCancelCB(Widget, XtPointer, XtPointer);
static void	NotesPopdnCB(Widget, XtPointer, XtPointer);
static void	NotesCancelCB(Widget, XtPointer, XtPointer);
static void	DisplayDef(DmHelpWinPtr, char *, char *);
static void	NotesChgCB(Widget, XtPointer, XtPointer);
static int	SaveNotesCB(Widget, XtPointer, XtPointer);
static void	DelNotesCB(Widget, XtPointer, XtPointer);
static void	DelNotes(DmHelpWinPtr hwp, DmHelpNotesPtr np);
static int	DoSearchCB(Widget, XtPointer, XtPointer);
static void	AddBmarkCB(Widget, XtPointer, XtPointer);
static void	BtnGoToBmarkCB(Widget, XtPointer, XtPointer);
static void	LstGotoBmarkCB(Widget, XtPointer, XtPointer);
static void	DelBmarkCB(Widget, XtPointer, XtPointer);
static void	DelAllBmarkCB(Widget, XtPointer, XtPointer);
static void	DelBmark(DmHelpAppPtr hap, DmHelpBmarkPtr bmp, Boolean all);
static void	UpdateBmarkFile(DmHelpAppPtr hap);
static int	UpdateNotesFile(DmHelpFilePtr hfp);
static void	FreeBmark(DmHelpBmarkPtr bmp);
static void	FreeAllBmarks(DmHelpAppPtr hap);
static void	PopdownShell(Widget shell);
static void	GlossHelpCB(Widget, XtPointer, XtPointer);
static void	BmarkHelpCB(Widget, XtPointer, XtPointer);
static void	NotesHelpCB(Widget, XtPointer, XtPointer);
static void	SearchHelpCB(Widget, XtPointer, XtPointer);
static void	DefHelpCB(Widget, XtPointer, XtPointer);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

static String	btn_fields[] = {
	XtNlabel, XtNmnemonic, XtNselectProc, XtNdefault, XtNuserData,
	XtNsensitive
};

static char	*list_fields[] = {
	XtNset, XtNformatData, XtNuserData,
};

typedef struct {
	XtArgVal	set;
	XtArgVal	formatData;
	XtArgVal	userData;
} TokenRec, *TokenPtr;

static OlDtHelpInfo DefWinHelp = {
	NULL, NULL, "DesktopMgr/help.hlp", "310", NULL
};

static OlDtHelpInfo GlossWinHelp = {
	NULL, NULL, "DesktopMgr/help.hlp", "340", NULL
};

static OlDtHelpInfo BmarkWinHelp = {
	NULL, NULL, "DesktopMgr/help.hlp", "150", NULL
};

static OlDtHelpInfo SearchWinHelp = {
	NULL, NULL, "DesktopMgr/help.hlp", "220", NULL
};

static OlDtHelpInfo NotesWinHelp = {
	NULL, NULL, "DesktopMgr/help.hlp", "260", NULL
};

/***************************public*procedures****************************

    Public Procedures
*/


/****************************procedure*header*****************************
 * Callback to jump to a section when a link is selected, and display
 * a definition window when a term is selected.
 *
 * If SCRIPT is defined, use it to look up a link or a definition.
 *
 * If a link is selected and SCRIPT is defined, then use SCRIPT as the
 * reference to the link.  In this case, SCRIPT must follow the format
 * defined for a reference to a link (i.e. filename^section_tag/name).
 *
 * If a link is selected and SCRIPT is not defined, then construct
 * a reference using KEY which assumed to be a section name.  In this
 * case, the section has to be in the current file; otherwise, SCRIPT
 * must be defined to specify a help file name.
 */
void
DmHtextSelectCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
#define KEY	HyperSegmentKey(hs)
#define TEXT	HyperSegmentText(hs)
#define SCRIPT	HyperSegmentScript(hs)
#define DEF	HyperSegmentRV(hs)

	HyperSegment   *hs = (HyperSegment *)call_data;
	DmHelpWinPtr   hwp = (DmHelpWinPtr)client_data;
	DmHelpLocPtr   hlp;
	char           *ref = NULL;
	char           buf[256];

	DmClearStatus((DmWinPtr)hwp);

	if (DEF != False) { /* term selected */

		if (SCRIPT) {
			if (!(ref = DtGetProperty(&(hwp->hsp->defs), SCRIPT, NULL)) &&
		    	    !(ref = DtGetProperty(&(hwp->hfp->defs), SCRIPT, NULL))) {
					DmVaDisplayStatus((DmWinPtr)hwp, True,
						TXT_NO_HELP_DEF, KEY);
					return;
			}
		} else {
		    if (!(ref = DtGetProperty(&(hwp->hsp->defs), KEY, NULL)) &&
		    	   !(ref = DtGetProperty(&(hwp->hfp->defs), KEY, NULL))) {
					DmVaDisplayStatus((DmWinPtr)hwp, True,
						TXT_NO_HELP_DEF, KEY);
					return;
			}
		}
		DisplayDef(hwp, (char *)KEY, ref);

	} else { /* link selected */
		DmHelpAppPtr hap;

		if (SCRIPT) {
			hlp = DmHelpRefToLoc(SCRIPT);
		} else {
			sprintf(buf, "^%s", KEY); 
			hlp = DmHelpRefToLoc(buf);
		}

		if (hlp == NULL || (hlp->file == NULL && hlp->sect_tag == NULL)) {
			DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NO_HELP_LINK);
			return;
		}

		hap = DmGetHelpApp(hwp->app_id);
		DmDisplayHelpSection(hwp, hwp->app_id, hwp->hfp->title,
			hlp->file, hlp->sect_tag, UNSPECIFIED_POS, UNSPECIFIED_POS);
	}
#undef KEY
#undef TEXT
#undef SCRIPT
}	/* end of DmHtextSelectCB */


/****************************procedure*header*****************************
 * This callback displays the next section in a help file
 */
void
DmNextSectionCB(w, client_data, call_data)
Widget	w;
XtPointer client_data;
XtPointer call_data;
{
	DmHelpWinPtr	hwp = (DmHelpWinPtr)client_data;

	DmClearStatus((DmWinPtr)hwp);

	if (hwp->hsp && ((int)(hwp->hsp - hwp->hfp->sections) <
				(int)(hwp->hfp->num_sections - 1))) {

		DmHelpAppPtr hap = DmGetHelpApp(hwp->app_id);

		/* set this to False in case some text was entered but not saved */
		hwp->hsp->notes_chged = False;

		/* display the next section */
		DmDisplayHelpSection(hwp, hwp->app_id, hwp->hfp->title, NULL,
			((hwp->hsp + 1)->tag ? (hwp->hsp + 1)->tag :
			(hwp->hsp + 1)->name), UNSPECIFIED_POS, UNSPECIFIED_POS);
	} else {
		/* Currently at the last section; this should never happen. */
		return;
	}

}	/* end of DmNextSectionCB */


/****************************procedure*header*****************************
 * This callback displays the previous section in a help file
 */
void
DmPrevSectionCB(w, client_data, call_data)
Widget	w;
XtPointer client_data;
XtPointer call_data;
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	DmClearStatus((DmWinPtr)hwp);

	if (hwp->hsp && hwp->hsp == hwp->hfp->sections) {
		/* Currently at the first section; this should never happen. */
		return;

	} else if (hwp->hsp && (hwp->hsp != hwp->hfp->sections)) {

		DmHelpAppPtr hap = DmGetHelpApp(hwp->app_id);

		/* set this to False in case some text was entered but not saved */
		hwp->hsp->notes_chged = False;

		/* display the previous section */
		DmDisplayHelpSection(hwp, hwp->app_id, hwp->hfp->title, NULL,
			((hwp->hsp - 1)->tag ? (hwp->hsp - 1)->tag :
			(hwp->hsp - 1)->name), UNSPECIFIED_POS, UNSPECIFIED_POS);
	}

}	/* end of DmPrevSectionCB */


/****************************procedure*header*****************************
 * This callback switches the view to the previous view at the top of
 * the history stack.
 */
void
DmBackTrackCB(w, client_data, call_data)
Widget	w;
XtPointer client_data;
XtPointer call_data;
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	if (hwp->stack == NULL || hwp->sp == -1) {
		/* Stack of visited section is empty; this should never happen. */
		return;
	}
	/* set this to False in case some text was entered but not saved */
	if (hwp->hsp)
		hwp->hsp->notes_chged = False;

	DmClearStatus((DmWinPtr)hwp);
	DmPopHelpStack(w, hwp);

}	/* end of DmBackTrackCB */

/****************************procedure*header*****************************
 * Displays the table of contents for a given help file. A dollar sign ($)
 * is used as a delimiter in a link.
 */
void
DmTOCCB(w, client_data, call_data)
Widget	w;
XtPointer client_data;
XtPointer call_data;
{
	DmHelpAppPtr hap;
	DmHelpWinPtr hwp;
	char         title[256];
	char         *file = NULL;
	char         *sect_name;
	char         *sect_tag;
	
	hwp = (DmHelpWinPtr)client_data;

	/*
	 * If called from Go To menu, simply return if TOC is already
	 * being displayed.  The same check is made in DmDisplayHelpTOC()
	 * for when this routine is not called from the Go To menu.
	 */
	if (hwp->hsp && call_data) {
		/* called from Go To menu */ 
		if (strcmp(hwp->hsp->name, TABLE_OF_CONTENTS) == 0) {
			return;
		} else { 
			file = strdup(hwp->hfp->name);
			if (hwp->hsp->name)
				sect_name = strdup(hwp->hsp->name);
			else
				sect_name = NULL;
			if (hwp->hsp->tag)
				sect_tag  = strdup(hwp->hsp->tag);
			else
				sect_tag = NULL;
		}
	}

	hap = DmGetHelpApp(hwp->app_id);

	/* deactivate the "irrelevant" buttons */
	DmChgHelpWinBtnState(hwp, False, False);
	XtSetArg(Dm__arg[0], XtNsensitive, True);

	/* make sure Go To and Search buttons are sensitive */
	OlFlatSetValues(hwp->menubar, 3, Dm__arg, 1);
	OlFlatSetValues(hwp->menubar, 5, Dm__arg, 1);

	sprintf(title, "%s %s: %s", hap->title, Dm__gettxt(TXT_HELP_STR),
		Dm__gettxt(TXT_TOC));

	XtVaSetValues(hwp->shell, XtNtitle, title, NULL);
	DmClearStatus((DmWinPtr)hwp);

	if (hwp->hfp->toc == NULL) {
		/* create table of contents section */
		DmHelpSectPtr hsp;
		int level1, level2, level3, level4, level5, level6;
		int sect_cnt;
		int i;
		int bufsz = 0;
		char *buf;
		char *savebuf;

		sect_cnt = hwp->hfp->num_sections;

		/* get size of buffer */
		hsp = hwp->hfp->sections;

		for (i = 0; i < sect_cnt; i++, hsp++) { 
			bufsz += strlen(hsp->name) + (hsp->alias ? strlen(hsp->alias) :
					strlen(hsp->name)) + 17;
		}
		bufsz += strlen(Dm__gettxt(TXT_TOC));

		hsp = hwp->hfp->sections;
		level1 = 0;
		for (; sect_cnt; sect_cnt--, hsp++) {
			if (hsp->level == 0)
				bufsz += strlen(hsp->name) + (hsp->alias ?
					strlen(hsp->alias)  : 0) + 22;

			else if (hsp->level == 1)
				bufsz += strlen(hsp->name) + (hsp->alias ?
					strlen(hsp->alias)  : 0) + 27;

			else if (hsp->level == 2)
				bufsz += strlen(hsp->name) + (hsp->alias ?
					strlen(hsp->alias)  : 0) + 30;

			else if (hsp->level == 3)
				bufsz += strlen(hsp->name) + (hsp->alias ?
					strlen(hsp->alias)  : 0) + 40;

			else if (hsp->level == 4)
				bufsz += strlen(hsp->name) + (hsp->alias ?
					strlen(hsp->alias)  : 0) + 50;

			else if (hsp->level == 5)
				bufsz += strlen(hsp->name) + (hsp->alias ?
					strlen(hsp->alias)  : 0) + 70;

			else if (hsp->level == 6)
				bufsz += strlen(hsp->name) + (hsp->alias ?
					strlen(hsp->alias)  : 0) + 70;
		}  

		hsp = hwp->hfp->sections;
		sect_cnt = hwp->hfp->num_sections;

		buf = (char *)MALLOC(bufsz * sizeof(char));
		savebuf = buf;

		/* set up global links using section names and aliases */
		for (i = 0; i < sect_cnt; i++, hsp++) { 
			buf += sprintf(buf, "^""%%""%s^^%s\n", hsp->name,
					hsp->alias ? hsp->alias : hsp->name);
		}
		buf += sprintf(buf, "%s", Dm__gettxt(TXT_TOC));

		hsp = hwp->hfp->sections;
		level1 = 0;
		for (; sect_cnt; sect_cnt--, hsp++) {
			if (hsp->level == 0) {
				/* do nothing */
				if (hsp->alias)
					buf += sprintf(buf, "\n ""\\k""$%s^^%s$\n",
							hsp->name, hsp->alias);
				else
					buf += sprintf(buf, "\n ""\\k""$%s$\n", hsp->name);

			} else if (hsp->level == 1) {
				/* reset all sublevels to 0 */
				level2 = level3 = level4 = level5 = level6 = 0;
				if (hsp->alias)
					buf += sprintf(buf, "\n %d. ""\\k""$%s^^%s$\n",
							++level1, hsp->name, hsp->alias);
				else
					buf += sprintf(buf, "\n %d. ""\\k""$%s$\n",
							++level1, hsp->name);

			} else if (hsp->level == 2) {
				level3 = level4 = level5 = level6 = 0;
				if (hsp->alias)
					buf += sprintf(buf, "     %d.%d. \\k$%s^^%s$\n",
							level1, ++level2, hsp->name, hsp->alias);
				else
					buf += sprintf(buf, "     %d.%d. \\k$%s$\n",
							level1, ++level2, hsp->name);

			} else if (hsp->level == 3) {
				level4 = level5 = level6 = 0;
				if (hsp->alias)
					buf += sprintf(buf,
							"          %d.%d.%d. \\k$%s^^%s$\n",
							level1, level2, ++level3, hsp->name,
							hsp->alias);
				else
					buf += sprintf(buf, "          %d.%d.%d. \\k$%s$\n",
							level1, level2, ++level3, hsp->name);

			} else if (hsp->level == 4) {
				level5 = level6 = 0;
				if (hsp->alias)
					buf += sprintf(buf,
					"                 %d.%d.%d.%d. \\k$%s^^%s$\n",
							level1, level2, level3,
							++level4, hsp->name, hsp->alias);
				else
					buf += sprintf(buf,
							"                 %d.%d.%d.%d. \\k$%s$\n",
							level1, level2, level3,
							++level4, hsp->name);

			} else if (hsp->level == 5) {
				level6 = 0;
				if (hsp->alias)
					buf += sprintf(buf,
						"                          %d.%d.%d.%d.%d. "
						"\\k$%s^^%s$\n", level1, level2, level3, level4,
						++level5, hsp->name, hsp->alias);
				else
					buf += sprintf(buf,
						"                          %d.%d.%d.%d.%d. "
						"\\k$%s$\n", level1, level2, level3, level4,
						++level5, hsp->name);

			} else if (hsp->level == 6) {
				if (hsp->alias)
					buf += sprintf(buf,
						"                            %d.%d.%d.%d.%d.%d. "
						"\\k$%s^^%s$\n", level1, level2, level3, level4,
						level5, ++level6, hsp->name, hsp->alias);
				else
					buf += sprintf(buf,
						"                            %d.%d.%d.%d.%d.%d. "
						"\\k$%s$\n", level1, level2, level3, level4,
						level5, ++level6, hsp->name);
			}
		}

		hsp = (DmHelpSectPtr)CALLOC(1, sizeof(DmHelpSectRec));
		/* initialize the table of contents section structure */
		hsp->name           = strdup(TABLE_OF_CONTENTS);
		hsp->raw_data       = savebuf;
		hsp->raw_size       = strlen(savebuf);
		hsp->attrs          = 0;
		hsp->keywords.count = 0;
		hsp->defs.count     = 0;
		hwp->hfp->toc       = hsp;
	}

	if (hwp->hfp->toc->cooked_data == NULL)
		/* cook section first */
		DmProcessHelpSection(hwp->hfp->toc);

	XtSetArg(Dm__arg[0], XtNstring, hwp->hfp->toc->cooked_data);
	XtSetValues(hwp->htext, Dm__arg, 1);

	/*
	 * Push current section onto stack here if there is a current
	 * section and this was called from the Go To menu.
	 */ 
	if (file) {
		DmPushHelpStack(hwp, file, sect_name, sect_tag);
		FREE((void *)file);
		if (sect_name)
			FREE((void *)sect_name);
		if (sect_tag)
			FREE((void *)sect_tag);
	}

	/* set current section */
	hwp->hsp = hwp->hfp->toc;

} /* end of DmTOCCB */


/****************************procedure*header*****************************
 * Displays the definition of all terms in a help file in a popup window.
 */
void
DmGlossaryCB(w, client_data, call_data)
Widget	w;
XtPointer client_data;
XtPointer call_data;
{
	Widget swin;
	Widget lca;
	Widget uca;
	Widget stext;
	Widget buttons;

	char   *definition;
	char   buf[256];
	char   *gbuf;
	char   *savegbuf;

	XtArgVal *p;
	XtArgVal *btn_items;

	int width;
	int height;
	int cnt = 0;
	int bufsz = 0;

	DtPropPtr pp;
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;
	DmHelpAppPtr hap = DmGetHelpApp(hwp->app_id);

	DmClearStatus((DmWinPtr)hwp);
	if (hwp->gloss_shell != NULL) {
		XtPopup(hwp->gloss_shell, XtGrabNone);
		return;
	}

	cnt = hwp->hfp->defs.count;

	if (cnt == 0) {
		/* no definitions */
		DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NO_GLOSSARY);
		return;
	}

	sprintf(buf, "%s: %s", hap->title, Dm__gettxt(TXT_GLOSSARY));
	XtSetArg(Dm__arg[0], XtNtitle,   buf);
	XtSetArg(Dm__arg[1], XtNpushpin, OL_IN);
	hwp->gloss_shell = XtCreatePopupShell("gloss_shell",
					popupWindowShellWidgetClass, hwp->shell, Dm__arg, 2);
	XtAddCallback(hwp->gloss_shell, XtNpopdownCallback, GlossPopdnCB, hwp);

	XtSetArg(Dm__arg[0], XtNupperControlArea, &uca);
	XtSetArg(Dm__arg[1], XtNlowerControlArea, &lca);
	XtGetValues(hwp->gloss_shell, Dm__arg, 2);

	XtSetArg(Dm__arg[0], XtNhStepSize,	&width);
	XtSetArg(Dm__arg[1], XtNvStepSize,	&height);
	XtGetValues(hwp->swin, Dm__arg, 2);

	XtSetArg(Dm__arg[0], XtNviewWidth,    DEFAULT_HELPFILE_WIDTH * width);
	XtSetArg(Dm__arg[1], XtNviewHeight,   15 * height);
	XtSetArg(Dm__arg[2], XtNhStepSize,    width);
	XtSetArg(Dm__arg[3], XtNvStepSize,    height);
	XtSetArg(Dm__arg[4], XtNstopPosition, OL_GRANULARITY);
	swin = XtCreateManagedWidget("swin", scrolledWindowWidgetClass,
					uca, Dm__arg, 5);

	cnt = hwp->hfp->defs.count;
	pp = hwp->hfp->defs.ptr;

	for (; cnt; cnt--, pp++) {
		/* account for newlines */
		bufsz += strlen(pp->name) + strlen(pp->value) + 10;
	}

	gbuf = (char *)MALLOC(bufsz * sizeof(char));
	savegbuf = gbuf;

	cnt = hwp->hfp->defs.count;
	pp = hwp->hfp->defs.ptr;

	for (; cnt; cnt--, pp++) {
		gbuf += sprintf(gbuf, "%s - %s\n\n", pp->name, pp->value);
	}

	XtSetArg(Dm__arg[0], XtNuserData, savegbuf);
	XtSetValues(hwp->gloss_shell, Dm__arg, 1);

	XtSetArg(Dm__arg[0], XtNstring, savegbuf);
	stext = XtCreateManagedWidget("stext", staticTextWidgetClass,
			swin, Dm__arg, 1);

	btn_items = p = (XtArgVal *)MALLOC(sizeof(XtArgVal) * 6 * 2);


	*p++ = (XtArgVal)(Dm__gettxt(TXT_CANCEL_STR));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_CANCEL_STR);
	*p++ = (XtArgVal)GlossCancelCB;
	*p++ = (XtArgVal)True;
	*p++ = (XtArgVal)savegbuf;
	*p++ = (XtArgVal)True;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_HELP_ELLIPSIS));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_HELP_STR);
	*p++ = (XtArgVal)GlossHelpCB;
	*p++ = (XtArgVal)False;
	*p++ = (XtArgVal)NULL;
	*p++ = (XtArgVal)True;

	XtSetArg(Dm__arg[0], XtNitems,         btn_items);
	XtSetArg(Dm__arg[1], XtNnumItems,      2);
	XtSetArg(Dm__arg[2], XtNitemFields,    btn_fields);
	XtSetArg(Dm__arg[3], XtNnumItemFields, XtNumber(btn_fields));
	XtSetArg(Dm__arg[4], XtNlayoutType,    OL_FIXEDROWS);
	XtSetArg(Dm__arg[5], XtNmeasure,       1);
	XtSetArg(Dm__arg[6], XtNclientData,    hwp);
	
	buttons = XtCreateManagedWidget("buttons", flatButtonsWidgetClass,
				lca, Dm__arg, 7);

	/* register help */
	GlossWinHelp.app_title = Dm__gettxt(TXT_PRODUCT_NAME);
	OlRegisterHelp(OL_WIDGET_HELP, hwp->gloss_shell, NULL, OL_DESKTOP_SOURCE,
		(XtPointer)&GlossWinHelp);

	XtPopup(hwp->gloss_shell, XtGrabNone);

}	/* end of DmGlossaryCB */

/****************************procedure*header*****************************
 * Popup a prompt window for for string to search for and calls DoSearch()
 * to find the string in the current help file.
 */
void
DmSearchCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	Widget uca;
	Widget lca;
	Widget caption;
	Widget buttons;

	XtArgVal *p;
	XtArgVal *btn_items;

	char   buf[256];

	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;
	DmHelpAppPtr hap = DmGetHelpApp(hwp->app_id);

	DmClearStatus((DmWinPtr)hwp);
	if (hwp->search_shell != NULL) {
		XtPopup(hwp->search_shell, XtGrabNone);
		return;
	}

	sprintf(buf, "%s: %s", hap->title, Dm__gettxt(TXT_SEARCH));
	XtSetArg(Dm__arg[0], XtNtitle, buf);
	XtSetArg(Dm__arg[1], XtNpushpin, OL_IN);
	hwp->search_shell = XtCreatePopupShell("Search",
						popupWindowShellWidgetClass, w, Dm__arg, 2);
	XtAddCallback(hwp->search_shell, XtNpopdownCallback, SearchPopdnCB, hwp);

	XtSetArg(Dm__arg[0], XtNupperControlArea,	&uca);
	XtSetArg(Dm__arg[1], XtNlowerControlArea,	&lca);
	XtGetValues(hwp->search_shell, Dm__arg, 2);

	XtSetArg(Dm__arg[0], XtNalignment, OL_TOP);
	XtSetArg(Dm__arg[1], XtNlabel,     Dm__gettxt(TXT_SEARCH_FOR));

	caption = XtCreateManagedWidget("caption", captionWidgetClass,
					uca, Dm__arg, 2);

	XtSetArg(Dm__arg[0], XtNstring, "");
	hwp->search_tf = XtCreateManagedWidget("search_tf",
					textFieldWidgetClass,
					caption, Dm__arg, 1);

	XtSetArg(Dm__arg[0], XtNfocusWidget, hwp->search_tf);
	XtSetValues(hwp->search_shell, Dm__arg, 1);

	btn_items = p = (XtArgVal *)MALLOC(sizeof(XtArgVal) * 6 * 3);

	*p++ = (XtArgVal)(Dm__gettxt(TXT_HW_SEARCH_STR));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_HW_SEARCH_STR);
	*p++ = (XtArgVal)DoSearchCB;
	*p++ = (XtArgVal)True;
	*p++ = (XtArgVal)hwp;
	*p++ = (XtArgVal)True;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_CANCEL_STR));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_CANCEL_STR);
	*p++ = (XtArgVal)SearchCancelCB;
	*p++ = (XtArgVal)False;
	*p++ = (XtArgVal)hwp->search_shell;
	*p++ = (XtArgVal)True;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_HELP_ELLIPSIS));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_HELP_STR);
	*p++ = (XtArgVal)SearchHelpCB;
	*p++ = (XtArgVal)False;
	*p++ = (XtArgVal)hwp;
	*p++ = (XtArgVal)True;

	XtSetArg(Dm__arg[0], XtNitems,         btn_items);
	XtSetArg(Dm__arg[1], XtNnumItems,      3);
	XtSetArg(Dm__arg[2], XtNitemFields,    btn_fields);
	XtSetArg(Dm__arg[3], XtNnumItemFields, XtNumber(btn_fields));
	XtSetArg(Dm__arg[4], XtNlayoutType,    OL_FIXEDROWS);
	XtSetArg(Dm__arg[5], XtNmeasure,       1);
	XtSetArg(Dm__arg[6], XtNclientData,    hwp);
	
	buttons = XtCreateManagedWidget("buttons", flatButtonsWidgetClass,
				lca, Dm__arg, 7);

	/* register help */
	SearchWinHelp.app_title = Dm__gettxt(TXT_PRODUCT_NAME);
	OlRegisterHelp(OL_WIDGET_HELP, hwp->search_shell, NULL, OL_DESKTOP_SOURCE,
		(XtPointer)&SearchWinHelp);

	XtPopup(hwp->search_shell, XtGrabNone);

}	/* end of DmSearchCB */

/****************************procedure*header*****************************
 * Searches for a user-specified string in the current help file and
 * displays the section it is contained in in the help window.
 * A search always begins at the beginning of a file and a message is
 * displayed when the search reaches the end of a file.
 * "hsp->matched" is set to True for a section which has been "matched";
 * and is reset to False if the search is repeated for the same file.
 * Sections for which hsp->matched is True are skipped in a search.
 * "repeat" is used to indicate whether a search is being done on the
 * same string in the same file as the previous search.
 */
static int
DoSearchCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	int  cnt;
	char *str;
	char	*p, *savep;
	char	*q, *saveq;
	static char *savestr = NULL;
	static char *savefile = NULL;
	Boolean repeat = False;
	DmHelpSectPtr  hsp;
	DmHelpAppPtr	hap;
	DmHelpWinPtr   hwp = (DmHelpWinPtr)client_data;

	XtSetArg(Dm__arg[0], XtNstring, &str);
	XtGetValues(hwp->search_tf, Dm__arg, 1);

	if (strlen(str) == 0) {
	    DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NO_SEARCH);
	    return(0);
	}

	if ((strcmp(savestr, str) == 0) &&
	     strcmp(savefile, hwp->hfp->name) == 0) {
		/* We are searching for the same string in the same file
		 * as previous search.
		 */
		repeat = True;
	} else {
		/* We are doing a search in either a different file and/or
		 * for a different string as the previous search.
		 */
		repeat = False;
		cnt = hwp->hfp->num_sections;
		hsp = hwp->hfp->sections;
		for (; cnt; cnt--, hsp++) {
			hsp->matched = False;
		}
	}
	/* save name of current file and string to search for */
	savestr = str;
	savefile = hwp->hfp->name;
	hap = DmGetHelpApp(hwp->app_id);

	/* start searching */
	cnt = hwp->hfp->num_sections;
	hsp = hwp->hfp->sections;

	for (; cnt; cnt--, hsp++) {
		DmProcessHelpSection(hsp);

		/* convert section to lower case */
		for (p = savep = strdup(hsp->cooked_data); *p != NULL; ++p) {
			*p = (char)tolower((int)((unsigned char)*p));
		}

		/* convert string to lower case */
		for (q = saveq = strdup(str); *q != NULL; ++q) {
			*q = (char)tolower((int)((unsigned char)*q));
		}

		if (strstr((char *)(hsp->cooked_data), (char *)str)) {

			/* found string in section */
			if (repeat) {
				/* skip sections which are already matched */ 
				if (hsp->matched)
					continue;
			}

			/* This must be done before displaying the wrap-around
			 * message or else it will be cleared in DmDisplayHelpSection.
			 */
			DmDisplayHelpSection(hwp, hwp->app_id, hwp->hfp->title,
				hwp->hfp->name, hsp->tag, UNSPECIFIED_POS,UNSPECIFIED_POS);

			/* if last section, reset hsp->match to False */ 
			if ((int)(hsp - hwp->hfp->sections) ==
			    (int)(hwp->hfp->num_sections - 1)) {
				int		i;
				DmHelpSectPtr	thsp;

				i = hwp->hfp->num_sections;
				thsp = hwp->hfp->sections;
				for (; i; i--, thsp++) {
					thsp->matched = False;
				}
				DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_SEARCH_WRAPPED);
			} else {
				hsp->matched = True;
				DmVaDisplayStatus((DmWinPtr)hwp, False, NULL);
			}
			PopdownShell(hwp->search_shell);
			return;
		}
		FREE((void *)savep);
		FREE((void *)saveq);
	}
	/* no match found */
	DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NO_MATCH);

}	/* end of DmDoSearch */

/****************************procedure*header*****************************
 * Displays the bookmark popup window to allow adding, deleting, and
 * jumping to a section associated to a bookmark.
 */
void
DmBookmarkCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	TokenPtr bmarks = NULL;
	TokenPtr bmarkp;
	int cnt = 0;
	char buf[256];

	Widget uca;
	Widget lca;
	Widget swin;
	Widget caption2;

	XtArgVal *p;
	XtArgVal *btn_items;

	DmHelpBmarkPtr	bmp;
	DmHelpAppPtr	hap;
	DmHelpWinPtr	hwp = (DmHelpWinPtr)client_data;

	DmClearStatus((DmWinPtr)hwp);

	if (hwp->bmark_shell != NULL) {
		XtPopup(hwp->bmark_shell, XtGrabNone);
		return;
	}

	hap = DmGetHelpApp(hwp->app_id);
	sprintf(buf, "%s: %s", hap->title, Dm__gettxt(TXT_BOOKMARK));
	XtSetArg(Dm__arg[0], XtNtitle, buf);
	XtSetArg(Dm__arg[1], XtNpushpin, OL_IN);
	hwp->bmark_shell = XtCreatePopupShell("Bookmark",
						popupWindowShellWidgetClass, w, Dm__arg, 2);
	XtAddCallback(hwp->bmark_shell, XtNpopdownCallback, BmarkPopdnCB, hwp);

	XtSetArg(Dm__arg[0], XtNupperControlArea, &uca);
	XtSetArg(Dm__arg[1], XtNlowerControlArea, &lca);
	XtGetValues(hwp->bmark_shell, Dm__arg, 2);

	XtSetArg(Dm__arg[0], XtNlayoutType,    OL_FIXEDWIDTH);
	XtSetArg(Dm__arg[1], XtNmeasure,       300);
	XtSetValues(uca, Dm__arg, 2);

	if (hap->num_bmark > 0) {
		bmarks = bmarkp = (TokenPtr)CALLOC(hap->num_bmark, sizeof(TokenRec));
		for (bmp = hap->bmp; bmp; bmp = bmp->next, bmarkp++) {
			bmarkp->set           = (XtArgVal)False;
			bmarkp->formatData    = (XtArgVal)bmp->blp;
			bmarkp->userData      = (XtArgVal)bmp;
		}
	}
	XtVaSetValues(hwp->bmark_shell, XtNuserData, bmarks, NULL);

	XtSetArg(Dm__arg[0], XtNalignment,	OL_TOP);
	XtSetArg(Dm__arg[1], XtNposition,	OL_LEFT);
	XtSetArg(Dm__arg[2], XtNlabel,	Dm__gettxt(TXT_CUR_BMARK));
	caption2 = XtCreateManagedWidget("caption2",
					captionWidgetClass, uca, Dm__arg, 3);

	swin = XtCreateManagedWidget("swin", scrolledWindowWidgetClass,
					caption2, Dm__arg, 0);

	XtSetArg(Dm__arg[0], XtNitems, (hap->num_bmark == 0) ? NULL : bmarks);
	XtSetArg(Dm__arg[1], XtNnumItems,      hap->num_bmark);
	XtSetArg(Dm__arg[2], XtNitemFields,    list_fields);
	XtSetArg(Dm__arg[3], XtNnumItemFields, XtNumber(list_fields));
	XtSetArg(Dm__arg[4], XtNformat,        "%0s%0s%s%0s");
	XtSetArg(Dm__arg[5], XtNclientData,    hwp);
	XtSetArg(Dm__arg[6], XtNnoneSet,       True);
	XtSetArg(Dm__arg[7], XtNexclusives,    True);
	XtSetArg(Dm__arg[8], XtNdblSelectProc, LstGotoBmarkCB);

	hwp->bmark_flist = XtCreateManagedWidget("flist", flatListWidgetClass,
					swin, Dm__arg, 9);

	btn_items = p = (XtArgVal *)MALLOC(sizeof(XtArgVal) * 6 * 6);

	*p++ = (XtArgVal)(Dm__gettxt(TXT_HW_GOTO_STR));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_HW_GOTO_STR);
	*p++ = (XtArgVal)BtnGoToBmarkCB;
	*p++ = (XtArgVal)True;
	*p++ = (XtArgVal)hwp;
	*p++ = (XtArgVal)(hap->num_bmark == 0 ? False : True);

	*p++ = (XtArgVal)(Dm__gettxt(TXT_HW_ADD_STR));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_HW_ADD_STR);
	*p++ = (XtArgVal)AddBmarkCB;
	*p++ = (XtArgVal)False;
	*p++ = (XtArgVal)hwp;
	*p++ = (XtArgVal)True;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_HW_DELETE_STR));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_HW_DELETE_STR);
	*p++ = (XtArgVal)DelBmarkCB;
	*p++ = (XtArgVal)False;
	*p++ = (XtArgVal)hwp;
	*p++ = (XtArgVal)(hap->num_bmark == 0 ? False : True);

	*p++ = (XtArgVal)(Dm__gettxt(TXT_HW_DELALL_STR));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_HW_DELALL_STR);
	*p++ = (XtArgVal)DelAllBmarkCB;
	*p++ = (XtArgVal)False;
	*p++ = (XtArgVal)hwp;
	*p++ = (XtArgVal)(hap->num_bmark == 0 ? False : True);

	*p++ = (XtArgVal)(Dm__gettxt(TXT_CANCEL_STR));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_CANCEL_STR);
	*p++ = (XtArgVal)BmarkCancelCB;
	*p++ = (XtArgVal)False;
	*p++ = (XtArgVal)hwp->bmark_shell;
	*p++ = (XtArgVal)True;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_HELP_ELLIPSIS));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_HELP_STR);
	*p++ = (XtArgVal)BmarkHelpCB;
	*p++ = (XtArgVal)False;
	*p++ = (XtArgVal)hwp;
	*p++ = (XtArgVal)True;

	XtSetArg(Dm__arg[0], XtNitems,         btn_items);
	XtSetArg(Dm__arg[1], XtNnumItems,      6);
	XtSetArg(Dm__arg[2], XtNitemFields,    btn_fields);
	XtSetArg(Dm__arg[3], XtNnumItemFields, XtNumber(btn_fields));
	XtSetArg(Dm__arg[4], XtNlayoutType,    OL_FIXEDROWS);
	XtSetArg(Dm__arg[5], XtNmeasure,       1);
	XtSetArg(Dm__arg[6], XtNclientData,    hwp);
	
	hwp->bmark_btns = XtCreateManagedWidget("buttons", flatButtonsWidgetClass,
					lca, Dm__arg, 7);

	/* register help */
	BmarkWinHelp.app_title = Dm__gettxt(TXT_PRODUCT_NAME);
	OlRegisterHelp(OL_WIDGET_HELP, hwp->bmark_shell, NULL, OL_DESKTOP_SOURCE,
		(XtPointer)&BmarkWinHelp);

	XtPopup(hwp->bmark_shell, XtGrabNone);
}	/* end of DmBookmarkCB */

/****************************procedure*header*****************************
 * Adds a bookmark for the current section, if one does not already exist.
 */
static void
AddBmarkCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	int cnt = 0;
	char *app_name;
	TokenPtr bmarks = NULL;
	TokenPtr bmarkp;
	TokenPtr p = NULL;
	DmHelpLocPtr    hlp;
	DmHelpBmarkPtr  bmp;
	DmBmarkLabelPtr blp;
	DmHelpAppPtr    hap;
	DmHelpWinPtr    hwp = (DmHelpWinPtr)client_data;

	hap = DmGetHelpApp(hwp->app_id);
	DmClearStatus((DmWinPtr)hwp);

	if (strcmp(hap->name, Dm__gettxt(TXT_DESKTOP_MGR)) == 0)
		app_name = hap->title;
	else
		app_name = hap->name;

	/* Check if bookmark already exists for the current section.
	 * Must check if sect_tag is set.
	 */
	for (bmp = hap->bmp; bmp; bmp = bmp->next) {
		if (strcmp(bmp->blp->app_name, app_name) == 0 &&
		    strcmp(bmp->blp->file_name, hwp->hfp->name) == 0 &&
		    strcmp(bmp->blp->sect_name, hwp->hsp->name) == 0 &&
			(bmp->blp->sect_tag && hwp->hsp->tag &&
		    strcmp(bmp->blp->sect_tag, hwp->hsp->tag) == 0)) {
				DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_BMARK_EXISTS);
				return;
			}
	}
	blp = (DmBmarkLabelPtr)CALLOC(1, sizeof(DmBmarkLabelRec)); 
	blp->app_name  = (XtPointer)strdup(app_name);
	blp->file_name = (XtPointer)strdup(hwp->hfp->name);

	if (hwp->hsp->name)
		blp->sect_name = (XtPointer)strdup(hwp->hsp->name);
	else
		blp->sect_name = NULL;

	if (hwp->hsp->tag)
		blp->sect_tag = (XtPointer)strdup(hwp->hsp->tag);
	else
		blp->sect_tag = NULL;

	bmp = (DmHelpBmarkPtr)CALLOC(1, sizeof(DmHelpBmarkRec));
	bmp->blp = blp;

	/* add bmp to the list */
	bmp->next = hap->bmp;
	hap->bmp = bmp;

	hap->num_bmark++;
	bmarks = bmarkp = (TokenPtr)CALLOC(hap->num_bmark, sizeof(TokenRec));

	for (bmp = hap->bmp; bmp; bmp = bmp->next, bmarkp++) {
		bmarkp->set        = (XtArgVal)False;
		bmarkp->formatData = (XtArgVal)bmp->blp;
		bmarkp->userData   = (XtArgVal)bmp;
	}

	XtSetArg(Dm__arg[0], XtNitems,    bmarks);
	XtSetArg(Dm__arg[1], XtNnumItems, hap->num_bmark);
	XtSetValues(hwp->bmark_flist, Dm__arg, 2);

	XtVaGetValues(hwp->bmark_shell, XtNuserData, &p, NULL);
	if (p)
		FREE((void *)p);
	XtVaSetValues(hwp->bmark_shell, XtNuserData, bmarks, NULL);

	/*
	 * Activate the Go To, Delete and Delete All buttons.
	 */
	XtSetArg(Dm__arg[0], XtNsensitive, True);
	OlFlatSetValues(hwp->bmark_btns, 0, Dm__arg, 1);
	OlFlatSetValues(hwp->bmark_btns, 2, Dm__arg, 1);
	OlFlatSetValues(hwp->bmark_btns, 3, Dm__arg, 1);

	PopdownShell(hwp->bmark_shell);

	/* update bookmark file */
	UpdateBmarkFile(hap);

}	/* end of AddBmarkCB */

/****************************procedure*header*****************************
 * Updates the bookmark file.
 */
static void
UpdateBmarkFile(hap)
DmHelpAppPtr	hap;
{
	int  len;
	int  fd;
	FILE *file;
	char buf[PATH_MAX];
	char lang[64];
	DmHelpBmarkPtr	bmp;
	struct stat	hstat;

	/* create directory in which bookmark file resides if it doesn't exist */ 
	len = sprintf(lang, "%s", getenv("LANG"));

	if (len == 0)
		strcpy(lang, "C");
	else {
		if (strchr(lang, '\n'))
			lang[len - 1] = '\0';
	}

	/* Save bookmarks separately for the different modules of dtm. */
	if (strcmp(hap->name, Dm__gettxt(TXT_DESKTOP_MGR)) == 0) {
		sprintf(buf, "%s/.dthelp/.%s/.%s", DESKTOP_DIR(Desktop), lang,
			hap->title);
	} else
		sprintf(buf, "%s/.dthelp/.%s/.%s", DESKTOP_DIR(Desktop), lang,
			hap->name);

	if (stat(buf, &hstat) != 0) {
		if (mkdirp(buf, 0755) == -1) {
			Dm__VaPrintMsg(TXT_MKDIR, buf);
			return;
		}

		/* create bookmark file */
		strcat(buf, "/.bookmark");
		if (creat(buf, 0644) == -1) {
			Dm__VaPrintMsg(TXT_TOUCH, buf);
			return;
		}
	} else {
		/* create bookmark file if it does not exist */
		if (strcmp(hap->name, Dm__gettxt(TXT_DESKTOP_MGR)) == 0) {
			sprintf(buf, "%s/.dthelp/.%s/.%s/.bookmark",
				DESKTOP_DIR(Desktop), lang, hap->title);
		} else
			sprintf(buf, "%s/.dthelp/.%s/.%s/.bookmark",
				DESKTOP_DIR(Desktop), lang, hap->name);

		if (stat(buf, &hstat) != 0) {
			if (creat(buf, 0644) == -1) {
				Dm__VaPrintMsg(TXT_TOUCH, buf);
				return;
			}
		}
	}

	if ((fd = open(buf, O_WRONLY | O_TRUNC | O_CREAT, 0644)) == -1) {
		Dm__VaPrintMsg(TXT_CANT_ACCESS_BMARK_FILE, buf);
		return;
	}
	if (lockf(fd, F_LOCK, 0L) == -1) {
err:
		Dm__VaPrintMsg(TXT_LOCK_FILE, buf, errno);
		close(fd);
		return;
	} 
	if ((file = fdopen(fd, "w")) == NULL) {
		goto err;
	}

	/*
	 * blp->app_name is also used to distinguish between the different
	 * modules of dtm.
      */

	/* write bookmarks to disk */
	for (bmp = hap->bmp; bmp; bmp = bmp->next) {
		fprintf(file, "%s^%s^%s^%s^\n", bmp->blp->app_name,
			bmp->blp->file_name, (bmp->blp->sect_tag ? bmp->blp->sect_tag :
			""), bmp->blp->sect_name);
	}
	fclose(file);

}	/* end of UpdateBmarkFile */

/****************************procedure*header*****************************
 * Jump to a selected bookmark.
 */
static void
BtnGoToBmarkCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	int i;
	TokenPtr bmarks;
	TokenPtr p;
	DmBmarkLabelPtr blp;
	DmHelpBmarkPtr  bmp;
	DmHelpWinPtr    hwp = (DmHelpWinPtr)client_data;
	DmHelpAppPtr    hap = DmGetHelpApp(hwp->app_id);

	DmClearStatus((DmWinPtr)hwp);
	if (hap->num_bmark == 0)
		return;

	XtSetArg(Dm__arg[0], XtNuserData, &bmarks);
	XtGetValues(hwp->bmark_shell, Dm__arg, 1);

	for (p = bmarks, i = 0; i < hap->num_bmark; i++, p++) {
		if (p->set) {
			bmp = (DmHelpBmarkPtr)(p->userData);
			DmDisplayHelpSection(hwp, hwp->app_id, hwp->hfp->title,
				bmp->blp->file_name, (bmp->blp->sect_tag ?
				bmp->blp->sect_tag : bmp->blp->sect_name),
				UNSPECIFIED_POS, UNSPECIFIED_POS);
			PopdownShell(hwp->bmark_shell);
			return;
		}
	}
	DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NO_BMARK_TO_GOTO);

}	/* end of BtnGoToBmarkCB */

/****************************procedure*header*****************************
 * Jump to the bookmark that was double-clikcked on in the list.
 */
static void
LstGotoBmarkCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	DmBmarkLabelPtr blp;
	DmHelpWinPtr    hwp = (DmHelpWinPtr)client_data;
	OlFlatCallData  *fcd = (OlFlatCallData *)call_data;

	DmClearStatus((DmWinPtr)hwp);

	XtSetArg(Dm__arg[0], XtNformatData, &blp);
	OlFlatGetValues(w, fcd->item_index, Dm__arg, 1);

	DmDisplayHelpSection(hwp, hwp->app_id, hwp->hfp->title, blp->file_name,
		(blp->sect_tag ? blp->sect_tag : blp->sect_name), UNSPECIFIED_POS,
		UNSPECIFIED_POS);

	PopdownShell(hwp->bmark_shell);
}	/* end of LstGotoBmarkCB */

/****************************procedure*header*****************************
 * Deletes one or all bookmarks depending on whether the Delete or
 * Delete All button was selected.
 */
static void
DelBmarkCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	int i;
	TokenPtr bmarks = NULL;
	TokenPtr bmarkp;
	TokenPtr p;
	DmHelpBmarkPtr	bmp;
	Boolean set = False;
	Boolean found = False;
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;
	DmHelpAppPtr hap = DmGetHelpApp(hwp->app_id);

	DmClearStatus((DmWinPtr)hwp);

	if (hap->num_bmark == 0)
		return;

	XtSetArg(Dm__arg[0], XtNuserData, &bmarks);
	XtGetValues(hwp->bmark_shell, Dm__arg, 1);

	for (p = bmarks, i = 0; i < hap->num_bmark; i++, p++) {
		if (p->set) {
			/* delete bookmark */
			DelBmark(hap, (DmHelpBmarkPtr)(p->userData), False);
			found = True;
		}
	}

	if (!found) { /* no bookmark was deleted */
		DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NO_BMARK_TO_DELETE);
		return;
	}

	if (hap->num_bmark > 0) {
		bmarks = bmarkp = (TokenPtr)CALLOC(hap->num_bmark, sizeof(TokenRec));
		for (bmp = hap->bmp; bmp; bmp = bmp->next, bmarkp++) {
			bmarkp->set           = (XtArgVal)False;
			bmarkp->formatData    = (XtArgVal)bmp->blp;
			bmarkp->userData      = (XtArgVal)bmp;
		}
		XtSetArg(Dm__arg[0], XtNitems, bmarks);
	} else
		XtSetArg(Dm__arg[0], XtNitems, NULL);
	XtSetArg(Dm__arg[1], XtNnumItems,  hap->num_bmark);
	XtSetValues(hwp->bmark_flist, Dm__arg, 2);

	XtVaGetValues(hwp->bmark_shell, XtNuserData, &p, NULL);
	if (p)
		FREE((void *)p);
	XtVaSetValues(hwp->bmark_shell, XtNuserData, (hap->num_bmark == 0 ?
		NULL : bmarks), NULL);

	/* 
	 * Deactivate Go To, Delete and Delete All buttons.
	 */
	if (hap->num_bmark == 0) {
		XtSetArg(Dm__arg[0], XtNsensitive, False);
		OlFlatSetValues(hwp->bmark_btns, 0, Dm__arg, 1);
		OlFlatSetValues(hwp->bmark_btns, 2, Dm__arg, 1);
		OlFlatSetValues(hwp->bmark_btns, 3, Dm__arg, 1);
	}
	PopdownShell(hwp->bmark_shell);

}	/* end of DelBmarkCB */

/****************************procedure*header*****************************
 * Deletes all bookmarks if all is True, in which case bmp is NULL.
 * If all is False, delete bmp.
 */
static void
DelBmark(DmHelpAppPtr hap, DmHelpBmarkPtr bmp, Boolean all)
{
	if (all) {
		/* free bookmark resources */
		FreeAllBmarks(hap);
		UpdateBmarkFile(hap);

	} else {
		register DmHelpBmarkPtr	tbmp = hap->bmp;

		if (tbmp == bmp) {
			hap->bmp = bmp->next;
			FreeBmark(bmp);
			UpdateBmarkFile(hap);
			hap->num_bmark--;

		} else {
			for (; tbmp->next; tbmp = tbmp->next) {
				if (tbmp->next == bmp) {
					tbmp->next = bmp->next;
					/* free bookmark resources */
					FreeBmark(bmp);

					/* update bookmark file */
					UpdateBmarkFile(hap);
					hap->num_bmark--;
					return;
				}
			}
		}
	}
}	/* end of DelBmark */
			
/****************************procedure*header*****************************
 * Deletes all existing bookmarks for the current help file.
 */
static void
DelAllBmarkCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	TokenPtr p = NULL;
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;
	DmHelpAppPtr hap = DmGetHelpApp(hwp->app_id);

	DmClearStatus((DmWinPtr)hwp);
	DelBmark(hap, NULL, True);

	XtSetArg(Dm__arg[0], XtNitems,         NULL);
	XtSetArg(Dm__arg[1], XtNnumItems,      0);
	XtSetValues(hwp->bmark_flist, Dm__arg, 2);

	XtVaGetValues(hwp->bmark_shell, XtNuserData, &p, NULL);
	if (p)
		FREE((void *)p);
	XtVaSetValues(hwp->bmark_shell, XtNuserData, NULL, NULL);

	/* 
	 * Deactivate Go To, Delete and Delete All buttons.
	 */
	XtSetArg(Dm__arg[0], XtNsensitive, False);
	OlFlatSetValues(hwp->bmark_btns, 0, Dm__arg, 1);
	OlFlatSetValues(hwp->bmark_btns, 2, Dm__arg, 1);
	OlFlatSetValues(hwp->bmark_btns, 3, Dm__arg, 1);
	PopdownShell(hwp->bmark_shell);

}	/* end of DelAllBmarkCB */

/****************************procedure*header*****************************
 * Free all bookmark resources.
 */
static void
FreeAllBmarks(hap)
DmHelpAppPtr hap;
{
	register DmHelpBmarkPtr save;
	register DmHelpBmarkPtr bmp = hap->bmp;

	while (bmp) {
		save = bmp->next;
		FreeBmark(bmp);
		bmp = save;
	}
	hap->bmp = NULL;
	hap->num_bmark = 0;

}	/* end of FreeAllBmarks */

/****************************procedure*header*****************************
 * Free bookmark resources associated with bmp.
 */
static void
FreeBmark(bmp)
DmHelpBmarkPtr	bmp;
{
	FREE((void *)(bmp->blp->app_name));
	FREE((void *)(bmp->blp->file_name));
	FREE((void *)(bmp->blp->sect_name));
	FREE((void *)(bmp->blp->sect_tag));
	FREE((void *)(bmp->blp));
	FREE((void *)bmp);

}	/* end of FreeBmark */

/****************************procedure*header*****************************
 * Pops up the Notes window.
 */
void
DmNotesCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	Widget uca;
	Widget lca;
	Widget swin;
	char buf[256];
	XtArgVal *p;
	XtArgVal *btn_items;
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;
	DmHelpAppPtr hap = DmGetHelpApp(hwp->app_id);

	DmClearStatus((DmWinPtr)hwp);

	if (hwp->notes_shell != NULL) {
		if (hwp->hfp->notesp)
			/* see if curent section has notes and display it if it does */
			DmReadNotes(hwp, hwp->hsp->tag ? hwp->hsp->tag:hwp->hsp->name);
		XtPopup(hwp->notes_shell, XtGrabNone);
		return;
	}

	sprintf(buf, "%s: %s", hap->title, Dm__gettxt(TXT_NOTES));
	XtSetArg(Dm__arg[0], XtNtitle, buf);
	XtSetArg(Dm__arg[1], XtNpushpin, OL_IN);
	hwp->notes_shell = XtCreatePopupShell("Notes",
					popupWindowShellWidgetClass, w, Dm__arg, 2);
	XtAddCallback(hwp->notes_shell, XtNpopdownCallback, NotesPopdnCB, hwp);

	XtSetArg(Dm__arg[0], XtNupperControlArea, &uca);
	XtSetArg(Dm__arg[1], XtNlowerControlArea, &lca);
	XtGetValues(hwp->notes_shell, Dm__arg, 2);

	swin = XtCreateManagedWidget("swin", scrolledWindowWidgetClass,
					uca, NULL, 0);

	XtSetArg(Dm__arg[0], XtNcursorPosition, 0);
	XtSetArg(Dm__arg[1], XtNselectStart,    0);
	XtSetArg(Dm__arg[2], XtNselectEnd,      0);

	hwp->notes_te = XtCreateManagedWidget("notes_te", textEditWidgetClass,
					swin, Dm__arg, 3);

	XtAddCallback(hwp->notes_te, XtNpostModifyNotification, NotesChgCB, hwp);

	btn_items = p = (XtArgVal *)MALLOC(sizeof(XtArgVal) * 6 * 4);

	*p++ = (XtArgVal)(Dm__gettxt(TXT_HW_SAVE_STR));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_HW_SAVE_STR);
	*p++ = (XtArgVal)SaveNotesCB;
	*p++ = (XtArgVal)True;
	*p++ = (XtArgVal)hwp;
	*p++ = (XtArgVal)True;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_HW_DELETE_STR));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_HW_DELETE_STR);
	*p++ = (XtArgVal)DelNotesCB;
	*p++ = (XtArgVal)False;
	*p++ = (XtArgVal)hwp;
	*p++ = (XtArgVal)True;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_CANCEL_STR));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_CANCEL_STR);
	*p++ = (XtArgVal)NotesCancelCB;
	*p++ = (XtArgVal)False;
	*p++ = (XtArgVal)hwp->notes_shell;
	*p++ = (XtArgVal)True;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_HELP_ELLIPSIS));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_HELP_STR);
	*p++ = (XtArgVal)NotesHelpCB;
	*p++ = (XtArgVal)False;
	*p++ = (XtArgVal)hwp;
	*p++ = (XtArgVal)True;

	XtSetArg(Dm__arg[0], XtNitems,         btn_items);
	XtSetArg(Dm__arg[1], XtNnumItems,      4);
	XtSetArg(Dm__arg[2], XtNitemFields,    btn_fields);
	XtSetArg(Dm__arg[3], XtNnumItemFields, XtNumber(btn_fields));
	XtSetArg(Dm__arg[4], XtNlayoutType,    OL_FIXEDROWS);
	XtSetArg(Dm__arg[5], XtNmeasure,       1);
	XtSetArg(Dm__arg[6], XtNclientData,    hwp);
	
	hwp->notes_btns = XtCreateManagedWidget("buttons", flatButtonsWidgetClass,
					lca, Dm__arg, 7);

	/* check if notes exists for current section */
	DmReadNotes(hwp, hwp->hsp->tag ? hwp->hsp->tag : hwp->hsp->name);

	/* register help */
	NotesWinHelp.app_title = Dm__gettxt(TXT_PRODUCT_NAME);
	OlRegisterHelp(OL_WIDGET_HELP, hwp->notes_shell, NULL, OL_DESKTOP_SOURCE,
		(XtPointer)&NotesWinHelp);

	XtPopup(hwp->notes_shell, XtGrabNone);

}	/* end of DmNotesCB */

/****************************procedure*header*****************************
 * Saves notes in Notes window in notes file.
 */
static int
SaveNotesCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	SaveResult save_result;
	TextBuffer *text_buffer;
	char buf[PATH_MAX];
	char lang[32];
	char *tfile = NULL;
	char *app_name;
	int  len;
	DmHelpNotesPtr	np;
	DmHelpAppPtr	hap;
	DmHelpWinPtr	hwp = (DmHelpWinPtr)client_data;

	DmClearStatus((DmWinPtr)hwp);

	text_buffer = OlTextEditTextBuffer(hwp->notes_te);

	if (!hwp->hsp->notes_chged) {
	    DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NO_SAVE_CHANGES);
	    return(-1);
	}
	hwp->hsp->notes_chged = False;
	hap = DmGetHelpApp(hwp->app_id);

	len = sprintf(lang, "%s", getenv("LANG"));

	if (len == 0)
		strcpy(lang, "C");
	else {
		if (strchr(lang, '\n'))
			lang[len - 1] = '\0';
	}

	if (strcmp(hap->name, Dm__gettxt(TXT_DESKTOP_MGR)) == 0)
		app_name = hap->title;
	else
		app_name = hap->name;
	sprintf(buf, "%s/.dthelp/.%s/.%s/.notes", DESKTOP_DIR(Desktop), lang,
		app_name);

	if (hwp->hfp->notes == NULL) {
		struct stat hstat;

		/* create the notes directory if it doesn't exist */
		if (stat(buf, &hstat) != 0) {
			if (mkdirp(buf, 0755) == -1) {
				DmVaDisplayStatus((DmWinPtr)hwp, True,
						TXT_NOTES_SAVED_FAIL);
				return(-1);
			}
		}
		sprintf(buf, "%s/.dthelp/.%s/.%s/.notes/%s", DESKTOP_DIR(Desktop),
			lang, app_name, basename(hwp->hfp->name));
		hwp->hfp->notes = strdup(buf);
	}

	/*
	 * Check if notes already exists for the section.
	 * Note that np->sect_tag can be NULL.
	 */
	for (np = hwp->hfp->notesp; np; np = np->next) {
		if ((np->sect_tag && hwp->hsp->tag &&
			strcmp(np->sect_tag, hwp->hsp->tag) == 0) ||
			(np->sect_name && hwp->hsp->name &&
			strcmp(np->sect_name, hwp->hsp->name) == 0)) {

				tfile = strdup(np->notes_file);
				break;
		}
	}
	
	if (!tfile) {
		sprintf(buf, "%s/.dthelp/.%s/.%s/.notes/", DESKTOP_DIR(Desktop),
			lang, app_name);
		tfile = tempnam(buf, NULL);
	}

	np = (DmHelpNotesPtr)CALLOC(1, sizeof(DmHelpNotesRec));
	np->notes_file = strdup(tfile);

	if (hwp->hsp->name)
		np->sect_name  = strdup(hwp->hsp->name);
	else
		np->sect_name  = NULL;

	if (hwp->hsp->tag)
		np->sect_tag   = strdup(hwp->hsp->tag);
	else
		np->sect_tag   = NULL;

	/* add notes to the list */
	np->next = hwp->hfp->notesp;
	hwp->hfp->notesp = np;

	save_result = SaveTextBuffer(text_buffer, tfile);
	if (save_result == SAVE_SUCCESS)
	{
		/* SaveTextBuffer() is not checking if the notes file
		 * is writable.
		 */
		DmVaDisplayStatus((DmWinPtr)hwp, False, TXT_NOTES_SAVED);
		UpdateNotesFile(hwp->hfp);
		PopdownShell(hwp->notes_shell);

		/* activate Delete button */
		OlVaFlatSetValues(hwp->notes_btns, 1, XtNsensitive, True, NULL);

		FREE((void *)tfile);
		return(0);

	} else {
		DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NOTES_SAVED_FAIL);
		FREE((void *)tfile);
		return(-1);
	}
}	/* end of SaveNotesCB */

/****************************procedure*header*****************************
 * Updates master note file for help file for the application.
 */
static int
UpdateNotesFile(hfp)
DmHelpFilePtr hfp;
{
	int fd;
	FILE *file;
	struct stat hstat;
	DmHelpNotesPtr	np;

	/* create notes file if it does not exist */
	errno = 0;
	if (stat(hfp->notes, &hstat) != 0) {
		if (errno == ENOENT) {

			if (creat(hfp->notes, 0644) == -1) {
				Dm__VaPrintMsg(TXT_TOUCH, hfp->notes);
				return;
			}
		}
	}

	if ((fd = open(hfp->notes, O_WRONLY | O_TRUNC | O_CREAT, 0644)) == -1) {
		Dm__VaPrintMsg(TXT_CANT_OPEN_NOTES_FILE, hfp->notes);
		return;
	}
	if (lockf(fd, F_LOCK, 0L) == -1) {
err:
		Dm__VaPrintMsg(TXT_LOCK_FILE, hfp->notes, errno);
		close(fd);
		return;
	} 
	if ((file = fdopen(fd, "w")) == NULL) {
		goto err;
	}

	/* start writing to notes file */
	for (np = hfp->notesp; np; np = np->next) {
		fprintf(file, "%s^%s^%s^\n", (np->sect_tag ? np->sect_tag : ""),
			np->sect_name ? np->sect_name : "", np->notes_file);
	}

	fclose(file);

}	/* end of UpdateNotesFile */

/****************************procedure*header*****************************
 * Delete notes file previously saved.
 */
static void
DelNotes(hwp, np)
DmHelpWinPtr   hwp;
DmHelpNotesPtr	np;
{
	register DmHelpNotesPtr	tnp = hwp->hfp->notesp;

	if (tnp == np) {
		hwp->hfp->notesp = np->next;
		UpdateNotesFile(hwp->hfp);

		/*deactivate Delete button */
		OlVaFlatSetValues(hwp->notes_btns, 1, XtNsensitive, False, NULL);
		
	} else {
		for (; tnp->next; tnp = tnp->next) {
			if (tnp->next == np) {
				tnp->next = np->next;
				UpdateNotesFile(hwp->hfp);

				/*deactivate Delete button */
				OlVaFlatSetValues(hwp->notes_btns, 1, XtNsensitive,
					False, NULL);
				return;
			}
		}
		Dm__VaPrintMsg(TXT_CANT_FIND_NOTES_FILE);
	}
} /* end of DelNotes */

/****************************procedure*header*****************************
 * Computes geometry of static text widget in scrolled window in a
 * definition window.
 */
static void
SwinComputeGeom(w, geom)
Widget		w;
OlSWGeometries	*geom;
{
	int		cnt;
	Dimension	width,
			height;

     cnt = 0;
     XtSetArg(Dm__arg[cnt], XtNwidth,  &width); cnt++;
     XtSetArg(Dm__arg[cnt], XtNheight, &height); cnt++;
     XtGetValues(w, Dm__arg, cnt);

     /* check if scrollbars are needed */
     if (width > 350)
         geom->force_hsb = True;
     else
         geom->force_hsb = False;
     if (height > 100)
         geom->force_vsb = True;
     else
         geom->force_vsb = False;

     if (geom->force_vsb)
         geom->sw_view_width =
          geom->bbc_width = 350 - geom->vsb_width;
     else
         geom->sw_view_width = geom->bbc_width = 350;
     geom->bbc_real_width = width;

     if (geom->force_hsb)
         geom->sw_view_height =
          geom->bbc_height = 100 - geom->hsb_height;
     else
         geom->sw_view_height = geom->bbc_height = 100;
     geom->bbc_real_height = height;

}	/* end of SwinComputeGeom */

/****************************procedure*header*****************************
 * Pops up a window to display definition of a selected term.
 */
static void
DisplayDef(hwp, term, def)
DmHelpWinPtr hwp;
char *term;
char *def;
{
	static Widget stext;
	Widget swin;
	Widget uca;
	Widget lca;
	Widget buttons;

	XtArgVal *p;
	XtArgVal *btn_items;

	char tl_buf[256];
	char def_buf[PATH_MAX];
	char *defn = NULL;
	int  width;
	int  height;

	DmHelpAppPtr hap = DmGetHelpApp(hwp->app_id);
	DmClearStatus((DmWinPtr)hwp);

	if (hwp->def_shell != NULL) {
		/* free the previous definition */
		XtSetArg(Dm__arg[0], XtNuserData, &defn);
		XtGetValues(hwp->def_shell, Dm__arg, 1);

		if (defn)
			free(defn);

		defn = (char *)MALLOC(sizeof(char) * (strlen(def) + 4));
		strcpy(defn, "\n");
		strcat(defn, def);
		strcat(defn, "\n");

		XtSetArg(Dm__arg[0], XtNstring, defn);
		XtSetValues(stext, Dm__arg, 1);

		XtSetArg(Dm__arg[0], XtNuserData, defn);
		XtSetValues(hwp->def_shell, Dm__arg, 1);

		sprintf(tl_buf, "%s: %s%s", hap->title,
			Dm__gettxt(TXT_DEF_OF), term);
		XtSetArg(Dm__arg[0], XtNtitle, tl_buf);
		XtSetValues(hwp->def_shell, Dm__arg, 1);

		XtPopup(hwp->def_shell, XtGrabNone);
		return;
	}

	sprintf(tl_buf, "%s: %s%s", hap->title, Dm__gettxt(TXT_DEF_OF),
		term);
	XtSetArg(Dm__arg[0], XtNtitle, tl_buf);
	hwp->def_shell = XtCreatePopupShell("shell",
					popupWindowShellWidgetClass, hwp->shell, Dm__arg, 1); 
	XtAddCallback(hwp->def_shell, XtNpopdownCallback, DefPopdnCB, hwp);

	XtSetArg(Dm__arg[0], XtNupperControlArea, (XtArgVal)&uca);
	XtSetArg(Dm__arg[1], XtNlowerControlArea, (XtArgVal)&lca);
	XtGetValues(hwp->def_shell, Dm__arg, 2);

	XtSetArg(Dm__arg[0], XtNhStepSize, &width);
	XtSetArg(Dm__arg[1], XtNvStepSize, &height);
	XtGetValues(hwp->swin, Dm__arg, 2);

	XtSetArg(Dm__arg[0], XtNviewWidth,  DEFAULT_HELPFILE_WIDTH * width);
	XtSetArg(Dm__arg[1], XtNviewHeight, 10 * height);
	XtSetArg(Dm__arg[2], XtNhStepSize,  height);
	XtSetArg(Dm__arg[3], XtNvStepSize,  width);
	XtSetArg(Dm__arg[4], XtNstopPosition, OL_GRANULARITY);
	swin = XtCreateManagedWidget("swin", scrolledWindowWidgetClass,
					uca, Dm__arg, 5);

	defn = (char *)MALLOC(sizeof(char) * (strlen(def) + 4));
	strcpy(defn, "\n");
	strcat(defn, def);
	strcat(defn, "\n");

	XtSetArg(Dm__arg[0], XtNstring,  defn);
	XtSetArg(Dm__arg[1], XtNgravity, NorthWestGravity);
	stext = XtCreateManagedWidget("stext", staticTextWidgetClass,
					swin, Dm__arg, 2);

	XtSetArg(Dm__arg[0], XtNuserData, defn);
	XtSetValues(hwp->def_shell, Dm__arg, 1);

	btn_items = p = (XtArgVal *)MALLOC(sizeof(XtArgVal) * 6 * 2);

	*p++ = (XtArgVal)(Dm__gettxt(TXT_CANCEL_STR));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_CANCEL_STR);
	*p++ = (XtArgVal)DefCancelCB;
	*p++ = (XtArgVal)True;
	*p++ = (XtArgVal)NULL;
	*p++ = (XtArgVal)True;

	*p++ = (XtArgVal)(Dm__gettxt(TXT_HELP_ELLIPSIS));
	*p++ = (XtArgVal)*Dm__gettxt(TXT_M_HELP_STR);
	*p++ = (XtArgVal)DefHelpCB;
	*p++ = (XtArgVal)False;
	*p++ = (XtArgVal)NULL;
	*p++ = (XtArgVal)True;

	XtSetArg(Dm__arg[0], XtNitems,         btn_items);
	XtSetArg(Dm__arg[1], XtNnumItems,      2);
	XtSetArg(Dm__arg[2], XtNitemFields,    btn_fields);
	XtSetArg(Dm__arg[3], XtNnumItemFields, XtNumber(btn_fields));
	XtSetArg(Dm__arg[4], XtNlayoutType,    OL_FIXEDROWS);
	XtSetArg(Dm__arg[5], XtNmeasure,       1);
	XtSetArg(Dm__arg[6], XtNclientData,    hwp);
	
	buttons = XtCreateManagedWidget("buttons", flatButtonsWidgetClass,
				lca, Dm__arg, 7);

	/* register help */
	DefWinHelp.app_title = Dm__gettxt(TXT_PRODUCT_NAME);
	OlRegisterHelp(OL_WIDGET_HELP, hwp->def_shell, NULL, OL_DESKTOP_SOURCE,
		(XtPointer)&DefWinHelp);

	XtPopup(hwp->def_shell, XtGrabNone);

}	/* end of DisplayDef */

/****************************procedure*header*****************************
 * Calls DelNotes() to delete notes.
 */
void
DelNotesCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	DmHelpWinPtr	hwp = (DmHelpWinPtr)client_data;
	DmHelpNotesPtr	np = NULL;

	DmClearStatus((DmWinPtr)hwp);

	/* clear text pane regardless */
	XtSetArg(Dm__arg[0], XtNsource,         "");
	XtSetArg(Dm__arg[1], XtNsourceType,     OL_STRING_SOURCE);
	XtSetArg(Dm__arg[2], XtNcursorPosition, 0);
	XtSetArg(Dm__arg[3], XtNselectStart,    0);
	XtSetArg(Dm__arg[4], XtNselectEnd,      0);
	XtSetValues(hwp->notes_te, Dm__arg, 5);

	if (hwp->hfp->notesp == NULL) {
	    DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NO_NOTES_TO_DELETE);
	    return;

	} else {
		struct stat hstat;

		DmClearStatus((DmWinPtr)hwp);

		for (np = hwp->hfp->notesp; np; np = np->next) {
			if (strcmp(hwp->hsp->tag, np->sect_tag) == 0) {
				break;
			}
		}
		if (np == NULL) {
			DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NO_NOTES_TO_DELETE);
			return;
		}

		if (stat(np->notes_file, &hstat) == 0) {
			if (remove(np->notes_file) == 0) {
				XtSetArg(Dm__arg[0], XtNsource, "");
				XtSetArg(Dm__arg[1], XtNsourceType,
					OL_STRING_SOURCE);
				XtSetValues(hwp->notes_te, Dm__arg, 2);

				DelNotes(hwp, np);

				DmVaDisplayStatus((DmWinPtr)hwp, False, TXT_NOTES_DELETED);

				PopdownShell(hwp->notes_shell);
			} else {
			    DmVaDisplayStatus((DmWinPtr)hwp, True,
					TXT_NOTES_DELETE_FAIL);
			}
				
		} else {
		    DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NOTES_DELETE_FAIL);
		}
	}
}	/* end of DelNotesCB */

/****************************procedure*header*****************************
 * Callback for textedit widget used in Notes window to set flag which
 * indicates whether the text was modified.
 */
static void
NotesChgCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	hwp->hsp->notes_chged = True;
}	/* end of NotesChgCB */

/****************************procedure*header*****************************
 * Pops down popup shell if pushpin is not in and if OL_OPENLOOK_GUI.
 */
static void
PopdownShell(shell)
Widget	shell;
{
	OlDefine state;

	if (shell == NULL)
		return;

	if (OlGetGui() == OL_OPENLOOK_GUI) {
		XtSetArg(Dm__arg[0], XtNpushpin, &state);
		XtGetValues(shell, Dm__arg, 1);

		if (state == OL_OUT)
			XtPopdown(shell);
	}
}	/* end of PopdownShell */

/****************************procedure*header*****************************
 * Displays help on the Definition window.
 */
static void
DefHelpCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	DmHelpWinPtr	hwp = (DmHelpWinPtr)client_data;
	DmHelpAppPtr	hap = DmGetHelpApp(hwp->app_id);

	DmDisplayHelpSection(hwp, hwp->app_id, NULL, "DesktopMgr/help.hlp",
		"310", UNSPECIFIED_POS, UNSPECIFIED_POS);

}	/* end of DefHelpCB */

/****************************procedure*header*****************************
 * Displays help on the Glossary window.
 */
static void
GlossHelpCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	DmHelpWinPtr	hwp = (DmHelpWinPtr)client_data;
	DmHelpAppPtr	hap = DmGetHelpApp(hwp->app_id);

	DmDisplayHelpSection(hwp, hwp->app_id, NULL, "DesktopMgr/help.hlp",
		"340", UNSPECIFIED_POS, UNSPECIFIED_POS);

}	/* end of GlossHelpCB */

/****************************procedure*header*****************************
 * Displays help on the Bookmark window.
 */
static void
BmarkHelpCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	DmHelpWinPtr	hwp = (DmHelpWinPtr)client_data;
	DmHelpAppPtr	hap = DmGetHelpApp(hwp->app_id);

	DmDisplayHelpSection(hwp, hwp->app_id, NULL, "DesktopMgr/help.hlp",
		"150", UNSPECIFIED_POS, UNSPECIFIED_POS);

}	/* end of BmarkHelpCB */

/****************************procedure*header*****************************
 * Displays help on the Notes window.
 */
static void
NotesHelpCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	DmHelpWinPtr	hwp = (DmHelpWinPtr)client_data;
	DmHelpAppPtr	hap = DmGetHelpApp(hwp->app_id);

	DmDisplayHelpSection(hwp, hwp->app_id, NULL, "DesktopMgr/help.hlp",
		"260", UNSPECIFIED_POS, UNSPECIFIED_POS);

}	/* end of NotesHelpCB */

/****************************procedure*header*****************************
 * Displays help on the Search window.
 */
static void
SearchHelpCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	DmHelpWinPtr	hwp = (DmHelpWinPtr)client_data;
	DmHelpAppPtr	hap = DmGetHelpApp(hwp->app_id);

	DmDisplayHelpSection(hwp, hwp->app_id, NULL, "DesktopMgr/help.hlp",
		"220", UNSPECIFIED_POS, UNSPECIFIED_POS);

}	/* end of SearchHelpCB */

/****************************procedure*header*****************************
 * Displays help on help window.
 */
void
DmUsingHelpCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	char *file;
	DmHelpWinPtr	hwp = (DmHelpWinPtr)client_data;
	DmHelpAppPtr	hap = DmGetHelpApp(hwp->app_id);

	/* Need to do this so that if hap->help_dir is specified,
	 * DesktopMgr/help.hlp will still be found.
	 */
	file = XtResolvePathname(DESKTOP_DISPLAY(Desktop), "help",
			"DesktopMgr/help.hlp", NULL, NULL, NULL, 0, NULL);

	DmDisplayHelpSection(hwp, hwp->app_id, NULL, file, "10",
		UNSPECIFIED_POS, UNSPECIFIED_POS);

}	/* end of DmUsingHelpCB */

/****************************procedure*header*****************************
 * Clears help window's message area and calls DmHelpDeskCB.
 */
void
DmOpenHelpDeskCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;
	DmClearStatus((DmWinPtr)hwp);
	DmHelpDeskCB(NULL, NULL, NULL);

} /* end of DmOpenHelpDeskCB */

/****************************procedure*header*****************************
 * Cancel callback for Glossary window.
 */
static void
GlossCancelCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;
	XtPopdown(hwp->gloss_shell);

} /* end of GlossCancelCB */

/****************************procedure*header*****************************
 * Popdown callback for Glossary window.
 */
static void
GlossPopdnCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	char *str;
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	XtSetArg(Dm__arg[0], XtNuserData, &str);
	XtGetValues(hwp->gloss_shell, Dm__arg, 1);

	/* free glossary */
	if (str)
		free(str);

	XtUnmapWidget(hwp->gloss_shell);
	XtDestroyWidget(hwp->gloss_shell);
	hwp->gloss_shell = NULL;

} /* end of GlossPopdnCB */

/****************************procedure*header*****************************
 * Callback for Cancel button for Definition window.
 */
static void
DefCancelCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;
	XtPopdown(hwp->def_shell);
} /* end of DefCancelCB */

/****************************procedure*header*****************************
 * Callback for XtNpopdownCallback for Definition window.
 */
static void
DefPopdnCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	char *defn = NULL;
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	XtSetArg(Dm__arg[0], XtNuserData, &defn);
	XtGetValues(hwp->def_shell, Dm__arg, 1);

	if (defn)
		free(defn);

	XtUnmapWidget(hwp->def_shell);
	XtDestroyWidget(hwp->def_shell);
	hwp->def_shell = NULL;

} /* end of DefPopdnCB */

/****************************procedure*header*****************************
 * Callback for Cancel button for Bookmark window.
 */
static void
BmarkCancelCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;
	XtPopdown(hwp->bmark_shell);
} /* end of BmarkCancelCB */

/****************************procedure*header*****************************
 * Callback for XtNpopdownCallback for Bookmark window.
 */
static void
BmarkPopdnCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	TokenPtr bmarks = NULL;
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	XtVaGetValues(hwp->bmark_shell, XtNuserData, &bmarks, NULL);
	if (bmarks)
		FREE((void *)bmarks);

	XtUnmapWidget(hwp->bmark_shell);
	XtDestroyWidget(hwp->bmark_shell);
	hwp->bmark_shell = NULL;

} /* end of BmarkPopdnCB */

/****************************procedure*header*****************************
 * Callback for Cancel button for Search window.
 */
static void
SearchCancelCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;
	XtPopdown(hwp->search_shell);
} /* end of SearchCancelCB */

/****************************procedure*header*****************************
 * Callback for XtNpopdownCallback for Search window.
 */
static void
SearchPopdnCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	XtUnmapWidget(hwp->search_shell);
	XtDestroyWidget(hwp->search_shell);
	hwp->search_shell = NULL;

} /* end of SearchPopdnCB */

/****************************procedure*header*****************************
 * Callback for Cancel button for Notes window.
 */
static void
NotesCancelCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;
	XtPopdown(hwp->notes_shell);
} /* end of NotesCancelCB */

/****************************procedure*header*****************************
 * Callback for XtNpopdownCallback for Notes window.
 */
static void
NotesPopdnCB(w, client_data, call_data)
Widget	w;
XtPointer	client_data;
XtPointer	call_data;
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	XtUnmapWidget(hwp->notes_shell);
	XtDestroyWidget(hwp->notes_shell);
	hwp->notes_shell = NULL;

} /* end of NotesPopdnCB */

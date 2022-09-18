/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:fn_finder.c	1.83"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/OlCursors.h>
#include <Gizmos.h>
#include <BaseWGizmo.h>
#include <ChoiceGizm.h>
#include <LabelGizmo.h>
#include <ListGizmo.h>
#include <MenuGizmo.h>
#include <ModalGizmo.h>
#include <NumericGiz.h>
#include <PopupGizmo.h>
#include <InputGizmo.h>
#include <STextGizmo.h>
#include <FileGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "error.h"
#include "extern.h"
#include "fn_find.h"
#include "CListGizmo.h"
#include "StatGizmo.h"
#include "SWinGizmo.h"

static void	fn_CloseFoundCB(Widget, XtPointer, XtPointer);
static void	fn_MoreFindingCB(Widget, XtPointer, XEvent *, Boolean *);
static void 	fn_InitInfo();
static void 	FoundWinWMCB();
static char *	fn_transform();
static void	fn_FindCB();
static void	fn_AddPathToList();
static void	fn_getfind();
static void	fn_HelpCB();
static void	fn_FoundHelpCB();
static void	DmFindCancel();
static Widget	CreateFindWindow();

/*
 * Each member of the Where to Look: list 
 * will have the following structure.
 */
typedef struct pathrec * pathrecptr;

typedef struct pathrec {
	char * path;
	pathrecptr next;
} PathRec;

#define XA	XtArgVal
#define B_A	(XtPointer)DM_B_ANY
#define B_O	(XtPointer)DM_B_ONE
#define B_M	(XtPointer)DM_B_ONE_OR_MORE
#define B_U     (XtPointer)DM_B_UNDO

/*
 * Need defines for types in the Where to Look Field
 */
#define FN_ALLUSERS 1
#define FN_DISK1    2
#define FN_DISK2    3
#define FN_PATH     4

/*
 * Menu for the Base Find Window Menu Bar.
 */
MenuItems MainItems[] = {
  { True, TXT_FILE_FIND,TXT_M_FILE_FIND,NULL, fn_FindCB },
  { True, TXT_CANCEL,	TXT_M_CANCEL,	NULL, fn_FindCancelCB },
  { True, TXT_P_HELP,	TXT_M_HELP,	NULL, fn_HelpCB, NULL, NULL, False },
  { NULL }
};

MenuGizmo MainMenu = {
       	NULL,
       	"MainMenu",
       	NULL,
       	MainItems,
       	NULL,
       	NULL,
       	CMD,
       	OL_FIXEDROWS,
       	1
};


/* 
 * Menu definition for the Menu Bar at the
 * Top of the Found Window. "File" "Edit" "Help"
 */
MenuItems FileMenuItems[] = {
 { True, TXT_FILE_OPEN,   TXT_M_FILE_OPEN,   NULL, DmFileOpenCB,    B_M },
 { True, TXT_FILE_COPY,   TXT_M_FILE_COPY,   NULL, DmFileCopyCB,    B_M },
 { True, TXT_FILE_RENAME, TXT_M_FILE_RENAME, NULL, DmFileRenameCB,  B_O },
 { True, TXT_FILE_MOVE,   TXT_M_FILE_MOVE,   NULL, DmFileMoveCB,    B_M },
 { True, TXT_FILE_LINK,   TXT_M_FILE_LINK,   NULL, DmFileLinkCB,    B_M },
 { True, TXT_FILE_DELETE, TXT_M_FILE_DELETE, NULL, DmFileDeleteCB,  B_M },
 { True, TXT_FILE_PRINT,  TXT_M_FILE_PRINT,  NULL, DmFilePrintCB,   B_M },
 { True, TXT_FILE_PROP,   TXT_M_FILE_PROP,   NULL, DmFilePropCB,    B_M },
 { True, TXT_FILE_EXIT,   TXT_M_FILE_EXIT,   NULL, fn_CloseFoundCB, B_A },
 { NULL }
};

MenuItems EditMenuItems[] = {
 {True,  TXT_EDIT_SELECT,   TXT_M_EDIT_SELECT,   NULL, DmEditSelectAllCB,  B_A},
 {True,  TXT_EDIT_UNSELECT, TXT_M_EDIT_UNSELECT, NULL, DmEditUnselectAllCB,B_M},
 {True, TXT_EDIT_UNDO,     TXT_M_EDIT_UNDO,     NULL, DmEditUndoCB,       B_U},
 { NULL }
};

MenuItems HelpMenuItems[] = {
 { True, TXT_HELP_FOLDER,   TXT_M_HELP_FOLDER,   NULL, fn_FoundHelpCB, B_A },
 { True, TXT_HELP_TOC,      TXT_M_HELP_TOC,      NULL, DmHelpTOCCB,      B_A },
 { True, TXT_HELP_HELPDESK, TXT_M_HELP_HELPDESK, NULL, DmHelpDeskCB,     B_A },
 { NULL }
};

MENU("filemenu", FileMenu);
MENU("editmenu", EditMenu);
MENU("helpmenu", HelpMenu);

static MenuItems FoundMenuBarItems[] = {
 { True,  TXT_FILE, TXT_M_FILE, (void *)&FileMenu, DmMenuSelectCB},
 { True,  TXT_EDIT, TXT_M_EDIT, (void *)&EditMenu, DmMenuSelectCB},
 { True,  TXT_HELP, TXT_M_HELP, (void *)&HelpMenu, DmMenuSelectCB},
/*
 { True, TXT_HELP, TXT_M_HELP, NULL, fn_FoundHelpCB},
*/
 { 0 }
};

MENUBAR("menubar", FoundMenuBar);

Setting 	filepattern;

/*
 * Menu Items and Menu Definition for the checkboxes used 
 * to ignore blanks.
 */
MenuItems FIgnoreMenuItems[] =
        {
                {(XA)True, " ", NULL, NULL},
		{0}
        };

MenuItems SIgnoreMenuItems[] =
        {
                {(XA)True, " ", NULL, NULL},
		{0}
        };


MenuGizmo FIgnoreMenu=
        { NULL, "FBlank", "FBlank", FIgnoreMenuItems, NULL, NULL, CHK};

ChoiceGizmo  FIgnoreField= {NULL, "FCase", TXT_IGNORE_CASE, &FIgnoreMenu, &filepattern};


static MenuGizmo SIgnoreMenu=
        { NULL, "SBlank", "SBlank", SIgnoreMenuItems, NULL, NULL, CHK};

ChoiceGizmo  SIgnoreField= {NULL, "CCase", TXT_IGNORE_CASE, &SIgnoreMenu, &filepattern};


static InputGizmo fn_pattern = {
	NULL, "filename", "", NULL, &filepattern, 20
};

static InputGizmo fn_content = {
	NULL, "filecontent", "", NULL, &filepattern, 20
};

static GizmoRec controlarray1[] = {
	{InputGizmoClass,	&fn_pattern},
	{ChoiceGizmoClass,	&FIgnoreField},
};

static GizmoRec controlarray2[] = {
	{InputGizmoClass,	&fn_content},
	{ChoiceGizmoClass,	&SIgnoreField},
};

static LabelGizmo label1 = {
	NULL, "Label1", TXT_FILE_NAME,
	controlarray1, XtNumber (controlarray1),
	OL_FIXEDROWS, 1, NULL, 0, True
};

static LabelGizmo label2 = {
	NULL, "Label2", TXT_FILE_CONTENT,
	controlarray2, XtNumber (controlarray2),
	OL_FIXEDROWS, 1, NULL, 0, True
};

static ListHead currentItem = {
	(ListItem *)0, 0, 0
};

static ListHead previousItem = {
	(ListItem *)0, 0, 0
};

static ListHead initialItem = {
	(ListItem *)0, 0, 0
};

Setting ListSetting = {
	"listsetting",
	(XtPointer)&initialItem,
	(XtPointer)&currentItem,
	(XtPointer)&previousItem
};

/*
 * Flat List Gizmo
 */

static ListGizmo fnList = {
	NULL,
	"fnlist",
	TXT_WHERE_LOOK,
	(Setting *)&ListSetting,
	"%s",
	False,
	4,
	NULL,
	False
};

static GizmoRec listArray[] = {
	{ListGizmoClass,	&fnList},
};

static LabelGizmo fnlistlabel = {
	NULL, "", TXT_WHERE_LOOK, listArray, XtNumber (listArray),
	OL_FIXEDROWS, 1, 0, 0, True
};

static InputGizmo fn_other = {
	NULL, "otherpath", TXT_ALSO_LOOK, "", &filepattern, 20
};

Setting numpattern = {
        "numsetting",
        0,
        0,
        0
};

static NumericGizmo fn_updated = {
	NULL, "daysupdated", TXT_LAST_UPDATE, 0, 365, &numpattern, 5, TXT_UPDATED_DAYS 
};

static StaticTextGizmo FN_clist_label = {
        NULL,                   /* help */
        "clist_label",           /* name */
        TXT_IB_FILE_TYPE,        /* string */
        WestGravity             /* gravity */
};

static CListGizmo FN_clist = {
	"fnclist",		/* name */
        4,			/* view width */
	NULL,			/* required property */
	False,			/* file */
        True,			/* sys class */
	False,                  /* xenix class */
        False,                   /* usr class */
        True,                   /* overridden class */
        False,                   /* exclusives behavior */
	False,                   /* noneset behaviour */
        NULL,         /* select proc */
};

static GizmoRec controlarray3[] = {
        {StaticTextGizmoClass,  &FN_clist_label},
        {CListGizmoClass,       &FN_clist},
};

static LabelGizmo label3 = {
	NULL, "Label3", NULL,
	controlarray3, XtNumber (controlarray3),
	OL_FIXEDROWS, 1, NULL, 0, True
};

static SWinGizmo swin_gizmo = {
	"swin",			/* name */
};

static StatusGizmo status_gizmo = {
	"status",		/* name */
	50			/* left percent */
};

static GizmoRec found_gizmos[] = {
	{SWinGizmoClass,	&swin_gizmo},
	{StatusGizmoClass,	&status_gizmo},
};

static GizmoRec fnwindowarray[] = {
	{LabelGizmoClass,	&label1},
	{LabelGizmoClass,	&label2},
	{LabelGizmoClass,	&fnlistlabel},
	{InputGizmoClass,	&fn_other},
	{NumericGizmoClass,	&fn_updated},
	{LabelGizmoClass,	&label3},
};

static HelpInfo FindWinHelp =
{ TXT_G_FOLDER_TITLE, NULL, "DesktopMgr/folder.hlp", "700", NULL };

static PopupGizmo FindWindow = {
        &FindWinHelp,               /* help */
        "findwindow",               /* widget name */
        TXT_FIND_WINDOW,               /* title */
        &MainMenu,               /* menu */
        fnwindowarray,              /* gizmo array */
        XtNumber(fnwindowarray),    /* number of gizmos */
};

/*
 * Menu for the Popup containing the Stop Button.
 */
MenuItems StopItems[] = {
 { True,  TXT_STOP,   TXT_M_STOP, NULL, fn_StopCB},
 { NULL }
};

MenuGizmo StopMenu = {
	NULL,
	"stopMenu",
	NULL,
	StopItems,
	NULL,
	NULL,
	CMD,
	OL_FIXEDROWS,
	1
};

static StaticTextGizmo stoptext = {
	NULL,			/* help */
	"stoptext",	      /* name */
	TXT_STOP_TEXT,	      /* string */
	CenterGravity		  /* gravity */
};

static GizmoRec stoparray[] = {
	{StaticTextGizmoClass,	&stoptext},
};

static PopupGizmo StopNotice = {
	NULL,			/* help */
	"stopnotice",		    /* widget name */
	TXT_STOP_WINDOW,	       /* title */
	&StopMenu,		 /* menu */
	stoparray,		/* gizmo array */
	XtNumber(stoparray),	/* number of gizmos */
};

static HelpInfo FoundWinHelp =
{ TXT_G_FOLDER_TITLE, NULL, "DesktopMgr/folder.hlp", "795", NULL };

static BaseWindowGizmo FoundWindow = {
	&FoundWinHelp,		/* help */
	"foundwindow",		/* shell widget name */
	TXT_FOUND_TITLE,	/* title */
	&FoundMenuBar,		/* menu bar */
	found_gizmos,		/* gizmo array */
	XtNumber(found_gizmos), /* # of gizmos in array */
	"",			/* icon_name */
	"",			/* name of pixmap file */
	" ",			/* error message */
	" ",			/* status message */
	75			/* size of error message in percent of footer */
};

/* Find Information Structure:
 * This structure contains all the information that is
 * relavant to one instance of the finder function. Since it
 * is possible for several folders to invoke a find, each
 * time a user selects the find button from a find window 
 * a FindInfoStruct is allocated. 
 */

typedef struct {
	DmFolderWinPtr window; 		/* Found Window */
	XtIntervalId    timeout_id;	/* id returned from add timeout */
	FILE 		* fptr;		/* file ptr of the pipe from popen */
	Boolean 	Complete_Flag;  /* flag indicating find op cmpleted */
	int		yposition; 	/* stores current Found Win y position*/
	DmFolderWinPtr 	FolderWin;      /* The Folder Window invoking find */
	char 		* lastfile;	/* left-over portion of last filename*/
	int		maxlen;		/* last max filename length */
	PopupGizmo	* stopnotice;	/* A popup gizmo is used for stop not*/
	char		* disk1;	/* Store the name of mount point     */
	char		* disk2; 	/* Store the name of mount point     */
} FindInfoStruct, *FindInfoPtr;



/****************************procedure*header*****************************
    DmFindCB - callback for bringing up a Find Prompt Box.  This callback
               is invoked if a user selects the find... button under the file
               menu.  The callback simply calls the CreateFindWindow routine
               which builds a Find Prompt Box.
*/
void
DmFindCB(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
	DmFolderWindow folder = (DmFolderWindow)DmGetWinPtr(widget);

	/* This could take ahile so entertain user */
	BUSY_CURSOR(folder->box);
	CreateFindWindow(folder);
}

/****************************procedure*header*****************************
    fn_CloseFoundCB-
*/
static void
fn_CloseFoundCB(Widget w, XtPointer client_data, XtPointer call_data)
{
        FindInfoStruct * info = (FindInfoStruct *)client_data;

	DmBringDownIconMenu(info->window);

	if (!info->Complete_Flag)
	{
		XtRemoveTimeOut(info->timeout_id);
                fn_pkill(info->fptr);
		info->Complete_Flag = True;
	}

	if (info->window->copy_prompt != NULL)
		FreeGizmo(FileGizmoClass, info->window->copy_prompt);

	if (info->window->rename_prompt != NULL)
		FreeGizmo(PopupGizmoClass, info->window->rename_prompt);

	if (info->window->move_prompt != NULL) 
		FreeGizmo(FileGizmoClass, info->window->move_prompt);

	if (info->window->link_prompt != NULL) 
		FreeGizmo(FileGizmoClass, info->window->link_prompt);

	FreeGizmo(PopupGizmoClass, info->stopnotice);
        DmCloseContainer(info->window->cp, DM_B_NO_FLUSH);
        DmDestroyIconContainer(info->window->shell, info->window->box, 
				info->window->itp, info->window->nitems);
	if (info->disk1){
		DtamUnMount(info->disk1);
		FREE(info->disk1);
	}

	if (info->disk2){
		DtamUnMount(info->disk2);
		FREE(info->disk2);
	}

	FREE((void *)info);
}					/* end of fn_CloseFoundCB */

/****************************procedure*header*****************************
    fn_InitInfo - routine is invoked each time the user selects the Find
                  button in the Find Prompt Box.  It is called from fn_FindCB
		  and is used to initialize the variables in the FindInfoStruct.
*/
static void
fn_InitInfo(FindInfoPtr * info) 
{
	/*
	 * Allocate the needed memory to carry the information
	 * needed.
	 */
   	*info = (FindInfoPtr) MALLOC (sizeof(FindInfoStruct));
	
	/* 
	 * Not guaranteed variables initialized to 0
	 * so they have to be initialized here.
	 */
	((FindInfoPtr)(*info))->lastfile = NULL;
	((FindInfoPtr)(*info))->Complete_Flag = False;
	((FindInfoPtr)(*info))->yposition = 
			    Ol_PointToPixel(OL_HORIZONTAL, ICON_MARGIN);
	((FindInfoPtr)(*info))->maxlen = 0;
	((FindInfoPtr)(*info))->disk1  = NULL;
	((FindInfoPtr)(*info))->disk2  = NULL;
}

/****************************procedure*header*****************************
 * CreateFindWindow - routine is invoked each time the find button in the
 *                    file menu is selected. It is called from DmFindCB.
 *
 * The \fICreateFindWindow\fP function is used to create 
 * the base find window which is drawn whenever the find
 * button is selected from the file menu in a folder window.
 * The window contains a "File Name(s):" field as well as
 * a " Word / Phrase" field.  The input for each of these fields
 * are passed to find and grep respectively. Below these fields are:
 *
 * "Where To Look:" list of relevant folders to search
 * "Also Look In:" alternate folder(s) to search not currently in the list
 * "File Type:"  The List of File Types on which to search
 * "Days Since File(s) Last Updated:"
 * 
 * Action Menu: "Find" "Cancel" "Help"
 * 
 */

static Widget 
CreateFindWindow(folder)
DmFolderWinPtr  folder;
{
#define FIRST 1

   	Arg		arg[20];
   	char		*alias;
	Cardinal 	total_items;
	ListItem	*finditems;
	LabelGizmo 	* clistarray;
	ListGizmo 	* fnlist;
	CListGizmo 	* fn_clist;
	PopupGizmo 	* findWindow;
	Widget		uca;

	struct passwd 	*user_entry;
	entryptr        list, loclist, headlist;

	unsigned short 	user_id;
	int 		cnt, i;
	Boolean AddBoth = True;


	/*
	 * If folder already has a Find Prompt Box associated with it,
	 * map the popup....
	 */
	if (folder->finderWindow) {

		((MenuGizmo *)folder->finderWindow->menu)->
				items[0].client_data = (char *) folder;
		((MenuGizmo *)folder->finderWindow->menu)->
				items[1].client_data = (char *) folder;

		MapGizmo(PopupGizmoClass, folder->finderWindow);
		return;
	}
	/*
	 * ...otherwise, copy the FindWindow (remember that this
	 * involves allocating space for the flatlist) and tie that
	 * popup to the folder.
 	 */

   	findWindow = CopyGizmo(PopupGizmoClass, &FindWindow);
	folder->finderWindow = findWindow;

	/*
	 * Get the Gizmo for the flat list that has been associated
	 * with this folder window (as mentioned this was MALLOC'ed
 	 * in the copy gizmo above.
	 */
	fnlist = (ListGizmo *)
	    QueryGizmo(PopupGizmoClass,
		       folder->finderWindow, GetGizmoGizmo, "fnlist");

	/* Check if the path for this folder is the same as the user's
	 * home path in the /etc/passwd file.  This determines if both
	 * the literal "My Folders" and the folder's path are added to
	 * the "Where To Look:" list.
	 */
	user_id = getuid();
	user_entry = getpwuid(user_id);
	if (strlen(folder->cp->path) == strlen(user_entry->pw_dir))
		if (!strcmp(folder->cp->path, user_entry->pw_dir))
			AddBoth = False;

	cnt = 0;
	headlist = loclist = list = (entry *) MALLOC (sizeof(entry));
	if (AddBoth) {
	    loclist->name	= strdup(folder->cp->path);
	    loclist->dir	= strdup(folder->cp->path);
	    loclist->removable	= FN_PATH;
	    cnt++;

	    loclist->next	= (entry *) MALLOC (sizeof(entry));
	    loclist		= loclist->next;
	}

	/* Add an entry for "My Folders" to the "Where To Look:" list */
	loclist->name		= strdup(Dm__gettxt(TXT_MY_FOLDERS));
	loclist->dir		= strdup(user_entry->pw_dir);
	loclist->removable	= FN_PATH;
	cnt++;

	/* Add an entry for "All Users" to the "Where To Look:" list */
	loclist->next		= (entry *) MALLOC (sizeof(entry));
	loclist			= loclist->next;
	loclist->name		= strdup(Dm__gettxt(TXT_USER_FOLDERS));
	loclist->dir		= strdup("/home");
	loclist->removable	= FN_ALLUSERS;
	cnt++;

	/* Add an entry for "diskette1/2" to the "Where To Look:" list */
  	alias = DtamGetAlias("diskette1", FIRST);
	if (alias) {
		loclist->next = (entry *) MALLOC (sizeof(entry));
		loclist = loclist->next;
		loclist->name = strdup(alias);
		loclist->dir = NULL;
		FREE(alias);
		loclist->removable  = FN_DISK1;
		cnt++;
	}

	alias = DtamGetAlias("diskette2", FIRST);
	if (alias) {
		loclist->next = (entry *) MALLOC (sizeof(entry));
		loclist = loclist->next;
		loclist->name = strdup(alias);
		loclist->dir = NULL;
		FREE(alias);
		loclist->removable  = FN_DISK2;
		cnt++;
	}

	/* Add an entry for "The Whole System" to the "Where To Look:" list */
	loclist->next		= (entry *) MALLOC (sizeof(entry));
	loclist			= loclist->next;
	loclist->name		= strdup(Dm__gettxt(TXT_WHOLE_SYSTEM));
	loclist->dir		= strdup("/");
	loclist->removable	= FN_PATH;
	cnt++;

	loclist->next =NULL;
   	setpwent();

	/*
	 * Allocate as many ListHead structures as there are members
	 * of the list (cnt), attach that allocated space to the flat
	 * list gizmo's current value. 
	 */
   	((ListHead *)(fnlist->settings->current_value))->list = 
			(ListItem *)MALLOC(sizeof(ListItem) * (cnt+1));
	loclist = headlist;
	finditems = 	(ListItem *)((ListHead *)
			(fnlist->settings->current_value))->list;
	/*
	 * Loop through each of the allocated ListItems and set the
	 * fields in the ListItems (set, fields, clientData) to the 
	 * values that was obtained in the linked list of loclists.
	 */
    	for(i=0; i< cnt; i++)
    	{
           finditems->set = False;
	   finditems->fields = (XtArgVal)loclist;
	   finditems++;
	   loclist = loclist->next;
     	}
	/*
	 * By default set the first entry in the list to True. The
	 * 1st entry being either the path or the literal "My Folders"
	 * Set the size in the ListHead Structure.
	 */
        ((ListHead *)(fnlist->settings->current_value))->list[0].set = True;

   	total_items = i;
	((ListHead *)(fnlist->settings->current_value))->size = i;

	/*
	 * Create the Popup Gizmo for the Find Prompt Box now that every
	 * thing is setup.  Needed to get the entries for the ListItems
	 * in the ListHead before doing this, otherwise would have been 
	 * creating the FlatList would 0 entries.
	 */
	CreateGizmo((Widget)folder->shell, PopupGizmoClass,
                            findWindow, NULL, 0);

	/* Change XtNcenter to True for upper control area */
	XtSetArg(arg[0], XtNupperControlArea, &uca);
	XtGetValues(findWindow->shell, arg, 1);
	XtSetArg(arg[0], XtNcenter, True);

	clistarray = (LabelGizmo *) findWindow->gizmos[5].gizmo;
	fn_clist = (CListGizmo *) clistarray->gizmos[1].gizmo;
	
	for (i = 0; i < fn_clist->cp->num_objs; i++) {
		XtSetArg(arg[0], XtNset, True);
		OlFlatSetValues(fn_clist->boxWidget, i, arg, 1);
	}

	((MenuGizmo *)findWindow->menu)->items[0].client_data = (char *) folder;
	((MenuGizmo *)findWindow->menu)->items[1].client_data = (char *) folder;

	MapGizmo(PopupGizmoClass, findWindow);

} /* end of CreateFindWindow */

/* ARGSUSED */
/****************************procedure*header*****************************
  fn_FindCancelCB - routine is invoked each time the cancel button in the
                    Find Prompt Box is selected. This routine is a callback
                    registered with the Cancel Button.
*/
static void
fn_FindCancelCB(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
	DmFolderWinPtr folder = (DmFolderWinPtr)client_data;

   	XtPopdown((Widget)folder->finderWindow->shell);
}


/* ARGSUSED */
/****************************procedure*header*****************************
  fn_FoundHelpCB -  routine is invoked each time the Found Window button in the
                    Help menu of the Found Window is selected. This routine is 
		    a callback registered with the Found Window Button.
*/
static void
fn_FoundHelpCB(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
    DmHelpAppPtr help_app = DmGetHelpApp(FOLDER_HELP_ID(Desktop));

	DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id,
		NULL, "DesktopMgr/folder.hlp", "795", UNSPECIFIED_POS,
		UNSPECIFIED_POS);
}


/* ARGSUSED */
/****************************procedure*header*****************************
  fn_FoundHelpCB -  routine is invoked each time the Help button at the
                    bottom of the Find Prompt Box is selected. This routine 
		    is a callback registered with the Help Button.
*/
static void
fn_HelpCB(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
    DmHelpAppPtr help_app = DmGetHelpApp(FOLDER_HELP_ID(Desktop));

	DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id,
		NULL, "DesktopMgr/folder.hlp", "700", UNSPECIFIED_POS,
		UNSPECIFIED_POS);
}

static void
fn_scrolldown(info)
FindInfoStruct * info;
{
	int	vsbproplen, vsbmaxlen;
	Widget  vsb;
                XtSetArg(Dm__arg[0], XtNvScrollbar, &vsb);
                XtGetValues(info->window->swin, Dm__arg, 1);

/*
                XtSetArg(Dm__arg[0], XtNproportionLength, &vsbproplen);
                XtSetArg(Dm__arg[1], XtNsliderMax, &vsbmaxlen);
                XtGetValues(vsb, Dm__arg, 2);

                XtSetArg(Dm__arg[0], XtNsliderValue, vsbmaxlen - vsbproplen);
                XtSetValues(vsb, Dm__arg, 1);
*/
		OlActivateWidget(vsb, OL_SCROLLBOTTOM, 0);
}

static void
fn_scrollup(info)
FindInfoStruct * info;
{
	Widget  vsb;
                XtSetArg(Dm__arg[0], XtNvScrollbar, &vsb);
                XtGetValues(info->window->swin, Dm__arg, 1);
		OlActivateWidget(vsb, OL_SCROLLTOP, 0);
}

/* ARGSUSED */
/****************************procedure*header*****************************
    fn_StopKeyCB-
*/
static void
fn_StopKeyCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
/*
    DmFolderWinPtr window = (DmFolderWinPtr)client_data;
*/
    FindInfoStruct * info = (FindInfoStruct *)client_data;
    OlVirtualEvent event = (OlVirtualEvent)call_data;
    DmTaskInfoListPtr tip;

    /* only interested in OL_STOP virtual event */
    if (event->virtual_name != OL_STOP)
    {
	event->consumed = False;
	return;
    }

    fn_StopCB(NULL,(XtPointer)info,NULL); 
    event->consumed = True;
}

/* ARGSUSED */
/****************************procedure*header*****************************
  fn_StopCB -  The StopCB CallBack is invoked as soon as the Stop Button
               in the Finding Window is selected.  The button can be selected
               at any point in the finding process; as such the Complete_Flag
               in the FindInfo Struct structure is essential in determining if
               find process (ie. popen) had completed (Complete_Flag = True)
               or is being interupted.
*/
static void
fn_StopCB(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
	FindInfoStruct * info = (FindInfoStruct *)client_data;

	/*
	 * Only Do the Following if the request to terminate the search is
	 * processed before the complete flag is set to True.
	 */
	if (info->Complete_Flag == False) {
		XtRemoveTimeOut(info->timeout_id);
                fn_pkill(info->fptr);
		info->Complete_Flag = True;
	}
        XtPopdown(info->stopnotice->shell);
	DmBusyWindow(info->window->shell, False,
                        fn_StopKeyCB, (XtPointer)info);
	fn_scrollup(info);
	DmDisplayStatus((DmWinPtr)(info->window));
	DmVaDisplayStatus((DmWinPtr)(info->window), True, TXT_STOP_SEARCH);
}

/* ARGSUSED */
/****************************procedure*header*****************************
  CreateFoundWindow -  The Create FW is invoked whenever the user selects the
		       find button in the Find Prompt Box.  The routine is 
		       called from the fn_FindCB after all the find selections
              	       are verified and a pipe is opened to the output of a
	      	       find request.  The routine creates a base window with
		       a flat icon box. The View of the entries in the box
		       is only given in long view.
*/
static void
CreateFoundWindow(info)
FindInfoStruct * info;
{
#define INIT_ICON_SLOTS        50
	char * 		name;
	char * 		pp;
	char * 		prefix;
        DmObjectPtr 	op;
	Widget  	swin;
	Arg 		arg[50];
	XFontStruct *	font;

	DtAttrs   	calc_geom = 0;
	Dimension	view_width;
	MenuGizmo * 	foundmenu;
	MenuGizmo * 	filemenu;
	BaseWindowGizmo *base;
	/*
	 * Allocate space for the FolderWindow Structure
	 */
	info->window = (DmFolderWinPtr)CALLOC(1, sizeof(DmFolderWinRec));
	/*
	 * Allocate space for the DmContainer Structure
	 */
        if ((info->window->cp = 
	(DmContainerPtr)CALLOC(1, sizeof(DmContainerRec))) == NULL) {
		Dm__VaPrintMsg(TXT_MEM_ERR);
                return;
        }
	/*
	 * Initialize the path to "", this is how a found window id id'ed
	 */
        info->window->cp->path 	  = strdup("");
        info->window->cp->count   = 1;
        info->window->cp->num_objs= 0;

	/* identify as a Folder window */
	info->window->attrs       = DM_B_FOLDER_WIN | DM_B_FOUND_WIN;
	info->window->view_type   = DM_LONG;
	info->window->sort_type   = DM_BY_TYPE;

	base 		= CopyGizmo(BaseWindowGizmoClass, &FoundWindow);
	prefix 		= Dm__gettxt(TXT_FOUND_TITLE);
	base->title 	= prefix;
	info->window->gizmo_shell = base;

	/* Set the client data for submenus */
   	foundmenu= base->menu;
	foundmenu->client_data = (XtPointer)(info->window);

	/* Get File Menu and set the client data of Exit Button to info */
	filemenu = GetSubMenuGizmo(foundmenu, 0);
	((MenuGizmo *) filemenu)->items[8].client_data = (char *) info;

	/*
	 * Give the Found Window Base Window a Pushpin to give it
	 * The Appearance of a Popup Shell.
	 */
	XtSetArg (arg[0], 	XtNwinType,	OL_WT_CMD);
	XtSetArg (arg[1], 	XtNpushpin,	OL_IN);
	XtSetArg (arg[2], 	XtNwindowHeader, True);
	XtSetArg (arg[3], 	XtNmenuButton,  False);
	XtSetArg (arg[4], 	XtNmenuType,	OL_MENU_LIMITED);
        XtSetArg (arg[5],	XtNwidth,
                 FOLDER_WINDOW_WIDTH(DESKTOP_SHELL(Desktop)));
        XtSetArg (arg[6],	XtNheight,
                 FOLDER_WINDOW_HEIGHT(DESKTOP_SHELL(Desktop)));

	info->window->shell = CreateGizmo(NULL, BaseWindowGizmoClass, base,
					  arg, 7);
	info->window->swin  = (Widget)QueryGizmo(BaseWindowGizmoClass, base,
                                 GetGizmoWidget, "swin");

	OlAddCallback(info->window->shell, XtNwmProtocol, FoundWinWMCB, (char *)info);

	XtVaGetValues(DESKTOP_SHELL(Desktop), XtNfont, &font, 0);

        XtSetArg(arg[0], XtNdrawProc, 		(XtArgVal)DmDrawLongIcon);
        XtSetArg(arg[1], XtNmovableIcons, 	(XtArgVal)False);
        XtSetArg(arg[2], XtNdropProc,  		Dm__FolderDropProc);
	XtSetArg(arg[3], XtNdblSelectProc,      Dm__FolderSelect2Proc);
	XtSetArg(arg[4], XtNclientData,         info->window);
	XtSetArg(arg[5], XtNmenuProc,           DmIconMenuProc);
	XtSetArg(arg[6], XtNpostSelectProc,	DmButtonSelectProc);
	XtSetArg(arg[7], XtNpostAdjustProc,	DmButtonSelectProc);

	/*
	 * Initialize the number of items to 50 to avoid MALLOCs of
	 * item structures.
	 */
	info->window->nitems = INIT_ICON_SLOTS;
	info->window->box = DmCreateIconContainer(info->window->swin,
					calc_geom,
                                        arg, 8,
                                        info->window->cp->op,
                                        info->window->cp->num_objs,
                                        &(info->window->itp),
                                        info->window->nitems,
                                        NULL,
                                        DESKTOP_FONTLIST(Desktop),
                                        DESKTOP_FONT(Desktop),
                                        DM_FontHeight(Desktop));

	/* make the iconbox the initial focus widget */
	XtSetArg(Dm__arg[0], XtNfocusWidget, info->window->box);
	XtSetValues(info->window->shell, Dm__arg, 1);

	/* Compute the view width and set the scrolled window to it */
	view_width = GRID_WIDTH(Desktop) * FOLDER_COLS(Desktop);

	XtVaSetValues(info->window->swin, XtNviewWidth, view_width,
                  XtNviewHeight, GRID_HEIGHT(Desktop) * FOLDER_ROWS(Desktop),
                  NULL);

        /* put something in the status area */
	DmDisplayStatus((DmWinPtr)(info->window));
 
	XtRealizeWidget(info->window->shell);
    	{
        	DmViewFormatType type = (info->window->cp->attrs & DM_B_NO_INFO) ?
            			(DmViewFormatType)-1 : info->window->view_type;
        	info->window->view_type = (DmViewFormatType)-1;
        	DmFormatView(info->window, type);
    	}


}



/* ARGSUSED */
static void
fn_getfind(XtPointer client_data, XtIntervalId * id)
{

	FindInfoStruct * info = (FindInfoStruct *)client_data;
	char * p;
        char buffer[5120];
	char * name;
	char * piecefile;
	int nbytes, i, newnbytes, namelen, startlen, vsbmaxlen, vsbproplen;
	Arg               arg[6];
        DmObjectPtr 	op;
	Boolean		AddToFound = True;
	Dimension	pane_width;
	Dimension	pane_height;
	Dimension	swin_height;
	Dimension	margin = Ol_PointToPixel(OL_HORIZONTAL, ICON_MARGIN);
	Widget  	vsb;
	DmItemPtr   	item;
	int             adjustbuffer = 80;
	Dimension   pad = Ol_PointToPixel(OL_HORIZONTAL, INTER_ICON_PAD);

	piecefile = NULL;

	/*
	 * Need to Fix:  For now leave space at from of buffer
         * 		 to account for \n as the first thing in buffer
         */
	buffer[5119] = '\0';
        if (!((nbytes=
	read(fileno(info->fptr), buffer+adjustbuffer, 5039))== NULL))
	{
	    /* Since the read is NONBLOCK, need to check if there is currently
	     * anything on the pipe. If there is no data available, and the
	     * pipe is still open - addtimeout ....
	     */
	    if (nbytes == -1) {
		info->timeout_id = XtAddTimeOut(500, fn_getfind, info);
		return;
	    }

	    /* else: process the data coming in. */
	    XtVaGetValues(info->window->swin, XtNviewHeight,
			  &swin_height, NULL);
	    if (info->yposition > swin_height)
		fn_scrolldown(info);

	    if (!(buffer[(nbytes - 1) + adjustbuffer] == '\n')) {
		/*
		 * Loop until we find the first newline
		 */
		for(newnbytes = nbytes - 1; 
		    buffer[newnbytes + adjustbuffer] != '\n'; 
		    newnbytes--);
		buffer[nbytes + adjustbuffer] = '\0';
		piecefile = strdup(buffer+adjustbuffer + newnbytes +1);
		buffer[newnbytes + adjustbuffer] = '\0';
	    }
	    else {
		newnbytes = nbytes;
		buffer[newnbytes + adjustbuffer] = '\0';
	    }
	    startlen = info->maxlen;
	    XtVaGetValues(info->window->box, XtNwidth, &pane_width, NULL);
	    if (buffer[adjustbuffer] == '\n'){
		adjustbuffer = adjustbuffer - strlen(info->lastfile);
		strncpy(buffer + adjustbuffer, 
			info->lastfile,
			strlen(info->lastfile));
		FREE(info->lastfile);
		info->lastfile = NULL;
	    }
	    for(i=0, name = strtok(buffer + adjustbuffer, "\n");
		name != NULL; name = strtok(NULL, " \t\n"))
	    {
		namelen = strlen(name);
		if (info->lastfile)
		{
		    char * newname = (char *)
			MALLOC(strlen(info->lastfile) + namelen + 1);
		    strcpy(newname, info->lastfile);
		    strcat(newname, name);
		    FREE(info->lastfile);
		    info->lastfile = NULL;
		    name = newname;
		    namelen = strlen(name);
		}

		op = Dm__CreateObj(info->window->cp, name, 
				   DM_B_SET_TYPE | DM_B_INIT_FILEINFO);

		/* Can't continue if failed to create obj */
		if (op == NULL)
		{
		    DmVaDisplayStatus((DmWinPtr)info->window, True,
				      StrError(ERR_CreateEntry));
		    break;
		}

		/* Do not want to pass hidden files to DmInitObjType */
		if (op->attrs & DM_B_HIDDEN)
		    continue;

		/* TAKEN FROM DMAddObjToFolder */

		DmInitObjType(info->window->box, op);

		/* Get "FREE" item from FIconBox */
		(void)Dm__GetFreeItems(&info->window->itp, 
				       &info->window->nitems, 1, &item);

		/* Generate the item's label (based on view-type) 
		   and size */
		item->object_ptr = (XtArgVal)op;
		if (info->maxlen < namelen)
		    info->maxlen = namelen;
		item->label = (XtArgVal)
		    strdup(Dm__MakeItemLabel(item, info->window->view_type,
					     info->maxlen));

		DmComputeItemSize(item, info->window->view_type, 
				  (Dimension *)&item->icon_width,
				  (Dimension *)&item->icon_height);

		item->x			= margin;	
		item->y			= info->yposition;	
		item->managed		= True;
		item->select		= False;
		item->busy		= False;
		item->client_data	= NULL;
		item->object_ptr	= (XtArgVal)op;

		info->yposition += DM_LongRowHeight(Desktop) + pad;
		
		if (info->yposition > 32500) {
			AddToFound = False;
			break;
		}

		/* DO EXACTLY AS SORTITEMS BUT TAKE SORT ITEMS OUT OF LOOP */
	    }


	    if (info->maxlen  > startlen)
		DmComputeLayout(info->window->box, info->window->itp, 
				info->window->nitems,
				info->window->view_type, pane_width, 0, 0);

	    DmTouchIconBox((DmWinPtr)info->window, NULL, 0);

		/* Put in Check to see if y dimension has maxed out */
		/* The y coordinate is limited to a signed short 2**15 */
	    if (!AddToFound) {
		fn_pkill(info->fptr);
		info->Complete_Flag = True;
        	XtPopdown(info->stopnotice->shell);
		DmBusyWindow(info->window->shell, False,
				fn_StopKeyCB, (XtPointer)info);
		fn_scrollup(info);
		DmDisplayStatus((DmWinPtr)(info->window));
		DmVaDisplayStatus((DmWinPtr)(info->window), True, TXT_SEARCH_TERMINATED);
        	return;
	    }

	    if (piecefile)
		info->lastfile = piecefile;

	    /* put something in the status area */
	    DmDisplayStatus((DmWinPtr)(info->window));

	    info->timeout_id = XtAddTimeOut(500, fn_getfind, info);
	}
	else
	{
		fn_scrollup(info);
		/*
		 * If got here there is no more to read and the timer has
		 * not been enabled, so, close the pipe and there is no need
		 * to XtRemoveTimeout.
		 */
		fclose(info->fptr);
		info->Complete_Flag = True;
		XtPopdown(info->stopnotice->shell);
		DmBusyWindow(info->window->shell, False,
                     	fn_StopKeyCB, (XtPointer)info);
		DmDisplayStatus((DmWinPtr)(info->window));
		DmVaDisplayStatus((DmWinPtr)(info->window), True, TXT_COMPLETE_SEARCH);
		return;
	}
}

/*
 * The FindingCB routine is invoked after an event is received
 * indicating that a mapped event of the notice find in progress popup 
 * has occurred. XtAddEventHandler is the means through which MoreFindingCB 
 * is invoked.
 * MoreFindingCB basically registers the get_find routine with 
 * XtAddTimeOut.  Given that a find can be extremely time consuming,
 * This allows for other events to be processed if no information is
 * available from the fd.
 */ 
static void
fn_MoreFindingCB(Widget popup, XtPointer client_data,
		 XEvent * xevent, Boolean * continue_to_dispatch)
{
	register XMapEvent *event = (XMapEvent *)xevent;
	Arg  args[6];
	FindInfoStruct * info = (FindInfoStruct *)client_data;
	Widget	stopbutton;

        if(event->type == MapNotify) {
		DmVaDisplayStatus((DmWinPtr)(info->window), True, TXT_START_SEARCH);
		info->timeout_id = XtAddTimeOut(500, fn_getfind, info);
		DmBusyWindow(info->window->shell, True,
                     	fn_StopKeyCB, (XtPointer)info);
		stopbutton = (Widget) (((MenuGizmo *)(info->stopnotice->menu))->child);
                OlAddCallback(stopbutton, XtNconsumeEvent, fn_StopKeyCB, info);

	}

}

/*
 * The FindingCB routine is invoked after an event is received
 * indicating that a mapped event of the found window has occurred.
 * XtAddEventHandler is the means through which FindingCB is invoked.
 * An Event Handler is added here to account for the Notice Window 
 * indicating that the find is in progress.
 */ 
static void
fn_FindingCB(Widget popup, XtPointer client_data,
	     XEvent * xevent, Boolean * continue_to_dispatch)
{
	register XMapEvent *event = (XMapEvent *)xevent;
	Arg  args[6];
	FindInfoStruct * info = (FindInfoStruct *)client_data;

        if(event->type == MapNotify) {
		info->stopnotice = CopyGizmo(PopupGizmoClass, &StopNotice);
		((MenuGizmo *)info->stopnotice->menu)-> items[0].client_data
                                                        = (char *) info;
		XtSetArg (args[0], XtNwinType, OL_WT_NOTICE);
		XtSetArg (args[1], XtNx, 500);
		CreateGizmo((Widget)info->window->box, PopupGizmoClass,
                            info->stopnotice, args, 2);
		MapGizmo(PopupGizmoClass, info->stopnotice);
   		XtAddEventHandler(info->stopnotice->shell, StructureNotifyMask, 
					FALSE, fn_MoreFindingCB, info);

	}

}


/* The FindCB is invoked whenever the "Find" button in the find
 * Base Window is selected.  The routines calls CreateFindingWindow;
 * and also constructs a command string eg. "find /home -name "p" -print
 * which is used as an argument to the popen.
 */

/* ARGSUSED */
static void
fn_FindCB(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
	extern FILE *fn_popen();
	FindInfoPtr info;
	DmFolderWinPtr folder = (DmFolderWinPtr)client_data;
	char findcmd[4096];
	char * tmp_string;
	char * trns_string;
	char * file_string;
	char * other_string;
	char * mntpnt;
	Arg  args[6];
	Boolean typeset[DM_FTYPE_SEM -1];
	Boolean set, allset, alreadyset, special_type;
	struct  stat    st_buf;
	entryptr loclist;
	int i,j, flags, num_path, num_files, num_types;

	ListGizmo  * fnlist;
	LabelGizmo * clistarray;
	CListGizmo * fn_clist;
	pathrecptr local;
	pathrecptr scratchlocal;
	pathrecptr tmplocal;
	struct passwd *user_entry;

#define FN_STRCAT(A,B)	{						\
			if ((int)(strlen(A) + strlen(B)) > 4096) {     	\
			 	SetPopupMessage(info->window->finderWindow, GetGizmoText(TXT_FNEXCEEDED_ERR)); \
				return;					\
			}						\
			else {						\
				strcat(A,B);				\
			}						\
			}

	fn_InitInfo(&info);
        info->FolderWin = (DmFolderWinPtr)folder;
	SetPopupMessage(info->FolderWin->finderWindow, " ");
	fnlist = (ListGizmo *)QueryGizmo(PopupGizmoClass, 
				(PopupGizmo *)info->FolderWin->finderWindow,
                                 GetGizmoGizmo, "fnlist");

	loclist = (entryptr)((ListHead *)
			(fnlist->settings->current_value))->list[0].fields;

   	XtSetArg(args[0], XtNstring, &tmp_string);
   	XtGetValues((Widget)QueryGizmo(PopupGizmoClass, 
				(PopupGizmo *)info->FolderWin->finderWindow, 
				GetGizmoWidget, "otherpath"), args, 1);
	num_path = 0;

	strcpy(findcmd, "find   ");
	local = NULL;

	for(i = 0; i < ((ListHead *)(fnlist->settings->current_value))->size; 
									i++) {
		if(((ListHead *)(fnlist->settings->current_value))->list[i].set
								 == True) {
			if ((loclist->removable == FN_DISK1) || (loclist->removable == FN_DISK2)) {
                                flags = DtamMountDev(loclist->name, &mntpnt);

                                if(flags == DTAM_NO_DISK) {
					SetPopupMessage(
						info->FolderWin->finderWindow,
						GetGizmoText(TXT_NODISK_ERR));
						return;
				}
				else
                                if (flags == DTAM_CANT_MOUNT) {
					SetPopupMessage(
						info->FolderWin->finderWindow,
						GetGizmoText(TXT_CANTMOUNT_ERR));
						return;
				}
				else
                                if (flags == DTAM_NOT_OWNER) {
					SetPopupMessage(
						info->FolderWin->finderWindow,
						GetGizmoText(TXT_NOTOWNER_ERR));
                                        return;
                                }
				else{
                                	if (!(flags == DTAM_MOUNTED)) {
					  if (loclist->removable == FN_DISK1) {
						if (info->disk1)
							FREE(info->disk1);
						info->disk1 = strdup(mntpnt);
					  }
					  if (loclist->removable == FN_DISK2) {
						if (info->disk2)
							FREE(info->disk2);
						info->disk2 = strdup(mntpnt);
					  }
					}
					loclist->dir = strdup(mntpnt);
					FREE(mntpnt);
				}
			fn_AddPathToList(&local, loclist->dir);
			num_path++;
			}
			else
			if (loclist->removable == FN_ALLUSERS) {
        			while(!((user_entry= getpwent()) == NULL)) {
                                        if ((user_entry->pw_uid < 60000) &&
                                            (user_entry->pw_uid > 100)) {
					 fn_AddPathToList(&local, user_entry->pw_dir );
					 num_path++;
					}
				}
				setpwent();
			}
			else {
			fn_AddPathToList(&local, loclist->dir);
			num_path++;
			}
		}
		loclist = loclist->next;
	}
	for(other_string=strtok(tmp_string, " \t\n"); other_string; 
					other_string=strtok(NULL, " \t\n")) {
		
                (void)stat(other_string, &st_buf);
		if(!((st_buf.st_mode & S_IFMT) == S_IFDIR)){
			SetPopupMessage(info->FolderWin->finderWindow,
						GetGizmoText(TXT_INVOTHER_ERR));
			return;
		}
		fn_AddPathToList(&local, other_string);
		num_path++;
	}
	if(num_path == 0) {
		SetPopupMessage(info->FolderWin->finderWindow,
					GetGizmoText(TXT_NOPATH_ERR));
		return;
	}
	scratchlocal = local;
	while(scratchlocal != NULL) {
		FN_STRCAT(findcmd, scratchlocal->path);
		FN_STRCAT(findcmd, " ");
		scratchlocal = scratchlocal->next;
	}
	scratchlocal = local;
	while(scratchlocal != NULL) {
		tmplocal = scratchlocal->next;
		FREE(scratchlocal->path);
		FREE((void *)scratchlocal); 
		scratchlocal = tmplocal;
	}
	
   	XtSetArg(args[0], XtNstring, &tmp_string);
   	XtGetValues((Widget)QueryGizmo(PopupGizmoClass, 
				(PopupGizmo *)info->FolderWin->finderWindow,
				GetGizmoWidget, "filename"), args, 1);
	num_files = 0;
	if (!(tmp_string[0] == '\0')){
	 FN_STRCAT(findcmd, " \\( ");
	 for(file_string=strtok(tmp_string, " \t\n"); file_string; 
					file_string=strtok(NULL, " \t\n")) {
		
		num_files++;
		if (num_files > 1)
			FN_STRCAT(findcmd, " -o -name \"")
		else
			FN_STRCAT(findcmd, " -name \"")
		XtSetArg(args[0], XtNset, &set);
        	OlFlatGetValues(GetChoiceButtons((ChoiceGizmo *)
				QueryGizmo(PopupGizmoClass, 
				(PopupGizmo *)info->FolderWin->finderWindow,
				GetGizmoGizmo, "FCase")), 0, args, 1);

		if (set == True) {
			trns_string = fn_transform(file_string, strlen(file_string));
			FN_STRCAT(findcmd, trns_string)
			FN_STRCAT(findcmd, "\"")
			FREE(trns_string);
		}
		else{
			FN_STRCAT(findcmd, file_string)
			FN_STRCAT(findcmd, "\"")
		}
	}
	 FN_STRCAT(findcmd, " \\) ")
	FREE(tmp_string);
	}

   	XtSetArg(args[0], XtNstring, &tmp_string);
   	XtGetValues((Widget)QueryGizmo(PopupGizmoClass, 
				(PopupGizmo *)info->FolderWin->finderWindow,
				GetGizmoWidget, "daysupdated"), args, 1);

	if (!(tmp_string[0] == '0')) {
		FN_STRCAT(findcmd, " -mtime ");
		FN_STRCAT(findcmd, "-");
		FN_STRCAT(findcmd, tmp_string);
		FN_STRCAT(findcmd, " ");
	}

	/*
	 * Go through list looking for Set Icon, to populate typeset array
	 */
	allset = True;
	alreadyset = False;
	special_type = False;
	set = False;
	num_types = 0;

	clistarray = (LabelGizmo *) (PopupGizmo *)info->FolderWin->
						finderWindow->gizmos[5].gizmo;
	fn_clist = (CListGizmo *) clistarray->gizmos[1].gizmo;

	for (i = 0; i < fn_clist->cp->num_objs; i++) {
		XtSetArg(args[0], XtNset, &set);
        	OlFlatGetValues(fn_clist->boxWidget, i, args, 1);
		typeset[i] = set;
		
		if (!set) 
			allset = False;
		else
			num_types++;
	}
	
   	XtSetArg(args[0], XtNstring, &tmp_string);
   	XtGetValues((Widget)QueryGizmo(PopupGizmoClass, 
				(PopupGizmo *)info->FolderWin->finderWindow,
				GetGizmoWidget, "filecontent"), args, 1);

	if (!allset) {

	   if (num_types > 1)
			FN_STRCAT(findcmd, " \\( ");

	
           /*
	    * Check for Directories
            */
	   if (typeset[DM_FTYPE_DIR -1]) {
			FN_STRCAT(findcmd, " \\( -type d \\) ");
			alreadyset = True;
	   }

           /*
	    * Check for regular files (executable or data files)
            */
	   if (typeset[DM_FTYPE_EXEC -1] && typeset[DM_FTYPE_DATA -1]) {
		if (alreadyset)
			FN_STRCAT(findcmd, " -o ");
		FN_STRCAT(findcmd, " \\( -type f \\)");
		if (!alreadyset)
			alreadyset = True;
	   }
	   else
		if (typeset[DM_FTYPE_EXEC -1]){
			if (alreadyset)
				FN_STRCAT(findcmd, " -o ");
			FN_STRCAT(findcmd, 
			" \\( \\( -perm -010 -o -perm -100 -o -perm 001 \\)  -type f \\) ");
			if (!alreadyset)
				alreadyset = True;
	   	}
		else
	   	if (typeset[DM_FTYPE_DATA -1]) {
			if (alreadyset)
				FN_STRCAT(findcmd, " -o ")
			FN_STRCAT(findcmd, 
			" \\( \\( ! \\( -perm -010 -o -perm -100 -o -perm 001 \\) \\)  -type f \\) ")
			if (!alreadyset)
				alreadyset = True;
		}

           /*
	    * Check for FIFO (named pipe) files
            */
	   if (typeset[DM_FTYPE_FIFO -1]) {
		if (!(tmp_string[0] == '\0')){
			special_type = True;
		}
		else {
			if (alreadyset)
				FN_STRCAT(findcmd, " -o ");
			FN_STRCAT(findcmd, " \\( -type p \\)");
			if (!alreadyset)
				alreadyset = True;
		}
	   }

           /*
	    * Check for Character Special files
            */
	   if (typeset[DM_FTYPE_CHR -1]) {
		if (!(tmp_string[0] == '\0')){
			special_type = True;
		}
		else {
			if (alreadyset)
				FN_STRCAT(findcmd, " -o ");
			FN_STRCAT(findcmd, " \\( -type c \\) ");
			if (!alreadyset)
				alreadyset = True;
		}
	   }

           /*
	    * Check for Block Special files
            */
	   if (typeset[DM_FTYPE_BLK -1]) {
		if (!(tmp_string[0] == '\0')){
			special_type = True;
		}
		else {
			if (alreadyset)
				FN_STRCAT(findcmd, " -o ");
			FN_STRCAT(findcmd, " \\( -type b \\) ");
		}
	   }

	   if (num_types > 1)
			FN_STRCAT(findcmd, " \\) ");

	   if (!(tmp_string[0] == '\0')){
		if (special_type) {
        		SetPopupMessage(info->FolderWin->finderWindow,
        				GetGizmoText(TXT_SPECIALTYPE_ERR));
			FREE(tmp_string);
			
        		return;
		}
           }
	}
	else {
	   if (!(tmp_string[0] == '\0'))
			FN_STRCAT(findcmd, " \\( -type f \\) ");
	}



	FN_STRCAT(findcmd, " -follow ");

	if (!(tmp_string[0] == '\0')) {
		FN_STRCAT(findcmd, " -print 2> /dev/null |xargs grep -l ");
		XtSetArg(args[0], XtNset, &set);
        	OlFlatGetValues(GetChoiceButtons((ChoiceGizmo *)
				QueryGizmo(PopupGizmoClass, 
				(PopupGizmo *)info->FolderWin->finderWindow,
				GetGizmoGizmo, "CCase")), 0, args, 1);

		if (set == True) {
			FN_STRCAT(findcmd, "-i ");
		}
		FN_STRCAT(findcmd, "\""); 
		FN_STRCAT(findcmd, tmp_string); 
		FN_STRCAT(findcmd, "\" ");
	}
	else {
		FN_STRCAT(findcmd, " -print");
	}
	FN_STRCAT(findcmd, " 2> /dev/null");


   	BringDownPopup((Widget)info->FolderWin->finderWindow->shell);

	CreateFoundWindow(info);

	info->fptr = NULL;
	info->fptr = fn_popen(findcmd, "r");
	if (info->fptr == NULL) {
		SetPopupMessage(info->FolderWin->finderWindow,
                                        GetGizmoText(TXT_FIND_FAILED));
		perror("dtmgr find");
		return;
	}
	fcntl(fileno(info->fptr), F_SETFL, O_NONBLOCK);

   	XtAddEventHandler(info->window->shell, StructureNotifyMask, FALSE, fn_FindingCB, info);

}

static void
fn_AddPathToList(pathptr, dir)
pathrecptr * pathptr;
char * dir;
{
	pathrecptr head;
	pathrecptr current;
	pathrecptr local;
	pathrecptr tail;
	pathrecptr previous;
	int dirlen, pathlen;
	Boolean AddToList = True;
	Boolean AlreadyAdded = False;

	if (!(*pathptr)) {
		*pathptr = (pathrecptr) MALLOC (sizeof(PathRec));
		((pathrecptr)(*pathptr))->next= NULL;
		((pathrecptr)(*pathptr))->path= strdup(dir);
		return;
	}

	head = current = previous = local = *pathptr;

	while (!(local == NULL)) {
		tail = local;
		local = local->next;
	}
	
	dirlen = strlen(dir);

	while (!(current == NULL)) {
		pathlen = strlen(current->path);

		if (pathlen > dirlen) {
			/*
			 * BELOW: If the path up to dirlen is equal
			 * then remove current path since it is taken care 
			 * of by dir.
			 */
			if (!strncmp(current->path, dir, dirlen)) {

				/*
				 * remove from list
 				 */
				if (!(previous == current)) {
			    		previous->next = current->next;
					FREE(current->path);
					FREE((void *)current);
					current = previous->next;
				}
				else {
				    	if (current == *pathptr) {
						*pathptr = current->next;
					}
					
					previous = current->next;
					FREE(current->path);
					FREE((void *)current);
					current = previous;
				}

			}
			else {
				/* Add to end of list */
				previous = current;
				current = current->next;
			}
		}
		else
		if (dirlen > pathlen) {
			/*
			 * BELOW: If the dir up to pathlen is equal
			 * then do nothing since it is taken care 
			 * of by path.
			 */
			if (!strncmp(current->path, dir, pathlen)) {
				/* do not add to list becase already taken 
				 * care of
				 */
				AddToList = False;
				previous = current;
				current = current->next;
			}
			else {
				previous = current;
				current = current->next;
			}
		}
		else
		if (dirlen == pathlen) {
			if (!strncmp(current->path, dir, pathlen)) {
				AddToList = False;
				previous = current;
				current = current->next;
			}
			else {
				previous = current;
				current = current->next;
			}
		}
			
	}
	if (AddToList) {
		if (!(previous == NULL)) {
			local = (pathrecptr) MALLOC (sizeof(PathRec));
			local->next= NULL;
			local->path= strdup(dir);
			previous->next = local;
		}
		else {
			*pathptr = (pathrecptr) MALLOC (sizeof(PathRec));
			((pathrecptr)(*pathptr))->next= NULL;
			((pathrecptr)(*pathptr))->path= strdup(dir);
			return;
		}
	}
}

static char *
fn_transform(file_string, len)
char * file_string;
int len;
{
	char * tmp_string;
	char * new_string;
	int incnt, outcnt, tmp_char;

	/*
	 * newstring represents the max size eg. i -> [iI] 
         */
	new_string = (char *) MALLOC (((sizeof(char))*len *4) + (sizeof(char)));

	outcnt = 0;

	for (incnt = 0; incnt < len; incnt++)
	{
		if(islower(file_string[incnt])) {
			new_string[outcnt] = '['; outcnt++;
			new_string[outcnt] = file_string[incnt]; outcnt++;
			new_string[outcnt] = toupper(file_string[incnt]); outcnt++;
			new_string[outcnt] = ']'; outcnt++;
		}
		else 
			if(isupper(file_string[incnt])) {
				new_string[outcnt] = '['; outcnt++;
				new_string[outcnt] = file_string[incnt]; outcnt++;
				new_string[outcnt] = tolower(file_string[incnt]); outcnt++;
				new_string[outcnt] = ']'; outcnt++;
			}
			else {

				new_string[outcnt] = file_string[incnt]; outcnt++;
			}
	}
	incnt++;
	new_string[outcnt]='\0';
	return new_string;
}


static void
FoundWinWMCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	OlWMProtocolVerify *wm_data = (OlWMProtocolVerify *)call_data;
        FindInfoStruct * info = (FindInfoStruct *)client_data;

    if (wm_data->msgtype == OL_WM_DELETE_WINDOW)
    {

	fn_CloseFoundCB(NULL, info, NULL);

    } else
    {
	/* do the default */
	OlWMProtocolAction(w, wm_data, OL_DEFAULTACTION);
    }
}

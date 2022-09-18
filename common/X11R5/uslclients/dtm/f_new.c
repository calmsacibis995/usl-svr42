/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:f_new.c	1.44"


/******************************file*header********************************

    Description:
	This file contains the source code for the callback for the New
	button in a folder's File menu used to create new files.
*/
						/* #includes go here	*/

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Error.h>
#include <Xol/FButtons.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/ChoiceGizm.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/ListGizmo.h>
#include <Gizmo/LabelGizmo.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/InputGizmo.h>

#include "Dtm.h"
#include "CListGizmo.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/
static void	TmplCB(Widget, XtPointer, XtPointer);
static void	CListCB(Widget, XtPointer, XtPointer);
static void	CreateCB(Widget, XtPointer, XtPointer);
static int	CreateFile(DmFolderWinPtr fwp, Boolean open);
static void	NewFileFMProc(DmProcReason reason, XtPointer client_data,
			      XtPointer call_data, char * src, char * dst);
static int	VerifyTmpl(DmFolderWinPtr fwp);
static void	UpdateTemplates(DmFolderWinPtr, char *, Widget flist);
static int	VerifyInputs(DmFolderWinPtr fwp);

					/* public procedures		*/
void		DmPromptCreateFile(DmFolderWindow folder);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/


/* defines for New popup */
#define DM_DONT_USE_TEMPLATE 1 /* either no template or none selected */
#define DM_USE_TEMPLATE      2 /* valid template selected             */
#define DM_CREATE_N_OPEN     3 /* open file after creating it         */

/* Gizmo declarations for New popup window */

static MenuItems createMenuItems[] = {
  { True, TXT_NEW_CREATE_AND_OPEN, TXT_M_FILE_OPEN,  NULL, NULL, NULL },
  { True, TXT_NEW_CREATE,          TXT_M_NEW_CREATE, NULL, NULL, NULL },
  { True, TXT_CANCEL,              TXT_M_CANCEL,     NULL, NULL, NULL },
  { True, TXT_P_HELP,              TXT_M_HELP,       NULL, NULL, NULL },
  { NULL }
};

static MenuGizmo createMenu = {
	NULL, "create_menu", "_X_", createMenuItems, CreateCB, NULL
};

static CListGizmo clist_gizmo = {
	"clist",            /* widget name */
	2,                  /* view width */
	"_TEMPLATE",        /* required property */
	False,		     /* file */
	True,               /* sys class */
	False,		     /* xenix class */
	True,               /* usr class */
	False,              /* overridden class */
	True,               /* exclusive behavior */
	False,              /* noneset behavior */
	CListCB,            /* select proc */
};

static GizmoRec label0_rec[] = {
	{ CListGizmoClass,	&clist_gizmo },
};

static LabelGizmo label0_gizmo = {
    NULL,                 /* help */
    "class",              /* widget name */
    TXT_NEW_FILE_TYPE,    /* caption label */
    label0_rec,           /* gizmo array */
    XtNumber(label0_rec), /* number of gizmos */
    OL_FIXEDCOLS,         /* layout type */
    1,                    /* measure */
    NULL,                 /* arglist */
    0,                    /* num_args */
    False,                /* align caption */
};

static ListHead	dummyListHead = { NULL };
static Setting	dummySettings = {
    NULL, &dummyListHead, &dummyListHead, &dummyListHead, 0,
};

/* Caption label doesn't show up if LabelGizmo is not used */ 
static ListGizmo template_gizmo = {
	NULL,         /* help info */
	"template",   /* name of widget */
	NULL,         /* caption label */
	&dummySettings,/* settings */
	"%s",         /* format */
	True,         /* exclusive */
	3,            /* height */
	NULL,         /* font */
	False,        /* copy settings */
	NULL,         /* execute callback */
	TmplCB,       /* select callback */
	NULL,         /* unselect callback */
	NULL,         /* arglist */
	0,            /* num_args */
};

static GizmoRec label1_rec[] = {
	{ ListGizmoClass,	&template_gizmo },
};

static LabelGizmo label1_gizmo = {
	NULL,                 /* help */
	"caption",            /* widget name */
	TXT_NEW_TEMPLATE,     /* caption label */
	label1_rec,           /* gizmo array */
	XtNumber(label1_rec), /* number of gizmos */
	OL_FIXEDCOLS,         /* layout type */
	1,                    /* measure */
	NULL,                 /* arglist */
	0,                    /* num_args */
	False,                /* align caption */
};

static InputGizmo filename_gizmo = {
	NULL, "name", TXT_NEW_FILE_NAME, NULL, NULL, 20, NULL 
};

static GizmoRec create_gizmo[] = {
	{ LabelGizmoClass,	&label0_gizmo },
	{ LabelGizmoClass,	&label1_gizmo },
	{ InputGizmoClass,	&filename_gizmo },
};

static HelpInfo NewWinHelp =
{ TXT_G_FOLDER_TITLE, NULL, "DesktopMgr/folder.hlp", "420", NULL };

static PopupGizmo create_popup = {
	&NewWinHelp,            /* help */
	"create",               /* name of shell */
	TXT_NEW_TITLE,          /* window title */
	&createMenu,            /* pointer to menu info */
	create_gizmo,           /* gizmo array */
	XtNumber(create_gizmo), /* number of gizmos */
	NULL,                   /* args applied to shell */
	0,                      /* num_args */
};

typedef enum {
	CreateAndOpen,
	CreateOnly,
	CreateCancel,
	CreateHelp
} CreateFileMenuItemIndex;
	
static char	*list_fields[] = { XtNset, XtNformatData };

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
	Sets default file name to name of selected template.
*/
static void
TmplCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget		flist;
	ListNodePtr	tnodep = NULL;
	int			nitems = 0;
	DmFolderWindow	fwp    = (DmFolderWinPtr)client_data;
	OlFlatCallData	*fcd   = (OlFlatCallData *)call_data;

	flist = (Widget)QueryGizmo(PopupGizmoClass, fwp->createWindow,
				GetGizmoWidget, "template");

	XtSetArg(Dm__arg[0], XtNformatData, &tnodep);
	OlFlatGetValues(flist, fcd->item_index, Dm__arg, 1);

	if (tnodep == NULL) {
		SetInputText(fwp->createWindow, 2, "", False);
	} else {
		SetInputText(fwp->createWindow, 2, basename(tnodep->name), True);
	}
}				/* End of TmplCB */

/****************************procedure*header*****************************
	Updates the list of templates when a file type is selected.
*/
static void
CListCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget flist;
	Widget caption;
	LabelGizmo *labelGiz;
	ListNodePtr  tnodep;
	ListTokenPtr lip = NULL;
	int  nitems    = 0;
	char *template = NULL;
	DmFolderWindow      fwp  = (DmFolderWinPtr)client_data;
	OlFIconBoxButtonCD  *fcd = (OlFIconBoxButtonCD *)call_data;
	DmItemPtr           itp  = ITEM_CD(fcd->item_data);
	DmFclassPtr         fcp  = FCLASS_PTR(itp);

	labelGiz = (LabelGizmo *)(fwp->createWindow->gizmos[1].gizmo);
	caption = (Widget)QueryGizmo(LabelGizmoClass, labelGiz,
				GetGizmoWidget, "caption");

	flist = (Widget)QueryGizmo(PopupGizmoClass, fwp->createWindow,
			GetGizmoWidget, "template");

	/* free previous list items */
	XtVaGetValues(flist, XtNuserData, &lip, NULL);
	if (lip) {
		ListNodePtr save;
		ListNodePtr tp = (ListNodePtr)(lip->formatData);

		while (tp) {
			save = tp->next;
			FREE((void *)(tp->name));
			FREE((void *)tp);
			tp = save;
		}
		FREE((void *)lip);
	}

	/* get value of TEMPLATE property of item */
	if (template = DtGetProperty(&(fcp->plist), TEMPLATE, NULL)) {
		XtSetSensitive(caption, True);
		UpdateTemplates(fwp, template, flist);

	} else { /* no template */
		/* Make flist widget empty and insensitive */
		XtSetArg(Dm__arg[0], XtNitems,     NULL);
		XtSetArg(Dm__arg[1], XtNnumItems,  0);
		XtSetArg(Dm__arg[2], XtNuserData,  NULL);
		XtSetValues(flist, Dm__arg, 3);

		XtSetSensitive(caption, False);
		SetInputText(fwp->createWindow, 2, "", True);
	}
}				/* End of CListCB */

/****************************procedure*header*****************************
    CreateCB - callback for buttons in New popup window.
*/
static void
CreateCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    Widget		tf;
    DmFolderWindow	fwp = (DmFolderWinPtr)client_data;
    OlFlatCallData	*fcd = (OlFlatCallData *)call_data;

	tf = (Widget)QueryGizmo(PopupGizmoClass, fwp->createWindow,
			GetGizmoWidget, "name");

    switch(fcd->item_index)
    {
    case CreateAndOpen:
    case CreateOnly:

		/* False for not to open file after creation */
		if (CreateFile(fwp, (fcd->item_index == CreateAndOpen))) {
			OlSetInputFocus(tf, RevertToNone, CurrentTime);
			BringDownPopup((Widget)GetPopupGizmoShell(fwp->createWindow));
		} else
			OlSetInputFocus(tf, RevertToNone, CurrentTime);
		break;

    case CreateCancel:
		OlSetInputFocus(tf, RevertToNone, CurrentTime);
		XtPopdown((Widget)_OlGetShellOfWidget(w));
		break;
		
    case CreateHelp:
		{
		DmHelpAppPtr help_app = DmGetHelpApp(FOLDER_HELP_ID(Desktop));

		DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id,
			NULL, "DesktopMgr/folder.hlp", "420",
			UNSPECIFIED_POS, UNSPECIFIED_POS);
		OlSetInputFocus(tf, RevertToNone, CurrentTime);
		}
		break;
    }
}					/* End of CreateCB */


/****************************procedure*header*****************************
    CreateFile - callback for New operation to create a new file.
	open is used as a flag to open or not open the file after
	it's created.  fwp->create_prompt.flag is used to indicate
	whether a template should be used.
*/

static int
CreateFile(DmFolderWinPtr fwp, Boolean open)
{
	Widget tf;
	DmFileType  ftype;
	DmFclassPtr fcp;
	Boolean success = False;

	/* Clear message area */
	DmVaDisplayStatus((DmWinPtr)fwp, False, NULL);

	if (VerifyInputs(fwp) == -1)
		return(0);

	tf = (Widget)QueryGizmo(PopupGizmoClass, fwp->createWindow,
			GetGizmoWidget, "name");

	fcp = FCLASS_PTR((DmItemPtr)fwp->create_prompt.userdata);

	if (((DmFmodeKeyPtr)(fcp->key))->attrs & DM_B_SYS) {
		if (strcmp(((DmFmodeKeyPtr)(fcp->key))->name, "DIR") == 0) {
			if ((mkdir(fwp->create_prompt.current,
				DESKTOP_UMASK(Desktop))) != -1) {

				DmVaDisplayStatus((DmWinPtr)fwp, False, TXT_CREATE_SUCCESS,
					basename(fwp->create_prompt.current));
				success = True;
				DmDisplayStatus((DmWinPtr)fwp);

			} else {
				/* check errno & display reason for failure */
				if (errno == EACCES) {
					/* no permission */
					DmVaDisplayStatus((DmWinPtr)fwp, True,
						TXT_CREATE_NOPERM);
				} else {
					/* failure due to other reasons */
					DmVaDisplayStatus((DmWinPtr)fwp, True,
						TXT_CREATE_FAILED,
						basename(fwp->create_prompt.current));
				}
				return(0);
			}
		} else if (strcmp(((DmFmodeKeyPtr)(fcp->key))->name, "DATA") == 0) {
			int new_fd;

			/* turn off execute bit */
			if (((new_fd = creat(fwp->create_prompt.current,
				DESKTOP_UMASK(Desktop) &
				~(S_IXUSR | S_IXGRP | S_IXOTH)))) != -1) {
					close(new_fd);

					DmVaDisplayStatus((DmWinPtr)fwp, False,
						TXT_CREATE_SUCCESS,
						basename(fwp->create_prompt.current));
					success = True;
					DmDisplayStatus((DmWinPtr)fwp);
			} else {
				/* check errno & display reason for failure */
				if (errno == EACCES) {
					/* no permission */
					DmVaDisplayStatus((DmWinPtr)fwp, True,
						TXT_CREATE_NOPERM);
				} else {
					/* failure due to other reasons */
					DmVaDisplayStatus((DmWinPtr)fwp, True,
						TXT_CREATE_FAILED,
						basename(fwp->create_prompt.current));
				}
				return(0);
			}

		} else {
			/* should never get here */
			open = False;
			return(0);
		}

	} else if (strcmp(((DmFnameKeyPtr)(fcp->key))->name, "DIR") == 0) {
			if ((mkdir(fwp->create_prompt.current,
				DESKTOP_UMASK(Desktop))) != -1) {

				DmVaDisplayStatus((DmWinPtr)fwp, False,
					TXT_CREATE_SUCCESS,
					basename(fwp->create_prompt.current));
				success = True;
				DmDisplayStatus((DmWinPtr)fwp);

			} else {
				/* check errno & display reason for failure */
				if (errno == EACCES) {
					/* no permission */
					DmVaDisplayStatus((DmWinPtr)fwp, True,
						TXT_CREATE_NOPERM);
				} else {
					/* failure due to other reasons */
					DmVaDisplayStatus((DmWinPtr)fwp, True,
						TXT_CREATE_FAILED,
						basename(fwp->create_prompt.current));
				}
				return(0);
			}
	} else if (strcmp(((DmFnameKeyPtr)(fcp->key))->name, "DATA") == 0 &&
				fwp->create_prompt.previous == NULL) {
		int new_fd;

		/* Creating a file of DATA file class and not using a template.
		 * if a tmeplate is being used, this will fall through to
		 * calling DmDoFileOp later on in this function.
		 */

		/* turn off execute bit */
		if (((new_fd = creat(fwp->create_prompt.current,
			DESKTOP_UMASK(Desktop) & ~(S_IXUSR | S_IXGRP | S_IXOTH)))) != -1)
		{
			close(new_fd);
			DmVaDisplayStatus((DmWinPtr)fwp, False, TXT_CREATE_SUCCESS,
				basename(fwp->create_prompt.current));
			success = True;
			DmDisplayStatus((DmWinPtr)fwp);
		} else {
			/* check errno & display reason for failure */
			if (errno == EACCES) {
				/* no permission */
				DmVaDisplayStatus((DmWinPtr)fwp, True, TXT_CREATE_NOPERM);
			} else {
				/* failure due to other reasons */
				DmVaDisplayStatus((DmWinPtr)fwp, True, TXT_CREATE_FAILED,
					basename(fwp->create_prompt.current));
			}
			return(0);
		}
	} else {
		DmFileOpInfoPtr opr_info;

		if (fwp->create_prompt.flag != DM_USE_TEMPLATE) {
			DmVaDisplayStatus((DmWinPtr)fwp, True, TXT_NO_TEMPLATE);
			return(0);
		}

		/* if open is True, set flag to DM_CREATE_N_OPEN */
		if (open)
			fwp->create_prompt.flag = DM_CREATE_N_OPEN;

		/* load parameters into opr_info struct */
		opr_info = (DmFileOpInfoPtr)MALLOC(sizeof(DmFileOpInfoRec));
		opr_info->type		  = DM_COPY;
		opr_info->options	  = 0;
		opr_info->target_path = strdup(fwp->create_prompt.current);
		opr_info->src_path	  = dirname(strdup(fwp->create_prompt.previous));
		opr_info->src_list	  = (char **)MALLOC(sizeof(char *));
		opr_info->src_cnt	  = 1;
		opr_info->src_win	  = NULL;
		opr_info->dst_win	  = fwp;
		opr_info->x		  = opr_info->y = UNSPECIFIED_POS;

		opr_info->src_list[0] =
				strdup(basename(fwp->create_prompt.previous));

		(void)DmDoFileOp(opr_info, NewFileFMProc, NULL);
		return(0);
	}

	/* Successfully created file, now open it. (Note that item is added
	   to folder but may not be displayed due to filter).
	*/
	if (open && success) {
	    DmObjectPtr obj;

	    obj = Dm__CreateObj(fwp->cp,
				basename(fwp->create_prompt.current),
				DM_B_SET_TYPE | DM_B_INIT_FILEINFO);

		if (obj != NULL) {
			Dimension	wrap_width;
			Cardinal	indx;

			XtSetArg(Dm__arg[0], XtNwidth, &wrap_width);
			XtGetValues(fwp->box, Dm__arg, 1);

			indx = DmAddObjToFolder(fwp, obj, UNSPECIFIED_POS,
					UNSPECIFIED_POS, wrap_width);

			if (indx != OL_NO_ITEM) {
				/* Must keep items sorted if not ICONIC */
				if (fwp->view_type != DM_ICONIC)
					DmSortItems(fwp, fwp->sort_type, 0, 0, wrap_width);

				/* "open" file */
				DmOpenObject((DmWinPtr)fwp, obj);
				return(1);
			}
		}
		DmVaDisplayStatus((DmWinPtr)fwp, True, TXT_CANT_OPEN_FILE,
			fwp->create_prompt.current);
		return(1);
	}
}					/* End of CreateFile */


/****************************procedure*header*****************************
    NewFileFMProc- This is called from DmDoFileOp() with DM_DONE or DM_ERROR
	from an operation to create a new file using a template;
	i.e. DmDoFileOp() was called with with DM_COPY request.
*/

static void
NewFileFMProc(DmProcReason reason, XtPointer client_data,
	      XtPointer call_data, char * src, char * dst)
{
    DmTaskInfoListPtr	tip = (DmTaskInfoListPtr)call_data;
    DmFileOpInfoPtr	opr_info = (DmFileOpInfoPtr)tip->opr_info;
    DmFolderWindow	win  = opr_info->dst_win;
    char *		name = basename(opr_info->target_path);

    switch(reason)
    {
    case DM_DONE:
	if (!(opr_info->src_info[0] & SRC_B_IGNORE))
	{
	    DmUpdateWindow(opr_info, DM_UPDATE_DSTWIN);

	    if (win->create_prompt.flag == DM_CREATE_N_OPEN)
	    {
		DmItemPtr itp = DmObjNameToItem((DmWinPtr)win, name);

		if (itp != NULL)
		    DmOpenObject((DmWinPtr)win, ITEM_OBJ(itp));
	    }

	    DmVaDisplayStatus((DmWinPtr)win, False, TXT_CREATE_SUCCESS, name);
	    BringDownPopup((Widget)GetPopupGizmoShell(win->createWindow));
	}

	DmFreeTaskInfo(tip);
	break;

    case DM_ERROR:
	/* display reason for failure */
	DmVaDisplayStatus((DmWinPtr)win, True, TXT_CREATE_FAILED, name);
	break;
    }
}					/* end of NewFileFMProc() */

/****************************procedure*header*****************************
 * This function returns a 0 if the selected file type has one or more
 * and none is selected or a selected template is valid (i.e. it exists)
 * otherwise, it returns a -1. Also set fwp->create_prompt.flag to
 * DM_USE_TEMPLATE if selected template is valid.
 */
static int
VerifyTmpl(DmFolderWinPtr fwp)
{
	Widget		flist;
	struct stat	mystat;
	ListNodePtr	tnodep;
	int			nitems;
	int			i;
	char			*s;
	char			*full_path = NULL;
	char			*tmpl_dir = NULL;
	Boolean		set = False;

	if (fwp->create_prompt.previous) {
		FREE(fwp->create_prompt.previous);
		fwp->create_prompt.previous = NULL;
	}

	flist = (Widget)QueryGizmo(PopupGizmoClass, fwp->createWindow,
				GetGizmoWidget, "template");

	XtSetArg(Dm__arg[0], XtNnumItems, &nitems);
	XtGetValues(flist, Dm__arg, 1);

	if (nitems == 0)
		/* no template */
		return(0);

	/* find selected item, if any */
	for (i = 0; i < nitems; i++) {

		XtSetArg(Dm__arg[0], XtNset, &set);
		OlFlatGetValues(flist, i, Dm__arg, 1);

		if (set) {
			XtSetArg(Dm__arg[0], XtNformatData, &tnodep);
			OlFlatGetValues(flist, i, Dm__arg, 1);
			break;
		}
	}

	if (!set) {
		/* no template selected */ 
		return(0);
	}

	/* check if selected template exists */
	s = tnodep->name;
	if (*s != '/') {
		if ((tmpl_dir = (char *)DmGetDTProperty(TEMPLATEDIR, NULL))) {
			full_path = DmMakePath(tmpl_dir, tnodep->name);
		} else {
			full_path = XtResolvePathname(DESKTOP_DISPLAY(Desktop),
						"template", tnodep->name, NULL, NULL, NULL,
						0, NULL);
		}

		if (full_path == NULL) {
			/* give up, don't know where else to look */
			DmVaDisplayStatus((DmWinPtr)fwp, True, TXT_BAD_TEMPLATE,
				tnodep->name);
			return(-1);
		}
	} else {
		full_path = tnodep->name;
	}

	if (stat(full_path, &mystat) == 0) {
		fwp->create_prompt.previous = strdup(full_path);
		fwp->create_prompt.flag     = DM_USE_TEMPLATE;
		return(0);
	} else {
		DmVaDisplayStatus((DmWinPtr)fwp, True, TXT_BAD_TEMPLATE, full_path);
		return(-1);
	}
}				/* end of TmplExist() */

/****************************procedure*header*****************************
 * This function parses the value of the TEMPLATE file class property
 * and displays the template(s) in a flat list widget.  The value
 * can comprise one or a list of templates.
 */
static void
UpdateTemplates(DmFolderWindow fwp, char * template, Widget flist)
{
	char *p;
	char *s;
	int cnt = 0;
	int n = 0;
	ListTokenPtr list_items;
	ListNodePtr tnodep;
	ListNodePtr head = NULL;

	if ((p = strchr(template, ',')) == NULL) {  /* only one template */
		tnodep = (ListNodePtr)MALLOC(sizeof(ListNodeRec));
		tnodep->name = (char *)strdup(template);
		tnodep->next = NULL;
		head = tnodep;

		/* build list items */
		list_items = (ListTokenPtr)MALLOC(sizeof(ListTokenRec));
		list_items->set        = (XtArgVal)False;
		list_items->formatData = (XtArgVal)tnodep;
		cnt = 1;

	} else { /* multiple templates */
		ListNodePtr  t;
		ListTokenPtr lip;
		ListNodePtr  save = NULL;

		p = NULL;
		s = template;

		while (*s != '\n') {
			if ((p = strchr(s, ',')) != NULL) {
				tnodep = (ListNodePtr)MALLOC(sizeof(ListNodeRec));
				tnodep->name = (char *)strndup(s, p - s);
				tnodep->next = NULL;
				if (save)
					save->next = tnodep;
				save = tnodep;
				/* save beginning of list */
				if (head == NULL)
					head = tnodep;
				++cnt;

				/* skip ',' */
				s = ++p;
			} else {
				tnodep = (ListNodePtr)MALLOC(sizeof(ListNodeRec));
				tnodep->name = strdup(s);
				tnodep->next = NULL;
				if (save)
					save->next = tnodep;
				save = tnodep;
				if (head == NULL)
					head = tnodep;
				++cnt;
				break;
			}
		}
		list_items = lip = (ListTokenPtr)MALLOC(cnt * sizeof(ListTokenRec));
		for (t = head; t; t = t->next, lip++) {
    			/* build list items */
			lip->set        = (XtArgVal)False;
			lip->formatData = (XtArgVal)t;
		}
	}
	
	n = 0;
	XtSetArg(Dm__arg[n], XtNitems,    NULL); ++n;
	XtSetArg(Dm__arg[n], XtNnumItems, 0); ++n;
	XtSetValues(flist, Dm__arg, n);

	n = 0;
	XtSetArg(Dm__arg[n], XtNitems,          list_items); ++n;
	XtSetArg(Dm__arg[n], XtNnumItems,       cnt); ++n;
	XtSetArg(Dm__arg[n], XtNitemFields,     list_fields); ++n;
	XtSetArg(Dm__arg[n], XtNnumItemFields,  XtNumber(list_fields)); ++n;
	XtSetArg(Dm__arg[n], XtNclientData,     fwp); ++n;
	XtSetArg(Dm__arg[n], XtNnoneSet,        False); ++n;
	XtSetArg(Dm__arg[n], XtNuserData,       list_items); ++n;
	XtSetValues(flist, Dm__arg, n);
	SetInputText(fwp->createWindow, 2, basename(head->name), True);

}				/* end of UpdateTemplates() */

/****************************procedure*header*****************************
    VerifyInputs- This function verifies the input in the create new
	file prompt window.  Returns a 0 if all inputs are valid;
	otherwise, returns a -1.
*/

static int
VerifyInputs(DmFolderWindow fwp)
{
	int			i;
	CListGizmo	*clist;
	struct stat	mystat;
	char			*p;
	char			*file_name;
	char			*str = NULL;
	DmItemPtr   	itp;
	DmFclassPtr	fcp;

	/* check if a file type is selected */
	clist = (CListGizmo *)QueryGizmo(PopupGizmoClass,
 					fwp->createWindow, GetGizmoGizmo, "clist");

	fwp->create_prompt.userdata = NULL;
	for(i = 0, itp = clist->itp; i < clist->cp->num_objs ; itp++, i++) {
		if (ITEM_MANAGED(itp) && ITEM_SELECT(itp)) {
			fwp->create_prompt.userdata = (XtPointer)itp;
			break;
		}
	}

	if (fwp->create_prompt.userdata == NULL) {
		DmVaDisplayStatus((DmWinPtr)fwp, True, TXT_NO_FILE_TYPE);
		return(-1);
	}

	if (fwp->create_prompt.current) {
		FREE(fwp->create_prompt.current);
		fwp->create_prompt.current = NULL;
	}

	/* get file name */
	str = GetInputText(fwp->createWindow, 2);

	if ((str == NULL) || (str[0] == '\0')) {
		DmVaDisplayStatus((DmWinPtr)fwp, True, TXT_NO_FILE_NAME);
		return(-1);
	}

	if (str[0] != '/') { /* file name is not a full path */
		struct statvfs	v;

		/* make sure there's no intermediate directories. */
		if (strchr(str, '/') != NULL) {
			DmVaDisplayStatus((DmWinPtr)fwp, True, TXT_IN_THIS_FOLDER);
			FREE(str);
			return(-1);
		}
			
		/* make sure file name does not exceed NAME_NAX */
		statvfs(fwp->cp->path, &v);
		if (strlen(str) > v.f_namemax) {
			DmVaDisplayStatus((DmWinPtr)fwp, True, TXT_NAME_TOO_LONG,
				v.f_namemax);
			FREE(str);
			return(-1);
		}

		/* make sure file name has no double qoutes */
		if (strchr(str, '"')) {
			DmVaDisplayStatus((DmWinPtr)fwp, True, TXT_DBL_QOUTES_IN_NAME);
			FREE(str);
			return(-1);
		}
		/* construct a full path name */
		file_name = DmMakePath(fwp->cp->path, str);
		FREE(str);

	} else {				/* 'str' is a full path */

		/* A full path name is entered. Make sure new file is */
		/* within current directory.                          */
		struct statvfs	v;
		char	*s;
		char	*ss;
		char	*p;
		char	*pp;
		char	*t;

		/* fwp->cp->path may have redundant '/' in front of it */
		/* skip any preceeding '/' in it as well as in str     */
		s = ss = strdup(str);
		while (*s == '/')
			++s;

		p = pp = strdup(fwp->cp->path);
		while (*p == '/')
			++p;

		/* make sure file is to reside in current folder */
		if (strncmp(p, s, strlen(p)) != 0) {
			DmVaDisplayStatus((DmWinPtr)fwp, True, TXT_IN_THIS_FOLDER);
			FREE(ss);
			FREE(pp);
			FREE(str);
			return(-1);
		}

		/* skip the '/' after the folder path */
		t = s + strlen(p) + 1;

		/* make sure there are no subdirectories */
		if (strchr(t, '/')) {
			DmVaDisplayStatus((DmWinPtr)fwp, True, TXT_IN_THIS_FOLDER);
			FREE(ss);
			FREE(pp);
			FREE(str);
			return(-1);
		}

		/* make sure file name does not exceed NAME_NAX */
		statvfs(fwp->cp->path, &v);
		if (strlen(file_name) > v.f_namemax) {
			DmVaDisplayStatus((DmWinPtr)fwp, True, TXT_NAME_TOO_LONG,
				v.f_namemax);
			FREE(ss);
			FREE(pp);
			FREE(str);
			return(-1);
		}

		/* make sure file name has no embedded space(s) */
		if (strchr(str, '"')) {
			DmVaDisplayStatus((DmWinPtr)fwp, True, TXT_DBL_QOUTES_IN_NAME);
			FREE(ss);
			FREE(pp);
			FREE(str);
			return(-1);
		}
		file_name = str;
		FREE(ss);
		FREE(pp);
		FREE(str);
	}

	/* make sure file does not already exist */
	errno = 0;
	if (stat(file_name, &mystat) == 0)
	{
		DmVaDisplayStatus((DmWinPtr)fwp, True, TXT_ALREADY_EXISTS,
			basename(file_name));
		return(-1);
	}

    /* make sure the file name specified matches the value of the */
    /* selected file class's PATTERN property, if it's defined.   */

	fcp = FCLASS_PTR((DmItemPtr)fwp->create_prompt.userdata);
	p = DtGetProperty(&(fcp->plist), PATTERN, NULL);
	if (p) {
		if (!gmatch(basename(file_name), p)) {
			DmVaDisplayStatus((DmWinPtr)fwp, True, TXT_NOT_MATCH_PATTERN,p);
			return(-1);
		}
	}
	fwp->create_prompt.current = strdup(file_name);

	/* Check if a template is selected and if it's valid.
	 * fwp->create_prompt.flag may be altered in VerifyTmpl.
	 */
	fwp->create_prompt.flag = DM_DONT_USE_TEMPLATE;
	if (VerifyTmpl(fwp) == -1) {
		if (fwp->create_prompt.current)
			FREE(fwp->create_prompt.current);
		fwp->create_prompt.current = NULL;
		return(-1);
	}
	/* inputs are valid */
	return(0);
}					/* End of VerifyInputs */

/***************************private*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    DmPromptCreateFile - create popup to prompt for info to create new file.
*/
void
DmPromptCreateFile(DmFolderWindow fwp)
{
	Widget flist;
	Widget shell;
	Widget tf;
	CListGizmo *clist_gizmo = NULL;

	DmVaDisplayStatus((DmWinPtr)fwp, False, NULL);

	if (fwp->createWindow == NULL) {
		Widget caption;
		LabelGizmo *labelGiz;
		int nitems = 0;
		int i = 0;
		DmItemPtr itp;
		DmFclassPtr fcp;
		char *template = NULL;

		fwp->createWindow = CopyGizmo(PopupGizmoClass, &create_popup);

		/* set client_data of button to the folder window pointer */
		((MenuGizmo *)(fwp->createWindow->menu))->client_data =
			(XtPointer)(fwp);

		CreateGizmo(fwp->shell, PopupGizmoClass, fwp->createWindow,
			NULL, 0);

		shell = GetPopupGizmoShell(fwp->createWindow);

		tf = (Widget)QueryGizmo(PopupGizmoClass, fwp->createWindow,
				GetGizmoWidget, "name");

		XtSetArg(Dm__arg[0], XtNfocusWidget, tf);
		XtSetValues(shell, Dm__arg, 1);

		clist_gizmo = (CListGizmo *)QueryGizmo(PopupGizmoClass,
					fwp->createWindow, GetGizmoGizmo, "clist");

		/* make Datafile the default */
		XtVaGetValues(clist_gizmo->boxWidget, XtNnumItems, &nitems, NULL);

		for (i = 0, itp = clist_gizmo->itp; i < nitems; i++, itp++) {
			if (itp->managed) {
				fcp = FCLASS_PTR(itp);
				/* check for both built-in and user-defined DATA
				 * file class.
				 */
				if (strcmp(((DmFmodeKeyPtr)(fcp->key))->name,
					"DATA") == 0) {

					OlVaFlatSetValues(clist_gizmo->boxWidget,
						(itp - clist_gizmo->itp), XtNset, True, NULL);
					break;
				}
			}
		}
		XtSetArg(Dm__arg[0], XtNclientData, (XtPointer)fwp);
		XtSetArg(Dm__arg[1], XtNitemsTouched, True);
		XtSetValues(clist_gizmo->boxWidget, Dm__arg, 2);

		flist = (Widget)QueryGizmo(PopupGizmoClass, fwp->createWindow,
					GetGizmoWidget, "template");

		XtSetArg(Dm__arg[0], XtNclientData, (XtPointer)fwp);
		XtSetValues(flist, Dm__arg, 1);

		labelGiz = (LabelGizmo *)(fwp->createWindow->gizmos[1].gizmo);
		caption = (Widget)QueryGizmo(LabelGizmoClass, labelGiz,
					GetGizmoWidget, "caption");

		/* get value of TEMPLATE property of Datafile file class */
		template = DtGetProperty(&(ITEM_OBJ(itp)->fcp->plist),TEMPLATE,NULL);
		if (template) {
			XtSetSensitive(caption, True);
			UpdateTemplates(fwp, template, flist);
		} else {
			/* no template, make list widget insensitive */
			XtSetSensitive(caption, False);
		}

		/* Initialize file name, template name and itp to NULL */
		fwp->create_prompt.current  = NULL;
		fwp->create_prompt.previous = NULL;
		fwp->create_prompt.userdata = NULL;
	}
	MapGizmo(PopupGizmoClass, fwp->createWindow);

}				/* End of PromptCreateFile */

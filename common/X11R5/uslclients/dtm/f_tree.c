/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:f_tree.c	1.90"

/******************************file*header********************************

    Description:
	This file contains the source code for tree view.
*/
						/* #includes go here	*/
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <Xol/FlatP.h>		/* to get label_gc from Flat */
#include <Xol/OlCursors.h>
#include <buffutil.h>

#include <Gizmos.h>
#include <BaseWGizmo.h>
#include <MenuGizmo.h>
#include <FileGizmo.h>
#include <PopupGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "SWinGizmo.h"
#include "StatGizmo.h"

/*****************************file*variables******************************

    Declare these struct here before forward declarations below since
    some functions use these types.
*/

typedef struct _Node {
    DmObjectRec	obj;
    u_short	level;
    Boolean	subdirs_shown;
    Boolean	has_subdirs;
} Node;

typedef Bufferof(Node)		Tree;


/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/
static void	AddNode(char * path, Node * parent, Node * old_node);
static void	AddToTree(Tree * branch, Cardinal indx);
static void	BeginHere(DmFolderWindow, DmItemRec *);
static void     BeginOtherCB(Widget, XtPointer, XtPointer);
static void	BeginParent(DmFolderWindow tree_win);
static void	BeginTree(Widget, char * path);
static void	CloseTreeViewCB(Widget, XtPointer, XtPointer);
static int	Compare(const void *, const void *);
static int	I18NCompare(const void *, const void *);
static void	CreateIcons(Tree *, DmItemRec **);
static Tree *	CreateTree(char * path, u_short level, u_short depth);
static void	DeleteBranch(Tree * root, Cardinal indx, Boolean inclusive);
static void	DeleteFromTree(Cardinal, Boolean inclusive);
static void	DeleteIcons(DmItemRec **, Cardinal *, Cardinal, int);
static void	ExposureEH(Widget, XtPointer, XEvent *, Boolean *);
static void	Find(Node *, Cardinal, char *, Node **, Node **);
static void	FixLinks(Tree * root, DmItemRec * items);
static void	FreeTree(Node * root_node, int cnt);
static char *	HideFolders(DmFolderWindow tree_win, Cardinal item_index);
static void	InsertIcons(DmItemRec **, Cardinal *, Cardinal, DmItemRec *, int);
static Node *	ParentNode(Node *);
static char *	ShowAllFolders(DmFolderWindow tree_win, Cardinal item_index);
static char *	ShowFolders(DmFolderWindow, Cardinal, u_short depth);
static void	ViewMenuCB(Widget, XtPointer, XtPointer);
static void	WalkTree(Tree *, char *, u_short, u_short);
static void	WMCB(Widget w, XtPointer client_data, XtPointer call_data);

					/* public procedures		*/
void		DmDrawTreeIcon(Widget, XtPointer, XtPointer);
void		DmFolderOpenTreeCB(Widget, XtPointer, XtPointer);
void		TreeIconMenuCB(Widget, XtPointer, XtPointer);
void		Dm__UpdateTreeView(char * dir1, char * dir2);


/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

#define INIT_NODE(node, node_name, node_level, stat_buf) \
    (node)->obj.container	= TREE_WIN(Desktop)->cp; \
    (node)->obj.name		= strdup(node_name); \
    (node)->obj.ftype		= DM_FTYPE_DIR; \
    (node)->obj.fcp		= fmodeKey->fcp; \
    (node)->obj.attrs		= 0; \
    (node)->obj.plist.count	= 0; \
    (node)->obj.plist.ptr	= NULL; \
    (node)->level		= node_level; \
    (node)->subdirs_shown	= False; \
    (node)->has_subdirs		= False; \
    DmInitObj(&(node)->obj, stat_buf, DM_B_INIT_FILEINFO)

/* Note: src->used must be incremented just before calling InsertIntoBuffer
   (then decremented) to work around func's string-dependency.
*/
#define InsertBuffer(target, src, indx) \
	(src)->used++; \
	InsertIntoBuffer((Buffer *)(target), (Buffer *)(src), indx); \
	(src)->used--

#define GrowBuf(buf, inc)	GrowBuffer(buf, inc); (buf)->used += inc

/* NOTE: A ptr to a FmodeKey struct for a tree_win (dir type) is saved
   statically while in tree view.  Perhaps this can remain.  The small icon
   is used for tree view.
*/
static DmFmodeKeyPtr fmodeKey;

#define ICON_WIDTH		fmodeKey->small_icon->width
#define ICON_HEIGHT		fmodeKey->small_icon->height
#define HORIZ_PAD		OlMMToPixel(OL_HORIZONTAL, 4)
#define VERT_PAD		OlMMToPixel(OL_VERTICAL, 4)
#define Y_DELTA			(Dimension)( (int)ICON_HEIGHT + (int)VERT_PAD )

#define TreeWin			TREE_WIN(Desktop)
#define TreePath(win)		DM_WIN_PATH(win)
#define NODE(item)		( (Node *)ITEM_OBJ(item) )
#define ITEM_NODE(items, i)	NODE(items + i)
#define ROOT_NODE(root_item)	NODE(TreeWin->itp)  /* Note: 'item' unused */
#define BUF_NODE(buf, indx)	( (buf).p + indx )
#define IS_SYMLINK(node)	( (node)->obj.attrs & DM_B_SYMLINK )
#define NAME(node)		( (node)->obj.name )
#define PATH(node)		( ROOT_DIR(NAME(node)) ? \
				 NAME(node) : DmObjPath(&(node)->obj) )

/* Make node name by removing "root" path from fullpath. */
#define MakeNodeName(fullpath, root_len) \
    strcpy(Dm__buffer, (fullpath) + (root_len) + 1)

/* Put these extern function prototypes here instead of in extern.h to avoid
   compiler warning, "dubious tag declaration", about use of 'stat *'.
*/
extern int	DmValidatePrompt(Widget, char *, char **, struct stat *);
extern void	DmInitObj(DmObjectPtr, struct stat *, DtAttrs);

/******************************gizmo*structs*******************************
    Gizmo struct's for Tree view window
*/

#define B_A	(XtPointer)DM_B_ANY
#define B_O	(XtPointer)DM_B_ONE
#define B_M	(XtPointer)DM_B_ONE_OR_MORE
#define B_U     (XtPointer)DM_B_UNDO

static MenuItems FMapFileMenuItems[] = {
  { True, TXT_FILE_OPEN,   TXT_M_FILE_OPEN,   NULL, DmFileOpenCB,    B_M },
  { True, TXT_FILE_COPY,   TXT_M_FILE_COPY,   NULL, DmFileCopyCB,    B_M },
  { True, TXT_FILE_RENAME, TXT_M_FILE_RENAME, NULL, DmFileRenameCB,  B_O },
  { True, TXT_FILE_MOVE,   TXT_M_FILE_MOVE,   NULL, DmFileMoveCB,    B_M },
  { True, TXT_FILE_LINK,   TXT_M_FILE_LINK,   NULL, DmFileLinkCB,    B_M },
  { True, TXT_FILE_DELETE, TXT_M_FILE_DELETE, NULL, DmFileDeleteCB,  B_M },
  { True, TXT_FILE_PRINT,  TXT_M_FILE_PRINT,  NULL, DmFilePrintCB,   B_M },
  { True, TXT_FILE_PROP,   TXT_M_FILE_PROP,   NULL, DmFilePropCB,    B_M },
  { True, TXT_FILE_EXIT,   TXT_M_FILE_EXIT,   NULL, CloseTreeViewCB, B_A },
  { NULL }
};

MENU("fmapfilemenu", FMapFileMenu);

static MenuItems FMapViewMenuItems[] = {
  { True, TXT_TREE_SHOW,    TXT_M_TREE_SHOW,     NULL, ViewMenuCB,  B_M },
  { True, TXT_TREE_HIDE,    TXT_M_TREE_HIDE,     NULL, ViewMenuCB,  B_M },
  { True, TXT_TREE_SHOW_ALL,TXT_M_TREE_SHOW_ALL, NULL, ViewMenuCB,  B_M },
  { True, TXT_TREE_HERE,    TXT_M_TREE_HERE,     NULL, ViewMenuCB,  B_O },
  { True, TXT_TREE_MAIN,    TXT_M_TREE_MAIN,     NULL, ViewMenuCB,  B_A },
  { True, TXT_TREE_OTHER,   TXT_M_TREE_OTHER,    NULL, ViewMenuCB,  B_A },
  { True, TXT_TREE_UP,      TXT_M_TREE_UP,       NULL, ViewMenuCB,  B_A },
  { NULL }
};

MENU("fmapviewmenu", FMapViewMenu);

static MenuItems FMapEditMenuItems[] = {
 {True, TXT_EDIT_SELECT,   TXT_M_EDIT_SELECT,   NULL, DmEditSelectAllCB,  B_A},
 {True, TXT_EDIT_UNSELECT, TXT_M_EDIT_UNSELECT, NULL, DmEditUnselectAllCB,B_M},
 {False,TXT_EDIT_UNDO,     TXT_M_EDIT_UNDO,     NULL, DmEditUndoCB,       B_U},
 {NULL}
};

MENU("fmapeditmenu", FMapEditMenu);

static MenuItems FMapHelpMenuItems[] = {
 { True, TXT_HELP_FMAP,     TXT_M_HELP_FOLDER,   NULL, DmHelpSpecificCB, B_A },
 { True, TXT_HELP_TOC,      TXT_M_HELP_TOC,      NULL, DmHelpTOCCB,      B_A },
 { True, TXT_HELP_HELPDESK, TXT_M_HELP_HELPDESK, NULL, DmHelpDeskCB,     B_A },
 { NULL }
};

MENU("fmaphelpmenu", FMapHelpMenu);

static MenuItems FMapMenuBarItems[] = {
  { True, TXT_FILE, 	TXT_M_FILE,  (void *)&FMapFileMenu,  DmMenuSelectCB}, 
  { True, TXT_EDIT,	TXT_M_EDIT,  (void *)&FMapEditMenu,  DmMenuSelectCB},
  { True, TXT_VIEW,	TXT_M_VIEW,  (void *)&FMapViewMenu,  DmMenuSelectCB},
  { True, TXT_HELP,	TXT_M_HELP,  (void *)&FMapHelpMenu,  DmMenuSelectCB},
  { NULL }
};

MENUBAR("fmapmenubar", FMapMenuBar);

#undef B_A
#undef B_O
#undef B_M
#undef B_U

static SWinGizmo swin_gizmo = { "swin" };
static StatusGizmo status_gizmo = { "status", 50 /* left percent */ };
static GizmoRec fmap_gizmos[] = {
  { SWinGizmoClass,	&swin_gizmo },
  { StatusGizmoClass,	&status_gizmo },
};

static HelpInfo FMapWinHelp =
{ TXT_FOLDER_MAP, NULL, "DesktopMgr/fmap.hlp", NULL, NULL };

static BaseWindowGizmo FMapWindow = {
    &FMapWinHelp,		/* help */
    "foldermap",		/* shell widget name */
    TXT_FOLDER_MAP,		/* title */
    &FMapMenuBar,		/* menu bar */
    fmap_gizmos,		/* gizmo array */
    XtNumber(fmap_gizmos),	/* # of gizmos in array */
    "",				/* icon_name */
    "fmap48.icon",		/* name of pixmap file */
    " ",			/* error message */
    " ",			/* status message */
    100				/* % of footer for error message */
};

/* Define Gizmo structs for "BeginOther" prompt */

static MenuItems  BeginOtherMenubarItems[] = {
  { True, TXT_OPEN,	TXT_M_OPEN },
  { True, TXT_CANCEL,	TXT_M_CANCEL },
  { True, TXT_P_HELP,	TXT_M_HELP },
  { NULL }
};

static MenuGizmo BeginOtherMenubar =
  { NULL, "filemapmenu", "_X_", BeginOtherMenubarItems, BeginOtherCB };

static HelpInfo FMapOpenWinHelp =
{ TXT_FOLDER_MAP, NULL, "DesktopMgr/fmap.hlp", "300", NULL };

static FileGizmo BeginOtherPrompt =
  { &FMapOpenWinHelp, "filemapprompt", TXT_OPEN_TITLE,
	&BeginOtherMenubar, "", "", ".", FOLDERS_AND_FOLDERS };

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    path_strcmp- compare path strings
*/
static int
path_strcmp(char * p1, char * p2)
{
    int		result;

    if (strcmp(setlocale(LC_COLLATE, NULL), "C") != 0) /* not in C locale */
	result = strcoll(p1, p2);
    else
    {
	char *	fullpath1 = strdup( p1 );
	char *      fullpath2 = strdup( p2 );
	char *      p;

	for (p = fullpath1; *p; p++)
	   if (*p == '/')
	      *p = '\1';

	for (p = fullpath2; *p; p++)
	   if (*p == '/')
	      *p = '\1';

	result = strcmp(fullpath1, fullpath2);

	FREE(fullpath1);
	FREE(fullpath2);
    }

    return(result);

} /* end of path_strcmp */

/****************************procedure*header*****************************
    AddNode- given non-NULL parent node and unique path, add node for 'path'.
*/
static void
AddNode(char * path, Node * parent, Node * old_node)
{
    Node *	node;
    Node	new;
    Tree	branch;
    int		root_len;

    if (!parent->has_subdirs)		/* parent now has subdirs */
	parent->has_subdirs = True;

    if (!parent->subdirs_shown)		/* and subdirs are shown */
	parent->subdirs_shown = True;

    /* Find insertion point.  We know there is no duplicate */
    for (node = parent + 1;
	 node < ROOT_NODE(TreeWin) + TreeWin->nitems; node++)
	if (path_strcmp(PATH(node), path) > 0)
	    break;

    /* Add new node before 'node' */
    if ( (root_len = strlen(TreePath(TreeWin))) == 1 )
	root_len = 0;
    if (old_node == NULL)
    {
	/* Initialize new node */
	INIT_NODE(&new, MakeNodeName(path, root_len), parent->level + 1, NULL);
    } else
    {
	new = *old_node;
	old_node->obj.objectdata = NULL;
	new.obj.name = strdup(MakeNodeName(path, root_len));
	new.level = parent->level + 1;
	new.subdirs_shown = False;	/* though it might HAVE subdirs */
    }

    /* Insertion requires using buffers so use pseudo one here. */
    branch.p	= &new;
    branch.size	= branch.used = 1;
    branch.esize= sizeof(Node);
    AddToTree(&branch, (--node) - ROOT_NODE(TreeWin));

}					/* end of AddNode */

/****************************procedure*header*****************************
    AddToTree- add 'branch' to tree AFTER index 'indx'.  Insert the
	node, make and insert icons, and fix links.
*/
static void
AddToTree(Tree * branch, Cardinal indx)
{
    Tree	root;
    DmItemPtr	items = NULL;

    /* Insertion requires using buffers so make use pseudo buffer here that
       points to the "root" of all the nodes.
    */
    root.p	= ROOT_NODE(TreeWin);
    root.size	= root.used = TreeWin->nitems;
    root.esize	= sizeof(Node);

    InsertBuffer(&root, branch, indx + 1);
    CreateIcons(branch, &items);
    InsertIcons(&TreeWin->itp, &TreeWin->nitems, indx, items, branch->used);
    FixLinks(&root, TreeWin->itp);
    FREE((void *)items);
}					/* end of AddToTree */

/****************************procedure*header*****************************
    BeginHere- called to re-root tree after user selects "Begin Here" from
	View menu or Icon menu.
*/
static void
BeginHere(DmFolderWindow tree_win, DmItemRec * item)
{
    char * path;

    /* If item is already root of the tree, return now */
    if (item - tree_win->itp == 0)
    {
	DmVaDisplayStatus((DmWinPtr)tree_win, True, TXT_ALREADY_HERE);
	return;
    }

    path = strdup( PATH(NODE(item)) );	/* since Dm__buffer used */
    BeginTree(tree_win->box, path);
    FREE(path);
}					/* end of BeginHere */

/****************************procedure*header*****************************
    BeginOtherCB- called when user selects "Open" on "Start At Other" prompt.
*/
static void
BeginOtherCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData *	p = (OlFlatCallData *)call_data;
    DmFolderWindow	tree_win;
    DmHelpAppPtr	help_app;
    char * 		foldername;
    struct stat		stat_buf;
    int			status;

    switch(p-> item_index)
    {
    case FilePromptTask:
	tree_win = TREE_WIN(Desktop);
	status = DmValidatePrompt(tree_win->folder_prompt->textFieldWidget,
				  tree_win->folder_prompt->directory,
				  &foldername, &stat_buf);
	/* Be sure input is a directory */
	if ((status == 0) && S_ISDIR(stat_buf.st_mode))
	{
	    BeginTree(tree_win->box, foldername);
	    BringDownPopup( (Widget)_OlGetShellOfWidget(widget) );

	} else
	{
	    DmVaDisplayStatus((DmWinPtr)tree_win, True, TXT_NOT_DIR, foldername);
	}
	if (foldername != NULL)
	    FREE(foldername);
	break;

    case FilePromptCancel:
	XtPopdown( (Widget)_OlGetShellOfWidget(widget) );
	break;

    case FilePromptHelp:
	help_app = DmGetHelpApp(FMAP_HELP_ID(Desktop));
	DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id, NULL,
		"DesktopMgr/fmap.hlp", "300", UNSPECIFIED_POS, UNSPECIFIED_POS);
	break;
    }
}					/* End of BeginOtherCB */

/****************************procedure*header*****************************
    BeginParent- called when user selects "Next Level Up" from View menu.
*/
static void
BeginParent(DmFolderWindow tree_win)
{
    Node *	node = ROOT_NODE(tree_win);
    char *	path;

    /* If this is already "/", return now */
    if (ROOT_DIR( NAME(node) ))
    {
	DmVaDisplayStatus((DmWinPtr)tree_win, True, TXT_NO_PARENT);
	return;
    }
    path = strdup( PATH(node) );
    BeginTree(tree_win->box, dirname(path));
    FREE(path);
}					/* end of BeginParent */

/****************************procedure*header*****************************
    BeginTree- root the tree view at 'path'
	(Widget 'widget' is passed so that busying the cursor is centralized)

	Note: be careful about freeing tree_win path since this may be
	the same as 'path' passed in (so save it in 'old_path').
*/
static void
BeginTree(Widget widget, char * path)
{
    DmFolderWindow	tree_win = TREE_WIN(Desktop);
    char *		old_path = TreePath(tree_win);
    char *		bname;
    Tree		root;

    /* This could take awhile so entertain the user */
    if (widget != NULL)
	BUSY_CURSOR(widget);

    /* Change the path of the (pseudo) container. "/" is special*/
    if (ROOT_DIR(path))
    {
	TreePath(tree_win) = strdup(path);
	bname = path;

    } else
    {
	char * dir = strdup(path);
	TreePath(tree_win) = strdup(dirname(dir));
	FREE(dir);
	bname = basename(path);
    }

    /* Now create "root" node and create tree and icons */
    root.size	= root.used = 0;
    root.esize	= sizeof(Node);
    root.p	= NULL;

    /* Make "starter" node and init with path and level '0' */
    GrowBuf((Buffer *)&root, 1);
    INIT_NODE(BUF_NODE(root, 0), bname, 0, NULL);

    /* Only create branch if "root" is not a symbolic link */
    if ( !IS_SYMLINK(BUF_NODE(root, 0)) )
    {
	Tree * branch = CreateTree(path, 1, 1 + TREE_DEPTH(Desktop) - 1);

	if (!BufferEmpty(branch))
	{
	    BUF_NODE(root, 0)->has_subdirs = True;
	    BUF_NODE(root, 0)->subdirs_shown = True;
	    InsertBuffer(&root, branch, 1);
	}
	FreeBuffer((Buffer *)branch);
    }

    /* Free previous tree nodes (if any) */
    if (ROOT_NODE(tree_win) != NULL)
	FreeTree(ROOT_NODE(tree_win), tree_win->nitems);

    CreateIcons(&root, &tree_win->itp);
    tree_win->nitems = root.used;
    DmTouchIconBox((DmWinPtr)tree_win, NULL, 0);
    DmDisplayStatus((DmWinPtr)tree_win);

    if (old_path != NULL)
	FREE(old_path);
}					/* end of BeginTree */

/****************************procedure*header*****************************
    CloseTreeView-
*/
void
CloseTreeViewCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmUnmapWindow((DmWinPtr)TREE_WIN(Desktop));

}					/* end of CloseTreeViewCB */

/****************************procedure*header*****************************
    Compare-
*/
static int
Compare(const void * arg1, const void * arg2)
{
    Node *	node1 = (Node *)arg1;
    Node *	node2 = (Node *)arg2;
    char *	fullpath1 = strdup( NAME(node1) );
    char *      fullpath2 = strdup( NAME(node2) );
    int		result;

    result = path_strcmp(fullpath1, fullpath2);

    FREE(fullpath1);
    FREE(fullpath2);

    return(result);
}

/****************************procedure*header*****************************
    I18NCompare-
*/
static int
I18NCompare(const void * arg1, const void * arg2)
{
    Node *	node1 = (Node *)arg1;
    Node *	node2 = (Node *)arg2;
    char *	fullpath1 = strdup( NAME(node1) );
    char *	fullpath2 = strdup( NAME(node2) );
    int		result;

#ifdef USE_MBLEN_BASED_ALGORITHM
    char *      p;
    int		len;

    p = fullpath1;

    while (*p) {
	if ((len = mblen(p, MB_LEN_MAX)) > 1)
	   p += len;
	else {
	   if (*p == '/')
		*p = '\1';
	   p++;
	}
    }

    p = fullpath2;

    while (*p) {
	if ((len = mblen(p, MB_LEN_MAX)) > 1)
	   p += len;
	else {
	   if (*p == '/')
		*p = '\1';
	   p++;
	}
    }
#endif /* USE_MBLEN_BASED_ALGORITHM */

    result = strcoll(fullpath1, fullpath2);

    FREE(fullpath1);
    FREE(fullpath2);

    return(result);
}

/****************************procedure*header*****************************
    CreateIcons- create corresponding item icons given 'tree'
*/
static void
CreateIcons(Tree * tree, DmItemRec ** items)
{
    DmItemRec *		item;
    Node *		node;
    Position		y;
    Dimension		margin = HORIZ_PAD;
    Dimension		x_inc = ICON_WIDTH + margin;
    Dimension		height = ICON_HEIGHT;
    Dimension		y_inc = Y_DELTA;

#define TXT_WIDTH(I) DmTextWidth(Desktop, (I)->label, strlen(ITEM_LABEL(I)))

    *items = (DmItemRec *)REALLOC((void *)*items, tree->used*sizeof(DmItemRec));

    y		= VERT_PAD;
    item	= *items;
    for (node = BUF_NODE(*tree, 0); node < BUF_NODE(*tree, tree->used); node++)
    {
	item->label		= ROOT_DIR(NAME(node)) ?
					(XtArgVal)NAME(node) :
					(XtArgVal)basename(NAME(node));
	item->x			= (XtArgVal)margin + x_inc * node->level;
	item->y			= (XtArgVal)y;
	item->icon_width	= (XtArgVal)ICON_WIDTH +
					ICON_PADDING + TXT_WIDTH(item);
	item->icon_height	= (XtArgVal)height;
	item->managed		= (XtArgVal)True;
	item->select		= (XtArgVal)False;
	item->busy		= (XtArgVal)False;
	item->client_data	= (XtArgVal)NULL;
	item->object_ptr	= (XtArgVal)node;	
	item++;
	y += y_inc;
    }
}					/* end of CreateIcons */

/****************************procedure*header*****************************
    CreateTree- generate a list of directory tree nodes starting at 'path'.
	Consider the level of this branch to be 'root_level'.  Extend tree
	to up to BUT EXCLUDING 'terminate_level'.  (WalkTree will set the
	level of each branch node according to the level of the "root").

	Note: caller is responsible for freeing tree.
*/
static Tree *
CreateTree(char * path, u_short root_level, u_short terminate_level)
{
    Tree * root = (Tree *)AllocateBuffer(sizeof(Node), 0);

    WalkTree(root, path, root_level, terminate_level);

    if (!BufferEmpty(root)) {
	qsort((void *)root->p, (size_t)root->used,
	      (size_t)sizeof(Node),
	      strcmp(setlocale(LC_COLLATE, NULL), "C") ? I18NCompare : Compare);
    }

    return (root);
}					/* end of CreateTree */

/****************************procedure*header*****************************
    DeleteBranch- delete nodes which represent branch "below" 'indx'.
*/
static void
DeleteBranch(Tree * root, Cardinal indx, Boolean inclusive)
{
    Node *	node;
    u_short	root_level = BUF_NODE(*root, indx)->level;
    Cardinal	num_nodes = root->used;
    Cardinal	start;
    Cardinal	end;

    start = inclusive ? indx : indx + 1;	/* preserve 'indx' */

    for (end = start, node = BUF_NODE(*root, start);
	 end < num_nodes; end++, node++)
    {
	if ((end > indx) && (node->level <= root_level))
	    break;

	FREE( NAME(node) );
	XtFree( node->obj.objectdata );		/* FileInfo */
    }

    /* Ripple down any remaining nodes */
    if (end < num_nodes)
    {
	Node *	src;
	Node *	dst;

	for (src = BUF_NODE(*root, end), dst = BUF_NODE(*root, start);
	     src < BUF_NODE(*root, num_nodes); src++, dst++)
	    *dst = *src;
    }

    end -= start;		/* Compute number of nodes in branch */
    /* Shrink buffer by number of nodes in branch */
    GrowBuf((Buffer *)root, -end);

}					/* end of DeleteBranch */

/****************************procedure*header*****************************s
    DeleteFromTree- common code to remove nodes from tree (from HideFolders
	and UpdateTreeView, for instance).  'inclusive' means delete the
	'indx' node and its branch (if any), otherwise, just delete the
	branch.

	NOTE: if last dir of parent is deleted, must update 'has_subdirs'
*/
static void
DeleteFromTree(Cardinal indx, Boolean inclusive)
{
    Tree	root;

    root.p	= ROOT_NODE(TreeWin);
    root.size	= root.used = TreeWin->nitems;
    root.esize	= sizeof(Node);		/* to be safe but probably not used */

    DeleteBranch(&root, indx, inclusive);
    DeleteIcons(&TreeWin->itp, &TreeWin->nitems,
		inclusive ? indx : indx + 1, TreeWin->nitems - root.used);
    FixLinks(&root, TreeWin->itp);
}					/* end of DeleteFromTree */

/****************************procedure*header*****************************
    DeleteIcons- delete 'count' number of icons starting at 'item_index'.
	Shrink buffer of items and (as a convenience) set the new number of
	items into 'num_items'
*/
static void
DeleteIcons(DmItemRec ** items, Cardinal * num_items, Cardinal item_index,
	    int count)
{
    int move_cnt = *num_items - count - item_index;

    if (move_cnt > 0)		/* are there any to the "right" ? */
    {
	Dimension	y_dec = count * Y_DELTA;
	DmItemRec *	item;

	/* First, adjust their 'y' values */
	for (item = *items + item_index + count;
	     item < *items + *num_items; item++)
	    item->y -= y_dec;


	/* Now move them 'left' */
	(void)memmove(*items + item_index,
		      *items + item_index + count,
		      move_cnt * sizeof(DmItemRec));
    }
    *num_items -= count;		/* dec number of items */

    /* Realloc items array smaller */
    *items = (DmItemRec *)REALLOC((void *)*items, *num_items*sizeof(DmItemRec));

}					/* end of DeleteIcons */

/****************************procedure*header*****************************
    DrawLines-
*/
static void
DrawLines(DmItemPtr * item, DmItemPtr parent, DmItemPtr end)
{
    Widget	ibox = TreeWin->box;
    GC		gc = ((FlatWidget)ibox)->flat.label_gc;
    u_short	child_level = NODE(parent)->level + 1;

    while (((*item) < end) && (NODE(*item)->level >= child_level))
    {
	if (NODE(*item)->level > child_level)
	    DrawLines(item, (*item) - 1, end);

	else		/* draw line from child to parent */
	{
	    XPoint		points[3];
	    Position		x, y;

	    x = ITEM_X(*item);
	    y = ITEM_Y(*item);

	    points[0].x = x;
	    points[1].x = x - HORIZ_PAD - ICON_WIDTH/(unsigned)2;
	    points[2].x = points[1].x;

	    points[0].y = y + ICON_HEIGHT/2;
	    points[1].y = points[0].y;

	    /* Final 'y' value must point to parent:
	       Folder icons are not square and there's no way of knowing
	       where in the pixmap the bottom of the tree_win is so the number
	       below is 'empirical'
	    */
	    points[2].y = y -
		(((*item) - parent) * Y_DELTA) + ICON_HEIGHT - 2;

	    XDrawLines(XtDisplay(ibox), XtWindow(ibox),
		       gc, points, XtNumber(points), CoordModeOrigin);

	    /* Now, if node has subdirs, indicate this with a "dangling" branch
	       if (node->has_subdirs && !node->subdirs_shown)
	       {
		   points[0].x = points[1].x = x + ICON_WIDTH/(unsigned)2;
		   points[0].y = y + ICON_HEIGHT;
		   points[1].y = points[0].y + VERT_PAD/2;

		   XDrawLines(XtDisplay(ibox), XtWindow(ibox),
			      gc, points, 2, CoordModeOrigin);
	       }
	    */

	    (*item)++;

	} /* draw line to parent */

    } /* while descendant */
}					/* end of DrawLines */

/****************************procedure*header*****************************
    ExposureEH- event handler for exposes on FIconBox.  If there are
    sub-nodes,  draw the connecting lines.
*/
static void
ExposureEH(Widget ibox, XtPointer client_data, XEvent* xevent, Boolean* cont)
{
    DmFolderWindow	tree_win = (DmFolderWindow)client_data;
    DmItemPtr		item;
    Node *		node;
    Node *		end;

    if (tree_win->nitems < 2)
	return;

    item = DM_WIN_ITEM(tree_win, 1);		/* 1st descendant */
    DrawLines(&item, tree_win->itp, DM_WIN_ITEM(tree_win, tree_win->nitems));

}					/* end of ExposureEH */

/****************************procedure*header*****************************
    Find- find 'path' in tree.  Since tree is sorted, search can be
	optimized: at each node, 'path' must compare >= node path.
*/
static void
Find(Node * start, Cardinal cnt, char * path, Node ** node, Node ** parent)
{
    char *	dup_path = strdup(path);
    char *	parent_path = dirname(dup_path); 
    Boolean	root_found = False;

    for (*parent = start; *parent < (start + cnt); (*parent)++)
    {
	char *	node_path = PATH(*parent);
	int	result = path_strcmp(parent_path, node_path);

	if (result == 0)
	{
	    /* Tree no longer in true sorted order so search not OPTIMIZED */
	    root_found = True;
	    break;
	}
    }
    if (!root_found)
	*parent = NULL;

    FREE(dup_path);

    /* Assuming valid dir path as input, loop would not fall thru;
       comparison would fail first.
    */
    if ((*parent != NULL) && (*parent)->subdirs_shown)
    {
	/* 'parent' is set.  Now get 'node' */
	for (*node = (*parent) + 1; *node < (start + cnt); (*node)++)
	{
	    int result = path_strcmp(PATH(*node), path);
	    if (result > 0)
		break;
	    if (result == 0)
		return;
	}
    }

    /* Assuming valid dir path as input, loop would not fall thru;
       comparison would fail first.
    */
    *node = NULL;
}					/* end of Find */

/****************************procedure*header*****************************
    FixLinks- FIconBox items must point to "object's".  The "object" data is
	stored in the tree nodes.  The items point to the tree nodes
	initially when the tree (and items) are built from scratch.  When the
	tree nodes change (as from Hide/Show Folders), however, the
	pointers in the items must be fixed up.

*/
static void
FixLinks(Tree * root, DmItemRec * items)
{
    DmItemRec *	item = items;
    Node *	node = BUF_NODE(*root, 0);
    Cardinal	i;

    for (i = root->used; i != 0; i--)
	item++->object_ptr = (XtArgVal)node++;

}					/* end of FixLinks */

/****************************procedure*header*****************************
    FreeTree-
*/
static void
FreeTree(Node * root_node, int cnt)
{
    Node * node;

    for (node = root_node; cnt != 0; cnt--, node++)
	if (NAME(node) != NULL)
	    FREE(NAME(node));

    if (root_node != NULL)
	FREE((void *)root_node);
}					/* end of FreeTree */

/****************************procedure*header*****************************
    HideFolders- called to hide (sub)folders when user selects "Hide" from
	View menu or Icon menu.
*/
static char *
HideFolders(DmFolderWindow tree_win, Cardinal item_index)
{
    Node *	node = ITEM_NODE(tree_win->itp, item_index);

    /* Point to node and see if no subdirs or subdirs already not shown. */
    if (!node->has_subdirs || !node->subdirs_shown)
	return( !node->has_subdirs ? TXT_NO_SUBDIRS : TXT_ALREADY_HIDDEN );

    node->subdirs_shown = False;
    DeleteFromTree(item_index, False);
    return(NULL);
}					/* end of HideFolders */

/****************************procedure*header*****************************
    InsertIcons-
*/
static void
InsertIcons(DmItemRec ** base, Cardinal * num_items, Cardinal item_index,
	    DmItemRec * items, int count)
{
    Buffer	src;
    Buffer	dst;
    DmItemRec *	item;
    Cardinal	i;
    Position	y;
    Dimension	y_inc = Y_DELTA;

    src.size	= src.used = count;
    src.esize	= sizeof(DmItemRec);
    src.p	= (BufferElement *)items;

    dst.size	= dst.used = *num_items;
    dst.esize	= sizeof(DmItemRec);
    dst.p	= (BufferElement *)*base;

    InsertBuffer(&dst, &src, item_index + 1);

    *base	= (DmItemRec *)dst.p;	/* Return new icon array */
    *num_items	= dst.used;		/* A convenience */

    /* Now adjust 'y' in new items and those below */
    for (i = 0, item = *base + item_index + 1,
	 y = (*base)[item_index].y + y_inc;
	 i < dst.used - (item_index + 1);
	 i++, item++, y += y_inc)
	item->y = y;

}					/* end of InsertIcons */

/****************************procedure*header*****************************
    ParentNode-
*/
static Node *
ParentNode(Node * node)
{
    Node * parent = node;

    while ((parent->level > 0) && (parent->level >= node->level))
	   parent--;

    return(parent);
}					/* end of ParentNode */

/****************************procedure*header*****************************
    ShowAllFolders- called to show all (sub)folders when user selects "Show
	All Levels" from View menu or Icon menu.
*/
static char * 
ShowAllFolders(DmFolderWindow tree_win, Cardinal item_index)
{
    Node *	node;
    DmItemPtr	item;
    Cardinal	bottom_index;
    u_short	level;
    Boolean	found;

    node = ITEM_NODE(tree_win->itp, item_index);	/* Point to this node */

    /* Nothing to do if no subdirs to show or node is a symbolic link */
    if (!node->has_subdirs || IS_SYMLINK(node))
	return( !node->has_subdirs ? TXT_NO_SUBDIRS : TXT_DIR_IS_SYMLINK );

    /* This could take awhile so entertain the user */
    BUSY_CURSOR(tree_win->box);
	    
    /* Show subdirs starting at this node and return */
    if (!node->subdirs_shown)
	return( ShowFolders(tree_win, item_index, 0) );

    /* Getting here means node is already showing some subdirs.  Must find
       "leaf" nodes and begin showing from there.  Note: we know it has
       subdirs.  Start at the "bottom" and work up so we don't iterate over
       new branches added (branches are inserted AFTER index).
    */
    level = node->level;
    item = DM_WIN_ITEM((DmFolderWindow)tree_win, item_index + 1);

    /* Break when non-descendant found or end of items */
    while ((item < DM_WIN_ITEM((DmFolderWindow)tree_win, tree_win->nitems)) &&
	   (((Node *)ITEM_OBJ(item))->level > level))
    {
	item++;
    }

    /* We now have bracketed the descendants of item_index.
       CAUTION: must use index 'bottom_index' here rather than item ptr
       since 'itp' & 'nitems' may change with each call.
    */
    found = False;
    for (bottom_index = item - tree_win->itp - 1;	/* ie, pre-dec */
	 bottom_index != item_index; bottom_index--)
     {
	node = ITEM_NODE(tree_win->itp, bottom_index);

	/* Expand non-symlink nodes with subdirs not shown */
	if (node->has_subdirs && !node->subdirs_shown && !IS_SYMLINK(node))
	{
	    (void)ShowFolders(tree_win, bottom_index, 0);
	    found = True;
	}
    }
    return( found ? NULL : TXT_ALL_SHOWN );

}					/* end of ShowAllFolders */

/****************************procedure*header*****************************
    ShowFolders- called to show (sub)folders when user selects "Show" from
	View menu or Icon menu.
*/
static char * 
ShowFolders(DmFolderWindow tree_win, Cardinal item_index, u_short depth)
{
    Tree *	branch;
    Node *	node;
    char *	path;

    /* First, point to node and see if sub folders are already shown. */
    node = ITEM_NODE(tree_win->itp, item_index);

    /* Nothing to do if no subdirs to show or they're already shown
       or node is a symbolic link
    */
    if (!node->has_subdirs || node->subdirs_shown || IS_SYMLINK(node))
	return(!node->has_subdirs ? TXT_NO_SUBDIRS :
	       node->subdirs_shown ? TXT_ALREADY_SHOWN : TXT_DIR_IS_SYMLINK );

    node->subdirs_shown = True;

    /* Create (sub)tree and insert it */
    path = strdup( PATH(node) );		/* since Dm__buffer used */
    branch = CreateTree(path, node->level + 1,
			(depth == 0) ? 0 : node->level + 1 + depth - 1);
    FREE((void *)path);

    /* Now add branch, make and insert icons, etc. */
    AddToTree(branch, item_index);
    FreeBuffer((Buffer *)branch);

    return(NULL);
}				/* end of ShowFolders */

/****************************procedure*header*****************************
    ViewMenuCB- callback for all buttons in View menu.
*/
static void 
ViewMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	tree_win = TREE_WIN(Desktop);
    OlFlatCallData *	d = (OlFlatCallData *)call_data;
    DmTreeOption	option = (DmTreeOption)d->item_index;
    Cardinal		i;
    Boolean		touch_items;
    FileGizmo *         prompt;

    DmClearStatus((DmWinPtr)tree_win);		/* Clear footer first */

    switch (option)
    {
    case DM_SHOW_SUBS:
    case DM_HIDE_SUBS:
    case DM_SHOW_ALL_SUBS:
	touch_items = False;

	/* CAUTION: must use index 'i' here rather than item ptr since
	   'itp' & 'nitems' may change with each call.
	*/ 
	for (i = 0; i < tree_win->nitems; i++)
	    if (ITEM_MANAGED(tree_win->itp + i) && ITEM_SELECT(tree_win->itp + i))
		touch_items |=
		    ((option == DM_SHOW_SUBS) ? ShowFolders(tree_win, i, 1) :
		     (option == DM_HIDE_SUBS) ? HideFolders(tree_win, i) :
		     ShowAllFolders(tree_win, i)) == NULL;

    if (touch_items)
    {
	DmTouchIconBox((DmWinPtr)tree_win, NULL, 0);
    	DmDisplayStatus((DmWinPtr)tree_win);
    }
	break;

    case DM_BEGIN_MAIN:
	/* If already rooted at Main, issue warning and break now */
	if (path_strcmp(DmMakePath(TreePath(tree_win), NAME(ROOT_NODE(tree_win))),
		   DESKTOP_HOME(Desktop)) == 0)
	{
	    DmVaDisplayStatus((DmWinPtr)tree_win, False, TXT_ALREADY_MAIN);
	    break;
	}

	BeginTree(tree_win->box, DESKTOP_HOME(Desktop));
	break;

    case DM_BEGIN_OTHER:
	/* Create prompt if necessary and map it.
	   Note: We use the tree_win_prompt field of the tree_win structure given 
	   that the Folder Map does not have a Folder Menu.
	*/
	if ( (prompt = tree_win->folder_prompt) == NULL )
	{
	    prompt = tree_win->folder_prompt =
		CopyGizmo(FileGizmoClass, &BeginOtherPrompt);
	    prompt->directory = strdup(PATH(ROOT_NODE(tree_win)));
	    CreateGizmo(tree_win->shell, FileGizmoClass, prompt, NULL, 0);
	    SetFileGizmoMessage(prompt, TXT_ENTER_PATH_MSG);
	}
	MapGizmo(FileGizmoClass, prompt);
	break;

    case DM_BEGIN_HERE:
	/* Get index of selected item and begin tree there (or from parent) */
	XtVaGetValues(tree_win->box, XtNlastSelectItem, &i, NULL);

	/* Safety check for now.  Later, button will be insensitive */
	if (i == OL_NO_ITEM)
	    break;

	BeginHere(tree_win, tree_win->itp + i);
	break;

    case DM_BEGIN_PARENT:
	BeginParent(tree_win);
	break;

    default:
	break;
    }
}					/* end of ViewMenuCB */

/****************************procedure*header*****************************
    WalkTree- this routine is called recursively to traverse a directory
	tree rooted at 'path'.  The "root" level is 'root_level' and
	recursion should continue until a level of 'terminate_level' is
	reached (except when 'terminate_level' is '0' which means recurse
	until all leaf nodes are found; ie., "infinite depth").  The head of
	the tree list, 'head', is realloc'ed for each new node added to the
	list at each subdir entry.
*/
static void
WalkTree(Tree * root, char * path, u_short root_level, u_short terminate_level)
{
    DIR *		dirp;
    struct dirent *	dir_ent;
    Node *		node;
    int			root_len;
    u_short		current_level;
    Cardinal		parent_indx;

    if ( (dirp = opendir(path)) == NULL)
    {
	Dm__VaPrintMsg(TXT_OPENDIR, errno, path);
	return;
    }

    /* An empty buffer indicates the initial call */
    if (BufferEmpty(root))
    {
	parent_indx	= OL_NO_ITEM;
	current_level	= root_level;

    } else
    {
	parent_indx	= root->used - 1;
	current_level	= BUF_NODE(*root, parent_indx)->level + 1;
    }

    /* Compute length of container path.  This is constant for the duration
       of building a tree so it's unfortunate that it's being re-computed.
       (If tree rooted at "/", ignore it)
    */
    if ( (root_len = strlen(TreePath(TreeWin))) == 1 )
	root_len = 0;

    while ( (dir_ent = readdir(dirp)) != NULL )
    {
	struct stat	stat_buf;
	char *		fullpath;

	if (dir_ent->d_name[0] == '.')
	    continue;

	fullpath = (char *)MALLOC((size_t)strlen(path) +
				  strlen(dir_ent->d_name) +
				  1 +			/* '/' */
				  1);			/* '\0' */

	(void)strcpy(fullpath, path);
	if (!ROOT_DIR(path))
	    (void)strcat(fullpath, "/");
	(void)strcat(fullpath, dir_ent->d_name);

	/* (void)printf("%s\n", fullpath); */

	if ( stat(fullpath, &stat_buf) == -1)
	{
	    Dm__VaPrintMsg(TXT_STAT, errno, fullpath);
	    FREE((void *)fullpath);
	    continue;
	}

	/* Ignore non-directories */
	if ( !S_ISDIR(stat_buf.st_mode) )
	{
	    FREE((void *)fullpath);
	    continue;
	}

	/* A subdir was found... indicate this to the parent (if any) */
	if (parent_indx != OL_NO_ITEM)
	{
	    /* CAUTION: parent_node is only used locally here.  It is made
	       invalid when buffer is realloced, for instance.
	    */
	    Node * parent_node = BUF_NODE(*root, parent_indx);

	    if ((parent_node != NULL) && !parent_node->has_subdirs)
	    {
		parent_node->has_subdirs = True;

		/* There are 2 conditions for breaking after finding the
		   first subdir:
		     -	the parent node is a symbolic link to a dir.  We don't
			continue so as to avoid cycles but we do go one level
			of recursion farther to indicate whether the dir the
			link points to has subdirs (and we've now found one).
		     -	we've reached the 'terminate_level' (except when
			'terminate_level is '0') and have gone one level of
			recursion farther to indicate whether the "leaf"
			parent has any subdirs (and we've now found one).
		*/
		if (IS_SYMLINK(parent_node) || 
		    ((current_level > terminate_level) &&
		     (terminate_level != 0)))
		{
		    FREE((void *)fullpath);
		    break;
		}
		/* else */
		parent_node->subdirs_shown = True;
	    }
	}

	GrowBuf((Buffer *)root, 1);	/* Alloc node for this dir entry */

	/* CAUTION: parent_node is made invalid after GrowBuf */

	node = BUF_NODE(*root, root->used - 1);	/* point to new node */

	/* Make node name, set path and level. */
	INIT_NODE(node, MakeNodeName(fullpath, root_len),
		  current_level, &stat_buf);
		  
	WalkTree(root, fullpath, root_level, terminate_level);
	FREE((void *)fullpath);
    }

    if (closedir(dirp) == -1)
	Dm__VaPrintMsg(TXT_CLOSEDIR, errno);

    return;
}					/* end of WalkTree */

/****************************procedure*header*****************************
    WMCB-
*/
static void
WMCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlWMProtocolVerify *wm_data = (OlWMProtocolVerify *)call_data;

    if (wm_data->msgtype == OL_WM_DELETE_WINDOW) {
	DmBringDownIconMenu((DmWinPtr)TREE_WIN(Desktop));
	DmUnmapWindow((DmWinPtr)TREE_WIN(Desktop));
    }

}					/* end of WMCB */

/***************************private*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    DmDrawTreeIcon- currently just draws same icon as used in "Name" view.
*/
void
DmDrawTreeIcon(Widget widget, XtPointer client_data, XtPointer call_data)
{
    DmDrawNameIcon(widget, (XtPointer)NULL, call_data);
}

/****************************procedure*header*****************************
    DmFolderOpenTreeCB- callback when user presses button to open tree view.
*/
void
DmFolderOpenTreeCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	window;
    DmFolderWindow	originating_folder;

    if ( (window = TREE_WIN(Desktop)) != NULL )
    {
	DmMapWindow((DmWinPtr)window);
	return;
    }

    /* This could take awhile so entertain the user.  If this is a request
       from the icon in the destiny window, widget 'w' will be NULL so use
       the "top" window in that case.  Otherwise, request is from menu so get
       the originating folder.  Note: this is all done since window must be
       VIEWABLE.
    */
    originating_folder = (w == NULL) ?
	DESKTOP_TOP_FOLDER(Desktop) : (DmFolderWindow)DmGetWinPtr(NULL);
    BUSY_CURSOR(originating_folder->box);

    /* Initialize the Folder Window Structure for the  Folder Map.  First
       allocate the Folder structure and next allocate a Container Pointer
       structure.
    */
    window = TREE_WIN(Desktop) = 
	(DmFolderWinPtr)CALLOC(1, sizeof(DmFolderWinRec));

    window->cp = (DmContainerPtr)CALLOC(1, sizeof(DmContainerRec));
    if (window->cp == NULL)
    {
	Dm__VaPrintMsg(TXT_MEM_ERR);
	return;
    }

    /* Initialize Folder Structure. */
    window->cp->path	= strdup(DESKTOP_HOME(Desktop));
    window->cp->count	= 1;
    window->cp->num_objs= 0;
    window->attrs	= DM_B_FOLDER_WIN | DM_B_TREE_WIN;
    window->view_type	= DM_NAME;		/* sort of */
    window->sort_type	= DM_BY_NAME;
    window->gizmo_shell	= &FMapWindow;

    /* Set this window as the client_data for sub-menus */
    FMapMenuBar.client_data = (char *)window;

    /* Format the "Main" button with user's home login */
    sprintf(Dm__buffer, Dm__gettxt(TXT_TREE_MAIN), DESKTOP_HOME(Desktop));
    FMapViewMenuItems[DM_BEGIN_MAIN].label = strdup(Dm__buffer);

    /* Create The BaseWindow Gizmo that Houses the Folder Map
     * There is only one tree view - hence no need to copy gizmo
     */
    window->shell = CreateGizmo(NULL, BaseWindowGizmoClass, 
				&FMapWindow, NULL, 0);
    window->swin  = (Widget)QueryGizmo(BaseWindowGizmoClass, 
				       &FMapWindow, GetGizmoWidget, "swin");

    XtSetArg(Dm__arg[0], XtNdrawProc,		DmDrawTreeIcon);
    XtSetArg(Dm__arg[1], XtNmovableIcons,	False);
    XtSetArg(Dm__arg[2], XtNdropProc,		Dm__FolderDropProc);
    XtSetArg(Dm__arg[3], XtNdblSelectProc,	Dm__FolderSelect2Proc);
    XtSetArg(Dm__arg[4], XtNclientData,		window);
    XtSetArg(Dm__arg[5], XtNmenuProc,		DmIconMenuProc);
    XtSetArg(Dm__arg[6], XtNpostSelectProc,	DmButtonSelectProc);
    XtSetArg(Dm__arg[7], XtNpostAdjustProc,	DmButtonSelectProc);
    XtSetArg(Dm__arg[8], XtNtriggerMsgProc,     DmFolderTriggerNotify);

    window->nitems = 5;
    window->box = DmCreateIconContainer(window->swin, 0,
					Dm__arg, 9,
					window->cp->op, window->cp->num_objs,
					&(window->itp), window->nitems,
					NULL,
					DESKTOP_FONTLIST(Desktop),
					DESKTOP_FONT(Desktop),
					DM_FontHeight(Desktop));

    /* make the iconbox the initial focus widget */
    XtSetArg(Dm__arg[0], XtNfocusWidget, window->box);
    XtSetValues(window->shell, Dm__arg, 1);
    OlAddCallback(window->shell, XtNwmProtocol, WMCB, NULL);

    /* Bring up initial view of tree */
    fmodeKey = DmFtypeToFmodeKey(DM_FTYPE_DIR);	/* set (fixed) FmodeKey */
    DmInitSmallIcons(DESKTOP_SHELL(Desktop));
    XtAddEventHandler(window->box,		/* for drawing lines */
		      ExposureMask, False, ExposureEH, (XtPointer)window);

    BeginTree(originating_folder->box, TreePath(window) );

    XtRealizeWidget(window->shell);	/* Realize and Map the window */
    DmMapWindow((DmWinPtr)window);

    /* Set the granularity of the scrollbar(s) */
    /* NOTE: taken from DmFormatView.  Should be shared */
    XtSetArg(Dm__arg[0], XtNhStepSize, GRID_WIDTH(Desktop) / 2);
    XtSetArg(Dm__arg[1], XtNvStepSize, DM_NameRowHeight(Desktop));
    XtSetValues(window->swin, Dm__arg, 2);
	
    /* put something in the status area */
    DmDisplayStatus((DmWinPtr)window);

	/* register for help */
	FMAP_HELP_ID(Desktop) = DmNewHelpAppID(XtScreen(window->shell),
						XtWindow(window->shell),
						Dm__gettxt(TXT_DESKTOP_MGR),
						Dm__gettxt(TXT_FOLDERMAP),
						DESKTOP_NODE_NAME(Desktop),
						NULL, "fmap.icon")->app_id;

}					/* end of DmFolderOpenTreeCB */

/****************************procedure*header*****************************
    TreeIconMenuCB- called for all buttons in Icon menu.
*/
void
TreeIconMenuCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData *	d = (OlFlatCallData *)call_data;
    /* CAUTION: '3' used here to account for Open, Properties, & Delete */
    DmTreeOption	option = (DmTreeOption)(d->item_index - 3);
    DmItemPtr		item = (DmItemPtr)client_data;
    DmFolderWindow	tree_win = TREE_WIN(Desktop);
    char *		err_msg;
    Cardinal		i;

    DmClearStatus((DmWinPtr)tree_win);		/* Clear footer first */

    switch(option)
    {
    case DM_SHOW_SUBS:
    case DM_HIDE_SUBS:
    case DM_SHOW_ALL_SUBS:
	i = (Cardinal)(item - tree_win->itp);
	err_msg =
	    (option == DM_SHOW_SUBS) ? ShowFolders(tree_win, i, 1) :
	    (option == DM_HIDE_SUBS) ? HideFolders(tree_win, i) :
		ShowAllFolders(tree_win, i);

	if (err_msg == NULL ) 
	{
	    DmTouchIconBox((DmWinPtr)tree_win, NULL, 0);
    	    DmDisplayStatus((DmWinPtr)tree_win);
	}
	else
	    DmVaDisplayStatus((DmWinPtr)tree_win, True, err_msg);
	break;

    case DM_BEGIN_HERE:
	BeginHere(tree_win, item);
	break;

    case DM_BEGIN_PARENT:
    case DM_BEGIN_OTHER:
    case DM_BEGIN_MAIN:
    default:
	break;
    }
}					/* end of TreeIconMenuCB */

/****************************procedure*header*****************************
    Dm__UpdateTreeView- add, delete or modify node in tree view based on
	non-NULL old_path & new_path:

	old_path == NULL  :	add node for 'new_path'
	new_path == NULL  :	delete node with 'old_path'
	else			modify node with 'old_path' to 'new_path'

	For deletions and modifications, 'old_path' may not be found in tree
	since caller may not know if they currently appear in the tree or if
	the path represents a dir.  If found, descendant paths must also be
	processed.
*/
void
Dm__UpdateTreeView(char * old_path, char * new_path)
{
    Node *	node;
    Node *	parent_node;

    if (TreeWin == NULL)	/* return now if tree is not up */
	return;

    if (old_path == NULL)			/* ie. ADDITION */
    {
	/* Get ptr to node and parent node */
	Find(ROOT_NODE(TreeWin), TreeWin->nitems, new_path, &node, &parent_node);

	if ((parent_node == NULL) ||		/* does not contain parent */
						/* branch not shown: */
	    (parent_node->has_subdirs && !parent_node->subdirs_shown) ||
	    (node != NULL))			/* ignore duplicates */
	    return;

	AddNode(new_path, parent_node, NULL);

    } else if (new_path == NULL)		/* ie. DELETION */
    {
	int	old_len = strlen(old_path);

	/* For deletes, check first to see if the "root" path of tree is
	   same or descendant of path being deleted ('old_path').

	   If 'old_path' matches tree root or the root node, tree must be
	   re-rooted.  The best guess is to re-root it to Main (rather than
	   re-rooting it to the parent of the dir being deleted, for example).
	   If we must re-root but Main is also same or descendant of
	   'old_path', what then??  Do nothing: system is probably in bad
	   state and everything will come down soon anyway.
	*/
	if (DmSameOrDescendant(old_path, PATH(ROOT_NODE(TreeWin)), old_len))
	{
	    if (!DmSameOrDescendant(old_path, DESKTOP_HOME(Desktop), old_len))
		BeginTree(NULL, DESKTOP_HOME(Desktop));	/* don't busy cursor */

	    return;
	}

	/* Get ptr to node and parent node */
	Find(ROOT_NODE(TreeWin), TreeWin->nitems, old_path, &node, &parent_node);

	if ((parent_node == NULL) ||		/* does not contain parent */
	    (node == NULL))			/* no node for this path */
	    return;

	DeleteFromTree(node - ROOT_NODE(TreeWin), True);

    } else					/* ie. MODIFICATION */
    {
	int	old_len = strlen(old_path);
	Node *	new_node;
	Node *	parent_new_node;

	/* For mods, check first to see if the "root" path of tree is same
	   or descendant of path being modified ('old_path').

	   Must check for affects of by both 'old_path' & 'new_path':
				parent of
		'old_path'	'new_path'
		in tree		not in tree	delete 'old_path' and branch
		not in tree	in tree		add new node for 'new_path'
		in tree		in tree		delete old, add new
		not in tree	not in tree	do nothing
	*/
	if (DmSameOrDescendant(old_path, TreePath(TreeWin), old_len))
	{
	    char * save = TreePath(TreeWin);

	    TreePath(TreeWin) =
		strdup((save[old_len] == '\0') ? new_path :  /* exact match */
		       DmMakePath(new_path, save + old_len + 1));
	    FREE((void *)save);
	    return;
	}

	/* Get ptr to nodes and parent nodes */
	Find(ROOT_NODE(TreeWin), TreeWin->nitems, old_path, &node, &parent_node);
	Find(ROOT_NODE(TreeWin), TreeWin->nitems,
	     new_path, &new_node, &parent_new_node);

	if ((node == NULL) && (parent_new_node == NULL))
	    return;

	if ((parent_new_node != NULL) && parent_new_node->subdirs_shown &&
	    (new_node == NULL))		/* Should be */
	{				/* 'new_path' will be in tree */
	    /* Insert new node using data from 'node' (if any) */
	    AddNode(new_path, parent_new_node, node);

	    /* NOTE: for now, must re-find 'old_path' in tree since
	       AddNode may have just realloc'ed the tree.
	    */
	    if (node != NULL)
		Find(ROOT_NODE(TreeWin), TreeWin->nitems,
		     old_path, &node, &parent_node);
	}

	if (node != NULL)		/* 'old_path' in tree */
	    DeleteFromTree(node - ROOT_NODE(TreeWin), True);
    }

    /* Getting here means tree was 'touched' */
    DmTouchIconBox((DmWinPtr)TreeWin, NULL, 0);
    DmDisplayStatus((DmWinPtr)TreeWin);
}					/* end of Dm__UpdateTreeView */

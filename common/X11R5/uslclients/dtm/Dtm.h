/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef __Dtm_h_
#define __Dtm_h_

#pragma ident	"@(#)dtm:Dtm.h	1.169"

/******************************file*header********************************

    Description:
	This file is the header for the desktop manager.

    Organization:
	I.	INCLUDES
	II.	TYPEDEF's, STRUCT's and ENUM's
	III.	#define's for BIT-FIELDS/OPTIONS
	IV.	#define's for INTEGER VALUES
	V.	#define's for STRINGS
	VI.	MACROS
*/

#include <stdio.h>

#include <DtI.h>
#include <dtutil.h>
#include <help.h>

/******************************************************************************
	II.  TYPDEF's, STRUCT's and ENUM's
*/

typedef int (*PFI)();

/* Enumeration types for File choices.  Note: order is important since
   used in table-driven code.
*/
typedef enum {
    DM_NO_OP,
    DM_COPY,
    DM_MOVE,
    DM_DELETE,
    DM_HARDLINK,
    DM_SYMLINK,
    DM_COPY_DEL,
    DM_BEGIN,
    DM_MKDIR,
    DM_RMDIR,
    DM_OVERWRITE,
    DM_ENDOVER,
    DM_RENAME		/* pseudo-op which gets re-mapped to DM_MOVE */
} DmFileOpType;

/* Enumeration types for View choices.  Note that the order of the constants
   here must match the order of the buttons in the View menu and submenus.  See
   dtmsgstrs for label definitions and the appropriate "create" source file for
   the order of the callbacks.
*/
/* View Sort menu items */
typedef enum {
    DM_BY_TYPE,
    DM_BY_NAME,
    DM_BY_SIZE,
    DM_BY_TIME,
    DM_BY_POSITION	/* (the default)  Not strictly a menu item. */
} DmViewSortType;

/* Enumeration types for Customized View */
typedef enum {
    DM_SHOW_CV,
    DM_RESET_CV,
    DM_CANCEL_CV,
    DM_HELP_CV,
} DmCustomView;


/* View Format menu items */
typedef enum {
    DM_ICONIC,
    DM_NAME,
    DM_LONG
} DmViewFormatType;

/* Tree options menu items */
typedef enum {
    DM_SHOW_SUBS,
    DM_HIDE_SUBS,
    DM_SHOW_ALL_SUBS,
    DM_BEGIN_HERE,
    DM_BEGIN_MAIN,
    DM_BEGIN_OTHER,
    DM_BEGIN_PARENT
} DmTreeOption;

/* defines used in FileGizmo Prompts */
typedef enum {
    FilePromptTask,
    FilePromptCancel,
    FilePromptHelp
} FileChoiceMenuItemIndex;

/* Types of client_data for MenuItems.

   None of the following values can be zero, because, if zero, then the
   gizmo code will copy the container's client_data into the item.
   NOTE: The constants must be defined in this order. See the code.
*/
typedef enum {
    DM_B_ANY = 1,
    DM_B_ONE_OR_MORE,
    DM_B_ONE,
    DM_B_UNDO,
    DM_B_TREE_OPTS,
    DM_B_NON_TREE_VIEW
} DmMenuItemCDType;

/* Structures used in New window for Template list */

/*
 * This structure is used as nodes in a linked list of templates
 * of a file class.
 */
typedef struct {
     XtPointer name;     /* name of template file */
     XtPointer next;
} ListNodeRec, *ListNodePtr;

/*
 * This structures is used to create a list of items for the Template
 * flat list widget.
 */
typedef struct {
     XtArgVal set;
     XtArgVal formatData;
} ListTokenRec, *ListTokenPtr;


/* The first few (3) fields in DmFmodeKeyRec and DmFnameKeyRec must be
   the same.

   An array of DmFmodeKeyRec structures contain the information about
   internal file classes. The DTM uses this information to determine the
   file class of a file.
*/
typedef struct dmFmodeKeyRec {
    const char *name;			/* name of class */
    DtAttrs	attrs;
    DmFclassPtr	fcp;			/* ptr to actual entry */
					/* End of position-dependent fields */
    mode_t	fmt;			/* FMT part of mode from stat() */
    mode_t	perm;			/* PERM part of mode from stat() */
    dev_t	rdev;			/* rdev from stat() */
    DmFileType	ftype;			/* DTM file type */
    DmGlyphPtr	small_icon;		/* for folders and exececutables
					   in "Tree" and "Name" format */
					/* 'short' fields go next */
					/* 'byte' fields go last */
} DmFmodeKeyRec, *DmFmodeKeyPtr;

/* The list of file classes read from the file database is stored in a 
   list of DmFnameKeyRec structure. Each structure stores information 
   that DTM uses to qualify files that belong to the file class.
*/
typedef struct dmFnameKeyRec *DmFnameKeyPtr;
typedef struct dmFnameKeyRec {
    char *		name;		/* class name */
    DtAttrs		attrs;
    DmFclassPtr		fcp;		/* ptr to actual entry */
    unsigned short	level;		/* file level */
    DmFnameKeyPtr	next;		/* ptr to the next entry */
    DmFnameKeyPtr	prev;		/* ptr to the prev entry */
					/* End of position-dependent fields */
    char *		re;		/* compiled regular expression */
    char *		lre;		/* compiled link regular expression */
    DmFileType		ftype;		/* file type */
					/* 'short' fields go next */
					/* 'byte' fields go last */
} DmFnameKeyRec;

/* DmFclassFileRec structure contains info about each class file.
   Note:This structure is in the same link list as the DmFnameKeyRec
	structure. Thus, the first 6 fields must be the same. Check the
	DM_B_CLASSFILE bit in attrs to distinguish the two structures.
*/
typedef struct dmFclassFileRec *DmFclassFilePtr;
typedef struct dmFclassFileRec {
    char *		name;		/* user specified file name */
    DtAttrs		attrs;		/* encoded info */
    DmFclassFilePtr	next_file;	/* ptr to the next file */
    unsigned short	level;		/* file level */
    DmFnameKeyPtr	next;		/* ptr to the next entry */
    DmFnameKeyPtr	prev;		/* ptr to the prev entry */
					/* End of position-dependent fields */
    char *		fullpath;	/* fullpath of file */
    time_t		last_read_time;
					/* 'short' fields go next */
					/* 'byte' fields go last */
} DmFclassFileRec;

/*			*** FILE OPERATIONS ***				*/

/* reason to invoke client proc during file manipulation operation */
typedef enum {
    DM_INIT_VAL = 1,
    DM_DONE,
    DM_OPRBEGIN,
    DM_ERROR,
    DM_REPORT_PROGRESS,
    DM_OVRWRITE
} DmProcReason;

/* file classification used by file manipulation routines */
#define DM_NO_FILE	0
#define DM_IS_DIR	1
#define	DM_IS_FILE	2
#define	DM_IS_SPECIAL	3
#define DM_IS_SYMLINK	4

/* Bits used in source info in DmFileOpCDRec.
   (The LSBs are used to store source type)
*/
#define SRC_B_SKIP_OVERWRITE	( 1 << 8 )
#define SRC_B_ERROR		( 1 << 9 )
#define SRC_B_IGNORE		( SRC_B_SKIP_OVERWRITE | SRC_B_ERROR )
#define SRC_TYPE_MASK		( SRC_B_SKIP_OVERWRITE - 1 )

/* 'attrs' bits for file operation.  (OVERWRITE is also used as 'option') */
#define DM_B_DIR_EXISTS		( OVERWRITE << 1 )
#define DM_B_DIR_NEEDED_4FILES	( DM_B_DIR_EXISTS << 1 )
#define DM_B_TARGET_ADJUSTED	( DM_B_DIR_NEEDED_4FILES << 1 )
#define DM_B_FILE_OP_STOPPED	( DM_B_TARGET_ADJUSTED << 1 )

typedef void (*DmClientProc)(DmProcReason reason, XtPointer client_data,
			     XtPointer call_data, char * str1, char * str2);

/* The FileOpInfo structure contains "Input" and "output" parameters
   describing the file operation.  Fields marked with '@' represent input to
   DmDoFileOp.
*/
typedef struct dmFileOpInfoRec {
    DmFileOpType	type;		/* @ file operation type */
    DtAttrs		attrs;		/*   target "type" & internal flags */
    DtAttrs		options;	/* @ REPORT_PROGRESS, OVERWRITE, etc */
    char *		target_path;	/* @ target of the operation */
    char *		src_path;	/* @ current directory */
    DtAttrs *		src_info;	/*   info about each source item */
    char **		src_list;	/* @ list of files to operate on */
    int			src_cnt;	/* @ size of src_list (& src_info) */
    struct dmFolderWinRec * src_win;	/* @ source win of file op (if any) */
    struct dmFolderWinRec * dst_win;	/* @ dest win of file op (if any) */
    int			cur_src;	/*   source item being processed */
    int			task_cnt;	/*   # of subtasks completed */
    int			error;		/*   error (if any) */
					/*   'short' fields next */
    Position		x;		/* @ x position of drop */
    Position		y;		/* @ y position of drop */
					/*   'byte' fields last */
} DmFileOpInfoRec, *DmFileOpInfoPtr;

/* The TaskList structure stores information about a sub-task to be 
   executed by a background work proc.  A series of sub-tasks may be
   needed to perform a file opr.   The TaskInfo structure (below) represents
   the file opr and contains the head of the list of sub-tasks.
*/
typedef struct dmTaskListRec {
    char *		source;		/* source file name for sub-task */
    char *		target;		/* destination for sub-task */
    DmFileOpType	type;		/* operation type */
    struct dmTaskListRec * next;	/* next task in the list */
} DmTaskListRec, *DmTaskListPtr;

/* The DmTaskInfo structure stores information about an operation
   initiated from a folder window, WB etc.
*/
typedef struct dmTaskInfoListRec {
    struct dmTaskInfoListRec * next;
    struct dmTaskInfoListRec * prev;
    DmTaskListPtr	task_list;
    DmTaskListPtr	cur_task;
    int			rfd;		/* read fd for incomplete copy opr. */
    int			wfd;		/* write fd for incomplete copy opr. */
    DmClientProc	client_proc;
    XtPointer		client_data;
    DmFileOpInfoPtr	opr_info;
					/* 'short' fields go next */
					/* 'byte' fields last */
} DmTaskInfoListRec, *DmTaskInfoListPtr;

/*			*** END OF FILE OPERATIONS ***			*/

/* This structure stores various options for filtering a view. */
typedef struct keylist {
    void *		key;
    struct keylist *	next;
} KeyList;

typedef struct dmFVFilter {
    char *	pattern;	/* filename pattern */
    DtAttrs	attrs;		/* view flags: DM_SHOW_DOT_FILES */
    Boolean     type[8]; 	/* Array of booleans of for 6 basic sys types */
} DmFVFilter, *DmFVFilterPtr;

/*	File Property Sheet-related structures			*/

/* Information about textfields in the File Property sheet */
typedef struct {
	Widget w;		/* textfield widget */
	char *prev;		/* previous string to restore */
} DmFPropTFRec, *DmFPropTFPtr;

/* Information about permissions in the File Property sheet */
typedef struct {
	Widget 		w;	/* flat buttons widget */
	XtArgVal	*perms;	/* permissions items */
	Boolean		rprev;	/* previous read perm. */
	Boolean		wprev;	/* previous write perm. */
	Boolean		eprev;	/* previous exec. perm */
} DmFPropPermsRec, *DmFPropPermsPtr;

/* File Property Sheet structure: list of these hangs off Folder window */
typedef struct _DmFPropSheetRec *DmFPropSheetPtr;
typedef struct _DmFPropSheetRec {
    DmFPropSheetPtr	next;		/* next property sheet */
    struct dmFolderWinRec * window;	/* folder window owner */
    Widget		shell;		/* popup shell */
    DmItemPtr		item;		/* item within folder */
    DmFPropTFPtr	fntfptr;	/* file name TF info. */
    DmFPropTFPtr	owntfptr;	/* owner name TF info. */
    DmFPropTFPtr	grptfptr;	/* group name TF info. */
    DmFPropPermsPtr	ownptr;		/* owner perm. info. */
    DmFPropPermsPtr	grpptr;		/* group perm. info. */
    DmFPropPermsPtr	othptr;		/* other perm. info. */
    Widget		timestw;	/* mod. time static text wid.*/	
    Widget		iclstw;		/* icon class name stext wid.*/	
    DmFPropTFPtr	cmtptr;		/* comment field  info. */
    XtArgVal *		button_items;	/* flat info. for ctl. buttons*/
    short		prop_num;	/* serial prop. sheet # */
    Boolean		flag;		/* whether to popdown */
} DmFPropSheetRec;

/* This structure stores info about clients that want to be notified
   when a folder window is closed.
*/
typedef struct dmCloseNotifyRec {
    long		serial;	
    Window		client;
    Atom		replyq;
					/* 'short' fields go next */
    unsigned short	version;
					/* 'byte' fields go last */
} DmCloseNotifyRec, *DmCloseNotifyPtr;

/* This is the standard header that is in both the folder window and the
   toolbox window structure.
*/
#define DM_WIN_HEADER(WINTYPE) \
    DtAttrs		attrs;		/* window type, etc. */		\
    void *		gizmo_shell;	/* "toplevel" gizmo (BaseWindow, etc) */\
    Widget		shell;		/* shell */			\
    Widget		swin;		/* scrolled window */		\
    Widget		box;		/* icon box */			\
    DmContainerPtr	cp;		/* ptr to container struct */	\
    DmItemPtr		itp;		/* ptr to item list */		\
    Cardinal		nitems;		/* number of items */		\
    char *		title;		/* this may not be needed */	\
    DmViewFormatType	view_type;	/* current view type */		\
    DmViewSortType	sort_type;	/* current sort type */		\
    WINTYPE		next;		/* next window */		\
					/* put 'short' fields next */


typedef struct dmWinRec *DmWinPtr;
typedef struct dmWinRec {
	DM_WIN_HEADER(DmWinPtr)
} DmWinRec;

#define DM_WIN_PATH(win)	( (win)->cp->path )
#define DM_WIN_BASE_GIZMO(win)	( (BaseWindowGizmo *)((win)->gizmo_shell) )
#define DM_WIN_ITEM(W, I)	((W)->itp + (I))
#define DM_WB_PATH(D)		DM_WIN_PATH(DESKTOP_WB_WIN(D))
#define IS_WB_PATH(D, path)	( strcmp(path, DM_WB_PATH(D)) == 0 )
#define NUM_WB_OBJS(D)        (DESKTOP_WB_WIN(D)->cp->num_objs)
#define WB_IS_EMPTY(D)        (NUM_WB_OBJS(D) == 0)

/* A list of DmFolderWindowRec structures is used to store the information 
   about open folder windows.
*/
typedef struct dmFolderWinRec *DmFolderWinPtr;
typedef struct dmFolderWinRec {
    DM_WIN_HEADER(DmFolderWinPtr)

    DmFVFilter		filter;		/* view filter info */
    struct _FileGizmo *	copy_prompt;
    struct _FileGizmo *	move_prompt;
    struct _FileGizmo *	link_prompt;
    struct _PopupGizmo* rename_prompt;
    DmPromptRec		create_prompt;
    struct _FileGizmo *	folder_prompt;
    struct _PopupGizmo* customWindow;	/* Popup Gizmo for customized view */
    struct _PopupGizmo* finderWindow;	/* Popup Gizmo for find utility */
    struct _PopupGizmo* createWindow;	/* Popup Gizmo for New utility */
    struct _ModalGizmo*	overwriteNotice;/* Overwrite notice */
    DmFPropSheetPtr	fpsptr;		/* list of file prop. sheets */
    DmTaskInfoListPtr	task_id;
    int			num_notify;	/* number of notify entries */
    DmCloseNotifyPtr	notify;		/* ptr to the list of notify entries */
					/* 'short' fields go next */
					/* 'byte' fields go last */
    Boolean		filter_state;
} DmFolderWinRec, *DmFolderWindow;

typedef struct dmDnDFileOpRec {
    OlDnDTriggerOperation	type;
    DmWinPtr			wp;
    Atom 			selection;
    Time			timestamp;
					/* 'short' fields next */
    Position			root_x;
    Position			root_y;
} DmDnDFileOpRec, *DmDnDFileOpPtr;

typedef struct dmDnDInfoRec {
    DmWinPtr			wp;		/* base window struct ptr */
    DmItemPtr			ip;		/* "current" item */
    DmItemPtr *			list_idx;	/* "current" item in list */
    DmItemPtr *			ilist;		/* item list */
    Atom			selection;	/* source selection */
    Time			timestamp;	/* timestamp of trigger msg */
    OlDnDTriggerOperation	opcode;		/* opcode (copy or move) */
    DtAttrs			attrs;		/* transaction attributes */
    XtPointer			user_data;	/* a hook for special info */
						/* Put 'short' fields next */
    Position			x;		/* position rel. to dst win */
    Position			y;		/* position rel. to dst win */
						/* Put 'byte' fields last */
} DmDnDInfoRec, *DmDnDInfoPtr;

/* Structure for appl resources */
typedef struct dmOptions {
    OlFontList	*	font_list;		/* NOTE: remove this */
    Pixel		help_key_color;		/* help key color */
    int			sync_interval;		/* in millisecs */
    int			wb_timer_interval; 	/* timer interval */
    int			wb_timer_unit;		/* minute, hour or day */
					/* 'short' fields go next */
    Dimension		grid_width;
    Dimension		grid_height;
					/* 'byte' fields go last */
    u_char		folder_cols;
    u_char		folder_rows;
    Boolean		show_full_path;
    Boolean		show_hidden_files;
    u_char		tree_depth;		/* default depth of tree view */
    Boolean		wb_suspend;		/* was timer suspended */
    int		     wb_cleanUpMethod;	/* is clean-up method */
} DmOptions;

/* DmDesktopRec structure contains all the global data associated with the
   desktop. If multiple (vitual) desktops are supported in the future, then
   several instances of this structure will be created.
*/
typedef struct dmDesktopRec {
    Widget		init_shell;	/* shell from Xt[App]Initialize() */
    DmFmodeKeyPtr	fmkp;		/* list of internal file classes */
    DmFnameKeyPtr	fnkp;		/* list of external file classes */
    DmFclassFilePtr	fcfp;		/* list of class files */
    DmFolderWindow	folders;		/* list of folder windows */
    DmFolderWindow	wb_win;		/* folder window pointer for wb */
    DmFolderWindow	help_desk;	/* folder window ptr for help desk */
    DmFolderWindow	top_folder;	/* DESKTOP HOME folder */
    DmFolderWindow	tree_win;		/* Tree view window */
    DmHelpAppPtr	help_info;	/* list of HM's app info */
    DtPropList		properties;	/* desktop property list */
    DmOptions		options;		/* desktop options */
    DmTaskInfoListPtr	current_task;	/* current task info. */	
    DmGlyphPtr		shortcut;		/* glyph for shortcut */
    Window		wb_icon_win;	/* wb icon window id */
    int			folder_help_id;	/* Help Appl ID for folders */
    int			wb_help_id;	/* Help Appl ID for wastebasket */
    int			ib_help_id;	/* Help Appl ID for icon binder */
    int			fmap_help_id;	/* Help Appl ID for folder map */
    int			desktop_help_id;	/* Help Appl ID for Destiny window */
    char *		node_name;
    mode_t		umask;
    char *		home;		/* convenient to store $HOME */
    char *		cwd;			/* current working directory */
    char *		dt_dir;		/* desktop directory */
    XFontStruct	*	font;		/* The default (FIconBox) font */
    XFontStruct	*	fixed_font;	/* The default fixed width font */
    XtIntervalId	sync_timer;	/* so timer can be removed */
    struct _StaleList *	stale_folders;	/* list of folders needing updates */
					/* visited folder "ring buffer": */
#define MAX_VISITED_FOLDERS	7
    char *		visited_folders[MAX_VISITED_FOLDERS];
    char **		visited_head;	/* points to "head" of visited array */
					/* Put 'short' fields next */
    u_short		stale_index;
					/* Put 'byte' fields last */
    Boolean		bg_not_regd;	/* XtAppProc() registered or not */
} DmDesktopRec, *DmDesktopPtr;

extern DmDesktopPtr Desktop;	/* There is one global Destop struct */

/* macros to access the global desktop structure */
#define DESKTOP_CUR_TASK(D)	( (D)->current_task )
#define DESKTOP_CWD(D)		( (D)->cwd )
#define DESKTOP_DIR(D)		( (D)->dt_dir )
#define DESKTOP_FIXED_FONT(D)	( (D)->fixed_font )
#define DESKTOP_FMKP(D)		( (D)->fmkp )
#define DESKTOP_FNKP(D)		( (D)->fnkp )
#define DESKTOP_FOLDERS(D)	( (D)->folders )
#define DESKTOP_FONT(D)		( (D)->font )
#define DESKTOP_HELP_DESK(D)	( (D)->help_desk )
#define DESKTOP_HELP_ID(D)	( (D)->desktop_help_id )
#define	DESKTOP_HELP_INFO(D)	( (D)->help_info )
#define	DESKTOP_HOME(D)		( (D)->home )
#define DESKTOP_NODE_NAME(D)	( (D)->node_name )
#define DESKTOP_OPTIONS(D)	( (D)->options )
#define DESKTOP_PROPS(D)	( (D)->properties )
#define DESKTOP_SHELL(D)	( (D)->init_shell )
#define DESKTOP_SHORTCUT(D)	( (D)->shortcut )
#define DESKTOP_TOP_FOLDER(D)	( (D)->top_folder )
#define DESKTOP_TOP_TB(D)	( (D)->top_tb )
#define DESKTOP_UMASK(D)	( (D)->umask )
#define DESKTOP_WB_ICON(D)	( (D)->wb_icon_win )
#define DESKTOP_WB_WIN(D)	( (D)->wb_win )
#define STALE_FOLDERS(D)	( (D)->stale_folders )
#define STALE_INDEX(D)		( (D)->stale_index )
#define SYNC_TIMER(D)		( (D)->sync_timer )
#define TREE_WIN(D)		( (D)->tree_win )
#define VISITED_FOLDERS(D)	( (D)->visited_folders )
#define VISITED_HEAD(D)		( (D)->visited_head )
#define WB_HELP_ID(D)		( (D)->wb_help_id )
#define BINDER_HELP_ID(D)	( (D)->ib_help_id )
#define FMAP_HELP_ID(D)		( (D)->fmap_help_id )
#define FOLDER_HELP_ID(D)	( (D)->folder_help_id )

#define DESKTOP_SCREEN(D)	( XtScreen(DESKTOP_SHELL(D)) )
#define DESKTOP_DISPLAY(D)	( XtDisplay(DESKTOP_SHELL(D)) )

/* macros to access desktop options */
#define DESKTOP_FONTLIST(D)	( DESKTOP_OPTIONS(D).font_list )
#define FOLDER_COLS(D)		( DESKTOP_OPTIONS(D).folder_cols )
#define FOLDER_ROWS(D)		( DESKTOP_OPTIONS(D).folder_rows )
#define GRID_WIDTH(D)		( DESKTOP_OPTIONS(D).grid_width )
#define GRID_HEIGHT(D)		( DESKTOP_OPTIONS(D).grid_height )
#define SHOW_FULL_PATH(D)	( DESKTOP_OPTIONS(D).show_full_path )
#define SHOW_HIDDEN_FILES(D)	( DESKTOP_OPTIONS(D).show_hidden_files )
#define SYNC_INTERVAL(D)	( DESKTOP_OPTIONS(D).sync_interval )
#define TREE_DEPTH(D)		( DESKTOP_OPTIONS(D).tree_depth )

/******************************************************************************

	III.  #define's for BIT-FIELDS/OPTIONS
*/

/* Attributes for DmWinRec 'attrs' field
	The DM_B_*_WIN defines below are used to identify the type of window.
	This would normally be done by registering the 'Window' structure as
	client data but this cannot be done when menus are shared between
	folders, for instance.
*/
#define DM_B_SHOWN_MSG		(1 << 0)
#define DM_B_FILE_OP_BUSY	(1 << 1)
#define DM_B_FOLDER_WIN		(1 << 2)
#define DM_B_FOUND_WIN		(1 << 3)
#define DM_B_HELP_WIN		(1 << 4)
#define DM_B_HELPDESK_WIN	(1 << 5)
#define DM_B_TREE_WIN		(1 << 6)
#define DM_B_WASTEBASKET_WIN	(1 << 7)

#define DM_B_BASE_WINDOW	(DM_B_FOLDER_WIN | DM_B_FOUND_WIN | \
				 DM_B_HELP_WIN | DM_B_HELPDESK_WIN | \
				 DM_B_TREE_WIN | DM_B_WASTEBASKET_WIN)


/* options to DmOpenFolderWindow() and DmOpenDir() */
#define DM_B_INIT_FILEINFO	(1 << 0)
#define DM_B_SHOW_HIDDEN_FILES	(1 << 1)
#define DM_B_SET_TYPE		(1 << 2)
#define DM_B_TIME_STAMP		(1 << 3)
#define DM_B_READ_DTINFO	(1 << 4)

/* options to DmCloseDir() & DmCloseContainer() */
#define DM_B_NO_FLUSH		(1 << 0)

/* DmReadDtInfo() options */
#define INTERSECT		(1 << 0)

/* options to DmWriteDtInfo() */
#define DM_B_PERM		(1 << 0)

/* options to DmAddShortcut() and DmUniqueLabel */
#define DM_B_DUPCHECK		(1 << 0)
#define DM_B_MOVE		(1 << 1)

/* options to DmDnDNewTransaction() */
#define DM_B_SEND_EVENT		(1 << 0)

/* options for updating a folder window */
#define DM_UPDATE_SRCWIN	( 1 << 0)
#define DM_UPDATE_DSTWIN	( 1 << 1)

/* options in DmFnameKeyRec and DmFmodeKeyRec structure */
#define DM_B_FILETYPE		(1 << 0)
#define DM_B_FILEPATH		(1 << 1)
#define DM_B_REGEXP		(1 << 2)
#define DM_B_SYS		(1 << 3)
#define DM_B_DELETED		(1 << 4)
#define DM_B_NEW		(1 << 5)
#define DM_B_VISUAL		(1 << 6)
#define DM_B_TYPING		(1 << 7)
#define DM_B_REPLACED		(1 << 8)
#define DM_B_NEW_NAME		(1 << 9)
#define DM_B_ACTION		(1 << 10)
#define DM_B_CLASSFILE		(1 << 11)
#define DM_B_BAD_CLASSFILE	(1 << 12)
#define DM_B_WRITE_FILE		(1 << 13)
#define DM_B_READONLY		(1 << 14)
#define DM_B_LREGEXP		(1 << 15)
#define DM_B_LFILEPATH		(1 << 16)
#define DM_B_OVERRIDDEN		(1 << 17)

/* options for layout routines */
#define UPDATE_LABEL		(1 << 1)   /* 0 for NONE, undefined for now */
#define	RESTORE_ICON_POS	(1 << 2)
#define	SAVE_ICON_POS		(1 << 3)

/* options for file operations-DmDoFileOp (internal ones must fit in u_char) */
#define OVERWRITE		( 1 << 0 )	/* Also used internally */
#define REPORT_PROGRESS		( 1 << 1 )
#define OPRBEGIN		( 1 << 2 )
#define	UNDO			( 1 << 3 )
#define	RMNEWDIR		( 1 << 4 )	/* during UNDO */
#define MULTI_PATH_SRCS		( 1 << 5 )
#define EXTERN_DND		( 1 << 6 )
#define EXTERN_SEND_DONE	( 1 << 7 )
#define DONT_ADJUST_TARGET	( 1 << 8 )
#define DONT_OVERWRITE		( 1 << 9 )	/* return error instead */

/* options for DmDnDInfoRec */
#define DM_B_TRANS_IN		( 1 << 1 )	/* incoming transaction */
#define DM_B_COPY_OP 		( 1 << 2 )	/* copy operation */
#define DM_B_DELETE  		( 1 << 3 )	/* delete object when done */

/******************************************************************************
	IV.  #define's for INTEGER VALUES
*/


/******************************************************************************
	V.  #define's for STRINGS
*/

#define DM_INFO_FILENAME	".dtinfo"

/* desktop properties */
#define ICONPATH		"_ICONPATH"
#define FILEDB_PATH		"FILEDB_PATH"
#define WBDIR			"WBDIR"
#define HDPATH			"HDPATH"
#define DESKTOPDIR		"DESKTOPDIR"
#define TEMPLATEDIR		"TEMPLATEDIR"
#define IGNORE_SYSTEM		"_IGNORE_SYSTEM"

/* class properties */
#define REAL_PATH		"_REALPATH"
#define PATTERN			"_PATTERN"
#define LPATTERN		"_LPATTERN"
#define LFILEPATH		"_LFILEPATH"
#define TEMPLATE		"_TEMPLATE"
#define UNIQUE			"_UNIQUE"
#define CLASS_NAME		"_CLASSNAME"
#define ICON_LABEL		"_ICONLABEL"
#define SYSTEM			"_SYSTEM"

/* format used to convert time to a string */
#define TIME_FORMAT		"%X %a %x"

/******************************************************************************
	VI.  MACROS
*/

/* Macros to get default width and height of folder window.  */
#define FOLDER_WINDOW_WIDTH(widget)	(68 * WidthOfScreen(		\
						XtScreen(widget)) / 100)

#define FOLDER_WINDOW_HEIGHT(widget)	(35 * HeightOfScreen(	\
						XtScreen(widget)) / 100)

#define IS_TREE_WIN(D, W)	( (W)->attrs & DM_B_TREE_WIN )
#define IS_FOUND_WIN(D, W)	( (W)->attrs & DM_B_FOUND_WIN )
#define IS_WB_WIN(D, W)		( (W)->attrs & DM_B_WASTEBASKET_WIN )

/* Macro to find out what WB clean-up method is */
#define DM_WBIsByTimer(D)	( DmWBCleanUpMethod(D) == 0 )
#define DM_WBIsOnExit(D)		( DmWBCleanUpMethod(D) == 1 )
#define DM_WBIsImmediate(D)	( DmWBCleanUpMethod(D) == 2 )
#define DM_WBIsNever(D)		( DmWBCleanUpMethod(D) == 3 )

#define OBJ_CLASS_NAME(OP)	(((DmFnameKeyPtr)((OP)->fcp->key))->name)
#define BUSY_CURSOR(W)		DtLockCursor((W), 2000L, NULL, NULL, \
					OlGetBusyCursor(W))

/* Macro to check for "/" */
#define ROOT_DIR(path)	( (path[0] == '/') && (path[1] == '\0') )

/* check if the name is either "." or ".." */
#define IS_DOT_DOT(P)	(P[0] == '.') &&			\
                         ((P[1] == '\0') ||			\
                         ((P[1] == '.') && (P[2] == '\0')))

/* macros used with DmObjectRec structure */
#define IS_EXEC(OP)		((OP)->ftype == DM_FTYPE_EXEC)

/* Macro to compute text width of string using the Desktop font[list] */
#define DmTextWidth(desktop, str, cnt) \
    DM_TextWidth(DESKTOP_FONT(desktop), DESKTOP_FONTLIST(desktop), str, cnt)

/* Macro to compute width/height of "Desktop" font[list] */
#define DM_FontWidth(desktop) \
    OlFontWidth(DESKTOP_FONT(desktop), DESKTOP_FONTLIST(desktop))
#define DM_FontHeight(desktop) \
    OlFontHeight(DESKTOP_FONT(desktop), DESKTOP_FONTLIST(desktop))

/* Macros for computing "row heights" of views with "fixed" heights.
   (Assumption: DM_FTYPE_DIR and DM_FTYPE_DATA small glyphs are same height.)
   + 2 ICON_PADDING is for frame around label; + 2-1/2 is for shortcut
   glyph and space between shortcut and the glyph.
*/
    /* Height in NAME view is greater of "small" glyph and (max) font */
#define DM_NameRowHeight(D)                                    		\
	(Dimension)DM_Max((int)DM_FontHeight(D) + 2 * ICON_PADDING,     \
	(int)(DmFtypeToFmodeKey(DM_FTYPE_DIR)->small_icon->height 	\
	+ 2 * ICON_PADDING + ICON_PADDING / 2)) 

    /* Height in LONG view is greater of "small" glyph and fixed font */
#define DM_LongRowHeight(D)                                      	\
	(Dimension)DM_Max((int)(DmFtypeToFmodeKey(DM_FTYPE_DIR)->	\
	small_icon->height + + 2 * ICON_PADDING + ICON_PADDING / 2),	\
	(int)OlFontHeight(DESKTOP_FIXED_FONT(D),        		\
	DESKTOP_FONTLIST(D)) + 2 * ICON_PADDING)

/* Macros to clear Footer areas */
#define DmClearStatus(window)	DmVaDisplayStatus(window, False, NULL);
#define DmClearState(window)	DmVaDisplayState(window, NULL);

/* Macros for sync timer */
#define Dm__AddSyncTimer(D)	SYNC_TIMER(D) = \
    XtAddTimeOut(SYNC_INTERVAL(D), Dm__SyncTimer, (XtPointer)(D))
#define Dm__RmSyncTimer(D)	XtRemoveTimeOut(SYNC_TIMER(D))

/* Macro to make an item label based on the view-type */
#define Dm__MakeItemLabel(item, view_type, maxlen) (view_type == DM_LONG) ? \
    DmGetLongName(item, (maxlen) + 3) : DmGetObjectName(ITEM_OBJ(item))


/* These macros are used to define MenuGizmo structures. */

#define MENU_ITEM(LABEL, MNEMONIC, SELECT_PROC)	\
    {						\
	(XtArgVal)True,	/* sensitive */		\
	LABEL,					\
	MNEMONIC,				\
	NULL,		/* sub menu/resource */	\
	SELECT_PROC,				\
	NULL,		/* client_data */	\
	False,		/* set */		\
    }

#define MENU(NAME, ITEMS)	\
	static MenuGizmo ITEMS = {				\
		NULL,			/* help */			\
		NAME,			/* shell widget name */		\
		NULL,			/* title (implies pin) */	\
		ITEMS ## Items,		/* items */			\
		NULL,			/* default selectProc */	\
		NULL,			/* default clientData */	\
		CMD,			/* button type */		\
		OL_FIXEDCOLS,		/* layout type */		\
		1,			/* measure */			\
		0,			/* default item index */	\
	}

#define MENU_BAR(NAME, ITEMS, SELECT, DEFAULT_ITEM)			\
	static MenuGizmo ITEMS = {					\
		NULL,			/* help */			\
		NAME,			/* shell widget name */		\
		NULL,			/* title (implies pin) */	\
		ITEMS ## Items,		/* items */			\
		SELECT,			/* default selectProc */	\
		NULL,			/* default clientData */	\
		CMD,			/* button type */		\
		OL_FIXEDROWS,		/* layout type */		\
		1,			/* measure */			\
		DEFAULT_ITEM,		/* default item index */	\
	}

#define MENUBAR(NAME, ITEMS)	MENU_BAR(NAME, ITEMS, NULL, OL_NO_ITEM)

#endif /* __Dtm_h_ */




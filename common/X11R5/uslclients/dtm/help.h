/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:help.h	1.63"

#ifndef _help_h
#define _help_h

#include <mapfile.h>
#include <HyperText.h>
#include <DtI.h>

/* default highlighting color for hypertext widget */
#define DEFAULT_HELP_KEY_COLOR	"#00000000E000"

/* default width of help window text pane (in number of chars) */
#define DEFAULT_HELPFILE_WIDTH  70

/* default section name for a section with no name; not seen by the user */
#define DEFAULT_SECTION_NAME "Unknown"

/* section name for Table of Contents */
#define TABLE_OF_CONTENTS "TOC"

/* default height of help window text pane (in number of lines) */
#define DEFAULT_HWIN_HEIGHT 15

#define XtNhelpKeyColor	"helpKeyColor"

#define DESCRP         "_DESCRP"
#define HELP_FILE      "_HELPFILE"
#define HELP_DIR       "_HELPDIR"
#define DFLT_ICONLABEL "_DFLT_ICONLABEL"

/*
 * Since keywords and definitions are stored as properties. Attrs is
 * used to distinguish them.
 */
#define DM_B_KEYWORD	(1 << 0)
#define DM_B_DEF		(1 << 1)

/* macros for accessing HyperText widget's HyperSegment structures */
#define HyperSegmentKey(HS)        ((HS)->key)
#define HyperSegmentText(HS)       ((HS)->text)
#define HyperSegmentScript(HS)     ((HS)->script)
#define HyperSegmentRV(HS)         ((HS)->reverse_video)

typedef struct dmHelpAppRec *DmHelpAppPtr;

/*
 * This flat list expects a pointer to a vector of pointers to
 * be supplied as XtNformatData.  Therefore, instead of making
 * the bookmark label fields part of the DmHelpBmarkRec structure,
 * they are contained in a separate structure.
 */

typedef struct dmBmarkLabelRec *DmBmarkLabelPtr;
typedef struct dmBmarkLabelRec {
	XtPointer		app_name;  /* application name                */
	XtPointer		file_name; /* file name                       */
	XtPointer		sect_name; /* section name for display        */
	XtPointer		sect_tag;	 /* section tag to locate a section */
} DmBmarkLabelRec;

typedef struct dmHelpBmarkRec *DmHelpBmarkPtr;
typedef struct dmHelpBmarkRec {
	DmBmarkLabelPtr	blp;		/* ptr to bookmark label */
	DmHelpBmarkPtr		next;	/* ptr to next bookmark */
} DmHelpBmarkRec;

/*
 * This structure describes a unique location in a help file.
 */
typedef struct {
	char		*file;      /* help file name */
	char		*sect_tag;  /* section tag    */
	char		*sect_name; /* section name   */
	char		*string;    /* string source  */
} DmHelpLocRec, *DmHelpLocPtr;

/*
 * This structure contains the name of a section and the name of its
 * associated notes file.
 */
typedef struct dmHelpNotesRec *DmHelpNotesPtr;
typedef struct dmHelpNotesRec {
	char			*sect_tag;     /* section tag     */
	char			*sect_name;    /* section name    */
	char			*notes_file;   /* notes file name */
	DmHelpNotesPtr	next;          /* next notes rec  */
} DmHelpNotesRec;

/*
 * This structure describes each section in a help file. The sections are
 * read in pre-order into an array.
 */
typedef struct dmHelpSectRec *DmHelpSectPtr;
typedef struct dmHelpSectRec {
	unsigned short	level;         /* level in the tree */
	DtAttrs		attrs;         /* section attributes */
	char			*name;         /* section name */
	char			*alias;        /* alias section name */
	char			*tag;          /* section tag */
	char			*raw_data;     /* ptr to the section in file*/
	char			*cooked_data;  /* ptr to processed section */
	unsigned long	raw_size;      /* size of the raw section */
	unsigned long	cooked_size;   /* size of processed section */
	DtPropList	keywords;      /* list of keywords */
	DtPropList	defs;          /* list of definitions */
	Boolean		matched;       /* matched section in search */
	Boolean		notes_chged;   /* was notes changed */
} DmHelpSectRec;

/*
 * This structure describes a help file.
 */
typedef struct {
	int			count;         /* usage count */
	int			version;       /* version */
	char			*name;         /* file name */
	char			*title;        /* title */
	char			*notes;        /* name of notes file */
	DmHelpNotesPtr	notesp;        /* pointer to notes */
	DmMapfilePtr	mp;            /* map file data */
	DmHelpSectPtr	toc;           /* ptr to toc section */
	DmHelpSectPtr	sections;      /* ptr to section array */
	unsigned short	num_sections;  /* size of sections array */
	unsigned short	width;         /* width of displayed text */
	DtAttrs		attrs;         /* file attributes */
	DtPropList	keywords;      /* list of keywords */
	DtPropList	defs;          /* list of definitions */
	DmHelpSectPtr	prev_hsp;      /* previous match */
} DmHelpFileRec, *DmHelpFilePtr;

/*
 * This structure describes a help window. The history stack is stored here.
 */
typedef struct {
	/* These fields must be the same as DmWinRec */
	DtAttrs		attrs;         /* window type, etc. */	
	void *		gizmo_shell;   /* "toplevel" gizmo */
	Widget		shell;         /* shell */
	Widget		swin;          /* scrolled window */

	Widget		def_shell;     /* definition shell */
	Widget		gloss_shell;   /* glossary shell */
	Widget		notes_shell;   /* notes shell */
	Widget		notes_btns;    /* notes flat buttons */
	Widget		search_shell;  /* search shell */
	Widget		bmark_shell;   /* bookmark shell */
	Widget		bmark_swin;    /* bookmark swin */
	Widget		bmark_btns;    /* bookmark flat buttons */
	Widget		htext;         /* hypertext widget */
	Widget		notes_te;      /* notes textedit widget */
	Widget		search_tf;     /* search textfield widget */
	Widget		bmark_flist;   /* bookmark flat list */
	Widget		magnifier;     /* magnifier widget */
	Widget		menubar;       /* menubar widget */
	DmHelpFilePtr	hfp;           /* ptr to file rec */
	DmHelpSectPtr	hsp;           /* ptr to current section rec*/
	DmHelpLocPtr	stack;         /* ptr to stack */
	int			stack_size;    /* stack size */
	int			sp;            /* stack pointer (idx) */
	int			app_id;        /* application id */
} DmHelpWinRec, *DmHelpWinPtr;

/*
 * This structure stores info about each application that has registered
 * with the help manager. It assumes that there is one window per application.
 * There is a linked list of these structures hanging off of the desktop
 * structure.
 */
typedef struct dmHelpAppRec {
	Screen		*scrn;         /* app screen ptr */
	Window		app_win;       /* app toplevel win id*/
	int			app_id;        /* application id */
	int            num_bmark;     /* number of bookmarks */
	char			*name;         /* application name */
	char			*title;        /* application title */
	char			*help_dir;     /* help directory */
	DmHelpWinRec	hlp_win;       /* help window data */
	DmHelpAppPtr	next;          /* next app */
	DmHelpBmarkPtr	bmp;           /* ptr to list of bookmarks */
	Pixmap		icon_pixmap;   /* icon pixmap */
} DmHelpAppRec;

/*
 * This structure is used for storing the file class information
 * for icons and the list of applications in the Help Desk.
 */
typedef struct dmHDDataRec *DmHDDataPtr;
typedef struct dmHDDataRec {
	DmFclassPtr	fcp;           /* ptr to file class */
	DmHelpAppPtr	hap;           /* ptr to app structure */
	Pixel		key_color;     /* hypertext color */
} DmHDDataRec;


/** external declarations **/
extern DmHDDataPtr	hddp;

/* general routines */
extern char *DmGetSectTag(DmHelpFilePtr hfp, char *sect_name, Boolean alias);
extern char *Dm__GetSectName(DmHelpFilePtr hfp, char *sect_tag);
extern int Dm__GetNameValue(DmMapfilePtr mp, char **name, char **value);
extern int Dm__GetNotes(DmHelpFilePtr hfp, int app_id);
extern int DmGetKeyword(DmMapfilePtr mp, DtPropListPtr plistp);
extern int DmGetDefinition(DmMapfilePtr mp, DtPropListPtr plistp);
extern int DmReadDefFile(DmMapfilePtr mp, DtPropListPtr plistp, char *path);
extern DmHelpLocPtr DmHelpRefToLoc(char *p);

/* application related routines */
extern void DmRemoveAppFromList(DmHelpAppPtr hap);
extern void DmFreeHelpAppID(int app_id);

/* help window related routines */
extern int DmCreateHelpWindow(DmHelpWinPtr hwp, int  x, int  y, char *geom_str,
	Boolean iconic);
extern void DmCloseHelpWindow(DmHelpWinPtr hwp);
extern int DmDisplayHelpString(DmHelpWinPtr hwp, int app_id, char *title,
	char *string, int  x, int  y);
extern void DmChgHelpWinBtnState(DmHelpWinPtr hwp, Boolean skip_menubar,
	Boolean sensitive);

/* help file related routines */
extern DmHelpFilePtr DmOpenHelpFile(int app_id, char *filename);
extern void DmCloseHelpFile(DmHelpFilePtr hfp);

/* section related routines */
extern int DmProcessHelpSection(DmHelpSectPtr hsp);
extern void DmFreeHelpSection(DmHelpSectPtr hsp);
extern void DmFreeAllHelpSections(DmHelpSectPtr hsp, int count);
extern DmHelpSectPtr DmGetSection(DmHelpFilePtr hfp, char *name);
extern int DmPushHelpStack(DmHelpWinPtr hwp, char *file, char *sect_name,
	char *sect_tag);

/* help window button callbacks */
extern void DmHtextSelectCB(Widget, XtPointer, XtPointer);
extern void DmNextSectionCB(Widget, XtPointer, XtPointer);
extern void DmPrevSectionCB(Widget, XtPointer, XtPointer);
extern void DmBackTrackCB(Widget, XtPointer, XtPointer);
extern void DmTOCCB(Widget, XtPointer, XtPointer);
extern void DmGlossaryCB(Widget, XtPointer, XtPointer);
extern void DmUsingHelpCB(Widget, XtPointer, XtPointer);
extern void DmOpenHelpDeskCB(Widget, XtPointer, XtPointer);
extern void DmHelpDeskCB(Widget, XtPointer, XtPointer);
extern void DmSearchCB(Widget, XtPointer, XtPointer);
extern void DmNotesCB(Widget, XtPointer, XtPointer);
extern void DmReadNotes(DmHelpWinPtr hwp, char *cur_tag);
extern void DmBookmarkCB(Widget, XtPointer, XtPointer);
extern void DmAddBookmarkCB(Widget, XtPointer, XtPointer);
extern void DmGoToBookmarkCB(Widget, XtPointer, XtPointer);
extern void DmHelpWinPopdownCB(Widget, XtPointer, XtPointer);

/* external help desk routines */
extern void DmHDSelectProc(Widget, XtPointer, XtPointer);
extern void DmHDDblSelectProc(Widget, XtPointer, XtPointer);
extern void DmHDMenuProc(Widget, XtPointer, XtPointer);
extern void DmHDGlossaryCB(Widget, XtPointer, XtPointer);
extern void DmHDIMOpenCB(Widget, XtPointer, XtPointer);
extern void DmHDOpenCB(Widget, XtPointer, XtPointer);
extern void DmHDAlignCB(Widget, XtPointer, XtPointer);
extern int  DmDelAppFromHD(char *app_name, char *icon_label);
extern void DmHDHelpCB(Widget, XtPointer, XtPointer);
extern void DmHDHelpTOCCB(Widget, XtPointer, XtPointer);
extern void DmGetHDAppInfo(char *help_file, char **label, char **descrp);
extern int DmAddAppToHD(char *app_name, char *icon_label, char *icon_file,
	char *help_file, char *help_dir, char *fullpath);

extern Boolean DmHDTriggerNotify(Widget w, Window win, Position x, Position y,
	Atom selection, Time time_stamp, OlDnDDropSiteID drop_site_id,
	OlDnDTriggerOperation dnd_op, Boolean send_done, XtPointer closure);

#endif /* _help_h */

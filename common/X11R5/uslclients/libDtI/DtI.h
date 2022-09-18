/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef __DtI_h_
#define __DtI_h_

#pragma ident	"@(#)libDtI:DtI.h	1.61"

#include <stdio.h>
#include <limits.h>
#include <sys/types.h>		/* for time_t in DmContainerRec */
#include <Xol/OpenLook.h>
#include <Xol/OlgP.h>
#include <Dt/Desktop.h>
#include "FIconBox.h"

/* file types */
typedef enum {
    DM_FTYPE_DIR = 1,
    DM_FTYPE_EXEC,
    DM_FTYPE_DATA,
    DM_FTYPE_FIFO,
    DM_FTYPE_CHR,
    DM_FTYPE_BLK,
    DM_FTYPE_SEM,
    DM_FTYPE_SHD,
    DM_FTYPE_UNK,
} DmFileType;

/*
 * The DmGlyphRec structure is used to store pixmap information by
 * pixmap caching functions.
 */
typedef struct dmGlyphRec {
	char		*path;		/* filename */
	Pixmap		pix;
	Pixmap		mask;
	Dimension	width;
	Dimension	height;
	short		depth;
	short		count;		/* usage count */
} DmGlyphRec, *DmGlyphPtr;

/*
 * This structure stores info returned from stat(2) about each file, and
 * hangs off of the DmObjectRec.
 */
typedef struct dmFileInfo {
	mode_t		mode;
	nlink_t		nlink;
	uid_t		uid;
	gid_t		gid;
	off_t		size;
	time_t		mtime;
} DmFileInfo, *DmFileInfoPtr;

/*
 * The DmFclassRec structure stores the information about file class.
 * The information stored in this structure is common to both the internal
 * list of file classes and the list of file classes described in the
 * file database.
 */
typedef struct dmFclassRec {
	DtAttrs		attrs;
	DtPropList	plist;
	DmGlyphPtr	glyph;
	DmGlyphPtr	cursor;
	void *		key;		/* ptr back to FnameKey or FmodeKey */
} DmFclassRec, *DmFclassPtr;

/*
 * Information about each open directory and other containers is stored 
 * in the DmContainerRec structure.
 */
typedef struct dmContainerRec {
    struct dmContainerRec *	next;
    char *			path;
    int				count;		/* usage count */
    struct dmObjectRec *	op;
    int				num_objs;	/* number of objects */
    DtAttrs			attrs;
    DtPropList			plist;		/* folder win properties */
    time_t			time_stamp;	/* "update" time */
    void *			data;		/* user specific data */
} DmContainerRec, *DmContainerPtr;

/*
 * Information about an object in a container is stored in the
 * DmObjectRec structure.
 */
typedef struct dmObjectRec {
				/* CAUTION: Items in Tree view depend on
				   the ordering of the following fields */
    DmContainerPtr	container;
    char *		name;
    DmFileType		ftype;
    DmFclassPtr		fcp;
    DtAttrs		attrs;
    DtPropList		plist;
				/* End of order-dependent fields */
    int			x;
    int			y;
    void *		objectdata;
    struct dmObjectRec *next;
} DmObjectRec, *DmObjectPtr;

/*
 * Information about the items in an iconbox is stored in an array of
 * DmItemRec structure.
 */
typedef struct dmItemRec {
    XtArgVal	/* String	*/	label;	
    XtArgVal	/* Position	*/	x;	
    XtArgVal	/* Position	*/	y;
    XtArgVal	/* Dimension	*/	icon_width;
    XtArgVal	/* Dimension	*/	icon_height;
    XtArgVal	/* Boolean	*/	managed;
    XtArgVal	/* Boolean	*/	select;
    XtArgVal	/* Boolean	*/	busy;
    XtArgVal	/* XtPointer	*/	client_data;
    XtArgVal	/* DmObjectPtr	*/	object_ptr;
} DmItemRec, *DmItemPtr;

/* macros to access members of DmItemRec */
#define ITEM_LABEL(item)	( (char *)	(item)->label )
#define ITEM_X(item)		( (Position)	(item)->x )
#define ITEM_Y(item)		( (Position)	(item)->y )
#define ITEM_WIDTH(item)	( (Dimension)	(item)->icon_width )
#define ITEM_HEIGHT(item)	( (Dimension)	(item)->icon_height )
#define ITEM_MANAGED(item)	( (Boolean)	(item)->managed )
#define ITEM_SELECT(item)	( (Boolean)	(item)->select )
#define ITEM_BUSY(item)		( (Boolean)	(item)->busy )
#define ITEM_CLIENT(item)	(		(item)->client_data )
#define ITEM_OBJ(item)		( (DmObjectPtr)	(item)->object_ptr )

#define ITEM_OP(item)		ITEM_OBJ(item)
#define OBJECT_PTR(item)	ITEM_OBJ(item)

/* Macros to access fields given item_data from FIconBox call data */
#define ITEM_CD(ID)		( (DmItemPtr)((ID).items) + (ID).item_index )
#define OBJECT_CD(ID)		( (DmObjectPtr)ITEM_OBJ(ITEM_CD(ID)) )

/* Macros to access DmObjectPtr object_ptr fields given an item pointer */
#define FILEINFO_PTR(item)	( (DmFileInfoPtr)ITEM_OBJ(item)->objectdata )
#define FCLASS_PTR(item)	( (DmFclassPtr)ITEM_OBJ(item)->fcp )
#define OBJ_IS_DIR(op)		( (op)->ftype == DM_FTYPE_DIR )
#define ITEM_IS_DIR(item)	( ITEM_OBJ(item)->ftype == DM_FTYPE_DIR )
#define ITEM_OBJ_NAME(item)	( ITEM_OBJ(item)->name )

/* Macros to access GlyphPtr fields given an item pointer */
#define GLYPH_PTR(item)		( (DmGlyphPtr)FCLASS_PTR(item)->glyph )
#define SMALLICON_PTR(item)	( (DmGlyphPtr)FCLASS_PTR(item)->small_icon )

/*
 * This structure is used by the generic DmPropertyEventHandler()
 */
typedef struct {
	Atom		prop_name;	/* queue name */
	DtMsgInfo const	*msgtypes;	/* list of msg types */
	int		(**proc_list)();/* list of procs */
	int		count;		/* # of msg types */
} DmPropEventData;

/* constants for caching types */
#define DM_CACHE_DND		1
#define DM_CACHE_PIXMAP		2
#define DM_CACHE_FOLDER		3
#define DM_CACHE_HELPFILE	4

/* options for DmAddObjToIcontainer() and DmCreateIconContainer() */
#define DM_B_CALC_SIZE		(1 << 0)
#define DM_B_CALC_POS		(1 << 1)
#define DM_B_SPECIAL_NAME	(1 << 2)
#define DM_B_NO_INIT		(1 << 3)
#define DM_B_CHECK_DUP		(1 << 4)
#define DM_B_ADD_TO_END		(1 << 5)

/* options for DmCreatePromptBox() */
#define DM_B_SHELL_CREATED	(1 << 0)

/* options used in DmContainerRec */
#define DM_B_NO_INFO		(1 << 0)
#define DM_B_FLUSH_DATA		(1 << 1)
#define DM_B_INITED		(1 << 2)

/* options used in DmObjectRec */
#define DM_B_HIDDEN		(1 << 0)
#define DM_B_SHORTCUT		(1 << 1)
#define DM_B_SYMLINK		(1 << 2)

/* options in DmFclassRec structure */
#define DM_B_VAR		(1 << 0)
#define DM_B_FREE		(1 << 1)

/* common property names */
#define ICONFILE		"_ICONFILE"
#define FILETYPE		"_FILETYPE"
#define FILEPATH		"_FILEPATH"
#define DFLTICONFILE		"_DFLTICONFILE"
#define OPENCMD			"_Open"
#define PRINTCMD		"_Print"
#define DROPCMD			"_DROP"

#define _DEFAULT_PRINTER	"_DEFAULT_PRINTER"

/* instance property names */
#define ICONLABEL		"_ICONLABEL"

/* size of the global arg list */
#define ARGLIST_SIZE	16

/* global variables */
extern char Dm__buffer[PATH_MAX];
extern Arg  Dm__arg[ARGLIST_SIZE];

#define ICON_PADDING	1
#define ICON_MARGIN	4	/* Margin (in points) around icons in pane */
#define INTER_ICON_PAD	3	/* Space between icons (in points) */
#define ICON_LABEL_GAP	2	/* Space between icon and label (in points) */

/* Define a value to indicate an "unspecified" position.  Some large-enough
   negative number is needed but not large enough to make it valid.
*/
#define UNSPECIFIED_POS -1000

/* MACROS */
#define DmMakePath(path, name)	Dm_MakePath(path, name, Dm__buffer)
#define Dm_ObjPath(obj, buf)	Dm_MakePath((obj)->container->path, \
					    (obj)->name, buf)
#define DmObjPath(obj)		Dm_ObjPath(obj, Dm__buffer)
#define DmMakeDir(path, name)	dirname(Dm__MakePath(path, name))
#define DmObjDir(obj)		dirname(Dm__ObjPath(obj))
#define Dm__MakePath		DmMakePath
#define Dm__MakeDir		DmMakeDir
#define Dm__ObjPath		DmObjPath
#define Dm__ObjDir		DmObjDir


#define VertJustifyInGrid(y, item_height, grid_height, delta) \
    ( ((delta = grid_height - item_height) > 0) ? y + delta : y )

/* Convenience macro for getting text width (should be in OpenLook.h) */
#define DM_TextWidth(font, fontlist, str, cnt) \
    ( ((fontlist) == NULL) ? \
     XTextWidth(font, (char *)(str), cnt) : \
     OlTextWidth(fontlist, (unsigned char *)(str), cnt) ) \

/* Convenience macros we all use somewhere */
#define DM_Max(x, y)		( ((x) > (y)) ? (x) : (y) )
#define DM_Min(x, y)		( ((x) < (y)) ? (x) : (y) )
#define DM_AssignMax(x, y)	if ((y) > (x)) x = (y)
#define DM_AssignMin(x, y)	if ((y) < (x)) x = (y)
     
/* function prototypes */
extern int	Dm__GetFreeItems(DmItemPtr * items, Cardinal * num_items,
				 Cardinal need_cnt, DmItemPtr * ret_item);

extern char *	DmGetObjectName(DmObjectPtr op);
extern void	Dm__vaprtwarning(const char *format,...);
extern void	Dm__vaprterror(const char *format,...);
extern DmGlyphPtr DmGetPixmap(Screen *screen, char *name);
extern DmGlyphPtr DmGetCursor(Screen *screen, char *name);
extern void	DmReleasePixmap(Screen *screen, DmGlyphPtr gp);
extern char *	Dm_MakePath(char * path, char * name, char * buf);
extern char *	Dm__expand_sh(char *, char * (*)(), XtPointer);
extern void	DmRefreshIcon(Widget flat, DmItemPtr ip);
extern int	Dm__ObjectToIndex(Widget flat, DmObjectPtr op);
extern int	Dm__ItemToIndex(Widget flat, DmItemPtr ip);
extern void	DmInitObjType(Widget w, DmObjectPtr op);
extern void	DmSizeIcon(DmItemPtr, OlFontList *, XFontStruct *);
extern int	Dm__ItemNameToIndex(DmItemPtr ip, int nitems, char *name);
extern char	*DmGetObjProperty(DmObjectPtr op, char *name, DtAttrs *attrs);
extern void	DmSetObjProperty(DmObjectPtr op,
			      char *name,
			      char *value,
			      DtAttrs attrs);
extern DtPropPtr DmFindObjProperty(DmObjectPtr op, DtAttrs attrs);
extern DmFclassPtr DmNewFileClass(void *key);
extern void DmDelFileClass(Screen *scrn, DmFclassPtr *list, DmFclassPtr fcp);
extern int Dm__strnicmp(const char *str1, const char *str2, int len);
extern int Dm__stricmp(const char *str1, const char *str2);
extern char *CvtToRegularExpression(char *expression);
extern void DmSetIconPath(char *path);
extern DmGlyphPtr DmCreateBitmapFromData(Screen *screen,
					 char *name,
					 unsigned char *data,
					 unsigned int width,
					 unsigned int height);
extern void DmDrawIcon( Widget w, XtPointer client_data, XtPointer call_data);
extern void DmDrawIconGlyph(Widget w,
			    GC gc,
			    DmGlyphPtr gp,
			    OlgAttrs *attrs,
			    int x,
			    int y,
			    unsigned attr_flags);
extern Widget DmCreateIconContainer(Widget parent,
				    DtAttrs attrs,
				    ArgList args,
				    Cardinal num_args,
				    DmObjectPtr objp,
				    Cardinal num_objs,
				    DmItemPtr *itemp,
				    Cardinal num_items,
				    Widget *swin,
				    OlFontList *fontlist,
				    XFontStruct *font,
				    int charheight);

extern void Dm__AddToObjList(DmContainerPtr, DmObjectPtr, DtAttrs);

#define DmAddObjectToIconContainer(w, items, num_items, cp, obj, x, y, attrs, fontlist, font, charheight) \
    Dm__AddObjToIcontainer(w, items, num_items, cp, op, x, y, \
			      attrs, fontlist, font, 0, 0, 0)

extern Cardinal Dm__AddObjToIcontainer(
		Widget,			/* FIconBox widget */
		DmItemPtr *,		/* ptr to items array */
		Cardinal *,		/* ptr to num_items */
		DmContainerPtr,		/* ptr to container */
		DmObjectPtr,		/* ptr to obj to be added */
		Position, Position,	/* x,y (or DM_UNSPECIFIED_POS) */
		DtAttrs,		/* options */
		OlFontList *,		/* font list (for sizing) */
		XFontStruct *,		/* font (for sizing) */
		Dimension,		/* wrap width (for positioning) */
		Dimension,		/* grid width (for positioning) */
		Dimension);		/* grid height (for positioning) */
			       
extern Pixmap Dm__CreateIconMask(Screen *screen, DmGlyphPtr gp);
extern void DmCreateIconCursor(Widget, XtPointer, XtPointer);
extern void DmRegisterReqProcs(Widget w, DmPropEventData *edp);
extern int DmDispatchRequest(Widget w, Atom selection, char *str);
extern void DmIconAdjustProc(Widget, XtPointer, XtPointer);
extern void DmIconSelect1Proc(Widget, XtPointer, XtPointer);
extern void DmDestroyIconContainer(Widget shell, Widget w,
				   DmItemPtr ilist, int nitems);
extern void DmUpdateDnDRect();
extern Boolean DmIntersectItems(DmItemPtr item, Cardinal num_items,
				int x, int y, int w, int h);
extern void DmGetAvailIconPos(DmItemPtr,	/* items */
			      Cardinal,		/* num_items */
			      Dimension,	/* item width */
			      Dimension,	/* item height */
			      Dimension,	/* wrap width */
			      Dimension,	/* grid width */
			      Dimension,	/* grid height */
			      Position *,	/* x (returned) */
			      Position *);	/* y (returned) */

/* debugging macros */
#define CHKPT()		fprintf(stderr,"checkpoint in file %s line=%d\n", __FILE__, __LINE__)
#define MEMCHK()	{ char *__p__; CHKPT();__p__ = malloc(4); free(__p__); }
#define MEMCHECK(SIZE)	{ char *__p__; CHKPT();__p__ = malloc(SIZE); free(__p__); }

/* desktop administration routines, available for Finder, other clients */

/*	diagnostic flags for diskettes	*/

#define	DTAM_UNDIAGNOSED	0
#define	DTAM_S5_FILES		1
#define	DTAM_UFS_FILES		2
#define	DTAM_FS_TYPE		(1<<3)-1	/* up to 7 file system types */
#define	DTAM_BACKUP		1<<3
#define	DTAM_CPIO		2<<3
#define	DTAM_CPIO_BINARY	3<<3
#define	DTAM_CPIO_ODH_C		4<<3
#define	DTAM_TAR		5<<3
#define	DTAM_CUSTOM		6<<3	/* TAR with file in /etc/perms */
#define	DTAM_DOS_DISK		7<<3
#define	DTAM_UNFORMATTED	8<<3
#define	DTAM_NO_DISK		9<<3
#define	DTAM_UNREADABLE		10<<3
#define	DTAM_BAD_ARGUMENT	11<<3
#define	DTAM_BAD_DEVICE		12<<3
#define	DTAM_DEV_BUSY		13<<3
#define	DTAM_UNKNOWN		1<<8
/*
 *	the two following values are bit-flags that may be or-ed with the above
 */
#define	DTAM_PACKAGE		1<<9	/* can be cpio or File system format */
#define	DTAM_INSTALL		1<<10	/* can be cpio or File system format */

#define	DTAM_UNMOUNTED		0
#define	DTAM_MOUNTED		1<<10
#define	DTAM_MIS_MOUNT		1<<11
#define	DTAM_CANT_MOUNT		1<<12
#define	DTAM_CANT_OPEN		1<<13
#define	DTAM_NOT_OWNER		1<<14
#define	DTAM_NO_ROOM		1<<15
#define	DTAM_READ_ONLY		1<<16
#define	DTAM_TFLOPPY		1<<17
#define	DTAM_FIRST		1
#define	DTAM_NEXT		0

extern	char	*DtamGetDev(		/* find a line in /etc/device.tab */
			char *		/* pattern in device.tab to match */,
			int		/* first or next match in table. */
		);

extern	char	*DtamDevAttr(		/* find an attribute in a devtab line */
			char *		/* table entry (one line to search) */,
			char *		/* attribute (e.g. mountpt) to find */
		);

extern	char	*DtamDevAlias(		/* "internationalized alias attribute */
			char *		/* table entry (one line to search) */
		);

extern	char	*DtamMapAlias(		/* i18n "alias" -> real devtab alias */
			char *		/* table entry (one line to search) */
		);

extern	char	*DtamDevDesc(		/* "internationalized desc attribute */
			char *		/* table entry (one line to search) */
		);

extern	char	*DtamGetAlias(		/* DtamGetDev + DtamDevAlias */
			char *		/* pattern in device.tab to match */,
			int		/* first or next match in table. */
		);

extern	int	DtamCheckDevice(	/* device diagnostic function */
			char *		/* alias of device to diagnose */
		);

extern	int	DtamMountDev(
			char *		/* alias of device to mount */,
			char **		/* mount point: char * value returned */
		);

extern	int	DtamUnMount(
			char *		/* mount point */
		);

extern	char	*DtamGetTxt(
			char *		/* input string, possibly I18N-ized */
		);

#endif /* __DtI_h_ */

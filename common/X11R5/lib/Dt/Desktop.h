/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef __Desktop_h__
#define __Desktop_h__

/* ident	"@(#)Dt:Desktop.h	1.24" */

/*
 **************************************************************************
 *
 * Description:
 *              This file contains all the things necessary to use the
 *       Desktop API.
 *
 **************************************************************************
 */

/* general typedefs */
typedef unsigned long DtAttrs;

/* property related */
#include <Dt/Property.h>

/* request and reply messages */
#include <Dt/DtMsg.h>

/* drag&drop */
#include <X11/Intrinsic.h>
#include <DnD/OlDnDVCX.h>

/* version */
#define DT_VERSION	0

/* help types for help requests */
#define DT_STRING_HELP		0
#define DT_SECTION_HELP		1
#define DT_TOC_HELP			2
#define DT_OPEN_HELPDESK		3

/* Drag&Drop hint */
#define DT_COPY_OP		0
#define DT_MOVE_OP		1
#define DT_LINK_OP		2

/* options for DtNewDNDTransaction() */
#define DT_B_SEND_EVENT		(1 << 0)
#define DT_B_STATIC_LIST	(1 << 1)

/* properties */
#define _DT_QUEUE(D)		XInternAtom((D), "_DT_QUEUE", False)
#define _DT_REPLY(D)		XInternAtom((D), "_DT_REPLY", False)
#define _WB_QUEUE(D)		XInternAtom((D), "_WB_QUEUE", False)
#define _WB_REPLY(D)		XInternAtom((D), "_WB_REPLY", False)
#define _HELP_QUEUE(D)		XInternAtom((D), "_HELP_QUEUE", False)
#define _HELP_REPLY(D)		XInternAtom((D), "_HELP_REPLY", False)


/*
 * This structure is used as the call data structure for DtGetFileNames().
 */
typedef struct {
	char		**files;	/* accumulated list of file names */
	DtAttrs		attrs;		/* attributes */
	Boolean		error;		/* error */
	int		nitems;		/* # of source items */
	Time		timestamp;	/* timestamp of trigger message */
	Boolean		send_done;	/* send_done from trigger message */
} DtDnDInfo, *DtDnDInfoPtr;

/*
 * This structure is used as the call data structure for DtNewDnDTransaction().
 */
typedef struct {
	char		**files;	/* accumulated list of file names */
	DtAttrs		attrs;		/* attributes */
	Widget		widget;		/* owner widget */
	Atom		selection;	/* DnD transaction ID */
	char		**fp;		/* current file name */
	Time		drop_timestamp;	/* timestamp of drop action */
	int		hint;		/* DnD hint */
	int		state;		/* transaction state */
} DtDnDSend, *DtDnDSendPtr;

/**********************************************/
/**********    FUNCTION PROTOTYPES     ********/
/**********************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* initialization function protoypes */
extern void DtInitialize(Widget w);

/* property related function prototypes */
extern char *DtGetProperty(DtPropListPtr plistp, char *name, DtAttrs *attrs);
extern char *DtSetProperty(DtPropListPtr plistp, char *name, char *value,
			   DtAttrs attrs);
extern char *DtAddProperty(DtPropListPtr plistp, char *name, char *value,
			   DtAttrs attrs);
extern void DtFreePropertyList(DtPropListPtr plistp);
extern DtPropListPtr DtCopyPropertyList(DtPropListPtr dst, DtPropListPtr src);
extern DtPropPtr DtFindProperty(DtPropListPtr plistp, DtAttrs attrs);
extern int DtStrToPropertyList(char *str, DtPropListPtr plistp);
extern int DtPropertyListSize(DtPropListPtr plistp);
extern char *DtPropertyListToStr(char *buff, DtPropListPtr plistp);
extern char *DtAttrToString(DtAttrs attrs, char *buff);

/* cache related function prototypes */
extern int DtPutData(Screen *scrn, long type, void *id, int id_len, void *data);
extern void *DtGetData(Screen *scrn, long type, void *id, int id_len);
extern int DtDelData(Screen *scrn, long type, void *id, int id_len);

/* general utility function prototypes	*/
#define DtGetAppId(dpy,id)	XGetSelectionOwner(dpy,id)
extern Window	DtSetAppId(Display *, Window, char *);

extern void	DtChangeProcessIcon(Widget, Pixmap, Pixmap);
extern Widget	DtCreateProcessIcon(Widget, ArgList, Cardinal,
				    ArgList, Cardinal,
				    ArgList, Cardinal);


/* drag&drop routines */
extern DtDnDInfoPtr DtGetFileNames(Widget	w,
				   Atom		selection,
				   Time		timestamp,
				   Boolean	send_done,
				   void		(*proc)(),
				   XtPointer	client_data);

extern DtDnDSendPtr DtNewDnDTransaction(
			Widget				w,
			char				**files,
			DtAttrs				attrs,
			Position			root_x,
			Position			root_y,
			Time				drop_timestamp,
			Window				dst_win,
			int				hint,
			XtCallbackProc			del_proc,
			XtCallbackProc			state_proc,
			XtPointer			client_data);

#ifdef __cplusplus
}
#endif

#endif /* __Desktop_h__ */



/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _wb_h
#define _wb_h

#pragma ident	"@(#)dtm:wb.h	1.74"

#include <time.h>
#include <DtI.h>

#define VERSION     "_VERSION"
#define TIME_STAMP  "_TIMESTAMP"

/* default values for wastebasket timer settings */
#define	DEFAULT_WB_TIMER_INTERVAL   7
#define	DEFAULT_WB_TIMER_UNIT       2
#define	DEFAULT_WB_CLEANUP          0
#define	DEFAULT_WB_SUSPEND          False

/* resources for wastebasket properties */
#define	XtNwbSuspend		"wbSuspend"
#define	XtNwbCleanUpMethod	"wbCleanUpMethod"
#define	XtNwbTimerInterval	"wbTimerInterval"
#define	XtNwbTimerUnit		"wbTimerUnit"

#define	FTIMEUNIT (86400000)
#define	DAYUNIT   (86400000)
#define	HOURUNIT  (3600000)
#define	MINUNIT   (60000)
#define	NUM_ALLOC 10

/* macros */
#define WB_BY_TIMER(w)    (w->cleanUpMethod == 0)
#define WB_ON_EXIT(w)     (w->cleanUpMethod == 1)
#define WB_IMMEDIATELY(w) (w->cleanUpMethod == 2)
#define WB_NEVER(w)       (w->cleanUpMethod == 3)

enum WB_OPS {
	DM_PUTBACK,
	DM_WBDELETE,
	DM_EMPTY,
	DM_DROP,
	DM_TIMER,
	DM_MOVETOWB,
	DM_MOVEFRWB,
	DM_IMMEDDELETE,
	DM_TIMERCHG
};

enum CleanUp_Methods {
	WBByTimer,
	WBOnExit,
	WBImmediately,
	WBNever
};


typedef struct {
	DmGlyphPtr    fgp;
	DmGlyphPtr    egp;
	XtIntervalId  timer_id;
	char          *time_str;
	Boolean       suspend;
	Boolean       restart;
	int           cleanUpMethod;
	int           interval;
	time_t        tm_start;
	unsigned int  unit_idx;
	unsigned long time_unit;
	unsigned long tm_remain;
	unsigned long tm_interval;
	Widget        icon_shell;
	DmFclassPtr   fcp;
} DmWbDataRec, *DmWbDataPtr;

typedef struct {
	Screen         *screen;
	DmItemPtr      itp;
	long           serial;
	int            op_type;
	Window         client;
	Atom           replyq;
	unsigned short version;
} DmWBMoveFileReqRec, *DmWBMoveFileReqPtr;

typedef struct {
	int       op_type;
	DmItemPtr itp;
	char      *target;
} DmWBCPDataRec, *DmWBCPDataPtr;

/* external variables */
extern DmWbDataPtr wbdp;
extern char *wb_dtinfo;

extern void	DmWBTimerProc(XtPointer client_data, XtIntervalId timer_id);
extern void	DmWBEMPutBackCB(Widget, XtPointer, XtPointer);
extern void	DmWBEMFilePropCB(Widget, XtPointer, XtPointer);
extern void	DmWBIMFilePropCB(Widget, XtPointer, XtPointer);
extern void	DmWBIMPutBackCB(Widget, XtPointer, XtPointer);
extern void	DmWBEMDeleteCB(Widget, XtPointer, XtPointer);
extern void	DmWBIMDeleteCB(Widget, XtPointer, XtPointer);
extern void	DmWBPropCB(Widget, XtPointer, XtPointer);
extern void	DmWBHelpCB(Widget, XtPointer, XtPointer);
extern void	DmWBPropHelpCB(Widget, XtPointer, XtPointer);
extern void	DmWBHelpTOCCB(Widget, XtPointer, XtPointer);
extern void	DmWBHelpDeskCB(Widget, XtPointer, XtPointer);
extern void	DmConfirmEmptyCB(Widget, XtPointer, XtPointer);
extern void	DmWBDelete(char ** src_list, int src_cnt, DmWBCPDataPtr cpdp);
extern int	DmWBGetVersion(DmItemPtr itp, char *fname);
extern void	DmSwitchWBIcon();
extern void	DmWBRestartTimer();
extern void	DmLabelWBFiles();
extern void	DmEmptyWB(Widget, XtPointer, XtPointer);
extern void	DmWBDropProc(Widget, XtPointer, XtPointer);
extern void	DmWBTimerCB(Widget, XtPointer, XtPointer);
extern void	DmWBClientProc(DmProcReason, XtPointer, XtPointer,
				char *, char *);
extern void	DmGetWBPixmaps();
extern void	DmCreateWBIconShell(Widget toplevel, Boolean iconic);
extern void	DmWBToggleTimerBtn(char *label, char mnemonic, Boolean sensitive);
extern void	DmWBResumeTimer();
extern void	DmWBSuspendTimer();
extern DmFileInfoPtr WBGetFileInfo(char *path);

#endif /* _wb.h */

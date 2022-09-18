/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtmail:aliasshow.c	1.13"
#endif

#define ALIASSHOW_C

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <Xol/Error.h>
#include <Xol/Form.h>
#include <Xol/Flat.h>
#include <Xol/FList.h>
#include <Xol/ControlAre.h>
#include <Xol/StaticText.h>
#include <Xol/RubberTile.h>
#include <X11/Shell.h>			/* need this for XtNtitle */
#include <sys/types.h>			/* need this for XtNtitle */
#include <stdio.h>
#include "mail.h"
#include <Gizmo/Gizmos.h>
#include <Gizmo/LabelGizmo.h>
#include "RMailGizmo.h"
#include "SendGizmo.h"
#include "ListGizmo.h"

/* Functions defined in this module */

void 				AliasShowPopupCB();
void 				AliasShowPrep();
void 				AliasShowSelectCB();
void 				AliasShowUnselectCB();
char *				ExpandAlias(char *);
void 				AliasDropProcCB();

extern HelpInfo				AliasesHelp;
extern int					Version;
extern Widget				Root;
extern Boolean				SendOnly;
extern MailRec *			mailRec;
extern AliasRec *			aliasRec;
extern BaseWindowGizmo  	MainWindow;
static ListBuf *			nameList = NULL;
extern Pixmap				UserAliasPixmap;
extern Pixmap				SysAliasPixmap;
extern Pixmap				UserGroupPixmap;
extern Pixmap				SysGroupPixmap;

#ifdef UseXtApp
extern XtAppContext app_con;
	/* must get application context from main - not done yet */
#endif

ListHead		nameShow;
ListHead *		showList = (ListHead *)&nameShow;

static Setting aliasShowSetting = {
	"Header",
	(XtPointer)NULL,
	(XtPointer)NULL,
	(XtPointer)"x"
 };

ListGizmo aliasShow = {
    NULL,                   /* help info */
    "aliasShow",            /* name of widget */
    "",                     /* caption */
    &aliasShowSetting,      /* settings */
    "%b %18w",        		/* format */
    FALSE,                  /* exclusive */
    LISTHEIGHT,             /* height */
    NULL,                   /* font: default */
    NULL,                   /* list itself to be set later */
    AliasShowSelectCB,      /* execute (double click) same as select */
    AliasShowSelectCB,
    AliasShowUnselectCB,
};
ListGizmo *             aliasShowP = &aliasShow;

static GizmoRec showArray[] = {
	{ListGizmoClass,	&aliasShow},
};

static LabelGizmo showContainer = {
	NULL, 
	"showContainer", 
	TXT_NAMES, 
	showArray, 
	XtNumber (showArray),
	OL_FIXEDROWS, 
	1, 
	0, 
	0, 
	True
};

static GizmoRec AliasShowGiz[] = {
	{LabelGizmoClass,	&showContainer},
};

static MenuItems aliasShowMenuItems[] = {
	{True, BUT_CANCEL,		MNEM_CANCEL, NULL, CancelCB},
	{True, BUT_HELP,		MNEM_HELP,   NULL, HelpCB,(XtPointer)HelpAliases},
	{NULL, NULL,			(XtArgVal)NULL}
};

static MenuGizmo aliasShowMenu = {
	NULL,				/* Help		*/
	"",					/* Name		*/
	"aliasShowMenu",	/* Title	*/
	aliasShowMenuItems,	/* Items	*/
	0,					/* Function	*/
	NULL,				/* Client data	*/
	CMD,				/* Button type	*/
	OL_FIXEDROWS,		/* Layout type	*/
	1,					/* Measure	*/
	0					/* Default item	*/
};

static PopupGizmo *	aliasShowPopup = (PopupGizmo *)0;

static PopupGizmo	AliasShowPopup = {
	&AliasesHelp,
	"aliasShow",
	TXT_E_MAIL_ALIASES,
	&aliasShowMenu,
	AliasShowGiz,
	XtNumber (AliasShowGiz),
};


void
AliasShowPopupCB(wid, clientData, callData)
Widget wid;
XtPointer clientData;
XtPointer callData;
{
Arg			arg[6];

	AliasShowPrep();

	if (aliasShowPopup == (PopupGizmo *)0) {
		aliasShowPopup = &AliasShowPopup;
		CreateGizmo (
			GetBaseWindowShell (&MainWindow),
			PopupGizmoClass,
			aliasShowPopup,
			NULL, 0
		);
	}
	/* Make it show (MakeItSo); make the change take effect */
	aliasRec->aliasShowWidget = (Widget) QueryGizmo (
			ListGizmoClass, 
			aliasShowP, 
			GetGizmoWidget,
			"aliasShow"
	);
	/***  fprintf (stderr, "AliasShowPopupCB: aliasRec->aliasShowWidget <%#x>, size %d\n",aliasRec->aliasShowWidget,showList->size); /***/

	XtSetArg (arg[0], XtNviewHeight,	(XtArgVal) LISTHEIGHT);
	XtSetArg (arg[1], XtNdropProc,    	AliasDropProcCB);
	XtSetArg (arg[2], XtNnumItems,    	showList->size);
	XtSetArg (arg[3], XtNitemsTouched,	True);

	XtSetValues (aliasRec->aliasShowWidget, arg, 4);

	MapGizmo (BaseWindowGizmoClass, &MainWindow);
	MapGizmo (PopupGizmoClass, aliasShowPopup);
}



void
AliasShowPrep()
{
int			i;
ListHead *	scrollList;

	aliasShow.list = showList;	/* point the gizmo to the data */
	scrollList = mailRec->alias;

	if (nameList == NULL)
	{
		/***  fprintf (stderr, "AliasShowPrep: nameList is NULL\n"); /***/
		nameList = (ListBuf *) 
				AllocateBuffer (sizeof(ListItem), aliasRec->name->used +GROWTH);
	}
	showList->items = (ListItem *) nameList->p;
	/***  fprintf (stderr, "AliasShowPrep: nameList <%#x>, showList->items <%#x>\n",nameList,showList->items); /***/
	showList->numFields = 2;
	showList->size = aliasRec->name->used;
	/***  fprintf (stderr, "AliasShowPrep: showList->numFields %d, showList->size %d\n",showList->numFields,showList->size); /***/

	for (nameList->used=1, i=0; i < aliasRec->name->used; i++, nameList->used++)
	{
		XtArgVal * tmpAP;
		XtArgVal * tmpP = (XtArgVal *)
							MALLOC(showList->numFields*sizeof(XtArgVal));
		tmpAP = (XtArgVal *) scrollList->items[i].fields;
		tmpP[0] = (XtArgVal) tmpAP[0];	/* copy bitmap from master alias list */
		tmpP[1] = (XtArgVal) aliasRec->name->p[i];
		showList->items[i].fields = (XtArgVal) tmpP;
		showList->items[i].set = (XtArgVal) False;
		if (BufferFilled(nameList))
		{
			GrowBuffer ((Buffer *) nameList, GROWTH);
			showList->items = (ListItem *) nameList->p;	/* reset list pointer */
		}
	}
	/***  fprintf (stderr, "AliasShowPrep, done: aliasRec->name->used %d, aliasRec->name->p[0] <%s>, fields <%#x>\n",aliasRec->name->used,aliasRec->name->p[0],showList->items[0].fields); /***/
}


void
AliasShowSelectCB(wid, clientData, callData)
Widget wid;
XtArgVal clientData;
OlFlatCallData * callData;
{
/*	int         itemNum = callData->item_index;
	showList->items[itemNum].set = True;
	return;
*/
}



void
AliasShowUnselectCB(wid, clientData, callData)
Widget wid;
XtArgVal clientData;
OlFlatCallData * callData;
{
/*
	int         itemNum = callData->item_index;
	showList->items[itemNum].set = False;
	return;
*/
}



/* ExpandAlias takes a name or set of names and provides the "final"  */
/* addresses according to this user's mailrc and system mailrc files. */
extern char *
ExpandAlias(char * name)
{
char *	addrSys;
char *	addrAll;

	addrSys = GenerateAliasTable (name, aliasRec->systemAlias, True);
	addrAll = GenerateAliasTable (addrSys, aliasRec->userAlias, True);
	FREE (addrSys);
	return (addrAll);

}


void
AliasDropProcCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
OlFlatDropCallData *call_data;
{
	int		i = call_data->item_data.item_index;
	OlFlatDropCallData *	p = call_data;
	ManageRec *	mng;
	SendRec *	sp;
	ReadRec *	rp;
	Widget		widget;
	SendGizmo *	gizmo;

	if (p->drop_status != OlDnDDropSucceeded)
	{
	    /***  if (p->drop_status == OlDnDDropFailed) fprintf (stderr, "AliasDropProcCB: drop Failed \n"); /***/
	    /***  else if (p->drop_status == OlDnDDropCanceled) fprintf (stderr, "AliasDropProcCB: drop Canceled \n"); /***/
	    /***  else fprintf (stderr, "AliasDropProcCB: bad value <%#x>\n",p->drop_status); /***/
	    return;
	}
	/***  fprintf(stderr, "AliasDropProcCB: p->dst_info: <%#x>, p->dst_info->window: <%#x>\n", p->dst_info,p->dst_info->window); /***/

	widget = XtWindowToWidget (XtDisplay (Root), (p->dst_info)->window);

	if (widget == (Widget)0)
	{
		/* DroppedOutsideDtmail (wid, p); ---  don't care */
		return;
	}
	if ((sp = FindSendRec (widget)) != (SendRec *)0)
	{
		/***  fprintf (stderr, "Dropped on send window\n"); /***/
		gizmo = (SendGizmo *) QueryGizmo (BaseWindowGizmoClass,
					sp->baseWGizmo, GetGizmoGizmo, SEND);
		
		/***  fprintf (stderr, "widget <%#x>, gizmo <%#x>, to <%#x>, cc <%#x>, bcc <%#x>\n",widget,gizmo,gizmo->to,gizmo->cc,gizmo->bcc); /***/
		if ((widget ==gizmo->to)||(widget ==gizmo->cc)||(widget ==gizmo->bcc))
		{
			char * moveNames = MALLOC (BUF_SIZE);
			int len = BUF_SIZE;

			*moveNames = 0;
			/***  fprintf (stderr, "AliasDropProcCB: aliasRec->name->used %d\n",aliasRec->name->used); /***/
			for (i = 0; i < aliasRec->name->used; i++)
			{
				XtArgVal * tmpP[2];
				tmpP[1] = (XtArgVal *) aliasRec->name->p[i];
				/***  fprintf (stderr, "AliasDropProcCB: i %d, name <%s>, set %d\n",i,tmpP[1],showList->items[i].set); /***/
				if (showList->items[i].set)
				{
					if ((int)(strlen(moveNames) + strlen(aliasRec->name->p[i]) 
															+ 5) > len)
					{
						moveNames = REALLOC (moveNames, len + BUF_SIZE);
						len += BUF_SIZE;
						/***  fprintf (stderr, "AliasDropProcCB: i %d, string <%s>, len %d\n",i,moveNames,strlen(moveNames)); /***/
					}
					strcat (moveNames, " ");
					strcat (moveNames, aliasRec->name->p[i]);
				}
			}
			strcat (moveNames, " ");
			/***  fprintf (stderr, "AliasDropProcCB: string <%s>, len %d\n",moveNames,strlen(moveNames)); /***/
			/* later check names and clean up blanks appropriately */
			OlTextEditInsert (widget, moveNames, strlen (moveNames));
			FREE (moveNames);
		}
		return;
	}
}

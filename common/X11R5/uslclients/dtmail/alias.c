/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtmail:alias.c	1.19"
#endif

#define ALIAS_C

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <Xol/Error.h>
#include <Xol/Form.h>
#include <Xol/FList.h>
#include <Xol/ControlAre.h>
#include <Xol/StaticText.h>
#include <Xol/RubberTile.h>
#include <X11/Shell.h>			/* need this for XtNtitle */
#include "mail.h"
#include "RMailGizmo.h"
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/STextGizmo.h>
#include <Gizmo/LabelGizmo.h>
#include <Gizmo/InputGizmo.h>
#include <Gizmo/SpaceGizmo.h>
#include "useralias.16"
#include "usergroup.16"
#include "sysalias.16"
#include "sysgroup.16"

/* Functions defined in this module */
void				AliasSelectCB ();
void				AliasUnselectCB ();
void				NoticeMessage ();
ListGizmo *			GetAliasListGizmo ();
Widget				GetAliasListWidget ();
void				InitAliasScrollingList ();
void				AliasWinInit ();

extern HelpInfo		AliasManagerHelp;
extern int			Version;
extern Widget		Root;
extern MenuGizmo	AliasMenu;
extern Boolean		SendOnly;
extern MailRec *	mailRec;
extern int			position;
AliasRec 			AliasRect;
AliasRec *			aliasRec = &AliasRect;
static ListHead		aliasListHead;
ListHead *			scrollList = (ListHead *)&aliasListHead;
Boolean				OnlyOneQ(char *);

static StaticTextGizmo firstHalf = {
	NULL, "AliasFirstHalf",
	TXT_ALIAS_HEAD_1,
	NorthWestGravity,
	NULL
};

static StaticTextGizmo secondHalf = {
	NULL, "AliasSecondHalf",
	TXT_ALIAS_HEAD_2,
	NorthWestGravity,
	NULL
};

static GizmoRec headerHalfs[] = {
	{StaticTextGizmoClass,	&firstHalf},
	{StaticTextGizmoClass,	&secondHalf}
};

static LabelGizmo AliasHeaderLine = {
	NULL, "AliasHeader", "", headerHalfs, XtNumber (headerHalfs),
	OL_FIXEDROWS, 1, 0, 0, True
};


static Setting aliasListSetting = {
	"None",
	(XtPointer)NULL,
	(XtPointer)NULL,
	(XtPointer)"_"
};

static Setting	aliasNameSettings;	/* settings */
static Setting	aliasAddressSettings;	/* settings */
InputGizmo aliasName = {
			NULL,							/* help */	
			"inputName",					/* name of the widget */
			TXT_NAME_COLON,					/* caption label */
			"",								/* initial text */
			(Setting *) &aliasNameSettings,	/* settings */
			16								/* number of characters to show */
};
InputGizmo *	aliasNameP = &aliasName;

InputGizmo aliasAddress = {
			NULL,								/* help */	
			"inputAddress",						/* name of the widget */
			TXT_ADDRESS_COLON,					/* caption label */
			"",									/* initial text */
			(Setting *) &aliasAddressSettings,	/* settings */
			36									/* num characters to show */
};
InputGizmo *	aliasAddressP = &aliasAddress;

static GizmoRec inputAliasArray[] = {
	{InputGizmoClass,	&aliasName},
	{InputGizmoClass,	&aliasAddress}
};

static LabelGizmo inputAliasContainer = {
	NULL,						/* Help info */
	"AliasContainer",			/* name of the shell */
	"",							/* caption label */
	inputAliasArray,			/* list of Gizmos to insert */
	XtNumber(inputAliasArray),	/* number of Gizmos to insert */
	OL_FIXEDROWS,				/* layout type */
	1, 0, 0, True				/* num rows, arg array, num args, align */
};

static MenuItems modalItems[] = {
    {True, BUT_OK,	MNEM_OK,		NULL,	CancelCB},
    {NULL}
};

static MenuGizmo Okay = {
	NULL,			/* help		*/
	"modal",		/* name		*/
	NULL,			/* title	*/
	modalItems,		/* items	*/
	0,				/* function	*/
	NULL,			/* client_data	*/
	CMD,			/* buttonType	*/
	OL_FIXEDROWS,	/* layoutType	*/
	1,				/* measure	*/
	0				/* default Item	*/
};

ModalGizmo modalNotice = {
    NULL,
    "modalNotice",
    TXT_JUST_MAIL,
    &Okay,
    "------------------- Feature Not Yet Implemented -------------------",
};
ModalGizmo *	noticeP = &modalNotice;


static SpaceGizmo vertSpace4mm = {
	4,							/* height */
	0,							/* width */
};

ListGizmo aliasList = {
	NULL,					/* help info */
	ALIAS_LIST,				/* name of widget */
	"",						/* caption */
	&aliasListSetting,		/* settings */
	"%b %18w %32w",			/* format */ 
	True,					/* exclusive */
	LISTHEIGHT,				/* height */
	NULL,					/* font: default */
	NULL,					/* list itself to be set later */
	AliasSelectCB,			/* execute (double click) same as select */
	AliasSelectCB,
	AliasUnselectCB,
};
ListGizmo *				aliasListP = &aliasList;

static GizmoRec aliasListArray[] = {
	{LabelGizmoClass,	&AliasHeaderLine},
	{ListGizmoClass,	&aliasList},
	{SpaceGizmoClass,	&vertSpace4mm},
	{LabelGizmoClass,	&inputAliasContainer}
};

BaseWindowGizmo AliasWindow = {
	&AliasManagerHelp,			/* Help info */
	"AliasWindow",				/* name of the shell */
	NULL,						/* title of widget */
	&AliasMenu,					/* Menubar menu gizmo */
	aliasListArray,				/* list of Gizmos to insert */
	XtNumber (aliasListArray),	/* number of Gizmos to insert */
	TXT_ALIAS_MANAGER_ICON,		/* name when iconic */
	"alias.icon",				/* icon */
	" ",						/* left footer error message */
	" ",						/* right footer error message */
	75,							/* percent of error message in left footer */
};
BaseWindowGizmo *	AliasWindowP = &AliasWindow;

static XtCallbackRec	ProtocolCB[] = {
	{WindowManagerEventHandler, (XtPointer)AliasExitCB},
	{NULL}
};
Arg argList[] = {
	XtNwmProtocol,	(XtArgVal)ProtocolCB
};

/* ---------------------------------------------------------------------- */
Pixmap UserAliasPixmap = (Pixmap)0;
Pixmap SysAliasPixmap;
Pixmap UserGroupPixmap;
Pixmap SysGroupPixmap;

static void
AliasGlyphInit ()
{

	/* Read in bitmaps that show group vs. single // user vs system aliases */
	if (UserAliasPixmap == (Pixmap)0)
	{
		UserAliasPixmap = XCreateBitmapFromData (
			XtDisplay (Root),
			RootWindow (XtDisplay (Root), 0),
			(char *)useralias_bits, useralias_width, useralias_height
		);
		UserGroupPixmap = XCreateBitmapFromData (
			XtDisplay (Root),
			RootWindow (XtDisplay (Root), 0),
			(char *)usergroup_bits, usergroup_width, usergroup_height
		);
		SysAliasPixmap = XCreateBitmapFromData (
			XtDisplay (Root),
			RootWindow (XtDisplay (Root), 0),
			(char *)sysalias_bits, sysalias_width, sysalias_height
		);
		SysGroupPixmap = XCreateBitmapFromData (
			XtDisplay (Root),
			RootWindow (XtDisplay (Root), 0),
			(char *)sysgroup_bits, sysgroup_width, sysgroup_height
		);
	}
}


void
AliasSelectCB (widg, client_data, call_data)
Widget			widg;
XtArgVal		client_data;
OlFlatCallData *	call_data;
{
	int			itemNum = call_data->item_index;
	int			i;
	XtArgVal *	tmpP = (XtArgVal *) (scrollList->items[itemNum]).fields;

	SetBaseWindowMessage (AliasWindowP,"");	/* clear out any footer text */

	XtVaSetValues (
		aliasName.textFieldWidget,
		XtNstring, tmpP[1],
		(String)0
	);
	XtVaSetValues (
		aliasAddress.textFieldWidget,
		XtNstring, tmpP[2],
		(String)0
	);
	position = itemNum;
	/* make the delete button sensitive */
	SetSensitivity (AliasWindowP, "aliasEdit", 3, 3, True);
	/***  fprintf (stderr, "AliasSelectCB: position %d, name <%s>, addr <%s>\n",call_data->item_index, tmpP[1], tmpP[2]); /***/
}

static void
AliasUnselectCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
OlFlatCallData *call_data;
{
	ListHead *  lp = (ListHead *)call_data->user_data;
	int     itemNum = call_data->item_index;

	SetBaseWindowMessage (AliasWindowP,"");	/* clear out any footer text */

	/* make the delete button insensitive */
	SetSensitivity (AliasWindowP,"aliasEdit", 3, 3, False);
	/***  fprintf (stderr, "AliasUnselectCB: widget <%#x>, aliasRec->aliasWidget <%#x>, position %d\n",wid,aliasRec->aliasWidget,position); /***/
}


void
NoticeMessage (mess)
char *	mess;
{
	SetModalGizmoMessage (noticeP, GetGizmoText (mess));
    MapGizmo (ModalGizmoClass, noticeP);
}


/* Double clicking on an item means select the item */



ListGizmo *
GetAliasListGizmo (aliasRec)
AliasRec *	aliasRec;
{
	return (ListGizmo *) QueryGizmo (
		BaseWindowGizmoClass, aliasRec->baseWGizmo,
		GetGizmoGizmo, ALIAS_LIST
	);
}

Widget
GetAliasListWidget (aliasRec)
AliasRec *	aliasRec;
{
	return (Widget) QueryGizmo (
		BaseWindowGizmoClass, aliasRec->baseWGizmo,
		GetGizmoWidget, ALIAS_LIST
	);
}

void
InitAliasScrollingList (firstName, firstAddress)
char ** firstName;
char ** firstAddress;
{
Widget		aliasListWidget;
Widget		menuwidget;
int			i;
char *		aliasBuf; 
char *		oldAliasBuf; 
char *		bufP;
char *		prevP;
char		NL[2] = { 0xa, 0 };
char		TAB[2] = { 0x9, 0 };
char *		namePrev = "";

	aliasList.list = mailRec->alias = scrollList;

	/* populate list with values currently in system and user mailrc files */
	aliasRec->systemMailrc = GetGizmoText (TXT_SYSTEM_MAILRC);
	if ((aliasRec->userMailrc = (char *)getenv(GetGizmoText(TXT_MAILRC_ENV))) 
						== NULL)
	{
		char *	tmp1;
		char *	tmp2;
		tmp1 = (char *) getenv ((char *)"HOME");
		tmp2 = GetGizmoText (TXT_DOT_MAILRC);
		aliasRec->userMailrc = (char *) MALLOC(strlen(tmp1) + strlen(tmp2) + 2);
		sprintf (aliasRec->userMailrc,"%s/%s",tmp1,tmp2);
	}

	aliasRec->userAlias   = ReadAlias (aliasRec->userMailrc);
	aliasRec->systemAlias = ReadAlias (aliasRec->systemMailrc);

	/* allocate number of entries and create the array of strings */
	aliasRec->aliasTotal = 
			aliasRec->userAlias->used + aliasRec->systemAlias->used;	
	aliasRec->name =
		(CharBuf *)AllocateBuffer(sizeof(char *),aliasRec->aliasTotal+GROWTH);
	aliasRec->addr =
		(CharBuf *)AllocateBuffer(sizeof(char *),aliasRec->aliasTotal+GROWTH);

	aliasRec->userCount = 0;
	for (i = 0; i < aliasRec->userAlias->used; i++) /* first get the user aliases */
	{
		if (strcmp(aliasRec->userAlias->p[i].name,namePrev) != 0) /* not a repeat */
		{
			aliasRec->name->p[aliasRec->name->used] = 
				STRDUP (aliasRec->userAlias->p[i].name);
			aliasRec->addr->p[aliasRec->addr->used] = 
				GenerateAliasTable 
					(aliasRec->userAlias->p[i].name,aliasRec->userAlias,False);
			/* don't recurse; just deliver addresses associated with name */

			namePrev = aliasRec->userAlias->p[i].name;
			(aliasRec->name->used)++; (aliasRec->addr->used)++;
			aliasRec->userCount++;
		}
	}
	/* delete aliases whose names are in user list from system alias list */
	for (i = 0; i < aliasRec->name->used; i++)
	{
		int numDeleted = DeleteAlias(aliasRec->name->p[i], aliasRec->systemAlias);
		/***  if (numDeleted > 0) fprintf (stderr, "Deleted system alias <%s>, appeared %d times in systemAlias list\n",aliasRec->name->p[i],numDeleted); /***/
	}
	aliasRec->systemCount = 0;
	for (i = 0; i < aliasRec->systemAlias->used; i++) /* now get the system aliases */
	{
		if (strcmp(aliasRec->systemAlias->p[i].name,namePrev) != 0) /* not a repeat */
		{
			aliasRec->name->p[aliasRec->name->used] = 
					STRDUP (aliasRec->systemAlias->p[i].name);
			aliasRec->addr->p[aliasRec->addr->used] = 
				GenerateAliasTable (aliasRec->systemAlias->p[i].name,
									aliasRec->systemAlias,False);
			/* don't recurse; just deliver addresses associated with name */

			namePrev = aliasRec->systemAlias->p[i].name;
			(aliasRec->name->used)++; (aliasRec->addr->used)++;
			aliasRec->systemCount++;
		}
	}
	aliasRec->aliasTotal = aliasRec->userCount + aliasRec->systemCount;	/* adjust for group alias repeats */
	/***  fprintf (stderr, "Remaining aliases: %d user, %d system, %d total\n",aliasRec->userCount,aliasRec->systemCount,aliasRec->aliasTotal); /***/

	/* place this information into the list data structure */
	aliasRec->list = 
			(ListBuf *) AllocateBuffer (sizeof(ListItem), aliasRec->name->size);
	scrollList -> items = (ListItem *) aliasRec->list->p;
	scrollList -> size = aliasRec->name->used;	/* NOTE: size means num used */
	scrollList -> numFields = 3;		/* glyph; name; address */

	if (aliasRec->name->used > 0)/* are there any? => fill lists with ptrs */
	{
		/* save first name and address for initial highlighting */
		*firstName		= (char *) aliasRec->name->p[0];
		*firstAddress	= (char *) aliasRec->addr->p[0];

		/* now copy all the pointers */
		for (i = 0; i < aliasRec-> name->used; i++)
		{
			XtArgVal * tmpP = (XtArgVal *)
							MALLOC(scrollList->numFields*sizeof(XtArgVal));

			tmpP[1] = (XtArgVal) aliasRec->name->p[i];	/* name */
			tmpP[2] = (XtArgVal) aliasRec->addr->p[i];	/* address */
			/* test for user vs. system; single vs. group */
			tmpP[0] = (XtArgVal) ((i<aliasRec->userCount)?
				(OnlyOneQ((char *)tmpP[2])?UserAliasPixmap:UserGroupPixmap):
				(OnlyOneQ((char *)tmpP[2])?SysAliasPixmap:SysGroupPixmap));

			/* now actually copy the pointer structure */
			scrollList->items[i].fields = (XtArgVal) tmpP;
			scrollList->items[i].set = (XtArgVal) False;
		}
		/* set count in list buffer */
		aliasRec->list->used = aliasRec->name->used;
	}
	else
	{ 	/* no aliases: set initial name and address to null string */
		*firstName		= (char *) "";
		*firstAddress	= (char *) "";
	}
	scrollList->items[0].set = (XtArgVal) True;
}


void AliasWinInit (UserName)
char *		UserName;
{
char		TitleBar[BUF_SIZE];
Widget		shell;
char *		firstName;
char *		firstAddress;
Arg			arg[5];

	/* the following used to be in subroutine /* CreateAliasRec (mailRec); */
	aliasRec = (AliasRec *)CALLOC (1,sizeof(AliasRec));

	aliasRec->baseWGizmo = (BaseWindowGizmo *)0;
	mailRec->aliasRec = aliasRec;	/* let dad know where we are */
	/* the above used to be in subroutine /* CreateAliasRec (mailRec); */

	AliasGlyphInit();	/* initialize the glyphs */

	InitAliasScrollingList (&firstName, &firstAddress);

	/* sprintf (TitleBar, GetGizmoText (TXT_ALIAS_MANAGER), UserName); */
	sprintf (TitleBar, GetGizmoText (TXT_ALIAS_MANAGER));
	firstHalf.font = secondHalf.font = GetGizmoText (TXT_LUCY_BOLD);
	AliasWindow.name = TitleBar;
	AliasWindow.title = TitleBar;
	CreateGizmo ( Root, BaseWindowGizmoClass, &AliasWindow, argList, (int) 1);

	aliasRec->baseWGizmo = &AliasWindow;
	/* Set the weights of the gizmos in the base window */
	XtSetArg (arg[0], XtNweight, 0);
	XtSetArg (arg[1], XtNallowShellResize, False);
	XtSetValues (QueryGizmo(BaseWindowGizmoClass,aliasRec->baseWGizmo,
						GetGizmoWidget, "aliasMenu"), arg, 1);

	XtSetArg (arg[0], XtNstring, (XtArgVal) firstName);
	XtSetValues (aliasName.textFieldWidget, arg, 1);
	
	XtSetArg (arg[0], XtNstring, (XtArgVal) firstAddress);
	XtSetValues (aliasAddress.textFieldWidget, arg, 1);

    shell = GetBaseWindowShell (&AliasWindow);
	XtAddCallback (
		GetBaseWindowShell (&AliasWindow),
		XtNdestroyCallback, AliasExitCB, aliasRec->baseWGizmo
	);

	XtVaSetValues (
		GetBaseWindowShell (&AliasWindow),
		XtNwmProtocolInterested,
		OL_WM_DELETE_WINDOW,
		(String)0
	);

	aliasRec->aliasWidget = GetAliasListWidget (aliasRec);
	XtVaSetValues (
		aliasRec->aliasWidget,
		XtNweight,	1,
		XtNnoneSet,	True,
		XtNuserData,	mailRec->alias,	/* alias list attached to mailRec */
		(String)0
	);
	/***  fprintf (stderr, "AliasWinInit: aliasRec->aliasWidget <%#x>\n", aliasRec->aliasWidget); /***/

	aliasRec->aliasScrollWidget = XtParent(aliasRec->aliasWidget);

	/* create a notice Gizmo */
	CreateGizmo (GetBaseWindowShell (&AliasWindow),
				  ModalGizmoClass, noticeP, 0, 0);

	/* Set initial sensitivities */
	if (aliasRec->aliasTotal > 0)
	  SetSensitivity (AliasWindowP,"aliasEdit", 3, 3, True);  /* Delete */
	else
	  SetSensitivity (AliasWindowP,"aliasEdit", 3, 3, False); /* Delete */
	SetSensitivity (AliasWindowP,"aliasEdit", 4, 4, False); /* Undelete */
	SetSensitivity (AliasWindowP,"aliasEdit", 0, 0, False); /* Undo */
}

Boolean OnlyOneQ(string)
char *		string;
{
char *	charP;
int		i;
	
	charP = string;
	while (*charP == ' ' || *charP == ',')
	{
		if ((int)(charP - string) >= (int) strlen(string)) 
							return (True); /* zero? */
		charP++;
	}
	while (*charP != ' ' && *charP != ',')
	{
		if ((int)(charP - string) >= (int) strlen(string)) 
							return (True); /* one */
		charP++;
	}
	/* found first interior blank; check for other address here */
	while (*charP == ' ' && *charP == ',')
	{
		if ((int)(charP - string) >= (int) strlen(string)) 
							return (True); /* one */
		charP++;
	}
	return (False);	/* two or more words */
}


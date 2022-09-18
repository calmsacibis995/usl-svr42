/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtmail:aliasedit.c	1.19"
#endif

#define ALIASEDIT_C

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
#include <Gizmo/Gizmos.h>
#include <Gizmo/InputGizmo.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/LabelGizmo.h>

/* Functions defined in this module */

void	AliasUndeleteCB ();
void	AliasUndeleteApplyCB ();
void	AliasDeleteCB ();
void	AliasAddCB ();
void	_AliasAddCB ();
void	AliasNewCB ();
void	AliasUndoCB ();
void	AddCancelCB ();
void	AddOverwriteCB ();
void	ApplyAlias ();
void	UndoPrep ();
void	MakeItSo ();
void 	AliasUndeleteCB();
void 	AliasUndeleteCB();
void 	AliasUndeleteSelectCB();
void 	AliasUndeleteUnselectCB();

Widget				aliasUndeleteWidget = 0;

extern HelpInfo		AliasManagerUndeleteHelp;
extern int			Version;
extern Widget		Root;
extern MenuGizmo	AliasMenu;
extern Boolean		SendOnly;
extern MailRec *	mailRec;
extern AliasRec *	aliasRec;
extern BaseWindowGizmo *	AliasWindowP;
extern InputGizmo 	aliasName;
extern InputGizmo 	aliasAddress;

CharBuf *			addrUndeleteBuf;
static char *		legalName = 
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";

Boolean				replace;
int					position;
extern Pixmap		UserAliasPixmap;
extern Pixmap		SysAliasPixmap;
extern Pixmap		UserGroupPixmap;
extern Pixmap		SysGroupPixmap;


typedef enum {
			OpApplyNewAlias,		/* alias was added */
			OpApplyModAlias,		/* alias was changed */
			OpDeleteAlias,			/* alias was deleted from user area */
			OpSysDeleteAlias,		/* alias was deleted from system area */
			OpUndeleteAlias,		/* undelete was last run */
			OpNewAlias,				/* user asked for new alias */
			OpNewNewAlias			/* after undoing new, user wants back */
}	OpPrev;

static char *		nameP;
static char *		addrP;
static char *		namePrevP = NULL;
static char *		addrPrevP = NULL;
static int			positionPrev;
static OpPrev		opPrev;
static char *		namePrevSys = NULL;
static char *		addrPrevSys = NULL;
static int			positionPrevSys;


ListBuf *		undeleteBuf = NULL;

ListHead		UndeleteList;
ListHead *		undeleteList = (ListHead *)&UndeleteList;

static Setting aliasUndeleteSetting = {
	"Header",
	(XtPointer)NULL,
	(XtPointer)NULL,
	(XtPointer)"x"
 };

ListGizmo aliasUndelete = {
    NULL,                   /* help info */
    "aliasUndelete",        /* name of widget */
    "",                     /* caption */
    &aliasUndeleteSetting,  /* settings */
    "%b %18w",          	/* format */
    FALSE,                  /* exclusive */
    LISTHEIGHT,             /* height */
    NULL,                   /* font: default */
    NULL,                   /* list itself to be set later */
    AliasUndeleteSelectCB,      /* execute (double click) same as select */
    AliasUndeleteSelectCB,
    AliasUndeleteUnselectCB,
};
ListGizmo *             aliasUndeleteP = &aliasUndelete;

static GizmoRec undeleteArray[] = {
	{ListGizmoClass,	&aliasUndelete},
};

static LabelGizmo undeleteContainer = {
	NULL, 
	"undeleteContainer", 
	TXT_NAMES, 
	undeleteArray, 
	XtNumber (undeleteArray),
	OL_FIXEDROWS, 
	1, 
	0, 
	0, 
	True
};

static GizmoRec AliasUndeleteGiz[] = {
	{LabelGizmoClass,	&undeleteContainer},
};

static MenuItems aliasUndeleteMenuItems[] = {
	{True, BUT_UNDELETE,		MNEM_UNDELETE,	NULL, AliasUndeleteApplyCB},
	{True, BUT_CANCEL,			MNEM_CANCEL,	NULL, CancelCB},
	{True, BUT_HELP,			MNEM_HELP,		NULL, HelpCB, (XtPointer)HelpAliasManagerUndelete},
	{NULL}
};

static MenuGizmo aliasUndeleteMenu = {
	NULL,				/* Help		*/
	"",					/* Name		*/
	"aliasUndeleteMenu",	/* Title	*/
	aliasUndeleteMenuItems,	/* Items	*/
	0,					/* Function	*/
	NULL,				/* Client data	*/
	CMD,				/* Button type	*/
	OL_FIXEDROWS,		/* Layout type	*/
	1,					/* Measure	*/
	0					/* Default item	*/
};

static PopupGizmo *	aliasUndeletePopup = (PopupGizmo *)0;

static PopupGizmo	AliasUndeletePopup = {
	&AliasManagerUndeleteHelp,
	"aliasUndelete",
	TXT_E_MAIL_UNDELETE,
	&aliasUndeleteMenu,
	AliasUndeleteGiz,
	XtNumber (AliasUndeleteGiz),
};

static MenuItems addItems[] = {
    {True, BUT_OVERWRITE,	MNEM_OVERWRITE,	  NULL,   AddOverwriteCB},
    {True, BUT_CANCEL,		MNEM_CANCEL,	  NULL,   AddCancelCB},
    {True, BUT_HELP,		MNEM_HELP,		  NULL,   HelpCB,(XtPointer)HelpAliasManagerOverwrite},
    {NULL}
};

static MenuGizmo addMenu = {
    NULL,           /* help     */
    "addrepeat",    /* name     */
    NULL,		    /* title    */
    addItems, 		/* items    */
    0,              /* function */
    NULL,           /* client_data  */
    CMD,            /* buttonType   */
    OL_FIXEDROWS,   /* layoutType   */
    1,              /* measure  */
    0               /* default Item */
};

ModalGizmo addNotice = {
    NULL,
    "addNotice",
    TXT_JUST_MAIL,
    &addMenu,
    TXT_OVERWRITE_ALIAS,
};
ModalGizmo *    addNoticeP = &addNotice;

/*--------------------------------------------------------------------------*/

void 
UndoPrep (operation, lastPosition, lastNameP, lastAddrP)
OpPrev	operation;
int		lastPosition;
char *	lastNameP;
char *	lastAddrP;
{
	/* save modified info for potential undo */
	opPrev = operation;
	positionPrev = lastPosition; 	/* new data is in here */

	if (namePrevP != NULL)
	{
		FREE (namePrevP);
		FREE (addrPrevP);
	}
	if (lastNameP == NULL)
	{	/* empty strings; put them in */
		namePrevP = NULL;
		addrPrevP = NULL;
	}
	else /* save name and address? */
	{
		namePrevP = STRDUP (lastNameP);	/* old name and address */
		addrPrevP = STRDUP (lastAddrP);
	}
	/***  fprintf (stderr, "UndoPrep: Op <%#x>, position %d, name <%s>, addr <%s>\n",opPrev,positionPrev,namePrevP,addrPrevP); /***/
}


void
ApplyAlias (applyPos, replace)
int			applyPos;		/* position where new or changed element goes */
Boolean		replace;
{
ListHead *	scrollList;
XtArgVal *	tmpP;
int			j;
int			k;
Arg			arg[1];
int			numDeleted;

	/***  fprintf (stderr, "ApplyAlias, replace %d\n", replace); /***/

	scrollList = mailRec->alias;	/* list head */

	if (!replace)
	{
		/***  fprintf (stderr, "ApplyAlias/!replace, applyPos %d, name <%s>, addr <%s>\n",applyPos,nameP,addrP); /***/
		/* first move the rest of the items up one */
		for (j = aliasRec->name->used; j >= applyPos; j--)
		{	/* move the elements */
			scrollList->items[j].fields = scrollList->items[j-1].fields;
			scrollList->items[j].clientData = scrollList->items[j-1].clientData;
			aliasRec->name->p[j] = aliasRec->name->p[j-1];
			aliasRec->addr->p[j] = aliasRec->addr->p[j-1];

		}
		/* save modified info for potential undo */
		UndoPrep (OpApplyNewAlias,applyPos, 
						aliasRec->name->p[j], aliasRec->addr->p[j]);
	}
	else /* replace */
	{
		/***  fprintf (stderr, "ApplyAlias/replace: applyPos (%d), name <%s>, addr <%s>\n",applyPos,nameP,addrP); /***/

		/* save modified info for potential undo */
		UndoPrep (OpApplyModAlias,applyPos,
				aliasRec->name->p[applyPos],aliasRec->addr->p[applyPos]);
	}

	/***  fprintf (stderr, "ApplyAlias: applyPos (%d), name <%s>, addr <%s>\n",applyPos,nameP,addrP); /***/
	/***  fprintf (stderr, "ApplyAlias: scrollList->numFields %d\n",scrollList->numFields); /***/
	tmpP = (XtArgVal *) MALLOC(scrollList->numFields*sizeof(XtArgVal));

	/* now actually copy the pointer structure */
	scrollList->items[applyPos].fields = (XtArgVal) tmpP;
	tmpP[0] = (XtArgVal) (OnlyOneQ(addrP)?UserAliasPixmap:UserGroupPixmap);
	aliasRec->name->p[applyPos] = STRDUP (nameP);
	tmpP[1] = (XtArgVal) aliasRec->name->p[applyPos];
	aliasRec->addr->p[applyPos] = STRDUP (addrP);
	tmpP[2] = (XtArgVal) aliasRec->addr->p[applyPos];
	/***  fprintf (stderr, "New entry: applyPos %d, tmpP[0] <%#x>, UserGroupPixmap <%#x>, UserAliasPixmap <%#x>\n",applyPos,tmpP[0],UserGroupPixmap,UserAliasPixmap); /***/
	/***  fprintf (stderr, "New entry: applyPos %d, tmpP[0] <%#x>, tmpP[1] <%s>, tmpP[2] <%s>\n",applyPos,tmpP[0],tmpP[1],tmpP[2]); /***/

	/* fix up the background storage tables */
	/***  fprintf (stderr, "Before background fixup: aliasRec->userCount %d, aliasRec->systemCount %d, aliasRec->aliasTotal %d\n",aliasRec->userCount,aliasRec->systemCount,aliasRec->aliasTotal); /***/
	/***  fprintf (stderr, "Before background fixup: aliasRec->userAlias <%#x>, aliasRec->systemAlias <%#x>\n",aliasRec->userAlias,aliasRec->systemAlias,aliasRec->aliasTotal); /***/
	numDeleted = ReplaceAlias (aliasRec->name->p[applyPos],
							aliasRec->addr->p[applyPos],aliasRec->userAlias);
	if (numDeleted == 0)
	{	/* this alias was added to the user list - no replacement */
		aliasRec->userCount++;
		aliasRec->aliasTotal++;
	}
	/***  fprintf (stderr, "Replace/add user alias <%s> had %d addresses, user count %d\n", aliasRec->name->p[applyPos],numDeleted,aliasRec->userCount); /***/
	/* clean up in case it was a system alias */
	numDeleted = 
			DeleteAlias (aliasRec->name->p[applyPos],aliasRec->systemAlias);
	if (numDeleted > 0)
	{	/* there was a system alias with that name that was deleted */
		aliasRec->systemCount--;
		aliasRec->aliasTotal--;
		/* must also delete system alias from viewable list */
		for (j = aliasRec->userCount; j < aliasRec->aliasTotal; j++)
		{
			if (strcmp(aliasRec->name->p[j],aliasRec->name->p[applyPos]) == 0)
			{
				/* save info for possible undo */
				positionPrevSys = j;
				namePrevSys = aliasRec->name->p[j];	/* instead of FREEing it */
				addrPrevSys = aliasRec->addr->p[j];	/* instead of FREEing it */
				/* FREE (aliasRec->name->p[j]);  implicit above */
				/* FREE (aliasRec->addr->p[j]);  implicit above */

				/* move the rest of the items down one */
				for (k = j; k < aliasRec->aliasTotal; k++)
				{	/* move the elements */
					scrollList->items[k].fields = 
									scrollList->items[k+1].fields;
					scrollList->items[k].clientData = 
									scrollList->items[k+1].clientData;
					aliasRec->name->p[k] = aliasRec->name->p[k+1];
					aliasRec->addr->p[k] = aliasRec->addr->p[k+1];
				}
			
				for (k = 0; k < aliasRec->name->used; k++)
				{	/* unset all the elements */
					scrollList->items[k].set = (XtArgVal) False;
				}	
			
				/* decrease all the counts - must consolidate this! */
				scrollList->size--;
				aliasRec->list->used = 
				aliasRec->name->used = 
				aliasRec->addr->used = scrollList->size;
				/***  fprintf (stderr, "ApplyAlias, delete system alias after lowering the counters, aliasRec->name->used %d\n",aliasRec->name->used); /***/

				/* Set sensitivities */
				SetSensitivity (AliasWindowP,"aliasEdit", 3, 3, False);
				SetSensitivity (AliasWindowP,"aliasEdit", 0, 0, True);
				SetSensitivity (AliasWindowP,"aliasEdit", 4, 4, True);
			
				MakeItSo();
			}
		}
	}

	/***  fprintf (stderr, "Replace/add user alias <%s> had %d addresses, user count %d\n", aliasRec->name->p[applyPos],numDeleted,aliasRec->userCount); /***/

	/***  fprintf (stderr, "ApplyAlias: set/unset elements, name <%s>, addr <%s>\n",nameP,addrP); /***/

	for (j = 0; j < aliasRec->name->used; j++)
	{	/* unset the rest of the elements */
		scrollList->items[j].set = (XtArgVal) False;
	}	
	scrollList->items[applyPos].set = (XtArgVal) True;


	/***  fprintf (stderr, "ApplyAlias: bump the counters, name <%s>, addr <%s>\n",nameP,addrP); /***/
	if (!replace)
	{
		/* increase all the counts - must consolidate this! */
		scrollList->size++;
		aliasRec->list->used = 
		aliasRec->name->used = 
		aliasRec->addr->used = scrollList->size;
		/***  fprintf (stderr, "ApplyAlias, don't replace after bumping the counters, aliasRec->name->used %d\n",aliasRec->name->used); /***/
	}

	/***  fprintf (stderr, "ApplyAlias: Set Sensitivies, name <%s>, addr <%s>\n",nameP,addrP); /***/
	/* Set sensitivities */
	SetSensitivity (AliasWindowP,"aliasEdit", 0, 0, True);	/* Undo */
    if (scrollList->size >0)
        SetSensitivity (AliasWindowP,"aliasEdit", 3, 3, True);  /* Delete */
    else
        SetSensitivity (AliasWindowP,"aliasEdit", 3, 3, False); /* Delete */

	scrollList->items[applyPos].set = (XtArgVal) True;

	MakeItSo();

	aliasRec->unsaved = True;
}


void
AddOverwriteCB (widg, client_data, call_data)
Widget				widg;
XtArgVal			client_data;
OlFlatCallData *	call_data;
{ 
	SetBaseWindowMessage (AliasWindowP,"");	/* remove any footer message */

	ApplyAlias (position,True);		/* replace existing alias */
	XtPopdown ((Widget) _OlGetShellOfWidget (widg));
}



void
AddCancelCB (widg, client_data, call_data)
Widget				widg;
XtArgVal			client_data;
OlFlatCallData *	call_data;
{ 
	SetBaseWindowMessage (AliasWindowP,"");	/* remove any footer message */

	if (nameP != NULL) FREE (nameP);
	if (addrP != NULL) FREE (addrP);
	XtPopdown ((Widget) _OlGetShellOfWidget (widg));
}


void
AliasUndoCB (widg, client_data, call_data)
Widget				widg;
XtArgVal			client_data;
OlFlatCallData *	call_data;
{ 
ListHead *	scrollList;
XtArgVal *	tmpP;
int			j;
int			k;
Arg			arg[5];

	SetBaseWindowMessage (AliasWindowP,"");	/* remove any footer message */

	scrollList = mailRec->alias;	/* list head */

	/***  fprintf (stderr, "AliasUndo, opPrev <%#x>\n",opPrev); /***/

	switch (opPrev)
	{
		case OpApplyNewAlias:
		{
			/***  fprintf (stderr, "AliasUndo, OpApplyNewAlias\n"); /***/
			/* add alias back to system view area, if appropriate */
			if (namePrevSys != 0)
			{	/* also have to add back item to viewable system list */
				/***  fprintf (stderr, "AliasUndo, adding back system alias positionPrevSys\n"); /***/
				nameP = namePrevSys;	/* copy pointers */
				addrP = addrPrevSys;
				ApplyAlias (positionPrevSys, False);
				namePrevSys = addrPrevSys = 0;
			}
			/* delete alias from where it was added */
			position = positionPrev;
			AliasDeleteCB (widg, client_data, call_data);
			break;
		}
		case OpApplyModAlias:
		{
			/***  fprintf (stderr, "AliasUndo, OpApplyModAlias\n"); /***/
			/* add alias back to system view area, if appropriate */
			if (namePrevSys != 0)
			{	/* also have to add back item to viewable system list */
				/***  fprintf (stderr, "AliasUndo, adding back system alias positionPrevSys\n"); /***/
				nameP = namePrevSys;	/* copy pointers */
				addrP = addrPrevSys;
				ApplyAlias (positionPrevSys, False);
				namePrevSys = addrPrevSys = 0;
			}
			/* replace alias where it was modified in user table */
			position = positionPrev;
			nameP = STRDUP (namePrevP);
			addrP = STRDUP (addrPrevP);
			ApplyAlias (position, True);		/* replace existing alias */
    		SetSensitivity (AliasWindowP, "aliasEdit", 3, 3, True);
			scrollList->items[position].set = (XtArgVal) True;
			XtVaSetValues (
       			aliasName.textFieldWidget,
       			XtNstring, nameP,
       			(String)0
    		);
    		XtVaSetValues (
        		aliasAddress.textFieldWidget,
        		XtNstring, addrP,
        		(String)0
    		);
			MakeItSo();

			break;
		}
		case OpDeleteAlias:
		{
			/***  fprintf (stderr, "AliasUndo, OpDeleteAlias\n"); /***/
			nameP = STRDUP (namePrevP);
			addrP = STRDUP (addrPrevP);
			ApplyAlias (positionPrev, False); /* don't replace */
			UndoPrep (OpApplyNewAlias,positionPrev,nameP,addrP);
            if (scrollList->size >0)
                SetSensitivity (AliasWindowP, "aliasEdit", 3, 3, True);
            else
                SetSensitivity (AliasWindowP, "aliasEdit", 3, 3, False);
			scrollList->items[position].set = (XtArgVal) True;
			XtVaSetValues (
       			aliasName.textFieldWidget,
       			XtNstring, nameP,
       			(String)0
    		);
    		XtVaSetValues (
        		aliasAddress.textFieldWidget,
        		XtNstring, addrP,
        		(String)0
    		);
			/* take it off the undelete list */
			for (j = 0; j < undeleteList->size; j++)
			{
				tmpP = (XtArgVal *) undeleteList->items[j].fields;
				/***  fprintf (stderr, "AliasDelete, Undo Undelete: j %d, tmpP[1] <%s>, nameP <%s>\n",j,tmpP[1],nameP); /***/
				if (strcmp ((char *)tmpP[1],nameP) == 0)
				{	/* a name match; remove it and move the others back */
					undeleteList->items[j].set = (XtArgVal) False;
					for (k = j; k < undeleteList->size - 1; k++)
					{
						undeleteList->items[k].set = False;
						undeleteList->items[k].fields = 
										undeleteList->items[k+1].fields;
						addrUndeleteBuf->p[k] = addrUndeleteBuf->p[k+1];
					}
					FREE(addrUndeleteBuf->p[undeleteList->size]);
					undeleteList->size--; 
					undeleteBuf->used--;
					addrUndeleteBuf->used--;
					break;
				}
			}
			/***  fprintf (stderr, "AliasDelete, Undo Undelete: size %d, tmpP[1] <%s>, tmpP[0] <%#x>, addr <%s>\n",undeleteList->size,tmpP[1],tmpP[0],addrUndeleteBuf->p[undeleteList->size]); /***/
			if (undeleteList->size == 0) 
				SetSensitivity (AliasWindowP,"aliasEdit", 4, 4, False);

			if (aliasUndeleteWidget != 0)	/* make sure it has been created */
			{
				XtSetArg (arg[0], XtNitemsTouched,	True);
				XtSetArg (arg[1], XtNnumItems,undeleteList->size);
				XtSetValues (aliasUndeleteWidget, arg, 2);

				if (undeleteList->size == 0) 
				  XtPopdown ((Widget) _OlGetShellOfWidget(aliasUndeleteWidget));
				MakeItSo();
			}

			break;
		}
		case OpUndeleteAlias:
		{
			/***  fprintf (stderr, "AliasUndo, OpUndeleteAlias\n"); /***/
			break;
		}
		case OpNewAlias:
		{
			/***  fprintf (stderr, "AliasUndo, OpNewAlias\n"); /***/
			XtVaSetValues (
       			aliasName.textFieldWidget,
       			XtNstring, namePrevP,
       			(String)0
    		);
    		XtVaSetValues (
        		aliasAddress.textFieldWidget,
        		XtNstring, addrPrevP,
        		(String)0
    		);
    		position = positionPrev;
            /* make the delete button sensitive, if appropriate */
            if (scrollList->size >0)
                SetSensitivity (AliasWindowP, "aliasEdit", 3, 3, True);
            else
                SetSensitivity (AliasWindowP, "aliasEdit", 3, 3, False);

			scrollList->items[position].set = (XtArgVal) True;
			MakeItSo();
			UndoPrep (OpNewNewAlias,position,NULL,NULL);
			break;
		}
		case OpNewNewAlias:
		{	/* user changed mind about undoing new alias */
			AliasNewCB (widg, client_data, call_data);
			break;
		}
		default:
		{
			/***  fprintf (stderr, "AliasUndo, Default - shouldn't get here\n"); /***/
			break;
		}
	}
}


void
AliasNewCB (widg, client_data, call_data)
Widget				widg;
XtArgVal			client_data;
OlFlatCallData *	call_data;
{ 
ListHead *	scrollList;
int			j;

	SetBaseWindowMessage (AliasWindowP,"");	/* remove any footer message */

	XtVaSetValues (
        aliasName.textFieldWidget,
        XtNstring, "",
        (String)0
    );
    XtVaSetValues (
        aliasAddress.textFieldWidget,
        XtNstring, "",
        (String)0
    );

	scrollList = mailRec->alias;	/* list head */

	/* save old info for potential undo */
	if (scrollList->size > 0)
        UndoPrep (OpNewAlias,position,
                aliasRec->name->p[position],aliasRec->addr->p[position]);
	else
        UndoPrep (OpNewAlias,position, NULL, NULL);

	scrollList->items[position].set = (XtArgVal) False;

	for (j = 0; j < aliasRec->name->used; j++)
	{	/* unset the rest of the elements */
		scrollList->items[j].set = (XtArgVal) False;
	}	

	/* Set sensitivities */
	SetSensitivity (AliasWindowP,"aliasEdit", 0, 0, True);	/* Undo */

	MakeItSo();

}


void
AliasAddCB (widg, client_data, call_data)
Widget				widg;
XtArgVal			client_data;
OlFlatCallData *	call_data;
{ 
Arg			arg[1];


	SetBaseWindowMessage (AliasWindowP,"");	/* remove any footer message */

	/* get current values from widget */

	XtSetArg (arg[0], XtNstring, &nameP);
	XtGetValues (aliasName.textFieldWidget, arg, 1);
	XtSetArg (arg[0], XtNstring, &addrP);
	XtGetValues (aliasAddress.textFieldWidget, arg, 1);

	_AliasAddCB();

}


void
_AliasAddCB()		/* assumes nameP and addrP have been set */
{
ListHead *	scrollList;
int			i, j;
XtArgVal *	tmpP;
	scrollList = mailRec->alias;	/* list head */

	/* check for validity of name and address */
	if (strlen(nameP) == 0) 
	{	/* tell user to put name in field */
		NoticeMessage (TXT_MUST_BE_A_NAME);

		FREE (nameP);
		FREE (addrP);
		return;
	}
	else if (strlen(addrP) == 0)
	{	/* tell user to put address in field */
		NoticeMessage (TXT_MUST_BE_AN_ADDRESS);
		FREE (nameP);
		FREE (addrP);
		return;
	}
	else if (strspn(nameP, legalName) != strlen (nameP))
	{
		NoticeMessage (TXT_ILLEGAL_CHARACTER);
		FREE (nameP);
		FREE (addrP);
		return;
	}

	/* first make room for new items */
	/* if we wind up adding, this may not be needed, but it doesn't hurt */
	if (BufferFilled(aliasRec->name))
	{
		GrowBuffer ((Buffer *) aliasRec->list,GROWTH);
		GrowBuffer ((Buffer *) aliasRec->name,GROWTH);
		GrowBuffer ((Buffer *) aliasRec->addr,GROWTH);
		scrollList -> items = (ListItem *) aliasRec->list->p;/* reset list ptr*/
	}

	/* see if name is already there and/or find place for new data */
	for (i = 0; i < aliasRec->userCount; i++)
	{
		tmpP = (XtArgVal *) scrollList->items[i].fields;
		if (strcmp ((char *)tmpP[1],nameP) == 0)
		{
			/***  fprintf (stderr, "Add, names match: name <%s>, tmpP[1] <%s>\n",nameP,tmpP[1]); /***/
			/* check to see if there's no change in either name or address */
			if ((strcmp ((char *)tmpP[1],nameP) == 0) && 
				(strcmp ((char *)tmpP[2],addrP) == 0))
			{
				/***  fprintf (stderr, "Add, addresses match: addr <%s>, tmpP[2] <%s>\n",addrP,tmpP[2]); /***/
				NoticeMessage(TXT_NO_CHANGE_NAME);
				FREE (nameP);
				FREE (addrP);
				return;
			}
			/***  else fprintf (stderr, "Add, addresses don't match: addr <%s>, tmpP[2] <%s>\n",addrP,tmpP[2]); /***/

			/* name already there; allow user to add to address*/
			if (QueryGizmo (ModalGizmoClass, addNoticeP,
							GetGizmoWidget, "addNotice") == (XtPointer) 0)
			{	/* first time; create Gizmo */
				CreateGizmo (GetBaseWindowShell (AliasWindowP),
					  ModalGizmoClass, addNoticeP, 0, 0);
			}
    		MapGizmo (ModalGizmoClass, addNoticeP);

			/* save location information for later */
			position = i;
			/***  fprintf (stderr, "Name repeat, maybe ApplyAlias: (%d), nameP <%s>, addrP <%s>\n",position,nameP,addrP); /***/
			return;		/* don't want to free nameP and addrP yet */
		}
		else if (strcmp ((char *)tmpP[1],nameP) > 0)
		{	/* found location for new name */
			/***  fprintf (stderr, "Standard ApplyAlias (%d)\n",i); /***/
			position = i;
			ApplyAlias (position, False);	/* don't replace alias */
			FREE (nameP);
			FREE (addrP);
			return;
		}
	}

	/* not found; put it at the end */
	position = aliasRec->userCount;
	ApplyAlias(position, False);
	FREE (nameP);
	FREE (addrP);
	return;
}


void
AliasDeleteCB (widg, client_data, call_data)
Widget				widg;
XtArgVal			client_data;
OlFlatCallData *	call_data;
{ 
int			itemNum = position;		/* last position selected */
int			j;
int			numDeleted;
ListHead *	scrollList;	/* list head */
Arg			arg[6];
XtArgVal *	tmpAP;
XtArgVal *	tmpP;
XtArgVal *	tmpNewP;
Boolean		foundMatch;

	SetBaseWindowMessage (AliasWindowP,"");	/* remove any footer message */

	scrollList = mailRec->alias;	/* list head */

	/***  fprintf (stderr, "AliasDeleteCB: item %d, name <%s>, addr <%s>\n",itemNum,aliasRec->name->p[itemNum],aliasRec->addr->p[itemNum]); /***/

	/* save info for potential undo */
	UndoPrep (OpDeleteAlias,itemNum, 
			aliasRec->name->p[itemNum], aliasRec->addr->p[itemNum]);

	/* update undelete list */
	aliasUndelete.list = undeleteList;	/* point the gizmo to the data */

	if (undeleteBuf == NULL)
	{
		undeleteBuf = (ListBuf *)AllocateBuffer(sizeof(ListItem), GROWTH);
		addrUndeleteBuf = (CharBuf *) AllocateBuffer (sizeof(ListItem), GROWTH);
		undeleteList->items = (ListItem *) undeleteBuf->p;
		undeleteList->numFields = 2;
		undeleteList->size = 0;
	}
	/***  fprintf (stderr, "AliasDeleteCB-prep Undelete: undeleteList->numFields %d, undeleteList->size %d\n",undeleteList->numFields,undeleteList->size); /***/

	if (BufferFilled(undeleteBuf))
	{
		GrowBuffer ((Buffer *) undeleteBuf, GROWTH);
		GrowBuffer ((Buffer *) addrUndeleteBuf, GROWTH);
		undeleteList->items = (ListItem *) undeleteBuf->p; /* reset list ptr */
	}
	/* copy the values for possible undelete */
	/* check to see if the value is already there */
	foundMatch = False;
	tmpAP = (XtArgVal *) scrollList->items[itemNum].fields;
	for (j = 0; j < undeleteList->size; j++)
	{
		tmpP = (XtArgVal *) undeleteList->items[j].fields;
		if (strcmp ((char *)tmpP[1],(char *)tmpAP[1]) == 0)
		{	/* a name match; substitute the newer one */
			foundMatch = True;
			tmpP[0] = (XtArgVal) tmpAP[0];	/* copy bitmap */
			undeleteList->items[j].set = (XtArgVal) False;
			FREE(addrUndeleteBuf->p[j]);
			addrUndeleteBuf->p[j] = 
					STRDUP ((char *) aliasRec->addr->p[itemNum]);
			/***  fprintf (stderr, "AliasDelete, Undelete prep substitute: size %d, tmpP[1] <%s>, tmpP[0] <%#x>, addr <%s>\n",undeleteList->size,tmpP[1],tmpP[0],addrUndeleteBuf->p[undeleteList->size]); /***/
		}
	}
	if (!foundMatch)
	{		/* add it at the end */
		tmpNewP = (XtArgVal *)
						MALLOC(undeleteList->numFields*sizeof(XtArgVal));
		tmpNewP[0] = (XtArgVal) tmpAP[0];	/* copy bitmap from alias list */
		tmpNewP[1] = (XtArgVal) tmpAP[1];
		undeleteList->items[undeleteList->size].fields = (XtArgVal) tmpNewP;
		undeleteList->items[undeleteList->size].set = (XtArgVal) False;
		addrUndeleteBuf->p[undeleteList->size] =
							STRDUP ((char *) aliasRec->addr->p[itemNum]);
		/***  fprintf (stderr, "AliasDelete, Undelete prep populate: size %d, tmpP[1] <%s>, tmpP[0] <%#x>, addr <%s>\n",undeleteList->size,tmpP[1],tmpP[0],addrUndeleteBuf->p[undeleteList->size]); /***/
		undeleteList->size++;  undeleteBuf->used++; addrUndeleteBuf->used++;
	}
	/* redisplay the data if show widget is already up */
	if (aliasUndeleteWidget != 0)	/* make sure it has been created */
	{
		XtSetArg (arg[0], XtNviewHeight,	(XtArgVal) LISTHEIGHT);
		XtSetArg (arg[1], XtNitemsTouched,	True);
		XtSetArg (arg[2], XtNnumItems,		undeleteList->size);
	
		XtSetValues (aliasUndeleteWidget, arg, 3);
	}

	/* fix up the background storage tables */
	if (itemNum < aliasRec->userCount)
	{	/* it was a user alias */
		numDeleted = DeleteAlias (aliasRec->name->p[itemNum],aliasRec->userAlias);
		aliasRec->userCount--;
		/***  fprintf (stderr, "Deleted user alias <%s>, had %d addresses, user count %d\n", aliasRec->name->p[itemNum],numDeleted,aliasRec->userCount); /***/
	}
	else
	{	/* it was a system alias */
		numDeleted = DeleteAlias (aliasRec->name->p[itemNum],aliasRec->systemAlias);
		aliasRec->systemCount--;
		/***  fprintf (stderr, "Deleted system alias <%s>, had %d addresses, system count %d\n", aliasRec->name->p[itemNum],numDeleted,aliasRec->systemCount); /***/
	}
	aliasRec->aliasTotal--;

	/* FREE (aliasRec->name->p[itemNum]);don't free; being used by undelete */
	/* FREE (aliasRec->addr->p[itemNum]);don't free; being used by undelete */

	/* move the rest of the items down one */
	for (j = itemNum; j < aliasRec->name->used; j++)
	{	/* move the elements */
		scrollList->items[j].fields = scrollList->items[j+1].fields;
		scrollList->items[j].clientData = scrollList->items[j+1].clientData;
		aliasRec->name->p[j] = aliasRec->name->p[j+1];
		aliasRec->addr->p[j] = aliasRec->addr->p[j+1];
	}

	for (j = 0; j < aliasRec->name->used; j++)
	{	/* unset all the elements */
		scrollList->items[j].set = (XtArgVal) False;
	}	

	/* decrease all the counts - must consolidate this! */
	scrollList->size--;
	aliasRec->list->used = 
	aliasRec->name->used = 
	aliasRec->addr->used = scrollList->size;

	/***  fprintf (stderr, "DeleteAlias after lower the counters, aliasRec->name->used %d\n",aliasRec->name->used); /***/

	/* Set sensitivities */
	SetSensitivity (AliasWindowP,"aliasEdit", 3, 3, False);	/* Delete */
	SetSensitivity (AliasWindowP,"aliasEdit", 0, 0, True);	/* Undo */
	SetSensitivity (AliasWindowP,"aliasEdit", 4, 4, True);	/* Undelete */

	aliasRec->unsaved = True;		/* there are now pending changes */
	MakeItSo();
}


void
MakeItSo()
{	/* make the change take effect */

Arg arg[10];

	if (aliasRec->aliasWidget != 0)
	{
		XtSetArg (arg[0], XtNviewHeight,	(XtArgVal) LISTHEIGHT);
		XtSetArg (arg[1], XtNitemsTouched,	True);
		XtSetArg (arg[2], XtNnumItems,		aliasRec->name->used);
		XtSetValues (aliasRec->aliasWidget, arg, 3);
	}

	if (aliasRec->aliasShowWidget != 0)	/* if it has been created already */
	{
		/***  fprintf (stderr, "MakeItSo, redisplay aliasShowWidget, aliasRec->name->used %d\n",aliasRec->name->used); /***/
		AliasShowPrep();
		XtSetArg (arg[0], XtNviewHeight,	(XtArgVal) LISTHEIGHT);
		XtSetArg (arg[1], XtNdropProc,    	AliasDropProcCB);
		XtSetArg (arg[2], XtNitemsTouched,	True);
		XtSetArg (arg[3], XtNnumItems,      aliasRec->name->used);
		XtSetValues (aliasRec->aliasShowWidget, arg, 4);
	}
}


void
AliasUndeleteCB(wid, clientData, callData)
Widget wid;
XtPointer clientData;
XtPointer callData;
{
int			i;
Arg			arg[8];

	/***  fprintf (stderr, "AliasUndeleteCB: size %d, Popup <%#x>\n",undeleteList->size,aliasUndeletePopup); /***/

	if (aliasUndeletePopup == (PopupGizmo *)0) {
		aliasUndeletePopup = &AliasUndeletePopup;
		CreateGizmo (
			GetBaseWindowShell (AliasWindowP),
			PopupGizmoClass,
			aliasUndeletePopup,
			NULL, 0
		);
	}
	/* Make it show (MakeItSo); make the change take effect */
	aliasUndeleteWidget = (Widget) QueryGizmo (
			ListGizmoClass, 
			aliasUndeleteP, 
			GetGizmoWidget,
			"aliasUndelete"
	);
	/***  fprintf (stderr, "AliasUndeleteCB: widget <%#x>\n",aliasUndeleteWidget); /***/

	XtSetArg (arg[0], XtNviewHeight,	(XtArgVal) LISTHEIGHT);
	XtSetArg (arg[1], XtNitemsTouched,	True);
	XtSetArg (arg[2], XtNnumItems,		undeleteList->size);

	XtSetValues (aliasUndeleteWidget, arg, 3);

	MapGizmo (PopupGizmoClass, aliasUndeletePopup);
}




void
AliasUndeleteSelectCB(wid, clientData, callData)
Widget wid;
XtArgVal clientData;
OlFlatCallData * callData;
{
	int         itemNum = callData->item_index;
	undeleteList->items[itemNum].set = True;
	return;
}



void
AliasUndeleteUnselectCB(wid, clientData, callData)
Widget wid;
XtArgVal clientData;
OlFlatCallData * callData;
{
	int         itemNum = callData->item_index;
	undeleteList->items[itemNum].set = False;
	return;
}


void
AliasUndeleteApplyCB(wid, clientData, callData)
Widget wid;
XtPointer clientData;
XtPointer callData;
{
Arg         arg[5];
XtArgVal *  tmpP;
char *		nameUndel;
char *		addrUndel;
int			i;
int			j;


	/***  fprintf (stderr, "AliasUndeletApplyCB: size %d\n",undeleteList->size); /***/

	for (i = 0; i < undeleteList->size; i++)
	{
		if (undeleteList->items[i].set)
		{
			tmpP = (XtArgVal *) undeleteList->items[i].fields;
			nameUndel = (char *) tmpP[1];
			addrUndel = (char *) addrUndeleteBuf->p[i];
			/***  fprintf (stderr, "Undeleting %d, name <%s>, addr <%s>, glyph <%#x>\n",i,nameUndel, addrUndel, tmpP[0]); /***/
			nameP = nameUndel;
			addrP = addrUndel;
			_AliasAddCB ();	/* put it back in */
		}
		/***  else fprintf (stderr, "Not Undeleting %d\n",i); /***/
	}
	/***  fprintf (stderr, "Clean up Undeleting\n"); /***/
	/* now clean up the deleted list */
	j = 0;
	for (i = 0; i < undeleteList->size; i++)
	{
		if (!undeleteList->items[i].set)
		{ 
			/***  fprintf (stderr, "Clean up move %d to %d\n",i,j); /***/
			if (j != i)
			{
				undeleteList->items[j].fields = undeleteList->items[i].fields;
				addrUndeleteBuf->p[j] = addrUndeleteBuf->p[i];
			}
			j++;
		}
	}
	undeleteList->size = undeleteBuf->used = addrUndeleteBuf->used = j;

	aliasUndeleteWidget = (Widget) QueryGizmo (
			ListGizmoClass, 
			aliasUndeleteP, 
			GetGizmoWidget,
			"aliasUndelete"
	);
	if (undeleteList->size == 0) 
		SetSensitivity (AliasWindowP,"aliasEdit", 4, 4, False);

	SetSensitivity (AliasWindowP,"aliasEdit", 0, 0, False);	/* turn off undo */

	if (aliasUndeleteWidget != 0)
	{
		XtSetArg (arg[0], XtNitemsTouched,	True);
		XtSetArg (arg[1], XtNnumItems,		undeleteList->size);
		XtSetValues (aliasUndeleteWidget, arg, 2);
	}

	if (undeleteList->size == 0) 
		XtPopdown ((Widget) _OlGetShellOfWidget (wid));
	else
		MapGizmo (PopupGizmoClass, aliasUndeletePopup);
}

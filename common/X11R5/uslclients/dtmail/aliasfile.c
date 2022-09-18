/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtmail:aliasfile.c	1.15"
#endif

#define ALIASFILE_C

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
#include <sys/types.h>			/* need this for XtNtitle */
#include <stdio.h>
#include "mail.h"
#include <Gizmo/Gizmos.h>
#include <Gizmo/LabelGizmo.h>
#include "RMailGizmo.h"
#include "ListGizmo.h"

/* Functions defined in this module */

void				AliasSaveCB ();
void				AliasExitCB ();
void				AliasExitYesCB ();
void				AliasExit();
void				AliasPrintCB ();

extern int					Version;
extern Widget				Root;
extern Boolean				SendOnly;
extern MailRec *			mailRec;
extern AliasRec *			aliasRec;
extern BaseWindowGizmo *	AliasWindowP;

#ifdef UseXtApp
extern XtAppContext app_con;
	/* must get application context from main - not done yet */
#endif

static MenuItems sureItems[] = {
    {True, BUT_OK,		MNEM_OK,	 	NULL,   AliasExitYesCB},
    {True, BUT_CANCEL,	MNEM_CANCEL, 	NULL,   CancelCB},
    {True, BUT_HELP,	MNEM_HELP,		NULL,   HelpCB, (XtPointer)HelpAliasManagerSure},
    {NULL}
};

static MenuGizmo sureMenu = {
    NULL,           /* help     */
    "sureExit",     /* name     */
    NULL,		    /* title    */
    sureItems, 		/* items    */
    0,              /* function */
    NULL,           /* client_data  */
    CMD,            /* buttonType   */
    OL_FIXEDROWS,   /* layoutType   */
    1,              /* measure  */
    0               /* default Item */
};

ModalGizmo sureNotice = {
    NULL,
    "sureNotice",
    TXT_JUST_MAIL,
    &sureMenu,
    TXT_R_U_SURE,
};
ModalGizmo *    sureNoticeP = &sureNotice;
/*----------------------------------------------------------------------*/

void
AliasPrintCB (widg, client_data, call_data)
Widget			widg;
XtArgVal		client_data;
OlFlatCallData *	call_data;
{ 
FILE *		fd;
int			i;
pid_t		pid;
char		outFile[BUF_SMALL];
int			retCode;
char 		Command[BUF_SMALL];



	pid = getpid();
	sprintf (outFile, GetGizmoText (TXT_DOT_ALIAS),(int)pid);

	fd = fopen (outFile, "w");
	if (fd == NULL)
	{
		fprintf (stderr, "Unable to open file for alias printing\n");
		SetBaseWindowMessage (AliasWindowP,GetGizmoText (TXT_CANT_OPEN_4_PRINT));
		return;
	}
	fprintf (fd, "%s\n\n%-20s %-59s\n\n", GetGizmoText(TXT_PRINT_TITLE), 
			GetGizmoText(TXT_TITLE_NAME), GetGizmoText(TXT_TITLE_ADDR));

	for (i = 0; i < aliasRec->name->used; i++)
	{
		fprintf (fd, "%-20s %-s\n",aliasRec->name->p[i],aliasRec->addr->p[i]);
	}

	if (fclose (fd) != 0)
	{
		fprintf (stderr, "Unable to close alias print file\n");
		return;
	}

	sprintf (Command, "PrtMgr %s",outFile);

	/* Workaround, part 2:                                               */
	/* We've release the pointer.  The menu is free.  Now is the time    */
	/* actually executing the print command.                             */

	/* "Flush" pending events from the input queue.  There seems to be   */
	/* a bug in how Xt handles timeouts.  It doesn't seem to process my  */
	/* pending events (e.g. take down menu, ungrab pointer) until after  */
	/* the time out is done (which is why I needed it in the first place!*/

	while (XtPending())
	{
		XEvent event;

		XtNextEvent(&event);
		XtDispatchEvent(&event);
	}

	/* LATER: do this with pipes instead of files */

    retCode = system(Command);

	if (retCode == 0) {
		SetBaseWindowMessage (AliasWindowP,GetGizmoText (TXT_ALIAS_2_PRINTER));
	}
	else {
		SetBaseWindowMessage (AliasWindowP,GetGizmoText (TXT_PRINT_FAILED));
	}

	sprintf (Command, "/bin/rm %s",outFile);
	if (system (Command) == -1)
	{
		fprintf (stderr, "Unable to remove alias print file\n");
		return;
	}
}


void
AliasExitCB (widg, client_data, call_data)
Widget			widg;
XtArgVal		client_data;
OlFlatCallData *	call_data;
{ 
	/***  fprintf (stderr,"AliasExitCB, check if we should terminate <%#x>\n", aliasRec); /***/
	if (aliasRec->unsaved)
	{	/* ask the user */
		if (QueryGizmo (ModalGizmoClass, sureNoticeP,
						GetGizmoWidget, "sureNotice") == (XtPointer) 0)
		{   /* first time; create Gizmo */
			CreateGizmo (Root, ModalGizmoClass, sureNoticeP, 0, 0);
		}
		MapGizmo (ModalGizmoClass, sureNoticeP);
		return;
	}
	AliasExit();
}

void
AliasExitYesCB (widg, client_data, call_data)
Widget			widg;
XtArgVal		client_data;
OlFlatCallData *	call_data;
{ 
	/***  fprintf (stderr,"AliasExitYesCB, about to terminate <%#x>\n", aliasRec); /***/

	XtPopdown ((Widget) _OlGetShellOfWidget (widg));

	AliasExit();

	return;
}


void
AliasExit ()
{ 
	/* Bring all popups down */
	if (aliasRec->aliasUndeletePopup != (PopupGizmo *)0) {
		XtPopdown (GetPopupGizmoShell (aliasRec->aliasUndeletePopup));
	}

	if (aliasRec->mapped == True) {
		UnmapShell (GetBaseWindowShell (AliasWindowP));
		aliasRec->mapped = False;
	}

	/***  fprintf (stderr,"AliasExit, terminating <%#x>\n", aliasRec); /***/
}


void
AliasSaveCB (widg, client_data, call_data)
Widget			widg;
XtArgVal		client_data;
OlFlatCallData *	call_data;
{ 
FILE *			fd;
TextBuffer *	textBuffer = 
					ReadFileIntoTextBuffer(aliasRec->userMailrc, NULL, NULL);
int				i;
int				j;
char *			p;
char *			q;
char *			outP;
Boolean			cont = False;

	/***  fprintf (stderr, "AliasSaveCB begin\n"); /***/
	fd = fopen (aliasRec->userMailrc, "w");
	if (fd == NULL)
	{
		fprintf (stderr, "Unable to open user .mailrc file\n");
		SetBaseWindowMessage (AliasWindowP,GetGizmoText(TXT_MAILRC_UNREADABLE));
		return;
	}

	if (textBuffer != NULL)
	{
		for (i = 0; i < LinesInTextBuffer(textBuffer); i++)
		{
			p = GetTextBufferLine(textBuffer, i);
			/***  fprintf (stderr, "From .mailrc: <%s>\n",p); /***/
			for (q = p; *q && (*q == ' ' || *q == '	'); q++); /* blank or tab */

			if ((strncmp(q, "alias ", 6)==0 || strncmp(q, "alias	", 6)==0) ||
				(strncmp(q, "group ", 6)==0 || strncmp(q, "group	", 6)==0) ||
				(cont == True))
			{
				/* these are alias lines - don't print them out
				/***  fprintf (stderr, "Don't write: <%s>\n",p); /***/
				if (q[(j = strlen (q)-1)] == '\\')
				{
					cont = True;
				}
				else {
					cont = False;
				}
			}
			else
			{
				cont = False;
				/* just write out what was read in */
				fprintf (fd, "%s\n",p);
				/***  fprintf (stderr, "write: <%s>\n",p); /***/
			}
			FREE(p);
		}
		FreeTextBuffer(textBuffer, (TextUpdateFunction)0, (caddr_t)0);
	}

	/* now write out the alias information for user aliases only */
	for (i = 0; i < aliasRec->userCount; i++)
	{
		outP = GenerateAliasTable 
						(aliasRec->name->p[i], aliasRec->userAlias, False);	
		/* write out alias line for what was read in */
		fprintf (fd, "alias %s %s\n",aliasRec->name->p[i],outP);
		FREE (outP);
	}

	aliasRec->unsaved = False;		/* the user is now up-to-date */

	if (fclose (fd) != 0)
	{
		fprintf (stderr, "Unable to close alias print file\n");
		return;
	}

	SetBaseWindowMessage (AliasWindowP,GetGizmoText (TXT_MAILRC_WRITTEN));

	return;
}



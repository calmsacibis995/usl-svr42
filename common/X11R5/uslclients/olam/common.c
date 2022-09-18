/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:common.c	1.15"
#endif

/*
** common.c - This file contains the functions (mostly callbacks and their
** helpers) that are common to both the Outgoing Remote Display and Accepted
** Remote Hosts popups.
*/


#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/MenuShell.h>
#include <Xol/FButtons.h>

#include "errors.h"
#include "common.h"


static void	ApplyMain();
static void	NoCallback();
static void	Quit();
static void	YesCallback();


int	num_popups = 0;			/* Incremented for each pop-up, */
					/* decremented for each pop-down, */
					/* process exits when 0 */

typedef struct {
	XtArgVal label;
	/*void (*cb)();*/ XtArgVal cb;
	XtArgVal data;
	XtArgVal sensitive;
	XtArgVal popupmenu;
	XtArgVal dflt;
	XtArgVal mnemonic;
} ButtonItem;

String buttonfields[] = {
	XtNlabel,
	XtNselectProc,
	XtNclientData,
	XtNsensitive,
	XtNpopupMenu,
	XtNdefault,
	XtNmnemonic,
};


static ButtonItem InsertButton[] = {
{ (XtArgVal)"Before",(XtArgVal)InsertBefore,(XtArgVal)NULL, (XtArgVal)True,
			(XtArgVal) NULL, (XtArgVal)False, (XtArgVal)'B'},
{(XtArgVal)"After", (XtArgVal)InsertAfter, (XtArgVal)NULL, (XtArgVal)True,
			(XtArgVal)NULL, (XtArgVal)True,(XtArgVal) 'A'},
};
	
static ButtonItem  controlitems[] = {
 { (XtArgVal)"Insert", (XtArgVal)NULL, (XtArgVal)NULL, (XtArgVal)True,(XtArgVal) NULL,(XtArgVal) False, (XtArgVal)'I'},
 { (XtArgVal)"Delete", (XtArgVal)Delete, (XtArgVal)NULL, (XtArgVal)True, (XtArgVal)NULL, (XtArgVal)False, (XtArgVal)'D'},
 { (XtArgVal)"Apply Edit", (XtArgVal)NULL, (XtArgVal)NULL, (XtArgVal)True,(XtArgVal) NULL, (XtArgVal)False,(XtArgVal) 'E'},
};

/*
** Callback for the "Apply" button.
** If anything has changed, save to disk, else give an appropriate
** message.
*/
void
Apply(w, client_data, call_data)
  Widget	w;
  XtPointer	client_data;
  XtPointer	call_data;
{
  int			i;		/* Loops through busy widgets */
  FILE			*stream;	/* Stream to write to */
  CommonStuffPtr	stuff;		/* Current popup's context */


  stuff = (CommonStuffPtr)client_data;

  ClearFooter(stuff->footer);		/* Clear footer on any operation */

  if (stuff->changed)			/* Something has changed */
    if ((stream = fopen(stuff->file_name, "w")) == (FILE *)NULL)
      {					/* File isn't writable; issue */
					/* message and insensitize the */
					/* appropriate widgets */
        FooterMsg(stuff->footer,
                            OlGetMessage(XtDisplay(w), NULL,
                            0,
                            OleNfilecommon,
                            OleTmsg1,
                            OleCOlClientOlamMsgs,
                            OleMfilecommon_msg1,
                            (XrmDatabase)NULL),
		            stuff->file_name);
	/*
	for (i = 0; i < NUM_BUSY_WIDGETS; ++i)
	  XtSetSensitive(stuff->busy_widget[i], FALSE);
	 */
	for (i = 0; i < XtNumber(controlitems); ++i)
		OlVaFlatSetValues(stuff->busy_widget, i,
			XtNsensitive, FALSE,
			(char *)0);
	stuff->file_is_writeable = FALSE;
      }
    else				/* fopen() was successful */
      {
	/*
	** Apply changes.
	*/
	ApplyMain(stuff, stream);
	if (fclose(stream) == EOF)
          error_exit( OlGetMessage(XtDisplay(w), NULL,
                        0,
                        OleNfilecommon,
                        OleTmsg2,
                        OleCOlClientOlamMsgs,
                        OleMfilecommon_msg2,
                        (XrmDatabase)NULL),
                        stuff->file_name, 2);
	stuff->changed = FALSE;
      }
  else					/* Nothing has been changed */
        FooterMsg(stuff->footer,
                            OlGetMessage(XtDisplay(w), NULL,
                            0,
                            OleNfilecommon,
                            OleTmsg3,
                            OleCOlClientOlamMsgs,
                            OleMfilecommon_msg3,
                            (XrmDatabase)NULL),
		            NULL);

  /*
  ** VerifyPopdown() checks this piece of context to allow/disallow
  ** popdowns on the appropriate buttons in the lower control area.
  */
  stuff->allow_popdown = TRUE;		/* Allow popdown on "Apply" */
					/* button-press */

}	/* Apply() */


/*
** Save contents of list to the appropriate file.
** Traverse internal list writing non-empty entries to `stream'.
*/ 
static void
ApplyMain(stuff, stream)
  CommonStuffPtr	stuff;
  FILE			*stream;
{
  register List		linked_list;	/* List of entries */
  register ListPosition	pos;		/* Traverses `linked_list' */
  register String	string;		/* Points to current entry's label */
  register OlListToken	token;		/* Points to current entry's token */


  linked_list = stuff->pfsl->linked_list;
  pos = ListHead(linked_list);

  while (pos != LIST_NULL_POS)		/* Traverse list to end */
    {
      token = (OlListToken)ListGet(linked_list, pos);
      /*
      ** Get scrolling list entry
      */
      string = OlListItemPointer(token)->label;

      if (string != NULL && *string != '\0')
	{				/* Got a non-empty item */
          DEBUG_MSG( OlGetMessage(XtDisplay(w), NULL,
                     0,
                     OleNfilecommon,
                     OleTmsg4,
                     OleCOlClientOlamMsgs,
                     OleMfilecommon_msg4,
                     (XrmDatabase)NULL),
                     string);
	  if (fprintf(stream, "%s\n", string) < 0)
          error_exit( OlGetMessage((Display *)NULL, NULL,
                        0,
                        OleNfilecommon,
                        OleTmsg5,
                        OleCOlClientOlamMsgs,
                        OleMfilecommon_msg5,
                        (XrmDatabase)NULL),
                        stuff->file_name, 2);
	}
      else				/* Item is empty */
	{
	  /*
	  ** Delete empty entry.
	  */
          DEBUG_MSG( OlGetMessage(XtDisplay(shell), NULL,
                     0,
                     OleNfilecommon,
                     OleTmsg6,
                     OleCOlClientOlamMsgs,
                     OleMfilecommon_msg6,
                     (XrmDatabase)NULL),
                     NULL);
	  PFSLdelete(stuff->pfsl, token);
	}

      pos = ListNext(linked_list, pos);	/* Get next node */
    }

}	/* ApplyMain() */


/*
** Callback for the "Delete" button.
** Delete selected item if there is one, else give a message.
*/
void
Delete(w, client_data, call_data)
  Widget	w;
  XtPointer	client_data;
  XtPointer	call_data;
{
  String		string;		/* Deleted item's label */
  CommonStuffPtr	stuff;		/* Current popup's context */


  stuff = (CommonStuffPtr)client_data;

  ClearFooter(stuff->footer);		/* Clear footer on any operation */

  if (stuff->pfsl->current_token != PFSL_NULL_TOKEN)
    {					/* There is a selected item; */
					/* delete it */
      string = OlListItemPointer(stuff->pfsl->current_token)->label;
      if (string != NULL && *string != '\0')
	{
	  /*
	  ** Only record a change if a non-empty item has been deleted
	  */
          DEBUG_MSG( OlGetMessage(XtDisplay(w), NULL,
                     0,
                     OleNfilecommon,
                     OleTmsg7,
                     OleCOlClientOlamMsgs,
                     OleMfilecommon_msg7,
                     (XrmDatabase)NULL),
                     string);
	  stuff->changed = TRUE;
	}

      PFSLdeleteCurrent(stuff->pfsl);
    }
  else					/* No item has been selected */
        FooterMsg(stuff->footer,
                            OlGetMessage(XtDisplay(w), NULL,
                            0,
                            OleNfilecommon,
                            OleTmsg8,
                            OleCOlClientOlamMsgs,
                            OleMfilecommon_msg8,
                            (XrmDatabase)NULL),
		            NULL);

}	/* Delete() */


/*
** Callback for dismissal (by the window manager) of a popup.
** If something has changed popup a notice asking if changes should be
** saved, else quit.
*/
void
Dismiss(w, client_data, call_data)
  Widget	w;
  XtPointer	client_data;
  XtPointer	call_data;
{
  CommonStuffPtr	stuff;		/* Current popup's context */


  stuff = (CommonStuffPtr)client_data;

  if (stuff->changed)			/* Change made, but not "Apply"'d */
    /*
    ** Popup notice asking to save changes.
    */
    XtPopup(stuff->save_notice, XtGrabNone);
  else					/* Nothing has changed */
    Quit();

}	/* Dismiss() */


/*
** Fills list with the contents of the associated file.
** Mostly checks statuses and issues appropriate messages and then
** calls PFSLfill to do the real work.
*/
void
FillList(stuff)
  CommonStuffPtr	stuff;
{
  int		exists;			/* Non-zero if file exists */
  register int	i;			/* Loops through busy widgets */
  int		readable;		/* Non-zero if file is readable or */
					/* directory is readable and file */
					/* doesn't exist (and has higher */
					/* precedence) */
  unsigned	stat_flags;		/* Mask returned by OpenFile() */
  FILE		*stream;		/* Stream to read entries from */
  int		writeable;		/* Non-zero if file is writable or */
					/* directory is writable and file */
					/* doesn't exist */


  stream = OpenFile(stuff->file_name, &stat_flags);

  exists = BIT_IS_SET(stat_flags, FILE_EXISTS);
  readable = BIT_IS_SET(stat_flags, FILE_READABLE);
  writeable = BIT_IS_SET(stat_flags, FILE_WRITEABLE);

  if (exists & readable & writeable)
    {
      if (stream == (FILE *)NULL)	/* File couldn't be opened despite */
					/* correct permissions */
          error_exit( OlGetMessage((Display *)NULL, NULL,
                        0,
                        OleNfilecommon,
                        OleTmsg9,
                        OleCOlClientOlamMsgs,
                        OleMfilecommon_msg9,
                        (XrmDatabase)NULL),
                        stuff->file_name);
    }
  else if (!exists & !writeable)	/* Doesn't exist and directory isn't */
					/* writable */
        FooterMsg(stuff->footer,
                            OlGetMessage((Display *)NULL, NULL,
                            0,
                            OleNfilecommon,
                            OleTmsg10,
                            OleCOlClientOlamMsgs,
                            OleMfilecommon_msg10,
                            (XrmDatabase)NULL),
		            stuff->file_name);
  else if (!readable & (!exists & !writeable | exists))
					/* File isn't readable */
        FooterMsg(stuff->footer,
                            OlGetMessage((Display *)NULL, NULL,
                            0,
                            OleNfilecommon,
                            OleTmsg11,
                            OleCOlClientOlamMsgs,
                            OleMfilecommon_msg11,
                            (XrmDatabase)NULL),
		            stuff->file_name);
  else if (exists & readable & !writeable)
					/* Can look at it but not modify it */
        FooterMsg(stuff->footer,
                            OlGetMessage((Display *)NULL, NULL,
                            0,
                            OleNfilecommon,
                            OleTmsg12,
                            OleCOlClientOlamMsgs,
                            OleMfilecommon_msg12,
                            (XrmDatabase)NULL),
		            stuff->file_name);

  if (stream != (FILE *)NULL)
    {
      PFSLfill(stuff->pfsl, stuff->get_line, stream);
      if (fclose(stream) == EOF)
          error_exit( OlGetMessage((Display *)NULL, NULL,
                        0,
                        OleNfilecommon,
                        OleTmsg2,
                        OleCOlClientOlamMsgs,
                        OleMfilecommon_msg2,
                        (XrmDatabase)NULL),
                        stuff->file_name, 2);
    }

  if (stuff->file_is_writeable != writeable)
    {					/* Writable state has changed */
      stuff->file_is_writeable = writeable;
      /*
      ** Update appropriate widgets to reflect the new state
      */
	for (i = 0; i < XtNumber(controlitems); ++i)
		OlVaFlatSetValues(stuff->busy_widget, i,
			XtNsensitive, (XtArgVal)stuff->file_is_writeable,
			(char *)0);
    }
  
}	/* FillList() */


/*
** Callback for "After" button from the "Insert" button stack.
** Open a new item in the list after the current one.
*/
void
InsertAfter(w, client_data, call_data)
  Widget	w;
  XtPointer	client_data;
  XtPointer	call_data;

{
  CommonStuffPtr	stuff;		/* Current popup's context */


  stuff = (CommonStuffPtr)client_data;

  ClearFooter(stuff->footer);		/* Clear footer on any operation */

  PFSLopen(stuff->pfsl, PFSL_AFTER);	/* Open new item */

}	/* InsertAfter() */


/*
** Callback for "Before" button from the "Insert" button stack.
** Open a new item in the list before the current one.
*/
void
InsertBefore(w, client_data, call_data)
  Widget	w;
  XtPointer	client_data;
  XtPointer	call_data;

{
  CommonStuffPtr	stuff;		/* Current popup's context */


  stuff = (CommonStuffPtr)client_data;

  ClearFooter(stuff->footer);		/* Clear footer on any operation */

  PFSLopen(stuff->pfsl, PFSL_BEFORE);	/* Open new item */

}	/* InsertBefore() */


/*
** Populate lower portion of popup window (ie the buttons).
** Create "Insert" menu, "Delete" and "Apply Edits" buttons, and the
** save-changes notice.  These widgets are "busy widgets"; they become
** insensitive when the respective file cannot be written.
** Note: The "Apply" and "Reset" buttons are created in CreatePopup().
*/
void
PopulateLower(control, stuff)
  Widget		control;
  CommonStuffPtr	stuff;
{
  Widget	w;			/* Points to menu buttons to allow */
					/* set-values */

/* Create a menu shell for the insert button */
	controlitems[0].popupmenu =
		(XtArgVal)XtVaCreatePopupShell("menu",
			popupMenuShellWidgetClass,
			topshell,
			XtNpushpin, OL_NONE,
			(char *)0);
	controlitems[2].cb = (XtArgVal)stuff->apply_edit;
	InsertButton[0].label = (XtArgVal)OlGetMessage(XtDisplay(control), NULL, 0,
				OleNbutton, OleTbefore, OleCOlClientOlamMsgs,
				OleMbutton_before, (XrmDatabase)NULL);
	InsertButton[0].mnemonic = (XtArgVal)*(OlGetMessage(XtDisplay(control), NULL, 0,
				OleNmnemonic, OleTbefore, OleCOlClientOlamMsgs,
				OleMmnemonic_before, (XrmDatabase)NULL));
	InsertButton[1].label = (XtArgVal)OlGetMessage(XtDisplay(control), NULL, 0,
				OleNbutton, OleTafter, OleCOlClientOlamMsgs,
				OleMbutton_after, (XrmDatabase)NULL);
	InsertButton[1].mnemonic = (XtArgVal)*(OlGetMessage(XtDisplay(control), NULL, 0,
				OleNmnemonic, OleTafter, OleCOlClientOlamMsgs,
				OleMmnemonic_after, (XrmDatabase)NULL));

	w = XtVaCreateManagedWidget("insert", flatButtonsWidgetClass,
				(Widget)controlitems[0].popupmenu,
				XtNrecomputeSize, (XtArgVal)True,
				XtNlayoutType, (XtArgVal)OL_FIXEDCOLS,
				XtNmeasure, (XtArgVal)1,
				XtNitemFields, (XtArgVal)buttonfields,
				XtNnumItemFields, (XtArgVal)XtNumber(buttonfields),
				XtNitems, (XtArgVal)InsertButton,
				XtNnumItems, (XtArgVal)XtNumber(InsertButton),
				(char *) 0);
	controlitems[0].label =  (XtArgVal)OlGetMessage(XtDisplay(control), NULL, 0,
				OleNtitle, OleTinsert, OleCOlClientOlamMsgs,
				OleMtitle_insert, (XrmDatabase)NULL);
	controlitems[0].mnemonic =  (XtArgVal)*(OlGetMessage(XtDisplay(control), NULL, 0,
				OleNmnemonic, OleTinsert, OleCOlClientOlamMsgs,
				OleMmnemonic_insert, (XrmDatabase)NULL));
	controlitems[1].label =  (XtArgVal)OlGetMessage(XtDisplay(control), NULL, 0,
				OleNbutton, OleTdel, OleCOlClientOlamMsgs,
				OleMbutton_del, (XrmDatabase)NULL);
	controlitems[1].mnemonic =  (XtArgVal)*(OlGetMessage(XtDisplay(control), NULL, 0,
				OleNmnemonic, OleTdel, OleCOlClientOlamMsgs,
				OleMmnemonic_del, (XrmDatabase)NULL));

	controlitems[2].label =  (XtArgVal)OlGetMessage(XtDisplay(control), NULL, 0,
				OleNbutton, OleTapplyEdit, OleCOlClientOlamMsgs,
				OleMbutton_applyEdit, (XrmDatabase)NULL);
	controlitems[2].mnemonic = (XtArgVal) *(OlGetMessage(XtDisplay(control), NULL, 0,
				OleNmnemonic,OleTapplyEdit,OleCOlClientOlamMsgs,
				OleMmnemonic_applyEdit, (XrmDatabase)NULL));

	controlitems[1].data = controlitems[2].data =
			InsertButton[0].data= InsertButton[1].data=
							 (XtArgVal)stuff;

  /*
  ** Attach "Before" and "After" buttons to `insert_menu'
  */
	stuff->busy_widget = XtVaCreateManagedWidget("control",
				flatButtonsWidgetClass, control,
				XtNrecomputeSize, True,
				XtNlayoutType, OL_FIXEDROWS,
				XtNmeasure, 1,
				XtNitemFields, buttonfields,
				XtNnumItemFields, XtNumber(buttonfields),
				XtNitems, controlitems,
				XtNnumItems, XtNumber(controlitems),
				(char *) NULL);

  stuff->save_notice = CreateYesNoNotice( OlGetMessage(XtDisplay(control),
                                          NULL, 0,                                                                        OleNfilecommon,
                                          OleTmsg13,
                                          OleCOlClientOlamMsgs,
                                          OleMfilecommon_msg13,
                                          (XrmDatabase)NULL),
					  YesCallback, Quit,
					  (XtPointer)stuff,
					  control); /* last arg = a parent */

}	/* PopulateLower() */


/*
** Quit application.
** Check the number of popups that are still up, if 0 then exit.
*/
static void
Quit()
{
  extern void	exit();


  if (--num_popups <= 0)
    exit(0);

}	/* Quit() */


/*
** Callback for "Reset" button.
** If something has changed, refill the list, else issue a message.  Doesn't
** allow pressing the "Reset" button pop-down the window.
*/
void
Reset(w, client_data, call_data)
  Widget	w;
  XtPointer	client_data;
  XtPointer	call_data;
{
  CommonStuffPtr	stuff;		/* Current popup's context */

  
  stuff = (CommonStuffPtr)client_data;

  ClearFooter(stuff->footer);		/* Clear footer on any operation */

  if (stuff->changed)			/* Something has changed */
    {
      PFSLdeleteAll(stuff->pfsl);	/* Delete all the items */
      FillList(stuff);			/* Fill the list again */
      stuff->changed = FALSE;
    }
  else					/* Nothing has changed */
        FooterMsg(stuff->footer,
                            OlGetMessage(XtDisplay(w), NULL,
                            0,
                            OleNfilecommon,
                            OleTmsg14,
                            OleCOlClientOlamMsgs,
                            OleMfilecommon_msg14,
                            (XrmDatabase)NULL),
		            NULL);

  stuff->allow_popdown = FALSE;		/* Disallow popdown on "Reset" */
					/* button-press */

}	/* Reset() */


/*
** Callback from the PopupWindow to verify that a button press on one of its
** auto-children should pop the window down.
** The context member `allow_popdown' will be true if "Apply" was pressed,
** and false if "Reset" was pressed.
*/
void
VerifyPopdown(w, client_data, call_data)
  Widget	w;
  XtPointer	client_data;
  XtPointer	call_data;
{
  Boolean	*allow_popdown;


  /*
  ** `call_data' points to a `Boolean' that is checked by the PopupWindow
  ** to determine whether the window should be popped-down when a
  ** button that it provided is pressed.
  */
  allow_popdown = (Boolean *)call_data;
  *allow_popdown = ((CommonStuffPtr)client_data)->allow_popdown;

}	/* VerifyPopdown() */


/*
** Callback for the "Yes" button in the save-changes notice.
** Apply the changes and then quit.
*/
static void
YesCallback(w, client_data, call_data)
  Widget	w;
  XtPointer	client_data;
  XtPointer	call_data;
{

  Apply(w, client_data, call_data);
  Quit();

}	/* YesCallback() */

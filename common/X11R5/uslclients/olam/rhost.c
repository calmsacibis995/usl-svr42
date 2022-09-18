/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:rhost.c	1.11"
#endif

/*
** rhost.c - This file contains the routines that are specific to the
** "Accepted Remote Hosts" popup.
**
** These routines are meant to administer "/etc/X0.hosts" (We only have one
** display per machine; although administering "/etc/X?.hosts" may come in
** the future.  This file is read at server start-up and contains a list of
** machines from which client connections are accepted.
*/


#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>

#include "errors.h"
#include "common.h"
#include "rhost.h"


static void	ApplyEdit();		/* Callback for "Apply Edits" button */
static String	GetHost();		/* Get a host from a file */
static void	InitializeHostStuff();	/* Initializes context */
static void	PopulateUpper();	/* Populates the upper part of the */
					/* popup */
static void	SelectItem();		/* Callback for item selection in */
					/* scrolling list */


static Widget	host_name;		/* Text field widget */


/*
** This routine creates the popup for the "Accepted Remote Hosts".
*/
Widget
CreateRhostPopup()
{
  Widget		footer;		/* Static text widget in footer */
  static CommonStuff	host_stuff;	/* Context for this popup */
  Widget		popup;		/* Popup window widget */
  Widget		lower_control;	/* Lower part of `popup' */
  Widget		upper_control;	/* Upper part of `popup' */

  popup = CreatePopup( OlGetMessage(XtDisplay(topshell), NULL, 0,
					OleNtitle, OleTpopupIn,
					OleCOlClientOlamMsgs,
					OleMtitle_popupIn,
					(XrmDatabase)NULL),
		       Apply, Reset, Dismiss, (XtPointer)&host_stuff,
		       &upper_control, &lower_control, &footer);

  REGISTER_HELP(popup, OlGetMessage(XtDisplay(topshell), NULL, 0,
					OleNtitle, OleThostHelp,
					OleCOlClientOlamMsgs,
					OleMtitle_hostHelp,
					(XrmDatabase)NULL),
			HOSTHELPFILE);

  /*
  ** Increment count of application's popups
  */
  ++num_popups;

  /*
  ** Initialize the context information
  */
  InitializeHostStuff(&host_stuff, footer);

  /*
  ** Populate control areas
  */
  PopulateUpper(upper_control, &host_stuff);
  PopulateLower(lower_control, &host_stuff);

  /*
  ** Fill the scrolling list
  */
  FillList(&host_stuff);

  return popup;

}	/* CreateRhostPopup() */


/*
** Callback for "Apply Edit" button.
** This popup's `apply_edit' context member points to this function.
** Changes the current scrolling list item to contain the contents
** of the "Host Name" text field.
*/
static void
ApplyEdit(w, client_data, call_data)
  Widget	w;
  XtPointer	client_data;
  XtPointer	call_data;
{
  static char		*string;	/* Contents of text field */
  static Arg		arg[] = {{XtNstring, (XtArgVal)&string}};
					/* Arg. used to grab `string' from */
					/* text field; these variables can */
					/* be static since the scrolling */
					/* list makes its own copy */
  CommonStuffPtr	stuff;		/* Current popup's context */

  
  stuff = (CommonStuffPtr)client_data;

  ClearFooter(stuff->footer);		/* Clear footer on any operation */

  if (stuff->pfsl->current_token != PFSL_NULL_TOKEN)
    {					/* Something is selected */
      /*
      ** Get contents of text field
      */
      XtGetValues(host_name, arg, XtNumber(arg));
      
      /*
      ** If the field is valid, change the current item
      */
      if (ValidateString(string, "", stuff->footer))
	{
            DEBUG_MSG( OlGetMessage(XtDisplay(w), NULL,
                       0,
                       OleNfilerhost,
                       OleTmsg1,
                       OleCOlClientOlamMsgs,
                       OleMfilerhost_msg1,
                       (XrmDatabase)NULL),
                       string);

	  PFSLchange(stuff->pfsl, string);
	  stuff->changed = TRUE;	/* Flag the change */
	}
    }
  else					/* No current item */
    FooterMsg(stuff->footer, 
                            OlGetMessage(XtDisplay(w), NULL,
                            0,
                            OleNfilerhost,
                            OleTmsg2,
                            OleCOlClientOlamMsgs,
                            OleMfilerhost_msg2,
                            (XrmDatabase)NULL),
                            NULL);

}	/* ApplyEdit() */


/*
** Returns a host name from `stream'.
** This function is assigned to the `get_line' member of this popup's
** context.  That member is in turn used as the second argument in calls to
** PFSLfill().
*/
static String
GetHost(stream)
  FILE	*stream;
{

  return GetLine(stream, ExtractHost);

}	/* GetHost() */


/*
** Initialize this popup's context.
*/
static void
InitializeHostStuff(stuff, footer)
  CommonStuffPtr	stuff;
  Widget		footer;
{

  stuff->apply_edit = ApplyEdit;	/* Callback for "Apply Edit" button */
  stuff->changed = FALSE;		/* Flag for user-made changes */
  stuff->file_is_writeable = TRUE;	/* Assume file is writable */
  stuff->footer = footer;		/* Static text in footer */
  (void)strncpy(stuff->file_name, HOSTFILE, (size_t)(MAXPATHLEN - 1));
					/* File to operate on */
  stuff->get_line = GetHost;		/* Returns a line for filling the */
					/* list */

}	/* InitializeHostStuff() */


/*
** Populates the upper part of the popup with a scrolling list and the text
** field.
*/
static void
PopulateUpper(control, stuff)
  Widget		control;
  CommonStuffPtr	stuff;
{

  stuff->pfsl = PFSLnew(control, OlGetMessage(XtDisplay(control), NULL, 0,
				OleNcaption, OleThost,
                		OleCOlClientOlamMsgs, OleMcaption_host,
                		(XrmDatabase)NULL),
 			SelectItem,
			(Cardinal)HOSTVIEWWIDTH, (Cardinal)HOSTVIEWHEIGHT);

  if (stuff->pfsl == PFSL_ERROR)
    error_exit( OlGetMessage(XtDisplay(control), NULL,
                0,
                OleNfilerhost,
                OleTmsg3,
                OleCOlClientOlamMsgs,
                OleMfilerhost_msg3,
                (XrmDatabase)NULL),
                NULL, 2);

 host_name = CreateTextField(control,
	OlGetMessage(XtDisplay(control), NULL, 0,
		OleNtextField, OleThost, OleCOlClientOlamMsgs,
		OleMtextField_host, (XrmDatabase)NULL),
 	ValidateNoSpace, (XtPointer)stuff->footer);

}	/* PopulateUpper() */


/*
** Callback for item selection in scrolling list (userMakeCurrent).
** Updates the text field with the contents of the selected entry.
*/
static void
SelectItem(w, client_data, call_data)
  Widget	w;
  XtPointer	client_data;
  XtPointer	call_data;
{
  Arg		arg[1];			/* @ */
  String	string;			/* Selected item's label */
  OlListToken	token;			/* Current item's token */


  token = (OlListToken)call_data;

  if (token != PFSL_NULL_TOKEN)		/* There is a current item */
    string = OlListItemPointer(token)->label;
  else					/* There is no current item */
    string = "";

  /*
  ** Update the text field with the contents of the current item
  */
  XtSetArg(arg[0], XtNstring, string);
  XtSetValues(host_name, arg, XtNumber(arg));

}	/* SelectItem() */

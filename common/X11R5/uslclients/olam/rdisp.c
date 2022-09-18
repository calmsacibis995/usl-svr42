/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:rdisp.c	1.12"
#endif

/*
** rdisp.c - This file contains the routines that are specific to the
** "Outgoing Remote Display" popup.
**
** These routines are meant to administer "/usr/X/lib/Xconnections" where
** each line is an entry of the form
**	<display name> <host name> <netspec>
** These entries are used by OPEN LOOK(tm) (XWIN 1.1 or later) clients to
** connect to the specified display on the specified remote host using the
** specified network type.  Valid network types depend on the networking
** software that has been installed.  "starlan" and "it" are the most
** common, representing STARLAN and TCP/IP respectively.  The actual netspec
** strings correspond to the name of the driver (without the leading "/dev")
** used to access the particular network.
**
** An entry with a "*" for both the Display Name and Host Name fields
** matches all display specifications.  This directs all connection  
** requests to use the specified network type.  There should usually be at
** most one entry of this type at the end of the list. 
**
** Entries are tried in order until a successful connection is made, an
** entry with "*" for both the Display Name and Host Name is tried, or the
** end of the list is reached. 
**
** Example:
** An entry containing "my6386-s", "my6386", and "starlan", would allow you
** to connect to the machine called "my6386" over the STARLAN network by
** using "my6386-s:0.0" as the display specification. 
**
** Example:
** An entry containing "*", "*", and "it", would allow you to connect to any
** machine over the TCP/IP network by using "<machine-name>:0.0" as the
** display specification (Provided <machine-name> does not match a preceding
** Display Name entry that produces a successful connection.). 
*/


#ifndef MEMUTIL
#include <malloc.h>
#else
#include <X11/memutil.h>
#endif
#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>

#include "errors.h"
#include "common.h"
#include "rdisp.h"

static void	ApplyEdit();		/* Callback for "Apply Edits" button */
static String	GetDisp();		/* Gets a display entry from a file */
static void	InitializeDispStuff();	/* Initializes context */
static void	PopulateUpper();	/* Populates the upper part of the */
					/* popup */
static void	SelectItem();		/* Callback for item selection in */
					/* scrolling list */


/*
** The three text field widgets for "Display Name", "Host Name", and
** "Netspec".
*/
static Widget		disp_field;
static Widget		host_field;
static Widget		netspec_field;


/*
** This routine creates the popup for the "Outgoing Remote Displays".
*/
Widget
CreateRdispPopup()
{
  static CommonStuff	disp_stuff;	/* Context for this popup */
  Widget		footer;		/* Static text widget in footer */
  Widget		popup;		/* Popup window widget */
  Widget		lower_control;	/* Lower part of `popup' */
  Widget		upper_control;	/* Upper part of `popup' */


            popup = CreatePopup( OlGetMessage(XtDisplay(topshell), NULL,
                        	0,
                        	OleNtitle,
                        	OleTpopupOut,
                        	OleCOlClientOlamMsgs,
                        	OleMtitle_popupOut,
                        	(XrmDatabase)NULL),
                       Apply, Reset, Dismiss, (XtPointer)&disp_stuff,
                       &upper_control, &lower_control, &footer);

  		REGISTER_HELP(popup, OlGetMessage(XtDisplay(popup), NULL, 0,
                        	OleNtitle,
                        	OleTdispHelp,
                        	OleCOlClientOlamMsgs,
                        	OleMtitle_dispHelp,
                        	(XrmDatabase)NULL),
				DISPHELPFILE);

  /*
  ** Increment count of application's popups
  */
  ++num_popups;

  /*
  ** Initialize the context information
  */
  InitializeDispStuff(&disp_stuff, footer);

  /*
  ** Populate control areas
  */
  PopulateUpper(upper_control, &disp_stuff);
  PopulateLower(lower_control, &disp_stuff);

  /*
  ** Fill the scrolling list
  */
  FillList(&disp_stuff);

  return popup;

}	/* CreateRdispPopup() */


/*
** Callback for "Apply Edit" button.
** This popup's `apply_edit' context member points to this function.
** Changes the current scrolling list item to contain the formated string
** containing the contents of the three text fields.
*/
static void
ApplyEdit(w, client_data, call_data)
  Widget	w;
  XtPointer	client_data;
  XtPointer	call_data;
{
  Arg			arg[1];
  char			*disp_string;	/* Contents of "Display Name" field */
  char			*host_string;	/* Contents of "Host Name" field */
  char			*netspec_string;
					/* Contents of "Netspec" field */
  char			*new_entry;	/* Formatted combination of 3 above */
  char			*new_entry_tmp[MAXLINE]; /* @ */
  CommonStuffPtr	stuff;		/* Current popup's context */



  stuff = (CommonStuffPtr)client_data;

  ClearFooter(stuff->footer);		/* Clear footer on any operation */

  if (stuff->pfsl->current_token != PFSL_NULL_TOKEN)
    {					/* Something is selected */
      /*
      ** Get contents of text fields
      */
      XtSetArg(arg[0], XtNstring, (XtArgVal)&disp_string);
      XtGetValues(disp_field, arg, (Cardinal)1);

      XtSetArg(arg[0], XtNstring, (XtArgVal)&host_string);
      XtGetValues(host_field, arg, (Cardinal)1);

      XtSetArg(arg[0], XtNstring, (XtArgVal)&netspec_string);
      XtGetValues(netspec_field, arg, (Cardinal)1);

      /*
      ** If all fields are valid, format them and change the current item.
      ** Note that the display name is not allowed to contain a ':' (besides
      ** the normal bad characters ValidateString() checks for).
      */
      if (ValidateString(disp_string, ":", stuff->footer) &&
	  ValidateString(host_string, "", stuff->footer) &&
	  ValidateString(netspec_string, "", stuff->footer))
	{
	  FormatDispEntry(disp_string, host_string, netspec_string,
			  new_entry_tmp);

	  /*
	  ** Copy the formated entry into some fresh memory @
	  */
	  if ((new_entry = malloc((unsigned)(strlen(new_entry_tmp) + 1)))
	      == NULL)
            error_exit( OlGetMessage(XtDisplay(w), NULL,
                        0,
                        OleNfilerdisp,
                        OleTmsg7,
                        OleCOlClientOlamMsgs,
                        OleMfilerdisp_msg7,
                        (XrmDatabase)NULL),
	                strlen(new_entry_tmp) + 1, 2);

	  (void)strcpy(new_entry, new_entry_tmp);
	  
	  /*
	  ** Update the list
	  */
          DEBUG_MSG( OlGetMessage(XtDisplay(w), NULL,
                     0,
                     OleNfilerdisp,
                     OleTmsg1,
                     OleCOlClientOlamMsgs,
                     OleMfilerdisp_msg1,
                     (XrmDatabase)NULL),
                     new_entry);

	  PFSLchange(stuff->pfsl, new_entry);

	  /*
	  ** Flag the change
	  */
	  stuff->changed = TRUE;
	}

      /*
      ** The text field allocates storage for the strings, so we have to
      ** free it
      */
      free(disp_string);
      free(host_string);
      free(netspec_string);
    }
  else					/* No current item */
          FooterMsg(stuff->footer,
                            OlGetMessage(XtDisplay(w), NULL,
                            0,
                            OleNfilerdisp,
                            OleTmsg2,
                            OleCOlClientOlamMsgs,
                            OleMfilerdisp_msg2,
                            (XrmDatabase)NULL),
                            NULL);


}	/* ApplyEdit() */


/*
** Returns a formated entry from `stream'.
** This function is assigned to the `get_line' member of this popup's
** context.  That member is in turn used as the second argument in calls to
** PFSLfill().
*/
static String
GetDisp(stream)
  FILE	*stream;
{

  return GetLine(stream, ExtractDisp);

}	/* GetDisp() */


/*
** Initialize this popup's context.
*/
static void
InitializeDispStuff(stuff, footer)
  CommonStuffPtr	stuff;
  Widget		footer;
{

  stuff->apply_edit = ApplyEdit;	/* Callback for "Apply Edit" button */
  stuff->changed = FALSE;		/* Flag for user-made changes */
  stuff->file_is_writeable = TRUE;	/* Assume file is writable */
  stuff->footer = footer;		/* Static text in footer */
  (void)strncpy(stuff->file_name, DISPFILE, (size_t)(MAXPATHLEN - 1));
					/* File to operate on */
  stuff->get_line = GetDisp;		/* Returns a line for filling the */
					/* list */
  
}	/* InitializeDispStuff() */


/*
** Populates the upper part of the popup with a scrolling list and the 3
** text fields.
*/
static void
PopulateUpper(control, stuff)
  Widget		control;
  CommonStuffPtr	stuff;
{

  stuff->pfsl = PFSLnew(control, OlGetMessage(XtDisplay(control), NULL, 0,
				OleNcaption,
				OleTdisplay,
				OleCOlClientOlamMsgs,
				OleMcaption_display,
				(XrmDatabase)NULL),
 			SelectItem,
			(Cardinal)DISPVIEWWIDTH, (Cardinal)DISPVIEWHEIGHT);
  if (stuff->pfsl == PFSL_ERROR)
    error_exit( OlGetMessage(XtDisplay(control), NULL,
                0,
                OleNfilerdisp,
                OleTmsg3,
                OleCOlClientOlamMsgs,
                OleMfilerdisp_msg3,
                (XrmDatabase)NULL),
                NULL, 2);

  disp_field = CreateTextField(control,
			OlGetMessage(XtDisplay(control), NULL, 0,
					OleNtextField, OleTdisplay,
					OleCOlClientOlamMsgs,
					OleMtextField_display,
					(XrmDatabase)NULL),
			ValidateNoSpaceOrColon, (XtPointer)stuff->footer);

  host_field = CreateTextField(control,
			OlGetMessage(XtDisplay(control), NULL, 0, OleNtextField,
				OleThost, OleCOlClientOlamMsgs,
				OleMtextField_host, (XrmDatabase)NULL),
			ValidateNoSpace, (XtPointer)stuff->footer);

  netspec_field = CreateTextField(control,
			OlGetMessage(XtDisplay(control), NULL, 0, OleNtextField,
				OleTnetspec, OleCOlClientOlamMsgs,
				OleMtextField_netspec, (XrmDatabase)NULL),
			ValidateNoSpace, (XtPointer)stuff->footer);

}	/* PopulateUpper() */


/*
** Callback for item selection in scrolling list (userMakeCurrent).
** Updates the text fields with the contents of the selected entry.
*/
static void
SelectItem(w, client_data, call_data)
  Widget	w;
  XtPointer	client_data;
  XtPointer	call_data;
{
  Arg		arg[1];
  char		*disp_string;		/* "Display name" part */
  char		*host_string;		/* "Host name" part */
  char		*netspec_string;	/* "Netspec" part */
  String	string;			/* Selected item's label */
  String	string_copy[MAXLINE];	/* Fresh copy of `string' */
  OlListToken	token;			/* Current item's token */
					/* (PFSL_NULL_TOKEN if nothing is */
					/* selected) */


  token = (OlListToken)call_data;

  if (token != PFSL_NULL_TOKEN)		/* There is a current item */
    {
      string = OlListItemPointer(token)->label;
      if (string != NULL && *string != '\0')
	{				/* The string isn't empty */
	  /*
	  ** Make a copy of `string' since ParseDispEntry() modifies its
	  ** first argument
	  */
	  strncpy(string_copy, string, (size_t)(MAXLINE - 1));
	  ParseDispEntry(string_copy, &disp_string, &host_string,
			 &netspec_string);

          DEBUG_MSG( OlGetMessage(XtDisplay(w), NULL,
                       0,
                       OleNfilerdisp,
                       OleTmsg4,
                       OleCOlClientOlamMsgs,
                       OleMfilerdisp_msg4,
                       (XrmDatabase)NULL),
                       disp_string);

          DEBUG_MSG( OlGetMessage(XtDisplay(w), NULL,
                       0,
                       OleNfilerdisp,
                       OleTmsg5,
                       OleCOlClientOlamMsgs,
                       OleMfilerdisp_msg5,
                       (XrmDatabase)NULL),
                       host_string);

          DEBUG_MSG( OlGetMessage(XtDisplay(w), NULL,
                       0,
                       OleNfilerdisp,
                       OleTmsg6,
                       OleCOlClientOlamMsgs,
                       OleMfilerdisp_msg6,
                       (XrmDatabase)NULL),
                       netspec_string);
	}
      else				/* Current item is empty */
	disp_string = host_string = netspec_string = "";
    }
  else					/* There is no current item */
    disp_string = host_string = netspec_string = "";
  
  /*
  ** Update the text fields with the contents of the current item
  */
  XtSetArg(arg[0], XtNstring, disp_string);
  XtSetValues(disp_field, arg, (Cardinal)1);

  XtSetArg(arg[0], XtNstring, host_string);
  XtSetValues(host_field, arg, (Cardinal)1);

  XtSetArg(arg[0], XtNstring, netspec_string);
  XtSetValues(netspec_field, arg, (Cardinal)1);

}	/* SelectItem() */

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:validate.c	1.6"
#endif

/*
** validate.c - This file contains routines to validate strings and output
** footer messages if bad characters are found.
*/


#include <ctype.h>
#include <string.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/TextField.h>

#include "errors.h"
#include "config.h"
#include "footer.h"
#include "validate.h"


/*
** Text field validation callback that doesn't allow any white-space in the
** string.
*/
void
ValidateNoSpace(w, client_data, call_data)
  Widget	w;
  XtPointer	client_data;
  XtPointer	call_data;
{

  (void)ValidateString(((OlTextFieldVerify *)call_data)->string, "",
		       (Widget)client_data);

}	/* ValidateNoSpace() */


/*
** Text field validation callback that doesn't allow any white-space or
** colons in the string.
*/
void
ValidateNoSpaceOrColon(w, client_data, call_data)
  Widget	w;
  XtPointer	client_data;
  XtPointer	call_data;
{

  (void)ValidateString(((OlTextFieldVerify *)call_data)->string, ":",
		       (Widget)client_data);

}	/* ValidateNoSpaceOrColon() */


/*
** This function does all of the work for the above two.
** A message is written to `footer' if `string' contains any illegal
** characters, is too long, or is empty.  Illegal characters are considered
** to be any of the following: any characters in `bad_chars' or
** ILLEGAL_CHARS (from "config.h"), white-space characters, and unprintable
** characters.
** 1 is returned if `string' is valid; 0 is returned otherwise.
*/
int
ValidateString(string, bad_chars, footer)
  char		*string;
  char		*bad_chars;
  Widget	footer;
{
  char		char_string[2];		/* String of first bad character */
  register char	*cptr;			/* Traverses `string' */
  int		len;			/* Length of `string' */
  char		worse_chars[MAXLINE];	/* Combination of `bad_chars' and */
					/* ILLEGAL_CHARS @ */


  ClearFooter(footer);			/* Clear footer on any operation */

  char_string[1] = '\0';		/* `char_string' holds one character */
  len = strlen(string);			/* @ */
  (void)sprintf(worse_chars, "%s%s", bad_chars, ILLEGAL_CHARS);

  if (len > MAXLINE - 1)		/* @ */
    {
      FooterMsg(footer, 
                       OlGetMessage(XtDisplay(footer), NULL,
                       0,
                       OleNfilevalidate,
                       OleTmsg1,
                       OleCOlClientOlamMsgs,
                       OleMfilevalidate_msg1,
                       (XrmDatabase)NULL),
                       NULL);

      return 0;
    }
  else if (len == 0)
    {
      FooterMsg(footer, 
                       OlGetMessage(XtDisplay(footer), NULL,
                       0,
                       OleNfilevalidate,
                       OleTmsg2,
                       OleCOlClientOlamMsgs,
                       OleMfilevalidate_msg2,
                       (XrmDatabase)NULL),
                       NULL);

      return 0;
    }
  else if (string != NULL)
    for (cptr = string; *cptr != '\0'; ++cptr)
      if (!isprint(*cptr))
	{
      FooterMsg(footer, 
                       OlGetMessage(XtDisplay(footer), NULL,
                       0,
                       OleNfilevalidate,
                       OleTmsg3,
                       OleCOlClientOlamMsgs,
                       OleMfilevalidate_msg3,
                       (XrmDatabase)NULL),
                       NULL);

	  return 0;
	}
      else if (isspace(*cptr))
	{
      FooterMsg(footer, 
                       OlGetMessage(XtDisplay(footer), NULL,
                       0,
                       OleNfilevalidate,
                       OleTmsg4,
                       OleCOlClientOlamMsgs,
                       OleMfilevalidate_msg4,
                       (XrmDatabase)NULL),
                       NULL);
	  return 0;
	}
      else if (strchr(worse_chars, *cptr) != NULL)
	{
	  /*
	  ** Copy `*cptr' into `char_string' since FooterMsg() expects its
	  ** third argument to be a string.
	  */
	  char_string[0] = *cptr;

      FooterMsg(footer, 
                       OlGetMessage(XtDisplay(footer), NULL,
                       0,
                       OleNfilevalidate,
                       OleTmsg5,
                       OleCOlClientOlamMsgs,
                       OleMfilevalidate_msg5,
                       (XrmDatabase)NULL),
                       char_string);
	  return 0;
	}

  return 1;				/* Success */

}	/* ValidateString() */

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:common.h	1.7"
#endif

/*
** common.h - This file includes the other internal headers, defines macros
** needed in "common.c", defines the structure for the context needed to
** represent a popup, and declares the functions defined in "common.c".
*/


#ifndef OLAM_COMMON_H
#define OLAM_COMMON_H


#include "config.h"
#include "create.h"
#include "error.h"
#include "file_stuff.h"
#include "footer.h"
#include "pfsl.h"
#include "rhost.h"
#include "util.h"
#include "validate.h"


/* mlp - no more - now just one flat */
/* #define NUM_BUSY_WIDGETS	3 */	/* Number of widgets that can be */
					/* insensitized (ie. the "Insert", */
					/* "Delete", and "Apply Edits" */
					/* buttons */


/*
** This structure contains the context information for a popup.  It is used
** by many of the routines (especially those in "common.c") to operate on a
** particular popup without knowing its specifics.
*/
typedef struct _common_stuff_struct
{
  Boolean	allow_popdown;		/* Used in VerifyPopdown() to */
					/* allow/disallow popdown on a */
					/* PopupWindow provided button press */
  void		(*apply_edit)();	/* Callback for "Apply Edit" button */
  Widget	busy_widget;
					/* Widgets to be insensitized when */
					/* file is not writable */
  Boolean	changed;		/* True if change has been made to */
					/* list */
  Widget	footer;			/* Widget to write messages to */
  Boolean	file_is_writeable;	/* True if file is writable */
  char		file_name[MAXPATHLEN];	/* File to be operated on */
  String	(*get_line)();		/* Function that produces a line */
					/* from file */
  PFScrollList	pfsl;			/* List of items */
  Widget	save_notice;		/* Notice widget to popup when there */
					/* are unsaved changes*/
}	CommonStuff, *CommonStuffPtr;


void	Apply();			/* Callback for "Apply" button */
void	Delete();			/* Callback for "Delete" button */
void	Dismiss();			/* Callback for XtNpopdownCallback */
void	FillList();			/* Fills list with contents of file */
void	InsertAfter();			/* Callback for "Insert After" */
void	InsertBefore();			/* Callback for "Insert Before" */
void	PopulateLower();		/* Populates the ControlPanel with */
					/* the "Insert", "Delete", and */
					/* "Apply Edits" buttons */
void	VerifyPopdown();		/* Allows/disallows popdown based on */
					/* value of `allow_popdown' */
void	Reset();			/* Callback for "Reset" button */


extern int	num_popups;		/* Number of popups currently */
					/* popped-up; used by Quit() to */
					/* determine if the last popup has */
					/* been popped-down */
extern Widget	topshell;	/* topleve shell returned by OlInitialize() */

#endif	/* OLAM_COMMON_H */

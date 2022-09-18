/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:olam.c	1.17"
#endif

/*
** olam.c - This file contains main() and some helper routines for the
** OPEN LOOK(tm) Administration Manager (olam).
**
** The Administration Manager currently administers the two files that deal
** with network connections to and from the current machine.  One file,
** "/etc/X0.hosts", is recognized by all X servers.  It is read at server
** start-up and contains a list of machines from which client connections
** are accepted.  The other file, "/usr/X/lib/Xconnections", is a
** value-added feature of AT&T XWIN 1.1 and later servers.  It contains a
** mapping from a display name to a host name and network which clients use
** to open remote connections.  See the comment at the top of "rdisp.c" for
** more info.
**
** This program is normally invoked from the "Utilities" submenu of the
** Workspace Manager (olwsm); because of this, there is no "base" window -
** only two  popup windows designed to look like other popups handled by
** olwsm.  The user doesn't know there is a separate application (except for
** the startup time :->).
** Two command-line arguments are recognized.  A "-d" produces the popup for
** administering "/usr/X/lib/Xconnections", and "-h" produces the popup for
** adminstering "/etc/X0.hosts".
**
** It is intended that olam be able to display both (or more in the future)
** popups at the same time and only exit when the last one goes away.  This
** is currently not the case, however, due to a problem having more than one
** scrolling list in an application; therefore, both "-d" and "-h" are not
** allowed to be specified at the same time.  The code is written to support
** multiple popups.  Only the argument processing code (in main()) disallows
** it.
**
** There is very much that is the same in these two popups, and this
** presented an opportunity for significant modularization.  The file
** "common.c" contains application-specific code that can be shared between
** the popups.  Two other files "rhost.c" and "rdisp.c" have the
** non-shareable (but still very similar) code for each popup.  (Note that
** "host" things have to do with "/etc/X0.hosts" and "disp" or "display"
** things have to do with "/usr/X/lib/Xconnections".)  There are a number of
** other files containing code to do more general things.
**
** config.h - Macro definintions for various magic strings and numbers 
**
** create.[ch] - Functions to create widgets of the variety most commonly
** needed in this application
**
** error.[ch] - Functions to format and print error messages
**
** file_stuff.[ch] - Functions to manipulate files (ie. opening, checking
** permissions, extracting information)
**
** footer.[ch] - Functions for producing footer messages
**
** pfsl.[ch] - Functions to implement an ADT based on the scrolling list
** widget and the general list routines below.
**
** util.h - Utility macros
**
** validate.[ch] - Functions for validating user input
**
** listd/list*.[ch] - Implement a library of general linked list manipulation
** routines
**
** There is some context (see typedef for CommonStuff) associated with each
** popup.  Many routines take an argument of this type (consistently called
** `stuff') that permit them to be used for either popup.
**
** Three application resources are recognized.  The value of "Beep" is
** compared against a magic number in "OpenLook.h" to determine whether
** footer messages should be accompanied by a beep.  "BeepVolume" is used as
** the second argument to XBell() and may be set from olwsm in the future.
** Finally, "Warnings" is a Boolean controlling whether or not warnings are
** produced (False by default).
*/


#include <stdio.h>
#include <string.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>

#include "errors.h"
#include "config.h"
#include "common.h"
#include "error.h"
#include "rdisp.h"
#include "rhost.h"


Widget		topshell; /* returned by Olinitialize() */

static void	IgnoreWarnings();	/* Do-nothing warning handler used */
					/* to ignore warnings */
static void	UsageError();		/* Prints a usage message and exits */

/*
** Application resource stuff
*/
static Boolean		false = FALSE;
static Boolean		warnings = FALSE;
					/* Controls whether warning messages */
					/* are produced */
static XtResource	resources[] =
{
  {"warnings", "Warnings", XtRBoolean, sizeof(Boolean), (Cardinal)&warnings,
   XtRBoolean, (XtPointer)&false
  }
};


main(argc, argv)
  int	argc;
  char 	*argv[];
{
  extern int	getopt();		/* getopt(3) */
  int		opt;
  extern int	opterr;
  int		rdisp_popup;		/* 1 if "-d" is an argument */
  int		rhost_popup;		/* 1 if "-h" is an argument */
  /* Widget	shell; */		/* Useless shell widget used to get */
					/* the application resources */


/*  This part of the code was moved here because you could not
**  get the bad usage localized message otherwise.
*/


  /*
  ** Get the application resources
  */
#ifdef MEMUTIL
 InitializeMemutil();
#endif

  OlToolkitInitialize(&argc, argv, NULL);
  topshell = XtInitialize("TopLevelShell", "olam", NULL, 0, &argc, argv);

  XtGetApplicationResources(topshell, NULL, resources,
			    XtNumber(resources), NULL, 0);


  _OlSetApplicationTitle( OlGetMessage(XtDisplay(topshell), NULL,
                          0,
                          OleNfileolam,
                          OleTmsg1,
                          OleCOlClientOlamMsgs,
                          OleMfileolam_msg1,
                          (XrmDatabase)NULL));


  /*
  ** Turn off warnings if necessary
  */
  if (warnings == FALSE)
    {
      XtSetWarningHandler(IgnoreWarnings);
      OlSetWarningHandler(IgnoreWarnings);
    }
  /*
  ** Point `progname' at `argv[0]' for use in the error functions
  */
  progname = argv[0];

  /*
  ** Set up for option processing
  */
  opterr = 0;
  opt = 0;
  rdisp_popup = 0;
  rhost_popup = 0;

  /*
  ** Get options
  ** If neither 'd' nor 'h' was specified,  default to 'h'.
  */
  while ((opt = getopt(argc, argv, "dh")) != -1)
    switch(opt)
      {
      case 'd':
	rdisp_popup = 1;
	break;
      case 'h':
	rhost_popup = 1;
	break;
      default:
	rhost_popup = 1;
	break;
      }

  if (!rdisp_popup && !rhost_popup || rdisp_popup && rhost_popup)
   {
     rhost_popup = 1;
     rdisp_popup = 0;
    }

  /*
  ** Popup specified popups
  */
  if (rdisp_popup)
    XtPopup(CreateRdispPopup(), XtGrabNone);

  if (rhost_popup)
    XtPopup(CreateRhostPopup(), XtGrabNone);

  XtMainLoop();

}	/* main() */


/*
** Do-nothing warning handler used to ignore warnings
*/
static void
IgnoreWarnings(message)
  char	*message;
{
  /* do nothing */
}	/* IgnoreWarnings() */


/*
** Produce a error message and exit
*/
static void
UsageError(w)
Widget w;
{
   OlVaDisplayErrorMsg(XtDisplay(w),
                      OleNfileolam,
                      OleTmsg2,
                      OleCOlClientOlamMsgs,
                      OleMfileolam_msg2,
                      progname);

  exit(2);

}	/* UsageError() */

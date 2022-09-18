/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:wmm.c	1.17"
#endif

/*
 * olwm.c
 *
 * The Flattened Window Manager Widget Unit Test
 * (can also serve as the window manager executable)
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/Error.h>
#include "Strings.h"

#define XtNwarnings   "warnings"
#define XtCWarnings   "Warnings"
#define XtNdoFork     "doFork"
#define XtCDoFork   "DoFork"

/* Which class to use for toplevel shell?  Depends on return from
 * OlGetGui()
 */
#define APPLICATION_NAME   "olwm"
#define APPLICATION_CLASS  "olwm"

#define MOT_APPLICATION_NAME   "mwm"
#define MOT_APPLICATION_CLASS  "Mwm"

static int  IgnoreClientErrors();
static void IgnoreWarnings();
extern void RestartWindowMgr();

static Boolean Warnings;
static char **argv_main, **envp_main;

extern String OleMbadExec_execvp;
static char ** custom_argv;

static XtResource resources[] =
   {
   { XtNwarnings, XtCWarnings, XtRBoolean, sizeof(Boolean),
     (Cardinal) &Warnings, XtRImmediate, (XtPointer)False },
   };
 
main(argc, argv, envp)
int argc;
char * argv[], *envp[];
{
Widget Shell;
Cardinal i;

#ifdef MEMUTIL
InitializeMemutil();
#endif

OlToolkitInitialize(&argc, argv, NULL);

/* Customized Resource Files */
/* If motif, olwm -xrm "*customization: -mo" */

if (OlGetGui() == OL_MOTIF_GUI)  {
                custom_argv = (char **)XtMalloc(sizeof(char *)*(argc+3));
                for (i = 0; i < argc; i++)
                        custom_argv[i] = argv[i];
                custom_argv[argc++] = "-xrm";
                custom_argv[argc++] = "*customization:-mo";
                custom_argv[argc] = (char *) NULL;
        }
        else
                custom_argv = argv;

	/* Notes from SAMC:
	 *
	 * You may want to use XtAppInitialize later on because of other
	 * functionalities. When you do this, you also need to do changes
	 * on your MainLoop, e.g., replace XtNextEvent with XtAppNextNext.
	 */
	Shell = (OlGetGui() != OL_MOTIF_GUI) 
		? XtInitialize(APPLICATION_NAME, APPLICATION_CLASS,
					NULL, 0, &argc, custom_argv)
		: XtInitialize(MOT_APPLICATION_NAME, MOT_APPLICATION_CLASS,
					NULL, 0, &argc, custom_argv);
XtGetApplicationResources(Shell, NULL, resources, XtNumber(resources), NULL, 0);

if (Warnings == False)
   {
   XtSetWarningHandler(IgnoreWarnings);
   OlSetWarningHandler(IgnoreWarnings);
   }

/* Save main() arguments */
argv_main = argv;
envp_main = envp;

SetupWindowManager(Shell);
StartWindowManager(Shell);

} /* end of main */
/*
 * IgnoreWarnings
 *
 */

static void IgnoreWarnings()
{

/*
 *
 *        This space intentionally left blank.
 *
 *
 */

} /* end of IgnoreWarnings */
/*
 * ServiceEvent
 *
 * The \fIServiceEvent\fR procedure is called when an event needs to be 
 * dispatched.  This name was chosen to match the name of the dispatch
 * routine in the OPEN LOOK(r) Workspace Manager, so that it is possible
 * to combine olwsm and olwm into one process.
 *
 */

extern void ServiceEvent(event)
XEvent * event;
{

XtDispatchEvent(event);

} /* end of ServiceEvent */


extern void
RestartWindowMgr(dsp)
Display *dsp;
{
execvp(argv_main[0], argv_main);
OlVaDisplayErrorMsg(dsp, OleNbadExec, OleTexecvp,
			OleCOlClientOlwmMsgs, OleMbadExec_execvp);
}

/*
 * Some throw-away function definitions to be "compatible" with
 * X11R4.  These work now since the window manager does not use
 * gadgets.
 */

/*
extern Screen *  XtScreenOfObject(o){return(XtScreen(o));}
extern Display * XtDisplayOfObject(o){return(XtDisplay(o));}
extern Window    XtWindowOfObject(o){return(XtWindow(o));}
*/

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olps:misc.c	1.9"
#endif

/*
 * misc.c - routines to execute program via olwsm,
 *		to open a file,
 *		to free variables,
 *		to set a message in olprintscreen's footer text area,
 */

#include "main.h"
#include "externs.h"
#include "error.h"
#include <Xol/ScrollingL.h>
#include <Xol/FButtons.h>
#define  TOTALCMD_WIDTH 500
#define SFT(x)  SetFooterText(footer_text, \
                              OlGetMessage(XtDisplay(toplevel), \
                                           NULL, BUFSIZ, \
                                           OleNfooterMsg, \
                                           concat(OleT,x), \
                                           OleCOlClientOlpsMsgs, \
                                           concat(OleMfooterMsg_,x), \
                                           (XrmDatabase)NULL))

static  char              totalcmd[TOTALCMD_WIDTH];

#include <Xol/WSMcomm.h>

char * ButtonFields2[] = {
        XtNlabel,
        XtNselectProc,
        XtNmnemonic
};
static int ExecuteSerial = 0;
static WSM_Request request;
infostruct info;

/*
 * ExecuteProgram -
 * because parent does not wait, can only tell if fork and exec succeeded,
 * no way to tell how program will terminate
 */
/* tempfile is passed in, printcmd is global, totalcmd is static */
void
ExecutePrint(tempfile,flag)
char *tempfile;
int  flag;
{
	int ret;
	
	f_failed = False;
	totalcmd[0]='\0';
	strcat(totalcmd,"sh -c \"");
	strcat(totalcmd," < ");
	strcat(totalcmd,tempfile);
	strcat(totalcmd," ");
	strcat(totalcmd,printcmd);

	if (flag) {
		strcat(totalcmd,";rm -f ");
		strcat(totalcmd,tempfile);
	}
	strcat(totalcmd,"\"");

	ret = system(totalcmd);
	if (ret < 0) {
                SFT(printFailed);
		_OlBeepDisplay(toplevel, 3);
		f_failed = True;
	}
}


int 
CanOpenFile(f,mode,file)
char * f;
char * mode;
FILE ** file;
{
	struct stat buf;
	int ret, f_error=False;

	ret = stat(f, &buf);
	strcpy(message, f);
	strcat(message, ": ");

	/* Request to open file for reading */
	if (*mode == 'r') {
		/* First, check if file exists */
		if (ret == -1) {
			f_error = True;
			switch (errno) {
			case ENOTDIR:
		 strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                               OleNfooterMsg, OleTpathInvalid,
                                               OleCOlClientOlpsMsgs,
                                               OleMfooterMsg_pathInvalid,
                                               (XrmDatabase)NULL));

				break;
			case ENOENT:
		 strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                             OleNfooterMsg, OleTnonexistentFile,
                                             OleCOlClientOlpsMsgs,
                                             OleMfooterMsg_nonexistentFile,
                                             (XrmDatabase)NULL));
				break;
			case EACCES:
		 strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                               OleNfooterMsg, OleTnoPathAccess,
                                               OleCOlClientOlpsMsgs,
                                               OleMfooterMsg_noPathAccess,
                                               (XrmDatabase)NULL));
				break;
			default:
		 strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                            OleNfooterMsg, OleTfileUnaccessible,
                                            OleCOlClientOlpsMsgs,
                                            OleMfooterMsg_fileUnaccessible,
                                            (XrmDatabase)NULL));
				break;
			}
		}
		else {
			/* File exists, now check to see if ordinary file */
			/* Check if it's not ordinary file */
			if ((buf.st_mode & 070000) != 0) {
				/* Check if directory 
			(first clear out last 3 digits)*/
				if ((buf.st_mode & 070000) == 040000) {
					f_error = True;
		 strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                               OleNfooterMsg, OleTisAdirectory,
                                               OleCOlClientOlpsMsgs,
                                               OleMfooterMsg_isAdirectory,
                                               (XrmDatabase)NULL));
				}
				/* Else must be special file */
				else {
					f_error = True;
		 strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                             OleNfooterMsg, OleTfileNotOrdinary,
                                             OleCOlClientOlpsMsgs,
                                             OleMfooterMsg_fileNotOrdinary,
                                             (XrmDatabase)NULL));
				}
			}
		}
		if (f_error == True) {
			_OlBeepDisplay(toplevel, 1);
			SetFooterText(footer_text, message);
			return(-1);
		}
	}
	else {
		/* Request to open file for writing */
		if (*mode == 'w') {
			/* Check if file not exist, or pathname errors */
			if (ret == -1) {
				switch (errno) {
					/* file does not exist, which is what we desire */
				case ENOENT:
					break;
				case ENOTDIR:
					f_error = True;
		 strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                               OleNfooterMsg, OleTpathInvalid,
                                               OleCOlClientOlpsMsgs,
                                               OleMfooterMsg_pathInvalid,
                                               (XrmDatabase)NULL));
					break;
				case EACCES:
					f_error = True;
		 strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                               OleNfooterMsg, OleTnoPathAccess,
                                               OleCOlClientOlpsMsgs,
                                               OleMfooterMsg_noPathAccess,
                                               (XrmDatabase)NULL));
					break;
				default:
					f_error = True;
		 strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                            OleNfooterMsg, OleTpathUnaccessible,
                                            OleCOlClientOlpsMsgs,
                                            OleMfooterMsg_pathUnaccessible,
                                            (XrmDatabase)NULL));
					break;
				}
				if (f_error == True) {
					_OlBeepDisplay(toplevel, 1);
					SetFooterText(footer_text, message);
					return(-1);
				}
			}
			/* File exists, now check to see if ordinary file */
			else {
				/* Check if it's not ordinary file */
				if ((buf.st_mode & 070000) != 0) {
					/* Check if directory 
			(first clear out last 3 digits)*/
					if ((buf.st_mode & 070000) == 040000) {
						f_error = True;
		 strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                               OleNfooterMsg, OleTisAdirectory,
                                               OleCOlClientOlpsMsgs,
                                               OleMfooterMsg_isAdirectory,
                                               (XrmDatabase)NULL));
					}
					/* Else must be special file */
					else {
						f_error = True;
	        strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                             OleNfooterMsg, OleTfileNotOrdinary,
                                             OleCOlClientOlpsMsgs,
                                             OleMfooterMsg_fileNotOrdinary,
                                             (XrmDatabase)NULL));
					}
				}
				if (f_error == True) {
					_OlBeepDisplay(toplevel, 1);
					SetFooterText(footer_text, message);
					return(-1);
				}
				/* Do this if first time around,
		   if second time around, go on and create file 
		   (i.e. overwrite chosen) */
				if (f_save_overwrite == False) {
					/* If got here, means it is ordinary file.
	   	      Now have to ask if okay to overwrite it */
					strcpy(message, f);
	         strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                             OleNfooterMsg, OleToverwrite,
                                             OleCOlClientOlpsMsgs,
                                             OleMfooterMsg_overwrite,
                                             (XrmDatabase)NULL));

					cnt = 0;
					XtSetArg(args[cnt], XtNstring, message); 
					cnt++;
					XtSetValues(save_notice_text, args, cnt);
					XtPopup(save_notice, XtGrabExclusive);
					return (-1);
				}
			}
		}
	}
	/*  If got here, it means it's okay to open file,
	or okay to create file */
	/*  Try to open/create the specified file */
	*file = fopen(f, mode);
	if (*file == NULL) {
		if (*mode == 'r')
	        strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                            OleNfooterMsg, OleTerrorOpeningFile,
                                            OleCOlClientOlpsMsgs,
                                            OleMfooterMsg_errorOpeningFile,
                                            (XrmDatabase)NULL));
		else
	         strcat(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                           OleNfooterMsg, OleTerrorCreatingFile,
                                           OleCOlClientOlpsMsgs,
                                           OleMfooterMsg_errorCreatingFile,
                                           (XrmDatabase)NULL));
		_OlBeepDisplay(toplevel, 1);
		SetFooterText(footer_text, message);
		return(-1);
	}
	else{
		return(0);
	}
}


void
Free1(p,msg)
char *p, *msg;
{
	if (p != NULL){
		free (p);
	}
}

static  XEvent	expevent;
static	Window	window;

void
SetFooterText(w, string)
Widget w;
char * string;
{

	cnt = 0;
	XtSetArg(args[cnt], XtNstring, string); 
	cnt++;
	XtSetValues(w, args, cnt);
	/* flush queue, then peel off events manually and dispatch them */
	XSync(dpy,False);
	window = XtWindow(w);
	while (XCheckWindowEvent(dpy, window, ExposureMask, &expevent) == True)
		XtDispatchEvent(&expevent);
}

void
Resolvefilename(w, infilestring, func)
Widget w;
char * infilestring;
void (*func)();
{
#define BUFSIZE 5120

        static	XEvent	expevent;
	static	Window	window;

        extern void Operation_Cancel();
	extern void filescrollCB();
	FILE *fptr;
	char cmd[TEXTF_WIDTH_1];
	char *filelist[BUFSIZE];
	char *p;
	char buffer[BUFSIZE];
	int i, y;

	Widget filescrollinglist, savescrollinglist, printscrollinglist;
	Widget	open_notice, open_notice_control, open_lower_control,
		open_notice_cancel, open_notice_footer, open_notice_text;

	ButtonItems CancelScroll[] = {
		{ (XtArgVal)NULL, (XtArgVal)Operation_Cancel, (XtArgVal)'C'}};

        OlListToken (* addtolistfn)();
	void (* deletefromlistfn)();
	OlListItem sl_item[TEXTF_WIDTH_1];
	OlListToken globaltoken[TEXTF_WIDTH_1];

        /*
         *  Format an echo to the shell; create a pipe for
	 *  reading the output of the echo.
	 */
	sprintf(cmd, "sh -c \'echo %s", infilestring);
	fptr = popen(cmd, "r");
	fgets(buffer, BUFSIZE, fptr);
	pclose(fptr);

        for(i=0, p=strtok(buffer, " \t\n"); p; p=strtok(NULL, " \t\n"))
	{
         	filelist[i++] = strdup(p);
	}
        if (i > 1)
	{

                cnt=0;
		open_notice =  XtCreatePopupShell("Resolve Multiple Files",
                                                popupWindowShellWidgetClass,
                                                w, args, cnt);

                cnt =0;
		XtSetArg(args[cnt], XtNupperControlArea, &open_notice_control);
cnt++;
      		XtSetArg(args[cnt], XtNlowerControlArea, &open_lower_control); cnt++
;
 		XtSetArg(args[cnt], XtNfooterPanel, &open_notice_footer); cnt++;
		XtGetValues(open_notice, args, cnt);
		open_notice_text = XtCreateManagedWidget("footer_text",
                                                staticTextWidgetClass,
                                                open_notice_footer, args, cnt);

                XtAddCallback(open_notice, XtNpopdownCallback, Operation_Cancel,				NULL);

		CancelScroll[0].label = (XtArgVal)OlGetMessage( XtDisplay(toplevel), NULL,\
					0, OleNbutton, OleTlabelCancel, 
					OleCOlClientOlpsMsgs, 
					OleMbutton_labelCancel,(XrmDatabase)NULL);
		cnt = 0;
		open_notice_cancel = XtVaCreateManagedWidget(
                               "Cancel", flatButtonsWidgetClass,
                                open_lower_control, 
				XtNrecomputeSize, True,
				XtNlayoutType, OL_FIXEDROWS,
				XtNmeasure, 1,
				XtNitemFields, ButtonFields2,
				XtNnumItemFields, XtNumber(ButtonFields2),
                                XtNitems, CancelScroll,
                                XtNnumItems, 1,
					(char *)0);

                /*
                 * Set up the scrolling list in the control area.
                 */
                cnt = 0;
		XtSetArg(args[cnt], XtNviewHeight, (XtArgVal)7); cnt++;
		filescrollinglist = XtCreateManagedWidget("scrollinglist",
                        scrollingListWidgetClass, open_notice_control, args, cnt);
		info.funcptr = func;
		info.popup = &open_notice;
  		/*
                * To add items
		*/
                cnt = 0;
		XtSetArg(args[cnt], XtNapplAddItem, (XtArgVal) &addtolistfn);
		cnt++;
		XtSetArg(args[cnt], XtNapplDeleteItem, (XtArgVal) &deletefromlistfn);
     		/* Add callback passing the function registered by the
                 * user to the callback; the function will be invoked as
                 * soon as the user selects a file.
                 */
                cnt++;
		XtGetValues(filescrollinglist, args, cnt);
		XtAddCallback(filescrollinglist, XtNuserMakeCurrent, filescrollCB, &info);

                for (y=0; y<i; y++)
		{
                 	 sl_item[y].label_type = (OlDefine) OL_STRING;
                        sl_item[y].label = XtMalloc(TEXTF_WIDTH_1 * sizeof(char));
  			sprintf(sl_item[y].label, "%s", filelist[y]);

                        globaltoken[y] = (*addtolistfn)(filescrollinglist,NULL, NULL, sl_item[y]);
                }
                strcpy(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
                                             OleNfooterMsg, OleTtooManyFiles,
                                             OleCOlClientOlpsMsgs,
                                             OleMfooterMsg_tooManyFiles,
                                             (XrmDatabase)NULL));


                XtSetArg(args[0], XtNstring, message);
		XtSetValues(open_notice_text, args, 1);

                XtPopup(open_notice, XtGrabNone);
	} /*end if i > 1 */
	else {

                (*func)(filelist[0]);
		return;
	}
	
}

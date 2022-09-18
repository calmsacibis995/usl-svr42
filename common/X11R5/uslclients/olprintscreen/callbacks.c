/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olps:callbacks.c	1.20"
#endif

/*
 * callbacks.c
 * higher level callbacks first
 *
 * "image_content": this external variable is used when one of
 *	"Capture Image" operations is performed. It's NULL by default
 *	(initialized in main.c)
 */

#include "main.h"
#include "externs.h"
#include "error.h"  
#include <X11/cursorfont.h>
#include <Xol/ScrollingL.h>
#include <Xol/OlCursors.h>
/*
#include "XShm.h"
*/

static	char  * filename, 
realfile[TEXTF_WIDTH_1] = {
	NULL},
	tempfile[TEXTF_WIDTH_1] = {
		NULL	};
	char *mktemp();

/* flag indicates the shm extension and local connection */

extern int xlocal;
extern XImage *shmimage;
extern int noflag;

/* handy macro */

#define SFT(x)	SetFooterText(footer_text, \
			      OlGetMessage(XtDisplay(toplevel), \
					   NULL, BUFSIZ, \
					   OleNfooterMsg, \
					   concat(OleT,x), \
					   OleCOlClientOlpsMsgs, \
					   concat(OleMfooterMsg_,x), \
					   (XrmDatabase)NULL))
							

/* ARGSUSED */
void
Open_Popup(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
	if (f_unmapping == False)
		f_open_pop = True;
}

/* ARGSUSED */
void
Open_Popdown(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
	if (f_unmapping == False)
		f_open_pop = False;
}

/* ARGSUSED */
void
Open_Pop(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
	Widget w = *(Widget *)client_data;
	SetFooterText(footer_text, "");
	/* New system - uses the address of the widget, so dereference it */
	XtPopup(w, XtGrabNone);
	XRaiseWindow( dpy, XtWindow(open_pop));
}

/* ARGSUSED */
void
Save_Popup(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
	if (f_unmapping == False)
		f_save_pop = True;
}

/* ARGSUSED */
void
Save_Popdown(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
	if (f_unmapping == False)
		f_save_pop = False;
}

/* ARGSUSED */
void
Save_Pop(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
	Widget w = *(Widget *)client_data;
	SetFooterText(footer_text, "");
	XtPopup(w, XtGrabNone);
	XRaiseWindow( dpy, XtWindow(save_pop));
}

/* ARGSUSED */
void
Printf_Popup(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
	if (f_unmapping == False)
		f_printf_pop = True;
}

/* ARGSUSED */
void
Printf_Popdown(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
	if (f_unmapping == False)
		f_printf_pop = False;
}

/* ARGSUSED */
void
Printf_Pop(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
	Widget w = *(Widget *)client_data;
	SetFooterText(footer_text, "");
	XtPopup(w, XtGrabNone);
	XRaiseWindow( dpy, XtWindow(printf_pop));
}

/* ARGSUSED */
void
Prop_Popup(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
	if (f_unmapping == False)
		f_prop_pop = True;
}

/* ARGSUSED */
void
Prop_Popdown(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
	if (f_unmapping == False)
		f_prop_pop = False;
}

/* ARGSUSED */
void
Properties_Pop(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
	Widget w = *(Widget *)client_data;
	SetFooterText(footer_text, "");
	XtPopup(w, XtGrabNone);
	XRaiseWindow( dpy, XtWindow(prop_pop));
}

/* ARGSUSED */
void
Open_Resolve(w, closure, call_data)
Widget w;
XtPointer closure, call_data;
{
 	extern void Resolvefilename();
	extern void Open_File();
	cnt = 0;
	XtSetArg(args[cnt], XtNstring, &filename);
	cnt++;
	XtGetValues (open_pop_textf, args, cnt);

        strcpy(realfile, filename);
	/* must free, since Textfields mallocs for you */
	Free1(filename,"Open_File: filename");

	Resolvefilename(toplevel, realfile, Open_File);
	return;

}


/* ARGSUSED */
void 
Open_File(file_string)
char * file_string;
{


        XtSetArg(args[0], XtNstring, file_string);
	XtSetValues(open_pop_textf, args, 1);

        SetFooterText(footer_text, \
                              OlGetMessage(XtDisplay(toplevel), \
                                           NULL, BUFSIZ, \
                                           OleNfooterMsg, \
                                           concat(OleT,loading), \
                                           OleCOlClientOlpsMsgs, \
                                           concat(OleMfooterMsg_,loading), \
                                           (XrmDatabase)NULL));


	if(xlocal)	image_struct = *shmimage;	

	if (FileToImage(file_string, &image_struct) == -1) 
		return;

	f_fromopenfile = True;
	image_content = &image_struct;	
	ImageToPane(image_content);

	f_fromopenfile = False;

	sprintf(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
				      OleNfooterMsg, OleTfileLoaded,
				      OleCOlClientOlpsMsgs,
				      OleMfooterMsg_fileLoaded,
				      (XrmDatabase)NULL), file_string);
	
	SetFooterText(footer_text, message);
}

/* ARGSUSED */
void
Open_Cancel(w, closure, call_data)
Widget w;
XtPointer closure, call_data;
{
        SFT(openCancelled);
}

/* ARGSUSED */
void
Operation_Cancel(w, closure, call_data)
Widget w;
XtPointer closure, call_data;
{
        SFT(operCancelled);
}


/* ARGSUSED */
void
Save_Resolve(w, closure, call_data)
Widget w;
XtPointer closure, call_data;
{
 	extern void Resolvefilename();
	extern void Save_File();
	cnt = 0;
	XtSetArg(args[cnt], XtNstring, &filename);
	cnt++;
	XtGetValues (save_pop_textf, args, cnt);

        strcpy(realfile, filename);
	/* must free, since Textfields mallocs for you */
	Free1(filename,"Open_File: filename");
	SFT(saveContents);

	Resolvefilename(toplevel, realfile, Save_File);
	return;

}


/* ARGSUSED */
void 
Save_File(file_string)
char * file_string;
{

        XtSetArg(args[0], XtNstring, file_string);
	XtSetValues(save_pop_textf, args, 1);

	/* should take out after desensitize works */
	if (f_contents == False){
	        SFT(noContentsSave);
		return;
	}


	/* open + close inside */

	if ( PaneToFile( file_string, image_content,
	    &header,
	    win_name, win_name_size,
	    colors,   ncolors,
	    buffer_size) == -1) {
		return;
	}
	sprintf(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
				      OleNfooterMsg, OleTfileWritten,
				      OleCOlClientOlpsMsgs,
				      OleMfooterMsg_fileWritten,
				      (XrmDatabase)NULL), file_string);
	SetFooterText(footer_text, message);
}

/* ARGSUSED */
void
Save_Overwrite(w, closure, call_data)
Widget w;
XtPointer closure, call_data;
{
	char * file_name;

	f_save_overwrite = True;
	XtSetArg(args[0], XtNstring, &file_name);
        XtGetValues(save_pop_textf, args, 1);
	Save_File(file_name);
	f_save_overwrite = False;
}

/* ARGSUSED */
void
Save_Cancel(w, closure, call_data)
Widget w;
XtPointer closure, call_data;
{
        SFT(saveCancelled);
}

/* ARGSUSED */
void
Print_Resolve(w, closure, call_data)
Widget w;
XtPointer closure, call_data;
{
 	extern void Resolvefilename();
	extern void Print_File();
	cnt = 0;
	XtSetArg(args[cnt], XtNstring, &filename);
	cnt++;
	XtGetValues (printf_pop_textf1, args, cnt);

        strcpy(realfile, filename);
	/* must free, since Textfields mallocs for you */
	Free1(filename,"Open_File: filename");
	SFT(prepareFile);

	Resolvefilename(toplevel, realfile, Print_File);
	return;

}

/* ARGSUSED */
void
Print_File(file_string)
char * file_string;
{

        XtSetArg(args[0], XtNstring, file_string);
	XtSetValues(printf_pop_textf1, args, 1);



	if (CanOpenFile(file_string,"r",&rw_file) == -1) {
		return;
	}
	ExecutePrint(file_string,False);	

	sprintf(message, OlGetMessage(XtDisplay(toplevel), NULL, BUFSIZ,
				      OleNfooterMsg, OleTfileSpooled,
				      OleCOlClientOlpsMsgs,
				      OleMfooterMsg_fileSpooled,
				      (XrmDatabase)NULL), file_string);
	if (!f_failed)
		SetFooterText(footer_text, message);
	(void) fclose(rw_file);
}

/* ARGSUSED */
void
Print_Cancel(w, closure, call_data)
Widget w;
XtPointer closure, call_data;
{
        SFT(printCancelled);
}


/* ARGSUSED */
void
Print_Contents(w, closure, call_data)
Widget w;
XtPointer closure, call_data;
{
	/* should take out after desensitize works */
	if (f_contents == False){
	        SFT(noContentsPrint);
		return;
	}

	SFT(prepareContents);

	/* file removed/unlinked only after xpr finishes reading it */
	strcpy(tempfile,"/usr/tmp/olpsXXXXXX");
	(void) mktemp(tempfile);

	/* open + close inside */
	if ( PaneToFile( tempfile, image_content,
	    &header,
	    win_name, win_name_size,
	    colors,   ncolors,
	    buffer_size) == -1) {
		return;
	}

	/* opened */
	if (CanOpenFile(tempfile,"r",&rw_file) == -1) {
		return;
	}

	ExecutePrint(tempfile,True);

	if (!f_failed)
	  SFT(contentsSpooled);

	(void) fclose(rw_file);
}


/* ARGSUSED */
void
Capture_Window(w, closure, call_data)
Widget w;
XtPointer closure, call_data;
{

	SetFooterText(footer_text, "");
	UnmapTop(w);

	if(xlocal)	image_ptr = shmimage;	

	WindowToImage(  &image_ptr,
	    &header,   &header_size,
	    &win_name, &win_name_size,
	    &colors,   &ncolors,
	    &buffer_size);

        XDefineCursor(dpy, rootwindow, wait_cursor);
        XFlush(dpy);

	/*SetFooterText(footer_text, "Creating contents, please wait...");*/

	image_content = image_ptr;
	ImageToPane(image_content);	

	SFT(doneCreate);
	MapTop();
        XDefineCursor(dpy, rootwindow, OlGetStandardCursor(toplevel));
}


/* ARGSUSED */
void
Capture_Screen(w, closure, call_data)
Widget w;
XtPointer closure, call_data;
{

	SetFooterText(footer_text, "");
	f_screen = True;
	UnmapTop(w);

	if(xlocal)	image_ptr = shmimage;

	WindowToImage(  &image_ptr,
	    &header,   &header_size,
	    &win_name, &win_name_size,
	    &colors,   &ncolors,
	    &buffer_size);
        XDefineCursor(dpy, rootwindow, wait_cursor);
        XFlush(dpy);

	/*SetFooterText(footer_text, "Creating contents, please wait...");*/
	image_content = image_ptr;
	ImageToPane(image_content);

	SFT(doneCreate);
	MapTop();
        XDefineCursor(dpy, rootwindow, OlGetStandardCursor(toplevel));
	/*f_contents = True;*/
	f_screen = False;
}


/* ARGSUSED */
void
Capture_Area(w, closure, call_data)
Widget w;
XtPointer closure, call_data;
{

	SetFooterText(footer_text, "");
	f_area = True;
	UnmapTop(w);

	if(xlocal)	image_ptr = shmimage;

	WindowToImage2(  &image_ptr,
	    &header,   &header_size,
	    &win_name, &win_name_size,
	    &colors,   &ncolors,
	    &buffer_size);
        XDefineCursor(dpy, rootwindow, wait_cursor);
        XFlush(dpy);

	/*SetFooterText(footer_text, "Creating contents, please wait...");*/
	image_content = image_ptr;
	ImageToPane(image_content);

	SFT(doneCreate);
	MapTop();
        XDefineCursor(dpy, rootwindow, OlGetStandardCursor(toplevel));
	/*f_contents = True;*/
	f_area = False;
}

void
ResetForm()
{
	if (f_contents == True) {
		if (f_toggle == False) {
			f_toggle = True;
			cnt = 0;
			XtSetArg(args[cnt], XtNwidth,  form_save_width+1);         
			cnt++;
			XtSetArg(args[cnt], XtNheight, form_save_height+1);         
			cnt++;
			XtSetValues(form, args, cnt);
		} else {
			f_toggle = False;
			cnt = 0;
			XtSetArg(args[cnt], XtNwidth,  form_save_width-1);         
			cnt++;
			XtSetArg(args[cnt], XtNheight, form_save_height-1);         
			cnt++;
			XtSetValues(form, args, cnt);
		}
	}
}


/* ARGSUSED */
void
Print_Area(w, closure, call_data)
Widget w;
XtPointer closure, call_data;
{
	int wt, h;

	SetFooterText(footer_text, "");
	/*SetFooterText(footer_text, "Preparing contents for printer, please wait...");*/
	f_area = True;
	f_bypass = True;

	UnmapTop(w);
	
	if(xlocal)	{
		if (image_content != NULL)
		{
			wt=image_content->width;
			h = image_content->height;
		}

		noflag = 0;
	}

	WindowToImage2(  &image_nocontent,
	    &header_nc,   &header_size_nc,
	    &win_name_nc, &win_name_size_nc,
	    &colors_nc,   &ncolors_nc,
	    &buffer_size_nc);

        /* Grab the pointer using wait cursor, letting it roam all over */
        /* Grab it after routine above */
        XDefineCursor(dpy, rootwindow, wait_cursor);
	XFlush(dpy);

	strcpy(tempfile,"/usr/tmp/olpsXXXXXX");
	(void) mktemp(tempfile);

	/* open + close inside */
	if ( PaneToFile( tempfile,    image_nocontent,
	    &header_nc,
	    win_name_nc, win_name_size_nc,
	    colors_nc,   ncolors_nc,
	    buffer_size_nc) == -1) {
		return;
	}

	if(xlocal)	{
		noflag = 1 ;
		if (image_content != NULL)
		{
			image_content->width = wt;
			image_content->height = h;
		}
	}

	/* opened*/
	if (CanOpenFile(tempfile,"r",&rw_file) == -1) {
		return;
	}

	ExecutePrint(tempfile,True);

	MapTop();
	ResetForm();

	if (!f_failed)
	  SFT(areaSpooled);

	(void) fclose(rw_file);

        /*XUndefineCursor(dpy, rootwindow);*/
        XDefineCursor(dpy, rootwindow, OlGetStandardCursor(toplevel));

	f_bypass = False;
	f_area = False;
}


/* ARGSUSED */
void
Print_Window(w, closure, call_data)
Widget w;
XtPointer closure, call_data;
{

	int wt, h;
	
	SetFooterText(footer_text, "");
	/*SetFooterText(footer_text, "Preparing contents for printer, please wait...");*/
	f_bypass = True;

	UnmapTop(w);

	if(xlocal)	{
		if (image_content != NULL)
	  	{
				/* for pixmap redisplay properly */
	    		wt = image_content->width;
	    		h = image_content->height;
	  	}
	    	noflag = 0;
	}

	WindowToImage(  &image_nocontent,
	    &header_nc,   &header_size_nc,
	    &win_name_nc, &win_name_size_nc,
	    &colors_nc,   &ncolors_nc,
	    &buffer_size_nc);

        /* Grab the pointer using wait cursor, letting it roam all over */
	XDefineCursor(dpy, rootwindow, wait_cursor);
	XFlush(dpy);

	strcpy(tempfile,"/usr/tmp/olpsXXXXXX");
	(void) mktemp(tempfile);

	/* open + close inside */
	if ( PaneToFile( tempfile,    image_nocontent,
	    &header_nc,
	    win_name_nc, win_name_size_nc,
	    colors_nc,   ncolors_nc,
	    buffer_size_nc) == -1) {
		return;
	}

	if(xlocal)	{
	   	noflag = 1;
	  	if (image_content != NULL)
	  	{
	   		image_content->width = wt;
	   		image_content->height = h;
	  	}
	}

	/* opened*/
	if (CanOpenFile(tempfile,"r",&rw_file) == -1) {
		return;
	}

	ExecutePrint(tempfile,True);


        MapTop();
        ResetForm();

	if (!f_failed)
	  SFT(windowSpooled);

	(void) fclose(rw_file);

        /*XUndefineCursor(dpy, rootwindow);*/
        XDefineCursor(dpy, rootwindow, OlGetStandardCursor(toplevel));

	f_bypass = False;
}


/* ARGSUSED */
void
Print_Screen(w, closure, call_data)
Widget w;
XtPointer closure, call_data;
{
	int wt, h;

	SetFooterText(footer_text, "");
	/*SetFooterText(footer_text, "Preparing contents for printer, please wait...");*/
	f_bypass = True;
	f_screen = True;

	UnmapTop(w);

	if(xlocal)	{
		if (image_content != NULL)
		{
	    		wt = image_content->width ;
	    		h = image_content->height ;
		}
	    	noflag = 0;
	}

	WindowToImage(  &image_nocontent,
	    &header_nc,   &header_size_nc,
	    &win_name_nc, &win_name_size_nc,
	    &colors_nc,   &ncolors_nc,
	    &buffer_size_nc);

        /* Define the pointer using wait cursor, letting it roam all over */
	XDefineCursor(dpy, rootwindow, wait_cursor);
	XFlush(dpy);

	strcpy(tempfile,"/usr/tmp/olpsXXXXXX");
	(void) mktemp(tempfile);

	/* open + close inside */
	if ( PaneToFile( tempfile,    image_nocontent,
	    &header_nc,
	    win_name_nc, win_name_size_nc,
	    colors_nc,   ncolors_nc,
	    buffer_size_nc) == -1) {
		return;
	}

	/* opened*/
	if (CanOpenFile(tempfile,"r",&rw_file) == -1) {
		return;
	}

	ExecutePrint(tempfile,True);

	if(xlocal)	{
		if (image_content != NULL)
		{
	    		image_content->width = wt ;
	    		image_content->height = h ;
		}
	    	noflag = 1;
	}

	MapTop();
	ResetForm();

	if (!f_failed)
	  SFT(screenSpooled);

	(void) fclose(rw_file);

        /*XUndefineCursor(dpy, rootwindow);*/
        XDefineCursor(dpy, rootwindow, OlGetStandardCursor(toplevel));
        /*XUngrabPointer(dpy, CurrentTime);       Done with pointer */

	f_bypass = False;
	f_screen = False;
}

void
filescrollCB(widget, clientData,callData)
Widget widget;
infostruct * clientData;
XtPointer callData;

{
 	OlListToken token = (OlListToken) callData;
	OlListItem *selected_listitem;

        selected_listitem = OlListItemPointer(token);
	XtPopdown( *clientData->popup);
	(*(clientData->funcptr))(selected_listitem->label);
}



/* ARGSUSED */
void
Redraw(w, event, region)
Widget w;
XEvent *event;
Region region;
{
	if (f_contents == True){
		redraw=True;
		XFlush(dpy);
		XCopyArea( dpy, pix, stub_window, gc, 0, 0, image_content->width, 
		    image_content->height, 0, 0);
	}
	redraw=False;
}


void
UnmapTop(w)
Widget w;
{
	Widget Shell = _OlGetShellOfWidget(w);

	if (f_contents == True) {
		cnt = 0;
		XtSetArg(args[cnt], XtNwidth,  &form_save_width);         
		cnt++;
		XtSetArg(args[cnt], XtNheight, &form_save_height);         
		cnt++;
		XtGetValues(form, args, cnt);
	}


	if (XtIsSubclass(Shell, popupMenuShellWidgetClass) == True){
		/* check it out */
		OlUnpostPopupMenu(Shell);
	}

	XSynchronize(dpy, 1);
	f_unmapping = True;
	if (f_prop_pop == True)
		XtUnmapWidget(prop_pop);
	if (f_printf_pop == True)
		XtUnmapWidget(printf_pop);
	if (f_save_pop == True)
		XtUnmapWidget(save_pop);
	if (f_open_pop == True)
		XtUnmapWidget(open_pop);

	XtUnmapWidget(toplevel);
	XFlush(dpy);
	XSynchronize(dpy, 0);
}


void
MapTop()
{
	XtMapWidget(toplevel);
	if (f_prop_pop == True)
		XtMapWidget(prop_pop);
	if (f_open_pop == True)
		XtMapWidget(open_pop);
	if (f_save_pop == True)
		XtMapWidget(save_pop);
	if (f_printf_pop == True)
		XtMapWidget(printf_pop);
	XRaiseWindow( dpy, XtWindow(toplevel));
	f_unmapping = False;
	XFlush(dpy);
}

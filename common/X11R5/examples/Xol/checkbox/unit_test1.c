/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olexamples:checkbox/unit_test1.c	1.12"
#endif

/*************************************************************************/
/*	SAMPLE USER INTERFACE FOR CREATION OF CHECKBOX WIDGET		*/
/*	Karen S. Kendler     			30 September 1988	*/
/*	Copyright (c) 1989 AT&T						*/
/************************************************************************/

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>		/* needed for menu behavior type */
#include <Xol/Exclusives.h>
#include <Xol/Form.h>
#include <Xol/RectButton.h>
#include <Xol/CheckBox.h>

char *malloc();
Widget checkbox;
int ncolors=16;

void select_callback(widget,clientData,callData)  /* TEST SELECT CALLBACK */
	Widget widget;
	caddr_t clientData, callData;
{
	printf("SELECT callback called\n");
}

void unselect_callback(widget,clientData,callData)  /* TEST UNSELECT CALLBACK */
	Widget widget;
	caddr_t clientData, callData;
{
	printf("UNSELECT callback called\n");
}

void callback0(widget,clientData,callData)	/* TOGGLE LABEL */
	Widget widget;
	caddr_t clientData, callData;
{
	static int count;
	Arg arg;
	Widget checkbox;
	char *biglabel="BIGGGGGGGLABELLLLLLL";
	char *label="LABEL";

	checkbox= (Widget) clientData;

	if(count==2) count=0;			/* recycle */
	if(count==0) {
		XtSetArg(arg,XtNlabel,(XtArgVal) biglabel);
	}
	else {
		XtSetArg(arg,XtNlabel,(XtArgVal) label);
	}

	XtSetValues(checkbox,&arg,1);
	count++;
}

void callback1(widget,clientData,callData)	/* TOGGLE DIM */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;
	static int count;
	Widget checkbox;

	checkbox= (Widget) clientData;

	if(count==2) count=0;			/* recycle */
	if(count==0) {
		XtSetArg(arg,XtNdim,(XtArgVal) TRUE);
	}
	else {
		XtSetArg(arg,XtNdim,(XtArgVal) FALSE);
	}
	XtSetValues(checkbox,&arg,1);
	count++;
}

void callback2(widget,clientData,callData)	/* TOGGLE SENSITIVE */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;
	static int count;
	Widget checkbox;

	checkbox= (Widget) clientData;

	if(count==2) count=0;			/* recycle */
	if(count==0) {
		XtSetSensitive(checkbox,FALSE);
	}
	else {
		XtSetSensitive(checkbox,TRUE);
	}
	count++;
}

void callback3(widget,clientData,callData) 	/* TOGGLE WIDGET SIZE */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg[2];
	static int count;
	Widget checkbox;

	checkbox= (Widget) clientData;

	if(count==3) count=0;			/* recycle */
	if(count==0) {
		XtSetArg(arg[0],XtNheight,(XtArgVal) 100);
		XtSetArg(arg[1],XtNwidth,(XtArgVal) 300);
	}
	else if(count==1){
		XtSetArg(arg[0],XtNheight,(XtArgVal) 200);
		XtSetArg(arg[1],XtNwidth,(XtArgVal) 600);
	}
	else if(count==2){
		XtSetArg(arg[0],XtNheight,(XtArgVal) 400);
		XtSetArg(arg[1],XtNwidth,(XtArgVal) 12000);
	}

	XtSetValues(checkbox,arg,2);
	count++;
}

void callback4(widget,clientData,callData)	/* PRINT LABEL */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;
	Widget checkbox;
	char *string;
	XFontStruct font;

	checkbox= (Widget) clientData;

	XtSetArg(arg,XtNlabel,(XtArgVal) &string);
	XtGetValues(checkbox,&arg,1);
	printf("Current string %s\n",string);
	XtSetArg(arg,XtNfont,(XtArgVal) &font);
	XtGetValues(checkbox,&arg,1);
	printf("Current font ID is %d\n",font.fid);
}

void callback5(widget,clientData,callData)	/* TOGGLE JUSTIFY */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;
	static int count;
	Widget checkbox;

	checkbox= (Widget) clientData;

	if(count==2) count=0;			/* recycle */
	if(count==0) {
		XtSetArg(arg,XtNjustify,(XtArgVal) OL_RIGHT);
	}
	else {
		XtSetArg(arg,XtNjustify,(XtArgVal) OL_LEFT);
	}
	XtSetValues(checkbox,&arg,1);
	count++;
}

void callback6(widget,clientData,callData)	/* TOGGLE SET */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;
	static int count;
	Widget checkbox;

	checkbox= (Widget) clientData;

	if(count==2) count=0;			/* recycle */
	if(count==0) {
		XtSetArg(arg,XtNset,(XtArgVal) TRUE);
	}
	else {
		XtSetArg(arg,XtNset,(XtArgVal) FALSE);
	}
	XtSetValues(checkbox,&arg,1);
	count++;
}

void callback7(widget,clientData,callData)	/* TOGGLE FONT COLOR */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;
	static int ncolor,first_time;

	checkbox= (Widget) clientData;

	if(first_time==0) {
		ncolor=-1;
		first_time=1;
	}
	ncolor++;
	if(ncolor>=16) ncolor=0;			/* recycle */
	XtSetArg(arg,XtNfontColor,(XtArgVal) ncolor);
	XtSetValues(checkbox,&arg,1);
}

void callback8(widget,clientData,callData)	/* TOGGLE BUTTON */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;
	static int count;
	Widget checkbox;

	checkbox= (Widget) clientData;

	if(count==2) count=0;			/* recycle */
	if(count==0) {
		XtSetArg(arg,XtNshellBehavior,(XtArgVal) PressDragReleaseMenu);
	}
	else {
		XtSetArg(arg,XtNshellBehavior,(XtArgVal) OtherBehavior);
	}
	XtSetValues(checkbox,&arg,1);
	count++;
}

void callback9(widget,clientData,callData)	/* TOGGLE SCALE */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;
	static int count;
	Widget checkbox;

	checkbox= (Widget) clientData;

	if(count==4) count=0;			/* recycle */
	
	switch(count) {
		case 0:
			XtSetArg(arg,XtNscale,(XtArgVal) SMALL_SCALE);
			break;
		case 1:
			XtSetArg(arg,XtNscale,(XtArgVal) MEDIUM_SCALE);
			break;
		case 2:
			XtSetArg(arg,XtNscale,(XtArgVal) LARGE_SCALE);
			break;
		case 3:
			XtSetArg(arg,XtNscale,(XtArgVal) EXTRA_LARGE_SCALE);
			break;
	}
	XtSetValues(checkbox,&arg,1);
	count++;
}

void callback10(widget,clientData,callData)	/* ERROR TRAPPING */
	Widget widget;
	caddr_t clientData, callData;
{
	Widget parent,new,checkbox,badparent,checkbox2;
	Arg arg[3];

	checkbox= (Widget) clientData;
	parent = XtParent(checkbox); 

	new=XtCreateManagedWidget("rb",rectButtonWidgetClass,checkbox,NULL,0);

	XtSetArg(arg[0],XtNborderWidth,(XtArgVal) 3);
	XtSetArg(arg[1],XtNborderColor,(XtArgVal) 3);
	XtSetArg(arg[2],XtNbackground,(XtArgVal) 3);
	XtSetValues(checkbox,arg,2);
}

void callback11(widget,clientData,callData)	/* TOGGLE BACKGROUND */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;
	static int ncolor,first_time;
	Widget parent;

	checkbox= (Widget) clientData;
	parent=XtParent(checkbox);

	if(first_time==0) {
		ncolor=-1;
		first_time=1;
	}
	ncolor++;
	if(ncolor>=16) ncolor=0;			/* recycle */
	XtSetArg(arg,XtNbackground,(XtArgVal) ncolor);
	XtSetValues(parent,&arg,1);
	XtSetArg(arg,XtNbackground,(XtArgVal) ncolor);
	XtSetValues(checkbox,&arg,1);
}

void callback12(widget,clientData,callData)	/* TOGGLE FOREGROUND */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;
	static int ncolor,first_time;

	checkbox= (Widget) clientData;

	if(first_time==0) {
		ncolor=-1;
		first_time=1;
	}
	ncolor++;
	if(ncolor>=16) ncolor=0;			/* recycle */
	XtSetArg(arg,XtNforeground,(XtArgVal) ncolor);
	XtSetValues(checkbox,&arg,1);
}

void callback13(widget,clientData,callData)	/* TOGGLE POSITION */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;
	static int count;
	Widget checkbox;

	checkbox= (Widget) clientData;

	if(count==2) count=0;			/* recycle */
	if(count==0) {
		XtSetArg(arg,XtNposition,(XtArgVal) OL_RIGHT);
	}
	else {
		XtSetArg(arg,XtNposition,(XtArgVal) OL_LEFT);
	}
	XtSetValues(checkbox,&arg,1);
	count++;
}

void bye(widget,clientData,callData)			/* EXIT DEMO */
	Widget widget;				/* TRY TO UNMANAGE A CHILD */
	caddr_t clientData, callData;
{
	printf("adios\n\n");
	exit(0);
}

/* P R O G R A M M E R   I N T E R F A C E */

/* WIDGET TREE: 

	TOPLEVEL
	FORM	
	ONE EXCLUSIVES, CHILD OF FORM 
	ONE CHECKBOX, CHILD OF FORM 
*/

int main(argc,argv)
	unsigned int argc;
	char **argv;
{

Widget	toplevel,form,exclusives,button;

Arg arg[20];
int i,j,n,r;

toplevel= OlInitialize("CB","demo",NULL,0,&argc, argv);

XtSetArg(arg[0],XtNbackground,(XtArgVal) 13);
XtSetArg(arg[1],XtNheight,(XtArgVal) 500);
XtSetArg(arg[2],XtNwidth,(XtArgVal) 500);

form=
   XtCreateManagedWidget("form",formWidgetClass,toplevel,arg,3);

r=0;
XtSetArg(arg[r],XtNlayoutType,(XtArgVal) OL_FIXEDCOLS);
r++;
XtSetArg(arg[r],XtNmeasure,(XtArgVal) 4);
r++;
XtSetArg(arg[r],XtNnoneSet,(XtArgVal) TRUE);
r++;
XtSetArg(arg[r],XtNxRefWidget, (caddr_t) form );
r++;
XtSetArg(arg[r],XtNyRefWidget, (caddr_t) form );
r++;
XtSetArg(arg[r],XtNxOffset,(XtArgVal) 5);
r++;
XtSetArg(arg[r],XtNyOffset,(XtArgVal) 5);
r++;
XtSetArg(arg[r],XtNxAddWidth,(XtArgVal) TRUE);
r++;
XtSetArg(arg[r],XtNyAddHeight,(XtArgVal) TRUE);

exclusives=
   XtCreateManagedWidget("exclusives",exclusivesWidgetClass,form,arg,9);

r=0;
XtSetArg(arg[r],XtNlayoutType,(XtArgVal) OL_FIXEDCOLS);
r++;
XtSetArg(arg[r],XtNmeasure,(XtArgVal) 4);
r++;
XtSetArg(arg[r],XtNnoneSet,(XtArgVal) TRUE);
r++;
XtSetArg(arg[r],XtNxRefWidget, (caddr_t) form );
r++;
XtSetArg(arg[r],XtNyRefWidget, (caddr_t) exclusives );
r++;
XtSetArg(arg[r],XtNxOffset,(XtArgVal) 5);
r++;
XtSetArg(arg[r],XtNyOffset,(XtArgVal) 5);
r++;
XtSetArg(arg[r],XtNxAddWidth,(XtArgVal) TRUE);
r++;
XtSetArg(arg[r],XtNyAddHeight,(XtArgVal) TRUE);
r++;
XtSetArg(arg[r],XtNset,(XtArgVal) TRUE);
r++;
XtSetArg(arg[r],XtNforeground,(XtArgVal) 3);
r++;
XtSetArg(arg[r],XtNscale,(XtArgVal) LARGE_SCALE);
r++;
XtSetArg(arg[r],XtNlabel,(XtArgVal) "LABEL");
r++;
XtSetArg(arg[r],XtNsensitive,(XtArgVal) TRUE);
r++;

checkbox=
   XtCreateManagedWidget("checkbox",checkBoxWidgetClass,form,arg,r);

   XtAddCallback(checkbox,XtNselect,select_callback,NULL);
   XtAddCallback(checkbox,XtNunselect,unselect_callback,NULL);

XtSetArg(arg[0],XtNlabel,(XtArgVal)"LABEL");
button=					/* this button toggles label */
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback0,(caddr_t)checkbox);
				
XtSetArg(arg[0],XtNlabel,(XtArgVal)"GETVAL");
button=					/* this button prints label */
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback4,(caddr_t)checkbox);
				
XtSetArg(arg[0],XtNlabel,(XtArgVal)"DIM");
button=				/* this button sets & unsets XtNdim */
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback1,(caddr_t)checkbox);

XtSetArg(arg[0],XtNlabel,(XtArgVal)"SENSITIVE");
button=				/* this button sets & unsets XtNsensitive */
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback2,(caddr_t)checkbox);

XtSetArg(arg[0],XtNlabel,(XtArgVal)"JUSTIFY");
button=				/* this button toggles left & right justify */
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback5,(caddr_t)checkbox);

XtSetArg(arg[0],XtNlabel,(XtArgVal)"SET");
button=				/* this button toggles set & unset */
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback6,(caddr_t)checkbox);

XtSetArg(arg[0],XtNlabel,(XtArgVal)"BUTTON");
button=				/* this button toggles 1 & 3  */
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback8,(caddr_t)checkbox);

XtSetArg(arg[0],XtNlabel,(XtArgVal)"BACKGROUND");
button=			/* this button tests position switch */
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback11,(caddr_t)checkbox);

XtSetArg(arg[0],XtNlabel,(XtArgVal)"FOREGROUND");
button=			/* this button tests foreground switch */
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback12,(caddr_t)checkbox);

XtSetArg(arg[0],XtNlabel,(XtArgVal)"POSITION");
button=			/* this button tests position switch */
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback13,(caddr_t)checkbox);

XtSetArg(arg[0],XtNlabel,(XtArgVal)"FONT COLOR");
button=			/* this button tests foreground switch */
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback7,(caddr_t)checkbox);

XtSetArg(arg[0],XtNlabel,(XtArgVal)"ERRORS");
button=			/* this button tests trapping of miscellaneous errors */
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback10,(caddr_t)checkbox);

XtSetArg(arg[0],XtNlabel,(XtArgVal) "EXIT");	/* this is the QUIT button */
button=XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,bye,NULL);

end: {}

XtRealizeWidget(toplevel); 
XtMainLoop();
}

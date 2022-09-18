/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olexamples:nonexclus/unit_test2.c	1.11"
#endif

/*************************************************************************/
/*	SAMPLE USER INTERFACE FOR CREATION OF NONEXCLUSIVES WIDGET	*/
/*	Karen S. Kendler     			21 August 1988		*/
/*	Copyright (c) 1989 AT&T						*/
/************************************************************************/

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Nonexclusi.h>
#include <Xol/CheckBox.h>
#include <Xol/RectButton.h>

int nbuttons=0, ncolors=16;

void callback4();

void callback0(widget,nbuttonsptr,callData)	/* RESET NUMBER OF COLUMNS */
	Widget widget;
	caddr_t nbuttonsptr, callData;
{
	Arg arg[2];
	Widget parent;
	int buttons;
	static int measure;

	measure++;
	buttons= (int) (*nbuttonsptr);
	if(measure>buttons) measure=1;			/* recycle */
	parent=XtParent(widget);
	XtSetArg(arg[0],XtNlayoutType,(XtArgVal) OL_FIXEDCOLS);
	XtSetArg(arg[1],XtNmeasure,(XtArgVal) measure);
	XtSetValues(parent,arg,2);
}

void callback00(widget,nbuttonsptr,callData)	/* RESET NUMBER OF ROWS */
	Widget widget;
	caddr_t nbuttonsptr, callData;
{
	Arg arg[2];
	Widget parent;
	int buttons;
	static int measure;

	measure++;
	buttons= (int) (*nbuttonsptr);
	if(measure>buttons) measure=1;			/* recycle */
	parent=XtParent(widget);
	XtSetArg(arg[0],XtNlayoutType,(XtArgVal) OL_FIXEDROWS);
	XtSetArg(arg[1],XtNmeasure,(XtArgVal) measure);
	XtSetValues(parent,arg,2);
}

void callback1(widget,clientData,callData)	/* ADD A BUTTON */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;
	Widget parent,new;
	char *label="MORE OF ME";

	parent=XtParent(widget);

	XtSetArg(arg,XtNlabel,(XtArgVal) label);
	new=XtCreateManagedWidget("rb",checkBoxWidgetClass,parent,&arg,1);
	XtAddCallback(new,XtNselect,callback1,NULL);
	nbuttons++;
}

void callback2(widget,clientData,callData)	/* DELETE A BUTTON */
	Widget widget;
	caddr_t clientData, callData;
{
	XtDestroyWidget(widget);
	nbuttons--;
}

void callback3(widget,clientData,callData)	/* EXPAND BUTTON LABEL */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg[3];
	char *newlabel="BIGGGGGGGGGGGGG";

	XtSetArg(arg[0],XtNlabel,(XtArgVal) newlabel);
	XtSetValues(widget,arg,1);
	XtRemoveCallback(widget,XtNselect,callback3,NULL);
	XtAddCallback(widget,XtNselect,callback4,NULL);
}

void callback4(widget,clientData,callData) /* SHRINK/EXPAND A BUTTON LABEL */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg[3];
	char *newlabel="TINY";

	XtSetArg(arg[0],XtNlabel,(XtArgVal) newlabel);
	XtSetValues(widget,arg,1);
	XtRemoveCallback(widget,XtNselect,callback4,NULL);
	XtAddCallback(widget,XtNselect,callback3,NULL);
}

void callback5(widget,ncolorsptr,callData)	/* CHANGE COLOR */
	Widget widget;
	caddr_t ncolorsptr, callData;
{
	Arg arg;
	int ncolors;
	static int ncolor,first_time;

	if(first_time==0) {
		ncolor=-1;
		ncolors= (int) (*ncolorsptr);
		first_time=1;
	}
	ncolor++;
	if(ncolor>ncolors) ncolor=0;			/* recycle */
	XtSetArg(arg,XtNforeground,(XtArgVal) ncolor);
	XtSetValues(widget,&arg,1);
}

void callback9(widget,clientData,callData)	/* TEST UNSELECT CALLBACK #1 */
	Widget widget;
	caddr_t clientData, callData;
{
	printf("FIRST UNSELECT CALLBACK\n");
}

void callback10(widget,clientData,callData)	/* TEST UNSELECT CALLBACK #2 */
	Widget widget;
	caddr_t clientData, callData;
{
	printf("SECOND UNSELECT CALLBACK\n");
}

void callback13(widget,clientData,callData)	/* TEST RECONFIGURING BUTTON */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg[4];
	
	XtSetArg(arg[0],XtNx,(XtArgVal) 1000);
	XtSetArg(arg[1],XtNy,(XtArgVal) 1000);
	XtSetArg(arg[2],XtNheight,(XtArgVal) 1000);
	XtSetArg(arg[3],XtNwidth,(XtArgVal) 1000);
	XtSetValues(widget,arg,4);
}

void callback15(widget,clientData,callData)	/* TOGGLE JUSTIFY */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;

	XtSetArg(arg,XtNlabelJustify,(XtArgVal) 0);
	XtGetValues(widget,&arg,1);
	if(arg.value==OL_LEFT) {
		XtSetArg(arg,XtNlabelJustify,(XtArgVal) OL_RIGHT);
	}
	else if(arg.value==OL_RIGHT) {
		XtSetArg(arg,XtNlabelJustify,(XtArgVal) OL_LEFT);
	}
	XtSetValues(widget,&arg,1);
}

void callback14(widget,clientData,callData)	/* TEST ERROR CONDITIONS */
	Widget widget;
	caddr_t clientData, callData;
{
	Widget parent;
	Arg arg[2];

	parent=XtParent(widget);
	
	XtSetArg(arg[0],XtNborderWidth,(XtArgVal) 3);
	XtSetArg(arg[1],XtNbackground,(XtArgVal) 13);
	XtSetValues(parent,arg,2);
	XtUnmanageChild(widget);
   	XtCreateManagedWidget("wrong",rectButtonWidgetClass,parent,NULL,0);
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
	ONE NONEXCLUSIVES, CHILD OF TOPLEVEL
	VARIABLE NUMBER OF CHECKBOXES, CHILDREN OF NONEXCLUSIVES
*/

int main(argc,argv)
	unsigned int argc;
	char **argv;
{

Widget	toplevel,box,nonexclusives,button;

Arg arg[5];
int i,j,n;

toplevel= OlInitialize("XB","demo",NULL,0,&argc, argv);

XtSetArg(arg[0],XtNlayoutType,(XtArgVal) OL_FIXEDCOLS);
XtSetArg(arg[1],XtNmeasure,(XtArgVal) 3);

nonexclusives=
   XtCreateManagedWidget("nonexclusives",nonexclusivesWidgetClass,toplevel,arg,2);

XtSetArg(arg[0],XtNlabel,(XtArgVal)"POOF");
XtSetArg(arg[1],XtNset,(XtArgVal)TRUE);	    /* will be unset */
button=					/* this button deletes itself */
   XtCreateManagedWidget("rb",checkBoxWidgetClass,nonexclusives,arg,2);
XtAddCallback(button,XtNselect,callback2,NULL);
nbuttons++;

					/* this button tries to reconfigure */
XtSetArg(arg[0],XtNlabel,(XtArgVal)"RECONFIGURE");
button=
XtCreateManagedWidget("rb",checkBoxWidgetClass,nonexclusives,arg,1);
XtAddCallback(button,XtNselect,callback13,NULL);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal)"COLUMNS"); /* this button changes config. */
button=
XtCreateManagedWidget("rb",checkBoxWidgetClass,nonexclusives,arg,1);
XtAddCallback(button,XtNselect,callback0,&nbuttons);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal)"ROWS"); /* this button changes config. */
button=
XtCreateManagedWidget("rb",checkBoxWidgetClass,nonexclusives,arg,1);
XtAddCallback(button,XtNselect,callback00,&nbuttons);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal) "MORE OF ME"); /* button reproduces */
XtSetArg(arg[1],XtNset,(XtArgVal)TRUE);	/* tests setting second button */
button=	
   XtCreateManagedWidget("rb",checkBoxWidgetClass,nonexclusives,arg,2);
XtAddCallback(button,XtNselect,callback1,NULL);
XtAddCallback(button,XtNunselect,callback9,NULL); /* tests unselect callback */
XtAddCallback(button,XtNunselect,callback10,NULL); /* tests unselect callback */
nbuttons++;

      /* this button changes own label to test expanding & shrinking buttons */
XtSetArg(arg[0],XtNlabel,(XtArgVal)"TINY"); 
button=
XtCreateManagedWidget("rb",checkBoxWidgetClass,nonexclusives,arg,1);
XtAddCallback(button,XtNselect,callback3,NULL);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal)"COLOR"); /* this button changes own color */
button=
XtCreateManagedWidget("rb",checkBoxWidgetClass,nonexclusives,arg,1);
XtAddCallback(button,XtNselect,callback5,&ncolors);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal)"JUSTIFY1"); /* toggles label position */
XtSetArg(arg[1],XtNposition,(XtArgVal)OL_LEFT);
XtSetArg(arg[2],XtNjustify,(XtArgVal)OL_RIGHT);
button=
XtCreateManagedWidget("rb",checkBoxWidgetClass,nonexclusives,arg,3);
XtAddCallback(button,XtNselect,callback15,NULL);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal)"JUSTIFY2"); /* toggles label position */
XtSetArg(arg[1],XtNposition,(XtArgVal)OL_RIGHT);
XtSetArg(arg[2],XtNjustify,(XtArgVal)OL_RIGHT);
button=
XtCreateManagedWidget("rb",checkBoxWidgetClass,nonexclusives,arg,3);
XtAddCallback(button,XtNselect,callback15,NULL);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal)"ERRORS"); /* this button generates errors */
button=
XtCreateManagedWidget("rb",checkBoxWidgetClass,nonexclusives,arg,1);
XtAddCallback(button,XtNselect,callback14,NULL);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal)"DIM"); /* this button loses dim when set */
XtSetArg(arg[1],XtNdim,(XtArgVal)TRUE);  /* this button loses dim when set */
button=
XtCreateManagedWidget("rb",checkBoxWidgetClass,nonexclusives,arg,2);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal) "EXIT");	/* this is the QUIT button */
button=XtCreateManagedWidget("rb",checkBoxWidgetClass,nonexclusives,arg,1);
XtAddCallback(button,XtNselect,bye,NULL);
nbuttons++;

XtRealizeWidget(toplevel); 
XtMainLoop();
}

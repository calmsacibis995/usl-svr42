/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olexamples:exclusives/unit_test1.c	1.12"
#endif

/*************************************************************************/
/*	SAMPLE USER INTERFACE FOR CREATION OF EXCLUSIVES WIDGET		*/
/*	Karen S. Kendler     			21 August 1988		*/
/*	Copyright (c) 1989 AT&T						*/
/************************************************************************/

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>		/* needed for menu behavior type */
#include <Xol/RectButton.h> 
#include <Xol/Exclusives.h> 

char *malloc();
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
	new=XtCreateManagedWidget("rb",rectButtonWidgetClass,parent,&arg,1);
	XtAddCallback(new,XtNselect,callback1,NULL);
	nbuttons++;
}

void callback2(widget,clientData,callData)	/* CHECK FOR DEFAULT */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;
	Widget parent;

	parent=XtParent(widget);

	XtSetArg(arg,XtNdefault,(XtArgVal) 0);
	XtGetValues(parent,&arg,1);

	if(arg.value==FALSE) {
		printf("THERE IS NO DEFAULT\n");
	}
	else if(arg.value==TRUE) {
		printf("THERE IS A DEFAULT\n");
	}
}

void callback3(widget,clientData,callData)	/* EXPAND BUTTON LABEL */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg[3];
	char *newlabel="BIGGGGGGGGGGGGGGGG";

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

void callback6(widget,clientData,callData)	/* SET SELF ON/OFF AS DEFAULT */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;

	XtSetArg(arg,XtNdefault,(XtArgVal) 0);
	XtGetValues(widget,&arg,1);
	if(arg.value==FALSE) {
		XtSetArg(arg,XtNdefault,(XtArgVal) TRUE);
	}
	else if(arg.value==TRUE) {
		XtSetArg(arg,XtNdefault,(XtArgVal) FALSE);
	}
	XtSetValues(widget,&arg,1);
}

void callback7(widget,clientData,callData)	/* TRIGGER DEFAULT */
	Widget widget;
	caddr_t clientData, callData;
{
	Widget parent;
	Arg arg;

	parent=XtParent(widget);
	XtSetArg(arg,XtNtrigger,(XtArgVal) TRUE);
	XtSetValues(parent,&arg,1);
}

void callback8(widget,clientData,callData)	/* TEST CHOGGLES */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;
	Widget parent;
	static int count;

	count++;
	if(count==2) count=0;			/* recycle */
	parent=XtParent(widget);
	if(count==0) {
		XtSetArg(arg,XtNnoneSet,(XtArgVal) FALSE);
	}
	else {
		XtSetArg(arg,XtNnoneSet,(XtArgVal) TRUE);
	}
	XtSetValues(parent,&arg,1);
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

void callback11(widget,clientData,callData)	/* TEST DESTROY WIDGET */
	Widget widget;
	caddr_t clientData, callData;
{
	XtDestroyWidget(widget);
	nbuttons--;
}

void callback12(widget,clientData,callData) /* TEST STORING & RETRIEVING DATA */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;
	char c='s', *cptr;
	int i=1, *iptr;
	caddr_t cdrt=(caddr_t) 12345, *cdrtptr;

	XtSetArg(arg,XtNdefaultData,(XtArgVal) &c);
	XtSetValues(widget,&arg,1);
	XtGetValues(widget,&arg,1);
	cptr= (char *) arg.value;
	printf("SetValues was char c = 's' : GetValues: %c\n",*cptr);

	XtSetArg(arg,XtNdefaultData,(XtArgVal) &i);
	XtSetValues(widget,&arg,1);
	XtGetValues(widget,&arg,1);
	iptr= (int *) arg.value;
	printf("SetValues was int i = 1 : GetValues: %d\n",*iptr);

	XtSetArg(arg,XtNdefaultData,(XtArgVal) &cdrt);
	XtSetValues(widget,&arg,1);
	XtGetValues(widget,&arg,1);
	cdrtptr= (caddr_t *) arg.value;
	printf("SetValues was caddr_t = 12345 : GetValues: %d\n",*cdrtptr);
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

void callback14(widget,clientData,callData)	/* TEST ERROR CONDITIONS */
	Widget widget;
	caddr_t clientData, callData;
{
	Widget parent;
	Arg arg[2];

	parent=XtParent(widget);
	
	XtSetArg(arg[0],XtNborderWidth,(XtArgVal) 3);
	XtSetArg(arg[1],XtNbackground,(XtArgVal) 3);
	XtSetValues(parent,arg,2);
	XtUnmanageChild(widget);
}

void callback15(widget,clientData,callData)	/* TEST MENU BEHAVIOR */
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;
	Widget parent;
	static int count;

	count++;
	if(count==2) count=0;			/* recycle */
	parent=XtParent(widget);
	if(count==0) {
		XtSetArg(arg,XtNshellBehavior,(XtArgVal) PressDragReleaseMenu);
	}
	else {
		XtSetArg(arg,XtNshellBehavior,(XtArgVal) OtherBehavior);
	}
	XtSetValues(parent,&arg,1);
}

void postselect(widget,clientData,callData)
	Widget widget;
	caddr_t clientData, callData;
{
	printf("POST SELECT CALLBACK TEST\n");
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
	ONE EXCLUSIVES, CHILD OF TOPLEVEL
	VARIABLE NUMBER OF RECTANGULAR BUTTONS, CHILDREN OF EXCLUSIVES
*/

int main(argc,argv)
	unsigned int argc;
	char **argv;
{

Widget	toplevel,exclusives,exclusives2,button;

Arg arg[10];
int i,j,n;

toplevel= OlInitialize("XB","demo",NULL,0,&argc, argv);

XtSetArg(arg[0],XtNlayoutType,(XtArgVal) OL_FIXEDCOLS);
XtSetArg(arg[1],XtNmeasure,(XtArgVal) 3);
XtSetArg(arg[2],XtNborderWidth,(XtArgVal) 3);
XtSetArg(arg[3],XtNborderColor,(XtArgVal) 3);
XtSetArg(arg[4],XtNrecomputeSize,(XtArgVal) TRUE);
XtSetArg(arg[5],XtNheight,(XtArgVal) 100);
XtSetArg(arg[6],XtNwidth,(XtArgVal) 300);

exclusives=
   XtCreateManagedWidget("exclusives",exclusivesWidgetClass,toplevel,arg,7);

XtSetArg(arg[0],XtNlabel,(XtArgVal)"POOF");
button=					/* this button deletes itself */
   XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback11,NULL);
nbuttons++;
					/* this button tries to reconfigure */
XtSetArg(arg[0],XtNlabel,(XtArgVal)"RECONFIGURE");
XtSetArg(arg[1],XtNx,(XtArgVal) 1000);
XtSetArg(arg[2],XtNy,(XtArgVal) 1000);
XtSetArg(arg[3],XtNheight,(XtArgVal) 1000);
XtSetArg(arg[4],XtNwidth,(XtArgVal) 1000);
XtSetArg(arg[5],XtNdim,(XtArgVal) TRUE);
button=
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,6);
XtAddCallback(button,XtNselect,callback13,NULL);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal)"DATA");/* button stores & retrieves data */
XtSetArg(arg[1],XtNdim,(XtArgVal) TRUE);
button=
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,2);
XtAddCallback(button,XtNselect,callback12,NULL);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal)"COLUMNS"); /* this button changes config. */
XtSetArg(arg[1],XtNdim,(XtArgVal) TRUE);
button=
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,2);
XtAddCallback(button,XtNselect,callback0,&nbuttons);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal)"ROWS"); /* this button changes config. */
button=
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback00,&nbuttons);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal)"CHOGGLES"); /* from FALSE to TRUE & vv */
button=
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback8,&nbuttons);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal)"DEFAULT1");	/* button sets self default */
button=
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback6,NULL);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal)"DEFAULT2");	/* button sets self default */
button=
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback6,NULL);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal)"TRIGGER");/* this button triggers default */
button=
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback7,NULL);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal) "MORE OF ME"); /* button reproduces */
XtSetArg(arg[1],XtNset,(XtArgVal)TRUE);	/* tests setting first button */
button=	
   XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,2);
XtAddCallback(button,XtNselect,callback1,NULL);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal)"IS DEFAULT");
XtSetArg(arg[1],XtNset,(XtArgVal)TRUE);	/* tests setting second button */
button=					/* tests is_default */
   XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,2);
XtAddCallback(button,XtNselect,callback2,NULL);
XtAddCallback(button,XtNunselect,callback9,NULL); /* tests unselect callback */
XtAddCallback(button,XtNunselect,callback10,NULL); /* tests unselect callback */
nbuttons++;

      /* this button changes own label to test expanding & shrinking buttons */
XtSetArg(arg[0],XtNlabel,(XtArgVal)"TINY"); 
button=
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback3,NULL);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal)"COLOR"); /* this button changes own color */
button=
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback5,&ncolors);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal)"ERRORS"); /* this button generates errors */
button=
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback14,NULL);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal)"MENU/NONMENU"); /* tests menu behavior */
button=
XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,callback15,NULL);
nbuttons++;

XtSetArg(arg[0],XtNlabel,(XtArgVal) "EXIT");	/* this is the QUIT button */
button=XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(button,XtNselect,bye,NULL);
nbuttons++;

XtRealizeWidget(toplevel); 
XtMainLoop();
}

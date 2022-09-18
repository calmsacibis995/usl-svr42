/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olexamples:form/unit_test1.c	1.12"
#endif

/*************************************************************************/
/*	SAMPLE USER INTERFACE FOR CREATION OF FORM WIDGET		*/
/*	Karen S. Kendler     			30 September 1988	*/
/*									*/
/*	Copyright (c) 1989 AT&T						*/
/*	Copyright (c) 1988 Hewlett-Packard Company 			*/
/*	Copyright (c) 1988 Massachusetts Institute of Technology	*/
/************************************************************************/

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/X.h>

#include <Xol/OpenLook.h>
#include <Xol/Form.h>
#include <Xol/Exclusives.h>
#include <Xol/Button.h>
#include <Xol/RectButton.h>

#include <Xol/OblongButt.h>
#include <Xol/Scrollbar.h>

#include <X11/Shell.h>
#include <Xol/ControlAre.h>
#include <Xol/Caption.h>

int n1,n2,n3,nbuttons=3;
static Widget 	toplevel,
	form,
	title,
	oblbutton,
	exclusives,ebutton1,ebutton2,ebutton3,
	exclusives2,mbutton,
	scrollbar,
	controlarea,cabutton1,cabutton2,cabutton3,
	caption,capbutton;

/* TITLE RESOURCES */

static Arg titleARGS[] = {
						/* widget resources */

	{ XtNlabel,(XtArgVal) "*** OPENLOOK (TM) WIDGETS ***" },
	{ XtNbackground,(XtArgVal) 15 },
	{ XtNforeground,(XtArgVal) 3 },

						/* form resources */
	{ XtNxRefWidget, NULL },
	{ XtNyRefWidget, NULL },
	{ XtNxResizable,(XtArgVal) TRUE },
	{ XtNheight, (XtArgVal) 20},
	{ XtNwidth, (XtArgVal) 100},
	{ XtNxAttachRight,(XtArgVal) TRUE },
	{ XtNlabelJustify,(XtArgVal) OL_CENTER },
};

/* EXCLUSIVES RESOURCES AND CALLBACKS */

			/* resources and callbacks for exclusives widget */

static Arg exclusivesARGS[] = {
						/* widget resources */

	{ XtNlayoutType,(XtArgVal) OL_FIXEDCOLS },
	{ XtNmeasure,(XtArgVal) 3 },

						/* form resources */
	{ XtNxRefWidget, NULL },
	{ XtNyRefWidget, NULL },
	{ XtNxOffset,(XtArgVal) 10 },
	{ XtNyOffset,(XtArgVal) 15 },
	{ XtNxAddWidth,(XtArgVal) TRUE },
	{ XtNyAddHeight,(XtArgVal) TRUE },

	{ XtNheight, (XtArgVal) 200 },
	{ XtNwidth, (XtArgVal) 200 },
};

void exclusivesCB(widget,clientData,callData)
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg[2];
	int n;
	Widget parent;
	int measure;

	n= (int) *clientData;
	printf("EXCLUSIVES callback for button %d\n",n);

}

void exclusivesCB2(widget,clientData,callData)
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;
	int n;
	Widget parent;

	parent= (Widget) clientData;

	XtUnmapWidget(parent);
	printf("EXCLUSIVES callback for reconfiguation\n");
	XtSetArg(arg,XtNlayoutType,(XtArgVal) 0);
	XtGetValues(parent,&arg,1);

	if(arg.value==OL_FIXEDROWS) {
		XtSetArg(arg,XtNlayoutType,OL_FIXEDCOLS);
		XtSetValues(parent,&arg,1);
	}
	else if(arg.value==OL_FIXEDCOLS) {
		XtSetArg(arg,XtNlayoutType,OL_FIXEDROWS);
		XtSetValues(parent,&arg,1);
	}
	XtMapWidget(parent);
}

void exclusivesCB3(widget,clientData,callData)
	Widget widget;
	caddr_t clientData, callData;
{
	exit(0);
}

static Arg exclusives2ARGS[] = {
						/* widget resources */

	{ XtNlayoutType,(XtArgVal) OL_FIXEDCOLS },
	{ XtNmeasure,(XtArgVal) 10 },
	{ XtNnoneSet,(XtArgVal) TRUE },

						/* form resources */
	{ XtNxRefWidget, NULL },
	{ XtNyRefWidget, NULL },
	{ XtNxOffset,(XtArgVal) 20 },
	{ XtNyOffset,(XtArgVal) 50 },
	{ XtNxAddWidth,(XtArgVal) TRUE },
	{ XtNyAddHeight,(XtArgVal) TRUE },
};

void exclusives2CB(widget,clientData,callData)
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;
	int n;
	Widget parent,b;

	printf("EXCLUSIVES callback for adding a button\n");
	parent= (Widget) clientData;

	mbutton=XtCreateManagedWidget
			("MORE",rectButtonWidgetClass,parent,NULL,0);

	XtAddCallback(mbutton,XtNselect,exclusives2CB,(caddr_t)exclusives2);

	XtSetArg(arg,XtNxRefWidget, (XtArgVal) parent);
	XtSetValues(scrollbar,&arg,1);
}

/* OBLONGBUTTON RESOURCES AND CALLBACKS */

static Arg oblbuttonARGS[] = {
						/* widget resources */
	{ XtNlabel, (XtArgVal) "OBLONG BUTTON" },

						/* form resources */
	{ XtNxRefWidget, NULL },
	{ XtNyRefWidget, NULL },
	{ XtNxOffset,(XtArgVal) 10 },
	{ XtNyOffset,(XtArgVal) 10 },
	{ XtNxAddWidth,(XtArgVal) TRUE },
	{ XtNyAddHeight,(XtArgVal) TRUE },
	{ XtNxVaryOffset,(XtArgVal) TRUE },
};

void oblbuttonCB(widget,clientData,callData)
	Widget widget;
	caddr_t clientData, callData;
{
	Arg arg;

	printf("OBLONGBUTTON callback\n");
	
	oblbuttonARGS[2].value = (XtArgVal) widget; 
	oblbutton= XtCreateManagedWidget("obutton",oblongButtonWidgetClass,form,
			oblbuttonARGS,XtNumber(oblbuttonARGS));
	XtAddCallback(oblbutton,XtNselect,oblbuttonCB,NULL);

	XtSetArg(arg,XtNyRefWidget, (XtArgVal) oblbutton);
	XtSetValues(caption,&arg,1);
}

/* SCROLLBAR RESOURCES AND CALLBACKS */

static Arg scrollbarARGS[] = {

						/* widget resources */
	{ XtNwidth,(XtArgVal) 200 },
	{ XtNheight,(XtArgVal) 200 },
	{ XtNbackground,(XtArgVal) 4 },
	{ XtNforeground,(XtArgVal) 8 },
	{ XtNproportionLength,(XtArgVal) 35 },

						/* form resources */
	{ XtNxRefWidget, NULL },
	{ XtNyRefWidget, NULL },
	{ XtNxOffset,(XtArgVal) 10 },
	{ XtNxAddWidth,(XtArgVal) TRUE },
	{ XtNyOffset,(XtArgVal) 15 },
	{ XtNyResizable,(XtArgVal) TRUE },
	{ XtNyAttachBottom,(XtArgVal)TRUE },
	{ XtNxAttachRight,(XtArgVal) TRUE },
	{ XtNxVaryOffset,(XtArgVal) TRUE },
};

void scrollbarCB1(w, closure, call_data)
    Widget w;
    caddr_t closure;
    OlScrollbarVerify *call_data;
{
    call_data->ok = TRUE;
    printf( "Dragged to %d%%\n", call_data->new_location );
}

void scrollbarCB2(w, closure, call_data)
    Widget w;
    caddr_t closure;
    OlScrollbarVerify *call_data;
{
    call_data->ok = TRUE;
    printf( "Paged to %d%%\n", call_data->new_location );
}

/* CONTROLAREA RESOURCES AND CALLBACKS */

static Arg controlareaARGS[] = {
						/* widget resources */
	{ XtNbackground,(XtArgVal) 14 },
	{ XtNforeground,(XtArgVal) 3 },
						/* form resources */
	{ XtNxRefWidget, NULL },
	{ XtNyRefWidget, NULL },
	{ XtNxOffset,(XtArgVal) 10 },
	{ XtNyOffset,(XtArgVal) 10 },
	{ XtNxAddWidth,(XtArgVal) TRUE },
	{ XtNyAddHeight,(XtArgVal) TRUE },
	{ XtNxVaryOffset,(XtArgVal) TRUE },
	{ XtNyVaryOffset,(XtArgVal) TRUE },
};

void controlareaCB1(widget,clientData,callData)
	Widget widget;
	caddr_t clientData, callData;
{
	int height;
	Arg arg;

	printf("CONTROLAREA callback for CONTROL button \n");
	
}

void controlareaCB2(widget,clientData,callData)
	Widget widget;
	caddr_t clientData, callData;
{
	printf("CONTROLAREA callback for AREA button \n");
}

void controlareaEXIT(widget,clientData,callData)
	Widget widget;
	caddr_t clientData, callData;
{
	printf("ADIOS\n");
	exit(0);
}

/* CAPTION RESOURCES AND CALLBACKS */

static Arg captionARGS[] = {
						/* widget resources */

	{ XtNlabel, (XtArgVal) "CAPTION WIDGET" },
	{ XtNposition, OL_TOP },
	{ XtNalignment, OL_CENTER },
	{ XtNspace, 5 },
						/* form resources */
	{ XtNxRefWidget, NULL },
	{ XtNyRefWidget, NULL },
	{ XtNxOffset,(XtArgVal) 10 },
	{ XtNyOffset,(XtArgVal) 10 },
	{ XtNxAddWidth,(XtArgVal) TRUE },
	{ XtNyAddHeight,(XtArgVal) TRUE },
	{ XtNxVaryOffset,(XtArgVal) TRUE },
	{ XtNyVaryOffset,(XtArgVal) TRUE },
};

void captionCB(widget,clientData,callData)
	Widget widget;
	caddr_t clientData, callData;
{
	printf("CAPTION callback for button \n");
}

static void
Quit(w,client_data,call_data)
Widget w;
caddr_t client_data;
caddr_t call_data;
{
	exit(0);
}

/* P R O G R A M M E R   I N T E R F A C E */

/* WIDGET TREE: 

	TOPLEVEL 
	FORM
	OL_WIDGETS, ALL CHILDREN OF FORM
*/

int main(argc,argv)
	int argc;
	char **argv;
{

Arg arg[30];
int i,j;

/*** initialize the environment ***/

toplevel= OlInitialize("OL_DEMO","openlook demo",NULL,0,&argc, argv);

/*** make the form ***/

i=0;						/* widget resources */
XtSetArg(arg[i],XtNbackground,(XtArgVal) 13);i++;

form= XtCreateManagedWidget("form",formWidgetClass,toplevel,arg,i);

/*** make a title out of a rectangular button ***/

titleARGS[3].value = (XtArgVal) form; 
titleARGS[4].value = (XtArgVal) form; 

title= XtCreateManagedWidget("title",buttonWidgetClass,form,
			titleARGS,XtNumber(titleARGS));

/*** make the controlarea below the label at the left of the form */

controlareaARGS[2].value = (XtArgVal) form; 
controlareaARGS[3].value = (XtArgVal) title; 

controlarea= XtCreateManagedWidget("ca",controlAreaWidgetClass,form,
			controlareaARGS,XtNumber(controlareaARGS));

XtSetArg(arg[0],XtNlabel, "CONTROL" );
cabutton1= 
	XtCreateManagedWidget("cab2",oblongButtonWidgetClass,controlarea,arg,1);
XtAddCallback(cabutton1,XtNselect,controlareaCB1,NULL);

XtSetArg(arg[0],XtNlabel, "AREA" );
cabutton2= 
	XtCreateManagedWidget("cab2",oblongButtonWidgetClass,controlarea,arg,1);
XtAddCallback(cabutton2,XtNselect,controlareaCB2,NULL);

XtSetArg(arg[0],XtNlabel, "EXIT" );
cabutton3= 
	XtCreateManagedWidget("cab3",oblongButtonWidgetClass,controlarea,arg,1);
XtAddCallback(cabutton3,XtNselect,controlareaEXIT,NULL);

/*** make an exclusives settings with three buttons and
     place it below the controlarea and to the left of the form ***/

exclusivesARGS[2].value = (XtArgVal) form; 
exclusivesARGS[3].value = (XtArgVal) controlarea;

exclusives= XtCreateManagedWidget("eb",exclusivesWidgetClass,form,
			exclusivesARGS,XtNumber(exclusivesARGS));

n1=1;
XtSetArg(arg[0],XtNlabel,(XtArgVal) "EXCLUSIVES");
ebutton1= XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(ebutton1,XtNselect,exclusivesCB2,(caddr_t) exclusives);

n2=2;
XtSetArg(arg[0],XtNlabel,(XtArgVal) "two");
ebutton2= XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(ebutton2,XtNselect,exclusivesCB2,(caddr_t) exclusives);

n3=3;
XtSetArg(arg[0],XtNlabel,(XtArgVal) "bye");
ebutton3= XtCreateManagedWidget("rb",rectButtonWidgetClass,exclusives,arg,1);
XtAddCallback(ebutton3,XtNselect,exclusivesCB3,NULL);

/*** make an oblong button and place it at the left of the form
						under the exclusives ***/

oblbuttonARGS[1].value = (XtArgVal) form; 
oblbuttonARGS[2].value = (XtArgVal) exclusives;

oblbutton= XtCreateManagedWidget("obutton",oblongButtonWidgetClass,form,
			oblbuttonARGS,XtNumber(oblbuttonARGS));
XtAddCallback(oblbutton,XtNselect,oblbuttonCB,NULL);

/*** make another exclusives settings with one button and place it below
     the oblong and to the right of the control area ***/

exclusives2ARGS[3].value = (XtArgVal) controlarea; 
exclusives2ARGS[4].value = (XtArgVal) oblbutton; 

exclusives2= XtCreateManagedWidget("eb",exclusivesWidgetClass,form,
			exclusives2ARGS,XtNumber(exclusives2ARGS));

mbutton=XtCreateManagedWidget("MORE",rectButtonWidgetClass,exclusives2,NULL,0);
XtAddCallback(mbutton,XtNselect,exclusives2CB,(caddr_t)exclusives2);

/*** make the caption below the second exclusives at the left of the form ***/

captionARGS[4].value = (XtArgVal) form; 
captionARGS[5].value = (XtArgVal) exclusives2; 

caption= XtCreateManagedWidget("caption",captionWidgetClass,form,
			captionARGS,XtNumber(captionARGS));

XtSetArg(arg[0],XtNlabel,(XtArgVal) "CAPTION BUTTON");
capbutton = 
	XtCreateManagedWidget("cbutton",oblongButtonWidgetClass,caption,arg,1);

XtAddCallback(capbutton,XtNselect,captionCB,NULL);

/*** make the scrollbar to the right of the second exclusives and
     attach it to the right and bottom of the form ***/

scrollbarARGS[5].value = (XtArgVal) exclusives2; 
scrollbarARGS[6].value = (XtArgVal) title; 

scrollbar= XtCreateManagedWidget("scrollbar",scrollbarWidgetClass,form,
			scrollbarARGS,XtNumber(scrollbarARGS));

XtAddCallback(scrollbar,XtNsliderMoved,scrollbarCB1,NULL);
XtAddCallback(scrollbar,XtNsliderMoved,scrollbarCB2,NULL);

XtRealizeWidget(toplevel); 

XtMainLoop();
}

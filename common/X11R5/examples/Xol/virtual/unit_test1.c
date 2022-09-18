/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:virtual/unit_test1.c	1.14"
#endif

/*
 *************************************************************************
 *
 * Description:
 *		This file does a unit test on the virtual button
 *	code.  Three areas are tested:
 *
 *		1. Initialization of the virtual button list
 *		2. Parsing of a string with the virtual button list
 *		3. Checking a virtual button event against the following
 *		   XEvents: XButtonEvent, XMotionEvent and XCrossingEvent
 *
 ******************************file*header********************************
 */

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/OblongButt.h>

/*************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *
 ************************************************************************/

					/* private procedures		*/

static void EventTest();		/* Tests virtual button event	*/
static void ReadDataBase();		/* reads info from the database	*/
static void Handler();			/* Checks event status		*/
static void InitializeTest();		/* virtual button intialization	*/
static void MapHandler();		/* sets input focus		*/
static void ParseTest();		/* Parses a virtual translation	*/

/*************************************************************************
 *
 * Define global variables and #defines
 *
 ************************************************************************/

static Widget  toplevel;
static char    name_array[50];
static char   *vbk_test_name = name_array;
static int     loop_counter;

/*
 *************************************************************************
 * main - this is the main driver program to test this stuff
 ****************************procedure*header*****************************
 */
int main(argc, argv)
	int   argc;
	char *argv[];
{
	int           test;
	char          response[30];
	Boolean       done = False;
	extern Widget toplevel;

	toplevel = OlInitialize(argv[0], "virtual_test", NULL, NULL,
				&argc, argv);

	do {
		printf("Which Test do you want to perform ???\n");
		printf("\t1. Initialize the virtual button list\n");
		printf("\t2. Parse a virtual translation string\n");
		printf("\t3. Test the virtual event routine ??\n");
		printf("\tq. Quit\n");
		printf("Enter test number ==> ");
		fflush(stdout);
		scanf("%s", response);

		if (*response == 'q' || *response == 'Q' |
		    *response == 'e' || *response == 'E')
			test = 4;
		else {
			if (sscanf(response, "%d", &test) != 1) {
				fflush(stdin);
				test = -1;
			}
		}

		switch(test) {
		case 1:
			InitializeTest();
			break;
		case 2:
			ParseTest();
			break;
		case 3:
			EventTest();
			break;
		case 4:
			done = True;
			printf("Terminating virtual unit test....\n");
			break;
		default:
			printf("Invalid input, try again .....\n");
		}

	} while (done == False);

} /* END OF main() */

/*
 *************************************************************************
 * EventTest - this routine tests to see if an XEvent matches a virtual
 * button 
 ****************************procedure*header*****************************
 */
static void
EventTest()
{
	static Widget	widget = (Widget)NULL;
	int		count;
	XEvent		xevent;
	extern Widget	toplevel;
	extern char *	vbk_test_name;
	extern int	loop_counter;
	Mask		mask;
	Arg		args[2];
	_OlVirtualMapping mappings[OL_MAX_VIRTUAL_MAPPINGS];

	printf("Enter the virtual token name to test against ==> ");
	fflush(stdout);
	scanf("%s",  vbk_test_name);

	if (_OlGetVirtualMappings(vbk_test_name, mappings,
					OL_MAX_VIRTUAL_MAPPINGS) == 0)
	{
		printf("Virtual token \"%s\" does not exist\
 or it has no mappings !!\n", vbk_test_name);
		return;
	}

	if (mappings[0].type == OL_VIRTUAL_KEY)
	{
		printf("Testing virtual key \"%s\"\n", vbk_test_name);

		mask = (KeyPressMask|KeyReleaseMask);
		printf("This routine tests KeyPress and KeyRelease Events\n");

		XtSetArg(args[0], XtNlabel, " Test Virtual Key Here  ");
	}
	else
	{
		char r[30];

		printf("Testing virtual button \"%s\"\n", vbk_test_name);

		mask = (ButtonPressMask|ButtonReleaseMask|
			EnterWindowMask|LeaveWindowMask);
		printf("Testing XEvents: ButtonPress, ButtonRelease,\n");
		printf("                 EnterNotify and LeaveNotify\n");
		printf("Do you want to include a ButtonMotion mask ?? ==> ");
		fflush(stdout);
		scanf("%s", r);
	 	if (*r == 'y' || *r == 'Y')
			mask |= ButtonMotionMask;

		XtSetArg(args[0], XtNlabel, "Test Virtual Button Here");
	}

	printf("Test how many XEvents before terminating ==> ");
	fflush(stdout);
	if (!scanf("%d", &count))
		count = -1;

	if (count < 0)
	{
		count = 10;
		printf("*** Defaulting to %d iterations\n", count);
	}

	if (mask & ButtonPressMask)
	{
		printf("\nMove mouse over the test widget and try different\n");
		printf("combinations of modifiers and mouse buttons.\n");
	}
	else
	{
		printf("\nAfter test widget maps, move the pointer over the\n");
		printf("test widget and type characters from the keyboard\n");
	}

	if (widget == (Widget)NULL)
	{
		widget = XtCreateManagedWidget("test",
			oblongButtonWidgetClass, toplevel, args, 1);
		XtRealizeWidget(toplevel);
	}
	else
	{
		XtSetValues(widget, args, 1);
		XtMapWidget(toplevel);
	}

	XtAddEventHandler(widget, mask, NULL, Handler, NULL);

			/* If we're testing virtual keys, set the input
			 * focus on the test widget			*/

	XSync(XtDisplay(toplevel), 0);
	if (mask & KeyPressMask)
	{
		XtAddEventHandler(widget, MapNotify, NULL, MapHandler, NULL);
	}

	for (loop_counter = 0; loop_counter < count; )
	{
		XtNextEvent(&xevent);
		XtDispatchEvent(&xevent);
	}

	if (mask & KeyPressMask)
	{
		XtAddEventHandler(widget, MapNotify, NULL, MapHandler, NULL);
	}
	XtRemoveEventHandler(widget, mask, NULL, Handler, NULL);

	putchar('\n');

	XtUnmapWidget(toplevel);
	XFlush(XtDisplay(toplevel));

					/* Purge the XEvent Queue	*/

	while(XPending(XtDisplay(toplevel)))
	{
		XNextEvent(XtDisplay(toplevel), &xevent);
	}
} /* END OF EventTest() */

/*
 *************************************************************************
 * ReadDataBase - this routine read information from the database.
 ****************************procedure*header*****************************
 */
static void
ReadDataBase(name, class, base)
	String		name;
	String		class;
	XtPointer	base;
{
	XtResource	rsc[1];

	rsc[0].resource_name	= name;
	rsc[0].resource_class	= class;
	rsc[0].default_addr	= "";
	rsc[0].resource_type	= XtRString;
	rsc[0].resource_size	= sizeof(String);
	rsc[0].default_type	= XtRString;
	rsc[0].resource_offset	= (Cardinal)0;

	printf("Reading \"%s\" from the database", rsc[0].resource_name);

				/* Read resources from the
				 * application's database	*/

	XtGetApplicationResources(toplevel, base,
			rsc, (Cardinal)1, (ArgList)NULL, (Cardinal)0);

} /* END OF ReadDataBase() */

/*
 *************************************************************************
 * Handler - this routine checks the event that test widget is receiving
 ****************************procedure*header*****************************
 */
static void
Handler(w, client_data, xevent)
	Widget   w;				/* Test Widget id	*/
	caddr_t  client_data;			/* unused		*/
	XEvent  *xevent;			/* virtual XEvent	*/
{
	char *		s;
	Boolean		exc;
	Boolean		non_exc;
	extern int	loop_counter;
	extern char *	vbk_test_name;

	if (xevent->xany.type == ButtonPress)
		s = "ButtonPress";
	else if (xevent->xany.type == ButtonRelease)
		s = "ButtonRelease";
	else if (xevent->xany.type == KeyPress)
		s = "KeyPress";
	else if (xevent->xany.type == KeyRelease)
		s = "KeyRelease";
	else if (xevent->xany.type == MotionNotify)
		s = "ButtonMotion";
	else if (xevent->xany.type == EnterNotify)
		s = "EnterNotify";
	else if (xevent->xany.type == LeaveNotify)
		s = "LeaveNotify";
	else
		return;

	++loop_counter;

	exc	= _OlIsVirtualEvent(vbk_test_name, xevent, True);
	non_exc	= _OlIsVirtualEvent(vbk_test_name, xevent, False);

	printf("  %2d: %14s: exclusive = %s, nonexclusive = %s\n", 
		loop_counter, s, (exc ? "TRUE": "FALSE"),
		(non_exc ? "TRUE" : "FALSE"));
} /* END OF Handler() */

/*
 *************************************************************************
 * InitializeTest - this routine initializes the virtual button list.
 * It allows the user to add virtual buttons beyond the core virtual 
 * buttons.
 ****************************procedure*header*****************************
 */
static void
InitializeTest()
{
	char *	ptr;
	char	buf[1024];

	printf("Add a new virtual button/key ?? ==> ");
	fflush(stdout);
	scanf("%s", buf);

	if (*buf == 'Y' || *buf == 'y')
	{
	    printf("Read from Terminal or database '*virtualMappings'?\
 (t or d) ==> ");
	    fflush(stdout);
	    scanf("%s", buf);

	    if (!(*buf == 'D' || *buf == 'd'))
	    {
		printf("Enter a virtual mapping string.\n");
		printf("Format:  Name1: [modifiers] <Button#>, \
[modifiers] <Button#>, \n");
		printf("         Name2: [modifiers] <your_key>, \
[modifiers] <another_key>\n");
		printf("Terminate the string with the tilde symbol, '.'\n==> ");
		fflush(stdout);
		scanf(" \n %[^.]s", buf);
		getchar();			/* absord '.'		*/
		getchar();			/* absord '\n'		*/

		ptr = buf;
	    }
	    else
	    {
		ReadDataBase("virtualMappings", "VirtualMappings", &ptr);
	    }

		_OlAppAddVirtualMappings(toplevel, ptr);
	}

	printf("See long/short form of virtual buttons/keys or\
 return (l or s or r) ==> ");
	fflush(stdout);
	scanf("%s", buf);

	if (*buf == 'L' || *buf == 'l')
		_OlDumpVirtualMappings(stdout, True);
	else if (*buf == 'S' || *buf == 's')
		_OlDumpVirtualMappings(stdout, False);
	  
} /* END OF InitializeTest() */

/*
 *************************************************************************
 * MapHandler - this routine set the input focus on the widget
 ****************************procedure*header*****************************
 */
static void
MapHandler(w, client_data, xevent)
	Widget   	w;			/* Test Widget id	*/
	caddr_t  	client_data;		/* unused		*/
	XEvent *	xevent;			/* virtual XEvent	*/
{
	XSetInputFocus(XtDisplay(w), XtWindow(w), RevertToNone, CurrentTime);
	XtRemoveEventHandler(w, MapNotify, NULL, MapHandler, NULL);
} /* END OF MapHandler() */

/*
 *************************************************************************
 * ParseTest - this routine parses a virtual translation string.  The user
 * can read the string in from the .Xdefaults file or from the terminal.
 ****************************procedure*header*****************************
 */
static void
ParseTest()
{
	char *	new;
	char	buf[1024];
	char *	ptr = buf;
	char *	name = "virtualTranslation";

	printf("Read from Terminal or database '*%s'? (t or d) ==> ", name);
	fflush(stdout);
	scanf("%s", ptr);

	if (!(*ptr == 'D' || *ptr == 'd'))
	{
		printf("Enter a translation (Terminate with '.')\n==> ");
		fflush(stdout);
		scanf(" \n %[^.]s", ptr);
		getchar();			/* Absorb '.'	*/
		getchar();			/* Absorb '\n'	*/
		putchar('\n');
	}
	else
	{
		ReadDataBase(name, "VirtualTranslation", &ptr);

		if (!ptr)
		{
			printf("Couldn't find \"%s\" in .Xdefaults\n",name);
			return;
		}
	}

	printf("\nOriginal Translation:\n\"%s\"\n", ptr);

	new = OlConvertVirtualTranslation(ptr);

	if (new)
		printf("\nParsed translation:\n\"%s\"\n", new);
	else
		printf("\nNo logical translation !!!!\n");

	if (new != ptr)
		XtFree(new);

} /* END OF ParseTest() */

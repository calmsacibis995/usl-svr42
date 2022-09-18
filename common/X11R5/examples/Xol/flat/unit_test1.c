/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:flat/unit_test1.c	1.13"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This program test the flattened exclusives widget.
 *
 ******************************file*header********************************
 */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/BaseWindow.h>
#include <Xol/OblongButt.h>
#include <Xol/FExclusive.h>
#include <Xol/FNonexclus.h>
#include <Xol/FCheckBox.h>
#include <Xol/Menu.h>

#include <Xol/bitmaps/a12pinout>
/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *
 **************************forward*declarations***************************
 */

static void	Callback();		/* Callback procedure		*/
static void	FlatCreate();		/* Creates flat widgets		*/
static char *	GetContainerType();	/* Returns string for container	*/
static void	ModifyExclusives();	/* changes set item		*/

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define	IMAGE	1
#define PIXMAP	2

static char *		new_label = (char *)NULL;
static Widget		flat_exc_widget = (Widget)NULL;
static Widget		menu_flat_exc_widget = (Widget)NULL;
static Pixel		default_fg = 0;
static Pixel		default_bg = 1;
static Pixel		gbl_bg;

static String exc_fields[] = {
	XtNlabel, XtNbackground, XtNselectProc, XtNunselectProc,
	XtNsensitive, XtNfontColor, XtNset
};
typedef struct {
	XtArgVal	label;		/* label string pointer	*/
	XtArgVal	background;	/* the item's background*/
	XtArgVal	select;		/* Select callback proc.*/
	XtArgVal	unselect;	/* Unselect callback proc.*/
	XtArgVal	sensitive;	/* Is this item sensitive ?*/
	XtArgVal	font_color;	/* color of the font	*/
	XtArgVal	set;
} FlatExclusives;

/*
 *************************************************************************
 * Callback - this is a callback procedure called from the sub-objects
 * in this unit test program.  The callback prints out the data supplied
 * with the OlFlatCallData structure.  The routine also queries the
 * sub-object to see if this is a select callback or an unselect callback.
 ****************************procedure*header*****************************
 */
static void
Callback(widget, client_data, call_data)
	Widget	widget;
	caddr_t	client_data;
	caddr_t	call_data;
{
	OlFlatCallData *	fd = (OlFlatCallData *)call_data;
	Cardinal		i;
	Arg			args[2];
	String			label = (String)NULL;
	Boolean			set = (Boolean)OL_IGNORE;
	static Boolean		check_set = True;

	printf("\nCallback called with OlFlatCallData:\n");
	printf("\tItem index     = %u\n", (unsigned)fd->item_index);
	printf("\tNum Items      = %u\n", (unsigned)fd->num_items);
	printf("\tItem Fields (%u) are:\n\t\t", (unsigned)fd->num_item_fields);
	for(i=0; i < fd->num_item_fields; ++i)
	{
		printf("%sXtN%s", (i == 0 ? " " : ", "), fd->item_fields[i]);
	}
	putchar('\n');

	XtSetArg(args[0], XtNset, &set);
	set = (Boolean)OL_IGNORE;
	OlFlatGetValues(widget, fd->item_index, args, 1);

	printf("XtGetValues shows that is a \"%s\" callback\n",
		(set == True ? "Select" : set == False ? "Unselect" :
					"GARBAGE"));

						/* Now, query the label	*/

	XtSetArg(args[0], XtNlabel, &label);

	OlFlatGetValues(widget, fd->item_index, args, 1);
	printf("XtGetValues shows label = \"%.30s\"\n", label);

				/*  Modify the label of this item	*/

	if (new_label != (char *) NULL)
	{
		XtSetArg(args[0], XtNlabel, new_label);
		OlFlatSetValues(widget, fd->item_index, args, 1);
		printf("Using XtSetValues to change label to \"%s\"\n\n",
			new_label);
	}

			/* If this widget is a flat non-exclusives and
			 * the index is 0, change the set item in the
			 * exclusives group.				*/

	if (XtClass(widget) == flatCheckBoxWidgetClass &&
	    fd->item_index ==  (Cardinal) 0)
	{
		check_set = set;
	}
	else if (XtClass(widget) == flatNonexclusivesWidgetClass &&
	    fd->item_index == (Cardinal) 0)
	{
		ModifyExclusives(flat_exc_widget, check_set, True);
		ModifyExclusives(menu_flat_exc_widget, check_set, False);
	}
} /* END OF Callback() */

/*
 *************************************************************************
 * FlatCreate - this routine creates flat widgets
 ****************************procedure*header*****************************
 */
static void
FlatCreate(num_subobjects, use_different_colors, label, name, class)
	int		num_subobjects;
	Boolean		use_different_colors;
	int		label;
	char *		name;
	WidgetClass	class;
{
	static XImage *	image = (XImage *)NULL;
	static Pixmap	pixmap = (Pixmap)None;
	char		buf[512];
	Widget		shell;
	int		a;
	Arg		args[10];
	static char *	colors[] = {"White","Blue","red","yellow", "orange"};
	static char *	font_clrs[] = {"Black","yellow","white","blue","black"};
	int		color_index;
	XrmValue	fromVal;
	XrmValue	toVal;
	FlatExclusives *items;
	Boolean		sensitive;
	Widget		flat;
	extern void	exit ();

	if (label == IMAGE)
	{
		if (image == (XImage *)NULL)
		{
			extern XImage * _OlGetImage();

			image = _OlGetImage(OlDefaultScreen, 12, "pinout");
		}
	}
	else if (label == PIXMAP)
	{
		if (pixmap == (Pixmap)None)
		{
			pixmap = XCreatePixmapFromBitmapData(OlDefaultDisplay,
					RootWindowOfScreen(OlDefaultScreen),
					a12pinout_bits, a12pinout_width,
					a12pinout_height, 0, 1,
					DefaultDepthOfScreen(OlDefaultScreen));
		}
	}

					/* Create the base window	*/

	(void)sprintf(buf, "%s Shell", name);

	name = XtNewString(buf);

	XtSetArg(args[0], XtNtitle, buf);

	shell = XtCreateApplicationShell(args[0].name,
			baseWindowShellWidgetClass, args, 1);

				/* Allocate some memory and fill it in	*/

	items = (FlatExclusives *)XtMalloc((Cardinal)
				(num_subobjects*sizeof(FlatExclusives)));

	fromVal.size = sizeof(String);
	for (a=0; a < num_subobjects; ++a)
	{
		if (use_different_colors == True) {
			color_index	= a % XtNumber(colors);
			fromVal.addr	= colors[color_index];

			XtConvert(shell, XtRString, &fromVal, XtRPixel, &toVal);
			items[a].background = (XtArgVal)
						*((Pixel *)toVal.addr);

			if (class == flatCheckBoxWidgetClass)
			{
				items[a].font_color =
					BlackPixelOfScreen(XtScreen(shell));
			}
			else
			{
				fromVal.addr = font_clrs[color_index];
				XtConvert(shell, XtRString, &fromVal,
						XtRPixel, &toVal);
				items[a].font_color = (XtArgVal)
						*((Pixel *)toVal.addr);
			}
		}
		else {
			items[a].font_color = default_fg;
			items[a].background = gbl_bg;
		}

		if (a != 0)
		{
			(void)sprintf(buf, "Choice %d",a);
			items[a].label = (XtArgVal)XtNewString(buf);
			items[a].select = (XtArgVal)Callback;
			items[a].unselect = (XtArgVal)Callback;
		}
		else if (class == flatNonexclusivesWidgetClass)
		{
			(void)sprintf(buf, "Change Exc",a);
			items[a].label = (XtArgVal)XtNewString(buf);
			items[a].select = (XtArgVal)Callback;
			items[a].unselect = (XtArgVal)Callback;
		}
		else if (class == flatCheckBoxWidgetClass)
		{
			(void)sprintf(buf, "Use Exc's list",a);
			items[a].label = (XtArgVal)XtNewString(buf);
			items[a].select = (XtArgVal)Callback;
			items[a].unselect = (XtArgVal)Callback;
		}
		else
		{
			items[a].label = (XtArgVal)"Quit";
			items[a].select = (XtArgVal)exit;
		}

		sensitive = True;

		switch(a % 10) {
		case 2:
			if (label)
			{
				sensitive = False;
				items[a].label = (XtArgVal)NULL;
			}
			break;
		case 3:
			if (label)
				items[a].label = (XtArgVal)NULL;
			break;
		case 7:
		case 9:
			sensitive = False;
			break;
		default:
			break;
		}
		items[a].sensitive = sensitive;
		items[a].set = (class == flatCheckBoxWidgetClass &&
				a == (Cardinal)0 ? True : False);
	
	} /* End of looping over all elements */

	a = 0;
	XtSetArg(args[a], XtNitems,	items);				++a;
	XtSetArg(args[a], XtNnumItems,	num_subobjects);		++a;
	XtSetArg(args[a], XtNitemFields,exc_fields);			++a;
	XtSetArg(args[a], XtNnumItemFields,XtNumber(exc_fields));	++a;
	XtSetArg(args[a], XtNbackground, gbl_bg);			++a;
	if (label == IMAGE)
	{
		XtSetArg(args[a], XtNlabelImage, image);		++a;
	}
	else if (label == PIXMAP)
	{
		XtSetArg(args[a], XtNbackgroundPixmap, pixmap);		++a;
	}

	if (a > XtNumber(args))
		OlError("FlatCreate: local Arg stack exceeded");

	flat = XtCreateManagedWidget(name, class, shell, args, a);

	if (class == flatExclusivesWidgetClass)
	{
		flat_exc_widget = flat;
	}

	XtRealizeWidget(shell);

				/* If the flat widget is not checkboxes,
				 * add menu to the flat widget		*/

return;
	if (class != flatCheckBoxWidgetClass)
	{
		Widget	pane;
		Arg	menu_args[1];

		char *	name = (class == flatExclusivesWidgetClass ?
				"flat exclusives menu" :
				"flat non-exclusives menu");

		shell = XtCreatePopupShell(name, menuShellWidgetClass,
				flat, (ArgList)NULL, (Cardinal)0);

		XtSetArg(menu_args[0], XtNmenuPane, &pane);
		XtGetValues(shell, menu_args, 1);

		flat = XtCreateManagedWidget(name, class, pane, args, a);
		if (class == flatExclusivesWidgetClass)
		{
			menu_flat_exc_widget = flat;
		}
	}
} /* END OF FlatCreate() */

/*
 *************************************************************************
 * GetContainerType - this function returns the string representation
 * for a container if given the container's id type.
 ****************************procedure*header*****************************
 */
static char *
GetContainerType(type)
	OlDefine type;
{
	char *	name;

#define CASE(t)	case t: name="t";break

	switch((int)type)
	{
	CASE(OL_FLAT_EXCLUSIVES);
	CASE(OL_FLAT_NONEXCLUSIVES);
	CASE(OL_FLAT_BUTTON);
	CASE(OL_FLAT_CHECKBOX);
	default:
		name = "bad container type!!!!!";
		break;
	}
	return(name);
} /* END OF GetContainerType() */

/*
 *************************************************************************
 * ModifyExclusives - this routine modifies the set item in a flat
 * exclusives widget.
 ****************************procedure*header*****************************
 */
static void
ModifyExclusives(w, use_app_list, echo_msg)
	Widget	w;
	Boolean	use_app_list;
	Boolean	echo_msg;
{
	FlatExclusives *	fexc_items = (FlatExclusives *)NULL;
	Arg			args[2];
	Cardinal		num_items;

	if (w == (Widget) NULL)
	{
		return;
	}
	XtSetArg(args[0], XtNitems, &fexc_items);
	XtSetArg(args[1], XtNnumItems, &num_items);
	XtGetValues(w, args, 2);

	if (fexc_items == (FlatExclusives *)NULL)
	{
		OlWarning("Couldn't find flat exclusive item list");
		return;
	}

	if (echo_msg)
		printf("Making the second flat exclusive setting to be\
 the current setting\n");

	if (use_app_list == False)
	{
		if (echo_msg == True)
			printf("by using XtNitemState on sub-object\n");

				/* modify the sub-object with the
				 * convenience routine.   The sub-object's
				 * index is the second parameter.	*/

		XtSetArg(args[0], XtNset, True);
		OlFlatSetValues(w, 1, args, 1);
	}
	else
	{
		Cardinal i;

		if (echo_msg == True)
			printf("by touching the application list directly\n");

				/* Modify the application's list	*/

		for (i=0; i < num_items; ++i)
		{
			fexc_items[i].set = (i == 1 ? True : False);
		}

				/* Inform the container that the list
				 * was modified				*/

		XtSetArg(args[0], XtNitemsTouched, True);
		XtSetValues(w, args, 1);
	}
} /* END OF Modify Exclusives */

/*
 *************************************************************************
 * main - program driver function
 ****************************procedure*header*****************************
 */
main(argc, argv) 
	int argc;
	char ** argv;
{
	Widget		shell;
	int		i;
	int		label = 0;
	int		num_subobjects = 4;
	Boolean		use_different_colors = False;
	extern char *	new_label;
	char *		line =
		"**********************************************************\n";

	printf("\n%s",line);
	printf("* Type 'e' immediately followed by a number (e.g., e30) to\n");
	printf("  indicate the number of exclusive settings that you want\n\n");
	printf("* Type 'l' immediately followed by a string (e.g.,\n");
	printf("  lsetting) to indicate the new label for exclusive\n");
	printf("  settings after the callback is called\n\n");
	printf("* Type the word 'colors' if you want different colored\n");
	printf("  settings\n\n");
	printf("* Type the work 'image' or 'pixmap' if you to test images\n");
	printf("  or pixmaps, respectively\n\n");
	printf("* Type 'bg' and then a color name (space-separated) to set\n");
	printf("  the background color of the flat widgets\n\n");
	printf("* Use -xrm '*resourceName: value' to set other resources\n");
	printf("%s\n",line);

	shell = OlInitialize("shell", "Demo", NULL, 0, &argc, argv);

	for (gbl_bg = default_bg, i=1; i<argc; ++i) {
		if (*argv[i] == 'e') {
			num_subobjects = atoi(argv[i]+1);
		}
		else if (*argv[i] == 'l') {
			if (strlen(argv[i]) > 1) {
				new_label = XtNewString((argv[i]+1));
			}
		}
		else if (!strcmp(argv[i], "colors"))
		{
			use_different_colors = True;
		}
		else if (!strcmp(argv[i], "image"))
		{
			label = IMAGE;
		}
		else if (!strcmp(argv[i], "pixmap"))
		{
			label = PIXMAP;
		}
		else if (!strncmp(argv[i], "bg", 2))
		{
			XrmValue	fromVal;
			XrmValue	toVal;

			fromVal.size	= sizeof(String);
			fromVal.addr	= argv[++i];
			XtConvert(shell, XtRString, &fromVal, XtRPixel, &toVal);
			gbl_bg = *((Pixel *)toVal.addr);
		}
	}

	FlatCreate(num_subobjects, use_different_colors, label,
			"flat exclusives", flatExclusivesWidgetClass);

	FlatCreate(num_subobjects, use_different_colors, label,
			"flat non-exclusives", flatNonexclusivesWidgetClass);

	FlatCreate(num_subobjects, use_different_colors, label,
			"flat checkbox", flatCheckBoxWidgetClass);

	XtMainLoop();
} /* END OF main() */


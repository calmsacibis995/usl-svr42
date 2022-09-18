/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:list/unit_test1.c	1.44"
#endif
/*
 unit_test1.c (C source file)
	Acc: 596865457 Tue Nov 29 22:57:37 1988
	Mod: 596865457 Tue Nov 29 22:57:37 1988
	Sta: 596865457 Tue Nov 29 22:57:37 1988
	Owner: 4777
	Group: 1985
	Permissions: 666
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/*	this program builds a basic list but is not interested in user
 *	interaction (callbacks).  although a basic list, the list is built
 *	with a head.  this head is passed to a print routine which prints
 *	the contents of the list as the widget sees it.  note that the
 *	head must be descended into.
 */

#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <dirent.h>		/* must follow IntrinsicP.h */
#include <Xol/OpenLook.h>
#include <Xol/Form.h>
#include <Xol/FooterPane.h>
#include <Xol/Menu.h>
#include <Xol/OblongButt.h>
#include <Xol/ScrollingL.h>
#include <Xol/StaticText.h>
#include <Xol/TextField.h>

extern Boolean SLdebug;

static Arg		args[20];
static Cardinal		cnt;
static OlListToken	current_item	= 0;
static Widget		current_lw	= 0;
static Widget		footer;
static int		num_lists	= 1;
static Boolean		selectable	= True;
static Cardinal		viewHeight	= 0;

static OlListToken	(*addItem)();
static void		(*deleteItem)();
static void		(*editOff)();
static void		(*editOn)();
static void		(*touch)();
static void		(*update)();
static void		(*view)();

static WidgetList	CreateLists();
static void		CreateMenu();
static void		CutCB();
static void		DeleteCB();
static void		EditCB();
static char		GetMnemonic();
static void		MakeCurrentCB();
static void		PopupMenuCB();
static void		PrintCB();
static void		Report();
static void		ReportCurrent();
static void		Usage();
static void		VerifyEditCB();

#define OFFSET 3


main(argc, argv)
    int		argc;
    char *	argv[];
{
    extern char *	optarg;
    extern int		optind;
    int			opterr = 0;
    int			opt;

    OlListToken	token;
    Widget	toplevel, panel, base;
    WidgetList	list_widgets;

    toplevel = OlInitialize(NULL, "SList", NULL, 0, &argc, argv);

    while ( (opt = getopt(argc, argv, "l:uv:z?")) != -1)
	switch (opt) {
	case 'l':
	    num_lists = atoi(optarg);
	    if (num_lists <= 0) num_lists = 1;
	    break;
	case 'u':
	    selectable = FALSE;
	    break;
	case 'v':
	    viewHeight = atoi(optarg);
	    break;
	case 'z':
#ifdef DEBUG
	    SLdebug = True;
#else
	    fprintf(stderr, "Sorry, not compiled for debugging\n");
#endif
	    break;
	case '?':
	    opterr++;
	    break;
	}
    if (opterr)
	Usage(argv[0]);		/* report usage & exit */

    if (optind >= argc)
	Usage(argv[0]);		/* report usage & exit */

    /* SINGLE CHILD OF TOPLEVEL */
    cnt = 0;
    panel = XtCreateManagedWidget("panel", footerPanelWidgetClass,
				  toplevel, args, cnt);

    /* "TOP" CHILD OF PANEL */
    cnt = 0;
    base = XtCreateManagedWidget("base", formWidgetClass,
				 panel, args, cnt);

    list_widgets = CreateLists(base, optind, argc, argv);

    current_lw = list_widgets[0];	/* make the current_lw the 1st */

    CreateMenu(list_widgets);

    /* "FOOTER" CHILD OF PANEL */
    cnt = 0;
    XtSetArg(args[cnt], XtNalignment, OL_RIGHT); cnt++;	/* doesn't work */
    XtSetArg(args[cnt], XtNwrap, False); cnt++;
    footer = XtCreateManagedWidget("footer", staticTextWidgetClass,
				   panel, args, cnt);
    Report("use MENU to edit");

    XtRealizeWidget(toplevel);
    XtMainLoop();
}

static WidgetList
CreateLists(parent, optind, argc, argv)
    Widget	parent;
    int		optind;
    int		argc;
    char *	argv[];
{
#define Pos(row, col) ((cols * row) + col)
    int			col, row, rows, cols;
    DIR *		dir;
    struct dirent *	entry;
    OlListItem		item;
    OlListToken		token;
    static WidgetList	lists;
    static WidgetList	text_field;
    static OlListToken * tokens;

    lists	= (WidgetList)XtMalloc(num_lists * sizeof(Widget));
    text_field	= (WidgetList)XtMalloc(num_lists * sizeof(Widget));
    tokens	= (OlListToken *)XtMalloc(num_lists * sizeof(OlListToken));

    cols = (int)ceil(sqrt((double)num_lists));
    rows = (num_lists + cols - 1) / cols;

    for (row=0; row<rows; row++)
	for (col=0; col<cols; col++) {
	    if (Pos(row, col) >= num_lists)
		break;

	    cnt = 0;
	    XtSetArg(args[cnt], XtNselectable, selectable); cnt++;
	    XtSetArg(args[cnt], XtNtraversalOn, True); cnt++;
	    XtSetArg(args[cnt], XtNviewHeight, viewHeight); cnt++;

	    XtSetArg(args[cnt], XtNxOffset, OFFSET); cnt++;
	    XtSetArg(args[cnt], XtNxResizable, True); cnt++;
	    XtSetArg(args[cnt], XtNxVaryOffset, False); cnt++;
	    XtSetArg(args[cnt], XtNyOffset, OFFSET); cnt++;
	    XtSetArg(args[cnt], XtNyResizable, True); cnt++;
	    XtSetArg(args[cnt], XtNyVaryOffset, False); cnt++;

	    if (col > 0) {
		XtSetArg(args[cnt], XtNxRefWidget, lists[Pos(0, col-1)]); cnt++;
		XtSetArg(args[cnt], XtNxAddWidth, True); cnt++;
	    }
	    if (row > 0) {
		XtSetArg(args[cnt], XtNyRefWidget, lists[Pos(row-1, 0)]); cnt++;
		XtSetArg(args[cnt], XtNyAddHeight, TRUE); cnt++;
	    }
	    if (row == rows - 1) {
		XtSetArg(args[cnt], XtNyAttachBottom, TRUE); cnt++;
	    }
	    if (col == cols - 1) {
		XtSetArg(args[cnt], XtNxAttachRight, TRUE); cnt++;
	    }

	    lists[Pos(row, col)] =
		XtCreateManagedWidget("slist", scrollingListWidgetClass,
				      parent, args, cnt);

	    cnt = 0;
	    if (row == 0 && col == 0) {	/* do these only once */
		XtSetArg(args[cnt], XtNapplAddItem, &addItem); cnt++;
		XtSetArg(args[cnt], XtNapplDeleteItem, &deleteItem); cnt++;
		XtSetArg(args[cnt], XtNapplEditClose, &editOff); cnt++;
		XtSetArg(args[cnt], XtNapplEditOpen, &editOn); cnt++;
		XtSetArg(args[cnt], XtNapplTouchItem, &touch); cnt++;
		XtSetArg(args[cnt], XtNapplUpdateView, &update); cnt++;
		XtSetArg(args[cnt], XtNapplViewItem, &view); cnt++;
	    }
	    XtSetArg(args[cnt], XtNtextField, &(text_field[Pos(row, col)])); cnt++;
	    XtGetValues(lists[Pos(row, col)], args, cnt);

	    if ((dir = opendir(argv[optind])) == NULL) {
		fprintf(stderr, "Couldn't open %s as directory \n", argv[optind]);
		Usage(argv[0]);	/* report usage & exit */
	    }
	    if (optind < argc - 1)
		optind++;

	    item.label_type	= OL_STRING;
	    item.attr		= /* OL_LIST_ATTR_CURRENT; */ 0;
	    while ((entry = readdir(dir)) != NULL) {
		String name = XtMalloc(strlen(entry->d_name) + 1);
		strcpy(name, entry->d_name); /* save file name */
		item.label = name;
		item.mnemonic = GetMnemonic(name);
		token = (*addItem)(lists[Pos(row, col)], NULL, NULL, item);
	    }
	    tokens[Pos(row, col)] = token;
	    closedir(dir);

	    XtAddCallback(lists[Pos(row, col)], XtNuserMakeCurrent,
			  MakeCurrentCB, NULL);
	    XtAddCallback(lists[Pos(row, col)], XtNuserDeleteItems, CutCB, NULL);
	    XtAddCallback(text_field[Pos(row, col)], XtNverification,
			  VerifyEditCB, lists[Pos(row, col)]);
	}
    return (lists);
}

static void
CreateMenu(list_widgets)
    WidgetList	list_widgets;
{
    Widget	widget, menuPane;
    int		i;

    /* Create Menu */
    cnt = 0;
    XtSetArg(args[cnt], XtNmenuAugment, False); cnt++;
    XtSetArg(args[cnt], XtNpushpin, OL_OUT); cnt++;
    widget = XtCreatePopupShell("Scrolling List: Edit", menuShellWidgetClass,
				list_widgets[0], args, cnt);

    /* Add callback to catch MENU button */
    for (i = 0; i < num_lists; i++)
	XtAddCallback(list_widgets[i], XtNconsumeEvent, PopupMenuCB, widget);

    /* Get Menu Pane */
    XtSetArg(args[0], XtNmenuPane, &menuPane);
    XtGetValues(widget, args, 1);

    /* Add buttons to menu pane */
    cnt = 0;
    XtSetArg(args[cnt], XtNaccelerator, "Ctrl<c>"); cnt++;
    XtSetArg(args[cnt], XtNmnemonic, 'c'); cnt++;
    widget = XtCreateManagedWidget("change", oblongButtonGadgetClass,
				 menuPane, args, cnt);
    XtAddCallback(widget, XtNselect, EditCB, NULL);

    cnt = 0;
    XtSetArg(args[cnt], XtNaccelerator, "Ctrl<d>"); cnt++;
    XtSetArg(args[cnt], XtNmnemonic, 'd'); cnt++;
    widget = XtCreateManagedWidget("delete", oblongButtonGadgetClass,
				   menuPane, args, cnt);
    XtAddCallback(widget, XtNselect, DeleteCB, NULL);

#ifdef DEBUG
    cnt = 0;
    XtSetArg(args[cnt], XtNaccelerator, "Ctrl<p>"); cnt++;
    XtSetArg(args[cnt], XtNmnemonic, 'p'); cnt++;
    widget = XtCreateManagedWidget("print", oblongButtonGadgetClass,
				  menuPane, args, cnt);
    XtAddCallback(widget, XtNselect, PrintCB, NULL);
#endif
}

static void
CutCB(lw, closure, call_data)
    Widget lw;				/* list widget */
    XtPointer closure, call_data;
{
    OlListDelete *	data = (OlListDelete *) call_data;
    Cardinal		cnt = data->num_tokens;
    OlListToken *	tokens = data->tokens;

    (*update)(lw, FALSE);

    while (cnt--)
    {
	if (*tokens == current_item)
	    current_item = NULL;
	(*deleteItem)(lw, *tokens++);
    }
    (*update)(lw, TRUE);
}

static void
DeleteCB(w, closure, call_data)
    Widget w;
    XtPointer closure, call_data;
{
    if (current_item == NULL)
    {
	Report("No item is current");
	return;
    }

    (*deleteItem)(current_lw, current_item);

    current_item = NULL;

    Report("");
}

static void
EditCB(w, closure, call_data)
    Widget w;
    XtPointer closure, call_data;
{
    if (current_item == NULL)
    {
	Report("No item is current");
	return;
    }

    (*editOn)(current_lw, FALSE, current_item);
}

static char
GetMnemonic(label)
    String label;
{
#define NUM_GRAPH ( '~' - '!' + 1 )
    static Boolean	mnemonic[NUM_GRAPH];
    String		str;
    int			i;

    for (str = label; *str != '\0'; str++)
    {
	i = tolower(*str) - '!';

	if (isgraph(*str) && !mnemonic[i])
	{
	    mnemonic[i] = True;
	    return(*str);
	}
    }

    /* no mnemonic found in label.  Search thru alphabet */
    for (i = 0; i < NUM_GRAPH; i++)
    {
	char ch = tolower(i + '!');

	if (isgraph(ch) && !mnemonic[i])
	{
	    mnemonic[i] = True;
	    return(ch);
	}
    }
	    
    return('\0');
}

static void
MakeCurrentCB(lw, closure, call_data)
    Widget	lw;
    XtPointer	closure, call_data;
{
    OlListToken selected_item = (OlListToken) call_data;

    if (current_item == selected_item && current_lw == lw)
	return;

    /* make previous current_item no longer current */
    if (current_item != NULL)
    {
	OlListItemPointer(current_item)->attr &= ~OL_B_LIST_ATTR_CURRENT;
	(*touch)(current_lw, current_item);
    }

    /* make the selected_item the current item */
    OlListItemPointer(selected_item)->attr |= OL_B_LIST_ATTR_CURRENT;
    (*touch)(lw, selected_item);

    /* Make sure we're in view.  This is for mnemonics and keyboard
	searches; item is made current but may not be visible
    */
    (*view)(lw, selected_item);

    current_item= selected_item;	/* new current_item */
    current_lw	= lw;			/* new current widget */

    ReportCurrent(current_item);
}

static void
PopupMenuCB(w, closure, call_data)
    Widget	w;
    XtPointer	closure, call_data;
{
    OlVirtualEvent	ve = (OlVirtualEvent)call_data;
    Position		x, y;

#define PopupMenu(set, x, y) \
	    OlMenuPopup((Widget)closure, OL_PRESS_DRAG_MENU, set, x, y, NULL);

    switch(ve->virtual_name)
    {
    case OL_MENU :
	ve->consumed = True;

	PopupMenu(False, 0, 0);
	break;

    case OL_MENUKEY :
	ve->consumed = True;

	XtTranslateCoords(w, w->core.width/2, w->core.height/2, &x, &y);

	PopupMenu(True, x, y);
	break;
    }
}

static void
PrintCB(w, closure, call_data)
    Widget w;
    XtPointer closure, call_data;
{
#ifdef DEBUG
extern _OlPrintList();
    _OlPrintList(current_lw);
#else
    Report("Sorry, not compiled for debugging");
#endif
}

static void
Report(msg)
    String msg;
{
    Arg arg[1];

    XtSetArg(arg[0], XtNstring, msg);
    XtSetValues(footer, arg, XtNumber(arg));
}

static void
ReportCurrent(item)
    OlListToken item;
{
    static char msg[100];

    (void) strcpy(msg, "Current: ");
    (void) strcat(msg, OlListItemPointer(item)->label);
    Report(msg);
}

static void
Usage(name)
    char *	name;
{
    fprintf(stderr, "usage: %s [options] directory [directory, ]\n", name);
    fprintf(stderr, "\twhere options are: %s %s %s %s %s",
	    "\n\t\t-l	number of lists to generate",
	    "\n\t\t-u	turn off selection",
	    "\n\t\t-v#	view size (items in view)",
	    "\n\t\t-z	debug",
	    "\n"
	    );
    exit(1);
}

static void
VerifyEditCB(w, closure, call_data)
    Widget w;
    XtPointer closure, call_data;
{
    Widget		lw = (Widget)closure; /* list widget */
    OlTextFieldVerify *	data = (OlTextFieldVerify *) call_data;

    /* "touch" item: stuff new string in it */
    OlListItemPointer(current_item)->label = XtNewString(data->string);
    (*touch)(lw, current_item);
    (*editOff)(lw);		/* turn off editing */
    ReportCurrent(current_item);
}

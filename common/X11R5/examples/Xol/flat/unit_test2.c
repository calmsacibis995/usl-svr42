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
#ident	"@(#)olexamples:flat/unit_test2.c	1.3"
#endif

/******************************file*header********************************

    Description:
	This program tests the flattened list widget.
*/
#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/FList.h>
#include <Xol/ScrolledWi.h>

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

typedef struct {
    XtArgVal	sensitive;
    XtArgVal	managed;
    XtArgVal	mapped;		/* when_managed */
    XtArgVal	fields;		/* vector of field data */
} FlatList;

static String item_resources[] = {
    XtNsensitive,
    XtNmanaged,
    XtNmappedWhenManaged,
    XtNformatData
};

typedef struct {
    XtPointer *	fields;
    Cardinal	num_fields;
    XImage *	selected_image;
    XImage *	unselected_image;
    int		image_field_indx;
} FieldInfo;

typedef struct {
    String	bitmap_filename;
    String	data_filename;
    Cardinal	view_height;
    Boolean	debug;
    Boolean	drop;
    Boolean	exclusives;
    Boolean	execute;
    Boolean	none_set;
    Boolean	scroll;
    Boolean	visibility;
} AppData;

extern Boolean FLdebug;

static Arg	args[20];
static Cardinal	cnt;

static char XtNdebug[]		= "debug";
static char XtNdrop[]		= "drop";
static char XtNvisibility[]	= "visibility";

/**************************forward*declarations***************************

    Forward Procedure declarations
*/
static void	DropCB OL_ARGS((Widget, XtPointer, XtPointer));
static void	ExecuteCB OL_ARGS((Widget, XtPointer, XtPointer));
static int	GetImageFieldIndex OL_ARGS((String));
static XImage *	GetImage OL_ARGS((Widget, String));
static void	GetItemFields OL_ARGS((String, FieldInfo *,
				       Cardinal *, String *));
static void	SelectCB OL_ARGS((Widget, XtPointer, XtPointer));
static void	SetImageField OL_ARGS((Widget, FieldInfo *,
				       OlFlatCallData *, XImage *));
static void	UnselectCB OL_ARGS((Widget, XtPointer, XtPointer));
static void	Usage OL_ARGS((String));
static void	VisibilityCB OL_ARGS((Widget, XtPointer, XtPointer));

/*******************************resources*********************************

    Define Application Resources and Options
*/

#define OFFSET(field) XtOffsetOf(AppData, field)

static XtResource
resources[] = {
  { XtNbitmap, XtCBitmap, XtRString, sizeof(String),
    OFFSET(bitmap_filename), XtRString, "opendot" },

  { XtNdebug, XtCDontCare, XtRBoolean, sizeof(Boolean),
    OFFSET(debug), XtRImmediate, (XtPointer) False },

  { XtNdrop, XtCDontCare, XtRBoolean, sizeof(Boolean),
    OFFSET(drop), XtRImmediate, (XtPointer) True },

  { XtNexclusives, XtCExclusives, XtRBoolean, sizeof(Boolean),
    OFFSET(exclusives), XtRImmediate, (XtPointer) True },

  { XtNexecute, XtCDontCare, XtRBoolean, sizeof(Boolean),
    OFFSET(execute), XtRImmediate, (XtPointer) True },

  { XtNfile, XtCFile, XtRString, sizeof(String),
    OFFSET(data_filename), XtRString, "list_data" },

  { XtNnoneSet, XtCNoneSet, XtRBoolean, sizeof(Boolean),
    OFFSET(none_set), XtRImmediate, (XtPointer) False },

  { XtNscroll, XtCScroll, XtRBoolean, sizeof(Boolean),
    OFFSET(scroll), XtRImmediate, (XtPointer) True },

  { XtNviewHeight, XtCViewHeight, XtRCardinal, sizeof(Cardinal),
    OFFSET(view_height), XtRImmediate, (XtPointer)0 },

  { XtNvisibility, XtCDontCare, XtRBoolean, sizeof(Boolean),
    OFFSET(visibility), XtRImmediate, (XtPointer) False },

};

#undef OFFSET

static XrmOptionDescRec options[XtNumber(resources)] = {
  { "-bitmap",		"*bitmap",	XrmoptionSepArg, NULL },
  { "-debug",		"*debug",	XrmoptionNoArg, "True" },
  { "-drop",		"*drop",	XrmoptionNoArg, "False" },
  { "-execute",		"*execute",	XrmoptionNoArg, "False" },
  { "-file",		"*file",	XrmoptionSepArg, NULL },
  { "-noneSet",		"*noneSet",	XrmoptionNoArg, "True" },
  { "-nonexclusives",	"*exclusives",	XrmoptionNoArg, "False" },
  { "-scroll",		"*scroll",	XrmoptionNoArg, "False" },
  { "-viewHeight",	"*viewHeight",	XrmoptionSepArg, NULL },
  { "-visibility",	"*visibility",	XrmoptionNoArg, "True" },
};

String desc[XtNumber(resources)] = {
    "filename (Bitmap for unselected items.  Default: opendot)",
    "(Turn on debugging mode)",
    "(Don't register drop callproc)",
    "(Don't register execute callproc)",
    "filename (List data file.  Default: list_data)",
    "(Exclusives only: allow zero controls set)",
    "(Nonexclusives behavior)",
    "(Don't put list in Scrolled Window)",
    "num (Number of items in view)",
    "(Register itemVisibility callback)",
};

/****************************procedure*header*****************************

    main - program driver function
*/
main(argc, argv)
    int		argc;
    char *	argv[];
{

#ifdef UseXtApp
    XtAppContext	app_con;
#endif

    Widget	shell, parent, flist;
    AppData	app_data;
    FieldInfo	field_info;
    FlatList *	items;
    Cardinal	num_items;
    String	format;
    Cardinal	i;

    OlToolkitInitialize(&argc, argv, (XtPointer)NULL);

	/* Do OlSetGui() here if only want OL_OPENLOOK_GUI. Comments	*/
	/* are because you didn't call OlToolkitInitialize before...	*/
#ifdef UseXtApp
    shell = XtAppInitialize(
			&app_con,		/* app_context_return	*/
			"FlatList",		/* application_class	*/
			options,		/* options		*/
			XtNumber(options),	/* num_options		*/
			&argc,			/* argc_in_out		*/
			argv,			/* argv_in_out		*/
			(String)NULL,		/* fallback_resources	*/
			(ArgList)NULL,		/* args			*/
			(Cardinal)0		/* num_args		*/
    );
#else
    shell = XtInitialize("Flat List", "FlatList",
			options, XtNumber(options), &argc, argv);
#endif

    if (argc > 1)
	Usage(argv[0]);

    XtGetApplicationResources(shell, &app_data,
			      resources, XtNumber(resources), NULL, 0);

    if (!app_data.exclusives && app_data.none_set)
	fprintf(stderr, "%s: none-set only applies to exclusives\n",
		argv[0]);

#ifdef DEBUG
    FLdebug = app_data.debug;
#else
    if (app_data.debug)
	fprintf(stderr, "%s: Sorry, not compiled for debugging\n", argv[0]);
#endif

    if (app_data.scroll)
	parent = XtCreateManagedWidget("Scrolled Window",
				       scrolledWindowWidgetClass,
				       shell, NULL, 0);
    else
	parent = shell;

    GetItemFields(app_data.data_filename, &field_info, &num_items, &format);

    items = (FlatList *)XtMalloc(sizeof(FlatList) * num_items);

    for (i = 0; i < num_items; i++)
    {
	items[i].sensitive	= (XtArgVal)True;
	items[i].managed	= (XtArgVal)True;
	items[i].mapped		= (XtArgVal)True;
	items[i].fields		= (XtArgVal)(field_info.fields +
					     i * field_info.num_fields);
    }
	
    field_info.image_field_indx = GetImageFieldIndex(format);

    if (app_data.view_height == 0)
	app_data.view_height = (num_items < 20) ? (num_items + 3) / 2 : 9;

    cnt = 0;
    if (field_info.image_field_indx >= 0)
    {
	field_info.selected_image = GetImage(parent, "star");
	field_info.unselected_image =
	    GetImage(parent, app_data.bitmap_filename);

	XtSetArg(args[cnt], XtNselectProc, SelectCB); cnt++;
	XtSetArg(args[cnt], XtNunselectProc, UnselectCB); cnt++;
	XtSetArg(args[cnt], XtNclientData, &field_info); cnt++;
    }
    if (app_data.drop)
	XtSetArg(args[cnt], XtNdropProc, DropCB); cnt++;
    if (app_data.execute)
	XtSetArg(args[cnt], XtNexecuteProc, ExecuteCB); cnt++;
    XtSetArg(args[cnt], XtNviewHeight, app_data.view_height); cnt++;
    XtSetArg(args[cnt], XtNexclusives, app_data.exclusives); cnt++;
    XtSetArg(args[cnt], XtNnoneSet, app_data.none_set); cnt++;
    XtSetArg(args[cnt], XtNitems, items); cnt++;
    XtSetArg(args[cnt], XtNnumItems, num_items); cnt++;
    XtSetArg(args[cnt], XtNitemFields, item_resources); cnt++;
    XtSetArg(args[cnt], XtNnumItemFields, XtNumber(item_resources)); cnt++;
    XtSetArg(args[cnt], XtNformat, format); cnt++;
    flist = XtCreateManagedWidget("FList", flatListWidgetClass,
				  parent, args, cnt);

    if (app_data.visibility)
	XtAddCallback(flist, XtNitemVisibility, VisibilityCB, NULL);

    XtRealizeWidget(shell);
#ifdef UseXtApp
    XtAppMainLoop(app_con);
#else
    XtMainLoop();
#endif

    return(0);			/* for lint */
}				/* END OF main() */

/****************************procedure*header*****************************
    ExecuteCB-
*/
/* ARGSUSED */
static void
ExecuteCB OLARGLIST((widget, client_data, call_data))
    OLARG( Widget,	widget )
    OLARG( XtPointer,	client_data )
    OLGRA( XtPointer,	call_data )
{
    OlFlatDropCD *	drop_data = (OlFlatDropCD *)call_data;
    Cardinal		indx = drop_data->item_data->item_index;

#ifndef TEST
    printf("Execute callback on item %d\n", indx);

#else
# ifdef nodef
    FlatList * list = (FlatList *)drop_data->item_data->items;

    list[indx].sensitive = False;

    XtVaSetValues(widget,
		  XtNitemsTouched, True,
		  NULL);
# else

    OlVaFlatSetValues(widget, indx,
		      XtNsensitive, False,
		      NULL);
# endif
#endif
}

/****************************procedure*header*****************************
    DropCB-
*/
/* ARGSUSED */
static void
DropCB OLARGLIST((widget, client_data, call_data))
    OLARG( Widget,	widget )
    OLARG( XtPointer,	client_data )
    OLGRA( XtPointer,	call_data )
{
    OlFlatDropCD *	drop_data = (OlFlatDropCD *)call_data;
    Cardinal		indx = drop_data->item_data->item_index;

#ifndef TEST
    printf("Drop callback on item %d\n", indx);

#else

    OlVaFlatSetValues(widget, indx,
		      XtNset /* mappedWhenManaged */, False,
		      NULL);
#endif
}

/****************************procedure*header*****************************
    GetImage-
*/
static XImage *
GetImage OLARGLIST((w, filename))
    OLARG( Widget,	w )
    OLGRA( String,	filename )
{
#define PATH "/usr/X/include/X11/bitmaps/"

    XImage *	image = NULL;
    Pixmap	bitmap;
    uint	width, height;
    int		hot_spot;
    int		stat;
    String	file;

    if ((filename == NULL) || (filename[0] == '\0'))
	return (NULL);

    if ((filename[0] == '/') ||
	((filename[0]=='.') && (filename[1]=='.') && (filename[2]=='/')))
    {
	file = filename;

    } else {
	file = (String)XtMalloc(strlen(PATH) + strlen(filename));

	(void)strcpy(file, PATH);
	(void)strcat(file, filename);

    }

    stat = XReadBitmapFile(XtDisplay(w), RootWindowOfScreen(XtScreen(w)),
			   file, &width, &height, &bitmap,
			   &hot_spot, &hot_spot);

    switch (stat)
    {
    case BitmapFileInvalid :
	OlError("Invalid bitmap file");
	break;

    case BitmapOpenFailed :
	OlError("Failed to open bitmap file");
	break;

    case BitmapNoMemory :
	OlError("Failed to allocate pixmap");
	break;

    case BitmapSuccess :
	break;

    default:
	OlError("Unknown status returned from XReadBitmapFile");
	break;
    }

    if (file != filename)
	XtFree(file);

    /* Extract XImage from Pixmap */

    image = XGetImage(XtDisplay(w), bitmap,
		      0, 0, width, height, AllPlanes, XYPixmap);

    /* Can't specify XYBitmap as format to XGetImage so cheat a little and
       change it after the fact
    */
    image->format = XYBitmap;

    XFreePixmap(XtDisplay(w), bitmap);

    return (image);
}

/****************************procedure*header*****************************
    GetImageFieldIndex-
*/
static int
GetImageFieldIndex OLARGLIST((format))
    OLGRA( String,	format )
{
    int indx;

    for (indx = -1; *format != '\0'; format++)
    {
	if (*format == '%')
	    indx++;

	else if (*format == 'i')
	    return(indx);
    }

    return(-1);
}

/****************************procedure*header*****************************
    GetItemFields- read field data from file and return to caller.

	filename - read data from this file.
	return_field_info - return fields and num_fields to this struct.
	return_cnt - return item count.
	return_format - return field format.
*/
static void
GetItemFields OLARGLIST((filename, return_field_info, return_cnt, return_format))
    OLARG( String,	filename )
    OLARG( FieldInfo *,	return_field_info )
    OLARG( Cardinal *,	return_cnt )
    OLGRA( String *,	return_format )
{
#define BUF_SIZE 1024
    XtPointer **fields;
    Cardinal	num_fields;
    Cardinal	field_cnt;
    Cardinal	item_cnt;
    FILE *	file;
    Boolean	eof;
    char	buf[BUF_SIZE];
    String	ptr;
    String *	field;
    String	start;

    if ((file = fopen(filename, "r")) == NULL)
	OlError("Failed to open list data file");	/* doesn't return */

    if (fgets(buf, BUF_SIZE, file) == NULL)	/* read format string */
	OlError("Failed to read format string from data file");

    *return_format = (String)strdup(buf);

    num_fields = 0;
    for (ptr = buf; *ptr != '\0'; ptr++)
	if (*ptr == '%')
	    num_fields++;

    fields = NULL;
    eof = False;
    for (item_cnt = 0; !eof; item_cnt++)
    {
	buf[0] = '\0';

	if (fgets(buf, BUF_SIZE, file) == NULL)
	    eof = True;

	if (buf[0] == '\0')
	    continue;

	fields = (XtPointer **)XtRealloc
	    (fields, sizeof(XtPointer *) * num_fields * (item_cnt + 1));

	/* Collect up all fields for this item into realloc'd memory */

	for (field_cnt = 0, start = buf, field = (String *)
	     (fields + (num_fields * item_cnt));
	     field_cnt < num_fields;
	     field_cnt++, start = ptr, field++)
	{
	    /* Run thru field looking for special chars */

	    for (ptr = start; (*ptr != '\t') && (*ptr != '\0'); ptr++)
	    {
		/* Special '\\' in input means '\n' in output  :-( */
		if (*ptr == '\\')
		{
		    *ptr = '\n';

		} else if ((ptr[0] == '\n')  && (ptr[1] == '\0')) {

		    /* Each item in the file is delimited by a '\n' (the last
		       item may or may not have a '\n') and fgets copies this
		       in.  Terminate the field at the '\n' and inc over it.
		    */

		    *ptr++ = '\0';
		}
	    }

	    *ptr++ = '\0';		/* delimit field and inc */
					/* duplicate string into 'field': */
	    *field = (*start == '\0') ? NULL : (String)strdup(start);
	}
    }

    fclose(file);

    return_field_info->fields		= (XtPointer *)fields;
    return_field_info->num_fields	= num_fields;
    *return_cnt				= item_cnt - 1;
}

/****************************procedure*header*****************************
    SelectCB-
*/
static void
SelectCB OLARGLIST((widget, client_data, call_data))
    OLARG( Widget,	widget )
    OLARG( XtPointer,	client_data )
    OLGRA( XtPointer,	call_data )
{
    FieldInfo *	field_info = (FieldInfo *)client_data;

    SetImageField(widget, field_info,
		  (OlFlatCallData *)call_data, field_info->selected_image);
}

/****************************procedure*header*****************************
    SetImageField-
*/
static void
SetImageField OLARGLIST((widget, field_info, fcd, image))
    OLARG( Widget,		widget )
    OLARG( FieldInfo *,		field_info )
    OLARG( OlFlatCallData *,	fcd )
    OLGRA( XImage *,		image )
{
    XtPointer *	field;

    field = field_info->fields + fcd->item_index * field_info->num_fields;

    field[field_info->image_field_indx] = (XtPointer)image;

    OlVaFlatSetValues(widget, fcd->item_index,
		      XtNformatData, field,
		      NULL);
}

/****************************procedure*header*****************************
    UnselectCB-
*/
static void
UnselectCB OLARGLIST((widget, client_data, call_data))
    OLARG( Widget,	widget )
    OLARG( XtPointer,	client_data )
    OLGRA( XtPointer,	call_data )
{
    FieldInfo *	field_info = (FieldInfo *)client_data;

    SetImageField(widget, (FieldInfo *)client_data,
		  (OlFlatCallData *)call_data, field_info->unselected_image);
}

/****************************procedure*header*****************************
    VisibilityCB-
*/
/* ARGSUSED */
static void
VisibilityCB OLARGLIST((widget, client_data, call_data))
    OLARG( Widget,	widget )
    OLARG( XtPointer,	client_data )
    OLGRA( XtPointer,	call_data )
{
    OlFlatItemVisibilityCD *	cd = (OlFlatItemVisibilityCD *)call_data;
    int				i;

    printf("num_enters= %d: ", cd->num_enters);

    for (i = 0; i < cd->num_enters; i++)
    {
	printf("%d ", cd->enters[i]);
    }
    printf("\n");

    printf("num_leaves= %d: ", cd->num_leaves);

    for (i = 0; i < cd->num_leaves; i++)
    {
	printf("%d ", cd->leaves[i]);
    }
    printf("\n");
}

/****************************procedure*header*****************************
    Usage-
*/
static void
Usage OLARGLIST((name))
    OLGRA( String,	name )
{
    int i;

    fprintf(stderr, "usage: %s [options]\n\twhere options are:\n", name);

    for (i = 0; i < XtNumber(resources); i++)
	fprintf(stderr, "\t%s %s\n", options[i].option, desc[i]);

    exit(1);
}

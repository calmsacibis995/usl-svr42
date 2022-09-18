/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olps:main.c	1.36"
#endif

/*
 * main.c - startup olprintscreen:
 * 	    declare all global variables,
 *	    create all the widgets 
 *	    except for scrolled window and stub
 */

#include "main.h"
#include "externs.h"

#include <Xol/OpenLookP.h>
#include <Xol/OlCursors.h>
#include <Xol/MenuShell.h>
#include <Xol/FButtons.h>
#include "error.h"

#include "olps.icon"
#ifdef oldcode
#include "wait.curs"
#endif
#include "area.curs"
#include "window.curs"

#ifdef MITSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include "XShm.h"
#include "Xstreams.h"

extern char TypeOfStream[];
XShmSegmentInfo shminfo;
#endif

XImage *shmimage;
int noflag=0;	/* flag indicate either print window, area, screen happened
			in shared memory version 	*/

int xlocal = 0;

static void FillButtonLabels();

extern Widget AddMenu();

char * ButtonFields[] = {
	XtNlabel,
	XtNselectProc,
	XtNmnemonic
};
char *NoticeFields[] = {
	XtNlabel,
	XtNselectProc,
	XtNmnemonic
};
/* On the "Save file" popup window, there is a notice that needs these
 * two buttons - OverWrite and Cancel
 */

/* Overwrite, Cancel */
NoticeItems SaveFileNoticeItems[] = {
	{ (XtArgVal)NULL, (XtArgVal)Save_Overwrite,(XtArgVal)'O'},
	{ (XtArgVal)NULL, (XtArgVal)Save_Cancel, (XtArgVal)'C'}
};
/* Save  or  Cancel */
ButtonItems SaveFileItems[] = {
	{ (XtArgVal)NULL, (XtArgVal)Save_Resolve, (XtArgVal)'S'},
	{ (XtArgVal)NULL, (XtArgVal)Save_Cancel, (XtArgVal)'C'}
};

/* Open */
ButtonItems OpenFileItems[] = {
	{ (XtArgVal)NULL, (XtArgVal)Open_Resolve, (XtArgVal)'O'},
	{ (XtArgVal)NULL, (XtArgVal)Open_Cancel, (XtArgVal)'C'}
};

/* Print */

ButtonItems PrintFileItems[] = {
	{ (XtArgVal)NULL, (XtArgVal)Print_Resolve, (XtArgVal)'P'},
	{ (XtArgVal)NULL, (XtArgVal)Print_Cancel, (XtArgVal)'C'}
};

/* static or extern declarations for menu shells, flat buttons */
/* Open..., and Save As... */
MenuItem FileMenuItems[] =  {
  { (XtArgVal)NULL, (XtArgVal)Open_Pop, (XtArgVal)&open_pop, (XtArgVal)True, 
		(XtArgVal)True, (XtArgVal)0,(XtArgVal)'O'},
  { (XtArgVal)NULL, (XtArgVal)Save_Pop, (XtArgVal)&save_pop,
		(XtArgVal)True, (XtArgVal)True, (XtArgVal)0,(XtArgVal)'A'}
};

/* Contents, Window, Area, Screen, Image File... */
MenuItem PrintMenuItems[] =  {
  { (XtArgVal)NULL, (XtArgVal)Print_Contents, (XtArgVal)0,	
		(XtArgVal)True, (XtArgVal)True, (XtArgVal)0, (XtArgVal)'C'},
  { (XtArgVal)NULL, (XtArgVal)Print_Window,	(XtArgVal)0,	
		(XtArgVal)True, (XtArgVal)True, (XtArgVal)0, (XtArgVal)'W'},
  { (XtArgVal)NULL, (XtArgVal)Print_Area,	(XtArgVal)0,	
		(XtArgVal)True, (XtArgVal)True, (XtArgVal)0, (XtArgVal)'A'},
  { (XtArgVal)NULL, (XtArgVal)Print_Screen,	(XtArgVal)0,
		(XtArgVal)True, (XtArgVal)True, (XtArgVal)0, (XtArgVal)'S'},
  { (XtArgVal)NULL, (XtArgVal)Printf_Pop, (XtArgVal)&printf_pop,
		(XtArgVal)True, (XtArgVal)True, (XtArgVal)0, (XtArgVal)'I'},
};

/* Window, Area, Screen */
MenuItem CaptureMenuItems[] =  {
  {(XtArgVal)NULL, (XtArgVal)Capture_Window, (XtArgVal)0,
		 (XtArgVal)True, (XtArgVal)True, (XtArgVal)0 , (XtArgVal)'W'},
  {(XtArgVal)NULL, (XtArgVal)Capture_Area,	(XtArgVal)0,
		 (XtArgVal)True, (XtArgVal)True, (XtArgVal)0 , (XtArgVal)'A'},
  {(XtArgVal)NULL, (XtArgVal)Capture_Screen, (XtArgVal)0,
		 (XtArgVal)True, (XtArgVal)True, (XtArgVal)0, (XtArgVal)'S'}
};

/* Main - Not really a menu, but a bunch of flat buttons in menubar style */

Menu FileMenu = {
	"File",
	FileMenuItems,
	XtNumber(FileMenuItems),
	True,
	OL_FIXEDCOLS,
	OL_NONE,
	NULL
};

Menu PrintMenu = {
	"Print",
	PrintMenuItems,
	XtNumber(PrintMenuItems),
	True,
	OL_FIXEDCOLS,
	OL_NONE,
	NULL
};

Menu CaptureMenu = {
	"Capture",
	CaptureMenuItems,
	XtNumber(CaptureMenuItems),
	True,
	OL_FIXEDCOLS,
	OL_NONE,
	NULL
};

#define FILEPOP	0
#define PRINT	1
#define CAPTURE	2
#define PROP	3

    /* "File", "Print, "Capture, "Properties..." */
static MenuItem MainItems[] = {
    {(XtArgVal)NULL,	(XtArgVal)0,	(XtArgVal)0, (XtArgVal)True,
		(XtArgVal)True,(XtArgVal)0, (XtArgVal)'F'},
    {(XtArgVal)NULL,		(XtArgVal)0,	(XtArgVal)0,
	(XtArgVal)True,(XtArgVal)True,(XtArgVal)0,(XtArgVal) 'r'},
    {(XtArgVal)NULL, (XtArgVal)0, (XtArgVal)0,	(XtArgVal)True, (XtArgVal)True,
				(XtArgVal)0, (XtArgVal)'C'},
    {(XtArgVal)NULL,	(XtArgVal)Properties_Pop,	(XtArgVal)&prop_pop,
		(XtArgVal)True,(XtArgVal)True,(XtArgVal)0, (XtArgVal)'P'}
};
Menu MainMenu = {
	"",
	MainItems,
	XtNumber(MainItems),
	False,
	OL_FIXEDROWS,
	OL_NONE,
	NULL
};

char * MenuFields[] = {
	XtNlabel,
	XtNselectProc,
	XtNclientData,
	XtNsensitive,
	XtNmappedWhenManaged,
	XtNpopupMenu,		
	XtNmnemonic
};


 Arg		  args[7];

int		  cnt;

char		* appname;
Display 	* dpy;
int		  screen;
GC 		  gc;
XGCValues 	  gc_val;
Window 		  stub_window;
Pixmap 		  pix;

FILE 		* rw_file = (FILE *)NULL;
XImage 		  image_struct,
                * image_ptr = NULL,
                * image_content = NULL, 
                * image_nocontent = NULL;

XWDFileHeader     header,               header_nc;
int               header_size,          header_size_nc;
char            * win_name = NULL,    *	win_name_nc = NULL;
int               win_name_size,        win_name_size_nc;
XColor          * colors = NULL,      * colors_nc = NULL;
int               ncolors,              ncolors_nc;
char            * buffer = NULL;
unsigned          buffer_size,          buffer_size_nc;

unsigned long 	  swaptest = 1;
XWindowAttributes win_info;
Window		  rootwindow;
int		  rootwidth, rootheight;
Dimension	  olps_width_limit, olps_height_limit,
		  form_save_width, form_save_height;

XEvent  	  event;

int 		  f_contents = False, 
		  f_contentssaved = False, 
		  f_bypass = False, 
		  f_screen = False, 
		  f_area = False, 
		  f_save_overwrite = False, 
		  redraw = False,
		  f_unmapping = False,
		  f_open_pop = False,
		  f_save_pop = False,
		  f_printf_pop = False,
		  f_prop_pop = False,
		  f_fromopenfile = False,
		  f_toggle = False,
		  f_failed = False;

Widget		  toplevel,
		  form,
		  upper_control,
	  	  scrollw,	
	  	  stub,
	  	  open_pop_textf,		
	  	  save_pop_textf,	
	  	  printf_pop_textf1, 
		  footer_text,
		  save_pop_ok,
		  printf_pop_ok,
		  open_pop_ok,
		  save_notice,
		  save_notice_control,
		  save_notice_text,
		  open_pop,
		  save_pop,
		  printf_pop,
		  prop_pop,
		  flat_notice_buttons;

char		  printcmd[PRINTCMD_WIDTH];
char    	  pixmapformat[TEXTF_WIDTH_2];
char 		  message[500];
static	char	  titlebar[TEXTF_WIDTH_1];

Cursor 		  wait_cursor,	/* busy cursor */
		  sa_cursor, 	/* select area cursor */
		  sw_cursor;	/* select window cursor */

main (argc, argv)
int argc;
char **argv;
{
#ifdef UseXtApp
	XtAppContext	app_con;
#endif

	/* Creation of visuals - all high level stuff */
	Widget  footer_panel;
	Widget  file_stack, file_menupane, file_open, file_saveas;
	Widget  print_stack, print_menupane, print_contents, print_window, 
		print_area, print_screen, print_file;	
	Widget	capture_stack, capture_menupane, 
		capture_window, capture_area, capture_screen;	
	Widget	properties;
	char	*getcwd(), *cwd;
    	Pixmap 	icon_pixmap;
    	XWindowAttributes 	win_info1;
        XColor d1, bg, fg;
        Colormap cmap;

	char *file_msg;

#ifdef MEMUTIL
	InitializeMemutil();
#endif

	OlToolkitInitialize(&argc, argv, NULL);

#ifdef UseXtApp
	toplevel = XtAppInitialize(
			&app_con,		/* app_context_return	*/
			"olprintscreen",	/* application_class	*/
			(XrmOptionDescList)NULL,/* options		*/
			(Cardinal)0,		/* num_options		*/
			&argc,			/* argc_in_out		*/
			argv,			/* argv_in_out		*/
			(String)NULL,		/* fallback_resources	*/
			(ArgList)NULL,		/* args			*/
			(Cardinal)0		/* num_args		*/
	);
#else
	toplevel = XtInitialize("olprintscreen", "olprintscreen",
				 NULL, 0, &argc, argv);
#endif
	file_msg = OlGetMessage( XtDisplay(toplevel),
						NULL,
						0,
						OleNfileMain,
						OleTfileTitle,
						OleCOlClientOlpsMsgs,
						OleMfileMain_fileTitle,
						(XrmDatabase)NULL);

	/* To get name for getting/setting .Xdefaults' resources later */
	appname = (char *) malloc (sizeof(char) * (strlen(argv[0]) + 1));
        strcpy(appname, argv[0]);
	system("/bin/rm -f /usr/tmp/olps??????");
	printcmd[0]='\0';
        strcpy(titlebar, OlGetMessage(	XtDisplay(toplevel),
					NULL,
					0,
					OleNfileMain,
					OleTprintScreen,
					OleCOlClientOlpsMsgs,
					OleMfileMain_printScreen,
					(XrmDatabase)NULL));

	cwd = getcwd((char *)NULL, TEXTF_WIDTH_1);
	strcat(titlebar, cwd);
	free(cwd);

	_OlSetApplicationTitle(titlebar);

	dpy = XtDisplay(toplevel);
	screen = DefaultScreen(dpy);

	/* Set this once, for capturing images later */
	rootwindow = RootWindow(dpy, screen);
   	XGetWindowAttributes(dpy, rootwindow, &win_info1);
	rootwidth = win_info1.width;
	rootheight = win_info1.height;

#ifdef MITSHM	
	if((TypeOfStream[dpy->fd] == X_LOCAL_STREAM) ||
	   (TypeOfStream[dpy->fd] == X_NAMED_STREAM))	{
		xlocal = 1;
	}


        if(xlocal)	{
            if (!XShmQueryExtension(dpy))
   		xlocal = 0;
        } 

	if (xlocal)	{

	        shmimage = XShmCreateImage(dpy, DefaultVisual(dpy,screen),
				DisplayPlanes(dpy,screen),ZPixmap,
                                0, &shminfo, rootwidth, rootheight);

/*
	        shmimage = XShmCreateImage(dpy, DefaultVisual(dpy,screen),
				1,XYPixmap,
                                0, &shminfo, rootwidth, rootheight);
*/
       		if(!shmimage)
			OlVaDisplayErrorMsg(	XtDisplay(toplevel),
						OleNfileMain,
						OleTbadShmimage,
						OleCOlClientOlpsMsgs,
						OleMfileMain_badShmimage);

       		shminfo.shmid = shmget(IPC_PRIVATE,Image_Size(shmimage),
                               IPC_CREAT|0777);

        	if (shminfo.shmid < 0)
			OlVaDisplayErrorMsg(	XtDisplay(toplevel),
						OleNfileMain,
						OleTbadShmget,
						OleCOlClientOlpsMsgs,
						OleMfileMain_badShmget);

       		shminfo.shmaddr = (char *)shmat(shminfo.shmid, 0, 0);
	        if (shminfo.shmaddr == ((char *)-1))
			OlVaDisplayErrorMsg(	XtDisplay(toplevel),
						OleNfileMain,
						OleTbadShmat,
						OleCOlClientOlpsMsgs,
						OleMfileMain_badShmat);

	        image_struct.data = shminfo.shmaddr;
	        shmimage->data = shminfo.shmaddr;
       		shminfo.readOnly = False;
	        XShmAttach(dpy, &shminfo);
	
		image_struct = *shmimage;
		image_nocontent = shmimage;

	}
#endif

	cnt = 0;
	XtSetArg(args[cnt], XtNtitle, titlebar);	cnt++;
	XtSetValues(toplevel, args, cnt);

	/* Set icon bitmap */
        icon_pixmap = (XtArgVal) XCreateBitmapFromData
			(dpy, rootwindow,
                         (OLconst char *)icon_olps_bits,
			 icon_olps_width, icon_olps_height);
	cnt = 0;
    	XtSetArg(args[cnt], XtNiconPixmap, icon_pixmap);  cnt++;
        XtSetValues (toplevel, args, cnt);

	cnt = 0;
	footer_panel = XtCreateManagedWidget(	"footer", 
                           			footerPanelWidgetClass,
                                     		toplevel, args, cnt);
	cnt = 0;
	XtSetArg(args[cnt], XtNshadowThickness, 0); cnt++;
	form = XtCreateManagedWidget("form",formWidgetClass,
				    footer_panel, args, cnt);

	OlRegisterHelp(	OL_WIDGET_HELP,
			(XtPointer)footer_panel,
			OlGetMessage( XtDisplay(toplevel),
					NULL,
					0,
					OleNfileMain,
					OleTgeneral,
					OleCOlClientOlpsMsgs,
					OleMfileMain_general,
					(XrmDatabase)NULL),
		       	OL_DISK_SOURCE,
			OlFindHelpFile(footer_panel,HELP_OLPS));

	cnt = 0;
        XtSetArg(args[cnt], XtNxResizable, TRUE);         cnt++;
        XtSetArg(args[cnt], XtNxAttachRight, TRUE);         cnt++;

        XtSetArg(args[cnt], XtNborderWidth, 0); cnt++;
        XtSetArg(args[cnt], XtNlayoutType, OL_FIXEDROWS); cnt++;
        XtSetArg(args[cnt], XtNmeasure, 1); cnt++;
        upper_control = XtCreateManagedWidget("control",controlAreaWidgetClass,
					form, args, cnt);

	FillButtonLabels();
	/* Flat menus and flat buttons */
	/* The first 3 return the menushell, the 4th returns
	 * the flatButtons widget, because they are being placed
	 * on the bulletin board.
	 */
	MainItems[FILEPOP].popup = (XtArgVal)AddMenu(toplevel, &FileMenu);
	MainItems[PRINT].popup = (XtArgVal)AddMenu(toplevel, &PrintMenu);
	MainItems[CAPTURE].popup = (XtArgVal)AddMenu(toplevel, &CaptureMenu);
	MainMenu.flatORshell = AddMenu(upper_control, &MainMenu);	
	
    /* Set Scrolled Window viewing up to maximum of 
       predetermined width and height limits
       (set earlier by system defaults or already overwritten
       by user resizing the application) */

	cnt = 0;
	XtSetArg(args[cnt], XtNxAddWidth, FALSE);		cnt++;
	XtSetArg(args[cnt], XtNyAddHeight, TRUE);		cnt++;
	XtSetArg(args[cnt], XtNyRefWidget, upper_control);	cnt++;
	XtSetArg(args[cnt], XtNxResizable, TRUE);		cnt++;
	XtSetArg(args[cnt], XtNyResizable, TRUE);		cnt++;
	XtSetArg(args[cnt], XtNxAttachRight, TRUE);             cnt++;
	XtSetArg(args[cnt], XtNyAttachBottom, TRUE);             cnt++;
	scrollw = XtCreateManagedWidget("ScrolledWindow",
					  scrolledWindowWidgetClass,
					  form, args, cnt);

        /*  Create Stub */
        cnt = 0;
    	XtSetArg(args[cnt], XtNheight, 40);         cnt++;
    	XtSetArg(args[cnt], XtNwidth, 80);           cnt++;
        XtSetArg(args[cnt], XtNborderWidth, 0);          cnt++;
        XtSetArg(args[cnt], XtNexpose, Redraw);                cnt++;
	XtSetArg(args[cnt], XtNtraversalOn, False);	       cnt++;
        stub = XtCreateManagedWidget("stub", stubWidgetClass,
                                    scrollw, args, cnt);

	OlDnDRegisterDDI(stub, OlDnDSitePreviewNone, PSTriggerNotify,
			 (OlDnDPMNotifyProc)NULL, True, NULL);
	
	/* Create "Save File" popup window (is this necessry?  why not
	 * create and destroy these on the fly??
	 * Some return values:
	 * save_pop = the popup window shell itself;
	 * save_pop_textf = the textfield inside
	 * save_pop_ok = OK button;
	 */
        CreateCommandWindow( upper_control, &save_pop,
		OlGetMessage( XtDisplay(toplevel), NULL, 0,
					OleNtitle,
					OleTlabelSave,
					OleCOlClientOlpsMsgs,
					OleMtitle_labelSave,
					(XrmDatabase)NULL),
		"Print Screen Save File", file_msg, &save_pop_textf, 
		"Save", /* this is the widget name */
		&save_pop_ok, /* Flat button in the popup shell */
		    SaveFileItems, XtNumber(SaveFileItems));
	/* file_saveas - the button; Save_Pop - the callback;
	 * save_pop - the client_data; Save_Pop does nothing more
	 * than call XtPopup() on client_data!  
	 */
        XtAddCallback(save_pop, XtNpopupCallback, Save_Popup, NULL);
        XtAddCallback(save_pop, XtNpopdownCallback, Save_Popdown, NULL);

	/* create notice popup,
	   attach string to textArea,
	   attach emanate widget */
	/* This code makes the notice shell a child of the popup
	 * shell, and makes the emanate widget for the notice
	 * the OK button.
	 */
	cnt = 0;
        XtSetArg(args[cnt], XtNemanateWidget, save_pop_ok); cnt++;
	save_notice = XtCreatePopupShell("notice", noticeShellWidgetClass,
                     	save_pop, 
			args, cnt);
	/* add Overwrite and Cancel buttons
	    to notice control area */
	cnt = 0;
        XtSetArg(args[cnt], XtNcontrolArea, &save_notice_control); cnt++;
        XtSetArg(args[cnt], XtNtextArea, &save_notice_text); cnt++;
	XtGetValues(save_notice, args, cnt);
	flat_notice_buttons = XtVaCreateManagedWidget("S_O_NoticeButtons",
			flatButtonsWidgetClass,
			save_notice_control,
			XtNitemFields, NoticeFields,
			XtNnumItemFields, XtNumber(NoticeFields),
			XtNitems, SaveFileNoticeItems,
			XtNnumItems, XtNumber(SaveFileNoticeItems),
			(char *)0 );

        CreateCommandWindow( upper_control,
		    &open_pop,
		    OlGetMessage( XtDisplay(toplevel), NULL, 0,
					OleNtitle,
					OleTlabelOpen,
					OleCOlClientOlpsMsgs,
					OleMtitle_labelOpen,
					(XrmDatabase)NULL),
                    "Print Screen Open File",
                    file_msg, 
                    &open_pop_textf, 
                    "Open", 
		    &open_pop_ok, /* flat button on the popup shell */
		    OpenFileItems,
		XtNumber(OpenFileItems));

        XtAddCallback(open_pop, XtNpopupCallback, Open_Popup, NULL);
        XtAddCallback(open_pop, XtNpopdownCallback, Open_Popdown, NULL);

        CreateCommandWindow( upper_control, &printf_pop,
		    OlGetMessage( XtDisplay(toplevel), NULL, 0,
					OleNtitle,
					OleTlabelPrint,
					OleCOlClientOlpsMsgs,
					OleMtitle_labelPrint,
					(XrmDatabase)NULL),
                    "Print Screen Print File",
                    file_msg,
                    &printf_pop_textf1, 
                    "Print", 
		    &printf_pop_ok, /* flat button on the popup shell */
		    PrintFileItems,
			XtNumber(PrintFileItems));

        XtAddCallback(printf_pop, XtNpopupCallback, Printf_Popup, NULL);
        XtAddCallback(printf_pop, XtNpopdownCallback, Printf_Popdown, NULL);

	CreatePropertyWindow( upper_control,
			&prop_pop);
        XtAddCallback(prop_pop, XtNpopupCallback, Prop_Popup, NULL);
        XtAddCallback(prop_pop, XtNpopdownCallback, Prop_Popdown, NULL);
	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) prop_pop, "Properties", 
		       OL_DISK_SOURCE, OlFindHelpFile(prop_pop, HELP_PROPERTY));

        footer_text = XtVaCreateManagedWidget(	"text", staticTextWidgetClass,
                      footer_panel,
                      XtNborderWidth, (XtArgVal)0,
                      XtNgravity, (XtArgVal)WestGravity,
                      XtNstring, (XtArgVal)OlGetMessage(XtDisplay(footer_panel),
                                    NULL, 0,
                                    OleNfooterMsg,
                                    OleTnoContents,
                                    OleCOlClientOlpsMsgs,
                                    OleMfooterMsg_noContents,
                                    (XrmDatabase)NULL),
                      (char *)0);

	
	XtRealizeWidget(toplevel);

	cnt = 0;
	XtSetArg(args[cnt], XtNwidth,  &olps_width_limit); cnt++;
	XtGetValues(upper_control, args, cnt);

	olps_height_limit = OlMMToPixel(OL_VERTICAL, MM_OLPS_HEIGHT); /* 100 MM */

        stub_window=XtWindow(stub);
        cnt = 0;
    	XtSetArg(args[cnt], XtNwidth, olps_width_limit);  cnt++;
    	XtSetValues(scrollw, args, cnt);
	
        /* Make the cursors */
        cmap = XDefaultColormap (dpy, screen);
        XAllocNamedColor (dpy, cmap, "white", &d1, &bg);
        XAllocNamedColor (dpy, cmap, "black", &d1, &fg);

#ifdef oldcode
        wait_cursor = XCreatePixmapCursor(dpy,
            XCreateBitmapFromData( dpy, rootwindow,
            wc_bits, wc_width,
            wc_height),
            XCreateBitmapFromData( dpy, rootwindow,
            wc_mask_bits,
            wc_mask_width,
            wc_mask_height),
            &fg, &bg, wc_x_hot, wc_y_hot);
#endif
	wait_cursor = OlGetBusyCursor(toplevel);

        sa_cursor = XCreatePixmapCursor(dpy,
            XCreateBitmapFromData( dpy, rootwindow,
            	(OLconst char *)sa_bits, sa_width, sa_height),
            XCreateBitmapFromData( dpy, rootwindow,
            	(OLconst char *)sa_mask_bits, sa_mask_width, sa_mask_height),
            &fg, &bg, sa_x_hot, sa_y_hot);

        sw_cursor = XCreatePixmapCursor(dpy,
            XCreateBitmapFromData( dpy, rootwindow,
            	(OLconst char *)sw_bits, sw_width, sw_height),
            XCreateBitmapFromData( dpy, rootwindow,
            	(OLconst char *)sw_mask_bits, sw_mask_width, sw_mask_height),
            &fg, &bg, sw_x_hot, sw_y_hot);

#ifdef UseXtApp
	XtAppMainLoop(app_con);
#else
	XtMainLoop();
#endif

#ifdef MITSHM
/*		XShmDetach(dpy, &shminfo);	*/
		shmctl(shminfo.shmid, IPC_RMID, 0);
#endif
}

extern Widget
AddMenu(parent, menu)
Widget parent;
Menu *menu;
{
	Widget	flatmenu;
	Widget  shell;

	shell = parent;
	if (menu->use_popup == True) {
		shell = XtVaCreatePopupShell(
			menu->label,
			popupMenuShellWidgetClass,
			parent,
			XtNpushpin, menu->pushpin,
			(char *) 0
		);
	}

	flatmenu = XtVaCreateManagedWidget ("_menu_",
			flatButtonsWidgetClass,
			shell,
			XtNmenubarBehavior, shell == upper_control,
			XtNlabelJustify, OL_LEFT,
			XtNrecomputeSize, True,
			XtNlayoutType, menu->orientation,
			XtNitemFields,	MenuFields,
			XtNnumItemFields, XtNumber(MenuFields),
			XtNitems, menu->items,
			XtNnumItems, menu->numitems,
			(char *)0
			);
	return(menu->use_popup == True ? shell : flatmenu);
}

#if !defined(USEAPPDEF)
/* Fill in all the labels for the buttons  - the fancy way.
 * SaveFileNoticeItems[]  - 2 - Save_Overwrite, Save_Cancel
 *
 * SaveFileItems[] - 1 -Save_File
 *
 * OpenFileItems[]  - 1 - Open_File
 *
 * PrintFileItems[] - 1 -Print_File
 *
 * FileMenuItems[] - 2 - Open_Pop, Save_Pop
 * PrintMenuItems[] -5 Print_Contents, Print_Window, Print_Area, Print_Screen,
 * 			Printf_Pop
 * CaptureMenuItems[] -3 Capture_Window, Capture_Area, Capture_Screen	
 *
 * MainItems - 4 on the control area - File, Print, Capture Image, Props
 */
static void
FillButtonLabels()
{
Display *display = XtDisplay(toplevel);

#define GET_BUTTONLABEL(T,M) OlGetMessage( display, NULL, 0, OleNbutton,\
					T, OleCOlClientOlpsMsgs, M,\
					(XrmDatabase)NULL)
#define GET_MNEM(T,M) *(OlGetMessage( display, NULL, 0, OleNmnemonic,\
					T, OleCOlClientOlpsMsgs, M,\
					(XrmDatabase)NULL))

	/* Notice popup - has 2 buttons */
	SaveFileNoticeItems[0].label =
			(XtArgVal)GET_BUTTONLABEL(OleToverwrite, OleMbutton_overwrite);
	SaveFileNoticeItems[0].mnemonic =
			(XtArgVal)GET_MNEM(OleToverwrite, OleMmnemonic_overwrite);
	SaveFileNoticeItems[1].label =
		(XtArgVal)GET_BUTTONLABEL(OleTsaveCancelled, OleMbutton_saveCancelled);
	SaveFileNoticeItems[1].mnemonic =
		(XtArgVal)GET_MNEM(OleTsaveCancelled, OleMmnemonic_saveCancelled);

	/* "Save As" File popup */
	SaveFileItems[0].label = 
		(XtArgVal)GET_BUTTONLABEL(OleTlabelSave, OleMbutton_labelSave);
	SaveFileItems[0].mnemonic = 
		(XtArgVal)GET_MNEM(OleTlabelSave, OleMmnemonic_labelSave);
	SaveFileItems[1].label = 
		(XtArgVal)GET_BUTTONLABEL(OleTlabelCancel, OleMbutton_labelCancel);
	SaveFileItems[1].mnemonic = 
		(XtArgVal)GET_MNEM(OleTlabelCancel, OleMmnemonic_labelCancel);

	/* "Open..." File popup */
 	OpenFileItems[0].label =
		(XtArgVal)GET_BUTTONLABEL(OleTlabelOpen, OleMbutton_labelOpen);
 	OpenFileItems[0].mnemonic =
		(XtArgVal)GET_MNEM(OleTlabelOpen, OleMmnemonic_labelOpen);
 	OpenFileItems[1].label =
		(XtArgVal)GET_BUTTONLABEL(OleTlabelCancel, OleMbutton_labelCancel);
 	OpenFileItems[1].mnemonic =
		(XtArgVal)GET_MNEM(OleTlabelCancel, OleMmnemonic_labelCancel);

	/* From image file popup */
 	PrintFileItems[0].label =
		(XtArgVal)GET_BUTTONLABEL(OleTlabelPrint, OleMbutton_labelPrint);
 	PrintFileItems[0].mnemonic =
		(XtArgVal)GET_MNEM(OleTlabelPrint, OleMmnemonic_labelPrint);
 	PrintFileItems[1].label =
		(XtArgVal)GET_BUTTONLABEL(OleTlabelCancel, OleMbutton_labelCancel);
 	PrintFileItems[1].mnemonic =
		(XtArgVal)GET_MNEM(OleTlabelCancel, OleMmnemonic_labelCancel);

 	FileMenuItems[0].label =
		(XtArgVal)GET_BUTTONLABEL(OleTlabelOpen2, OleMbutton_labelOpen2);
 	FileMenuItems[0].mnemonic =
		(XtArgVal)GET_MNEM(OleTlabelOpen2, OleMmnemonic_labelOpen2);
 	FileMenuItems[1].label =
		(XtArgVal)GET_BUTTONLABEL(OleTlabelSave2, OleMbutton_labelSave2);
 	FileMenuItems[1].mnemonic =
		(XtArgVal)GET_MNEM(OleTlabelSave2, OleMmnemonic_labelSave2);
				
 	PrintMenuItems[0].label  =
		(XtArgVal)GET_BUTTONLABEL(OleTlabelContents, OleMbutton_labelContents);
 	PrintMenuItems[0].mnemonic  =
		(XtArgVal)GET_MNEM(OleTlabelContents, OleMmnemonic_labelContents);
 	PrintMenuItems[1].label  =
		(XtArgVal)GET_BUTTONLABEL(OleTlabelWindow, OleMbutton_labelWindow);
 	PrintMenuItems[1].mnemonic  =
		(XtArgVal)GET_MNEM(OleTlabelWindow, OleMmnemonic_labelWindow);
 	PrintMenuItems[2].label  =
		(XtArgVal)GET_BUTTONLABEL(OleTlabelArea, OleMbutton_labelArea);
 	PrintMenuItems[2].mnemonic  =
		(XtArgVal)GET_MNEM(OleTlabelArea, OleMmnemonic_labelArea);
 	PrintMenuItems[3].label  =
		(XtArgVal)GET_BUTTONLABEL(OleTlabelScreen, OleMbutton_labelScreen);
 	PrintMenuItems[3].mnemonic  =
		(XtArgVal)GET_MNEM(OleTlabelScreen, OleMmnemonic_labelScreen);
 	PrintMenuItems[4].label  =
		(XtArgVal)GET_BUTTONLABEL(OleTlabelImageFile, OleMbutton_labelImageFile);
 	PrintMenuItems[4].mnemonic  =
		(XtArgVal)GET_MNEM(OleTlabelImageFile, OleMmnemonic_labelImageFile);

 	CaptureMenuItems[0].label =
		(XtArgVal)GET_BUTTONLABEL(OleTlabelWindow, OleMbutton_labelWindow);
 	CaptureMenuItems[0].mnemonic =
		(XtArgVal)GET_MNEM(OleTlabelWindow, OleMmnemonic_labelWindow);
 	CaptureMenuItems[1].label =
		(XtArgVal)GET_BUTTONLABEL(OleTlabelArea, OleMbutton_labelArea);
 	CaptureMenuItems[1].mnemonic =
		(XtArgVal)GET_MNEM(OleTlabelArea, OleMmnemonic_labelArea);
 	CaptureMenuItems[2].label =
		(XtArgVal)GET_BUTTONLABEL(OleTlabelScreen, OleMbutton_labelScreen);
 	CaptureMenuItems[2].mnemonic =
		(XtArgVal)GET_MNEM(OleTlabelScreen, OleMmnemonic_labelScreen);

 	MainItems[0].label =
		(XtArgVal)GET_BUTTONLABEL(OleTlabelFile, OleMbutton_labelFile);
 	MainItems[0].mnemonic =
		(XtArgVal)GET_MNEM(OleTlabelFile, OleMmnemonic_labelFile);
 	MainItems[1].label =
		(XtArgVal)GET_BUTTONLABEL(OleTlabelPrint, OleMbutton_labelPrint);
 	MainItems[1].mnemonic =
		(XtArgVal)GET_MNEM(OleTlabelPrint, OleMmnemonic_labelPrint);
 	MainItems[2].label =
		(XtArgVal)GET_BUTTONLABEL(OleTlabelCaptureImage,
			 		OleMbutton_labelCaptureImage);
 	MainItems[2].mnemonic =
		(XtArgVal)GET_MNEM(OleTlabelCaptureImage,
			 		OleMmnemonic_labelCaptureImage);
        if (OlGetGui() == OL_OPENLOOK_GUI) {
	  MainItems[3].label =
	    (XtArgVal)GET_BUTTONLABEL(OleTlabelProperties, OleMbutton_labelProperties);
	  MainItems[3].mnemonic =
	    (XtArgVal)GET_MNEM(OleTlabelProperties, OleMmnemonic_labelProperties);
	}
        else {
	  MainItems[3].label =
	    (XtArgVal)GET_BUTTONLABEL(OleTlabelOptions, OleMbutton_labelOptions);
	  MainItems[3].mnemonic =
	    (XtArgVal)GET_MNEM(OleTlabelOptions, OleMmnemonic_labelOptions);
	}
}

#endif


extern void
CreateCommandWindow OLARGLIST((parent, pop, pop_title, pop_name, caption1_label,
				pop_textf, ok_string, pop_ok, button_items,
				numitems))
OLARG( Widget, parent)
OLARG( Widget *, pop)
OLARG( char *, pop_title)
OLARG( char *, pop_name)
OLARG( char *, caption1_label)
OLARG( Widget *, pop_textf)
OLARG( char *, ok_string)
OLARG( Widget *, pop_ok)
OLARG( ButtonItems *, button_items)
OLGRA( int, numitems)

{
Widget	pop_upper,
	pop_lower,
	pop_footer,
	pop_caption1;
	int size;

	*pop = XtVaCreatePopupShell(pop_name,
	    			popupWindowShellWidgetClass, parent,
				XtNtitle, (XtArgVal) pop_title,
				(char *)NULL);

	cnt = 0;
	XtSetArg(args[cnt], XtNupperControlArea, &pop_upper); cnt++;
	XtSetArg(args[cnt], XtNlowerControlArea, &pop_lower); cnt++;
	XtSetArg(args[cnt], XtNfooterPanel, &pop_footer);     cnt++;
	XtGetValues(*pop, args, cnt);

	cnt = 0;
	XtSetArg(args[cnt], XtNtraversalManager, True); cnt++;
	XtSetValues(pop_upper, args, cnt);

	cnt = 0;
	XtSetArg(args[cnt], XtNlabel, caption1_label); 	cnt++;
	XtSetArg(args[cnt], XtNalignment, OL_BOTTOM);	cnt++;
	XtSetArg(args[cnt], XtNborderWidth, 0); 	cnt++;
	pop_caption1 = XtCreateManagedWidget(	"caption",
	    				captionWidgetClass,
	    				pop_upper, args, cnt);

	size = (int) OlMMToPixel(OL_HORIZONTAL, MM4);
	cnt = 0;
	XtSetArg(args[cnt], XtNtraversalOn, (XtArgVal)True);       cnt++;
	XtSetArg(args[cnt], XtNwidth, size);			 cnt++;


	*pop_textf = XtCreateManagedWidget(	"textfield",
				    textFieldWidgetClass,
				    pop_caption1, args, cnt);

	*pop_ok = XtVaCreateManagedWidget(ok_string, flatButtonsWidgetClass,
				pop_lower,
				XtNrecomputeSize, True,
				XtNlayoutType, OL_FIXEDROWS,
				XtNmeasure, 1,
				XtNitemFields, ButtonFields,
				XtNnumItemFields, XtNumber(ButtonFields),
				XtNitems, button_items,
				XtNnumItems, numitems,
				(char *) NULL);
}

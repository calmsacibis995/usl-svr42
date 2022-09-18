/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtadmin:fontmgr/fontmgr.c	1.35"

/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       fontmgr.c
 */

/*
*******************************************************************************
*   Include_files.
*******************************************************************************
*/

#include <stdio.h>
#include <locale.h>
#include <Intrinsic.h>
#include <StringDefs.h>
#include <Shell.h>
#include <OpenLook.h>

#include <OlDnDVCX.h>

#include <Gizmos.h>
#include <BaseWGizmo.h>
#include <PopupGizmo.h>
#include <MenuGizmo.h>
#include <NumericGiz.h>
#include <STextGizmo.h>
#include <LabelGizmo.h>
#include <SpaceGizmo.h>
#include <ChoiceGizm.h>

#include <RubberTile.h>
#include <ControlAre.h>
#include <Panes.h>
#include <Caption.h>
#include <StaticText.h>
#include <TextEdit.h>
#include <FList.h>
#include <ScrolledWi.h>
#include <Footer.h>
#include <Form.h>
#include <IntegerFie.h>

#include <Desktop.h>

#include <fontmgr.h>

/*
*******************************************************************************
*   Application callback declaration (callbacks should be in a separate file)
*******************************************************************************
*/
extern Boolean ParseXLFD(String, xlfd_type **);
extern void HelpCB();
extern void IntegrityCB();
void OutlinePSCB();
void FamilySelectCB();
void LookSelectCB();
void PSSelectCB();
void DisplayBitmapDeleteCB();
void DisplayOutlineDeleteCB();
void FontSetupPropCB();
void AddCB();
static void ApplyDefaultFontCB();
static void ApplyToAllCB();
static void ApplyToXtermCB();
static void ApplyFooterPropCB();
static void ApplySamplePropCB();
void CreateFontDB();
void ResetFont();
void DnDPopupAddWindow();
void MainHelpTOCCB();
void MainHelpAboutCB();
static void ExitCB();
static Boolean	DropNotify OL_ARGS((Widget, Window, Position,
					  Position, Atom, Time,
					  OlDnDDropSiteID,
					  OlDnDTriggerOperation, Boolean,
					  Boolean, XtPointer));
static void	SelectionCB OL_ARGS((Widget, XtPointer, Atom *, Atom *,
					XtPointer, unsigned long *, int *));


#define ALL_FONT_RESOURCE_NAME "font"
#define XTERM_FONT_RESOURCE_NAME "xterm*font"
#define ICON_FILE "font48.icon"
#define ClientClass          "fontmgr"

#define SAVE_MIN_SIZE 10
#define SAVE_MAX_SIZE 20

Widget       app_shellW;		  /* application shell widget       */
Widget base_shell;
Boolean secure = TRUE;
Boolean C_locale;
char *xwin_home;
_OlArenaType(FamilyArena) family_data;
view_type view_data = { &family_data };
view_type *view = &view_data;

static Xterm_set;

static Setting ps_gizmo_setting = {0, 0, 0, (XtPointer) DEFAULT_POINT_SIZE };
NumericGizmo ps_gizmo =
{0, 0, 0, MIN_PS_VALUE, MAX_PS_VALUE, &ps_gizmo_setting, 4};

static MenuItems delete_menu_item[] = {
{ TRUE, TXT_BITMAP_DDD, ACCEL_FILE_DELETE_BITMAP , 0, DisplayBitmapDeleteCB},
{ TRUE, TXT_OUTLINE_DDD,ACCEL_FILE_DELETE_OUTLINE,0, DisplayOutlineDeleteCB},
{ NULL }
};

static MenuGizmo delete_menu = {0, "d_menu", NULL, delete_menu_item};

#define ADD_BUT 0
#define DELETE_BUT 1
#define INTEGRITY_BUT 2
static MenuItems file_menu_item[] = {
{ TRUE, TXT_ADD_DDD,   ACCEL_FILE_ADD      , 0, AddCB},
{ TRUE, TXT_DELETE,    ACCEL_FILE_DELETE   , (char *) &delete_menu},
{ TRUE, TXT_INTEGRITY, ACCEL_FILE_INTEGRITY, 0, IntegrityCB},
{ TRUE, TXT_EXIT,      ACCEL_FILE_EXIT     , 0, ExitCB},
{ NULL }
};

static MenuItems font_name_menu_item[] = {
{ TRUE,TXT_SHORT_NAME,ACCEL_VIEW_FONT_SHORT,0,ApplyFooterPropCB,(char*)FALSE},
{ TRUE, TXT_XLFD_NAME,ACCEL_VIEW_FONT_XLFD ,0, ApplyFooterPropCB, (char*)TRUE  },
{ NULL }
};

enum sample_constant { E_SHORT_SAMPLE, E_FULL_SAMPLE };
static MenuItems sample_menu_item[] = {
{ TRUE, TXT_SHORT_SAMPLE, ACCEL_VIEW_DISPLAY_PHASE, 0,
      ApplySamplePropCB, (char *)E_SHORT_SAMPLE},
{ TRUE, TXT_FULL_SAMPLE, ACCEL_VIEW_DISPLAY_CHAR ,0,
      ApplySamplePropCB, (char *)E_FULL_SAMPLE},
{ NULL }
};

static MenuGizmo font_name_menu = {0, "fm", NULL, font_name_menu_item};
static MenuGizmo sample_menu = {0, "sm", NULL, sample_menu_item};

static MenuItems view_menu_item[] = {  
{ TRUE, TXT_FONT_NAME_MENU_LABEL, ACCEL_VIEW_FONT, (char *) &font_name_menu },
{ TRUE, TXT_SAMPLE_MENU_LABEL,    ACCEL_VIEW_DISPLAY, (char *) &sample_menu },
{ NULL }
};

MenuItems base_prop_menu_item[] = {  
{ TRUE, TXT_GENERAL, 0,0, FontSetupPropCB, (char *) E_GENERAL_PROP },
{ TRUE, TXT_ATM    , 0,0, FontSetupPropCB, (char *) E_ATM_PROP },
{ TRUE, TXT_TYPE_SCALER, 0,0, FontSetupPropCB, (char *) E_FOLIO_PROP },
{ TRUE, TXT_SPEEDO, 0,0, FontSetupPropCB, (char *) E_SPEEDO_PROP },
{ NULL }
};

static MenuGizmo prop_menu = {0, "pm", NULL, base_prop_menu_item};

static MenuItems edit_menu_item[] = {  
#ifdef old
{ TRUE, TXT_PROPERTIES, 'P', (char *) &prop_menu },
#endif
{ TRUE, TXT_APPLY_DEFAULT_FONT, ACCEL_EDIT_DEFAULT,0,
      ApplyDefaultFontCB, (char *) &view_data},
{ TRUE, TXT_APPLY_FONT_ALL,ACCEL_EDIT_ALL,0,
      ApplyToAllCB, (char *) &view_data},
{ TRUE, TXT_APPLY_FONT_XTERM,ACCEL_EDIT_TERMINAL,0,
      ApplyToXtermCB, (char *) &view_data},
{ NULL }
};

static HelpInfo help_about = { 0, "", HELP_PATH, TXT_MAIN_HELP_SECTION };
static HelpInfo help_TOC = { 0, "", HELP_PATH, "TOC" };
static HelpInfo help_desk = { 0, "", HELP_PATH, "HelpDesk" };
static HelpInfo help_main_menu = { 0, "", HELP_PATH, TXT_HELP_MAIN_MENU };

static MenuItems help_menu_item[] = {  
{ TRUE, TXT_HELP_ABOUT,ACCEL_HELP_FONT ,0, HelpCB, (char *) &help_about },
{ TRUE, TXT_HELP_TOC,  ACCEL_HELP_TABLE,0, HelpCB, (char *) &help_TOC },
{ TRUE, TXT_HELP_DESK, ACCEL_HELP_DESK ,0, HelpCB, (char *) &help_desk },
{ NULL }
};

static MenuGizmo file_menu = {0, "file_menu", NULL, file_menu_item};
static MenuGizmo view_menu = {0, NULL, NULL, view_menu_item};
static MenuGizmo edit_menu = {0, NULL, NULL, edit_menu_item};
static MenuGizmo help_menu = {0, "help_menu", NULL, help_menu_item};

#define EDIT_BUT 2
static MenuItems main_menu_item[] = {  
{ TRUE, TXT_FILE, ACCEL_BASE_FILE, (Gizmo) &file_menu},
{ TRUE, TXT_VIEW, ACCEL_BASE_VIEW, (Gizmo) &view_menu},
{ TRUE, TXT_EDIT, ACCEL_BASE_EDIT, (Gizmo) &edit_menu},
{ TRUE, TXT_HELP, ACCEL_BASE_HELP, (Gizmo) &help_menu},
{ NULL }
};
static MenuGizmo menu_bar = { &help_main_menu, "menu_bar", NULL,
    main_menu_item, 0, 0, 0, 0, 0, OL_NO_ITEM };

static MenuItems font_save_item[] = {
{ True, "1", "1" },
{ 0 }
};
static MenuGizmo   font_save_menu    = 
{ NULL, NULL, "_X_", font_save_item, NULL, NULL, EXC };

static Setting font_save_setting;
static ChoiceGizmo font_save_choice   = 
   { NULL, NULL, " ",  &font_save_menu, &font_save_setting };
static GizmoRec view_list[] = {
      { ChoiceGizmoClass,  &font_save_choice }
};

BaseWindowGizmo base = {0, "base", TXT_WINDOW_TITLE, (Gizmo)&menu_bar,
	view_list,
	XtNumber(view_list),
	TXT_ICON_TITLE, /* icon title */
	ICON_FILE,
	" ", " ", 90
    };


static void
ApplyFooterPropCB(Widget w,
		  XtPointer client_data,
		  XtPointer call_data)
{
    extern Boolean show_xlfd;

    show_xlfd = (Boolean) client_data;
    DisplayFont(view);

} /* end of ApplyFooterPropCB */


static void
ApplySamplePropCB(Widget w,
		  XtPointer client_data,
		  XtPointer call_data)
{
    if (E_SHORT_SAMPLE == (int)client_data)
	XtVaSetValues(view->sample_text,
		      XtNcursorPosition, 0,
		      XtNselectStart, 0,
		      XtNselectEnd, 0,
		      XtNsource, GetGizmoText(TXT_SAMPLE_TEXT), 0);
    else
	XtVaSetValues(view->sample_text,
		      XtNcursorPosition, 0,
		      XtNselectStart, 0,
		      XtNselectEnd, 0,
		      XtNsource, GetGizmoText(TXT_CHAR_SET), 0);

} /* end of ApplySamplePropCB */


static void
ApplyDefaultFontCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    /*
     * do dynamic update
     */
    font_save_choice.name = ALL_FONT_RESOURCE_NAME;
    font_save_menu.items[0].set = TRUE;
    font_save_menu.items[0].mod.resource_value = "olDefaultFont";
    ManipulateGizmo(ChoiceGizmoClass, &font_save_choice, SetGizmoValue);

    font_save_choice.name = XTERM_FONT_RESOURCE_NAME;
    font_save_menu.items[0].set = TRUE;
    font_save_menu.items[0].mod.resource_value = NlucidaTypewriter;
    ManipulateGizmo(ChoiceGizmoClass, &font_save_choice, SetGizmoValue);

    Xterm_set = TRUE;
    InformUser(TXT_APPLIED_DEFAULT_FONT_TO_ALL);

} /* end of ApplyDefaultFontCB */


void
ApplyToAllCB(w, client_data, call_data)
    Widget w;
    XtPointer client_data;
    XtPointer call_data;
{
    view_type *view = (view_type *) client_data;
    xlfd_type *xlfd_info;

#define POINT_SIZE_IN_RANGE(P) ((atoi(P->size) >= SAVE_MIN_SIZE) && \
				(atoi(P->size) <= SAVE_MAX_SIZE))
    ParseXLFD(view->cur_xlfd, &xlfd_info);
    if (!POINT_SIZE_IN_RANGE(xlfd_info)) {
	InformUser(TXT_POINT_SIZE_NOT_IN_RANGE);
	return;
    };

    font_save_choice.name = ALL_FONT_RESOURCE_NAME;
    font_save_menu.items[0].set = TRUE;
    font_save_menu.items[0].mod.resource_value = view->cur_xlfd;

    ManipulateGizmo(ChoiceGizmoClass, &font_save_choice, SetGizmoValue);

    /*
     * set xterm font resource
     */
    if (!Xterm_set) {
	font_save_choice.name = XTERM_FONT_RESOURCE_NAME;
	font_save_menu.items[0].set = TRUE;
	font_save_menu.items[0].mod.resource_value = NlucidaTypewriter;
	ManipulateGizmo(ChoiceGizmoClass, &font_save_choice, SetGizmoValue);
	Xterm_set = TRUE;
    }
    InformUser(TXT_APPLIED_FONT_TO_ALL);

} /* end of ApplyToAllCB */


void
ApplyToXtermCB(w, client_data, call_data)
    Widget w;
    XtPointer client_data;
    XtPointer call_data;
{
    view_type *view = (view_type *) client_data;
    xlfd_type *xlfd_info;

    ParseXLFD(view->cur_xlfd, &xlfd_info);
    if (!POINT_SIZE_IN_RANGE(xlfd_info)) {
	InformUser(TXT_POINT_SIZE_NOT_IN_RANGE);
	return;
    };

/*
 * notice: spacing could be '*'
 */
#define CHARACTER_SPACING 'c'
#define MONO_SPACING 'm'
    if ((tolower(xlfd_info->spacing[0]) == CHARACTER_SPACING) ||
	(tolower(xlfd_info->spacing[0]) == MONO_SPACING)) {
	font_save_choice.name = XTERM_FONT_RESOURCE_NAME;
	font_save_menu.items[0].set = TRUE;
	font_save_menu.items[0].mod.resource_value = view->cur_xlfd;
	ManipulateGizmo(ChoiceGizmoClass, &font_save_choice, SetGizmoValue);
	Xterm_set = TRUE;
	InformUser(TXT_APPLIED_FONT_TO_XTERM);
    }
    else
	InformUser(TXT_BAD_XTERM_FONT);

} /* end of ApplyToXtermCB */


static void
ExitCB() 
{
    exit(0);
}


static Widget
CreateCaption(Widget parent, Widget ref, String str)
{
    Arg largs[15];
    Cardinal n;
    Widget rubber;

    n = 0;
    if (parent != ref) {
	XtSetArg(largs[n], XtNrefWidget, ref);			n++;
    }
    XtSetArg(largs[n], XtNrefPosition, OL_RIGHT);		n++;
    XtSetArg(largs[n], XtNweight, 0);         n++;
    XtSetArg(largs[n], XtNalignment, OL_CENTER);		n++;
    XtSetArg(largs[n], XtNstring, str);			n++;
    XtSetArg(largs[n], XtNwrap, FALSE);			n++;
    rubber = XtCreateManagedWidget("caption", 
				   staticTextWidgetClass, parent, largs, n);
    return rubber;
} /* end of CreateCaption */


static void
CreateMainView(view)
    view_type *view;
{
    Widget parent = view->form;
    Widget look_box;
    Widget scroll_win, box_parent, family_sw, look_sw, size_label;
    Arg largs[15];
    Cardinal n;
    static String dummy[] = {" ", NULL};
    static String item_fields[] = {XtNlabel, XtNuserData};

    n = 0;
    XtSetArg(largs[n], XtNshadowThickness, 0);		n++;
    XtSetArg(largs[n], XtNlayoutHeight, OL_IGNORE);		n++;
    box_parent = XtCreateManagedWidget("box_parent", 
	 panesWidgetClass, parent, largs, n);

    n = 0;
    XtSetArg(largs[n], XtNrefPosition, OL_BOTTOM);			n++;
    scroll_win = XtCreateManagedWidget("ts",
          scrolledWindowWidgetClass, parent, largs, n);
    n = 0;
    XtSetArg(largs[n], XtNwrapMode, OL_WRAP_ANY);		n++;
    XtSetArg(largs[n], XtNsource, GetGizmoText(TXT_SAMPLE_TEXT));	n++;
    XtSetArg(largs[n], XtNlinesVisible, 7);	n++;
    XtSetArg(largs[n], XtNcharsVisible, 30);	n++;
    XtSetArg(largs[n], XtNcontrolCaret, False);	n++;

    /*
     * fontGroup needs to be rethinked
     */
    XtSetArg(largs[n], XtNfontGroup, NULL);	n++;
    view->sample_text = XtCreateManagedWidget("sample",
		textEditWidgetClass, scroll_win, largs, n);

    /*
     * create labels
     */
    view->upper = CreateCaption(box_parent, box_parent,
				GetGizmoText(TXT_TYPEFACE_FAMILY));
    look_box = CreateCaption( box_parent,view->upper,GetGizmoText(TXT_STYLE));
    size_label = CreateCaption(box_parent, look_box, GetGizmoText(TXT_POINT_SIZE));

    /*
     * create family
     */
    n = 0;
    XtSetArg(largs[n], XtNrefWidget, view->upper);			n++;
    family_sw = XtCreateManagedWidget("scroll_win",
	 scrolledWindowWidgetClass, box_parent, largs, n);
    n = 0;
    XtSetArg(largs[n], XtNitems, dummy);		n++;
    XtSetArg(largs[n], XtNnumItems, 1);	n++;
    XtSetArg(largs[n], XtNitemFields, item_fields);		n++;
    XtSetArg(largs[n], XtNnumItemFields, XtNumber(item_fields));	n++;
    XtSetArg(largs[n], XtNselectProc, FamilySelectCB);	n++;
    XtSetArg(largs[n], XtNclientData, view);			n++;
    XtSetArg(largs[n], XtNmaintainView, TRUE);			n++;
    view->family_exclusive = XtCreateManagedWidget("families",
		flatListWidgetClass, family_sw, largs, n);

    /*
     * create look
     */
    n = 0;
    XtSetArg(largs[n], XtNrefWidget, look_box);			n++;
    look_sw = XtCreateManagedWidget("look_sw",
          scrolledWindowWidgetClass, box_parent, largs, n);
    n = 0;
    XtSetArg(largs[n], XtNitems, dummy);		n++;
    XtSetArg(largs[n], XtNnumItems, 1);	n++;
    XtSetArg(largs[n], XtNitemFields, item_fields);	n++;
    XtSetArg(largs[n], XtNnumItemFields, XtNumber(item_fields));	n++;
    XtSetArg(largs[n], XtNsameWidth, OL_ALL);	n++;
    XtSetArg(largs[n], XtNlabelJustify, OL_CENTER);	n++;
    XtSetArg(largs[n], XtNselectProc, LookSelectCB);	n++;
    XtSetArg(largs[n], XtNclientData, view);			n++;
    XtSetArg(largs[n], XtNuserData, view->font_state[LOOK]);n++;
    XtSetArg(largs[n], XtNmaintainView, TRUE);			n++;
    view->look_exclusive = XtCreateManagedWidget("looks",
		flatListWidgetClass, look_sw, largs, n);

    /*
     * create point size
     */
    n = 0;
    XtSetArg(largs[n], XtNrefWidget, size_label);		n++;
    view->size_window = XtCreateManagedWidget("sw",
          scrolledWindowWidgetClass, box_parent, largs, n);
    n = 0;
    XtSetArg(largs[n], XtNitems, dummy);		n++;
    XtSetArg(largs[n], XtNnumItems, 1);	n++;
    XtSetArg(largs[n], XtNitemFields, item_fields);		n++;
    XtSetArg(largs[n], XtNnumItemFields, XtNumber(item_fields)); n++;
    XtSetArg(largs[n], XtNsameWidth, OL_ALL);	n++;
    XtSetArg(largs[n], XtNlabelJustify, OL_CENTER);	n++;
    XtSetArg(largs[n], XtNselectProc, PSSelectCB);	n++;
    XtSetArg(largs[n], XtNclientData, view);		n++;
    XtSetArg(largs[n], XtNuserData, view->font_state[POINT]);n++;
    XtSetArg(largs[n], XtNmaintainView, TRUE);			n++;
    view->size_exclusive = XtCreateManagedWidget("sizes",
		flatListWidgetClass, view->size_window, largs, n);

    view->cur_size = atoi(DEFAULT_POINT_SIZE);
    n = 0;
    XtSetArg(largs[n], XtNmaximumSize, 3);	n++;
    XtSetArg(largs[n], XtNvalueMin, MIN_PS_VALUE);		n++;
    XtSetArg(largs[n], XtNvalueMax, MAX_PS_VALUE);		n++;
    XtSetArg(largs[n], XtNvalue, view->cur_size);		n++;
    XtSetArg(largs[n], XtNrefWidget, size_label);	n++;
    XtSetArg(largs[n], XtNtype, OL_DECORATION);         n++;
    XtSetArg(largs[n], XtNweight, 0);         n++;
    view->ps_text = XtCreateManagedWidget("os", integerFieldWidgetClass,
				   box_parent, largs, n);
    XtAddCallback(view->ps_text, XtNvalueChanged, OutlinePSCB, view);

} /* end of CreateMainView */


static void
CreateStatusArea(view, parent)
    view_type *view;
    Widget parent;
{
    Arg largs[10];
    int n;

    n = 0;
    XtSetArg(largs[n], XtNleftFoot,    base.error); n++;
    XtSetArg(largs[n], XtNrightFoot,   base.status); n++;
    XtSetArg(largs[n], XtNleftWeight,  90);   n++;
    XtSetArg(largs[n], XtNrightWeight, 10); n++;
    XtSetArg(largs[n], XtNweight, 0);       n++;
    view->footer_text = XtCreateManagedWidget (
      "footer", footerWidgetClass, parent, largs, n   );

} /* end of CreateStatusArea */


void
UpdateMainView()
{
    DeleteFontDB( view_data.family_arena);

    /* fill database with font information */
    CreateFontDB(view_data.family_arena);

    /*  Initialize the state of the exclusives for the Reset button.  */
    ResetFont(&view_data);
} /* end of UpdateMainView */


int
MyXErrorHandler(Display *display, XErrorEvent *err)
{
    char msg[MAX_STRING];

    XGetErrorText(display, err->error_code, msg, sizeof(msg));
    fprintf(stderr, "fontsetup: X Error code %s\n", msg);

} /* end of MyXErrorHandler */


/*
 * search for X resource in .Xdefaults, return TRUE if found
 */
static Boolean
CheckResource(String res)
{
    FILE *file;
    int key_len = strlen(res);
    char buf[MAX_PATH_STRING];
    Boolean found = FALSE;

    sprintf(buf, "%s/.Xdefaults", getenv("HOME"));
    file = fopen(buf, "r");
    if (FileOK(file)) {
	while (fgets(buf, MAX_PATH_STRING, file) != NULL) {
	    if (strncmp(buf, res, key_len) == STR_MATCH) {
		found = TRUE;
		break;
	    }
	}
	fclose(file);
    }
    return found;
}


/* 
*******************************************************************************
*   MAIN function
*******************************************************************************
*/

main ( argc, argv )
int    argc;
char **argv;
{   
    int n;
    Widget deepest;
    Arg largs[10];
    int c;
    extern char *optarg;
    String device = NULL;
    String display = (String) getenv("DISPLAY");
    String language;

    OlToolkitInitialize(&argc, argv, (XtPointer)NULL);
    app_shellW = XtInitialize ( ClientName, ClientClass,NULL,0,&argc, argv );
    DtInitialize(app_shellW);

    XSetErrorHandler(MyXErrorHandler);

    while ((c = getopt(argc, argv, "?hvpf:")) != EOF)
	switch (c) {
	case '?':
	case 'h':
	    fprintf(stderr, "usage: fontmgr [-vp] [-f device]\n");
	    exit (2);
	    break;
	case 'p':
	    secure = FALSE;
	    break;
	case 'v':
	    printf("UNIX System V Release 4.2 P14\n\
compiled with Openlook v%d, Xt v%d\n",
		   OL_VERSION, XtSpecificationRelease);
	    exit (0);
	    break;
	case 'f':
	    device = optarg;
	    break;
	}

    if (display) {
	if ((*display != ':') && (strncmp(display, "unix:", 5)!=STR_MATCH)) 
	    secure = TRUE;
    }
    else
	secure = TRUE;

    xwin_home = getenv("XWINHOME");
    if (xwin_home)
	xwin_home = XtNewString(xwin_home);
    else
	xwin_home = "/usr/X";

    language = (String) setlocale(LC_MESSAGES, NULL);
    if (language && (strcmp(language,"C")!=STR_MATCH))
	C_locale = FALSE;
    else
	C_locale = TRUE;

    Xterm_set = CheckResource(XTERM_FONT_RESOURCE_NAME);

    file_menu_item[ADD_BUT].sensitive = !secure; 
    file_menu_item[DELETE_BUT].sensitive = !secure; 
    file_menu_item[INTEGRITY_BUT].sensitive = !secure; 
    main_menu_item[EDIT_BUT].sensitive = C_locale; 

    base.title = GetGizmoText(base.title);
    base.icon_name = GetGizmoText(base.icon_name);
    base_shell = CreateGizmo(app_shellW, BaseWindowGizmoClass, &base, NULL, 0);
    deepest = base.form;
    XtVaSetValues(deepest, XtNshadowThickness, 0, NULL);
    XtVaSetValues(menu_bar.child, XtNshadowThickness, 2, NULL);

    view_data.form = XtVaCreateManagedWidget("basePane", 
	 panesWidgetClass, deepest, XtNshadowThickness, 0, NULL);
    CreateMainView(&view_data);
    CreateStatusArea(&view_data, deepest);

    /* fill database with font information */
    CreateFontDB( view_data.family_arena);
    ResetToDefaultFont( &view_data);
    ResetFont( &view_data);

    /* this gizmo is for saving to .Xdefaults */
    XtUnmanageChild(font_save_choice.captionWidget);

    XtUnmanageChild(base.message);
    MapGizmo(BaseWindowGizmoClass, &base);
    XtDestroyWidget(base.message);

    if (device) {
	DnDPopupAddWindow (device);
    }

    OlDnDRegisterDDI(view_data.form, OlDnDSitePreviewNone, 
		     DropNotify,
		     (OlDnDPMNotifyProc)NULL, True, NULL);
    OlDnDRegisterDDI(base.icon_shell, OlDnDSitePreviewNone, 
		     DropNotify,
		     (OlDnDPMNotifyProc)NULL, True, NULL);
    XtMainLoop  ( );
}


static Boolean
DropNotify OLARGLIST((w, win, x, y, selection, timestamp,
			    drop_site_id, op, send_done, forwarded, closure))
  OLARG( Widget,		w)
  OLARG( Window,		win)
  OLARG( Position,		x)
  OLARG( Position,		y)
  OLARG( Atom,			selection)
  OLARG( Time,			timestamp)
  OLARG( OlDnDDropSiteID,	drop_site_id)
  OLARG( OlDnDTriggerOperation,	op)
  OLARG( Boolean,		send_done)
  OLARG( Boolean,		forwarded)
  OLGRA( XtPointer,		closure)
{
    XtGetSelectionValue(
			w, selection, OL_XA_FILE_NAME(XtDisplay(w)),
			SelectionCB, (XtPointer) send_done, timestamp
			);

    return(True);
} /* end of DropNotify */


static void
SelectionCB OLARGLIST ((w, client_data, selection, type, value, length,
			   format))
  OLARG( Widget,		w)
  OLARG( XtPointer,		client_data)
  OLARG( Atom *,		selection)
  OLARG( Atom *,		type)
  OLARG( XtPointer,		value)
  OLARG( unsigned long *,	length)
  OLGRA( int *,			format)
{
    Boolean send_done = (Boolean)client_data;
    String fullname;

    /* Since only OL_XA_FILE_NAME(dpy) is passed in, we know we have a
       valid type.
       */

    fullname = (String) value;
    if (fullname && *fullname) {
	XtMapWidget(base_shell);
	DnDPopupAddWindow( fullname);
    }

    /* We don't care if there was an error. 
       The transaction is done regardless. */

    XtFree(value);

    if (send_done == True) {
	OlDnDDragNDropDone(w, *selection, CurrentTime, NULL, NULL);
    }
} /* end of SelectionCB */



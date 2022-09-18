/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtadmin:fontmgr/font_del.c	1.14"

/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       font_del.c
 */

#include <stdio.h>
#include <Intrinsic.h>
#include <Shell.h>

#include <OpenLook.h>

#include <Gizmos.h>
#include <BaseWGizmo.h>
#include <PopupGizmo.h>
#include <MenuGizmo.h>
#include <ModalGizmo.h>

#include <ScrolledWi.h>
#include <FList.h>
#include <fontmgr.h>


/*
 * external data
 */
extern Widget       app_shellW;		  /* application shell widget       */
extern ModalGizmo prompt;
extern char *xwin_home;
_OlArenaType(FamilyArena) family_data;

extern String LowercaseStr(String);
extern void HelpCB();
static void DeleteCancelCB();
static void ConfirmCancelCB();
static void DoDeleteCB();
static void ApplyDeleteCB();
static void ResetDeleteCB();
static string_array_type _font_name, _xlfd, _selected_xlfd, _file_name;
static string_array_type _selected_dir;
static delete_type delete_info = { &_font_name, &_xlfd, &_selected_xlfd,
				   &_file_name, &_selected_dir};

static HelpInfo help_delete = { 0, "", HELP_PATH };
static HelpInfo help_confirm_delete = { 0, "", HELP_PATH };

static MenuItems delete_menu_item[] = {  
{ TRUE,TXT_DELETE_APPLY, ACCEL_DELETE_APPLY ,0, ApplyDeleteCB, (char*)&delete_info },
{ TRUE,TXT_RESET, ACCEL_DELETE_RESET ,0, ResetDeleteCB, (char*)&delete_info},
{ TRUE,TXT_CANCEL,ACCEL_DELETE_CANCEL,0, DeleteCancelCB, (char*)&delete_info },
{ TRUE,TXT_HELP_DDD,  ACCEL_DELETE_HELP  ,0, HelpCB, (char *)&help_delete },
{ NULL }
};
static MenuGizmo delete_menu = {0, "dm", "dm", delete_menu_item,
			 0, 0};
static PopupGizmo delete_popup = {0, "dp", TXT_FONT_DELETE, (Gizmo)&delete_menu };

static char *system_font[]= { 
    "-lucida-",
    "-lucidatypewriter-medium-r-",
    "-helvetica-",
    "-courier-medium-r-",
    "-open look cursor-",
    "-open look glyph-",
    "-clean-",
    "-fixed-"
};


/*
 * return the length of the string that includes 'num' of 'char'
 */
static int
strnchar(String str,char ch, int num)
{
    String p = str;

    for ( ; *p; p++) {
	if (*p == ch)
	    num--;
	if (num <= 0)
	    break;
    }
    return p - str;
}


/*
 * returns TRUE if a match is found
 * returns FALSE otherwise
 */
static Boolean
MatchFileWithFont(info, font_path, file_name, font_name)
     delete_type *info;
     char *font_path;
     char *file_name;
     char *font_name;
{
    int i, j, str_len;
    char full_path_name[MAX_PATH_STRING];
    char body_name[MAX_STRING];

    for (i=0; i<info->selected_xlfd->n_strs; i++) {
	if (info->bitmap)
	    str_len = strlen(font_name);
	else
	    str_len = strnchar(font_name, DELIM, 7);
	if (strncmp( LowercaseStr(font_name), info->selected_xlfd->strs[i],
		    str_len) == STR_MATCH) {
	    sprintf(full_path_name, "%s/%s", font_path, file_name);
	    InsertStringDB(info->file_name, full_path_name);

	    /* if outline font then delete afm file and pfb file */
	    if (!info->bitmap) {
		sscanf( file_name, "%[^.]", body_name);
		sprintf(full_path_name, "%s/%s/%s.afm", font_path,
			AFM_DIR, body_name);
		InsertStringDB(info->file_name, full_path_name);
		sprintf(full_path_name, "%s/%s.pfb", font_path, body_name);
		InsertStringDB(info->file_name, full_path_name);
	    }

	    /* keep a record of modified directories */
	    for( j=0; j<info->selected_dir->n_strs; j++) {
		if (strcmp(info->selected_dir->strs[j],font_path) == STR_MATCH)
		    break;
	    }
	    if (j >= info->selected_dir->n_strs)
		InsertStringDB(info->selected_dir, font_path);
	    
	    return TRUE;
	}
    }
    return FALSE;

} /* end of MatchFileWithFont */


static void
DeleteFontFiles(delete_type *info)
{
    int i;

    /* remove the font files from the disk */
    for(i=0; i<info->file_name->n_strs; i++) {
	unlink( info->file_name->strs[i]);
    }
} /* DeleteFontFiles */


/*
 * insert the family names that the user selected into an array
 */
static void
GetSelectedFonts(info)
    delete_type *info;
{
    Boolean selected;
    int i;

    for(i= info->font_name->n_strs - 1; i>=0; i--) {
	OlVaFlatGetValues(info->font_list, i, XtNset, &selected, 0);
	if (selected) {
	    InsertStringDB(info->selected_xlfd, info->xlfd->strs[i]);
	}
    }
} /* GetSelectedFonts */


/*
 * update the fonts.dir, the server, and us
 */
UpdateFonts( font_path, n_paths, bitmap)
    char **font_path;
    int n_paths;
    Boolean bitmap;
{
    char sys_cmd[MAX_PATH_STRING*4];
    int i;
    String derived_ps;

    /* update fonts.scale */
    if (!bitmap) {
	derived_ps = (String) GetDerivedPS();
	sprintf(sys_cmd, "DERIVED_INSTANCE_PS='%s' %s/bin/mkfontscale",
		derived_ps ? derived_ps : "", xwin_home	);
	for (i=0; i<n_paths; i++) {
	    /* only mkfontscale TYPE1 fonts */
	    if (strstr(font_path[i], "ype1")) {
		strcat(sys_cmd, " ");
		strcat(sys_cmd, font_path[i]);
	    }
	}
	system(sys_cmd);

	/* Becase of a bug in mkfontdir we have to do this.
	   The bug is, if fonts.scale has no xlfd entry,
	   mkfontdir will not update fonts.dir */
	for (i=0; i<n_paths; i++) {
	    sprintf(sys_cmd,
		    "/usr/bin/cp %s/fonts.scale %s/fonts.dir 2>/dev/null",
		    font_path[i], font_path[i]);
	    system(sys_cmd);
	}
    }

    /* perform mkfontdir */
    sprintf(sys_cmd, "%s/bin/mkfontdir", xwin_home);
    for (i=0; i<n_paths; i++) {
	strcat(sys_cmd, " ");
	strcat(sys_cmd, font_path[i]);
    }
    system(sys_cmd);

    sprintf(sys_cmd, "%s/bin/xset fp rehash", xwin_home);
    system(sys_cmd);

    UpdateMainView();

} /* end of UpdateFonts */

	
static void
DoDelete(delete_info)
    delete_type *delete_info;
{
    int i, read_count, fonts_in_dir;
    char **font_path;
    int n_paths;
    char dir_filename[MAX_PATH_STRING], file_name[MAX_STRING];
    char font_name[MAX_PATH_STRING];
    FILE *dir_file;

    BusyCursor(0);
    font_path = XGetFontPath(XtDisplay(delete_info->popup), &n_paths);
    for (i=0; i<n_paths; i++) {
	strcpy(dir_filename, font_path[i]);
	strcat(dir_filename, "/fonts.dir");
	dir_file = fopen(dir_filename, "r");
	if (!FileOK(dir_file))
	    continue;
	read_count = fscanf(dir_file, "%d\n", &fonts_in_dir);
	if ((read_count == EOF) || (read_count != 1)) {
	    fclose(dir_file);
	    continue;
	}
	while ((read_count = fscanf(dir_file, "%s %[^\n]\n", file_name,
				    font_name)) != EOF) {
	    if (read_count != 2) {
		break;
	    }
	    MatchFileWithFont(delete_info,
				  font_path[i], file_name, font_name);
	}
	fclose(dir_file);
    } /* for i */

    DeleteFontFiles(delete_info);
    UpdateFonts( delete_info->selected_dir->strs,
		delete_info->selected_dir->n_strs,
		delete_info->bitmap);
    XFreeFontPath(font_path);
    StandardCursor(0);
    InformUser(TXT_DELETE_FINISH);

} /* end of DoDelete */


static void
DoDeleteCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    delete_type *info = (delete_type *) client_data;

    XtPopdown( info->popup);
    XtPopdown( prompt.shell);
    InformUser(TXT_DELETE_START);
    ScheduleWork( DoDelete, info, 9);

} /* end of DoDeleteCB */


static void CreateDeletePopup(w, info, bitmap, title)
    Widget w;
    delete_type *info;
    Boolean bitmap;
    String title;
{
    static String item_fields[] = {XtNlabel};
    Widget scroll_win, upper;
    int n;
    Arg largs[20];
    
    
    if (info->popup == NULL) {
	/* create the popup */
	info->popup = CreateGizmo(w, PopupGizmoClass,
			      &delete_popup, NULL, 0);

	XtVaGetValues( info->popup, XtNupperControlArea, &upper, 0);

	/*  Create the controls in the upper control area.  */
	n = 0;
	scroll_win = XtCreateManagedWidget("scroll_win",
		 scrolledWindowWidgetClass, upper, largs, n);

	n = 0;
	XtSetArg(largs[n], XtNviewHeight, NUM_FAMILY_ITEMS);    n++; 
	XtSetArg(largs[n], XtNnumItems, info->font_name->n_strs);	n++;
	XtSetArg(largs[n], XtNitems, info->font_name->strs);		n++;
	XtSetArg(largs[n], XtNitemFields, item_fields);		 n++;
	XtSetArg(largs[n], XtNnumItemFields, XtNumber(item_fields)); n++;
	XtSetArg(largs[n], XtNexclusives, FALSE);		n++;
	info->font_list = XtCreateManagedWidget("families",
        flatListWidgetClass, scroll_win, largs, n);	
    }
    else {
	/* reset any previous selections */
	XtVaSetValues( info->font_list, XtNnumItems, 0, 0);

	n = 0;
	/* due to a bug in the flat list widget that causes the
	   scroll window to get larger, the XtNviewHeight has
	   to be specified */
	XtSetArg(largs[n], XtNviewHeight, NUM_FAMILY_ITEMS);    n++; 
	XtSetArg(largs[n], XtNnumItems, info->font_name->n_strs);	n++;
	XtSetArg(largs[n], XtNitems, info->font_name->strs);		n++;
	XtSetValues( info->font_list, largs, n);
    }

    XtVaSetValues(delete_popup.shell, XtNtitle, GetGizmoText(title), 0);
    MapGizmo(PopupGizmoClass, &delete_popup);

} /* end of CreateDeletePopup */


static Boolean
SystemFont( String xlfd)
{
    int i;

    for (i=0; i<XtNumber(system_font); i++) {
	if (strstr(xlfd, system_font[i]) != NULL)
	    return TRUE;
    }
    return FALSE;

} /* end of SystemFont */


static void 
DisplayDeletePopup( w, delete_info, title)
    Widget w;
    delete_type *delete_info;
    String title;
{
    Boolean bitmap = delete_info->bitmap;
    int i,j,k;
    _OlArenaType(FamilyArena) *family_arena = &family_data;
    _OlArenaType(LookArena) * look_arena;
    _OlArenaType(PSArena) * ps_arena;
    font_type *font_info;
    char font_name[MAX_PATH_STRING];

    /* get and display font list */
    DeleteStringsDB( delete_info->font_name);
    DeleteStringsDB( delete_info->xlfd);
    DeleteStringsDB( delete_info->selected_xlfd);
    DeleteStringsDB( delete_info->file_name);
    DeleteStringsDB( delete_info->selected_dir);
    for(i=0; i<_OlArenaSize(family_arena); i++) {
	look_arena = _OlArenaElement(family_arena, i).l;
	for(j=0; j<_OlArenaSize(look_arena); j++) {
	    ps_arena = _OlArenaElement(look_arena, j).l;
	    for(k=0; k<_OlArenaSize(ps_arena); k++) {
		font_info = _OlArenaElement(ps_arena, k).l;
		if (font_info->bitmap == bitmap) {
		    if (bitmap)
			sprintf( font_name, "%s  %s  %s",
				_OlArenaElement(family_arena, i).n,
				_OlArenaElement(look_arena, j).look_name,
				_OlArenaElement(ps_arena, k).ps);
		    else
			sprintf( font_name, "%s  %s",
				_OlArenaElement(family_arena, i).n,
				_OlArenaElement(look_arena, j).look_name);
		    if (!bitmap || !SystemFont(font_info->xlfd_name)) {
			InsertStringDB( delete_info->font_name, font_name);
			InsertStringDB( delete_info->xlfd, font_info->xlfd_name);
		    }
		}
	    }
	}
    }

    if (delete_info->font_name->n_strs)
	CreateDeletePopup(w, delete_info, bitmap, title);
    else {
	if (delete_info->popup)
	    XtPopdown(delete_info->popup);
	if (bitmap)
	    InformUser(TXT_NO_DELETABLE_BITMAP);
	else
	    InformUser(TXT_NO_DELETABLE_OUTLINE);
    }
} /* end of DisplayDeletePopup */


void DisplayBitmapDeleteCB( w, client, call )
Widget	     w;
XtPointer    client;
XtPointer    call;
{
    delete_info.bitmap = True;
    help_delete.section = TXT_HELP_DEL_BITMAP;
    help_confirm_delete.section = TXT_HELP_DEL_BITMAP_WARN;
    DisplayDeletePopup( w, &delete_info, TXT_DEL_BITMAP);
}


void DisplayOutlineDeleteCB( w, client, call )
Widget	     w;
XtPointer    client;
XtPointer    call;
{
    delete_info.bitmap = False;
    help_delete.section = TXT_HELP_DEL_OUTLINE;
    help_confirm_delete.section = TXT_HELP_DEL_OUTLINE_WARN;
    DisplayDeletePopup( w, &delete_info, TXT_DEL_OUTLINE);

} /* end of DisplayOutlineDeleteCB */


static void
ResetDeleteCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    delete_type *delete_info = (delete_type *) client_data;

    int i;

    for(i=0; i< delete_info->font_name->n_strs; i++)
	/* unselect all items */
	OlVaFlatSetValues(delete_info->font_list, i,
			  XtNset, FALSE,
			  (String) 0);
    
} /* end of ResetDeleteCB */


static void
ConfirmCancelCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    delete_type *delete_info = (delete_type *) client_data;

    XtPopdown( delete_info->popup);
    XtPopdown( prompt.shell);

} /* end of ConfirmCancelCB */


static void
DeleteCancelCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    delete_type *delete_info = (delete_type *) client_data;

    XtPopdown( delete_info->popup);

} /* end of DeleteCancelCB */


static void
ApplyDeleteCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    delete_type *info = (delete_type *) client_data;

    GetSelectedFonts(info);
    if (info->selected_xlfd->n_strs)
	PromptUser(TXT_CONFIRM_MESS, DoDeleteCB, info,
               ConfirmCancelCB, info, &help_confirm_delete);
    else {
	XtPopdown( info->popup);
	InformUser(TXT_NONE_DELETE);
    }
} /* end of ApplyDeleteCB */


void 
IntegrityCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    char **font_path;
    int n_paths;

    BusyCursor(0);
    font_path = XGetFontPath(XtDisplay(app_shellW), &n_paths);
    UpdateFonts( font_path, n_paths, FALSE);
    XFreeFontPath(font_path);
    StandardCursor(0);

} /* end of IntegrityCB */

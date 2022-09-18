/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtadmin:fontmgr/font_add.c	1.20"

/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       font_add.c
 */

#include <stdio.h>

#include <StringDefs.h>
#include <Intrinsic.h>

#include <OpenLook.h>

#include <Gizmos.h>
#include <MenuGizmo.h>
#include <ModalGizmo.h>
#include <PopupGizmo.h>
#include <RubberTile.h>
#include <InputGizmo.h>

#include <ScrolledWi.h>
#include <FList.h>
#include <Gauge.h>
#include <Caption.h>
#include <Stub.h>

#include <DtI.h>

#include <fontmgr.h>

enum media_status { NOT_DOS, NOT_ATM, ATM, NOT_INSERTED, NO_SUCH_DEVICE };

/*
 * external data
 */
extern Widget       app_shellW;		  /* application shell widget       */
extern Widget base_shell;
extern Boolean secure;
extern char *xwin_home;

extern    FILE *ForkWithPipe();

extern StandardCursor();
extern void HelpCB();

static void ApplyNextDiskCB();
static void ApplyCB();
static void ApplyAllCB();
static void CancelCB();
static void PopupAddWindow(Boolean);

static string_array_type _font_name; 
static string_array_type _disk_label;
static add_type add_info = { &_font_name, &_disk_label };

static HelpInfo help_insert_disk = { 0, 0, HELP_PATH, TXT_HELP_INSERT_DISK };
static HelpInfo help_insert_dos_disk = { 0,0, HELP_PATH, TXT_HELP_INSERT_DOS_DISK };
static HelpInfo help_insert_atm_disk = {0,0, HELP_PATH, TXT_HELP_INSERT_ATM_DISK };
static HelpInfo help_install = {0,0, HELP_PATH, TXT_HELP_INSTALL };

#define APPLY_BUT 0
#define APPLY_ALL_BUT 1
static MenuItems menu_item[] = {  
{ TRUE, TXT_ADD_APPLY,     ACCEL_ADD_APPLY    , 0, ApplyCB, (char *) &add_info },
{ TRUE, TXT_ADD_APPLY_ALL, ACCEL_ADD_APPLY_ALL, 0, ApplyAllCB, (char *)&add_info},
{ TRUE, TXT_CANCEL,    ACCEL_ADD_CANCEL   , 0, CancelCB, (char *)&add_info },
{ TRUE, TXT_HELP_DDD,      ACCEL_ADD_HELP     , 0, HelpCB, (char *)&help_install },
{ NULL }
	      };

static MenuGizmo menu = {0, "[dm", "dm", menu_item};
static PopupGizmo popup = {0, "dp", TXT_FONT_ADD, (Gizmo)&menu };

static char *atm_file_name[] = {
    "psfonts/COB_____",
    "psfonts/COBO____",
    "psfonts/COM_____",
    "psfonts/COO_____",
    "psfonts/HVB_____",
    "psfonts/HVBO____",
    "psfonts/HV______",
    "psfonts/HVO_____",
    "psfonts/SY______",
    "psfonts/TIB_____",
    "psfonts/TIBI____",
    "psfonts/TIR_____",
    "psfonts/TII_____"
};
 
static char *atm_font_name[] = {
    "Courier Bold",
    "Courier Bold Oblique",
    "Courier",
    "Courier Oblique",
    "Helvetica Bold",
    "Helvetica Bold Oblique",
    "Helvetica",
    "Helvetica Oblique",
    "Symbol",
    "Times Bold",
    "Times Bold Italic",
    "Times Roman",
    "Times Italic"
};


/*
 * get the ATM label for the specified disk number
 */
static String GetDiskLabel(add_type *info)
{
    int i;
    static char id[MAX_STRING], disk_label[MAX_STRING];
    int disk_num;

    disk_label[0] = 0;
    for(i=0; i<info->disk_label->n_strs; i++) {
	sscanf(info->disk_label->strs[i], "%s %d '%[^']",
	       id, &disk_num, disk_label);
	if (disk_num == info->disk_num)
	    break;
	else
	    disk_label[0] = 0;
    }
    return disk_label;

} /* end of GetDiskLabel */


void AddCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    BusyCursor(0);
    ScheduleWork (PopupAddWindow, FALSE, 1);

} /* end of AddCB */


static void
CreateAddWindow( info)
    add_type *info;
{
    static String item_fields[] = {XtNlabel};
    Widget scroll_win, upper, list_caption, gauge_caption, rubber;
    int n;
    Arg largs[20];

    /* if popup doesn't exist, then create it */
    if (!info->popup) {
	info->popup = CreateGizmo(base_shell, PopupGizmoClass,
			      &popup, NULL, 0);

	XtVaGetValues( info->popup, XtNupperControlArea, &upper, 0);

	n = 0;
	XtSetArg(largs[n], XtNorientation, OL_HORIZONTAL); n++;
	rubber = XtCreateManagedWidget("rubber",
			rubberTileWidgetClass, upper, largs, n);

	n = 0;
	XtSetArg(largs[n], XtNlabel, GetGizmoText(TXT_ADD_LIST_CAPTION)); n++;
	XtSetArg(largs[n], XtNposition, OL_TOP);    n++;
	XtSetArg(largs[n], XtNalignment, OL_CENTER);    n++;
	list_caption = XtCreateManagedWidget("lc",
			captionWidgetClass, rubber, largs, n);

	/*  Create the controls in the upper control area.  */
	n = 0;
	scroll_win = XtCreateManagedWidget("scroll_win",
		  scrolledWindowWidgetClass, list_caption, largs, n);

	n = 0;
	XtSetArg(largs[n], XtNwidth, 40); n++;
	XtSetArg(largs[n], XtNheight, 40); n++;
	XtSetArg(largs[n], XtNweight, 10); n++;
	XtCreateManagedWidget("stub", stubWidgetClass, rubber, largs, n);

	n = 0;
	XtSetArg(largs[n], XtNlabel, GetGizmoText(TXT_ADD_GAUGE_CAPTION)); n++;
	XtSetArg(largs[n], XtNposition, OL_TOP);    n++;
	XtSetArg(largs[n], XtNalignment, OL_CENTER);    n++;
	gauge_caption = XtCreateManagedWidget("lc",
			captionWidgetClass, rubber, largs, n);

	n = 0;
	XtSetArg(largs[n], XtNminLabel, "0 %");       n++;
	XtSetArg(largs[n], XtNmaxLabel, "100 %");       n++;
	XtSetArg(largs[n], XtNticks, 10);       n++;
	XtSetArg(largs[n], XtNtickUnit, OL_PERCENT);       n++;
	XtSetArg(largs[n], XtNshowValue, FALSE);       n++;
	info->gauge = XtCreateManagedWidget("g", 
				gaugeWidgetClass, gauge_caption, largs, n);

	n = 0;
	XtSetArg(largs[n], XtNviewHeight, NUM_FAMILY_ITEMS);    n++; 

	/* WARNING: always specify XtNitems when creating a flatlist,
	   the widget gets confused and core dumps upon
	   subsequent SetValues */
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
    /* reset gauge */
    XtVaSetValues( info->gauge, XtNsliderValue, 0, 0);
    info->select_cnt = info->font_name->n_strs;
    XtManageChild(menu.child);
    SetPopupMessage( &popup, GetGizmoText(TXT_BLANK));
    MapGizmo(PopupGizmoClass, &popup);
    StandardCursor(info->popup);

} /* end of CreateAddWindow */


/*
 * insert the family names that the user selected into an array
 */
static void
GetSelectedFonts( info)
    add_type *info;
{
    Boolean selected;
    int i;

    for(i=info->font_name->n_strs - 1; i>=0; i--) {
	OlVaFlatGetValues(info->font_list, i,
			  XtNset, &selected, 0);
	if (!selected) {
	    info->select_cnt--;
	    info->db[i].pfb_disk = info->db[i].afm_disk = 0;
	}
    }
} /* end of GetSelectedFonts */


static void InsertAddDB(add_type *info, String file_name,
			int pfb_disk, int afm_disk)
{
    if (info->db == NULL) {
	info->db = XtNew(add_db);
	info->font_cnt = 0;
    }
    else
	info->db = (add_db *) XtRealloc((char*)info->db,
					sizeof(add_db) * (info->font_cnt+1));
    strcpy(info->db[info->font_cnt].file_name, file_name);
    info->db[info->font_cnt].pfb_disk = pfb_disk;
    info->db[info->font_cnt].afm_disk = afm_disk;
    (info->font_cnt)++;

} /* end of InsertAddDB */


static Boolean
ReadType1(add_type *info, String dir)
{
    FILE *fp;
    char buf[PATH_MAX];
    char path_name[MAX_STRING];
    char body[MAX_STRING];
    char suffix[MAX_STRING];
    int pid, w;

    sprintf( buf, "%s:/%s", info->device, dir);
    if ((fp = ForkWithPipe(&pid, "/usr/bin/dosls", buf, NULL)) == NULL) {
	perror("fontmgr");
	return FALSE;
    }
    while (fgets(buf, sizeof(buf), fp) != NULL) {
	if (sscanf(buf, "%[^.]%s", body, suffix) < 2)
	    continue;
	if (strncmp(suffix, ".PFB", 4) == STR_MATCH) {
	    sprintf(path_name, "%s/%s", dir, body);
	    InsertAddDB(info, path_name, 1, 1);
	    InsertStringDB(info->font_name, body);
	}
    }
    fclose(fp);
    /* if we don't wait for the child process to exit, the child will turn
       into a zombie */
    while ((w = wait(NULL)) != pid && w != -1)
	;

    if (info->font_cnt) {
	/* insert a stub diskname entry */
	InsertStringDB(info->disk_label, "DISKNAME 1 'Font Disk'");
	info->adobe_foundry = FALSE;
    }
    else
	return FALSE;

    return TRUE;
} /* end of ReadType1 */


#ifdef old
static Boolean
ReadBasicATM(add_type *info)
{
    FILE *fp;
    char buf[PATH_MAX];
    int pid, w, i;

    sprintf( buf, "%s:/psfonts", info->device);
    if ((fp = ForkWithPipe(&pid, "/usr/bin/dosls", buf, NULL)) == NULL) {
	perror("fontmgr");
	return FALSE;
    }
    while (fgets(buf, sizeof(buf), fp) != NULL) {
	if (strncmp(buf, "COBO____.PFB", 12) == STR_MATCH) {
	    for (i=0; i<XtNumber(atm_file_name); i++) {
		InsertStringDB(info->font_name, atm_font_name[i]);
		InsertAddDB(info, atm_file_name[i], 1, 0);
	    }
	    /* insert a stub diskname entry */
	    InsertStringDB(info->disk_label, "DISKNAME 1 'Font Disk'");
	    break;
	}
    }
    fclose(fp);
    /* if we don't wait for the child process to exit, the child will turn
       into a zombie */
    while ((w = wait(NULL)) != pid && w != -1)
	;

    if (info->font_cnt == 0) {
	return FALSE;
    }
    return TRUE;
    
}
/* end of ReadBasicATM */
#endif

/*
 * returns FALSE if the specified path didnot contain any font files
 */
static
ReadATM(info)
    add_type *info;
{
    FILE *fp;
    char buf[PATH_MAX];
    char id[MAX_STRING];
    char font_name[MAX_STRING];
    char file_name[MAX_STRING];
    int pid, w, status = TRUE;
    int pfb_disk, inf_disk, ctf_disk, unused, abf_disk, afm_disk;
    
    sprintf( buf, "%s:/install.cfg", info->device);
    if ((fp = ForkWithPipe(&pid, "/usr/bin/doscat", buf, NULL)) == NULL) {
	perror("fontmgr");
	return FALSE;
    }
    while (fgets(buf, sizeof(buf), fp) != NULL) {
	if (sscanf(buf, "%s", id) < 1)
	    continue;
	if (strcmp("FONT", id) == STR_MATCH) {

	    /* got a font entry, parse it */
	    if (sscanf(buf, "%s %s %s %d %d %d %d %d %d",
		       id, font_name, file_name,
		       &pfb_disk, &inf_disk, &ctf_disk,
		       &unused, &abf_disk, &afm_disk) < 9)
		continue;
	    strcat( file_name, "___");
	    InsertStringDB(info->font_name, font_name);
	    InsertAddDB(info, file_name, pfb_disk, afm_disk);
	}
	else if (strcmp("DISKNAME", id) == STR_MATCH) {

	    /* got a diskname entry, save it */
	    InsertStringDB(info->disk_label, buf);
	}
    }
    fclose(fp);
    /* if we don't wait for the child process to exit, the child will turn
       into a zombie */
    while ((w = wait(NULL)) != pid && w != -1)
	;

    /* if there are no PFB files, then try the basic ATM fonts */
    if (info->font_name->n_strs == 0) {
	status = ReadType1( info, "psfonts");
    }

    /* if there are no ATM fonts, try third party Type1 fonts */
    if (status == FALSE)
	status = ReadType1(info, "");

    return status;
} /* end of ReadATM */


static void
InstallFonts(info)
    add_type *info;
{
    int i, pid, w, exit_status, cur_disk_num;
    char afm_dir[MAX_PATH_STRING];
    char *font_list[1];
    char sys_cmd[MAX_PATH_STRING];
    char buf[MAX_PATH_STRING];
    char font_dir[MAX_PATH_STRING];
    String file_name;

    sprintf(font_dir, "%s/lib/fonts/type1", xwin_home);
    sprintf(afm_dir, "%s/%s", font_dir, AFM_DIR);
    mkdir(font_dir, 0755);
    mkdir(afm_dir, 0755);

    cur_disk_num = info->disk_num;
    for(i=0; i<info->font_cnt; i++) {
	if (info->db[i].pfb_disk == info->disk_num) {
	    /* copy PFB files from dos disk to unix disk */
	    sprintf(sys_cmd, "/usr/bin/doscp %s:/%s.pfb %s",
		    info->device, info->db[i].file_name, font_dir);
	    if (system(sys_cmd))
		break;

	    /* generate ascii version */
	    file_name = strchr(info->db[i].file_name, '/');
	    if (file_name)
		file_name++;
	    else
		file_name = info->db[i].file_name;
	    sprintf(sys_cmd, "%s/%s.pfb", font_dir, file_name);
	    sprintf(buf, "%s/%s.pfa", font_dir, file_name);
	    BinaryToAscii(sys_cmd, buf);

	    /* delete pfb file */
	    unlink(sys_cmd);

	    info->db[i].pfb_disk = 0;       /* mark file as done */
	    XtVaSetValues( info->gauge, XtNsliderValue, ++info->slider_val, 0);
	    XFlush(XtDisplay(info->gauge));
	}

	/* copy AFM files from dos disk to unix disk */
	if (info->db[i].afm_disk == info->disk_num) {
	    sprintf(sys_cmd, "/usr/bin/doscp %s:/%s.afm %s",
		    info->device, info->db[i].file_name, afm_dir);
	    if (system(sys_cmd) && info->adobe_foundry) {
		break;
	    }
	    info->db[i].afm_disk = 0;       /* mark file as done */
	    XtVaSetValues( info->gauge, XtNsliderValue, ++info->slider_val, 0);
	    XFlush(XtDisplay(info->gauge));
	}
    } /* for i */

    /* make sure we got all the fonts out of the current disk */
    for(i=0; i<info->font_cnt; i++) {
	if ((info->db[i].pfb_disk == info->disk_num) ||
	    (info->db[i].afm_disk == info->disk_num)) {
	    (info->disk_num)--;
	    break;
	}
    }
	    
    ++(info->disk_num);
    /* if wanted disk is same as current disk then we are done */ 
    if ((info->disk_num <= info->disk_label->n_strs) &&
	(info->disk_num != cur_disk_num)) {
	sprintf(buf, "Insert %s", GetDiskLabel(info));
	PromptUser(buf, ApplyNextDiskCB, info,
               CancelCB, info, &help_insert_dos_disk);
    }
    else {
	font_list[0] = font_dir;
	UpdateFonts( font_list, 1, 0);
	InformUser(TXT_ADD_FINISH);
	XtPopdown(info->popup);
    }
} /* end of InstallFonts */


static void
DoApply( info)
    add_type *info;
{
    if (info->select_cnt) {
	SetPopupMessage( &popup, GetGizmoText(TXT_ADD_START));
	XtUnmanageChild(menu.child);
	BusyCursor(info->popup);
	XtVaSetValues( info->gauge, XtNsliderMax, (info->select_cnt*2)+2,
		  XtNsliderValue, info->slider_val, 0);
	XSync (XtDisplay(info->popup), FALSE);
	ScheduleWork(InstallFonts, info, 2);
    }
    else {
	InformUser(GetGizmoText(TXT_NONE_ADD_SEL));
	XtPopdown(info->popup);
    }
} /* end of DoApply */


static void
ApplyNextDiskCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    add_type *info = (add_type *) client_data;

    PopdownPrompt();
    XSync(XtDisplay(info->popup), FALSE);
    XSync(XtDisplay(info->popup), FALSE);
    XSync(XtDisplay(info->popup), FALSE);
    ScheduleWork(InstallFonts, info, 2);

} /* end of ApplyCB */


void
ApplyCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    add_type *info = (add_type *) client_data;

    GetSelectedFonts(info);
    DoApply(info);

} /* end of ApplyCB */


void
ApplyAllCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    add_type *info = (add_type *) client_data;

    DoApply(info);

} /* end of ApplyAllCB */


void
CancelCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    add_type *info = (add_type *) client_data;

    PopdownPrompt();
    if (info->popup)
	XtPopdown(info->popup);

} /* end of CancelCB */


static
GetFontName( info)
    add_type *info;
{
    int status, str_len;
    char buf[PATH_MAX];

    /* we need to resolve the link because dosdir has problems with links */
    if ((str_len = readlink(info->device, buf, PATH_MAX)) != -1) {
	strncpy(info->device, buf, str_len);
	info->device[str_len] = '\0';
    }

    if (ReadATM( info))
        return ATM;
    else
	return NOT_INSERTED;

#ifdef old
    status = DtamCheckMedia(info->device);
printf("stat=%d, dev=%s\n", status, info->device);
    if (status == DTAM_DOS_DISK) {
	if (ReadATM( info))
	    return ATM;
	else
	    return NOT_ATM;
    }

    if (status == DTAM_NO_DISK)
	return NOT_INSERTED;
    else if (status == DTAM_BAD_DEVICE)
	return NO_SUCH_DEVICE;
    else
	return NOT_DOS;
#endif
} /* end of GetFontName */


static void
PopupAddWindow(Boolean device_specified)
{
    int status, status_B = NOT_INSERTED, i;
    add_type *info = &add_info;

    if (secure) {
	StandardCursor(0);
	return;
    }
    /* init */
    DeleteStringsDB( info->font_name);
    DeleteStringsDB( info->disk_label);
    if (info->db)
	XtFree((char*)info->db);
    info->db = NULL;
    info->font_cnt = 0;
    info->slider_val = 1;
    info->disk_num = 1;
    info->adobe_foundry = TRUE;

    /* try both diskettes if none if specified */
    if (device_specified) {
	status = GetFontName(info);
    }
    else {
	strcpy( info->device, "/dev/dsk/f0t");
	if ((status = GetFontName(info)) != ATM) {
	    strcpy( info->device, "/dev/dsk/f1t");
	    status_B = GetFontName(info);
	}
    }

    if ((status == ATM) || (status_B == ATM)) {
	PopdownPrompt();
	CreateAddWindow(info);
    }
    else {
	if (info->popup)
	    XtPopdown(info->popup);
	if (status == NOT_DOS) {
	    PromptUser(TXT_INSERT_DOS_DISK, AddCB, info,
               CancelCB, info, &help_insert_dos_disk);
	}
    	else if (status == NOT_ATM) {
	    PromptUser(TXT_INSERT_ATM_DISK, AddCB, info,
               CancelCB, info, &help_insert_atm_disk);
	}
	else
	    PromptUser(TXT_INSERT_DISK, AddCB, info,
               CancelCB, info, &help_insert_disk);
    }
    StandardCursor(0);

} /* end of PopupAddWindow */


void
DnDPopupAddWindow( path)
    String path;
{
    strcpy(add_info.device, path);
    BusyCursor(0);
    ScheduleWork(PopupAddWindow, TRUE, 1);

} /* DnDPopupAddWindow */





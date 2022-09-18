/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:f_prop.c	1.55"

/******************************file*header********************************

    Description:
	This file contains the source code for folder-window file property
	sheets.
*/
						/* #includes go here	*/
#include <errno.h>
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>		/* for _OlGetShellOfWidget */
#include <Xol/TextField.h>
#include <Xol/Caption.h>
#include <Xol/FButtons.h>
#include <Xol/PopupWindo.h>
#include <Xol/StaticText.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/
static void		ApplyCB(Widget, XtPointer, XtPointer);
static Widget		CreateCaption(Widget parent, char * label);
static void		CreatePermButtons(Widget w, DmFPropPermsPtr permp,
					  mode_t mode, int owner, int type);
static Widget		CreateSheet(DmFolderWinPtr window, DmItemPtr ip);
static DmFPropSheetPtr	FindSheet(Widget w, DmFolderWinPtr window);
static DmFPropSheetPtr	GetNewSheet(DmFolderWinPtr window);
static void		PopdownCB(Widget, XtPointer, XtPointer);
static void		ResetCB(Widget, XtPointer, XtPointer);
static void		ResetTF(DmFPropTFPtr tfptr);
static mode_t		UpdatePermissions(DmFPropPermsPtr permp, int type);
static void		VerifyCB(Widget, XtPointer, XtPointer);
static void         HelpCB(Widget, XtPointer, XtPointer);

					/* public procedures		*/
void			Dm__PopupFilePropSheet(DmFolderWinPtr, DmItemPtr);
void			Dm__PopupFilePropSheets(DmFolderWinPtr window);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* property type for the comment field */
#define COMMENT		"Comment"

/* file permissions categories */
#define OWNER	1
#define	GROUP	2
#define	OTHER	3

static OlDtHelpInfo help_info;
	
/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    CreateCaption- function creates a caption for all the fields in
	file property sheet. Since every field has a caption label
	associated with it, a convenience routine is warranted.
*/
static Widget
CreateCaption(Widget parent, char * label)
{
    XtSetArg(Dm__arg[0], XtNborderWidth, 0);
    XtSetArg(Dm__arg[1], XtNposition, OL_LEFT);
    XtSetArg(Dm__arg[2], XtNlabel, label);
    XtSetArg(Dm__arg[3], XtNalignment, OL_CENTER);
    return(XtCreateManagedWidget(label, captionWidgetClass, 
				 parent, Dm__arg,4));

}				/* End of CreateCaption */

/****************************procedure*header*****************************
    GetNewSheet- function allocates a new file property structure.
	Since more than one file property sheets are allowed, for
	optimization purpose we are reusing the file property structure.
	Available struct is marked with prop_num == -1
*/ 
static DmFPropSheetPtr
GetNewSheet(DmFolderWinPtr window)
{

    DmFPropSheetPtr fpsptr, last;

    if (window->fpsptr == NULL)
    {
	fpsptr = (DmFPropSheetPtr)CALLOC(1, sizeof(DmFPropSheetRec));
	window->fpsptr = fpsptr;
	fpsptr->next = NULL;

	fpsptr->fntfptr = (DmFPropTFPtr)MALLOC(sizeof(DmFPropTFRec));	
	fpsptr->grptfptr = (DmFPropTFPtr)MALLOC(sizeof(DmFPropTFRec));	
	fpsptr->owntfptr = (DmFPropTFPtr)MALLOC(sizeof(DmFPropTFRec));	
	fpsptr->cmtptr = (DmFPropTFPtr)MALLOC(sizeof(DmFPropTFRec));	
	fpsptr->ownptr = (DmFPropPermsPtr)MALLOC(sizeof(DmFPropPermsRec));
	fpsptr->grpptr = (DmFPropPermsPtr)MALLOC(sizeof(DmFPropPermsRec));
	fpsptr->othptr = (DmFPropPermsPtr)MALLOC(sizeof(DmFPropPermsRec));
	return(fpsptr);
    }

    for(fpsptr = window->fpsptr; fpsptr->next; fpsptr = fpsptr->next)
	if(fpsptr->prop_num == -1)
	    return(fpsptr);

    /* check for the last one, this way last entry is handy in
       case we need at next lines
       */
    if (fpsptr->prop_num == -1)
	return(fpsptr);

    last = fpsptr;
    fpsptr = (DmFPropSheetPtr)CALLOC(1, sizeof(DmFPropSheetRec));
    last->next = fpsptr;
    fpsptr->next = NULL;
    fpsptr->fntfptr = (DmFPropTFPtr)MALLOC(sizeof(DmFPropTFRec));	
    fpsptr->grptfptr = (DmFPropTFPtr)MALLOC(sizeof(DmFPropTFRec));	
    fpsptr->owntfptr = (DmFPropTFPtr)MALLOC(sizeof(DmFPropTFRec));	
    fpsptr->cmtptr = (DmFPropTFPtr)MALLOC(sizeof(DmFPropTFRec));	
    fpsptr->ownptr = (DmFPropPermsPtr)MALLOC(sizeof(DmFPropPermsRec));	
    fpsptr->grpptr = (DmFPropPermsPtr)MALLOC(sizeof(DmFPropPermsRec));	
    fpsptr->othptr = (DmFPropPermsPtr)MALLOC(sizeof(DmFPropPermsRec));	
    return(fpsptr);

}				/* End of GetNewSheet */


/****************************procedure*header*****************************
    CreatePermButtons() function creates permission buttons for owner,
	group and other categories and sets appropriate state based on the
	permission on the file.
*/
static void
CreatePermButtons(Widget w, DmFPropPermsPtr permp, mode_t mode, int owner,
		  int type)
{

    static String p_fields[] = { XtNlabel, XtNset, XtNsensitive };
    XtArgVal *p;

    switch(type)
    {
    case OWNER:
	permp->rprev = (mode & S_IRUSR ? True : False);
	permp->wprev = (mode & S_IWUSR ? True : False);
	permp->eprev = (mode & S_IXUSR ? True : False);
	break;
    case GROUP:
	permp->rprev = (mode & S_IRGRP ? True : False);
	permp->wprev = (mode & S_IWGRP ? True : False);
	permp->eprev = (mode & S_IXGRP ? True : False);
	break;
    case OTHER:
	permp->rprev = (mode & S_IROTH ? True : False);
	permp->wprev = (mode & S_IWOTH ? True : False);
	permp->eprev = (mode & S_IXOTH  ? True : False);
	break;
    }

    permp->perms = p = (XtArgVal *)
	MALLOC(sizeof(XtArgVal) * XtNumber(p_fields) * 3);
    *p++ = (XtArgVal)Dm__gettxt(TXT_READ_PERM);
    *p++ = (XtArgVal)permp->rprev;
    *p++ = (XtArgVal)(owner <= 0 ? False : True);
    *p++ = (XtArgVal)Dm__gettxt(TXT_WRITE_PERM);
    *p++ = (XtArgVal)permp->wprev;
    *p++ = (XtArgVal)(owner <= 0 ? False : True);
    *p++ = (XtArgVal)Dm__gettxt(TXT_EXEC_PERM);
    *p++ = (XtArgVal)permp->eprev;
    *p++ = (XtArgVal)(owner <= 0 ? False : True);

    XtSetArg(Dm__arg[0], XtNmeasure,		3);
    XtSetArg(Dm__arg[1], XtNlayoutType,		OL_FIXEDCOLS);
    XtSetArg(Dm__arg[2], XtNitems,		permp->perms);
    XtSetArg(Dm__arg[3], XtNnumItems,		3);
    XtSetArg(Dm__arg[4], XtNitemFields,		p_fields);
    XtSetArg(Dm__arg[5], XtNnumItemFields,	XtNumber(p_fields));
    XtSetArg(Dm__arg[6], XtNbuttonType,		OL_RECT_BTN);
    permp->w = XtCreateManagedWidget("permWidget", flatButtonsWidgetClass,
				     w, Dm__arg, 7);

}				/* End of CreatePermButtons */

/* FindSheet() finds a file property structure given the prop_num */

static DmFPropSheetPtr
FindSheet(Widget w, DmFolderWinPtr window)
{
    Widget		shell = _OlGetShellOfWidget(w);
    DmFPropSheetPtr	fpsptr;

    for(fpsptr = window->fpsptr; fpsptr != NULL; fpsptr = fpsptr->next)
		if (shell == fpsptr->shell)
		    break;

    return(fpsptr);
}

/* CreateSheet() function the file property sheet for the item
   pointed to bt 'ip'.
*/
static Widget
CreateSheet(DmFolderWinPtr window, DmItemPtr ip)
{

    static int prop_num = 1;
    Widget	caption, uca, lca;
    DmFPropSheetPtr	fpsptr;
    int	owner;
    struct passwd *pw;
    struct group *gr;
    XtArgVal *p;
    char *name;
    char local_strbuf[29];
    int len;
    static String btn_fields[] = { XtNlabel, XtNmnemonic, XtNselectProc };

    fpsptr		= GetNewSheet(window);
    fpsptr->prop_num	= prop_num++;
    fpsptr->shell	= XtCreatePopupShell(Dm__gettxt(TXT_FILE_PROP_TITLE),
					     popupWindowShellWidgetClass,
					     window->shell, NULL, 0);
    fpsptr->flag	= False;
    fpsptr->window	= window;
    fpsptr->item	= ip;

    /* get upper and lower control area */
    XtSetArg(Dm__arg[0], XtNupperControlArea, &uca);
    XtSetArg(Dm__arg[1], XtNlowerControlArea, &lca);
    XtGetValues(fpsptr->shell, Dm__arg, 2);

    /* first fill up upper control area */
    /* are these really needed or they are defaults? check */
    XtSetArg(Dm__arg[0], XtNlayout, OL_FIXEDCOLS);
    XtSetArg(Dm__arg[1], XtNmeasure, 1);
    XtSetArg(Dm__arg[2], XtNtraversalManager, True);
    XtSetValues(uca, Dm__arg, 3);

    fpsptr->fntfptr->prev = strdup((ITEM_OBJ(ip))->name);
    fpsptr->fntfptr->w = DmCreateInputPrompt(uca,
		Dm__gettxt(TXT_FP_FILE_NAME), fpsptr->fntfptr->prev);

    /* display link info */
    if (ITEM_OBJ(ip)->attrs & DM_B_SYMLINK) {
		/* display symbolic link info */
		int len;
		char buffer[BUFSIZ];

		len = readlink(Dm__ObjPath(ITEM_OBJ(ip)), buffer, BUFSIZ);
		buffer[len] = '\0'; /* readlink doesn't do this */
		DmCreateStaticText(uca, Dm__gettxt(TXT_FPROP_SYMLINK),buffer);
    }
    else if (ITEM_OBJ(ip)->ftype != DM_FTYPE_DIR) {
		/* display hard link info */
		char nlinks[32];

		sprintf(nlinks, "%d", FILEINFO_PTR(ip)->nlink);
		DmCreateStaticText(uca, Dm__gettxt(TXT_FPROP_HARDLINK),nlinks);
    }

    /* create file, owner, and group input fileds */
    /* do not need to worry about 'pw' being NULL here */
    pw = getpwuid(FILEINFO_PTR(ip)->uid);
    if (!pw) {
	sprintf(local_strbuf, "%d", FILEINFO_PTR(ip)->uid);
	name = local_strbuf;
    }
    else
	name = pw->pw_name;
    fpsptr->owntfptr->prev = strdup(name);
    fpsptr->owntfptr->w = DmCreateInputPrompt(uca,
		Dm__gettxt(TXT_OWNER), name);

    /* do not need to worry about 'gr' being NULL  here */
    gr = getgrgid(FILEINFO_PTR(ip)->gid);
    if (!gr) {
	sprintf(local_strbuf, "%d", FILEINFO_PTR(ip)->gid);
	name = local_strbuf;
    }
    else
	name = gr->gr_name;
    fpsptr->grptfptr->prev = strdup(name);
    fpsptr->grptfptr->w = DmCreateInputPrompt(uca,
		Dm__gettxt(TXT_GROUP), name);

    /* set textfields sensitivity based on access() and uid */	
    owner = (getuid() == FILEINFO_PTR(ip)->uid);
    if (!owner)
    {
	XtSetSensitive(fpsptr->fntfptr->w, False);
	XtSetSensitive(fpsptr->owntfptr->w, False);
	XtSetSensitive(fpsptr->grptfptr->w, False);
    }

    /* create static text for modification time - watch out for ctime()
       return string
    */
    caption = CreateCaption(uca, Dm__gettxt(TXT_MODTIME));
    (void)strftime(local_strbuf, sizeof(local_strbuf), TIME_FORMAT,
	     localtime(&FILEINFO_PTR(ip)->mtime));
	
    XtSetArg(Dm__arg[0], XtNstring, local_strbuf);
    fpsptr->timestw = XtCreateManagedWidget("modTime", 
					    staticTextWidgetClass,
					    caption, Dm__arg, 1);

    /* create permission buttons, dimming 'em as appropriate */
    caption = CreateCaption(uca, Dm__gettxt(TXT_OWNER_ACCESS));
    CreatePermButtons(caption, fpsptr->ownptr, FILEINFO_PTR(ip)->mode,
		      owner, OWNER);

    caption = CreateCaption(uca, Dm__gettxt(TXT_GROUP_ACCESS));
    CreatePermButtons(caption, fpsptr->grpptr, FILEINFO_PTR(ip)->mode,
		      owner, GROUP);

    caption = CreateCaption(uca, Dm__gettxt(TXT_OTHER_ACCESS));
    CreatePermButtons(caption, fpsptr->othptr, FILEINFO_PTR(ip)->mode,
		      owner, OTHER);

    /* create static text for icon class name */
    caption = CreateCaption(uca, Dm__gettxt(TXT_ICON_CLASS));

    XtSetArg(Dm__arg[0], XtNstring, DmObjClassName(ITEM_OBJ(ip)));
    fpsptr->iclstw = XtCreateManagedWidget("className", 
					   staticTextWidgetClass,
					   caption, Dm__arg, 1);

    /* create comments field */
    fpsptr->cmtptr->prev = DmGetObjProperty(ITEM_OBJ(ip), COMMENT, NULL);
    fpsptr->cmtptr->prev = (fpsptr->cmtptr->prev) ?
			   strdup(fpsptr->cmtptr->prev) : strdup("");
    fpsptr->cmtptr->w = DmCreateInputPrompt(uca, Dm__gettxt(TXT_COMMENTS), 
					    fpsptr->cmtptr->prev);
    if (!owner)
	XtSetSensitive(fpsptr->cmtptr->w, False);

    /* now create buttons in lower control area */
    fpsptr->button_items = p = (XtArgVal *)
	MALLOC(sizeof(XtArgVal) * 4 * XtNumber(btn_fields));

    *p++ = (XtArgVal)Dm__gettxt(TXT_APPLY_STR);
    *p++ = (XtArgVal)*Dm__gettxt(TXT_M_APPLY_STR);
    *p++ = (XtArgVal)ApplyCB;
    *p++ = (XtArgVal)Dm__gettxt(TXT_RESET_STR);
    *p++ = (XtArgVal)*Dm__gettxt(TXT_M_RESET_STR);
    *p++ = (XtArgVal)ResetCB;
    *p++ = (XtArgVal)Dm__gettxt(TXT_CANCEL_STR);
    *p++ = (XtArgVal)*Dm__gettxt(TXT_M_CANCEL_STR);
    *p++ = (XtArgVal)PopdownCB;
    *p++ = (XtArgVal)Dm__gettxt(TXT_HELP_ELLIPSIS);
    *p++ = (XtArgVal)*Dm__gettxt(TXT_M_HELP_STR);
    *p++ = (XtArgVal)HelpCB;

    XtSetArg(Dm__arg[0], XtNitemFields,		btn_fields);
    XtSetArg(Dm__arg[1], XtNnumItemFields,	XtNumber(btn_fields));
    XtSetArg(Dm__arg[2], XtNitems,		fpsptr->button_items);
    XtSetArg(Dm__arg[3], XtNnumItems,		4);
    XtSetArg(Dm__arg[4], XtNlayoutType,		OL_FIXEDROWS);
    XtSetArg(Dm__arg[5], XtNmeasure,		1);
    XtSetArg(Dm__arg[6], XtNclientData,		fpsptr);
    XtSetArg(Dm__arg[7], XtNdefault,		True);
    XtCreateManagedWidget("buttons", flatButtonsWidgetClass,
			  lca, Dm__arg, 8);

    /* add popdown and verify callbacks */
    XtAddCallback(fpsptr->shell, XtNverify, VerifyCB, fpsptr);
    XtAddCallback(fpsptr->shell, XtNpopdownCallback, PopdownCB, fpsptr);

    return(fpsptr->shell);
}


/* UpdatePermissions() function gets called from ApplyCB().
   It updates the permission for the category indicated by 'type'.
   Note, that the mode change is not enough., additional info. about
   file type, sticky bit etc. needs to be restored as well from the original
   mode. The later is done by the caller.
*/
static mode_t
UpdatePermissions(DmFPropPermsPtr permp, int type)
{
	mode_t mode = 0;

	XtSetArg(Dm__arg[0], XtNset, &permp->rprev);
	OlFlatGetValues(permp->w, 0, Dm__arg, 1);
	XtSetArg(Dm__arg[0], XtNset, &permp->wprev);
	OlFlatGetValues(permp->w, 1, Dm__arg, 1);
	XtSetArg(Dm__arg[0], XtNset, &permp->eprev);
	OlFlatGetValues(permp->w, 2, Dm__arg, 1);

	switch(type)
		{
		case OWNER:
			if (permp->rprev)
				mode |= S_IRUSR;
			if (permp->wprev)
				mode |= S_IWUSR;
			if (permp->eprev)
				mode |= S_IXUSR;
			break;
		case GROUP:
			if (permp->rprev)
				mode |= S_IRGRP;
			if (permp->wprev)
				mode |= S_IWGRP;
			if (permp->eprev)
				mode |= S_IXGRP;
			break;
		case OTHER:
			if (permp->rprev)
				mode |= S_IROTH;
			if (permp->wprev)
				mode |= S_IWOTH;
			if (permp->eprev)
				mode |= S_IXOTH;
			break;
		}
	return(mode);
}

/* ApplyCB() function is a callback routine when the user clicks on
   an "Apply" button. It goes thru all the fields, validates new input
   if specified, and if valid, it initiates the change.
   Note the file name change is handles last, so if we had multiple filed
   change, we can take care of all other first, and then file renaming
   will save changing mode etc. later on.
   File name change may also involve additional work, such as, if the
   object was a directory and it/its descentdants are open, we may have
   to change the path.
*/
static	void
ApplyCB(Widget	w, XtPointer client_data, XtPointer call_data)
{
    DmFPropSheetPtr	fpsptr = (DmFPropSheetPtr)client_data;
    DmFolderWinPtr	window = (DmFolderWinPtr)fpsptr->window;
    char 		*fullsrc, *fulldst;
    char		*new_str, *new_own, *new_grp;
    struct passwd *	pw;
    struct		group	*gr;
    mode_t		new_mode, old_mode;
    DmItemPtr		ip;
    int 		gchanged, ochanged;
    mode_t		mymask = S_IFMT | S_ISUID | S_ISGID | S_ISVTX;

    /* check if file still exists */
    fullsrc = strdup(DmMakePath(window->cp->path, fpsptr->fntfptr->prev));
    if (access(fullsrc, F_OK) != 0)
    {
	DmVaDisplayStatus((DmWinPtr)window, True, 
			  TXT_FILE_NOT_EXIST, fpsptr->fntfptr->prev);
	XtFree(fullsrc);
	fpsptr->flag = True;			/* popup should stay up */
	return;	
    }

    ip = DmObjNameToItem((DmWinPtr)window, fpsptr->fntfptr->prev);
    if (!ip || !ITEM_MANAGED(ip))
    {
	DmVaDisplayStatus((DmWinPtr)window, True, 
			  TXT_FILE_NOT_IN_VIEW, fpsptr->fntfptr->prev);
	fpsptr->flag = True;		/* popup should stay up */
	return;	
    }

    /* update permissions. we are not checking changes in individual
       perms. for the sake of optimization.
    */
    old_mode =  FILEINFO_PTR(ip)->mode;
    new_mode = ((UpdatePermissions(fpsptr->ownptr, OWNER)) |
		(UpdatePermissions(fpsptr->grpptr, GROUP)) |
		(UpdatePermissions(fpsptr->othptr, OTHER)));

    FILEINFO_PTR(ip)->mode = (old_mode & mymask) | new_mode;
    chmod(fullsrc, FILEINFO_PTR(ip)->mode);

    /* now owner and group */
    XtSetArg(Dm__arg[0], XtNstring, &new_own);
    XtGetValues(fpsptr->owntfptr->w, Dm__arg, 1);

    XtSetArg(Dm__arg[0], XtNstring, &new_grp);
    XtGetValues(fpsptr->grptfptr->w, Dm__arg, 1);
    if (ochanged = strcmp(fpsptr->owntfptr->prev, new_own))
    {
	/* we have to validate new owner name, may not exist */
	if ( (pw = getpwnam(new_own)) != NULL )
	{
	    XtFree(fpsptr->owntfptr->prev);
	    fpsptr->owntfptr->prev = new_own;
	    FILEINFO_PTR(ip)->uid = pw->pw_uid;

	} else
	{
	    char *retstr;
	    long uid = strtol(new_own, &retstr, 10);

	    if ((new_own != retstr) && (errno != ERANGE))
	    {
		FILEINFO_PTR(ip)->uid = uid;

	    } else
	    {
		DmVaDisplayStatus((DmWinPtr)window, True,
				  TXT_INVALID_OWNER, new_own);
		XtFree(new_own);
		fpsptr->flag = True;	/* popup should stay up */
		return;			/* return now */
	    }
	}
    }	
	
    if (gchanged = strcmp(fpsptr->grptfptr->prev, new_grp))
    {
	/* we have to validate new group, may not exist */
	if ( (gr = getgrnam(new_grp)) != NULL )
	{
	    XtFree(fpsptr->grptfptr->prev);
	    fpsptr->grptfptr->prev = new_grp;
	    FILEINFO_PTR(ip)->gid = gr->gr_gid;

	} else
	{
	    char *retstr;
	    long gid = strtol(new_grp, &retstr, 10);

	    if ((new_grp != retstr) && (errno != ERANGE))
	    {
		FILEINFO_PTR(ip)->gid = gid;

	    } else
	    {
		DmVaDisplayStatus((DmWinPtr)window, True,
				  TXT_INVALID_GROUP, new_grp);
		XtFree(new_grp);
		fpsptr->flag = True;	/* popup should stay up */
		return;			/* return now */
	    }
	}
    }

    if (ochanged || gchanged)
	chown(fullsrc, FILEINFO_PTR(ip)->uid, FILEINFO_PTR(ip)->gid);

    /* no need to free new_own and new_grp */

    /* comment field */
    XtSetArg(Dm__arg[0], XtNstring, &new_str);
    XtGetValues(fpsptr->cmtptr->w, Dm__arg, 1);
    if (strcmp(new_str, fpsptr->cmtptr->prev))
    {
	XtFree(fpsptr->cmtptr->prev);
	fpsptr->cmtptr->prev = new_str;	/* no need to free */
	DmSetObjProperty(ITEM_OBJ(ip), COMMENT,
			 strdup(fpsptr->cmtptr->prev), NULL);

    } else
	XtFree(new_str);	

    /* update file name change */
    XtSetArg(Dm__arg[0], XtNstring, &new_str);
    XtGetValues(fpsptr->fntfptr->w, Dm__arg, 1);

    if (strcmp(fpsptr->fntfptr->prev, new_str))
    {
	DmObjectPtr op;
	struct statvfs mystatvfs;

	statvfs(window->cp->path, &mystatvfs);

	/* we don't allow '/' char while renaming a file */
	if (strchr(new_str, '/'))
	{
	    DmVaDisplayStatus((DmWinPtr)window, True, TXT_NO_SLASH);
	    XtFree(new_str);	
	}
	else if (strlen(new_str) > mystatvfs.f_namemax) /* file name limit */
	{
	    DmVaDisplayStatus((DmWinPtr)window, True,
			      TXT_NAME_TOO_LONG, mystatvfs.f_namemax);
	    XtFree(new_str);
	}
	else
	{
	    XtFree(fpsptr->fntfptr->prev);
	    fpsptr->fntfptr->prev = new_str;
	    fulldst = strdup(DmMakePath(window->cp->path, new_str));
	    if (access(fulldst, F_OK) != 0)
		if(rename(fullsrc, fulldst) == 0)
		{
		    Dimension width;
		    XtSetArg(Dm__arg[0], XtNviewWidth, &width);	
		    XtGetValues(window->box, Dm__arg, 1);

		    /* fix op first */
		    op = ITEM_OBJ(ip);
		    XtFree(op->name);
		    op->name = strdup(new_str);

		    /* and now ip */
		    XtFree(ITEM_LABEL(ip));

		    /* depending on the cur. view, we have to take specific
		       action to get new label to display, set new icon
		       geometry etc.
		    */
		    switch(window->view_type)
		    {
		    case DM_LONG:
		    {
			int maxlen =
			    ItemLabelsMaxLen(window->itp, window->nitems);

			ip->label = (XtArgVal)
			    strdup( Dm__MakeItemLabel(ip, DM_LONG, maxlen) );

			DmComputeItemSize(ip, DM_LONG,
					    (Dimension *)&ip->icon_width,
					    (Dimension *)&ip->icon_height);
		    }
			break;

		    case DM_ICONIC: 
		    {
			int save_w, save_h, tmp, x;
			ip->label = (XtArgVal)strdup(new_str);
			save_w = ITEM_WIDTH(ip);
			save_h = ITEM_HEIGHT(ip);
			DmComputeItemSize(ip, DM_ICONIC,
					    (Dimension *)&ip->icon_width,
					    (Dimension *)&ip->icon_height);
			x = ITEM_X(ip) + ((int)save_w - (int)ITEM_WIDTH(ip))/2;

			tmp = ITEM_WIDTH(ip);
			ip->icon_width = (XtArgVal)save_w;
			save_w = tmp;
			tmp = ITEM_HEIGHT(ip);
			ip->icon_height = (XtArgVal)save_h;
			save_h = tmp;

			XtSetArg(Dm__arg[0], XtNx, x);
			XtSetArg(Dm__arg[1], XtNwidth, save_w);
			XtSetArg(Dm__arg[2], XtNheight, save_h);
			OlFlatSetValues(window->box, ip-window->itp, Dm__arg, 3);
		    }
			break;

		    case DM_NAME:
			ip->label = (XtArgVal)strdup(new_str);

			DmComputeItemSize(ip, DM_NAME,
					    (Dimension *)&ip->icon_width,
					    (Dimension *)&ip->icon_height);
			break;

		    default:
			break;
		    }
	
		    /* if the object is a directory then we may have
		       to update path for open folders starting from
		       this dir. to all descendants
		    */
		    if (op->ftype == DM_FTYPE_DIR)
			DmFolderPathChanged(fullsrc, fulldst);
		}
	}
    }

    if (OlGetGui() == OL_OPENLOOK_GUI) {
	/* Check pushpin state */
	OlDefine pushpin;

	XtSetArg(Dm__arg[0], XtNpushpin, &pushpin);
	XtGetValues(fpsptr->shell, Dm__arg, 1);
	if (pushpin == OL_IN)
		return;
    }

    if (!fpsptr->flag)			/* DON'T stay up?? */
	PopdownCB(w, client_data, call_data);

}				/* end of ApplyCB */

/****************************procedure*header*****************************
    PopdownCB- get called when sheet pops down: from pulling pin,
    Apply, and Cancel.  Must check whether sheet has already been
    popped down (by looking at sheet->shell).  Free space, destroy
    sheet, and unbusy item.
*/
static void
PopdownCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFPropSheetPtr	sheet = (DmFPropSheetPtr)client_data;
    DmFolderWinPtr	window;
    DmItemPtr		ip;

    if (sheet->shell == NULL)
	return;

    XtDestroyWidget(sheet->shell);
    sheet->shell = NULL;

    window = (DmFolderWinPtr)sheet->window;
    ip = DmObjNameToItem((DmWinPtr)window, sheet->fntfptr->prev);

    XtFree(sheet->fntfptr->prev);
    XtFree(sheet->grptfptr->prev);
    XtFree(sheet->owntfptr->prev);
    XtFree(sheet->cmtptr->prev);
    /* XtFree(sheet->button_items); */
    sheet->prop_num = -1;

    /* unbusy associated icon in the window */
    if (ip && ITEM_MANAGED(ip))
    {
        Cardinal item_index = ip - window->itp;

    	OlVaFlatSetValues(window->box, item_index, XtNbusy, False, NULL);
    }
}

/* ResetTF() function resets the values in the text fields using
   the "prev" field (saved value). It gets called from the callback 
   routine ResetCB().
*/
static void
ResetTF(DmFPropTFPtr tfptr)
{

	char *new_str;

	/* we could do SetValues directly and reset the value in textfiled
	   using 'prev' field. But, this will generate ugly refresh for
	   the fileds whose value may not have been changed
	*/

	XtSetArg(Dm__arg[0], XtNstring, &new_str);
	XtGetValues(tfptr->w, Dm__arg, 1);
	if (strcmp(new_str, tfptr->prev))
		{
		XtSetArg(Dm__arg[0], XtNstring, tfptr->prev);
		XtSetValues(tfptr->w, Dm__arg, 1);
                }
	XtFree(new_str);
}

/* Dm__ResetPermissions() resets the permissions in the permission buttons
   using the info. saved in rpre, wprev, and eprev for each perm.
   categories.
*/
static void
Dm__ResetPermissions(permp)
DmFPropPermsPtr permp;
{
	Boolean set_val;

	/* again, we could simply do SetValues using 'prev' value. but it
	   generates ugly refresh even if the item was not changed
	*/
        XtSetArg(Dm__arg[0], XtNset, &set_val);
        OlFlatGetValues(permp->w, 0, Dm__arg, 1);
        if (set_val != permp->rprev)
                {
                XtSetArg(Dm__arg[0], XtNset, permp->rprev);
                OlFlatSetValues(permp->w, 0, Dm__arg, 1);
                }
        XtSetArg(Dm__arg[0], XtNset, &set_val);
        OlFlatGetValues(permp->w, 1, Dm__arg, 1);
        if (set_val != permp->wprev)
                {
                XtSetArg(Dm__arg[0], XtNset, permp->wprev);
                OlFlatSetValues(permp->w, 1, Dm__arg, 1);
                }
        XtSetArg(Dm__arg[0], XtNset, &set_val);
        OlFlatGetValues(permp->w, 2, Dm__arg, 1);
        if (set_val != permp->eprev)
                {
                XtSetArg(Dm__arg[0], XtNset, permp->eprev);
                OlFlatSetValues(permp->w, 2, Dm__arg, 1);
                }
}


/* ResetCB() function is a callback routine to reset the value in
   various fields once the user has made a change and clickedon the "Reset"
   button.
*/
static void
ResetCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmFPropSheetPtr	fpsptr = (DmFPropSheetPtr)client_data;
	DmFolderWinPtr	window = (DmFolderWinPtr)fpsptr->window;

	/* reset textfield values */
	ResetTF(fpsptr->fntfptr);
	ResetTF(fpsptr->owntfptr);
	ResetTF(fpsptr->grptfptr);
	ResetTF(fpsptr->cmtptr);

	/* reset permissions */
	Dm__ResetPermissions(fpsptr->ownptr);
	Dm__ResetPermissions(fpsptr->grpptr);
	Dm__ResetPermissions(fpsptr->othptr);

	fpsptr->flag = True;			/* popup should stay up */
}

/* VerifyCB() function is verification callback invoked by popup
   window widget to confirm whether it is ok to popdown the window.
*/
static void
VerifyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFPropSheetPtr	sheet = (DmFPropSheetPtr)client_data;

    if (sheet->flag)				/* should popup stay up? */
    {
	Boolean * ok = (Boolean *)call_data;

	*ok = sheet->flag = False;
    }
}

/****************************procedure*header*****************************
    HelpCB- displays help on file property sheet.
*/
static void
HelpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmFPropSheetPtr	fpsptr = (DmFPropSheetPtr)client_data;
	DmHelpAppPtr help_app = DmGetHelpApp(FOLDER_HELP_ID(Desktop));

	DmDisplayHelpSection(&(help_app->hlp_win), help_app->app_id, NULL,
			     "DesktopMgr/folder.hlp", "640",
			     UNSPECIFIED_POS, UNSPECIFIED_POS);

	fpsptr->flag = True;			/* popup should stay up */

} /* end of HelpCB */

/***************************private*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    Dm__PopupFilePropSheets() function gets called from the "Properties.."
	callback routine. It goes thru the selected item, sets 'em busy and
	calls routine to create file property sheet. If the property sheet
	for a window was already up, it simply raises it and returns.
*/
void
Dm__PopupFilePropSheets(DmFolderWinPtr window)
{
    DmItemPtr	item;
    Cardinal	i;

    /* go thru' each selected item and see if prop. needs to be createed */
    for(i = 0, item = window->itp; i < window->nitems; i++, item++)
	if ((ITEM_MANAGED(item)) && (ITEM_SELECT(item)))
	    Dm__PopupFilePropSheet(window, item);

} /* end of Dm__PopupFilePropSheets */

/****************************procedure*header*****************************
    Dm__PopupFilePropSheet- create (if necessary) and popup a property sheet
	for the item pointed to by 'item'.
*/
void
Dm__PopupFilePropSheet(DmFolderWinPtr window, DmItemPtr item)
{
    Cardinal		indx = item - window->itp;
    DmFPropSheetPtr	sheet;
    Widget		popup;

    /* If sheet is already up, raise it.  Otherwise, create it and popup it
       up.
    */
    for (sheet = window->fpsptr; sheet != NULL; sheet = sheet->next)
	if((sheet->prop_num != -1) &&
	   (!strcmp(sheet->fntfptr->prev, (ITEM_OBJ(item))->name)))
	{
	    XRaiseWindow(XtDisplay(sheet->shell), XtWindow(sheet->shell));
	    return;
	}

    /* Sheet not found.  Create one and popup it up */

    /* Make item busy (and refresh it!) */
    OlVaFlatSetValues(window->box, indx, XtNbusy, True, NULL);

    popup = CreateSheet(window, item);
    XtPopup(popup, XtGrabNone);

     /* register help for window */
     help_info.app_title = Dm__gettxt(TXT_FOLDER_TITLE);
     help_info.filename  = "DesktopMgr/folder.hlp";
     help_info.section   = "640";
     help_info.path      = NULL;
     help_info.title     = NULL;

     OlRegisterHelp(OL_WIDGET_HELP, popup, NULL, OL_DESKTOP_SOURCE,
          (XtPointer)&help_info);

    /* XRaiseWindow(XtDisplay(fps), XtWindow(fps)); */

}				/* end of Dm__PopupFilePropSheet */

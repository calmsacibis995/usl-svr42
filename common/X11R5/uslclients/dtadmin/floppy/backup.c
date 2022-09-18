/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:floppy/backup.c	1.8.2.43"
#endif

#include <fcntl.h>
#include <pwd.h>
#include <time.h>
#include <string.h>
#include <sys/utsname.h>
#include "media.h"

extern	void	InsertNotice();
extern	void	SetLocaleTags();
extern	char	*GetXWINHome();
static	void	CheckCpio(int blocks, XtIntervalId intid);
extern	char	*nbfgets (char *buf, int cnt, FILE *pFile);

extern	long	_dtam_flags;
extern	char	*CpioCmd;
extern  ExitValue success_flag;
extern  int	restricted_flag;
static	int	Blocks;

void	bhelpCB();
void	bkillCB();
void	openCB();
void	saveCB();
void	saveasCB();
void	dosaveCB();
void	cancelSaveCB();
void	ucancelCB();
void	doopenCB();
void	cancelOpenCB();
void	backupCB();
void	schedCB();
void	testCB();
void	excludeCB();
Boolean	BkupScript();
void	ResetIconBox();
void	MakeUserList();
void	BackupLaunchCB();

char	*CpioLine();

static MenuItems bfile_menu_item[] = {
	{ TRUE, label_open,  mnemonic_open, 0, openCB},
	{ TRUE, label_save,  mnemonic_save, 0, saveCB},
	{ TRUE, label_saveas,mnemonic_saveas, 0, saveasCB},
	{ TRUE, label_exit,  mnemonic_exit, 0, exitCB},
	{ NULL }
};

static MenuItems bbkup_menu_item[] = {
	{ TRUE, label_immed,  mnemonic_immed, 0, backupCB},
	{ TRUE, label_sched,  mnemonic_sched, 0, schedCB},
	{ TRUE, label_exclude,mnemonic_exclude, 0, excludeCB},
	{ NULL }
};

static HelpInfo BHelpBackup	= { 0, "", BHELP_PATH, help_intro };
static HelpInfo BHelpTOC	= { 0, "", BHELP_PATH, NULL };
static HelpInfo BHelpDesk	= { 0, "", BHELP_PATH, "HelpDesk"  };
static HelpInfo BHelpOpen	= { 0, "", BHELP_PATH, help_bkup_open };
static HelpInfo BHelpSave	= { 0, "", BHELP_PATH, help_bkup_save };
static HelpInfo BHelpConfirm	= { 0, "", BHELP_PATH, help_bkup_confirm };

static MenuItems bhelp_menu_item[] = {  
	{ TRUE, label_bkrst, mnemonic_bkrst, 0, bhelpCB, (char *)&BHelpBackup },
	{ TRUE, label_toc,   mnemonic_toc,   0, bhelpCB, (char *)&BHelpTOC },
	{ TRUE, label_hlpdsk,mnemonic_hlpdsk,0, bhelpCB, (char *)&BHelpDesk },
	{ NULL }
};

static MenuGizmo bfile_menu = {0, "file_menu", NULL, bfile_menu_item};
static MenuGizmo bbkup_menu = {0, "bkup_menu", NULL, bbkup_menu_item};
static MenuGizmo bhelp_menu = {0, "help_menu", NULL, bhelp_menu_item};

static MenuItems bmain_menu_item[] = {
	{ TRUE, label_file,   mnemonic_file, (Gizmo) &bfile_menu},
	{ TRUE, label_backup, mnemonic_backup, (Gizmo) &bbkup_menu},
	{ TRUE, label_help,   mnemonic_help, (Gizmo) &bhelp_menu},
	{ NULL }
};
static MenuGizmo bmenu_bar = { 0, "menu_bar", NULL, bmain_menu_item,
   				NULL, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM };

BaseWindowGizmo bbase = {0, "base", label_backup, (Gizmo)&bmenu_bar,
	NULL, 0, label_backup, "backup48.icon", " ", " ", 90 };

static MenuItems bwatch_menu_item[] = {  
	{ TRUE, label_cancel,  mnemonic_cancel, 0, bkillCB, NULL },
	{ TRUE, label_help,    mnemonic_help, 0,bhelpCB,(char *)&BHelpConfirm },
	{ NULL }
};
static MenuGizmo bwatch_menu = {0, "bwatch_menu", NULL, bwatch_menu_item};
static PopupGizmo bwatch = {0, "popup", title_doingBkup, (Gizmo)&bwatch_menu};

static MenuItems bnote_menu_item[] = {  
	{ TRUE, label_continue, mnemonic_continue, 0, testCB, NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, bkillCB, NULL },
	{ TRUE, label_help,   mnemonic_help, 0, bhelpCB,(char *)&BHelpConfirm },
	{ NULL }
};
static MenuGizmo bnote_menu = {0, "bnote_menu", NULL, bnote_menu_item};
static ModalGizmo bnote = {0, "", title_confirmBkup, (Gizmo)&bnote_menu};

static MenuItems buser_menu_item[] = {  
	{ TRUE, label_cancel, mnemonic_cancel, 0, ucancelCB, NULL },
	{ TRUE, label_help,   mnemonic_help, 0, bhelpCB, (char *)&BHelpTOC },
	{ NULL }
};
static MenuGizmo buser_menu = {0, "buser_menu", NULL, buser_menu_item};
static PopupGizmo buser = {0, "", title_bkupUsers, (Gizmo)&buser_menu};

static MenuItems bsave_menu_item[] = {  
	{ TRUE, label_save,   mnemonic_save, 0, dosaveCB, NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, cancelSaveCB, NULL },
	{ TRUE, label_help,   mnemonic_help, 0, bhelpCB, (char *)&BHelpSave },
	{ NULL }
};
static MenuGizmo bsave_menu = {0, "bsave_menu", NULL, bsave_menu_item};
static FileGizmo save_prompt = {0, "", title_bkupSave, (Gizmo)&bsave_menu, NULL,
				"", NULL, FOLDERS_AND_FILES, NULL };

static MenuItems bopen_menu_item[] = {  
	{ TRUE, label_open,   mnemonic_open, 0, doopenCB, NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, cancelOpenCB, NULL },
	{ TRUE, label_help,   mnemonic_help, 0, bhelpCB, (char *)&BHelpOpen },
	{ NULL }
};
static MenuGizmo bopen_menu = {0, "bopen_menu", NULL, bopen_menu_item};
static FileGizmo open_prompt = {0, "", title_bkupOpen, (Gizmo)&bopen_menu, NULL,
				"", NULL, FOLDERS_AND_FILES, NULL };

static MenuItems mounted_menu_item[] = {
	{ TRUE, label_exit,  mnemonic_exit, 0, exitCB, NULL},
	{ NULL }
	};
static MenuGizmo mounted_menu = {0, "mounted_menu", NULL, mounted_menu_item};
static ModalGizmo mounted = {0, "", title_mounted, (Gizmo)&mounted_menu};

static MenuItems in_use_menu_item[] = {
	{ TRUE, label_overwrite, mnemonic_overwrite, 0, BackupLaunchCB, NULL },
	{ TRUE, label_exit,  mnemonic_exit, 0, exitCB, NULL},
	{ NULL }
	};
static MenuGizmo in_use_menu = {0, "in_use_menu", NULL, in_use_menu_item};
static ModalGizmo in_use = {0, "", title_in_use, (Gizmo)&in_use_menu};

extern	char	*HALT_tag;
extern	char	*ERR_tag;
extern	char	*ERR_fld;
extern	char	*END_tag;

#define	ROOT		"/"

char	IGNORE[]	= "Ignore";
char	BKUP_HIST[]	= ".lastbackup";
char	INCR_HIST[]	= ".lastpartial";
char	BKUP_LOG[]	= "backuplog";
char	XARGS[]		= "xargs -i find {}";

FILE	*fmtfp;
FILE	*log;

#define WIDTH	(45*x3mm)
#define	HEIGHT	(32*y3mm)

#define INIT_X  32
#define INIT_Y  24
#define INC_X   72
#define INC_Y   24
#define	MARGIN	24

#define	SHELL_LINE  "#!/bin/sh"
#define	MARKER_LINE "#Backup Script File - DO NOT EDIT"

Dimension	ibx = INIT_X, iby = INIT_Y;

Widget		w_bkmsg, w_class, w_type, w_target,
		w_icons, w_user, w_log, w_bkmenu, w_bdesc, w_devmenu;

XtIntervalId	TID = 0;

DmItemPtr	b_itp;
DmContainerRec	b_cntrec;
DmFclassRec	doc_fcrec,
		dir_fcrec;

char		*bkup_cmd = NULL;
char		*bkup_doc = NULL;
char		*bdoc_alias;
char		*copy_source = NULL;
char		*bkup_source = NULL;
char		*user_source = NULL;
char		*user_home = NULL;
char		*system_home = "/etc";
char		*EOM = "EoM %d";
char		*home;
char		flpindex[] = "/tmp/flp_index";
char		pid[8];
char		*indexscript;
char		*pidindex;
char		*ptyname;
FILE		*ptyfp;
Boolean		owner;

pid_t		child_pgid = 0;

#define	B_IMMEDIATE	0
#define	B_SCRIPT	1

#define	BKUP_COMPL	0
#define	BKUP_INCR	1
#define	BKUP_FILES	2
#define HERE_DOC_WORD	"ThIsShOuLdNotCoLiDeWiThAnYfIlEnAmE"

static const char *FindErrorOutput = "FindCommandGeneratedErrors";
Boolean     FindErrors = FALSE;

Boolean		save_log = FALSE;
Boolean		do_sched = FALSE;
Boolean		index_ready = FALSE;

int		bkup_type = BKUP_COMPL;

#define	CLS_SYSTEM	0
#define	CLS_SELF	1
#define	CLS_USERS	2

int		bkup_class = CLS_SELF;
int		bkup_count = 0;
char		*user = NULL;
FileList	BkupTarget = NULL;	

DevItem		BkupDevice[N_DEVS];

typedef	struct	{
	char	*u_name;
	char	*u_home;
} UserItem, *UserList;

UserList	users = (UserList)NULL;
int		user_count = 0;

char		*UserFields[] = { XtNlabel, XtNuserData };

#define	BUNCH	32
#define	QUANTUM	(BUNCH*sizeof(UserItem))

char		*volume;
int		vol_count;

Boolean		wait = FALSE;
Boolean		formatting = FALSE;

char		*type_label[3];

static	char *
SummaryMsg(void)
{
struct	utsname	sysname;
static	char	*ptr, buf[128];
	char	*contents;

	if (bkup_type == BKUP_FILES)
		contents = "";
	else if (bkup_class == CLS_SYSTEM) {
		uname(&sysname);
		contents = sysname.nodename;
	}
	else
		contents = bkup_source;
	ptr = curdev? DtamDevAlias(curdev): bkup_doc;
	sprintf(buf, GetGizmoText(string_bkupSummary), type_label[bkup_type],
			GetGizmoText(label_backup), ptr, contents);
        if ((ptr != NULL) && curdev)
	    FREE(ptr);
	return buf;
}

static	void
bhelpCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	HelpInfo *help = (HelpInfo *) client_data;

	FooterMsg(bbase, NULL);
	help->app_title	= 
	help->title	= GetGizmoText(label_backup);
	help->section = GetGizmoText(STRDUP(help->section));
	PostGizmoHelp(bbase.shell, help);
}

static	void
DropBkupWatch(XtPointer closure, XtIntervalId id)
{
	XDefineCursor(theDisplay, XtWindow(bwatch.shell),
                  				GetOlStandardCursor(theScreen));
	XDefineCursor(theDisplay, XtWindow(w_gauge),
                  				GetOlStandardCursor(theScreen));
	XDefineCursor(theDisplay, XtWindow(bbase.shell),
                  				GetOlStandardCursor(theScreen));
	if (bwatch.shell && !closure )
		BringDownPopup(bwatch.shell);
	if (bnote.shell)
		BringDownPopup(bnote.shell);
}

static	void
StartBkupWatch()
{
	Widget	w_up;

	if (!bwatch.shell) {
		CreateGizmo(bbase.shell, PopupGizmoClass, &bwatch, NULL, 0);

		XtSetArg(arg[0], XtNupperControlArea, &w_up);
		XtGetValues(bwatch.shell, arg, 1);

		XtSetArg(arg[0], XtNwindowHeader, FALSE);
		XtSetValues(bwatch.shell, arg, 1);

		XtSetArg(arg[0], XtNlayoutType,		OL_FIXEDCOLS);
		XtSetArg(arg[1], XtNalignCaptions,	TRUE);
		XtSetArg(arg[2], XtNcenter,		TRUE);
		XtSetArg(arg[3], XtNwidth,		36*x3mm);
		XtSetArg(arg[4], XtNhPad,		x3mm);
		XtSetArg(arg[5], XtNvPad,		y3mm);
		XtSetArg(arg[6], XtNvSpace,		y3mm);

		XtSetValues(w_up, arg, 7);

		XtSetArg(arg[0], XtNalignment,	 OL_CENTER);
		XtSetArg(arg[1], XtNgravity,	 CenterGravity);
		XtSetArg(arg[2], XtNwidth,	 32*x3mm);
		XtSetArg(arg[3], XtNfont, 	 bld_font);

		w_bkmsg = XtCreateManagedWidget("text",
				staticTextWidgetClass, w_up, arg, 4);

		XtSetArg(arg[0], XtNheight, 2*y3mm);
		XtCreateManagedWidget("spacer", rectObjClass, w_up, arg, 1);

		XtSetArg(arg[0], XtNspan,		32*x3mm);
		XtSetArg(arg[1], XtNmappedWhenManaged,	FALSE);
		XtSetArg(arg[2], XtNorientation,	OL_HORIZONTAL);
		XtSetArg(arg[3], XtNminLabel,		" 0 %");
		XtSetArg(arg[4], XtNmaxLabel,		"100 %  ");
		XtSetArg(arg[5], XtNsliderMax,		100);
		XtSetArg(arg[6], XtNsliderValue,	0);
		XtSetArg(arg[7], XtNshowValue,		TRUE);
		XtSetArg(arg[8], XtNtickUnit,		OL_PERCENT);
		XtSetArg(arg[9], XtNticks,		10);

	 	w_gauge = XtCreateManagedWidget("gauge",
				gaugeWidgetClass, w_up, arg, 10);
	}
	XtSetArg(arg[0],  XtNstring, GetGizmoText(string_waitIndex));
	XtSetValues(w_bkmsg, arg, 1);
	MapGizmo(PopupGizmoClass, &bwatch);
	XDefineCursor(theDisplay, XtWindow(bwatch.shell),
					GetOlBusyCursor(theScreen));
	XDefineCursor(theDisplay, XtWindow(w_gauge),
					GetOlBusyCursor(theScreen));
	XDefineCursor(theDisplay, XtWindow(bbase.shell),
					GetOlBusyCursor(theScreen));
}

static	void
ucancelCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(buser.shell);
}

static	void
bkillCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	char	buf[80];

	_Dtam_p3close(cmdfp, SIGTERM);
	cmdfp[0] = cmdfp[1] = (FILE *)NULL;
	if (ptyfp) {
		fclose(ptyfp);
		ptyfp = (FILE *)NULL;
	}
	sprintf(buf, GetGizmoText(string_opKilled), GetGizmoText(label_backup));
	if (log) {
		fprintf(log, "%s\n", buf);
		fclose(log);
		log = (FILE *)NULL;
	}
	if (gauge_id) {
		XtRemoveTimeOut(gauge_id);
		gauge_id = 0;
	}
	if (w_gauge) {
		XtSetArg(arg[0], XtNsliderValue,	0);
		XtSetArg(arg[1], XtNmappedWhenManaged,	FALSE);
		XtSetValues(w_gauge, arg, 2);
	}
	if (TID) {
		XtRemoveTimeOut(TID);
		TID = 0;
	}
	FooterMsg(bbase, buf);
	DropBkupWatch((XtPointer)NULL, (XtIntervalId)NULL);
}

static	void
OpenFile(void)
{
    FILE	 *fp;
    char	 *ptr, *source, buf[BUFSIZ], *end, *target=NULL, *desc;
    int           Cset = 0, Tset = 0, Hset = 0, Sset = 0, i;
    Boolean       allSet = False;
    Boolean	  first;

    if ((fp=fopen(cur_file,"r")) == NULL) {
	sprintf(buf, GGT(string_opFailed), GGT(label_open));
	SetFileGizmoMessage(&open_prompt, buf);
	return;
    }

    first = True;
    while (nbfgets(buf, BUFSIZ-1, fp) != NULL)
    {
	if (first == True)
	{   /* read first few lines to make sure this is script file */
	    first = False;
	    if ((strncmp(buf,SHELL_LINE,strlen(SHELL_LINE)) != 0) ||
		(nbfgets (buf, BUFSIZ-1, fp) == NULL) ||
		(strlen(buf) != 1) || buf[0] != '\n'  ||
		(nbfgets (buf, BUFSIZ-1, fp) == NULL) ||
	    	(strncmp(buf,MARKER_LINE,strlen(MARKER_LINE)) != 0) ||
		(nbfgets (buf, BUFSIZ-1, fp) == NULL) ||
		(strlen(buf) != 1) || buf[0] != '\n'  ||
		(nbfgets (buf, BUFSIZ-1, fp) == NULL) ||
		(strncmp(buf,"Class=",6) != 0))
	    {
		SetFileGizmoMessage(&open_prompt,
			GetGizmoText(string_notScript));
		MapGizmo(FileGizmoClass, &open_prompt);
    	    	fclose(fp);
		return;
	    }
	}

	if ((ptr = strchr(buf, '=')) == NULL)
	    continue;
	ptr++;

	switch (*buf)
	{
	case 'C':
	    bkup_class = atoi(ptr);
	    Cset++;
	    break;
	case 'T':
	    bkup_type = atoi(ptr);
	    Tset++;
	    break;
	case 'H':
	    if ((end = strpbrk(ptr, " \t\n")) != NULL)
		*end = '\0';
	    home = STRDUP(ptr);	/* used to be ptr to static string */
	    Hset++;
	    break;
	case 'S':
            if ((end = strpbrk(ptr, " \t\n")) != NULL)
		*end = '\0';
	    if (source = DtamGetDev(ptr, FIRST)) /* single = is correct here */
		target = NULL;
	    else
		target = STRDUP(ptr);
	    Sset++;
	    break;
	}
	if (Cset && Tset && Hset && Sset) /* all ok */
	{
	    allSet = True;
	    break;
	}
    }
    if (allSet)			/* then read in filenames */
    {
	allSet = False;		/* need at least one filename */
	/* skip to the first filename */
	while(nbfgets(buf,BUFSIZ-1,fp))
	{
	    if (strstr(buf, HERE_DOC_WORD) == NULL)
		continue;
	    else
		break;
	}
	if (copy_source)
	    FREE(copy_source);
	copy_source = STRDUP(" ");
	while (nbfgets(buf, BUFSIZ-1, fp))
	{
	    if (strstr(buf, HERE_DOC_WORD) != NULL)
		break;
	    allSet = True;
	    if ((end = strpbrk(ptr, "\n")) != NULL)
		*end = '\0';
	    copy_source = (char *)REALLOC(copy_source,
					  strlen(copy_source)+strlen(buf)+2);
	    strcat(strcat(copy_source, buf), " ");
	}
	bkup_source = copy_source;
    }
    if (allSet)
    {
	BringDownPopup(open_prompt.shell);
	if (source)
	{			/*it's a device */
	    curdev = source;
	    curalias = DtamDevAttr(curdev, ALIAS);
/*          curalias = DtamMapAlias(DtamDevAlias(curdev)); */
	    desc = DtamDevDesc(curdev);
	    XtSetArg(arg[0], XtNstring, "");
	    XtSetValues(w_target, arg, 1);
	    XtSetArg(arg[0], XtNmappedWhenManaged, False);
	    XtSetValues(XtParent(w_target), arg, 1);
	    for (i=1; i< N_DEVS; i++)
	    {
		if (strcmp(BkupDevice[i].label, curalias) == 0)
		{
		    XtSetArg(arg[0], XtNset, True);
		    OlFlatSetValues(w_devmenu, i, arg, 1);
		    break;
		}
	    }
	}
	else
	{
	    curdev = NULL;
	    desc = curalias = bdoc_alias;
	    bkup_doc = target;
	    XtSetArg(arg[0], XtNstring, bkup_doc);
	    XtSetValues(w_target, arg, 1);
	    XtSetArg(arg[0], XtNmappedWhenManaged, True);
	    XtSetValues(XtParent(w_target), arg, 1);
	    XtSetArg(arg[0], XtNset, True);
	    OlFlatSetValues(w_devmenu, 0, arg, 1);
		
	}

        XtSetArg(arg[0], XtNlabel, (XtArgVal)desc);
        XtSetValues(w_bdesc, arg, 1);

	XtSetArg(arg[0], XtNset, TRUE);
	OlFlatSetValues(w_type,  bkup_type,  arg, 1);
	OlFlatSetValues(w_class, bkup_class, arg, 1);
	FooterMsg(bbase, SummaryMsg());
	ResetIconBox();
    }
    else
    {
	if (target != NULL)
	    FREE(target);
	SetFileGizmoMessage(&open_prompt,GetGizmoText(string_notScript));
	MapGizmo(FileGizmoClass, &open_prompt);
    }
    fclose(fp);
}

static	void
doopenCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
        if (cur_file)
	    FREE(cur_file);
	cur_file = GetFilePath(&open_prompt);
	OpenFile();
}

static	void
cancelOpenCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(open_prompt.shell);
	FooterMsg(bbase, NULL);
}

static	void
openCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	FooterMsg(bbase, NULL);
	SetFileGizmoMessage(&open_prompt, "");
	SetFileCriteria(&open_prompt, NULL, "");
	MapGizmo(FileGizmoClass, &open_prompt);
}

static	void
saveCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	FILE	*fp;
	char	*ptr, buf[PATH_MAX + 80];
	Boolean ok;

	if (!cur_file)
		saveasCB(wid, client_data, call_data);
	else {
		if ((ptr=strstr(cur_file,".bkup")) == NULL
		|| ptr[5] != '\0')
		{
		    cur_file = (char *) REALLOC(cur_file,
						strlen(cur_file) + 6);
		    strcat(cur_file, ".bkup");
		}
		if ((fp = fopen(cur_file, "w")) == NULL)
		{
		    sprintf(buf, GetGizmoText(string_cantWriteScript),
			    GetGizmoText(label_save), cur_file);
		    FooterMsg(bbase, buf);
		    return;
		}		    
		ok = BkupScript(fp);
		chmod(cur_file, 00755);
		if (ok == False)
		    return;
		sprintf(buf, GetGizmoText(string_savedAs), cur_file);
		FooterMsg(bbase, buf);
	}
}

static	void
cancelSaveCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(save_prompt.shell);
	FooterMsg(bbase, NULL);
}

static	void
dosaveCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
        if (cur_file)
	    FREE(cur_file);
	cur_file = GetFilePath(&save_prompt);
	BringDownPopup(save_prompt.shell);
	saveCB(wid, client_data, call_data);
}

static	void
saveasCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	FooterMsg(bbase, NULL);
	if (!save_prompt.shell)
		CreateGizmo(bbase.shell, FileGizmoClass, &save_prompt, NULL, 0);
	SetFileCriteria(&save_prompt, NULL, "");
	MapGizmo(FileGizmoClass, &save_prompt);
}

static	void
excludeCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	int	n, max;
	char	*ptr, *name;
	Boolean	set, change = FALSE;
   	char	delimitedName[PATH_MAX + 3] = " ";
  	char	*dnp = delimitedName, *dnp1 = &delimitedName[1];
	
	FooterMsg(bbase, NULL);
	XtSetArg(arg[0], XtNnumItems, &max);
	XtGetValues(w_icons, arg, 1);
	for (n = 0; n < max; n++) {
		XtSetArg(arg[0], XtNlabel,	&name);
		XtSetArg(arg[1], XtNset,	&set);
		OlFlatGetValues(w_icons, n, arg, 2);
		if (set) {
			change = TRUE;
   			strcat(strcpy(dnp1, name), " ");  /* " name " */
   			if (ptr=strstr(copy_source, dnp)) {
   				ptr++;
				while (*ptr && *ptr != ' ')
					*ptr++ = ' ';
			}
		}
	}
	if (change) {
		ResetIconBox();
	}
	else
		FooterMsg(bbase, GetGizmoText(string_noneset));
}

static	Boolean
WriteableMedium(void)
{
	char	buf[128];

	if (_dtam_flags & DTAM_READ_ONLY) {
		char	*dev = DtamDevAlias(curdev);
		sprintf(buf, GetGizmoText(string_cantWrite), dev);
		InsertNotice(&bnote, 0);
		XtVaSetValues(bnote.stext, XtNstring, buf, NULL);
		FREE(dev);
		return FALSE;
	}
	return TRUE;
}

static	void
testCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	int	diagn;
	char	*ptr, buf[80];

	switch (diagn = DtamCheckMedia(curalias)) {
	case DTAM_NO_DISK:
			break;		/* go beep? */
	case DTAM_UNFORMATTED:
			sprintf(buf, "%s -F -X -D %s &",
				GetXWINHome("bin/MediaMgr"), curalias);
			FooterMsg(bbase, ptr=GetGizmoText(string_doingFmt));
			if (w_bkmsg) {
				XtSetArg(arg[0], XtNstring, ptr);
				XtSetValues(w_bkmsg, arg, 1);
			}
			XSync(theDisplay, FALSE);
			fmtfp = popen(buf, "r");
			fcntl(fileno(fmtfp), F_SETFL, O_NONBLOCK);
			formatting = TRUE;
			BringDownPopup(bnote.shell);
			break;

	case DTAM_UNKNOWN:
	case DTAM_UNREADABLE:
		if (WriteableMedium())
			BringDownPopup(bnote.shell);
			wait = FALSE;
			if (vol_count > 1) { 
				fputs("\n", ptyfp);
				fflush(ptyfp);
			}
			break;
	default:
		if (diagn > 0) {
		    BringDownPopup(bnote.shell);
		    if (!in_use.shell)
		      CreateGizmo(bbase.shell,ModalGizmoClass, &in_use,NULL,0);
		    SetModalGizmoMessage(&in_use,GetGizmoText(string_in_use));
		    MapGizmo(ModalGizmoClass,&in_use);
    		    return;	/* check this !! */
		}
	}
}

static	void
MakeUserList()
{
	struct	passwd	*getpwent();
	struct	passwd	*pw;

	while (pw = getpwent()) {
		if (pw->pw_uid >= 100 && pw->pw_uid < UID_MAX-2) {
			if (user_count == 0)
				users = (UserList)MALLOC(QUANTUM);
			else if (user_count % BUNCH == 0)
				users = (UserList)REALLOC(users,
					(1+(user_count/BUNCH))*QUANTUM);
			users[user_count].u_name = STRDUP(pw->pw_name);
			users[user_count++].u_home = STRDUP(pw->pw_dir);
		}
	}
	endpwent();
}

static	void
selUserCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData	*olcd = (OlFlatCallData *) call_data;
	char		*name, *home_dir;

	XtSetArg(arg[0], XtNlabel, &name);
	XtSetArg(arg[1], XtNuserData, &home_dir);
	OlFlatGetValues(wid, olcd->item_index, arg, 2);

	user = name;
	home = bkup_source = home_dir;

	FooterMsg(bbase, SummaryMsg());
	ResetIconBox();
}

static	void
CreateUserPopup(Widget parent)
{
	Widget	w_up, w_tmp;

	CreateGizmo(parent, PopupGizmoClass, &buser, NULL, 0);

	XtSetArg(arg[0], XtNupperControlArea, &w_up);
	XtGetValues(buser.shell, arg, 1);

	w_tmp = XtCreateManagedWidget("scrolledWindow",
			scrolledWindowWidgetClass, w_up, NULL, 0);

	XtSetArg(arg[0], XtNviewHeight,	 	5);
	XtSetArg(arg[1], XtNnoneSet,		TRUE);
	XtSetArg(arg[2], XtNformat,		"%s");
	XtSetArg(arg[3], XtNitemFields,		UserFields);
	XtSetArg(arg[4], XtNnumItemFields,	2);
	XtSetArg(arg[5], XtNitems,		users);
	XtSetArg(arg[6], XtNnumItems,		user_count);
	XtSetArg(arg[7], XtNselectProc,		selUserCB);
	XtSetArg(arg[8], XtNexclusives,		TRUE);

	XtCreateManagedWidget("userList", flatListWidgetClass, w_tmp, arg, 9);
}

static	FileList
MakeObjectList(char *source, int *count)
{
struct	stat		st_buf;
register int		n;
	char		*ptr, *str;
	FileList	bklist = NULL;

	if (source)
	    str = STRDUP(source);
	else
	    str = NULL;
	for (n=0, ptr=strtok(str," ,\n"); ptr; ptr=strtok(NULL," ,\n")) {
		if (bklist)
			bklist=(FileList)REALLOC(bklist,(n+1)*sizeof(FileObj));
		else
			bklist=(FileList)MALLOC(sizeof(FileObj));
		bklist[n].bk_path = STRDUP(ptr);
		stat(ptr, &st_buf);
		bklist[n].bk_type = (st_buf.st_mode & S_IFDIR)?'d':'f';
		n++;
	}
	*count = n;
	if (str)
	    FREE(str);
	return bklist;
}

int	item_count = 0;

static	DmObjectPtr
AddItem(FileList b)
{
	DmObjectPtr	optr;

	optr = (DmObjectPtr)CALLOC(1, sizeof(DmObjectRec));
	optr->container = &b_cntrec;
	if (b->bk_type == 'd')
		optr->fcp = &dir_fcrec;
	else
		optr->fcp = &doc_fcrec;
	optr->name = b->bk_path;	/* maybe just basename? */
        optr->x = UNSPECIFIED_POS;
	optr->y = UNSPECIFIED_POS;
	optr->objectdata = (XtPointer)b;
	if (item_count++ == 0) {
		b_cntrec.op = optr;
	}
	else {
		DmObjectPtr endp = b_cntrec.op;
		while (endp->next)
			endp = endp->next;
		endp->next = optr;
	}
	b_cntrec.num_objs = item_count;
	return optr;
}

void
BackupTakeDrop(Widget wid, XtPointer client_data, XtPointer call_data)
{
	DtDnDInfoPtr    dip = (DtDnDInfoPtr)call_data;
	char		**p, *name, *ptr;
   	char		delimitedName[PATH_MAX + 3] = " ";
   	char		*dnp = delimitedName, *dnp1 = &delimitedName[1];

   	if (copy_source == NULL)
   		copy_source = STRDUP(" ");

	if (dip->files && *dip->files && **dip->files) {
		for (p=dip->files; *p; p++) {
			name = *p;
			if (name[0] == '/' && name[1] == '/')
				name++;
   			strcat(strcpy(dnp1, name), " ");  /* " name " */
   			if (ptr=strstr(copy_source, dnp))
				continue;
			else {
				copy_source = (char *)REALLOC(copy_source,
					strlen(copy_source)+strlen(name)+2);
				strcat(strcat(copy_source, name), " ");
			}
		}
	}
	else
		XRaiseWindow (theDisplay, XtWindow (bbase.shell));
	bkup_source = copy_source;
	FooterMsg(bbase, SummaryMsg());
	ResetIconBox();
}

static Boolean
TriggerNotify(	Widget			w,
		Window			win,
		Position		x,
		Position		y,
		Atom			selection,
		Time			timestamp,
		OlDnDDropSiteID		drop_site_id,
		OlDnDTriggerOperation	op,
		Boolean			send_done,
		XtPointer		closure)
{
	DtGetFileNames(w, selection, timestamp, send_done, BackupTakeDrop,
		       closure);
}

static	Widget
GetIconBox(Widget parent, int count)
{
	int	n;
	Widget	w_box;
	DmItemPtr	item;
	Dimension	width;

	item_count = 0; ibx = INIT_X; iby = INIT_Y;
	for (n = 0; n < count; n++)
		AddItem(BkupTarget+n);

	n = 0;	XtSetArg(arg[0], XtNmovableIcons,	TRUE);
	n++;	XtSetArg(arg[1], XtNminWidth,		1);
	n++;	XtSetArg(arg[2], XtNminHeight,		1);
	n++;	XtSetArg(arg[3], XtNdrawProc,		DmDrawIcon);
	if (bkup_type == BKUP_FILES) {
		n++;	XtSetArg(arg[4], XtNtriggerMsgProc, TriggerNotify);
	}
	w_box = DmCreateIconContainer(parent, DM_B_CALC_SIZE, arg, ++n,
			b_cntrec.op,count,&b_itp,count,NULL,NULL,def_font,1);

	XtVaGetValues(w_box, XtNwidth, &width, 0); 
	width -= MARGIN;	/* FIX: shouldn't be needed:  iconbox bug */

	for (item = b_itp; item < b_itp + count; item++)
	    if (ITEM_MANAGED(item))
	    {
		DmGetAvailIconPos(b_itp, count, ITEM_WIDTH(item),
				  ITEM_HEIGHT(item), width,
				  INC_X, INC_Y,
				  (Position *)&item->x,
				  (Position *)&item->y);
	    }
	XtSetArg(arg[0], XtNitemsTouched, TRUE);
	XtSetValues(w_box, arg, 1);

	return w_box;
}

static	void
ResetIconBox()
{
	XtPointer	items;

	if (BkupTarget)
		FREE(BkupTarget);
	XtUnmanageChild(w_icons);
	XtSetArg(arg[0], XtNitems, &items);
	XtGetValues(w_icons, arg, 1);
	XtDestroyWidget(w_icons);
	BkupTarget = MakeObjectList(bkup_source, &bkup_count);
	w_icons = GetIconBox(bbase.scroller, bkup_count);
}

static	void
SetBkupClass(Widget wid, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData	*olcd = (OlFlatCallData *) call_data;
	char		*ptr;

	XtSetArg(arg[0], XtNlabel, &ptr);
	OlFlatGetValues(wid, olcd->item_index, arg, 1);
	switch (bkup_class = olcd->item_index) {
	    case CLS_SYSTEM:	bkup_source = ROOT;
				home = system_home;
				break;
	    case CLS_USERS:	if (!buser.shell)
					CreateUserPopup(bbase.shell);
				MapGizmo(PopupGizmoClass, &buser);
				return;
				/* i.e., leave resetting of icon box to popup */
	    case CLS_SELF:	home = bkup_source = user_home;
				break;
	}
	bkup_type = BKUP_COMPL;
	XtSetArg(arg[0], XtNset, TRUE);
	OlFlatSetValues(w_type, bkup_type, arg, 1);
	FooterMsg(bbase, SummaryMsg());
	ResetIconBox();
}

static	void
SetBkupType(Widget wid, XtPointer client_data, XtPointer call_data)
{
	char		*ptr;
	OlFlatCallData	*olcd = (OlFlatCallData *) call_data;

	XtSetArg(arg[0], XtNlabel, &ptr);
	OlFlatGetValues(wid, olcd->item_index, arg, 1);
	FooterMsg(bbase,ptr);
	switch (bkup_type=olcd->item_index) {
	    case BKUP_COMPL:
	    case BKUP_INCR:	bkup_source = (bkup_class == CLS_SYSTEM) ?
				    ROOT : home;
				break;
	    case BKUP_FILES:	bkup_source = copy_source;
				break;
	}
	if (bkup_type == BKUP_FILES)
		XtSetArg(arg[0], XtNtriggerMsgProc, TriggerNotify);
	else
		XtSetArg(arg[0], XtNtriggerMsgProc, NULL);
	XtSetValues(w_icons, arg, 1);
	XtSetArg(arg[0], XtNsensitive, bkup_type==BKUP_FILES);
	OlFlatSetValues(w_bkmenu, 2, arg, 1);
	FooterMsg(bbase, SummaryMsg());
	ResetIconBox();
}

static	void
CheckCpio(int blocks, XtIntervalId intid)
{
extern	char	*ctime();
static	FILE	*fp;
static	int	fraction;
static	char	indexline[9+PATH_MAX];
static	Boolean	bkup_done;
	time_t	clk;
	char	*ptr, buf[BUFSIZ];
	int	n, i;
        int     status;

	if (formatting) {
		n = read(fileno(fmtfp), buf, BUFSIZ);
		if (n == 0) {
			formatting = wait = FALSE;
			pclose(fmtfp);
			if (vol_count > 1) {
				fputs("\n", ptyfp);
				fflush(ptyfp);
			}
			i = 100;
		}
		else
			i = 10000; 	/* keep waiting, use a long interval  */
	}
	else if (wait) {
		i = 1000;
	}
	else if (bkup_cmd == NULL) {	/* start off the actual backup */
		int	fdmaster;

		/*
		 *	set up for interaction with cpio; clone a pseudo-tty
		 */
		FooterMsg(bbase, NULL);
		if (curdev == NULL)
			ptyfp = (FILE *)NULL;
		else {
			fdmaster = open("/dev/ptmx", O_RDWR);
			grantpt(fdmaster);
			unlockpt(fdmaster);
			ptyname = ptsname(fdmaster);
			fcntl(fdmaster, F_SETFL, O_NONBLOCK);
			ptyfp = fdopen(fdmaster, "r+");
			setbuf(ptyfp, NULL);
		}
		bkup_cmd = STRDUP(CpioLine(B_IMMEDIATE));
		bkup_done = FALSE;
		_Dtam_p3open(bkup_cmd, cmdfp, TRUE);
		child_pgid = getpgid(_Dtam_p3pid(cmdfp));
		XtSetArg(arg[0], XtNstring, GetGizmoText(string_doingBkup));
		XtSetValues(w_bkmsg, arg, 1);

		if ((fp = fopen(pidindex, "r")) != NULL) {
			SetGauge(fraction = 0);
			XtSetArg(arg[0], XtNmappedWhenManaged, TRUE);
			XtSetValues(w_gauge, arg, 1);
		}
		if (save_log) {
			sprintf(buf, "%s/%s", home, BKUP_LOG);
			log = fopen(buf, "a");
			if (log) {	/* datestamp start of backup */
				fprintf(log, "%s\n", SummaryMsg());
				time(&clk);
				fprintf(log, GetGizmoText(string_startBkup),
						ctime(&clk));
			}
		}
		else
			log = (FILE *)NULL;
		i = curdev? 2500: 250;
	}
	else {
		i = 100;
		*buf = '\0';
		if (ptyfp && nbfgets(buf, BUFSIZ, ptyfp)) {
			if (strncmp(buf, EOM, 4)==0) {
				wait = TRUE;
				vol_count = atoi(buf+4);
				InsertNotice(&bnote, NO_DISK);
				i = 5000;
				rewind(ptyfp);
			}
		}
		*buf = '\0';
		while (nbfgets(buf, BUFSIZ, cmdfp[1])) {
			int	n = 0;
			if (*buf == '\0')
				break;
			if (strstr(buf, END_tag)
			||  strncmp(buf,HALT_tag,n=strlen(HALT_tag))==0) {
			/*
			 *	done! output time stamps
			 */
			        if (n != 0)
				    sprintf(ptr=indexline,
					    GetGizmoText(string_opFailed),
					    GetGizmoText(label_backup));
				else
				    sprintf(ptr=indexline, FindErrors ?
					    GetGizmoText(string_findErrors):
					    GetGizmoText(string_opOK),
					    GetGizmoText(label_backup));
				strcat(strcat(ptr, " - "), buf+n);
				XtSetArg(arg[0], XtNstring, ptr);
				XtSetValues(w_bkmsg, arg, 1);
				FooterMsg(bbase, ptr);
				if (nbfgets(buf, BUFSIZ, cmdfp[1]))
					;/* maybe error message; discard */
				XtSetArg(arg[0], XtNmappedWhenManaged, FALSE);
				XtSetValues(w_gauge, arg, 1);
				status = _Dtam_p3close(cmdfp, 0);
				if (status == 0)
				    success_flag = FindErrors ?
					FindErrs : NoErrs;
				else
				    success_flag = CpioErrs;
				child_pgid = 0;
				fclose(fp);
				if (ptyfp) {
					fclose(ptyfp);
					ptyfp = (FILE *)NULL;
				}
				unlink(pidindex);
				if (bkup_type != BKUP_FILES) {
					sprintf(buf, "echo `date` > %s/%s",
						home, bkup_type==BKUP_INCR?
							INCR_HIST: BKUP_HIST);
					system(buf);
					if (bkup_type == BKUP_COMPL) {
						sprintf(buf, "%/%", home,
								BKUP_INCR);
						unlink(buf);
					}
				}
				if (log) {
					time(&clk);
					fprintf(log, GetGizmoText(string_opOK),
						GetGizmoText(label_backup));
					fprintf(log, " -- %s", ctime(&clk));
					fclose(log);
					log = (FILE *)NULL;
				}
				XtAddTimeOut(2000,
					(XtTimerCallbackProc)DropBkupWatch,
					(XtPointer)FindErrors);
				TID = 0;
				return;
			}
			else if (*buf == '/') {	/* file transfer by cpio */
				/*
				 *	bother: cpio holds links to the end,
				 *	so that they are not reported in order;
				 *	I should do the same, saving their size
				 *	as well, and match against this list as
				 *	cpio gets to them and add to the gauge.
				 *	for now, I'm going to fake it.
				 */
				struct	stat	st;

				FooterMsg(bbase, buf);
				if (log)
					fputs(buf, log);
				if (bkup_done)
					continue;
			next:	nbfgets(indexline, sizeof(indexline), fp);
				if (strstr(indexline, "BLOCKS=")) {
					/*
					 * cpio is getting around to the link
					 * file that we accounted for in order
					 */
					bkup_done = TRUE;
					continue;
				}
				fraction += atoi(indexline);
				SetGauge(100*fraction/blocks);
				ptr = strchr(indexline,'\t')+1;
				if (strcmp(ptr, buf) != 0) {
					stat(ptr, &st);
					if (st.st_nlink > 1)
						goto next;
				}
			}
			else {
			/*
			 *	error condition -- report as notice
			 */
				if (strncmp(buf,ERR_tag,n=strlen(ERR_tag))==0) {
					if (buf[strlen(buf)-1] == '\n')
						buf[strlen(buf)-1] = '\0';
					XtSetArg(arg[0], XtNstring, buf+n);
					XtSetValues(w_bkmsg, arg, 1);
				}
				else {
					XtSetArg(arg[0],XtNstring,
					     GetGizmoText(string_unknownErr));
					XtSetValues(w_bkmsg, arg, 1);
				}
			}
		}
	}
	TID = XtAddTimeOut(i,(XtTimerCallbackProc)CheckCpio,(XtPointer)blocks);
}

static	void
FetchIndex(XtPointer dummy, XtIntervalId intid)
{
extern	int	errno;
struct	stat	st;
	int	media_type;
	char	*ptr, buf[BUFSIZ];
	int	n, blocks;

	if (cmdfp[1] == NULL) {
		/* cancelled! */
		unlink(indexscript);
		return;
	}
	*buf = '\0';
	while (nbfgets(buf, BUFSIZ-1, cmdfp[1])) {
		if (*buf == '\0')
			break;
		if (strncmp(buf, "BLOCKS=", 7) == 0)	/* last line */
			goto done;
		else if (strstr(buf, ERR_fld)) {
			fputs(buf, stderr);
			FooterMsg(bbase, GetGizmoText(string_cantIndex));
			_Dtam_p3close(cmdfp, SIGTERM);
			cmdfp[0] = cmdfp[1] = (FILE *)NULL;
			child_pgid = 0;
			unlink(indexscript);
			DropBkupWatch(NULL, NULL);
			return;
		}
		else if (strstr(buf, FindErrorOutput)) 
		        FindErrors = True;
		else if (strncmp(buf, "dtindex:", 8) == 0) {
			char	path[PATH_MAX];
			/*
			 *	unreadable file; check if symbolic link
			 *	(if so, ignore silently; else warn user)
			 */
			ptr = strrchr(buf,' ')+1;
			ptr[strlen(ptr)-1] = '\0';
			if (stat(ptr, &st) == 0) {
				sprintf(buf,GetGizmoText(string_unreadFile),ptr);
				XtSetArg(arg[0], XtNstring, buf);
				XtSetValues(w_bkmsg, arg, 1);
			}
		}
		else
			FooterMsg(bbase, strchr(buf,'\t')+1);
	}
	TID = XtAddTimeOut(100, (XtTimerCallbackProc)FetchIndex, NULL);
	return;

	/*
	 *	setup for the cpio command, checking directory targets for
	 *	writeability and device targets for appropriate volume present
	 */
done:	_Dtam_p3close(cmdfp, 0);
	cmdfp[0] = cmdfp[1] = (FILE *)NULL;
	child_pgid = 0;
	unlink(indexscript);
	Blocks = blocks = atoi(buf+7);
	if (bkup_cmd) {
		FREE(bkup_cmd);
		bkup_cmd = NULL;
	}
	wait = FALSE;
	if (curdev == NULL)
		n = 125;
	else {
		vol_count = 1;
		if (volume)
			FREE(volume);
		volume = DtamDevAttr(curdev,"volume");
		media_type = DtamCheckMedia(curalias);
		if (_dtam_flags & DTAM_MOUNTED) {
		  if (!mounted.shell)
		    CreateGizmo(bbase.shell,ModalGizmoClass,&mounted,NULL,0);
		    SetModalGizmoMessage(&mounted,GetGizmoText(string_mounted));
		    MapGizmo(ModalGizmoClass, &mounted);
		    return;
		}
		switch (media_type) {

	case NO_DISK:	wait = TRUE;
			InsertNotice(&bnote, NO_DISK);
			n = 10000;
			break;

	case UNFORMATTED: 
			sprintf(buf, "%s -F -X -D %s &",
				GetXWINHome("bin/MediaMgr"), curalias);
			fmtfp = popen(buf, "r");
			fcntl(fileno(fmtfp), F_SETFL, O_NONBLOCK);
			formatting = TRUE;
			n = 10000;
			break;

	case DTAM_UNKNOWN: 
	case DTAM_UNREADABLE: 
			/* DTAM_UNKNOWN: formatted, not "occupied" type (i.e. no data) */
			/* DTAM_UNREADABLE: most likely an empty tape; perhaps bad floppy */
			if (!WriteableMedium())
			 	wait = TRUE;
			n = 250;
			break;

	default:
			if (media_type > 0)
			{
		  	    	if (!in_use.shell)
		    		    CreateGizmo(bbase.shell,ModalGizmoClass,
						&in_use,NULL,0);
		    		SetModalGizmoMessage(&in_use,
						GetGizmoText(string_in_use));
				MapGizmo(ModalGizmoClass,&in_use);
				wait = TRUE;
		    		break;	/* check this !! */
			}
			/* shouldn't get here, but ... */

			/* if (!WriteableMedium())
			*	wait = TRUE;
			 */
			n = 250;
			break;
		}
	}
	TID = XtAddTimeOut(n,(XtTimerCallbackProc)CheckCpio,(XtPointer)blocks);
}


static	void
BackupLaunchCB()
{
	wait = FALSE;
	BringDownPopup(in_use.shell);
	if (vol_count > 1) { 
		fputs("\n", ptyfp);
		fflush(ptyfp);
	}
}


static	char
*CpioLine(int flag)
{
static	char	buf[BUFSIZ];
struct	stat	fbuf;
	char	*dev, *target;

	if (curdev)
		target = DtamDevAttr(curdev, "cdevice");
	else {
		XtSetArg(arg[0], XtNstring, &bkup_doc);
		XtGetValues(w_target, arg, 1);
		if (bkup_doc == NULL || *bkup_doc == '\0') {
			FooterMsg(bbase, GetGizmoText(string_bdocTitle));
			return NULL;
		}
		else if (stat(bkup_doc,&fbuf) == 0) {
			sprintf(buf, GetGizmoText(string_newFile), bkup_doc);
			FooterMsg(bbase, buf);
			return NULL;
		}
		target = bkup_doc;
	}
	*buf = '\0';
	sprintf(buf+strlen(buf),
		"/usr/bin/cut -f2 < %s | %s | %s -odlucvB -O %s",
		pidindex, GetXWINHome ("adm/RemoteFilt"), CpioCmd, target);
	if (curdev) {
		FREE(target);
		if (flag == B_IMMEDIATE)
			sprintf(buf+strlen(buf), " -M \"%s\n\" -G %s",
					 	EOM, ptyname);
	}
	if (flag == B_IMMEDIATE)
		strcat(buf, " 2>&1");
	else {
		sprintf(buf+strlen(buf)," >/tmp/bkupout.%s 2>/tmp/bkuperr.%s\n",
				pid, pid);
		/*
		 *	need to attach here mail on success/failure
		 *	and updating user logs and datefiles.
		 */
		sprintf(buf+strlen(buf), "/usr/bin/rm -f /tmp/*.%s\n", pid);
	}
	return buf;
}

static char *
getSource()
{
           char  buf[BUFSIZ];
    struct stat  fbuf;
    static char	*target;

    if (curdev)
	target = DtamDevAttr(curdev, "cdevice");
    else {
	XtSetArg(arg[0], XtNstring, &bkup_doc);
	XtGetValues(w_target, arg, 1);
	if (bkup_doc == NULL || *bkup_doc == '\0') {
	    FooterMsg(bbase, GetGizmoText(string_bdocTitle));
	    return NULL;
	}
	else if (stat(bkup_doc,&fbuf) == 0) {
	    sprintf(buf, GetGizmoText(string_newFile), bkup_doc);
	    FooterMsg(bbase, buf);
	    return NULL;
	}
	target = bkup_doc;
    }
    return target;
}


static	void
IndexCmd(FILE *scriptfd, int flag)
{
	char	*ptr, buf[PATH_MAX+8];
	Boolean	do_grep = FALSE;

	if (bkup_type != BKUP_FILES) {
		sprintf(buf, "%s/%s", home, IGNORE);
		do_grep = (access(buf,R_OK)==0);
	}
	*buf = '\0';
	if (bkup_type == BKUP_INCR) {
		sprintf(buf, " -newer %s/%s", home, INCR_HIST);
		if (access(buf+8, R_OK) != 0) {
			sprintf(buf+8, "%s/%s", home, BKUP_HIST);
			if (access(buf+8, R_OK) != 0)
				*buf = '\0';
		}
	}
	fprintf(scriptfd,
		"%s%s -print > /tmp/%cFILES.%s 2> /tmp/FIND.%s <<-\"%s\"\n",
		XARGS, 	*buf? buf: " ", do_grep? 'G': 'F', pid, pid,
		HERE_DOC_WORD);
	for (ptr = bkup_source; *ptr== ' '; ptr++)
		;
	for (; *ptr; ptr++)
	{
		if (*ptr == ' ') 
		{
			fputc('\n', scriptfd);
			while (*ptr == ' ')
				++ptr;
		}
		if (*ptr)
			fputc(*ptr, scriptfd);
		else
			ptr--;
	}
	fprintf(scriptfd, "\n%s\n", HERE_DOC_WORD);
	if (do_grep)
		fprintf(scriptfd,
			"fgrep -v -f %s/%s < /tmp/GFILES.%s > /tmp/FFILES.%s\n",
			home, IGNORE, pid, pid);
	fprintf(scriptfd, "if  [ -s /tmp/FIND.%s ]\nthen\n\techo '%s' 1>&2\nfi\n",
		pid, FindErrorOutput);
	fprintf(scriptfd,
		"if [ ! -s /tmp/FFILES.%s ]\nthen\n\techo '%s' 1>&2\nelse\n",
		pid, ERR_fld);
	fprintf(scriptfd, "\t%s -p %s %s\nfi\n",
		GetXWINHome("adm/dtindex"), pid, flag==B_IMMEDIATE? "-v": "");
	fprintf(scriptfd, "/usr/bin/rm -f /tmp/?FILES.%s\n", pid);
	fprintf(scriptfd, "/usr/bin/rm -f /tmp/FIND.%s\n", pid);
}

static	Boolean
BkupScript(FILE *scriptfp)
{
    char *source, *type, *ptr;

    if ((source = getSource()) == NULL)
    {
	char buf[BUFSIZ];

	fclose(scriptfp);
	sprintf(buf, GetGizmoText(string_opFailed),
		GetGizmoText(title_bkupSave));
	FooterMsg(bbase, buf);
		
	return False;
    }
    
    fprintf(scriptfp, "#!/bin/sh\n\n\
#Backup Script File - DO NOT EDIT\n\n\
Class=%d\n\
Type=%d\n\
Home=%s\n\
Source=%s\n\
\n\
if /sbin/tfadmin -t dtbackup 2>/dev/null\n\
then\n\
\tbackupCmd=\"/sbin/tfadmin dtbackup -s $Source -t $Type -h $Home\"\n\
else\n\
\tbackupCmd=\"%s -s $Source -t $Type -h $Home\"\n\
fi\n\
eval $backupCmd << \"%s\"\n",
	    bkup_class, bkup_type, home, source,
	    GetXWINHome("adm/dtbackup.sh"), HERE_DOC_WORD);
    for (ptr = bkup_source; *ptr== ' '; ptr++)
	;
    for (; *ptr; ptr++)
    {
		if (*ptr == ' ')
		{
	    	fputc('\n', scriptfp);
	    	while (*ptr == ' ')
				++ptr;
		}
		if (*ptr)
	    	fputc(*ptr, scriptfp);
		else 
			ptr--;
    }
    fprintf(scriptfp, "\n%s\n", HERE_DOC_WORD);
    if (curdev)
	FREE(source);
    fclose(scriptfp);
    return True;
}

void
NotePidFiles()	/* temporary files common to Backup and Restore */
{
	char	buf[64];

	sprintf(pid, "%d", getpid());
	sprintf(buf, "%s.%s", flpindex, pid);
	pidindex = STRDUP(buf);
	_DtamNoteTmpFile(pidindex);
}

static	void
backupCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
struct	stat	fbuf;
static	Boolean	first = TRUE;
	FILE	*fp;
	char	*ptr, buf[80];

	XtSetArg(arg[0], XtNset, &save_log);
	OlFlatGetValues(w_log, 0, arg, 1);
	if (curdev == NULL) {
		XtSetArg(arg[0], XtNstring, &bkup_doc);
		XtGetValues(w_target, arg, 1);
		if (bkup_doc == NULL || *bkup_doc == '\0') {
			FooterMsg(bbase, GetGizmoText(string_bdocTitle));
			return;
		}
		else if (stat(bkup_doc,&fbuf) == 0) {
			sprintf(buf, GetGizmoText(string_newFile), bkup_doc);
			FooterMsg(bbase, buf);
			return;
		}
	/*	else {
	/*		sprintf(buf, "touch %s", bkup_doc);
	/*		if (system(buf) != 0) {
	/*			if (!bnote.shell) {
	/*				CreateGizmo(bbase.shell,ModalGizmoClass,
	/*						&bnote, NULL, 0);
	/*			}
	/*			sprintf(buf, GetGizmoText(string_cantWrite),
	/*					bkup_doc);
	/*			SetModalGizmoMessage(&bnote, buf);
	/*			MapGizmo(ModalGizmoClass, &bnote);
	/*			return;
	/*		}
	/*	}
	*/}
	FooterMsg(bbase, NULL);
	if (first) {
		/*
		 *	note the temporary files for removal on exit
		 */
		first = FALSE;
		sprintf(buf, "/tmp/%s.bkup", pid);
		indexscript = STRDUP(buf);
		_DtamNoteTmpFile(indexscript);
		sprintf(buf, "/tmp/FFILES.%s", pid);
		_DtamNoteTmpFile(buf);
		buf[5] = 'G';
		_DtamNoteTmpFile(buf);
	}
	if (access(GetXWINHome("adm/dtindex"), X_OK) != 0)
		FooterMsg(bbase, GetGizmoText(string_cantIndex));
	else {
		FindErrors = FALSE;
		StartBkupWatch();
		fp = fopen(indexscript, "w");
		IndexCmd(fp, B_IMMEDIATE);
		fclose(fp);
		chmod(indexscript, 00755);
		_Dtam_p3open(indexscript, cmdfp, TRUE);
		child_pgid = getpgid(_Dtam_p3pid(cmdfp));
		TID = XtAddTimeOut(250, (XtTimerCallbackProc)FetchIndex, NULL);
	}
}

static	void
schedCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	FILE	*fp;
	char	*file, buf[PATH_MAX + 80];

	sprintf(buf, "%s %s.bkup", GetXWINHome("bin/dtsched"), pid);
	file = strstr(buf, pid);
	if ((fp = fopen(file, "w")) == NULL)
        {
	    char error_buf[PATH_MAX + 80];

	    sprintf(error_buf, GetGizmoText(string_cantWriteScript),
		    GetGizmoText(string_scheduling), file);
	    FooterMsg(bbase, error_buf);
	    return;
	}		    
	if (BkupScript(fp) == False)
	    return;
	chmod(file, 00755);

	strcat(buf, "&");
	system(buf);
	FooterMsg(bbase, GetGizmoText(string_callSched));
}

static	void
BdescCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	char		*desc;
	Widget		w_ud;
	OlFlatCallData	*olcd = (OlFlatCallData *) call_data;
	register  int	n = olcd->item_index;

	if (curdev)
		FREE(curdev);
	if (n == 0) {	/* backup to directory */
		desc = curalias = bdoc_alias;
		curdev = NULL;
	}
	else {
		curalias = DtamMapAlias(BkupDevice[olcd->item_index].label);
		curdev = DtamGetDev(curalias, FIRST);
		desc = DtamDevDesc(curdev);
	}
	XtSetArg(arg[0], XtNuserData, &w_ud);
	XtGetValues(wid, arg, 1);
	
	XtSetArg(arg[0], XtNlabel, (XtArgVal)desc);
	XtSetValues(w_ud, arg, 1);

	XtSetArg(arg[0], XtNmappedWhenManaged, n==0);
	XtSetValues(XtParent(w_target), arg, 1);
	if (n == 0)
		OlSetInputFocus(w_target, RevertToNone, CurrentTime);
	FooterMsg(bbase, SummaryMsg());
}

static	void
SetBkupDoc(Widget wid, XtPointer client_data, XtPointer call_data)
{
	struct	stat		fbuf;
	OlTextFieldVerify	*verify = (OlTextFieldVerify *)call_data;
	char			msgbuf[BUFSIZ];

	if (stat(verify->string,&fbuf) == 0) {
		sprintf(msgbuf, GetGizmoText(string_newFile), verify->string);
		FooterMsg(bbase, msgbuf);
	}
	else {
		bkup_doc = STRDUP(verify->string);
		FooterMsg(bbase, SummaryMsg());
	}
}

CreateBackupWindow(Widget parent, char *source)
{
static	ExclItem	BkupClass[3];
static	ExclItem	BkupType[3];
static	MBtnItem	BCheckBox[2];
	Widget		w_ctl, w_cap;
	char		*str, buf[80];
	int		n;

	NotePidFiles();
	SetLocaleTags();
	if (note.shell)
		XtDestroyWidget(note.shell);
	home = user_home = getenv("HOME");
	if (source) {
		bkup_type = BKUP_FILES;
   		bkup_source = copy_source = (char *)MALLOC(3+strlen(source));
   		strcat(strcat(strcpy(copy_source," "), source), " ");
	}
	else {
		bkup_source = user_home;
		/*
 		 * if a previous complete backup was done,
		 * default becomes incremental instead of complete
		 */
		sprintf(buf, "%s/%s", user_home, BKUP_HIST);
		if (access(buf,R_OK) == 0)
			bkup_type = BKUP_INCR;
	}
	dir_fcrec.glyph = DmGetPixmap(theScreen, "dir.icon");
	doc_fcrec.glyph = DmGetPixmap(theScreen, "datafile.icon");
	MakeUserList();
/*
 *	create base window
 */
	bbase.icon_name = GetGizmoText(bbase.icon_name);
	w_ctl = CreateMediaWindow(parent, &bbase, NULL, 0);
	w_bkmenu = GetMenu(GetSubMenuGizmo(bbase.menu, 1));
	XtSetArg(arg[0], XtNsensitive, bkup_type==BKUP_FILES);
	OlFlatSetValues(w_bkmenu, 2, arg, 1);

	XtSetArg(arg[0], XtNhPad,	x3mm);
	XtSetArg(arg[1], XtNhSpace,	x3mm);
	XtSetArg(arg[2], XtNvPad,	y3mm);
	XtSetArg(arg[3], XtNvSpace,	y3mm);

	XtSetValues(w_ctl, arg, 4);
/*
 *	create doc/device abbreviated button menu
 */
	BkupDevice[0].label = bdoc_alias = GetGizmoText(label_doc);
	w_bdesc = DevMenu(BkupDevice, 1, N_DEVS, w_ctl, 
			GetGizmoText(label_bkupToCaption),
			(XtPointer)BdescCB, "removable=\"true", &w_devmenu);
	XtSetArg(arg[0], XtNlabel, DtamDevDesc(curdev));
	XtSetValues(w_bdesc, arg, 1);
/*
 *	controls specific to Backup
 */
	XtSetArg(arg[0], XtNlabel,	 	GGT(label_targetCaption));
	XtSetArg(arg[1], XtNspace,	 	x3mm);
	XtSetArg(arg[2], XtNmappedWhenManaged,	FALSE);
	XtSetArg(arg[3], XtNposition,	 	OL_LEFT);

	w_cap = XtCreateManagedWidget("caption",
			captionWidgetClass, w_ctl, arg, 4);
	
	XtSetArg(arg[0], XtNcharsVisible, 37);
	w_target = XtCreateManagedWidget("textfield",
			textFieldWidgetClass, w_cap, arg, 1);
	XtAddCallback(w_target, XtNverification, SetBkupDoc, NULL);

        if (restricted_flag)
	{
	    /* only allow immediate backup of file specified  */
	    /*  in -C option to MediaMgr */
	    bkup_type  = BKUP_COMPL;
	    bkup_class = CLS_USERS;
	    OlVaFlatSetValues(bbkup_menu.child, 1, XtNsensitive, FALSE, NULL);
	    OlVaFlatSetValues(bbkup_menu.child, 2, XtNsensitive, FALSE, NULL);
	    OlVaFlatSetValues(bfile_menu.child, 0, XtNsensitive, FALSE, NULL);
	    OlVaFlatSetValues(bfile_menu.child, 1, XtNsensitive, FALSE, NULL);
	    OlVaFlatSetValues(bfile_menu.child, 2, XtNsensitive, FALSE, NULL);
	}
        else
	{
	    XtSetArg(arg[0], XtNlabel,	GGT(label_bkupTypeCaption));
	    XtSetArg(arg[1], XtNposition,	OL_LEFT);
	    XtSetArg(arg[2], XtNspace,	x3mm);

	    w_cap = XtCreateManagedWidget("bkuptypecaption",
					  captionWidgetClass, w_ctl, arg, 3);
	    SET_EXCL(BkupType, 0, complType,   bkup_type==0);
	    SET_EXCL(BkupType, 1, incrType,    bkup_type==1);
	    SET_EXCL(BkupType, 2, selectFiles, bkup_type==2);

	    XtSetArg(arg[0], XtNtraversalOn,	TRUE);
	    XtSetArg(arg[1], XtNbuttonType,	OL_RECT_BTN);
	    XtSetArg(arg[2], XtNexclusives,	TRUE);
	    XtSetArg(arg[3], XtNitemFields,	ExclFields);
	    XtSetArg(arg[4], XtNnumItemFields,	NUM_ExclFields);
	    XtSetArg(arg[5], XtNitems,		BkupType);
	    XtSetArg(arg[6], XtNnumItems,	3);
	    XtSetArg(arg[7], XtNselectProc,	SetBkupType);

	    w_type = XtCreateManagedWidget("typeexcl",
					   flatButtonsWidgetClass, w_cap,
					   arg, 8);

	    if (owner = _DtamIsOwner(OWN_BACKUP)) {
		XtSetArg(arg[0], XtNlabel,	GGT(label_bkupClassCaption));
		XtSetArg(arg[1], XtNposition,	OL_LEFT);
		XtSetArg(arg[2], XtNspace,	x3mm);
		w_cap = XtCreateManagedWidget("classcaption",
					      captionWidgetClass, w_ctl,
					      arg, 3);

		SET_EXCL(BkupClass, 0, systemClass, FALSE);
		SET_EXCL(BkupClass, 1, selfClass, TRUE);
		SET_EXCL(BkupClass, 2, userClass, FALSE);

		XtSetArg(arg[0], XtNtraversalOn,	TRUE);
		XtSetArg(arg[1], XtNbuttonType,		OL_RECT_BTN);
		XtSetArg(arg[2], XtNexclusives,		TRUE);
		XtSetArg(arg[3], XtNitemFields,		ExclFields);
		XtSetArg(arg[4], XtNnumItemFields,	NUM_ExclFields);
		XtSetArg(arg[5], XtNitems,		BkupClass);
		XtSetArg(arg[6], XtNnumItems,		3);
		XtSetArg(arg[7], XtNselectProc,		SetBkupClass);
		XtSetArg(arg[8], XtNsameWidth,		OL_ALL);

		w_class = XtCreateManagedWidget("classexcl",
						flatButtonsWidgetClass, w_cap,
						arg, 9);
	    }
	}

	SET_BTN(BCheckBox, 0, log, NULL);

	XtSetArg(arg[0], XtNtraversalOn,	TRUE);
	XtSetArg(arg[1], XtNitemFields,		MBtnFields);
	XtSetArg(arg[2], XtNnumItemFields,	NUM_MBtnFields);
	XtSetArg(arg[3], XtNitems,		BCheckBox);
	XtSetArg(arg[4], XtNnumItems,		1);
	XtSetArg(arg[5], XtNexclusives,		FALSE);
	XtSetArg(arg[6], XtNbuttonType,		OL_CHECKBOX);
	XtSetArg(arg[7], XtNsameWidth,		OL_NONE);

	w_log = XtCreateManagedWidget("checkbox",
			flatButtonsWidgetClass,w_ctl, arg, 8);
/*
 *	Backup window: icon box for "Select Files" mode
 */
	BkupTarget = MakeObjectList(bkup_source, &bkup_count);
	XtSetArg(arg[0], XtNwidth,	WIDTH);
	XtSetArg(arg[1], XtNheight,	(Dimension)HEIGHT/(Dimension)2);
	XtSetValues(bbase.scroller, arg, 2);

	w_icons = GetIconBox(bbase.scroller, bkup_count);
	type_label[0] = BkupType[0].label;
	type_label[1] = BkupType[1].label;
	type_label[2] = BkupType[2].label;
	FooterMsg(bbase, SummaryMsg());
	CreateGizmo(bbase.shell, ModalGizmoClass, &bnote, NULL, 0);
	CreateGizmo(bbase.shell, FileGizmoClass, &open_prompt, NULL, 0);
	MapGizmo(BaseWindowGizmoClass, &bbase);
	if (cur_file)
		OpenFile();
}

char *
nbfgets (char *buf, int cnt, FILE *pFile)
{
    int		n, max;
    char	*cp;

    /* Get a line from a file that was opened nodelay.  If the string
     * doesn't end with a newline, we had a timing problem.  Pause, and
     * then try again.  This assumes the last line ends with a newline.
     */

    if (!fgets (buf, cnt, pFile))
	return (0);

    n = strlen (buf);
    if (buf [n-1] == '\n')
	return (buf);

    cp = buf + n;
    cnt -= n;
    max = 5;
    while (cnt > 1 && --max >= 0)
    {
	sleep (1);

	if (fgets (cp, cnt, pFile))
	{
	    n = strlen (cp);
	    if (cp [n-1] == '\n')
		return (buf);

	    cp += n;
	    cnt -= n;
	}
    }
    return (buf);
}

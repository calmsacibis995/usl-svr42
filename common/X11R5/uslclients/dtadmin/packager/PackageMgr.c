/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:packager/PackageMgr.c	1.4.1.51"
#endif
/*
 *	PackageMgr - browse, install and delete software packages
 */
#include "packager.h"

#define	POPUP_INFO	0
#define	POPUP_ICON	1

#define	INFO_FIELDS	8
#define	ICON_FIELDS	4

Widget	w_toplevel, w_pkgop, w_acts, w_edit, w_view, w_tile;
Widget	w_iconbox = NULL, w_pkgbox = NULL;
Boolean	select_popup = FALSE;

Widget	w_popup[2],
	info_field[INFO_FIELDS], 
	icon_field[ICON_FIELDS];

#define	W_NAME	info_field[0]
#define	W_DESC	info_field[1]
#define	W_CAT	info_field[2]
#define	W_VERS	info_field[3]
#define	W_ARCH	info_field[4]
#define	W_VEND	info_field[5]
#define	W_DATE	info_field[6]
#define	W_SIZE	info_field[7]

#define	I_HDR	icon_field[0]
#define	I_TYPE	icon_field[1]
#define	I_SCW	icon_field[2]
#define	I_BOX	icon_field[3]

#define	APPL_ICON	0
#define	EXEC_ICON	1

#define	APP_VIEW	0
#define	SYS_VIEW	1
#define	ALL_VIEW	2
#define	DEV_VIEW	3

int	view_type = ALL_VIEW;
int	SPOOL_VIEW;
Boolean	media_context = NEXT;

Screen		*theScreen;
Display		*theDisplay;
XFontStruct	*def_font;

void	exitCB();
void	helpCB();
void	addCB();
void	deleteCB();
void	deletePKG();
void	iconCB();
void	infoCB();
void	installICON();
void	cancelICON();
void	cancelINFO();
void	cancelNOTE();
void	cancelMSG();
void	cancelPKG();
void	SysViewCB();
void	DevViewCB();
void	PopupHelpDeskCB();
void	GetMedia();
void	GetPkgBox();
void	PkgSize();
void	CreatePkgIcons();
void	CreateSetIcons();
void	promptCB();
void	SelectFileCB();
void	SelectPkg();
void	Click2Pkg();
void	FindInstalled();
void	Wait3_2Pkg();
void	WaitPkgOp();
void	SetSensitivity();
void	DTMInstall();

MenuItems	action_menu_item[] = {
	{ TRUE, label_icons, mnemonic_icons, 0, iconCB },
	{ TRUE, label_exit,  mnemonic_exit, 0, exitCB },
	{ NULL }
};

MenuItems	edit_menu_item[] = {
	{ TRUE, label_add,   mnemonic_add, 0, addCB},
	{ TRUE, label_delete,mnemonic_delete, 0, deleteCB},
	{ TRUE, label_info,  mnemonic_info, 0, infoCB},
	{ NULL }
};

MenuItems	sys_menu_item[] = {
	{ TRUE, label_apps,   mnemonic_apps, 0},
	{ TRUE, label_system, mnemonic_system, 0},
	{ TRUE, label_all,    mnemonic_all, 0},
	{ NULL }
};
MenuGizmo	sys_menu   = {0, "sys_menu", NULL, sys_menu_item, SysViewCB};

#define	MAX_DEVS	10

/* allocate MAX_DEVS +1 (Other...) +1 (NULL, required by MenuGizmo) entries */
MenuItems	dev_menu_item[MAX_DEVS+2] = {
	{ TRUE, label_spooled,mnemonic_spooled, 0},
	{ NULL }
};
MenuGizmo	dev_menu   = {0, "dev_menu", NULL, dev_menu_item, DevViewCB};

MenuItems	view_menu_item[] = {
	{ TRUE, label_inst,   mnemonic_inst, (MenuGizmo *)&sys_menu, 0},
	{ TRUE, label_uninst, mnemonic_uninst, (MenuGizmo *)&dev_menu, 0},
	{ NULL }
};

static	HelpInfo HelpIntro	= { 0, "", HELP_PATH, help_intro };
static	HelpInfo HelpTOC	= { 0, "", HELP_PATH, "TOC" };
static	HelpInfo HelpProps	= { 0, "", HELP_PATH, help_props };
static	HelpInfo HelpIcons	= { 0, "", HELP_PATH, help_icons };
static	HelpInfo HelpUninstalled= { 0, "", HELP_PATH, help_uninstalled };
static	HelpInfo HelpFolder	= { 0, "", HELP_PATH, help_folder };
static	HelpInfo HelpDesk	= { 0, "", HELP_PATH, "HelpDesk" };
static	HelpInfo HelpPkgwin	= { 0, "", HELP_PATH, help_pkgwin };

MenuItems	help_menu_item[] = {  
	{ TRUE, label_intro, mnemonic_intro, 0, helpCB, (char *)&HelpIntro },
	{ TRUE, label_toc,   mnemonic_toc, 0, helpCB, (char *)&HelpTOC },
	{ TRUE, label_hlpdsk,mnemonic_hlpdsk, 0, helpCB, (char *)&HelpDesk },
	{ NULL }
};

MenuGizmo	action_menu = {0, "action_menu", NULL, action_menu_item};
MenuGizmo	edit_menu   = {0, "edit_menu",   NULL, edit_menu_item};
MenuGizmo	view_menu   = {0, "view_menu", NULL, view_menu_item};
MenuGizmo	help_menu   = {0, "help_menu",   NULL, help_menu_item};

MenuItems	main_menu_item[] = {
	{ TRUE, label_action, mnemonic_action, (Gizmo) &action_menu},
	{ TRUE, label_edit,   mnemonic_edit, (Gizmo) &edit_menu},
	{ TRUE, label_view,   mnemonic_view, (Gizmo) &view_menu},
	{ TRUE, label_help,   mnemonic_help, (Gizmo) &help_menu},
	{ NULL }
};
MenuGizmo	menu_bar = { 0, "menu_bar", NULL, main_menu_item};

BaseWindowGizmo	base = {0, "base", string_appName, (Gizmo)&menu_bar,
	NULL, 0, string_iconName, ICON_NAME, " ", " ", 90 };

MenuItems	pkg_menu_item[] = {  
	{ TRUE, label_icons,  mnemonic_icons,  0, iconCB },
	{ TRUE, label_info,   mnemonic_info,   0, infoCB },
	{ TRUE, label_delete, mnemonic_delete, 0, deletePKG,  NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, cancelPKG,  NULL },
	{ TRUE, label_help,   mnemonic_help, 0, helpCB, (char *)&HelpPkgwin },
	{ NULL }
};
MenuGizmo	pkg_menu = {0, "pkgs", NULL, pkg_menu_item };
PopupGizmo	pkg_popup = {0,"popup",string_pkgTitle,(Gizmo)&pkg_menu };

MenuItems	icon_menu_item[] = {  
	{ TRUE, label_install, mnemonic_install, 0, installICON,  NULL },
	{ TRUE, label_cancel,  mnemonic_cancel, 0, cancelICON,   NULL },
	{ TRUE, label_help,    mnemonic_help, 0, helpCB, (char *)&HelpIcons },
	{ NULL }
};
MenuGizmo	icon_menu = {0, "icons", NULL, icon_menu_item };
PopupGizmo	icon_popup = {0,"popup",string_iconTitle,(Gizmo)&icon_menu };

MenuItems	info_menu_item[] = {  
	{ TRUE, label_cancel, mnemonic_cancel, 0, cancelINFO,   NULL },
	{ TRUE, label_help,   mnemonic_help, 0, helpCB, (char *)&HelpProps },
	{ NULL }
};
MenuGizmo	info_menu = {0, "properties", NULL, info_menu_item };
PopupGizmo	info_popup = {0,"popup",string_infoTitle,(Gizmo)&info_menu };

MenuItems	msg_item[] = {
	{ TRUE, label_cancel,mnemonic_cancel, 0, cancelMSG },
	{ TRUE, label_help,  mnemonic_help, 0, helpCB, (char *)&HelpIntro },
	{ NULL }
};
MenuGizmo  msg_menu = {0, "msg", "msg", msg_item };
ModalGizmo info_msg = {0, "msg", string_msgTitle, (Gizmo)&msg_menu};

MenuItems	note_item[] = {
	{ TRUE, label_go,    mnemonic_go, 0, GetMedia },
	{ TRUE, label_cancel,mnemonic_cancel, 0, cancelNOTE },
	{ TRUE, label_help,  mnemonic_help, 0, helpCB,(char *)&HelpUninstalled},
	{ NULL }
};
MenuGizmo  note_menu = {0, "note", "note", note_item };
ModalGizmo insert_note = {0, "insert", string_mediaTitle, (Gizmo)&note_menu};

MenuItems	prompt_menu_item[] = {
	{ TRUE, label_select, mnemonic_select, 0, promptCB, NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, promptCB, NULL },
	{ TRUE, label_help,   mnemonic_help, 0, helpCB, (char *)&HelpFolder },
	{ NULL}
};
MenuGizmo	prompt_menu = {0, "prompt", NULL, prompt_menu_item };
FileGizmo	*prompt;

FileGizmo	spool_prompt = {
	&HelpFolder, "spooldir", NULL, &prompt_menu, NULL, NULL, NULL, FOLDERS_ONLY
};

char    *Fields[]   = { XtNlabel, XtNmnemonic, XtNsensitive, XtNselectProc,
			XtNpopupMenu };

char    *ExclFields[]   = { XtNlabel, XtNmnemonic, XtNset };

FILE	*cmdfp[2];

#define	QUANTUM	200

PkgPtr	sys_list     = (PkgPtr)0;
PkgPtr	add_list     = (PkgPtr)0;
int	sys_max	     = QUANTUM;
int	add_max	     = 0;
int	sys_count    = 0;
int	add_count    = 0;
int	icon_count   = 0;
int	p_item_count = 0;

DmItemPtr	pkg_itp,	set_itp,	icon_itp;
DmFclassRec	pkg_fcrec,	set_fcrec,	exec_fcrec;
DmContainerRec	pkg_cntrec,	set_cntrec,	icon_cntrec;

Dimension	x3mm, y3mm;

Boolean		owner;
Boolean		have_dir = FALSE;
int		have_medium = 0;	/* or DTAM_PACKAGE or DTAM_CUSTOM */

#define WIDTH	(40*x3mm)
#define	HEIGHT	(20*y3mm)

#define INIT_X  16
#define INIT_Y  16
#define INC_X   96
#define INC_Y	64
#define	MARGIN	16

Dimension	p_ibx = INIT_X, p_iby = INIT_Y;

XtIntervalId	GIBtimeout = 0;

Arg	arg[12];

#define	PROGRAM_NAME	"PackageMgr"
#define	ICONDIR		"desktop/PackageMgr"
#define	PKGDIR		"/var/sadm/pkg"
#define	LOGFILE		"/var/sadm/install/contents"

char	buf[BUFSIZ+1] = "";
char	LANG_C[]  = "LANG=C ";
char	PKGINFO[] = "/usr/bin/pkginfo";
char	ADDPKG[]  = "/usr/sbin/pkgadd";
char	REMPKG[]  = "/usr/sbin/pkgrm";
char	CUSTOM[]  = "/sbin/custom";
char	PERMS[]   = "/etc/perms";	/* records custom packages */
char	APPL[]	  = "application";
char	XTERM[PATH_MAX+16];
char	*our_node = NULL;
uid_t	our_uid;
char	*curalias = NULL;
char	*curlabel = NULL;
char	*desktopdir;
char	*sysalias;
char	*spooled;
char	*lastalias;
char	*spooldir;
char	*spooldefault = "/usr/spool/pkg";
char	cmdtail[32];

PkgPtr
InitPkg(PkgPtr *listp, int *max, int n)
{
	PkgPtr	p;

	if (n == *max) {
		*max += QUANTUM;
		*listp = (PkgPtr)REALLOC(*listp, (1 + *max)*sizeof(PkgRec));
	}
	p = *listp + n;
	memset(p, 0, sizeof(PkgRec));
	return p;
}

static	void
promptCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	OlFlatCallData  *olcd = (OlFlatCallData *) call_data;
	int             n = olcd->item_index;

	BringDownPopup(GetFileGizmoShell(prompt));
	if (n == 1) {
		if (GIBtimeout) {
			XtRemoveTimeOut(GIBtimeout);
			GIBtimeout = (XtIntervalId)NULL;
			curalias = lastalias;
		}
	}
	else {
		have_dir = TRUE;
		spooldir = GetFilePath(prompt);
		sprintf(buf, GetGizmoText(string_uninstTitle), spooldir);
		SetBaseWindowTitle(&base, buf);
		GetPkgBox(NULL, NULL);
	}
}

static	void
SysViewCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData	*d = (OlFlatCallData *)call_data;

	FooterMsg(NULL);
	view_type = d->item_index;
	XtSetArg(arg[0], XtNlabel, &curlabel);
	OlFlatGetValues(w, d->item_index, arg, 1);
	sprintf(buf, GetGizmoText(string_instTitle), curlabel);
	SetBaseWindowTitle(&base, buf);
	curalias = sysalias;
	have_dir = FALSE;
	have_medium = 0;
	if (curalias == lastalias)
		CreateSetIcons(base.scroller);
	else
		FindInstalled();
	SetSensitivity();
	lastalias = curalias;
}

static	void
DevViewCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData	*d = (OlFlatCallData *)call_data;

	FooterMsg(NULL);
	XtSetArg(arg[0], XtNlabel, &curlabel);
	OlFlatGetValues(w, d->item_index, arg, 1);
	sprintf(buf, GetGizmoText(string_uninstTitle), curlabel);
	SetBaseWindowTitle(&base, buf);
	if (d->item_index == SPOOL_VIEW) {
		view_type = SPOOL_VIEW;
		have_dir = FALSE;
		curalias = spooled;
		MapGizmo(FileGizmoClass, prompt);
	}
	else {
		view_type = DEV_VIEW;
		XtSetArg(arg[0], XtNlabel, &curlabel);
		OlFlatGetValues(w, d->item_index, arg, 1);
		have_medium = 0;
		curalias = MapAlias(curlabel);
		GetMedia(NULL, NULL, NULL);
	}
	SetSensitivity();
	lastalias = curalias;
}

static	void
TellUsers(void)
{
struct	dirent	*dent;
	DIR	*dirp;
	FILE	*cmdpipe;
	char	*cmdbuf;
	Boolean	have_users = FALSE;
	PkgPtr	p;

	if ((cmdbuf = MALLOC(BUFSIZ)) == NULL)
		return;
	strcpy(cmdbuf, "/usr/bin/mailx");
	if (dirp = opendir(GetXWINHome("desktop/LoginMgr/Users"))) {
		while (dent = readdir(dirp)) {
			if (strcmp(dent->d_name,".") != 0
			&&  strcmp(dent->d_name,"..") != 0
			&&  strcmp(dent->d_name,"root") != 0
			&&  strcmp(dent->d_name,".dtinfo") != 0) {
				/*
				 *	root gets mail from pkgadd
				 */
				if ((cmdbuf = REALLOC(cmdbuf, strlen(cmdbuf)+
						strlen(dent->d_name)+2))==NULL)
					break;
				strcat(strcat(cmdbuf," "),dent->d_name);
				have_users = TRUE;
			}
		}
		closedir(dirp);
	}
	endpwent();
	if (have_users && (cmdpipe=popen(cmdbuf,"w")) != NULL) {
		fprintf(cmdpipe, "%s", GetGizmoText(string_mailMsg));
		for (p = add_list; p < add_list+add_count; p++) {
			if (p->pkg_opflag == 'F')
				fprintf(cmdpipe, "\t%s\n", p->pkg_name);
		}
		pclose(cmdpipe);
	}
	FREE(cmdbuf);
}

static	char
*FilterExecutable(char *name, char *line, char *format)
{
	char	*ptr, *ptr2, *path, *modeptr;
	int	n = 0;
/*
 *	isolate filename and mode on input lines that match the pkg name
 *	custom packages have the format:
 *
 *		PKGNAME<space>C+MODE<space>OWN/GRP<space>LINKS<space>PATH  ...
 *
 *	(C+MODE being char (d/f/x/...) followed by mode; I only look for 'x')
 *	with arbitrary whitespace between the fields (in general, tabs or
 *	spaces up to the next standard tab column)
 *
 *	and SVR4 packages are:
 *
 *		PATH C CLASS MODE ... PKGNAME[:CLASS] ...
 *
 *	(with some variants ignored here, including links symbolic and hard;
 *	since these paths are presented only as a fall back when there are no
 *	icons defined, it is not critical that we find all linked variants of
 *	executables, as long as each gets represented.)
 */
	if (*line=='#' || (ptr=strpbrk(line, " \t")) == NULL)
		return NULL;
	*ptr++ = '\0';
	/*
	 *	parse the line according to format (custom, or SVR4)
	 *	eliminate first those that are not even possibly executables
	 *	then, eliminate those that are not in the right package(s)
	 *	and finally, break out the mode and pathnames, and validate
	 *	executability by "other" -- return NULL if any test fails.
	 */
	if (format[0] == 'C') {
		if (strcmp(name, line) != 0)
			return NULL;
		while (isspace(*ptr))
			ptr++;
		if (*ptr != 'x')
			return NULL;
		modeptr = ++ptr;
		/*
		 *	find pathname field
		 */
		strtok(ptr, " \t\n");
		strtok(NULL," \t\n");
		strtok(NULL," \t\n");
		path = strtok(NULL," \t\n");
		if (*path == '.')
			++path;
	}
	else {
		if (*ptr != 'f' && *ptr != 'v')
			return NULL;
		if ((ptr2 = strstr(ptr, name)) == NULL)
			return NULL;
		else {
			--ptr2;
			if (!isspace(*ptr2) && *ptr2 != ':')
				return NULL;
			else {
				ptr2 += strlen(name)+1;
				if (*ptr2 != '\0' && *ptr2 != ':')
					return NULL;
			}
		}
		while (isspace(*++ptr))		/* step to class field */
			;
		while (!isspace(*++ptr))	/* and step through that */
			;
		modeptr = ++ptr;
		path = line;
	}
	while(isdigit(*modeptr)) {
		n = *modeptr-'0'+ n*8;
		modeptr++;
	}
	return (n & 1? path: NULL);
}

static	char
*FilterIcon(line)
	char	*line;
{
	if (strncmp(line,"ICON=",5) != 0)
		return NULL;
	return (strchr(line,'\t'));	/* step past icon filepath */
}

static	void
FreePkgList(PkgPtr list, int *count)	/* free strings allocated to packages */
{
register  int	n = *count;

	while (n--) {
		if (list[n].pkg_name)	FREE(list[n].pkg_name);
		if (list[n].pkg_desc)	FREE(list[n].pkg_desc);
		if (list[n].pkg_cat)	FREE(list[n].pkg_cat);
		if (list[n].pkg_fmt)	FREE(list[n].pkg_fmt);
		if (list[n].pkg_vers)	FREE(list[n].pkg_vers);
		if (list[n].pkg_vers)	FREE(list[n].pkg_vers);
		if (list[n].pkg_arch)	FREE(list[n].pkg_arch);
		if (list[n].pkg_vend)	FREE(list[n].pkg_vend);
		if (list[n].pkg_date)	FREE(list[n].pkg_date);
		if (list[n].pkg_size)	FREE(list[n].pkg_size);
	}
	*count = 0;
}

static	void
AddPackage(int n, char *str)
{
	char	*ptr;
	PkgPtr	p;

	p = InitPkg(&add_list, &add_max, n);
	p->pkg_cat = STRDUP("set");
	ptr = strchr(str,' ');
	*ptr = '\0';
	p->pkg_name = STRDUP(str);
	while (isspace(*++ptr))
		;
	p->pkg_desc = STRDUP(ptr);
	p->pkg_fmt = STRDUP("4.0");
}

/*
 *	ReadCustom scans through the initial segment of the TAR file for
 *	a custom medium, that is, through the perms file for the product,
 *	and sets the (single) add_list package structure.  The relevant
 *	fields of the perms file are prd=<name> for the name of the product
 *	-- which becomes the icon name, equivalent to the "short" name of a
 *	pkgadd package -- and set=<desc> for the longer name or description.
 *	In the event of multiple custom packages on a server, ReadCustom
 *	or something like it should be invoked in a loop stepping add_count.
 */
static	void
ReadCustom(XtPointer dummy, XtIntervalId tid)
{
	int	n;
	char	*ptr, *ptr2;
static	Boolean	init = TRUE;
static	char	*desc = NULL;

	if (cmdfp[1] == NULL) {		/* canceled -- restore sanity */
		*buf = '\0';
		goto finished;
	}
	n = read(fileno(cmdfp[1]), buf, BUFSIZ);
	switch (n) {
	case -1:/*
		 *	no current input; keep going
		 */
		break;
	case 0:	/*
		 *	end of file; we should NOT get this far!
		 */
		goto finished;
	default:/*
		 *	examine each (whole) line
		 */
		if (n < BUFSIZ)
			buf[n] = '\0';
		if (init) {
			ptr = strstr(buf,"/prd=");
			/*
			 *	note: this must exist or the disk would not
			 *	be recognized as Custom format.
			 */
			ptr2 = ptr+5;	/* start of product name */
			if (ptr = strchr(ptr2,'/'))
				*ptr = '\0';
			InitPkg(&add_list, &add_max, add_count);
			add_list[add_count].pkg_cat = STRDUP("set");
			add_list[add_count].pkg_fmt = STRDUP("Custom");
			add_list[add_count++].pkg_name = STRDUP(ptr2);
			init = FALSE;
		}
		else if (desc) {
		/*
		 *	rest of the description was truncated; finish it off
		 */
			ptr = strchr(buf,'\n');
			*ptr = '\0';
			desc = (char *)REALLOC(desc,strlen(desc)+strlen(buf)+1);
			strcat(desc, buf);
			add_list[add_count-1].pkg_desc = desc;
			goto finished;
		}
		else {
		/*
		 *	description follows "#set="
		 */
			if (ptr=strstr(buf,"#set=")) {
				desc = ptr+5;
				if (*desc == '"')
					desc++;
				if (ptr2=strpbrk(desc,"\"\n")) {
					*ptr2 = '\0';
					add_list[add_count-1].pkg_desc =
								STRDUP(desc);
					goto finished;
				}
				else
					desc = STRDUP(desc);
					/*
					 *	and continue for one more read
					 */
			}
			/* keep on reading */
		}
	}
	GIBtimeout = XtAddTimeOut(150, (XtTimerCallbackProc)ReadCustom, NULL);
	return;
finished:
	desc = NULL;
	init = TRUE;
	fclose(cmdfp[1]);
	cmdfp[1] = NULL;
	GIBtimeout = (XtIntervalId)0;
	BringDownPopup(info_msg.shell);
	CreateSetIcons(base.scroller);
}

static	void
ReadPkgList(XtPointer sys_pkg, XtIntervalId tid)
{
static	Boolean	pkgadd_flag = FALSE;
static	Boolean	set_flag = FALSE;
	PkgPtr	p = (PkgPtr)sys_pkg;
	int	n = 0;
	char	*ptr, *line;

	if (cmdfp[1] == NULL) {		/* canceled -- restore sanity */
		set_flag = pkgadd_flag = FALSE;
		*buf = '\0';
		GIBtimeout = (XtIntervalId)0;
		if (curalias != sysalias)
			CreateSetIcons(base.scroller);
		return;
	}
	ptr = buf+strlen(buf);
	n = read(fileno(cmdfp[1]), ptr, BUFSIZ-(ptr-buf));
	switch (n) {
	case -1:/*
		 *	no current input; keep going
		 */
		break;
	case 0:	/*
		 *	end of file; for sysalias, continue to find sets.
		 */
		_Dtam_p3close(cmdfp, SIGTERM);
		cmdfp[0] = cmdfp[1] = (FILE *)NULL;
		if (curalias != sysalias) {
			FooterMsg(buf);
			CreateSetIcons(base.scroller);
		}
		else if (set_flag == FALSE) {
			set_flag = TRUE;
			sprintf(buf, "%s%s -l -c set", LANG_C, PKGINFO);
			if (_Dtam_p3open(buf, cmdfp, FALSE) == -1)
				cmdfp[0] = cmdfp[1] = (FILE *)NULL;
			else
				break;	/* get pkginfo about sets */
		}
		*buf = '\0';
		set_flag = pkgadd_flag = FALSE;
		BringDownPopup(info_msg.shell);
		GIBtimeout = (XtIntervalId)0;
		CreateSetIcons(base.scroller);
		return;
	default:/*
		 *	examine each (whole) line
		 */
		ptr[n] = '\0';
		if (curalias != sysalias) {
			if (strstr(buf, "Type [go]")) {
				fputs("go\n",cmdfp[0]);
				*buf = '\0';
				break;
			}
			else if (line = strstr(buf, "available:")) {
				pkgadd_flag = TRUE;
				strcpy(buf, line+11);
			}
		}
		for (line = buf; *line == '\n'; line++)
			;
		for (ptr=strchr(line,'\n'); ptr; line=ptr,ptr=strchr(ptr,'\n')){
			char	*ptr2;
			while (*ptr == '\n')
				*ptr++ = '\0';
			if (*line == '\0')
				continue;
			if (curalias != sysalias) {
				if (pkgadd_flag && atoi(line) != 0) {
					while (isspace(*line) || isdigit(*line))
						line++;
					AddPackage(add_count++, line);
				}
				else if (strncmp(line,"Select package",14)==0) {
					fputs("q\n", cmdfp[0]);
				}
			}
			/*
			 *	in sysalias case, fill in pkginfo fields
			 */
			else  if (ptr2 = strstr(line, "PKGINST:")) {
				/* 
				 *	change package; select or add p
				 */
				ptr2 += 10;
				for (p = sys_list; p < sys_list+sys_count; p++)
					if (strcmp(ptr2, p->pkg_name) == 0)
						break;
				if (p == sys_list + sys_count) {
					p = InitPkg(&sys_list, &sys_max,
								sys_count++);
					p->pkg_name = STRDUP(ptr2);
				}
			}
			else  if (ptr2 = strstr(line, "STATUS:")) {
				if (strstr(ptr2,"preSVR4")) {
					p->pkg_cat = STRDUP("preSVR4");
					p->pkg_fmt = STRDUP("3.2");
				}
			}
			else if (ptr2 = strstr(line,"STATUS:")) {
				if (!p->pkg_cat)
					p->pkg_cat = STRDUP(ptr2+11);
				if (strncmp(ptr2,"preSVR4",7)==0)
					p->pkg_fmt = STRDUP("3.2");
			}
			else if (ptr2 = strstr(line,"NAME:")) {
				if (!p->pkg_desc)
					p->pkg_desc = STRDUP(ptr2+7);
				if (strstr(ptr2,"BUILT IN"))
					p->pkg_set = STRDUP("builtin");
			}
			else if (ptr2 = strstr(line,"ARCH:")) {
				if (!p->pkg_arch)
					p->pkg_arch = STRDUP(ptr2+7);
			}
			else if (ptr2 = strstr(line,"VERSION:")) {
				if (!p->pkg_vers)
					p->pkg_vers = STRDUP(ptr2+10);
			}
			else if (ptr2 = strstr(line,"VENDOR:")) {
				if (!p->pkg_vend)
					p->pkg_vend = STRDUP(ptr2+9);
			}
			else if (ptr2 = strstr(line,"DATE:")) {
				if (!p->pkg_date)
					p->pkg_date = STRDUP(ptr2+7);
			}
			else if (ptr2 = strstr(line,"blocks used")) {
				if (!p->pkg_size) {
					*--ptr2 = '\0';
					for (ptr2=line; isspace(*ptr2); ++ptr2)
						;
					PkgSize(ptr2, NULL, p);
				}
			}
		}
		if (*line && line != buf)	/* keep partial line */
			strcpy(buf, line);
		else
			*buf = '\0';
		break;
	}
	GIBtimeout = XtAddTimeOut(150, (XtTimerCallbackProc)ReadPkgList, p);
}

Boolean
PkgInstalled(PkgPtr p)
{
	char	str[BUFSIZ];
	int	n;

	if (p->pkg_fmt[0] == 'C') {
		if (strcmp(p->pkg_cat,"set")==0)
			sprintf(str, "/etc/perms/%s", p->pkg_name);
		else
			sprintf(str, "/etc/perms/%s", p->pkg_set);
		if (access(str,F_OK) != 0)
			return FALSE;
		else if (strcmp(p->pkg_cat,"set")==0)
			return TRUE;
		else {
			FILE	*pfp;
			char	*ptr;
			Boolean	deleted = FALSE;

			sprintf(str,"grep \"^%s\" /etc/perms/%s",
					p->pkg_name, p->pkg_set);
			if ((pfp=popen(str,"r")) == NULL)
				return FALSE;
			while (fgets(str, BUFSIZ, pfp)) {
				if (!isspace(str[strlen(p->pkg_name)]))
					continue;
				ptr = strtok(str, " \t");	/* "pkg" */
				ptr = strtok(NULL," \t");	/* mode  */
				ptr = strtok(NULL," \t");	/* owner */
				ptr = strtok(NULL," \t");	/* ????  */
				ptr = strtok(NULL," \t");	/* path  */
				if (access(ptr,F_OK) != 0) {
					deleted = TRUE;
					break;
				}
			}
			pclose(pfp);
			return !deleted;
		}
	}
	else {
		sprintf(str, "%s -q %s 2>/dev/null", PKGINFO, p->pkg_name);
		n = system(str);
		return n==0;
	}
}

static	char
*PkgLabel(p)
	PkgPtr	p;
{
static	char	plabel[80];

	sprintf(plabel,"%s:  %s", p->pkg_name, p->pkg_desc);
	return plabel;
}

/*
 *	this needs to be replaced by a multi-select
 */
static	PkgPtr
GetSelectedPkg()
{
	int		last, count;
	Widget		widget_selected;
	DmObjectPtr	optr;
	DmItemPtr	itp;

	if (select_popup) {
		widget_selected = w_pkgbox;
		itp = pkg_itp;
	}
	else {
		widget_selected = w_iconbox;
		itp = set_itp;
	}
	XtSetArg(arg[0], XtNnumItems, &last);
	XtGetValues(widget_selected, arg, 1);
	optr = (DmObjectPtr)NULL;
	for (count = 0; count < last; count++) {
		if (itp[count].managed && itp[count].select) {
			optr = (DmObjectPtr)itp[count].object_ptr;
			break;
		}
	}
	return (optr == NULL? (PkgPtr)NULL: (PkgPtr)(optr->objectdata));
}

static	void
PropertyEventHandler(w, client_data, xevent, cont_to_dispatch)
	Widget		w;
	XtPointer	client_data;
	XEvent		*xevent;
	Boolean		*cont_to_dispatch;
{
	DtReply		reply;
	int		ret;

	if (xevent->type != SelectionNotify)
		return;
	memset(&reply, 0, sizeof(reply));
	if (xevent->xselection.selection == _HELP_QUEUE(theDisplay)) {
		ret = DtAcceptReply(XtScreen(w),
			_HELP_QUEUE(theDisplay), XtWindow(w), &reply);
	}
	if (xevent->xselection.selection == _DT_QUEUE(theDisplay)) {
		ret = DtAcceptReply(XtScreen(w),
			_DT_QUEUE(theDisplay), XtWindow(w), &reply);
		if (ret == 0) {
			if (reply.create_fclass.status == DT_OK)
				FooterMsg(GetGizmoText(string_regClass));
			else if (reply.create_fclass.status != DT_DUP)
				FooterMsg(GetGizmoText(string_regFailed));
				/*
				 *	pass silently over duplicate registry
				 */
		}
		else
			FooterMsg(GetGizmoText(string_regFailed));
	}
}

static	char	*
ValidLocale(char *name, char *type)
{
static	char	pathnm[PATH_MAX];
	char	*ptr;

	if (*name == '/') {
		strcpy(pathnm, name);
		if (access(name, R_OK) != 0)
			return NULL;
		else
			return pathnm;
	}
	if (ptr = strchr(name,':')) {	/* locale is specified */
		strncpy(pathnm, name, ptr-name);
		pathnm[ptr-name] = '\0';
	}
	else
		strcpy(pathnm, name);
	ptr = XtResolvePathname(theDisplay,type,pathnm,NULL,NULL,NULL,0 ,NULL);
	return ptr? pathnm: NULL;
}

static	void
RegisterClass(char *class_lines, Boolean flag)
{
	char		*ptr, *str, *fname;
	DtRequest	*request;

	str = STRDUP(class_lines);
	for (ptr=strtok(str, " "); ptr; ptr=strtok(NULL," ")) {
		if (fname = ValidLocale(ptr, "classdb")) {
		/*
		 *	Kai points out that I should post multiple requests
		 *	on different queues or the replies will overwrite.
		 *	As we do not really expect multiple CLASS files in
		 *	most packages (only our desktop package has them)
		 *	this can be deferred for the time being.)
		 */
			request = (DtRequest *)CALLOC(1,sizeof(DtRequest));
			request->create_fclass.rqtype = flag?
						DT_CREATE_FILE_CLASS:
						DT_DELETE_FILE_CLASS;
			request->create_fclass.file_name = fname; 
			DtEnqueueRequest(theScreen,_DT_QUEUE(theDisplay),
					_DT_QUEUE(theDisplay),
					XtWindow(base.shell), request);
		}
		else {
			sprintf(buf, GetGizmoText(string_badClass), str);
			FooterMsg(buf);
			FREE(str);
			return;
		}
	}
	FREE(str);
}

static	void
RegisterHelp(char *help_lines, Boolean flag)
{
	char		*str, *ptr, *ptr2, *fname;
	DtRequest	*request;

	ptr = str = STRDUP(help_lines);
	while (ptr) {
		char	*next = strchr(ptr,'\n');
		ptr2 = strtok(ptr,"\t");
		ptr = (next && next[1] != '\0')? next+1: NULL;
		if (fname = ValidLocale(ptr2, "help")) {
			request = (DtRequest *)CALLOC(1,sizeof(DtRequest));
			if (flag) {
				request->add_to_helpdesk.rqtype =
							DT_ADD_TO_HELPDESK;
				request->add_to_helpdesk.help_file = fname; 
			}
			else {
				request->del_from_helpdesk.rqtype =
							DT_DEL_FROM_HELPDESK;
				request->del_from_helpdesk.help_file = fname; 
			}
		}
		else {
			sprintf(buf, GetGizmoText(string_badHelp), ptr2);
			FooterMsg(buf);
			FREE(str);
			return;
		}
		ptr2 = strtok(NULL,"\t");
		if (flag)
			request->add_to_helpdesk.icon_file = ptr2;
		ptr2 = strtok(NULL,"\t\n");
		if (flag)
			request->add_to_helpdesk.icon_label = ptr2;
		ptr2 = strtok(NULL,"\t\n");
		if (flag)
			request->add_to_helpdesk.app_name = ptr2;
		else
			request->del_from_helpdesk.app_name = ptr2;
		DtEnqueueRequest(theScreen, _HELP_QUEUE(theDisplay),
					    _HELP_QUEUE(theDisplay),
					    XtWindow(base.shell),
					    request);
	}
	FREE(str);
}

static	void
RegisterPkg(char *pkgname, Boolean flag)
{
	PkgPtr	p;

	for (p = sys_list; p < sys_list+sys_count; p++)
		if (strcmp(p->pkg_name, pkgname)==0) {
			if (p->pkg_help)
				RegisterHelp(p->pkg_help, flag);
			if (p->pkg_class)
				RegisterClass(p->pkg_class, flag);
			return;
		}
}

static	void
RegisterAll(PkgPtr p)
{
	FILE		*icondef;
	icon_obj	iobj;

	if (p->pkg_help)
		RegisterHelp(p->pkg_help, TRUE);
	if (p->pkg_class)
		RegisterClass(p->pkg_class, TRUE);
	iobj.pkg = NULL;
	sprintf(buf, "%s/%s", GetXWINHome(ICONDIR), p->pkg_name);
	if ((icondef=fopen(buf,"r")) == NULL)
		return;
	while(fgets(buf, BUFSIZ, icondef)) {	
		if (strncmp(buf,"ICON=",5)==0) {
			buf[strlen(buf)-1] = '\0';
			iobj.def = 1+strchr(buf+5,'\t');
			DTMInstall(&iobj);
		}
	}
	fclose(icondef);
}

static	char
*ToolboxName(char *icondef)
{
static	char	str[PATH_MAX];
static	char	*apptoolbox = NULL;
	char	*ptr , *ptr2;

	if (apptoolbox == NULL)
		apptoolbox = GetGizmoText(folder_apps);
	ptr = STRDUP(icondef);
	if (ptr2 = strchr(ptr,'\t')) {
		*ptr2 = '\0';
		if (strchr(ptr,'/'))
			strcpy(str, ptr);
		else if (ptr[0]=='.' && ptr[1]=='/')
			strcpy(str, ptr+2);
		else
			sprintf(str, "%s/%s", apptoolbox, ptr);
	}
	else
		sprintf(str, "%s/%s", apptoolbox, basename(ptr));
	FREE(ptr);
	return str;
}

static	char	*
ExpFileName(icon_obj *iobj)
{
static	char	*ptr;

	if (!iobj->pkg)
		ptr = iobj->def;
	else if (ptr=strstr(iobj->def,"$XWINHOME"))
		ptr = GetXWINHome(ptr+10);
	else
		ptr = 1+strrchr(iobj->def, '=');
	return STRDUP(ptr);
}

static	void
DTMInstall(icon_obj *iobj)
{
	char		*name, str[PATH_MAX];
	Boolean		is_dir = FALSE;
	int		result;
	DtRequest	request;

	memset(&request, 0, sizeof(request));
	request.sync_folder.rqtype = DT_SYNC_FOLDER;

	if (iobj->pkg) {
		RegisterPkg(iobj->pkg, TRUE);
		is_dir = (strcmp(iobj->def+strlen(iobj->def)-4, "\tDIR") == 0);
		if (!is_dir && strstr(iobj->def, "\tFILE=") == NULL)
		/*
	 	* syntax error -- just skip it, as we may want to expand here
	 	*/
			return;
	}
	name = ToolboxName(iobj->def);
	sprintf(str, "%s/%s", desktopdir, name);
	if (access(str,F_OK) != 0) {
		result = is_dir? mkdir(str, 0777):
				 symlink(ExpFileName(iobj), str);
		sprintf(buf, "%s %s %s", GetGizmoText(tag_instOp), name,
				result==0? GetGizmoText(tag_good):
				   	GetGizmoText(tag_bad));
		SetPopupMessage(&icon_popup, buf);
	}
/*
 *	the following is commented out as apparently unnecessary,
 *	and potentially disruptive of the reply on the _DT_QUEUE
 *	atom from a class registration attempt.
 *
 *	request.sync_folder.path = str;
 *	DtEnqueueRequest(theScreen,_DT_QUEUE(theDisplay), _DT_QUEUE(theDisplay),
 *					XtWindow(base.shell), &request);
 */
	if (owner) {
		sprintf(str, "%s/%s", GetXWINHome("desktop"), name);
		if (access(str,F_OK)) {
			result = is_dir? mkdir(str, 077):
					 symlink(ExpFileName(iobj), str);
		}
	}
}

static	void
installICON(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	DmObjectPtr	op;
	Boolean		chosen;
	int		n, cnt;
	char		str[80];

	for (cnt = n = 0; n < icon_count; n++) {
		XtSetArg(arg[0], XtNset, &chosen);
		OlFlatGetValues(I_BOX, n, arg, 1);
		if (chosen) {
			cnt++;
			XtSetArg(arg[0], XtNobjectData, &op);
			OlFlatGetValues(I_BOX, n, arg, 1);
			DTMInstall((icon_obj *)op->objectdata);
		}
	}
	if (cnt == 0)
		FooterMsg(GetGizmoText(string_noIcon));
}

static	void
cancelICON(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	BringDownPopup(w_popup[POPUP_ICON]);
}

static	void
cancelINFO(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	BringDownPopup(w_popup[POPUP_INFO]);
}

static	Widget
InfoCaption(Widget parent, char *label)
{
	Widget	w_cap;

	XtSetArg(arg[0], XtNposition,	(XtArgVal)OL_LEFT);
	XtSetArg(arg[1], XtNalignment,	(XtArgVal)OL_CENTER);
	XtSetArg(arg[2], XtNspace,	(XtArgVal)6);
	XtSetArg(arg[3], XtNlabel,	(XtArgVal)GetGizmoText(label));
	w_cap = XtCreateManagedWidget("caption", captionWidgetClass,
			parent, arg, 4);

	XtSetArg(arg[0], XtNwidth,	(XtArgVal)24*x3mm);
	return XtCreateManagedWidget("text", staticTextWidgetClass,
			w_cap, arg, 1);
}

CreateInfoSheet()
{
	Widget	w_up;

	CreateGizmo(base.shell, PopupGizmoClass, &info_popup, NULL, 0);

	w_popup[POPUP_INFO] = GetPopupGizmoShell(&info_popup);

	XtSetArg(arg[0], XtNupperControlArea, &w_up);
	XtGetValues(w_popup[POPUP_INFO], arg, 1);

	XtSetArg(arg[0], XtNvPad,	(XtArgVal)(y3mm));
	XtSetArg(arg[1], XtNvSpace,	(XtArgVal)(y3mm));
	XtSetArg(arg[3], XtNhSpace,	(XtArgVal)(x3mm));
	XtSetValues(w_up, arg, 4);

	W_NAME = InfoCaption(w_up, info_name);
	W_DESC = InfoCaption(w_up, info_desc);
	W_CAT  = InfoCaption(w_up, info_cat);
	W_VEND = InfoCaption(w_up, info_vendor);
	W_VERS = InfoCaption(w_up, info_version);
	W_ARCH = InfoCaption(w_up, info_arch);
	W_DATE = InfoCaption(w_up, info_date);
	W_SIZE = InfoCaption(w_up, info_size);
}

static	void
EndDropCB(Widget w, XtPointer files, XtPointer call_data)
{
	char	**path, str[BUFSIZ];
	Boolean	done;

	for (path = (char **)files; *path; path++) {
		if (access(*path, F_OK)==0 && strncmp(*path,"/tmp/",5)==0) {
			unlink(*path);
			done = FALSE;
		}
		else
			done = TRUE;
		sprintf(str, "%s %s %s", GetGizmoText(tag_instOp), &(*path[5]),
			done? GetGizmoText(tag_good): GetGizmoText(tag_bad));
		FREE(*path);
	}
	FREE(files);
}

static	void
DropIconCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	char			**files = (char **)NULL;
	OlFlatDropCallData	*drop = (OlFlatDropCallData *)call_data;
	DmObjectPtr		op;
	icon_obj		*iobj;
	Boolean			chosen, simple_exec = FALSE;
	char			str[PATH_MAX];
	int			ct, n;

	n = drop->item_data.item_index;
	XtSetArg(arg[0], XtNset, &chosen);
	OlFlatGetValues(I_BOX, n, arg, 1);
	if (!chosen) {
		ct = 1;
		XtSetArg(arg[0], XtNobjectData, &op);
		OlFlatGetValues(I_BOX, n, arg, 1);
		iobj = (icon_obj *)op->objectdata;
		if (!files) 
			files = (char **)MALLOC(2*sizeof(char *));
		if (iobj->pkg) {
			sprintf(str, "/tmp/%s", op->name);
			files[0] = STRDUP(str);
			symlink(ExpFileName(iobj), files[ct-1]);
			RegisterPkg(iobj->pkg, TRUE);
		}
		else {
			simple_exec = TRUE;
			files[0] = STRDUP(iobj->def);
		}
		files[1] = NULL;
	}
	else for (ct = n = 0; n < icon_count; n++) {
		XtSetArg(arg[0], XtNset, &chosen);
		OlFlatGetValues(I_BOX, n, arg, 1);
		if (chosen) {
			++ct;
			XtSetArg(arg[0], XtNobjectData, &op);
			OlFlatGetValues(I_BOX, n, arg, 1);
			iobj = (icon_obj *)op->objectdata;
			if (!files)
				files = (char **)MALLOC(2*sizeof(char *));
			else
				files = (char **)REALLOC(files,
							(ct+1)*sizeof(char *));
			if (iobj->pkg) {
				sprintf(str, "/tmp/%s", op->name);
				files[ct-1] = STRDUP(str);
				symlink(ExpFileName(iobj), files[ct-1]);
				RegisterPkg(iobj->pkg, TRUE);
			}
			else {
				simple_exec = TRUE;
				files[ct-1] = STRDUP(iobj->def);
			}
			files[ct] = NULL;
		}
	}
	if (ct)
		DtNewDnDTransaction(w, files, DT_B_STATIC_LIST,
					drop->root_info->root_x,
					drop->root_info->root_y,
					drop->ve->xevent->xbutton.time,
					drop->dst_info->window,
					simple_exec? DT_LINK_OP: DT_MOVE_OP,
					NULL,
					EndDropCB,
					(XtPointer)files);
}

CreateIconSheet()
{
	Widget		w_up;

	CreateGizmo(base.shell, PopupGizmoClass, &icon_popup, NULL, 0);

	w_popup[POPUP_ICON] = GetPopupGizmoShell(&icon_popup);
	XtSetArg(arg[0], XtNupperControlArea, &w_up);
	XtGetValues(w_popup[POPUP_ICON], arg, 1);

	XtSetArg(arg[0], XtNalignCaptions,	(XtArgVal)FALSE);
	XtSetArg(arg[1], XtNcenter,		(XtArgVal)FALSE);
	XtSetArg(arg[2], XtNvPad,		(XtArgVal)(y3mm));
	XtSetArg(arg[3], XtNvSpace,		(XtArgVal)(y3mm));
	XtSetArg(arg[4], XtNhPad,		(XtArgVal)(x3mm));
	XtSetArg(arg[5], XtNhSpace,		(XtArgVal)(x3mm));
	XtSetValues(w_up, arg, 6);

	XtSetArg(arg[0], XtNposition,	(XtArgVal)OL_TOP);
	XtSetArg(arg[1], XtNalignment,	(XtArgVal)OL_LEFT);
	XtSetArg(arg[2], XtNspace,	(XtArgVal)6);
	XtSetArg(arg[3], XtNlabel,	(XtArgVal)GetGizmoText(info_icons));
	I_TYPE = XtCreateManagedWidget("caption", captionWidgetClass,
			w_up, arg, 4);

	XtSetArg(arg[0], XtNheight,	(XtArgVal)(4*HEIGHT/5));
	XtSetArg(arg[1], XtNwidth,	(XtArgVal)WIDTH);
	I_SCW = XtCreateManagedWidget("package_window",
				scrolledWindowWidgetClass, I_TYPE, arg, 2);
}

static	void
GetInfo(p)
	PkgPtr		p;
{
	FooterMsg(NULL);
	SetPopupMessage(&info_popup, NULL);

	XtSetArg(arg[0], XtNstring, p->pkg_name); XtSetValues(W_NAME, arg, 1);
	XtSetArg(arg[0], XtNstring, p->pkg_desc); XtSetValues(W_DESC, arg, 1);
	XtSetArg(arg[0], XtNstring, p->pkg_cat);  XtSetValues(W_CAT,  arg, 1);
	XtSetArg(arg[0], XtNstring, p->pkg_vers); XtSetValues(W_VERS, arg, 1);
	XtSetArg(arg[0], XtNstring, p->pkg_arch); XtSetValues(W_ARCH, arg, 1);
	XtSetArg(arg[0], XtNstring, p->pkg_vend); XtSetValues(W_VEND, arg, 1);
	XtSetArg(arg[0], XtNstring, p->pkg_date); XtSetValues(W_DATE, arg, 1);
	XtSetArg(arg[0], XtNstring, p->pkg_size); XtSetValues(W_SIZE, arg, 1);
}

static	int	ibx, iby;

static	DmObjectPtr
AddInstallable(char *path, char *iname, DmFclassPtr fcp)
{
	DmObjectPtr	op, endp;
	icon_obj	*iobj;
	char		*ptr, *ptr2;

	if (icon_count < icon_cntrec.num_objs) {
		FREE(icon_itp[icon_count].label);
		icon_itp[icon_count].label = STRDUP(iname);
		icon_itp[icon_count].select = FALSE;
		icon_itp[icon_count].managed = TRUE;
		op = icon_itp[icon_count++].object_ptr;
		if (op->name)
			FREE(op->name);
		op->name = iname;
		op->fcp = fcp;
		iobj = (icon_obj *)op->objectdata;
		if (iobj->pkg) {
			FREE(iobj->pkg);
			iobj->pkg = NULL;
		}
		if (iobj->def)
			FREE(iobj->def);
	}
	else {
		iobj = (icon_obj *)CALLOC(1, sizeof(icon_obj));
		op = (DmObjectPtr)CALLOC(1, sizeof(DmObjectRec));
		op->name = iname;
		op->container = &icon_cntrec;
		op->objectdata = iobj;
		op->fcp = fcp;
		op->x = ibx;
		op->y = iby;
		if ((ibx += 4*INC_X/3) > WIDTH - MARGIN) {
			ibx = INIT_X;
			iby += INC_Y;
		}
		if (I_BOX) {
		/*
		 *	add new object to existing iconbox
		 *	(this use of Dm__AddToIcontainer is borrowed
		 *	from a similar context in nfs/container.c)
		*/
			int	nitems;
			XtSetArg(arg[0], XtNnumItems, &nitems);
			XtGetValues(I_BOX, arg, 1);
			icon_count++;
			Dm__AddObjToIcontainer(I_BOX, &icon_itp,
				&nitems, &icon_cntrec, op, ibx, iby,
				DM_B_CALC_SIZE | DM_B_NO_INIT,
				NULL, def_font, (Dimension)WIDTH, 0, 0);
		}
		else if (icon_count++ == 0)
			icon_cntrec.op = op;
		else {
			for (endp = icon_cntrec.op; endp->next; )
				endp = endp->next;
			endp->next = op;
		}
	}
	iobj->def = STRDUP(path);
	return op;
}

static	void
AddToExecs(char *name, FILE *dfp, char *format)
{
	DmObjectPtr	op;
	char		*ptr;

	while (fgets(buf, BUFSIZ, dfp)) {
		buf[strlen(buf)-1] = '\0';
		ptr = FilterExecutable(name, buf, format);
		if (ptr) {
			op = AddInstallable(ptr,
					    STRDUP(1+strrchr(ptr,'/')),
					    &exec_fcrec);
		}
	}
	fclose(dfp);
}

static	void
SelectFileCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmObjectPtr	op;
	OlFlatCallData	*d = (OlFlatCallData *)call_data;
	icon_obj	*iobj;
	char		*name;

	XtSetArg(arg[0], XtNobjectData, &op);
	OlFlatGetValues(w, d->item_index, arg, 1);
	iobj = (icon_obj *)op->objectdata;
	name = iobj->pkg? ToolboxName(iobj->def): iobj->def;
	SetPopupMessage(&icon_popup, name);
	DmIconSelect1Proc(w, client_data, call_data);
}

static	void
AddToIcons(char *name, FILE *dfp)
{
	DmObjectPtr	op;
	DmFclassPtr	fcp;
	icon_obj	*iobj;
	char		*ptr, *ptr2, *iname;

	while (fgets(buf, BUFSIZ, dfp)) {
		buf[strlen(buf)-1] = '\0';
		ptr = FilterIcon(buf);
		if (ptr) {
			*ptr++ = '\0';		/* isolate the ICON= field */
		/*
	 	*	get the glyph for it: 
	 	*/
			fcp = (DmFclassPtr)CALLOC(1, sizeof(DmFclassRec));
			if (fcp)
				fcp->glyph = DmGetPixmap(theScreen, buf+5);
			else
				fcp = &exec_fcrec;
		/*
	 	*	get the label for the glyph
	 	*/
			if (ptr2 = strchr(ptr, '\t')) {
				*ptr2 = '\0';
				iname = STRDUP(basename(DtamGetTxt(ptr)));
				*ptr2 = '\t';
			}
			else	/* shouldn't happen! */
				iname = STRDUP("???");
			op = AddInstallable(ptr, iname, fcp);
			iobj = (icon_obj *)op->objectdata;
			iobj->pkg = STRDUP(name);
		}
	}
	fclose(dfp);
}

static	void
SetPkgDefs(PkgPtr p)
{
	FILE	*icondef;
	char	*ptr;

	if (p->pkg_class)
		FREE(p->pkg_class);
	p->pkg_class = NULL;
	if (p->pkg_help)
		FREE(p->pkg_help);
	p->pkg_help  = NULL;
	sprintf(buf, "%s/%s", GetXWINHome(ICONDIR), p->pkg_name);
	if ((icondef=fopen(buf,"r")) == NULL)
		return;
	while(fgets(buf, BUFSIZ, icondef)) {	
		if (strncmp(buf,"HELP=",5)==0) {
			if (p->pkg_help == NULL)
				p->pkg_help = STRDUP(buf+5);
			else {
				p->pkg_help = (char *)REALLOC(p->pkg_help,
							strlen(p->pkg_help)+
								strlen(buf)-4);
				strcat(p->pkg_help,buf+5);
			}
		}
		else if (strncmp(buf,"CLASS=",6)==0) {
			buf[strlen(buf)-1] = '\0';
			if (p->pkg_class == NULL)
				p->pkg_class = STRDUP(buf+6);
			else {
				p->pkg_class = (char *)REALLOC(p->pkg_class,
							strlen(p->pkg_class)+
								strlen(buf)-4);
				strcat(strcat(p->pkg_class," "),buf+6);
			}
		}
	}
	fclose(icondef);
}

static	void
alignIcons()		/* borrowed with minimal adaption from nfs code */
{
	DmItemPtr	item;
	Dimension	width;
	Cardinal	nitems;

	XtSetArg(arg[0], XtNnumItems, &nitems);
	XtSetArg(arg[1], XtNwidth, &width);
	XtGetValues(I_BOX, arg, 2);
	width -= 20;		/* FIX: shouldn't be needed:  iconbox bug */
    
	for (item = icon_itp; item < icon_itp + nitems; item++)
		item->x = item->y = UNSPECIFIED_POS;
    
	for (item = icon_itp; item < icon_itp + nitems; item++) {
        	if (ITEM_MANAGED(item)) {
			DmGetAvailIconPos(icon_itp, nitems,
			      ITEM_WIDTH(item), ITEM_HEIGHT(item), width,
			      4*(Position)INC_X/3, INC_Y,
			      (Position *)&item->x, (Position *)&item->y);
		}
	}
}

static	void
SetBusy(Boolean waiting)
{
static	Cursor	busy = NULL, nonbusy;
	Widget	w_menu = GetMenu(&menu_bar);

	XtSetArg(arg[0], XtNsensitive, !waiting);
	OlFlatSetValues(w_menu, 0, arg, 1);
	OlFlatSetValues(w_menu, 1, arg, 1);
	OlFlatSetValues(w_menu, 2, arg, 1);
	if (!busy) {
		busy = GetOlBusyCursor(theScreen);
		nonbusy = GetOlStandardCursor(theScreen);
	}
	XDefineCursor(theDisplay, XtWindow(base.shell), waiting? busy: nonbusy);
	if (pkg_popup.shell) {
		w_menu = GetMenu(&pkg_menu);
		OlFlatSetValues(w_menu, 0, arg, 1);
		OlFlatSetValues(w_menu, 1, arg, 1);
		OlFlatSetValues(w_menu, 2, arg, 1);
		OlFlatSetValues(w_menu, 3, arg, 1);
		XDefineCursor(theDisplay, XtWindow(pkg_popup.shell),
						waiting? busy: nonbusy);
	}
}

static	void
GetIconView(p)
	PkgPtr	p;
{
	int	n;
	FILE	*idata;

	FooterMsg(NULL);
	SetPopupMessage(&icon_popup, NULL);
	SetBusy(TRUE);

	sprintf(buf, GetGizmoText(string_pkgTitle), p->pkg_name);
	XtSetArg(arg[0], XtNtitle, buf);
	XtSetValues(icon_popup.shell, arg, 1);
	icon_count = 0;
	ibx = INIT_X, iby = INIT_Y;

	if (p->pkg_class == NULL && p->pkg_help == NULL)
		SetPkgDefs(p);

	sprintf(buf, "%s/%s", ICONDIR, p->pkg_name);
	if (idata=fopen(GetXWINHome(buf),"r"))
		AddToIcons(p->pkg_name, idata);

	if (icon_count > 0) {
		XtSetArg(arg[0], XtNlabel, GetGizmoText(info_icons));
		XtSetValues(I_TYPE, arg, 1);
	}
	else {
		XtSetArg(arg[0], XtNlabel, "" /* GetGizmoText(info_execs) */);
		XtSetValues(I_TYPE, arg, 1);
		if (p->pkg_fmt[0] == 'C') {
			sprintf(buf, "%s/%s", PERMS,
					p->pkg_set? p->pkg_set: p->pkg_name);
			if (idata=fopen(buf,"r"))
				AddToExecs(p->pkg_name, idata, "C");
		}
		else if (idata=fopen(LOGFILE,"r")) {
			AddToExecs(p->pkg_name, idata, "4.0");
		}
		if (idata == NULL)
			FooterMsg(GetGizmoText(string_badLog));
	}
	if (I_BOX) {
		int	nitems;

		XtSetArg(arg[0], XtNnumItems, &nitems);
		XtGetValues(I_BOX, arg, 1);
		for (n = icon_count; n < nitems; n++) {
			icon_itp[n].managed = FALSE;
		}
		alignIcons();
		XtSetArg(arg[0], XtNitemsTouched, TRUE);
		XtSetValues(I_BOX, arg, 1);
	}
	else {
		icon_cntrec.num_objs = icon_count;
		XtSetArg(arg[0], XtNselectProc,		(XtArgVal)SelectFileCB);
		XtSetArg(arg[1], XtNmovableIcons,	(XtArgVal)FALSE);
		XtSetArg(arg[2], XtNminWidth,		(XtArgVal)1);
		XtSetArg(arg[3], XtNminHeight,		(XtArgVal)1);
		XtSetArg(arg[4], XtNdrawProc,		(XtArgVal)DmDrawIcon);
		XtSetArg(arg[5], XtNdropProc,		(XtArgVal)DropIconCB);

		I_BOX = DmCreateIconContainer(I_SCW, DM_B_CALC_SIZE, arg, 6,
				icon_cntrec.op, icon_count, &icon_itp,
				icon_count, NULL, NULL, def_font, 1);
	}
	SetBusy(FALSE);
}

static	void
DoPopup(p_select, index)
	PkgPtr	p_select;
	int	index;
{
	if (!p_select) {
		FooterMsg(GetGizmoText(string_noSelect));
		return;
	}
	if (index == POPUP_INFO)
		GetInfo(p_select);
	else
		GetIconView(p_select);
	MapGizmo(PopupGizmoClass, index==POPUP_INFO? &info_popup: &icon_popup);
}

/*
 *	note: CallCustomDel should deal with queueing multiple requests.
 *	Multiple independent invocations of custom will "work" -- but
 *	leave the post-custom cleanup (CheckDelete) confused (this can
 *	also happen if a custom and a pkgrm are spun off by deleteCB.)
 */
static	void
CallCustomDel(char *prdname, char *pkglist)
{
	char	*str;

	str = (char *)MALLOC(128+2*strlen(prdname)+2*strlen(pkglist));
	if (str == NULL) {
		FooterMsg(GetGizmoText(string_badCustom));
		return;
	}
	sprintf(str, "%s\"%s %s (%s)\" -e %s/dtexec -Z %s -s %s -r \"%s\"",
			XTERM,GetGizmoText(string_remTitle),prdname,pkglist+1,
			GetXWINHome("adm"),CUSTOM,prdname,pkglist+1);
	setuid(0);
	if (_Dtam_p3open(str, cmdfp, FALSE) != -1) {
		FooterMsg(GetGizmoText(string_invokeCustom));
		GIBtimeout = XtAddTimeOut(1000, (XtTimerCallbackProc)WaitPkgOp,
				strcmp(pkglist," ALL")==0? prdname:
							   STRDUP(pkglist));
	}
	else {
		setuid(our_uid);
		FooterMsg(GetGizmoText(string_badCustom));
	}
	FREE(str);
}

static	void
CallRemovePkg(char *name)
{
	char	str[128];

	sprintf(str, "%s\"%s%s\" -e %s/dtexec -Z /usr/bin/removepkg", XTERM,
					GetGizmoText(string_remTitle), name,
					GetXWINHome("adm"));
	setuid(0);
	if (_Dtam_p3open(str, cmdfp, FALSE) != -1) {
		FooterMsg(GetGizmoText(string_invokeRemPkg));
		GIBtimeout = XtAddTimeOut(1000, (XtTimerCallbackProc)Wait3_2Pkg,
									NULL);
	}
	else {
		setuid(our_uid);
		FooterMsg(GetGizmoText(string_badRemPkg));
	}
}

static	void
addCB(w, client_data, call_data)
	Widget		w;
	XtPointer	client_data;
	XtPointer	call_data;
{
	char	*ptr, names[BUFSIZ];
	Boolean	chosen;
	Boolean	cust_fmt = FALSE;
	int	n;
	PkgPtr	p_sel = GetSelectedPkg();

	if (!p_sel) {
		FooterMsg(GetGizmoText(string_noSelect));
		return;
	}
	for (*names = '\0', n = 0; n < p_item_count; n++) {
		XtSetArg(arg[0], XtNset, &chosen);
		OlFlatGetValues(w_iconbox, n, arg, 1);
		if (chosen) {
			sprintf(names+strlen(names)," %s",add_list[n].pkg_name);
			cust_fmt = (add_list[n].pkg_fmt[0] == 'C');
		}
	}
	if (*names == '\0')
		return;
	/*
	 *	note: the following is adapted to adding either a list of
	 *	pkgadd format add-ons, or a *single* custom format add-on.
	 *	If multiple custom packages are found, e.g. in a server
	 *	context, or if custom and pkgadd formats can be mixed, it
	 *	will be necessary to run several commands in succession --
	 *	a single pkgadd will handle all the pkgadd format add-ons,
	 *	at least one subsequent invocation of custom will be needed
	 *	to handle that format (probably, one should do a single call
	 *	to custom with the FIRST name in the list, and assume that
	 *	the user will have go on to handle the others wanted.
	 */
	if (cust_fmt) {
		char	c = curalias[strlen(curalias)-1] - 1;
		if (c == '0')
			c = ' ';
		sprintf(buf,
		    "%s\"%s%s\" -e %s/dtexec -Z %s -a %s -m /dev/install%c",
		    		XTERM, GetGizmoText(string_addTitle), names,
				GetXWINHome("adm"), CUSTOM, "ALL", c);
		setuid(0);
	}
	else
		sprintf(buf, "%s\"%s%s\" -e %s/dtexec %s -p -q -d %s %s", XTERM,
			GetGizmoText(string_addTitle), names,
			GetXWINHome("adm"), ADDPKG,
			curalias==spooled? spooldir: curalias, names);
	if (_Dtam_p3open(buf, cmdfp, FALSE) != -1) {
		FooterMsg(GetGizmoText(cust_fmt? string_invokeCustom:
						 string_invokePkgOp));
		GIBtimeout = XtAddTimeOut(1000, (XtTimerCallbackProc)WaitPkgOp,
								STRDUP(names+1));
		SetBusy(TRUE);
	}
	else {
		setuid(our_uid);
		FooterMsg(GetGizmoText(string_badPkgOp));
		GIBtimeout = 0;
	}
}

static	void
deleteCB(w, client_data, call_data)
	Widget		w;
	XtPointer	client_data;
	XtPointer	call_data;
{
	int		n;
	char		*ptr, names[BUFSIZ];
	Boolean		did_svr3_2 = FALSE;
	Boolean		chosen;
	DmObjectPtr	optr;
	PkgPtr		p_sel;

	if (select_popup) {
		deletePKG(NULL, client_data, call_data);
		return;
	}
	p_sel = GetSelectedPkg();
	if (!p_sel) {
		FooterMsg(GetGizmoText(string_noSelect));
		return;
	}
	for (*names = '\0', n = 0; n < p_item_count; n++) {
		XtSetArg(arg[0], XtNset, &chosen);
		XtSetArg(arg[1], XtNobjectData, &optr);
		OlFlatGetValues(w_iconbox, n, arg, 2);
		p_sel = (PkgPtr)optr->objectdata;
		if (chosen) {
			switch(p_sel->pkg_fmt[0]) {
		case '3':	if (!did_svr3_2) {
					did_svr3_2 = TRUE;
					CallRemovePkg(p_sel->pkg_name);
					/*
					 *	removepkg will ask for
					 *	specific packaged via menus
					 */
				}
				break;
		case 'C':	CallCustomDel(p_sel->pkg_name, " ALL");
				break;
		default:	strcat(strcat(names," "), p_sel->pkg_name);
				break;
			}
			SetBusy(TRUE);
		}
	}
	if (*names == '\0')
		return;
	sprintf(buf, "%s\"%s%s\" -e %s/dtexec %s %s", XTERM,
			GetGizmoText(string_remTitle), names, 
			GetXWINHome("adm"), REMPKG, names);
	if (_Dtam_p3open(buf, cmdfp, FALSE) != -1) {
		FooterMsg(GetGizmoText(string_invokePkgOp));
		GIBtimeout = XtAddTimeOut(1000, (XtTimerCallbackProc)WaitPkgOp,
							STRDUP(names));
	}
	else {
		FooterMsg(GetGizmoText(string_badPkgOp));
		GIBtimeout = 0;
	}
}

static	void
deletePKG(w, client_data, call_data)
	Widget		w;
	XtPointer	client_data;
	XtPointer	call_data;
{
	int		n;
	char		*ptr, *names = NULL;
	Boolean		chosen;
	Boolean		cust_type;
	Boolean		delete_all = TRUE;
	DmObjectPtr	optr;
	PkgPtr		p_sel = GetSelectedPkg();

	if (!select_popup || !p_sel) {
		SetPopupMessage(&pkg_popup, GetGizmoText(string_noSelect));
		return;
	}
	cust_type = (p_sel->pkg_fmt[0] == 'C');
	for (n = 0; n < p_item_count; n++) {
		XtSetArg(arg[0], XtNset, &chosen);
		XtSetArg(arg[1], XtNobjectData, &optr);
		OlFlatGetValues(w_pkgbox, n, arg, 2);
		p_sel = (PkgPtr)optr->objectdata;
		if (chosen) {
			if (names == NULL) {
				names = (char *)MALLOC(2+strlen(p_sel->pkg_name));
				*names = '\0';
			}
			else
				names = (char *)REALLOC(names,2+strlen(names)+
						strlen(p_sel->pkg_name));
			sprintf(names+strlen(names) ," %s", p_sel->pkg_name);
		}
		else if (p_sel->pkg_opflag != 'D')
			delete_all = FALSE;
	}
	if (delete_all)  {
		names = (char *)REALLOC(names, cust_type? 5:
						strlen(p_sel->pkg_set)+1);
		strcpy(names, (cust_type? " ALL": p_sel->pkg_set));
	}
	if (cust_type)
		CallCustomDel(p_sel->pkg_set, names);
	else {
		sprintf(buf, "%s\"%s%s\" -e %s/dtexec %s %s", XTERM,
					GetGizmoText(string_remTitle), names, 
					GetXWINHome("adm"), REMPKG, names);
		if (_Dtam_p3open(buf, cmdfp, FALSE) != -1) {
			SetPopupMessage(&pkg_popup,
					GetGizmoText(string_invokePkgOp));
			GIBtimeout = XtAddTimeOut(1000,
					(XtTimerCallbackProc)WaitPkgOp,
					STRDUP(names));
		}
		else {
			SetPopupMessage(&pkg_popup,
					GetGizmoText(string_badPkgOp));
			GIBtimeout = 0;
		}
	}
	FREE(names);
}

static	void
infoCB(w, client_data, call_data)
	Widget		w;
	XtPointer	client_data;
	XtPointer	call_data;
{
	PkgPtr	p_select = GetSelectedPkg();

	DoPopup(p_select, POPUP_INFO);
}

static	void
iconCB(w, client_data, call_data)
	Widget		w;
	XtPointer	client_data;
	XtPointer	call_data;
{
	PkgPtr	p_select = GetSelectedPkg();

	if (!p_select)
		FooterMsg(GetGizmoText(string_noSelect));
	else if (strcmp(p_select->pkg_cat,"set") == 0)
		CreatePkgIcons(p_select);
	else {
		SetPopupMessage(&icon_popup, NULL);
		DoPopup(p_select, POPUP_ICON);
	}
}

static	void
SetSensitivity(void)
{
	XtSetArg(arg[0], XtNsensitive, curalias==sysalias);
	OlFlatSetValues(w_acts, 0, arg, 1);	/* show contents */

	if (owner) {	
	/* delete */
		OlFlatSetValues(w_edit, 1, arg, 1);
	/* properties */
		OlFlatSetValues(w_edit, 2, arg, 1);
	/* add */
		XtSetArg(arg[0], XtNsensitive, curalias!=sysalias);
		OlFlatSetValues(w_edit, 0, arg, 1);
	}
}

static	void
SelectPkg(w, client_data, call_data)
	Widget		w;
	XtPointer	client_data;
	XtPointer	call_data;
{
	DmObjectPtr	op;
	PkgPtr		pp;
	OlFlatCallData	*d = (OlFlatCallData *)call_data;

	select_popup = (w == w_pkgbox);
	DmIconSelect1Proc(w, client_data, call_data);
	XtSetArg(arg[0], XtNobjectData, &op);
	OlFlatGetValues(w, d->item_index, arg, 1);
	pp = (PkgPtr)op->objectdata;
	FooterMsg(PkgLabel(pp));
}

static	void
Click2Pkg(w, client_data, call_data)
	Widget		w;
	XtPointer	client_data;
	XtPointer	call_data;
{
	DmObjectPtr	op;
	PkgPtr		pp;
	OlFlatCallData	*d = (OlFlatCallData *)call_data;

	if (curalias == sysalias) {
		SelectPkg(w, client_data, call_data);
		XtSetArg(arg[0], XtNobjectData, &op);
		OlFlatGetValues(w, d->item_index, arg, 1);
		pp = (PkgPtr)op->objectdata;
		if (strcmp(pp->pkg_cat,"set") == 0)
			CreatePkgIcons(pp);
		else {
			SetPopupMessage(&icon_popup, NULL);
			DoPopup(pp, POPUP_ICON);
		}
	}
}

static	void
AddItem(PkgPtr p, DmContainerRec *cntrec_ptr, Boolean no_sub)
{
	DmObjectPtr	optr;

	if (p->pkg_opflag == 'D')
		return;
	if (no_sub && p->pkg_set != NULL) /* packages in sets should be shown */
		return;		  /* in a separate popup icon window. */
	switch (view_type) {
		/*
		 *	filter out inappropriate icons to a view
		 */
	case SYS_VIEW:	if (strcmp(p->pkg_cat,"set")==0) {
				/* show it unless it's Custom */;
				if (p->pkg_fmt[0] == 'C')
					return;
				else
					break;
			}
			else if (strcmp(p->pkg_cat,APPL)==0)
				return;
			break;
	case APP_VIEW:	if (p->pkg_fmt[0] == 'C')
				break;
			if (strcmp(p->pkg_cat,"set")==0) {
				PkgPtr	q;
			/*
			 *	scan for application packages in the set
			 */
				for (q = sys_list; q < sys_list+sys_count; q++)
					if (q->pkg_set
					&& strcmp(q->pkg_set,p->pkg_name)==0
					&& strcmp(q->pkg_cat,APPL)==0)
						break;
				if (q == sys_list+sys_count)
					return;
			}
			else if (strcmp(p->pkg_cat,APPL) != 0)
				return;
			break;
	case ALL_VIEW:	
	default:	break;
	}
	optr = (DmObjectPtr)CALLOC(1, sizeof(DmObjectRec));
	optr->container = cntrec_ptr;
	optr->fcp = strcmp(p->pkg_cat, "set")==0? &set_fcrec: &pkg_fcrec;
	optr->name = p->pkg_name;
	optr->x = p_ibx;
	optr->y = p_iby;
	optr->objectdata = (XtPointer)p;
	if ((p_ibx += INC_X) > 9*WIDTH/10 - MARGIN) {
		p_ibx = INIT_X;
		p_iby += INC_Y;
	}
	if (p_item_count++ == 0) {
		cntrec_ptr->op = optr;
	}
	else {
		DmObjectPtr endp = cntrec_ptr->op;
		while (endp->next)
			endp = endp->next;
		endp->next = optr;
	}
	cntrec_ptr->num_objs = p_item_count;
}

static	void
CreateSetIcons(XtPointer parent)
{
	int	n, count;
	PkgPtr	list;

	if (w_iconbox) {
		XtUnmanageChild(w_iconbox);
		XtDestroyWidget(w_iconbox);
		w_iconbox = (Widget)NULL;
	}
	if (curalias == sysalias) {
		list = sys_list;
		count = sys_count;
	}
	else {
		list = add_list;
		count = add_count;
	}
	p_item_count = 0; p_ibx = INIT_X; p_iby = INIT_Y;
	for (n = 0; n < count; n++)
		AddItem(&list[n], &set_cntrec, TRUE);
	XtSetArg(arg[0], XtNselectProc,		(XtArgVal)SelectPkg);
	XtSetArg(arg[1], XtNmovableIcons,	(XtArgVal)FALSE);
	XtSetArg(arg[2], XtNminWidth,		(XtArgVal)1);
	XtSetArg(arg[3], XtNminHeight,		(XtArgVal)1);
	XtSetArg(arg[4], XtNdrawProc,		(XtArgVal)DmDrawIcon);
	XtSetArg(arg[5], XtNdblSelectProc,	(XtArgVal)Click2Pkg);

	w_iconbox = DmCreateIconContainer((Widget)parent, DM_B_CALC_SIZE,
			arg, 6, set_cntrec.op, p_item_count, &set_itp,
			p_item_count, NULL, NULL, def_font, 1);
	if (curalias != sysalias)
		sprintf(buf,GetGizmoText(string_spoolpkgs), p_item_count);
	else {
		char	*type = curlabel;
		if (strcmp(type, GetGizmoText(label_all)) == 0)
			type = "";
		sprintf(buf, GetGizmoText(string_syspkgs), p_item_count,
					type, our_node);
	}
	FooterMsg(buf);
}

static	void
CreatePkgIcons(PkgPtr pp)
{
static	Widget	w_pkgscw = NULL;
	int	n;
	char	*type = curlabel;
	char	*set_name = pp->pkg_name;

	if (w_pkgbox) {
		XtUnmanageChild(w_pkgbox);
		XtDestroyWidget(w_pkgbox);
		w_pkgbox = (Widget)NULL;
	}
	if (!w_pkgscw) {
		Widget	w_shell, w_up, w_menu;

		w_shell = CreateGizmo(base.shell, PopupGizmoClass, &pkg_popup,
								NULL, 0);
		XtSetArg(arg[0], XtNupperControlArea, &w_up);
		XtGetValues(w_shell, arg, 1);
		XtSetArg(arg[0], XtNheight,	(XtArgVal)(4*HEIGHT/5));
		XtSetArg(arg[1], XtNwidth,	(XtArgVal)WIDTH);
		w_pkgscw = XtCreateManagedWidget("package_window",
				scrolledWindowWidgetClass, w_up, arg, 2);
		if (!owner) {
			w_menu = GetMenu(&pkg_menu);
			XtSetArg(arg[0], XtNsensitive, FALSE);
			OlFlatSetValues(w_menu, 2, arg, 1);
		}
	}
	p_item_count = 0; p_ibx = INIT_X; p_iby = INIT_Y;
	for (n = 0; n < sys_count; n++)
		if (sys_list[n].pkg_set
		&&  strcmp(sys_list[n].pkg_set, set_name) == 0)
			AddItem(&sys_list[n], &pkg_cntrec, FALSE);
	XtSetArg(arg[0], XtNselectProc,		(XtArgVal)SelectPkg);
	XtSetArg(arg[1], XtNmovableIcons,	(XtArgVal)FALSE);
	XtSetArg(arg[2], XtNminWidth,		(XtArgVal)1);
	XtSetArg(arg[3], XtNminHeight,		(XtArgVal)1);
	XtSetArg(arg[4], XtNdrawProc,		(XtArgVal)DmDrawIcon);
	XtSetArg(arg[5], XtNdblSelectProc,	(XtArgVal)Click2Pkg);

	w_pkgbox = DmCreateIconContainer(w_pkgscw, DM_B_CALC_SIZE,
			arg, 6, pkg_cntrec.op, p_item_count, &pkg_itp,
			p_item_count, NULL, NULL, def_font, 1);
	if (strcmp(type, GetGizmoText(label_all)) == 0)
		type = "";
	sprintf(buf, GetGizmoText(format_numPkgs), p_item_count, type,set_name);
	SetPopupMessage(&pkg_popup, buf);
	sprintf(buf, GetGizmoText(string_pkgTitle), pp->pkg_desc);
	XtSetArg(arg[0], XtNtitle, buf);
	XtSetValues(pkg_popup.shell, arg, 1);
	MapGizmo(PopupGizmoClass, &pkg_popup);
}

static	void
GetPkgBox(XtPointer dummy, XtIntervalId intid)
{
	char	str[80], *where;

	if (curalias==sysalias || curalias==spooled && have_dir || have_medium)
		GIBtimeout = (XtIntervalId)NULL;	/* no more calls! */
	else {
		GIBtimeout = XtAddTimeOut(500, (XtTimerCallbackProc)GetPkgBox,
							NULL);
		return;
	}
	lastalias = curalias;
	if (curalias == sysalias) {
		where = our_node;
		sprintf(str, "%s%s -l 2>&1", LANG_C, PKGINFO);
	}
	else {
		where = (curalias == spooled? spooldir: curalias);
		sprintf(str, "%s%s -p -d %s 2>&1", LANG_C, ADDPKG, where);
		if (where == curalias)
			where = curlabel;
		if (add_max == 0) {
			add_max = QUANTUM;
			add_list = (PkgPtr)MALLOC((add_max+1)*sizeof(PkgRec));
		}
		if (add_count) {
			FreePkgList(add_list, &add_count);
			add_count = 0;
		}
	}
	if (have_medium == DTAM_CUSTOM) {
	/*
	 *	Get the name of the "product" and the "set" description
	 *	from the initial header and perms file of the TAR medium
	 *	This will read from the same fp as if we did a p3open.
	 */
		char	*devline = DtamGetDev(curalias, FIRST);
		char	*cdev = DtamDevAttr(devline, CDEVICE);
		if (cmdfp[1] = fopen(cdev,"r")) {
			*buf = '\0';
			fcntl(fileno(cmdfp[1]), F_SETFL, O_NONBLOCK);
			GIBtimeout = XtAddTimeOut(2500,
					(XtTimerCallbackProc)ReadCustom, NULL);
			sprintf(str, GetGizmoText(format_wait), where);
		}
		else
			sprintf(str, GetGizmoText(format_cantRead), curlabel);
		if (cdev)
			FREE(cdev);
		if (devline)
			FREE(devline);
	}
	else if (_Dtam_p3open(str, cmdfp, FALSE) != -1) {
		*buf = '\0';
		GIBtimeout = XtAddTimeOut(2500,(XtTimerCallbackProc)ReadPkgList,
							NULL);
		sprintf(str, GetGizmoText(format_wait), where);
	}
	else
		sprintf(str, GetGizmoText(format_cantRead), curlabel);
	SetModalGizmoMessage(&info_msg, str);
	MapGizmo(ModalGizmoClass, &info_msg);
	XSync(theDisplay, FALSE);
}

static	void
CheckDelete(char *names)
{
	PkgPtr	p, q, set_ptr = NULL;
	char	*ptr, list[BUFSIZ];
	Boolean	failed = FALSE;
	Boolean	some_left = FALSE;
	int	n;

	*list = '\0';
	for (p = sys_list; p < sys_list+sys_count; p++) {
		if ((ptr=strstr(names, p->pkg_name))== NULL
		|| ptr[n=strlen(p->pkg_name)] != ' ' && ptr[n] != '\0')
			continue;
		/*
		 *	this package (or set) was in the list for deletion
		 */
		if (strcmp(p->pkg_cat,"set") != 0) {
			if (select_popup)
				some_left = TRUE;
			if (p->pkg_set && set_ptr == NULL) {
				for (q=sys_list; q < sys_list+sys_count; q++) {
					if (strcmp(q->pkg_name,p->pkg_set)==0) {
						set_ptr = q;
						break;
					}
				}
			}
			if (PkgInstalled(p)) {
				failed = TRUE;
				strcat(strcat(list," "), p->pkg_name);
			}
			else {
				p->pkg_opflag = 'D';
				if (p->pkg_help)
					RegisterHelp(p->pkg_help, FALSE);
				if (p->pkg_class)
					RegisterClass(p->pkg_class, FALSE);
			}
		}
		else {
			set_ptr = p;
			if (PkgInstalled(p)) {
				failed = TRUE;
				strcat(strcat(list," "), p->pkg_name);
			}
			else
				p->pkg_opflag = 'D';
			for (q=sys_list; q < sys_list+sys_count; q++) {
				if (q->pkg_set
				&&  strcmp(q->pkg_set,p->pkg_name)==0
				&&  q->pkg_opflag != 'D') {
					if (PkgInstalled(q))
						some_left = TRUE;
					else {
						q->pkg_opflag = 'D';
						if (q->pkg_help)
							RegisterHelp
							    (q->pkg_help,FALSE);
						if (q->pkg_class)
							RegisterClass
							    (q->pkg_class,FALSE);
					}
				}
			}
		}
	}
	if (failed)
		sprintf(buf,GetGizmoText(format_opFmt),GetGizmoText(tag_delOp),
				list, GetGizmoText(tag_bad));
	else
		sprintf(buf,GetGizmoText(format_opFmt),GetGizmoText(tag_delOp),
				names, GetGizmoText(tag_good));
	if (select_popup) {
		if (failed || some_left) {
			SetPopupMessage(&pkg_popup, buf);
			CreatePkgIcons(set_ptr);
			return;
		}
		else {
			set_ptr->pkg_opflag = 'D';
			cancelPKG(NULL, NULL, NULL);
		}
	}
	FooterMsg(buf);
	CreateSetIcons(base.scroller);
}

static	void
CheckAdd(char *names)
{
	PkgPtr	p;
	char	*ptr, list[BUFSIZ];
	Boolean	failed = FALSE;
	int	n;
	int	index;

	*list = '\0';
	for (index = 0; index < add_count; index++) {
	        p = add_list + index;
		if ((ptr=strstr(names, p->pkg_name))== NULL
		|| ptr[n=strlen(p->pkg_name)] != ' ' && ptr[n] != '\0') {
			p->pkg_opflag = '\0';
			continue;
		}
		if (!PkgInstalled(p)) {
			failed = TRUE;
			p->pkg_opflag = 'T';
			strcat(strcat(list," "), p->pkg_name);
		}
		else {
			FILE	*fp;
			PkgPtr	q;
			/*
			 *	installation succeeded
			 *	(maybe not; check installation date
			 *	in case the package already existed)
			 */
			p->pkg_opflag = 'F';
			sprintf(buf, "%s/%s/setinfo", PKGDIR, p->pkg_name);
			if ((fp = fopen(buf, "r")) == NULL) {
				SetPkgDefs(p);
				RegisterAll(p);
			}
			else {
				while (fgets(buf, BUFSIZ, fp)) {
					if (*buf && *buf != '#' && *buf != '\n') {
						if (ptr = strchr(buf,'\t'))
							*ptr = ' ';
						AddPackage(add_count, buf);
				/* WARNING: p is now an invalid pointer */
				/* because add_list has been realloc'd  */
						q = add_list+add_count;
						add_count++;
						SetPkgDefs(q);
						RegisterAll(q);
					}
				}
				fclose(fp);
			}
		}
	}
	if (failed)
		sprintf(buf,GetGizmoText(format_opFmt),GetGizmoText(tag_addOp),
				list, GetGizmoText(tag_bad));
	else {
		sprintf(buf,GetGizmoText(format_opFmt),GetGizmoText(tag_addOp),
				names, GetGizmoText(tag_good));
		TellUsers();
	}
	FooterMsg(buf);
}

static	void
WaitPkgOp(char *names, XtIntervalId intid)
{
	char	str[BUFSIZ];

	if (cmdfp[1] == NULL || read(fileno(cmdfp[1]), str, BUFSIZ) == 0) {
		setuid(our_uid);
		SetBusy(FALSE);
		GIBtimeout = (XtIntervalId)NULL;	/* no more calls! */
		if (cmdfp[1]) {
			_Dtam_p3close(cmdfp, SIGTERM);
			cmdfp[0] = cmdfp[1] = (FILE *)0;
		}
		if (curalias==sysalias)
			CheckDelete(names);
		else
			CheckAdd(names);
		FREE(names);
	}
	else
		GIBtimeout = XtAddTimeOut(1000, (XtTimerCallbackProc)WaitPkgOp,
								names);
}

static	void
Wait3_2Pkg(XtPointer dummy, XtIntervalId intid)
{
	char	str[BUFSIZ];

	if (cmdfp[1] == NULL || read(fileno(cmdfp[1]), str, BUFSIZ) == 0) {
		setuid(our_uid);
		SetBusy(FALSE);
		GIBtimeout = (XtIntervalId)NULL;	/* no more calls! */
		curalias = sysalias;
		curlabel = "";
		view_type = ALL_VIEW;
		FooterMsg(NULL);
		FindInstalled();
		sprintf(buf, GetGizmoText(string_instTitle),
						GetGizmoText(label_all));
		SetBaseWindowTitle(&base, buf);
	}
	else
		GIBtimeout = XtAddTimeOut(1000, (XtTimerCallbackProc)Wait3_2Pkg,
								NULL);
}

static	void
cancelPKG(Widget wid, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(pkg_popup.shell);
	select_popup = FALSE;
}

static	void
cancelMSG(Widget wid, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(info_msg.shell);
	if (cmdfp[1]) {
		_Dtam_p3close(cmdfp, SIGTERM);
		cmdfp[0] = cmdfp[1] = (FILE *)NULL;
	}
}

static	void
cancelNOTE(Widget wid, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(insert_note.shell);
	if (GIBtimeout) {
		XtRemoveTimeOut(GIBtimeout);
		GIBtimeout = (XtIntervalId)NULL;
	}
	if (media_context == FIRST)
		exit(0);
}

static	void
CallInstallPkg(char *str)
{
	sprintf(str, "%s\"%s\" -e %s/dtexec -Z /usr/bin/installpkg", XTERM,
		GetGizmoText(string_svr3Title), GetXWINHome("adm"));
	setuid(0);
	if (_Dtam_p3open(str, cmdfp, FALSE) != -1) {
		FooterMsg(GetGizmoText(string_invokeInstPkg));
		GIBtimeout = XtAddTimeOut(1000,
			(XtTimerCallbackProc)Wait3_2Pkg, NULL);
		BringDownPopup(insert_note.shell);
		SetBusy(TRUE);
	}
	else {
		setuid(our_uid);
		SetModalGizmoMessage(&insert_note,
					GetGizmoText(string_badInstPkg));
	}
}

static	void
GetMedia(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	char    *ptr, cmdbuf[BUFSIZ];
	char    mnt_pt[] = "/install";
	char    target[] = "/install/install/INSTALL";
	Boolean	Is_Installpkg;
	int     result;
	int	diagn;
	char	*vol, *dev, str[256];

	have_medium = 0;
	if ((diagn=DtamCheckMedia(curalias)) == (DTAM_CPIO|DTAM_PACKAGE)) {
		BringDownPopup(insert_note.shell);
		have_medium = DTAM_PACKAGE;
	}
	else if (diagn == DTAM_CUSTOM) {
		BringDownPopup(insert_note.shell);
		have_medium = DTAM_CUSTOM;
	}
	else if (diagn == (DTAM_CPIO|DTAM_INSTALL) || diagn == DTAM_CPIO) {
		CallInstallPkg(str);
		return;
	}
	else if (diagn & DTAM_FS_TYPE) {
		/* let's verify that this is installpkg or not */
		dev = NULL;
		if (dev = DtamGetDev(curalias,FIRST))
			ptr = DtamDevAttr(dev,BDEVICE);
		if (dev == NULL || ptr == NULL) {
			if (dev != NULL) FREE (dev);
			return;
		}
			
		if ((_dtam_flags & DTAM_TFLOPPY) == 0)
			/* if this is a file system without trailing 't' */
			ptr[strlen(ptr)-1] = '\0';
		sprintf(cmdbuf, "/sbin/tfadmin fmount -r -F s5 %s %s",
                                ptr, mnt_pt);
		FREE(ptr);
		FREE(dev);

		/* let's mount it */
		result = system(cmdbuf);
		if (result != 0)
			return;
		/* if install/INSTALL exist, it must be a installpkg */
		if (access(target, F_OK) == 0)
			Is_Installpkg = True;
		else
			Is_Installpkg = False;
		/* let's umount it. */
		sprintf(cmdbuf, "/sbin/tfadmin fumount %s", mnt_pt);
		result = system(cmdbuf);
		if (result != 0)
			return;

	     	if (Is_Installpkg) {
			/*
			 *	guess it is installpkg format
			 */
			CallInstallPkg(str);
			return;
		}
		else {
			BringDownPopup(insert_note.shell);
			have_medium = DTAM_PACKAGE;
			/*
			 *	may be a file-system pkgadd; try it 
			 */
		}
	}
	else {
		vol = dev = NULL;
		if (dev = DtamGetDev(curalias,FIRST))
			if ((vol=DtamDevAttr(dev,"volume")) == NULL)
				vol = STRDUP(GetGizmoText(label_medium));
		if (diagn == DTAM_NO_DISK)
			*str = '\0';
		else
			sprintf(str, GetGizmoText(format_notPkg), vol);
		sprintf(str+strlen(str), GetGizmoText(format_insFmt),
				vol, curlabel, GetGizmoText(label_go));
		if (dev) FREE(dev);
		if (vol) FREE(vol);
		SetModalGizmoMessage(&insert_note, str);
		MapGizmo(ModalGizmoClass, &insert_note);
		return;
	}
	media_context = NEXT;
	GetPkgBox(NULL, NULL);
}

static	void
CreateFolderPrompt()
{
	prompt = CopyGizmo(FileGizmoClass, &spool_prompt);
	prompt->title = GetGizmoText(string_spoolPrompt);
	prompt->directory = spooldefault;
	CreateGizmo(base.shell, FileGizmoClass, prompt, NULL, 0);
	SetFileGizmoMessage(prompt, GetGizmoText(string_promptMsg));
	XtUnmanageChild(XtParent(prompt->textFieldWidget));
}

void
exitCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	exit(0);
}

void
helpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	HelpInfo *help = (HelpInfo *) client_data;
	static String help_app_title;

	if (help_app_title == NULL)
		help_app_title = GetGizmoText(string_appName);

	help->app_title = help_app_title;
	help->title = string_appName;
	help->section = GetGizmoText(help->section);
	PostGizmoHelp(base.shell, help);
}

static	void	InitDevices()
{
static	char	*type = "removable=\"true";
	char	*dev, *attr;
	int	n = 0;

	for (dev = DtamGetDev(type,FIRST); dev; dev = DtamGetDev(type,NEXT)) {
		if (strstr(dev,"display=\"false")) {
			FREE(dev);
			continue;
		}
		attr = DtamDevAttr(dev, CDEVICE);
		if (access(attr, R_OK) == -1) {
			FREE(attr);
			FREE(dev);
			continue;
		}
		dev_menu_item[n+1] = dev_menu_item[n];	/* bump up "Other" */
		attr = DtamDevAlias(dev);
		FREE(dev);
		dev_menu_item[n].sensitive = TRUE;
		dev_menu_item[n].label = attr;
		dev_menu_item[n].mnemonic = attr+strlen(attr)-1;
		if (++n >= MAX_DEVS)
			break;
	}
	SPOOL_VIEW = n;
}

char	*save_command[1];
int	save_count = 1;

main(argc, argv)
	int	argc;
	char	*argv[];
{
extern	char	*getenv();
extern	char	*optarg;
struct	utsname	sys_name;
	Window	win;
	char	*dev, buf[128];
	int	n;

#ifdef	MEMUTIL
	InitializeMemutil();
#endif
        (void)setsid();		/* become a session leader (divorce dtm) */

        /* undo some of the stuff we inherit from dtm */
        sigset(SIGCHLD, SIG_DFL);
        sigset(SIGINT,  SIG_DFL);
        sigset(SIGQUIT, SIG_DFL);
        sigset(SIGTERM, SIG_DFL);

	sprintf(XTERM, "%s/xterm -fn fixed -T ", GetXWINHome("bin"));
	uname(&sys_name);
	our_node = sys_name.nodename;
	our_uid = getuid();
	/*
	 *	owner will setuid(0) to perform custom/installpkg operations
	 *	and revert to this (for linking files, for example) when done
	 */
	owner = _DtamIsOwner(OWN_PACKAGE);
	chdir("/");
	edit_menu_item[0].sensitive =
	edit_menu_item[1].sensitive = owner; 
	if ((desktopdir = getenv("DESKTOPDIR")) == NULL)
		desktopdir = getenv("HOME");

	OlToolkitInitialize(&argc, argv, (XtPointer)NULL);
	w_toplevel = XtInitialize("packager","PackageMgr",NULL,0,&argc,argv);
	DtInitialize(w_toplevel);
	InitializeGizmos(PROGRAM_NAME, PROGRAM_NAME);
	_DtamWMProtocols(w_toplevel);

	while ((n = getopt(argc, argv, "D:")) != EOF) {
		switch (n) {
		case 'D':	if (dev = DtamGetDev(optarg, FIRST)) {
					curalias = DtamDevAttr(dev, ALIAS);
					curlabel = DtamDevAlias(dev);
					FREE(dev);
					media_context = FIRST;
				}
				break;
		}
	}
	sysalias = GetGizmoText(label_system);
	spooled  = GetGizmoText(label_spooled);
	if (!curalias)
		curalias = sysalias;
	lastalias = curalias;
	theScreen = XtScreen(w_toplevel);
	theDisplay = XtDisplay(w_toplevel);
	def_font = _OlGetDefaultFont(w_toplevel, OlDefaultFont);
	x3mm = OlMMToPixel(OL_HORIZONTAL,4);
	y3mm = OlMMToPixel(OL_VERTICAL,4);
	InitDevices();
	base.icon_name = GetGizmoText(base.icon_name);
	CreateGizmo(w_toplevel, BaseWindowGizmoClass, &base, NULL, 0);
	if (curalias == sysalias)
		sprintf(buf, GetGizmoText(string_instTitle),
					curlabel = GetGizmoText(label_all));
	else
		sprintf(buf, GetGizmoText(string_uninstTitle), curlabel);
	SetBaseWindowTitle(&base, buf);
	XtSetArg(arg[0], XtNwidth,  WIDTH);
	XtSetArg(arg[1], XtNheight, HEIGHT);
	XtSetValues(base.scroller, arg, 2);
	XtRealizeWidget(base.shell);
	win = DtSetAppId(theDisplay, XtWindow(base.shell), "dtpkg");
	if (win != None) {
		XMapWindow(theDisplay, win);
		XRaiseWindow(theDisplay, win);
		XFlush(XtDisplay(base.shell));
		exit(0);
	}
	w_acts = GetMenu(GetSubMenuGizmo(base.menu, 0));
	w_edit = GetMenu(GetSubMenuGizmo(base.menu, 1));
	SetSensitivity();

	CreateGizmo(base.shell, ModalGizmoClass, &info_msg, NULL, 0);
	CreateGizmo(base.shell, ModalGizmoClass, &insert_note, NULL, 0);
	CreateInfoSheet();
	CreateIconSheet();
	CreateFolderPrompt();
/*
 *	create icon box with logins
 */
	pkg_fcrec.glyph  = DmGetPixmap(theScreen, "pkgmgr.glyph");
	set_fcrec.glyph  = DmGetPixmap(theScreen, "pkgset.glyph");
	exec_fcrec.glyph = DmGetPixmap(theScreen, "exec.icon");
	pkg_cntrec.count = 1;

	sys_list = (PkgPtr)MALLOC((sys_max+1)*sizeof(PkgRec));

	MapGizmo(BaseWindowGizmoClass, &base);
	if (curalias != sysalias)
		GetMedia(NULL, NULL, NULL);
	else
		FindInstalled();
	XtAddEventHandler(base.shell, (EventMask)NoEventMask, True,
			PropertyEventHandler, (XtPointer)NULL);
	XtMainLoop();
}

static	void
PkgSize(char *str, PkgPtr set, PkgPtr pkg)
{
	char	*ptr;
	int	n = atoi(str);

	pkg->pkg_size = STRDUP(str);
	if (set == NULL) {
		for (set = sys_list; set < sys_list + sys_count; set++)
			if (strcmp(set->pkg_name, pkg->pkg_set) == 0)
				break;
		if (set == sys_list + sys_count)
			return;
	}
	if (set->pkg_size == NULL)
		set->pkg_size = CALLOC(1,16);
	n += atoi(set->pkg_size);
	sprintf(set->pkg_size, "%d", n);
}

static	void
ExpandSet(PkgPtr p)
{
	PkgPtr	q;
	FILE	*fp;
	char	*ptr;

	sprintf(buf, "%s/%s/setinfo", PKGDIR, p->pkg_name);
	fp = fopen(buf, "r");
	while (fgets(buf, BUFSIZ, fp)) {
		buf[strlen(buf)-1] = '\0';
		if (*buf && *buf != '#') {
			ptr = strtok(buf, " \t");
			for (q = sys_list; q < sys_list + sys_count; q++) {
				if (strcmp(ptr, q->pkg_name) == 0) {
					q->pkg_set = STRDUP(p->pkg_name);
					ptr = strtok(NULL, " \t");
					ptr = strtok(NULL, " \t");
					ptr = strtok(NULL, " \t");
					q->pkg_cat = STRDUP(ptr);
					ptr += strlen(ptr)+1;
					q->pkg_desc = STRDUP(ptr);
					break;
				}
			}
		}
	}
	fclose(fp);
	/*
	 *	we should go through and get set descrition and other
	 *	property fields by reading /var/sadm/pkg/<set>/pkginfo
	 */
}

static	void
ExpandCustomSet(PkgPtr p)
{
	PkgPtr	q;
	FILE	*fp;
	Boolean	show_it = FALSE;
	char	*ptr, *ptr2;

	sprintf(buf, "%s/%s", PERMS, p->pkg_name);
	if (fp=fopen(buf, "r")) {
		while (fgets(buf, BUFSIZ, fp)) {
			ptr2 = buf+strlen(buf)-1;
			*ptr2 = '\0';
			if (strncmp(buf, "#set=", 5)==0) {
				ptr = buf+5;
				if (*ptr == '"')
					ptr++;
				if (*--ptr2 == '"')
					*ptr2 = '\0';
				p->pkg_desc = STRDUP(ptr);
			}
			else if (strncmp(buf,"#!",2)==0) {
				if (strncmp(buf+2,"ALL",3) == 0
				&&  isspace(buf[5]))
					continue;
			/*
			 *	else it's a package in the custom set
			 */
				q = InitPkg(&sys_list, &sys_max, sys_count++);
				q->pkg_fmt = STRDUP("Custom");
				q->pkg_cat = STRDUP(APPL);
				q->pkg_set = STRDUP(p->pkg_name);
				for (ptr=buf+2; !isspace(*ptr); ++ptr)
					;
				*ptr = '\0';
				q->pkg_name = STRDUP(buf+2);
				while (isspace(*++ptr))
					;
				for (ptr2 = ptr; isdigit(*ptr2); ptr2++)
					;
				if (ptr2 > ptr) {
					*ptr2++ = '\0';
					PkgSize(ptr, p, q);
				}
				while (isspace(*ptr2))
					ptr2++;
				if (*ptr2)
					q->pkg_desc = STRDUP(ptr2);
				if (!PkgInstalled(q))
					q->pkg_opflag = 'D';
			}
		}
		fclose(fp);
		for (q=sys_list+sys_count-1;strcmp(q->pkg_name,p->pkg_name);q--) {
			if (q->pkg_set && strcmp(q->pkg_set,p->pkg_name)==0) {
				if (q->pkg_opflag != 'D') {
					show_it = TRUE;
					break;
				}
			}
		}
		if (!show_it)
			p->pkg_opflag = 'D';	/* suppress it */
	}
}

static	void
FindInstalled(void)
{
struct	dirent	*dent;
	DIR	*dirp;
	PkgPtr	p;

	if (sys_count)
		FreePkgList(sys_list, &sys_count);
	if (dirp = opendir(PKGDIR)) {
		while (dent = readdir(dirp)) {
			if (strcmp(dent->d_name,".") == 0
			||  strcmp(dent->d_name,"..") == 0)
				continue;
			sprintf(buf, "%s/%s/pkginfo", PKGDIR, dent->d_name);
			if (access(buf,R_OK)!=0)
				continue;
			p = InitPkg(&sys_list, &sys_max, sys_count++);
			p->pkg_name = STRDUP(dent->d_name);
			p->pkg_fmt  = STRDUP("4.0");
			sprintf(buf, "%s/%s/setinfo", PKGDIR,dent->d_name);
			if (access(buf,R_OK)==0)
				p->pkg_cat  = STRDUP("set");
		}
		closedir(dirp);
		/*
		 *	for sets, locate the component packages
		 */
		for (p = sys_list; p < sys_list+sys_count; p++)
			if (strcmp(p->pkg_cat,"set") == 0)
				ExpandSet(p);
	}
	/*
	 *	look for Xenix (custom) pacakges in /etc/perms
	 */
	if (dirp = opendir(PERMS)) {
		while (dent = readdir(dirp)) {
			if (strcmp(dent->d_name,".") == 0
			||  strcmp(dent->d_name,"..") == 0
			||  strcmp(dent->d_name,"inst") == 0
			||  strcmp(dent->d_name,"bundle") == 0)
				continue;
			p = InitPkg(&sys_list, &sys_max, sys_count++);
			p->pkg_name = STRDUP(dent->d_name);
			p->pkg_fmt  = STRDUP("Custom");
			p->pkg_cat  = STRDUP("set");
			ExpandCustomSet(p);
		}
		closedir(dirp);
	}
	GetPkgBox(NULL, NULL);
}

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:userad/LoginMgr.c	1.7.2.45"
#endif
/*
 *	LoginMgr - administer user accounts and groups, including "owner"
 */
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <libgen.h>
#include <pwd.h>
#include <grp.h>
#include <shadow.h>
#include <limits.h>
#include <errno.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Xlib.h>
#include <X11/Vendor.h>

#include <Xol/OpenLookP.h>
#include <Xol/Dynamic.h>
#include <Xol/MenuShell.h>
#include <Xol/PopupWindo.h>
#include <Xol/BaseWindow.h>
#include <Xol/ControlAre.h>
#include <Xol/AbbrevButt.h>
#include <Xol/FButtons.h>
#include <Xol/FList.h>
#include <Xol/StaticText.h>
#include <Xol/TextField.h>
#include <Xol/Caption.h>
#include <Xol/FooterPane.h>
#include <Xol/Notice.h>
#include <Xol/ScrolledWi.h>

#include <Dt/Desktop.h>
#include <libDtI/DtI.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/ModalGizmo.h>
#include <Gizmo/SpaceGizmo.h>

#define ApplicationClass "LoginMgr"
#define ProgramName	"LoginMgr"
#define	HELP_PATH	"dtadmin/LoginMgr.hlp"
#define	ICON_NAME	"user48.glyph"
#define EOS		'\0'
#define WHITESPACE	" \t\n"
#define OWNER		"owner"
#define GROUPHEIGHT	(XtArgVal)4
#define LOWEST_USER_UID	100

#include "../dtamlib/dtamlib.h"
#include "login_msgs.h"

extern	char	*getenv();
extern	char	*GetXWINHome();
extern	struct	passwd	*getpwent();
extern	struct	group	*getgrent(), *getgrnam(), *getgrgid();

void	permCB();
void	propCB();
void	addCB();
void	deleteCB();
void	exitCB();
void	helpCB();
void	applyCB();
void	resetCB();
void	cancelCB();
void	yesCB();
void	noCB();
void	OKCB();
void	applyPermCB();
void	resetPermCB();
void	cancelPermCB();
void	SetViewCB();
void	UnselExtraCB();
void	hideExtraCB();

void	DisplayPrompt();
void	ErrorNotice();
void	CheckPermList();
Boolean	FileCheck();
Boolean	AddLoginEntry();
int	DeleteLoginEntry();
Boolean	InitPasswd();
Boolean removeWhitespace();
void	ResetIconBox();
Widget	GetIconBox();
void	SetPopupValues();
void	SetGroupValues();
void	Reselect();
void	reinitPopup();
void	busyCursor();
void	standardCursor();
int	cmpuid();
int	cmplogin();
int	cmpgroup();

Widget	w_toplevel, w_iconbox, w_baseshell;
Widget	w_popup, w_gpopup, w_perm;
Widget	w_login, w_desc, w_home, w_group, w_glist = NULL, w_uid, w_shell;
Widget	w_remote, w_gname, w_gid, w_own, w_checks;
Widget	w_dtm, w_gui, w_extra;
Widget  w_extctls[2];

Screen		*theScreen;
Display		*theDisplay;
XFontStruct	*def_font;

typedef enum _backupStatus
{ NoErrs, NoAttempt, FindErrs, CpioErrs } backupStatus;
backupStatus backupFiles();

typedef enum _action_menu_index 
{ action_perm, action_exit } action_menu_index; 

static MenuItems action_menu_item[] = {
	{ TRUE, label_perm,  mnemonic_perm, 0, permCB},
	{ TRUE, label_exit,  mnemonic_exit, 0, exitCB},
	{ NULL }
};

static MenuItems edit_menu_item[] = {
	{ TRUE, label_new,   mnemonic_new, 0, addCB},
	{ TRUE, label_delete,mnemonic_delete, 0, deleteCB},
	{ TRUE, label_prop,  mnemonic_prop, 0, propCB},
	{ NULL }
};

static MenuItems view_menu_item[] = {
	{ TRUE, label_users,   mnemonic_users, 0, SetViewCB},
	{ TRUE, label_groups,  mnemonic_groups, 0, SetViewCB},
	{ TRUE, label_sysaccts,mnemonic_sysaccts, 0, SetViewCB},
	{ NULL }
};

static HelpInfo HelpIntro	= { 0, "", HELP_PATH, help_intro };
static HelpInfo HelpProps	= { 0, "", HELP_PATH, help_props };
static HelpInfo HelpGroups	= { 0, "", HELP_PATH, help_groups};
static HelpInfo HelpPerms	= { 0, "", HELP_PATH, help_perms };
static HelpInfo HelpDesk	= { 0, "", HELP_PATH, "HelpDesk"};
static HelpInfo HelpTOC		= { 0, "", HELP_PATH, "TOC" };

static MenuItems help_menu_item[] = {  
	{ TRUE, label_intro, mnemonic_intro,  0, helpCB, (char *)&HelpIntro },
	{ TRUE, label_toc,   mnemonic_toc,    0, helpCB, (char *)&HelpTOC },
	{ TRUE, label_hlpdsk,mnemonic_hlpdsk, 0, helpCB, (char *)&HelpDesk },
	{ NULL }
};

static MenuGizmo action_menu = {0, "action_menu", NULL, action_menu_item};
static MenuGizmo edit_menu   = {0, "edit_menu",   NULL, edit_menu_item};
static MenuGizmo view_menu   = {0, "view_menu",   NULL, view_menu_item};
static MenuGizmo help_menu   = {0, "help_menu",   NULL, help_menu_item};

static MenuItems main_menu_item[] = {
	{ TRUE, label_action, mnemonic_action, (Gizmo) &action_menu},
	{ TRUE, label_view,   mnemonic_view, (Gizmo) &view_menu},
	{ TRUE, label_edit,   mnemonic_edit, (Gizmo) &edit_menu},
	{ TRUE, label_help,   mnemonic_help, (Gizmo) &help_menu},
	{ NULL }
};
static MenuGizmo menu_bar = {
    0, "menu_bar", NULL, main_menu_item, NULL, NULL, CMD,
    OL_FIXEDROWS, 1, OL_NO_ITEM }; 

BaseWindowGizmo base = {0, "base", string_userBaseLine, (Gizmo)&menu_bar,
	NULL, 0, string_iconName, ICON_NAME, " ", " ", 90 };

static MenuItems prop_menu_item[] = {  
	{ TRUE, label_apply,  mnemonic_apply, 0, applyCB,    NULL },
	{ TRUE, label_reset,  mnemonic_reset, 0, resetCB,    NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, cancelCB,   NULL },
	{ TRUE, label_help,   mnemonic_help, 0, helpCB, (XtPointer)&HelpProps },
	{ NULL }
};
static MenuGizmo prop_menu = {0, "properties", NULL, prop_menu_item };
static PopupGizmo prop_popup = {0,"popup",string_propLine,(Gizmo)&prop_menu };

static MenuItems group_menu_item[] = {  
	{ TRUE, label_apply,  mnemonic_apply, 0, applyCB,    NULL },
	{ TRUE, label_reset,  mnemonic_reset, 0, resetCB,    NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, cancelCB,   NULL },
	{ TRUE, label_help,   mnemonic_help, 0, helpCB, (XtPointer)&HelpGroups },
	{ NULL }
};
static MenuGizmo group_menu = {0, "properties", NULL, group_menu_item };
static PopupGizmo group_popup = {0,"popup",string_groupLine,(Gizmo)&group_menu};

static MenuItems perm_menu_item[] = {  
	{ TRUE, label_apply,  mnemonic_apply, 0, applyPermCB,    NULL },
	{ TRUE, label_reset,  mnemonic_reset, 0, resetPermCB,    NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, cancelPermCB,   NULL },
	{ TRUE, label_help,   mnemonic_help, 0, helpCB, (XtPointer)&HelpPerms },
	{ NULL }
};
static MenuGizmo perm_menu = {0, "privileges", NULL, perm_menu_item };
static PopupGizmo perm_popup = {0,"popup",string_permLine,(Gizmo)&perm_menu };

static MenuItems confirm_item[] = {
	{ TRUE, label_yes, mnemonic_yes, 0, yesCB },
	{ TRUE, label_no,  mnemonic_no, 0, noCB },
	{ TRUE, label_help,mnemonic_help, 0, helpCB, (XtPointer)&HelpTOC },
	{ NULL }
};
static	MenuGizmo confirm_menu = {0, "note", "note", confirm_item };
static	ModalGizmo confirm = {0, "warn", string_confLine, (Gizmo)&confirm_menu};

static MenuItems condelete_item[] = {
	{ TRUE, label_yes, mnemonic_yes, 0, yesCB },
	{ TRUE, label_no,  mnemonic_no, 0, noCB },
	{ TRUE, label_help,mnemonic_help, 0, helpCB, (XtPointer)&HelpTOC },
	{ NULL }
};
static MenuItems condelgrp_item[] = {
	{ TRUE, label_yes, mnemonic_yes, 0, yesCB },
	{ TRUE, label_no,  mnemonic_no, 0, noCB },
	{ TRUE, label_help,mnemonic_help, 0, helpCB, (XtPointer)&HelpTOC },
	{ NULL }
};
static	SpaceGizmo vspace = {1, 1};
static  GizmoRec conArray[] = {{ SpaceGizmoClass, &vspace}};
static	MenuGizmo condelete_menu = {0, "note", "note", condelete_item };
static	MenuGizmo condelgrp_menu = {0, "note", "note", condelgrp_item };
static	ModalGizmo condelete=
{0,"warn",string_confLine,(Gizmo)&condelete_menu, NULL, conArray, 1 };
static	ModalGizmo condelgrp ={0,"warn",string_confLine,(Gizmo)&condelgrp_menu};

static MenuItems errnote_item[] = {
	{ TRUE, label_OK,  mnemonic_OK, 0, OKCB },
/*	{ TRUE, label_help,mnemonic_help, 0, helpCB, (XtPointer)&HelpTOC },
*/
	{ NULL }
};
static	MenuGizmo errnote_menu = {0, "note", "note", errnote_item };
static	ModalGizmo errnote = {0, "warn", string_errLine, (Gizmo)&errnote_menu };


#define	FooterMsg(txt)	SetBaseWindowMessage(&base,txt)

typedef	struct	{ char	*	label;
		  XtArgVal	mnem;
		  Boolean	sensitive;
		  XtArgVal	selCB;
		  XtArgVal      subMenu;
} Items;

#define	N_FIELDS	5

char    *Fields[]   = { XtNlabel, XtNmnemonic, XtNsensitive, XtNselectProc,
			XtNpopupMenu };

#define	GGT	GetGizmoText
#define	SET_BTN(id,n,name)	id[n].label = GGT(label##_##name);\
				id[n].mnem = (XtArgVal)*GGT(mnemonic##_##name);\
				id[n].sensitive = TRUE;\
				id[n].selCB = (XtArgVal)name##CB

typedef	struct	{ char	*	label;
		  XtArgVal	mnem;
		  Boolean	setting;
} ExclItem;

char    *ExclFields[]   = { XtNlabel, XtNmnemonic, XtNset };

#define SET_EXCL(id,n,name,st)	id[n].label = GGT(label##_##name);\
                               	id[n].mnem = (XtArgVal)*GGT(mnemonic##_##name);\
				id[n].setting = st;

typedef	struct	{
	char	*label;
	Boolean	set;
} FListItem, *FListPtr;

FListPtr	GroupItems = (FListPtr)0;

char	*ListFields[] = { XtNlabel, XtNset };

typedef	struct	passwd	UserRec, *UserPtr;

UserPtr	u_list = (UserPtr)0;
UserPtr	u_reset;
int	u_cnt = 0;
int	uid_cnt = 0;
int	*uid_list;
int	owner_set=0;

typedef	struct	_grec {
		char	*g_name;
		gid_t	g_gid;
} GroupRec, *GroupPtr;

GroupPtr	g_list = (GroupPtr)0;
GroupPtr	g_reset;
int		g_cnt = 0;
int		max_gid = 0;

int		item_count = 0;

DmItemPtr	u_itp,		g_itp;
DmFclassRec	u_fcrec,	g_fcrec;
DmContainerRec	u_cntrec,	g_cntrec;

typedef	struct	{
	char	*label;
	char	*cmds;
	char	*help;
	Boolean	granted;
} PermRec, *PermPtr;

PermPtr	p_list = (PermPtr)0;
int	p_cnt = 0;

Dimension	xinch, yinch;

Boolean		I_am_owner, this_is_owner;

#define	MOTIF_DTM	-1
#define	NO_DTM		0
#define	OL_DTM		1

Boolean	dtm_account = True;
int	dtm_style = MOTIF_DTM;

#define	USERS	0
#define	GROUPS	1
#define	SYSLOG	2

int	view_type = USERS;

#define	KSH		"/usr/bin/ksh"
#define	BSH		"/sbin/sh"

static	char	*HOME_DIR	= "/home/";

static	char	*PRIV_TABLE	= "desktop/LoginMgr/PrivTable";
static	char	*PERM_FILE	= "desktop/LoginMgr/Users";
static	char	*ADMINUSER	= "/usr/bin/adminuser ";
static	char	*MAKE_OWNER	= "adm/make-owner";

static	char	*ADD_USER	= "/usr/sbin/useradd";
static	char	*DEL_USER	= "/usr/sbin/userdel";
static	char	*MOD_USER	= "/usr/sbin/usermod";
static	char	*ADD_GROUP	= "/usr/sbin/groupadd";
static	char	*DEL_GROUP	= "/usr/sbin/groupdel";
static	char	*MOD_GROUP	= "/usr/sbin/groupmod";
static	char	*AGE_UID	= "/etc/security/ia/ageduid";

static	char	*ADD_DTUSER	= "adm/dtadduser";
static	char	*DEL_DTUSER	= "adm/dtdeluser";
static	char	*CHG_DTVAR	= "adm/olsetvar";
static	char	*UNSETVAR	= "adm/olunsetvar";

#define	LEAVE_HOME	0
#define	DEL_HOME	1
#define	BKUP_HOME	2
#define	XFER_HOME	4

int	home_flag = DEL_HOME;

#define WIDTH	(6*xinch)
#define	HEIGHT	(3*yinch)

#define INIT_X  35
#define INIT_Y  20
#define INC_X   70
#define INC_Y   70
#define	MARGIN	20

Dimension	ibx = INIT_X, iby = INIT_Y;

char	*fmt, *context, *operation;
char	*login, *desc, *home, *group, *remote, *shell, *uid, *gname, *gid;
int 	sethome = 1;

#define	P_ADD	1
#define	P_CHG	2
#define	P_DEL	3
#define	P_OWN	4
#define	P_DEL2	5

int	u_pending = 0;	    /* user/system view operation in progress */
int	g_pending = 0;      /* group view operation in progress */
int	exit_code;

/*
 *	many of the exit codes from useradd/mod/del and groupadd/mod/del are
 *	"impossible" because of prior validation of their inputs; these and
 *	any others that would be more mystifying than helpful to most users
 *	are reported with a "generic" error message.
 */
static	void
DiagnoseExit(char *op, char *type, char *name, int popup_type)
{
	char	buf[128];

	if (exit_code == 0)
		return;
	exit_code /= 256;
	*buf = '\0';
	if (popup_type == GROUPS) {
		switch (exit_code) {
		case 9:
			sprintf(buf,GetGizmoText(string_dupl),name,type);
			break;
		case 10:
			sprintf(buf,GetGizmoText(string_noPerms),op,type);
			break;
		}
	}
	else {
		switch (exit_code) {
		case 8: if (u_pending != P_ADD) {
				sprintf(buf,GetGizmoText(string_inUse),
								op,type,name);
				break;
			}
			/* in the P_ADD case, this appears to be a symptom
			 * of permissions problems -- try to second guess.
			 */
		case 6:
			sprintf(buf,GetGizmoText(string_noPerms),op,type);
			break;
		case 9:
			sprintf(buf,GetGizmoText(string_dupl),name,type);
			break;
		case 10:/*
			 *	shouldn't happen -- refers to error in groups
			 *	file, whereas LoginMgr doesn't use the -G flag
			 */
			break;
		case 11:
		case 12:
			sprintf(buf,GetGizmoText(string_badDir),op,type,name);
			break;
		}
	}
	if (*buf == '\0')
		sprintf(buf, GetGizmoText(string_unknown), op, type, name);
	ErrorNotice(buf, popup_type);
}

static void
setContext(int popup_type)
{
    switch (popup_type)
    {
    case GROUPS:
	context = GetGizmoText(tag_group);
	break;
    case USERS:
	context = GetGizmoText(tag_login);
	break;
    case SYSLOG:
	context = GetGizmoText(tag_sys);
	break;
    default:
	break;
    }	    
    return;
}
void
moveFocus(Widget wid)
{
    Time time;
    
    time = CurrentTime;
    if (OlCanAcceptFocus(wid, time))
	XtCallAcceptFocus(wid, &time);
}

void
resetFocus(int popup_type)
{
    Arg arg[5];

    switch (popup_type)
    {
    case GROUPS:
	XtSetArg(arg[0], XtNfocusWidget, (XtArgVal)w_gname);
	XtSetValues(w_gpopup, arg, 1);
	break;
    case USERS:
	/* FALL THRU */
    case SYSLOG:
	XtSetArg(arg[0], XtNfocusWidget, (XtArgVal)w_login);
	XtSetValues(w_popup, arg, 1);
	break;
    }
    return;
}

static	void
FreeUserList(void)
{
	UserPtr	up;
	char	*p;

	free (uid_list);
	while (u_cnt--) {
		up = &u_list[u_cnt];
		if (p = up->pw_name)	free(p);
		if (p = up->pw_comment)	free(p);
		if (p = up->pw_dir)	free(p);
		if (p = up->pw_shell)	free(p);
	}
	free (u_list);
	u_list = (UserPtr)0;
	u_cnt = 0;
}

#define	BUNCH	16
#define	U_QUANT	(BUNCH*sizeof(UserRec))
#define	G_QUANT	(BUNCH*sizeof(GroupRec))

static	Boolean
MakeUserList(void)
{
static  time_t   lastListTime = 0;
struct  stat	 pwStat;
struct	passwd	*pwd;
	FILE	*fp;
	UserPtr	up;
	char	buf[40];
	int	n, ret;

        while ((ret = stat("/etc/passwd", &pwStat)) != 0 && errno == EINTR)
	       ;		/* try again */
        if (ret != 0)
	{
	    if (u_list)
		return False;
            else
		exit(1);
	}

        if (lastListTime >= pwStat.st_mtime)
	    return False;
        else
	    lastListTime = pwStat.st_mtime;
	if (u_list)
	        FreeUserList();
	while (pwd = getpwent()) {
		if (pwd->pw_uid > UID_MAX)
			continue;
		if (u_cnt == 0) {
			u_list = (UserPtr)malloc(U_QUANT);
		}
		else if (u_cnt % BUNCH == 0) {
			u_list = (UserPtr)realloc((void *)u_list,
						(1+(u_cnt/BUNCH))*U_QUANT);
		}
		up = &u_list[u_cnt++];
		up->pw_name = strdup(pwd->pw_name);
		up->pw_uid = pwd->pw_uid;
		up->pw_gid = pwd->pw_gid;
		up->pw_comment = pwd->pw_comment? strdup(pwd->pw_comment): NULL;
		up->pw_dir = pwd->pw_dir? strdup(pwd->pw_dir): NULL;
		up->pw_shell = pwd->pw_shell? strdup(pwd->pw_shell): NULL;
	}
	endpwent();
	if (uid_list = (int *)malloc(u_cnt*sizeof(int))) {
		for (n = 0; n < u_cnt; n++)
			uid_list[n] = u_list[n].pw_uid;
		/*
		 *	attach ageing uids to the list, so they won't
		 *	be chosen by default (this still requires a
		 *	a test in Validate(), as the user my try override
		 */
		if (fp=fopen(AGE_UID,"r")) {
			while (fgets(buf, 40, fp)) {
				uid_list = (int *)realloc(uid_list,
							sizeof(int)*(n+1));
				uid_list[n++] = atoi(buf);
			}
			fclose(fp);
		}
		uid_cnt = n;
		qsort((void *)uid_list, uid_cnt, sizeof(int), cmpuid);
	}
	qsort((void *)u_list, u_cnt, sizeof(UserRec), cmplogin);

        return True;
}

static	int
NextUid()
{
    register  int	n;

    if (MakeUserList())
	ResetIconBox();
    for (n = 1; n < uid_cnt; n++)
	if (uid_list[n]-uid_list[n-1] > 1)
	    if (view_type == SYSLOG || uid_list[n-1] >= LOWEST_USER_UID - 1)
		break;
    return uid_list[n-1]+1;
}

static	void
FreeGroupList()
{
	char	*p;

/*	free(GroupItems);			-- this causes core dump; why?
/*	GroupItems = (FListPtr)NULL;		-- I'm going to allow the leak.
*/	while (g_cnt--) {
		if (p = g_list[g_cnt].g_name)	free(p);
	}
	free (g_list);
	g_list = (GroupPtr)0;
	g_cnt = 0;
}

static	void
MakeGroupList(void)
{
struct	group	*gp;
	char	*str;
	int	n;

	if (g_list)
		FreeGroupList();
	max_gid = 0;
	while (gp = getgrent()) {
		if (gp->gr_gid > UID_MAX)
			continue;
		if (g_cnt == 0)
			g_list = (GroupPtr)malloc(G_QUANT);
		else if (g_cnt % BUNCH == 0)
			g_list = (GroupPtr)realloc((void *)g_list,
						(1+(g_cnt/BUNCH))*G_QUANT);
		g_list[g_cnt].g_name = strdup(gp->gr_name);
		if ((g_list[g_cnt].g_gid  = gp->gr_gid) > max_gid)
			if (gp->gr_gid < UID_MAX-2)
			/*
			 * special case to filter out nobody,noaccess
			 */
				max_gid = gp->gr_gid;
		g_cnt++;
	}
	endgrent();
	qsort((void *)g_list, g_cnt, sizeof(GroupRec), cmpgroup);
	if (GroupItems = (FListPtr)malloc(g_cnt*sizeof(FListItem)))
		for (n = 0; n < g_cnt; n++) {
			GroupItems[n].label = g_list[n].g_name;
			GroupItems[n].set = FALSE;
		}
	if (w_glist) {
		XtVaSetValues(w_glist,	XtNitems,	GroupItems,
					XtNnumItems,	g_cnt,
			                XtNviewHeight,	GROUPHEIGHT,
				NULL);
	}
}

static	int
ChangeDTVars(char * node, Boolean dtm_flag, int style_flag)	/* 0 => no change, 1 => ok, -1 => bad */
{
	char	buf[BUFSIZ];
	Boolean	node_change = FALSE;

	*buf = EOS;

	if (remote==NULL || node==NULL)
		node_change = (remote != node);
	else
		node_change = strcmp(node,remote);

	if (dtm_flag != dtm_account) {
	/*
	 *	add or delete desktop environment
	 */
		if (dtm_flag == False) {
			operation = GetGizmoText(tag_delOp);
			sprintf(buf, "%s %s", GetXWINHome(DEL_DTUSER), login);
		}
		else {
			operation = GetGizmoText(tag_addOp);
			strcpy(buf, GetXWINHome(ADD_DTUSER));
			if (style_flag == MOTIF_DTM)
				strcat(buf, " -m");
			if (node)
				strcat(strcat(buf," -r "),node);
			strcat(strcat(buf, " "), login);
		}
	}
	else {
		operation = GetGizmoText(tag_chgOp);
		*buf = EOS;
		if (dtm_flag && dtm_style != style_flag) {
			sprintf(buf, " %s XGUI %s %s ", GetXWINHome(CHG_DTVAR),
				style_flag==OL_DTM? "OPEN_LOOK": "MOTIF", login);
		}
		if (node_change) {
			if (*buf) {
				buf[0] = '(';
				buf[strlen(buf)-1] = ';';
			}
			if (node)
				sprintf(buf+strlen(buf), "%s REMOTE %s %s",
					GetXWINHome(CHG_DTVAR), node, login);
			else 
				sprintf(buf+strlen(buf), "%s REMOTE %s",
					GetXWINHome(UNSETVAR), login);
			if (*buf == '(')
				strcat(buf,")");
		}
	}
	if (*buf == EOS)	/* nothing to do */
	    return 1;
	exit_code = system(buf);
	return (exit_code == 0? 1: -1);
}


static	UserPtr
SelectedUser()
{
	int		last, count;
	DmObjectPtr	optr;

	XtVaGetValues(w_iconbox, XtNlastSelectItem, &last,
				 XtNselectCount,    &count,
				 NULL);
	if (count == 0)
		return (UserPtr)NULL;
	else {
		OlVaFlatGetValues(w_iconbox, last, XtNobjectData, &optr, NULL);
		return (UserPtr)optr->objectdata;
	}
}


static	void
noCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    int pending = (view_type == GROUPS) ? g_pending : u_pending;
    Arg arg[5];

    FooterMsg(NULL);
    if (pending == P_OWN)
	resetPermCB(wid, NULL, NULL);
    else if (pending == P_DEL2)
    {
	XtPopdown(confirm.shell);
	return;
    }

    if (pending != P_DEL)
    {
	if (client_data == w_gpopup)
	    standardCursor(GROUPS);
	else 
	    if (client_data == w_popup) {
		standardCursor(USERS);
		if (sethome == 0)
		{
		    sethome = 1;
		    home = HOME_DIR;
		    XtSetArg(arg[0], XtNstring, (XtArgVal)home);
		    XtSetValues(w_home, arg, 1);
 		}
 	    }
	BringDownPopup(confirm.shell);
    }
    else if (view_type == GROUPS)
	BringDownPopup(condelgrp.shell);
    else
	BringDownPopup(condelete.shell);
}

static void
no2CB(Widget  wid, XtPointer client_data, XtPointer call_data)
{
    home_flag ^= DEL_HOME;
    yesCB(wid, client_data, call_data);
}

static	void
yesCB(Widget  wid, XtPointer client_data, XtPointer call_data)
{
    PopupGizmo *this_popup;
    Boolean	op_OK, flag;
    Boolean	update_iconbox = False;
    Boolean	dtm_flag;
    int		style_flag;
    int		result = 1, v_result = 0;
    char       *name, *node;
    char	buf[BUFSIZ];
    int		popup_type;
    int		pending;
    backupStatus status;

    if (client_data == NULL)
	popup_type = view_type;
    else
    {
	if ((Widget)client_data == w_gpopup)
	    popup_type = GROUPS;
	else
	    popup_type = (atoi(uid) < LOWEST_USER_UID) ? SYSLOG : USERS;
    }

    setContext(popup_type);
    pending = (popup_type == GROUPS) ? g_pending : u_pending;

    noCB(wid, NULL, call_data); /*to do popdown */
    if (pending == P_OWN) {
	name = login;
	this_popup = &perm_popup;
    }
    else if (popup_type == GROUPS)
    {
	name = gname;
	this_popup = &group_popup;
    }
    else {
	name = login;
	this_popup = &prop_popup;
	XtVaGetValues(w_remote, XtNstring, &node, NULL);
	if (!*node)
	    node = NULL;
	OlVaFlatGetValues(w_dtm, 0, XtNset, &dtm_flag, NULL);
	if (dtm_flag)
	{
	    dtm_flag = True;
	    OlVaFlatGetValues(w_gui, 0, XtNset, &flag, NULL);
	    style_flag = flag? MOTIF_DTM: OL_DTM;
	}
    }
    switch (pending) {

    case P_OWN:
	sprintf(buf,"%s %s %s",GetXWINHome(MAKE_OWNER),owner_set?"":"-",name);
	op_OK = (system(buf) == 0);
	exit_code = 0;		/* don't diagnose specific failures */
	sprintf(buf, GetGizmoText(format_permFmt), name, 
		op_OK ? GetGizmoText(tag_good) : GetGizmoText(tag_bad));
	FooterMsg(buf);
	CheckPermList(SelectedUser());
	resetPermCB(w_own, NULL, NULL);
	break;
    case P_DEL:
	if (popup_type == GROUPS) {
	    sprintf(buf, "%s %s", DEL_GROUP, gname);
	    exit_code = system(buf);
	    op_OK = (exit_code == 0);
	    break;
	}
	else {
	    status = backupFiles(&home_flag);
	    if (WIFEXITED(status))
		status = (backupStatus)WEXITSTATUS(status);
	    else
		status = CpioErrs;
	    if (home_flag & DEL_HOME) /* one "&" correct here */
	    {
		char notebuf[BUFSIZ];

		u_pending = P_DEL2;
		switch (status)
		{
		case NoErrs:
		    sprintf(notebuf, GetGizmoText(string_deleteFiles), home);
		    break;
		case NoAttempt:
		    sprintf(notebuf, GetGizmoText(string_notBackedUp),
			    name, home);
		    break;
		case FindErrs:
		    /* FALL THRU */
		case CpioErrs:
		    /* FALL THRU */
		default:
		    sprintf(notebuf, GetGizmoText(string_backupErrs),
			    name, home);
		    break;
		}
		DisplayPrompt(notebuf, w_popup);
		return;
	    }
	}
	/* FALL THRU */
    case P_DEL2:
	pending = u_pending = P_DEL;
	exit_code =  DeleteLoginEntry(home_flag);
	op_OK = (exit_code == 0);
	break;
    case P_ADD:
	if ((Widget)client_data == w_gpopup) {
	    sprintf(buf, "%s -g %s -o %s", ADD_GROUP, gid, gname);
	    exit_code = system(buf);
	    op_OK = (exit_code == 0);
	}
	else {
	    if (op_OK = AddLoginEntry()) {
		update_iconbox = True;
		InitPasswd();
		if (dtm_flag) {
		    operation = GetGizmoText(tag_addOp);
		    context = GetGizmoText(tag_desktop);
		    strcpy(buf, GetXWINHome(ADD_DTUSER));
		    if (style_flag == MOTIF_DTM)
			strcat(buf, " -m");
		    if (node)
			sprintf(buf+strlen(buf),
				" -r %s", node);
		    sprintf(buf+strlen(buf)," %s", login);
		    op_OK = (system(buf) == 0);
		    exit_code = 0;
		}
	    }
	}
	break;
    case P_CHG:
	if (popup_type == GROUPS) {
	    if (atoi(gid) != g_reset->g_gid ||  strcmp(gname,g_reset->g_name))
	    {
		sprintf(buf, "%s -g %s -o ", MOD_GROUP, gid);
		if (strcmp(gname,g_reset->g_name))
		{
		    name = g_reset->g_name;
		    sprintf(buf+strlen(buf),"-n %s ",gname);
		}
		strcat(buf, g_reset->g_name);
		exit_code = system(buf);
		op_OK = (exit_code == 0);
		result = op_OK? 1: -1;
	    }
	}
	else {
	    update_iconbox = op_OK = ((result=ChangeLoginProps())==1);
	    if (result >= 0) {
		v_result = ChangeDTVars(node, dtm_flag, style_flag);
		if (v_result) {
		    op_OK = (v_result == 1);
		    context = GetGizmoText(tag_desktop);
		}
	    }
	}
	break;
    }
    if (result == 0 && v_result == 0) {
	sprintf(buf, GetGizmoText(string_noChange), login);
	SetPopupMessage(this_popup, buf);
    }
    else {
	sprintf(buf, fmt, operation, context, name,
		(op_OK? GetGizmoText(tag_good) : GetGizmoText(tag_bad)));
	if (pending == P_DEL)
	    FooterMsg(buf);
	else if (pending != P_OWN)
	    SetPopupMessage(this_popup, buf);
	if (pending == P_CHG && result == -1 && !I_am_owner) {
	    sprintf(buf, GetGizmoText(string_noPerms), operation,
		    context, name);
	    ErrorNotice(buf, popup_type);
	}
	else if (exit_code)
	    DiagnoseExit(operation, context, name, popup_type);
	if (op_OK || update_iconbox)
	{
	    if (popup_type == GROUPS)
		MakeGroupList();
	    else
		MakeUserList();
	    ResetIconBox();
	    if (pending != P_DEL)
		Reselect((popup_type == GROUPS) ? gname: login);
	    if (pending == P_ADD)
		reinitPopup(popup_type);
            BringDownPopup(this_popup-> shell);
	}
    }
    standardCursor(popup_type);
}

static	void
DisplayPrompt (char *buf, Widget wid)
{
    Arg arg[4];
    
    if (!confirm.shell)
	CreateGizmo(w_baseshell, ModalGizmoClass, &confirm, NULL, 0);

    SetModalGizmoMessage(&confirm, buf);
    OlVaFlatSetValues(confirm_menu.child, 0, XtNclientData,
		      (XtArgVal)wid, 0);
    OlVaFlatSetValues(confirm_menu.child, 1,
		      XtNclientData, (XtArgVal)wid,
		      XtNselectProc, (wid == w_popup && u_pending ==
				      P_DEL2) ? no2CB : noCB, 0);
    MapGizmo(ModalGizmoClass, &confirm);
}

static	void
UnselFileCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	OlFlatCallData	*d = (OlFlatCallData *)call_data;
	
	switch(d->item_index) {
		case 0:	home_flag ^= DEL_HOME;
			break;
		case 1:	home_flag ^= BKUP_HOME;
			break;
	}
}

static	void
SelFileCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	OlFlatCallData	*d = (OlFlatCallData *)call_data;

	switch(d->item_index) {
		case 0:	home_flag |= DEL_HOME;
			break;
		case 1:	home_flag |= BKUP_HOME;
			break;
	}
}

static	void
ConfirmDelete(char *buf)
{
static	ExclItem	FileDisp[3];
static  Widget		w_txt, w_file,w_up;

	if (view_type == GROUPS) {
		if (!condelgrp.shell)
			CreateGizmo(w_baseshell, ModalGizmoClass, &condelgrp,
								NULL, 0);
		SetModalGizmoMessage(&condelgrp, buf);
		MapGizmo(ModalGizmoClass, &condelgrp);
		return;
	}
	if (!condelete.shell) {
		CreateGizmo(w_baseshell, ModalGizmoClass, &condelete, NULL, 0);
		w_up = condelete.control;
		XtVaSetValues(w_up,
				XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
				XtNalignCaptions,	(XtArgVal)FALSE,
				XtNcenter,		(XtArgVal)TRUE,
				XtNvSpace,		(XtArgVal)yinch/4,
				NULL);
		w_txt = XtVaCreateManagedWidget("text",textEditWidgetClass,
				w_up,
				XtNeditType,		OL_TEXT_READ,
				XtNcharsVisible,	(XtArgVal)24,
				XtNlinesVisible,	(XtArgVal)3,
				XtNfont, (XtArgVal)_OlGetDefaultFont(w_toplevel,
							OlDefaultNoticeFont),
				NULL);
		SET_EXCL(FileDisp, 0, remove, TRUE);
		SET_EXCL(FileDisp, 1, bkup, FALSE);
		w_file = XtVaCreateManagedWidget("check",flatButtonsWidgetClass,
				w_up,
				XtNtraversalOn,		(XtArgVal)TRUE,
				XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
				XtNbuttonType,		(XtArgVal)OL_CHECKBOX,
				XtNitemFields,		(XtArgVal)ExclFields,
				XtNnumItemFields,	(XtArgVal)3,
				XtNitems,		(XtArgVal)FileDisp,
				XtNnumItems,		(XtArgVal)2,
				XtNselectProc,		(XtArgVal)SelFileCB,
				XtNunselectProc,	(XtArgVal)UnselFileCB,
				XtNvSpace,		(XtArgVal)yinch/6,
				NULL);
	}
	XtVaSetValues(w_txt, XtNsource, (XtArgVal)buf, NULL);
	FileDisp[0].setting = TRUE;
	FileDisp[1].setting = FALSE;
	XtVaSetValues(w_file,
			XtNitemsTouched,	(XtArgVal)TRUE,
			XtNmappedWhenManaged,	(XtArgVal)(view_type != GROUPS),
			NULL);
	MapGizmo(ModalGizmoClass, &condelete);
}

void
OKCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    int popup_type = (int)client_data;
    Arg arg[5];

    if (popup_type != GROUPS && u_pending != P_DEL)
    {
	if (sethome == 0)
	{
	    sethome = 1;
	    home = HOME_DIR;
	    XtSetArg(arg[0], XtNstring, (XtArgVal)home);
	    XtSetValues(w_home, arg, 1);
	}
    }
    standardCursor(popup_type);
    BringDownPopup(errnote.shell);
}

void
ErrorNotice (char *buf, int popup_type)
{
	if (!errnote.shell)
		CreateGizmo(w_baseshell, ModalGizmoClass, &errnote, NULL, 0);

	SetModalGizmoMessage(&errnote, buf);
	OlVaFlatSetValues(errnote_menu.child, 0, XtNclientData,
		      (XtArgVal)popup_type, 0);
	MapGizmo(ModalGizmoClass, &errnote);
}

static	void
UnselExtraCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    SetPopupMessage(&prop_popup, "");
    XtUnmanageChildren(w_extctls, XtNumber(w_extctls));
}


static	void
SelExtraCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    SetPopupMessage(&prop_popup, "");
    XtManageChildren(w_extctls, XtNumber(w_extctls));
}

static	void
Reselect(itemname)
	char	*itemname;
{
    int	n = item_count;
    char	*label;

    while (n--) {
	OlVaFlatGetValues(w_iconbox, n, XtNlabel, &label, NULL);
	OlVaFlatSetValues(w_iconbox, n, XtNset,
			  strcmp(label,itemname)==0, NULL);
    }
    XtVaSetValues(w_iconbox, XtNitemsTouched, TRUE, NULL);
    if (view_type == GROUPS)
    {
	g_reset = (GroupPtr)SelectedUser();
	SetGroupValues(g_reset);
    }
    else
    {
	u_reset = SelectedUser();
	SetPopupValues(u_reset);
    }
}
static	backupStatus
backupFiles(int *flag)
{
    char buf[BUFSIZ];

    if (*flag & BKUP_HOME)
    {
	if (access(home,R_OK) == -1)
	{
	    *flag = LEAVE_HOME;
	    return NoAttempt;
	}
	/* LoginMgr has more privs than MediaMgr so use tfadmin to */
	/* invoke MediaMgr with only the privs it needs */
	sprintf(buf, "/sbin/tfadmin MediaMgr -B -C %s -L", home);
	return ((backupStatus)system(buf));
    }
    return NoErrs;
}


/*
 *	DeleteLoginEntry invokes /usr/sbin/userdel; flag indicates disposal
 *	of the home directory -- currently it is left alone OR just deleted.
 */
static	int
DeleteLoginEntry(flag)
	int	flag;
{
	char	*ptr, buf[BUFSIZ];

	/* needed so login is removed from mail list for pkgadd */
	sprintf(buf, "if %s -t LoginMgr %s >/dev/null 2>&1 ;then %s - %s;fi", 
		ADMINUSER, login, GetXWINHome(MAKE_OWNER), login);
	system(buf);

	sprintf(buf, "%s -d %s; %s %s%s",  ADMINUSER, login,
		DEL_USER, flag&DEL_HOME? "-r ": "", login); 
	return (system(buf));
}

static	Boolean
AddLoginEntry()
{
	char	buf[BUFSIZ];

	sprintf(buf, "%s -m -c \"%s\" -u %s -o ", ADD_USER, desc, uid);
	if (*home)
		sprintf(buf+strlen(buf), "-d %s ", home);
	if (strcmp(group,"other"))
		sprintf(buf+strlen(buf), "-g %s ", group);
	if (*shell)
		sprintf(buf+strlen(buf), "-s %s ", shell);
	strcat(buf, login);
	exit_code=system(buf);
	return (exit_code==0);
}

/*
 *	the new account should be set up with a password; for now, it is
 *	done through a rather crude invocation of passwd(1) via xterm.
 */
static	char	 *pwdcmd =
	"exec xterm -geometry 32x6 -t 'Password Manager' -e /usr/bin/passwd ";

static	Boolean
InitPasswd(void)
{
	char	buf[128];

	strcat(strcpy(buf, pwdcmd), login);
	return (system(buf)==0);
}

static	int
ChangeLoginProps(void)
{
	Boolean	change = FALSE;
	int	i, n;
	char	buf[BUFSIZ];

	strcpy(buf, MOD_USER);
	if (strcmp(login,u_reset->pw_name)) {
		sprintf(buf+strlen(buf), " -l %s ", login);
		change = TRUE;
	}
	if (strcmp(desc,u_reset->pw_comment)) {
		sprintf(buf+strlen(buf), " -c \"%s\" ", desc);
		change = TRUE;
	}
	if (u_reset->pw_uid != (i=atoi(uid))) {
		sprintf(buf+strlen(buf), " -u %s ", uid);
		for (n = 0; n < u_cnt; n++) {
			if (u_list[n].pw_uid == i) {
				strcat(buf, "-o ");
				break;
			}
		}
		change = TRUE;
	}
	if (*group) {
		struct	group	*gp = getgrgid(u_reset->pw_gid);
		if (gp == NULL || strcmp(group, gp->gr_name)) {
			sprintf(buf+strlen(buf), " -g %s ", group);
			change = TRUE;
		}
	}
	if (strcmp(home,u_reset->pw_dir)) {
		sprintf(buf+strlen(buf), " -d %s -m ", home);
		change = TRUE;
	}
	if (strcmp(shell,u_reset->pw_shell)) {
		sprintf(buf+strlen(buf), " -s %s ", shell);
		change = TRUE;
	}
	if (change && I_am_owner) {
		strcat(buf, u_reset->pw_name);
		exit_code=system(buf);
		return (exit_code==0? 1: -1);
	}
	else if (change) {
		return -1;
	}
	else
		return 0;
}

static	void
ResetIconBox(void)
{
	XtUnmanageChild(w_iconbox);
	XtDestroyWidget(w_iconbox);
	w_iconbox = GetIconBox(base.scroller);
}

char	vld_msg[BUFSIZ];
char	*vfmt = NULL;

static	Boolean
Validate(void)
{
    struct	group	*gp;
    Boolean	valid = TRUE;
    char	*ptr, buf[80], *start, *extra_label = NULL;
    int	gidno, n;

    if (vfmt == NULL)
	vfmt = GetGizmoText(format_syntaxFmt);
    *vld_msg = '\0';

    (void)removeWhitespace(login);
    if (*login == '\0') {
	valid = FALSE;
	sprintf(vld_msg, GetGizmoText(format_noIdFmt),
		GetGizmoText(label_login));
    }
    else if (ptr = strpbrk(login, " :")) {
	valid = FALSE;
	sprintf(vld_msg+strlen(vld_msg), vfmt, *ptr,
		GetGizmoText(label_login), login);
    }
    else if (u_reset == NULL || strcmp(login, u_reset->pw_name))
    {   /* new or modified login name; dis-allow duplicates. */
	int index;
	
	for (index = 0; index < u_cnt; index++)
	    if (strcmp(login, u_list[index].pw_name) == 0)
	    {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), GetGizmoText(format_inUse),
			GetGizmoText(label_login), login);
		break;
	    }
    }
    if (strchr(desc, ':')) {
	valid = FALSE;
	sprintf(vld_msg+strlen(vld_msg), vfmt, ':',
		GetGizmoText(label_desc), desc);
    }
    if (strchr(home,':')) {
	valid = FALSE;
	extra_label = GetGizmoText(label_home);
	(void)removeWhitespace(extra_label);
        sprintf(vld_msg+strlen(vld_msg), vfmt, ':',
		extra_label, home);
    }
    if ((n = strspn(uid,"0123456789")) != strlen(uid)) {
	valid = FALSE;
	extra_label = GetGizmoText(label_uid);
	(void)removeWhitespace(extra_label);
	sprintf(vld_msg+strlen(vld_msg), vfmt, uid[n],
		extra_label, uid);
    }
    else if (atoi(uid) > UID_MAX) { /* defined in <limits.h> */
	valid = FALSE;
	sprintf(vld_msg+strlen(vld_msg), GetGizmoText(format_maxIdFmt),
		GetGizmoText(label_uid), uid, UID_MAX);
    }
    else {			/* check for uid being aged */
	sprintf(buf,"/usr/bin/grep '^%s:' %s >/dev/null 2>&1",uid,AGE_UID);
	if (system(buf) == 0) {
	    valid = FALSE;
	    sprintf(vld_msg+strlen(vld_msg),
		    GetGizmoText(format_ageIdFmt),
		    GetGizmoText(label_uid), uid);
	}
    }
    if (*group) {
	if (gp = getgrnam(group))
	    gidno = gp->gr_gid;
	else {
	    gidno = 1;
	    valid = FALSE;
	    strcat(vld_msg, GetGizmoText(string_badGroup));
	}
    }

    (void)removeWhitespace(shell);
    if (*shell == EOS)		/* shell field is empty */
    {
	valid = FALSE;
	strcat(vld_msg,GetGizmoText(string_shellRequired));
    }
    else if ( FileCheck("", shell, X_OK, atoi(uid), gidno)==FALSE)
    {
	valid = FALSE;
	strcat(strcat(vld_msg,GetGizmoText(string_badShell)), shell);
    }
    return valid;
}

static	Boolean
ValidateGroup(char *name, char *gidno)
{
	Boolean	valid = TRUE;
	char	*ptr;
	int	n;

	if (vfmt == NULL)
		vfmt = GetGizmoText(format_syntaxFmt);
	*vld_msg = '\0';
	if (*name == '\0') {
		valid = FALSE;
		sprintf(vld_msg, GetGizmoText(format_noIdFmt),
				 GetGizmoText(label_gname));
	}
	else if (ptr = strpbrk(name, " :")) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), vfmt, *ptr,
					GetGizmoText(label_gname), name);
	}
	if (*gid == '\0') {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), GetGizmoText(format_noIdFmt),
			GetGizmoText(label_gid));
	}
	if ((n = strspn(gid,"0123456789")) != strlen(gid)) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), vfmt, gid[n],
					GetGizmoText(label_gid), gid);
	}
	else if (atoi(gid) > UID_MAX) {	/* defined in <limits.h> */
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), GetGizmoText(format_maxIdFmt),
				GetGizmoText(label_gid), gid, UID_MAX);
	}
	else
	{
	    for (n = 0; n < g_cnt; n++)
                if (strcmp(g_list[n].g_name, name) == 0)
		{
		    /* if just changing gid then name will be in list */
		    if (g_reset == NULL || g_list[n].g_gid == g_reset->g_gid) 
			break;

		    valid = FALSE;
		    sprintf(vld_msg+strlen(vld_msg),
			    GetGizmoText(format_inUse),
			    GetGizmoText(label_gname), name);
		    break;
		}
	}
	    
	return valid;
}

static	void
CheckDuplicate(int uidno, char *buf)
{
	int	n;

	for (n = 0; n < uid_cnt; n++)
		if (uidno == uid_list[n]) {
			sprintf(buf+strlen(buf), GetGizmoText(format_reuseFmt),
					GetGizmoText(label_uid), uidno);
			break;
		}
}

static	void
CheckDuplGroup(int gidno, char *buf)
{
	int	n;

	for (n = 0; n < g_cnt; n++)
		if (gidno == g_list[n].g_gid) {
			sprintf(buf+strlen(buf), GetGizmoText(format_reuseFmt),
					GetGizmoText(label_gid), gidno);
			break;
		}
}

static	void
applyCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    int		idno;
    char       *name;
    char	buf[PATH_MAX];
    Widget	shell_wid;
    int		pending;
    int		popup_type;

    if ((shell_wid = _OlGetShellOfWidget(wid)) == NULL)
	shell_wid = (view_type == GROUPS) ? w_gpopup : w_popup;
    if (shell_wid == w_gpopup)
	popup_type = GROUPS;
    else
	popup_type = USERS;	/* for now... see below */
    FooterMsg(NULL);
    busyCursor(popup_type);

    if (popup_type == GROUPS)
    {
	pending = g_pending;
	context = GetGizmoText(tag_group);
	SetPopupMessage(&group_popup, "");
	XtVaGetValues(w_gname, XtNstring, &gname, NULL);
	name = gname;
	XtVaGetValues(w_gid, XtNstring, &gid, NULL);
	idno = atoi(gid);
	if (!ValidateGroup(name, gid))
	{
	    ErrorNotice(vld_msg, popup_type);
	    standardCursor(popup_type);
	    return;
	}
    }
    else
    {
	SetPopupMessage(&prop_popup, "");
	pending = u_pending;
	XtVaGetValues(w_uid,   XtNstring, &uid,  NULL);
	idno = atoi(uid);
	popup_type = (idno < LOWEST_USER_UID) ? SYSLOG : USERS;
	context = GetGizmoText((popup_type == USERS) ? tag_login : tag_sys);
	XtVaGetValues(w_login, XtNstring, &login, NULL);
	name = login;
	XtVaGetValues(w_desc,  XtNstring, &desc,  NULL);
	XtVaGetValues(w_home,  XtNstring, &home,  NULL);
	removeWhitespace(home);
	if (*home == EOS)
	    home = HOME_DIR;
	if (strcmp(home,HOME_DIR)==0) {
	    strcat(strcpy(buf,home),login);
	    home = strdup(buf);		/* memory leak */
	    XtVaSetValues(w_home, XtNstring, (XtArgVal)home, NULL);
	    sethome = 0;
	}
	XtVaGetValues(w_group, XtNstring, &group,  NULL);
	XtVaGetValues(w_shell, XtNstring, &shell,  NULL);
	if (!Validate())
	{
	    ErrorNotice(vld_msg, popup_type);
	    standardCursor(popup_type);
	    return;
	}
    }

    fmt = GetGizmoText(format_applyFmt);
    operation = (pending == P_ADD)? GetGizmoText(tag_addOp):
    GetGizmoText(tag_chgOp);

    if (pending == P_CHG)
    {
	if (shell_wid == w_gpopup)
	    name = g_reset->g_name;
	else
	    name = u_reset->pw_name;
    }

    sprintf(buf, fmt, operation, context, name, GetGizmoText(tag_query));
    if (shell_wid == w_gpopup) {
	if (pending == P_ADD || idno != g_reset->g_gid)
	    CheckDuplGroup(idno, buf);
    }
    else if (pending == P_ADD || idno != (int)u_reset->pw_uid)
	CheckDuplicate(idno, buf);
    DisplayPrompt(buf, shell_wid);
}

static	void
SetGroup(Widget wid, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData	*d = (OlFlatCallData *)call_data;

	XtVaSetValues(w_group,
			XtNstring, (XtArgVal)GroupItems[d->item_index].label,
			NULL);
}

static	void
SetDesktop(Widget wid, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData	*d = (OlFlatCallData *)call_data;
	int dtm_flag;

	dtm_flag = (d->item_index == 0);
	XtVaSetValues(w_gui, XtNsensitive, dtm_flag, NULL);
	XtVaSetValues(XtParent(w_gui), XtNsensitive, dtm_flag, NULL);
	XtVaSetValues(w_remote, XtNsensitive, dtm_flag, NULL);
	XtVaSetValues(XtParent(w_remote),XtNsensitive,dtm_flag,NULL);
}


/*
 *	check file permissions; note that the bits to access(2) defined in
 *	unistd.h have the same structure as the 3-bit groupings for user,
 *	group and other in the full file mode as returned by stat.  Access
 *	can't be used here, as I am checking on someone ELSE's permissions.
 *	FileCheck will fail if the file does not exist, if permissions are bad
 */
static	Boolean
FileCheck(char *dir, char *base, int bits, int uid, int gid)
{
struct	stat	stbuf;
	char	path[PATH_MAX];

	sprintf(path,"%s/%s", dir, base);
	if (stat(path,&stbuf) != 0)
		return FALSE;
	else {
		/*	note that these tests may be different in ES,
		 *	or for both SVR4.0 and ES after "owner" stuff
		 *	is honestly implemented.
		 */
		if (uid == 0)
			return TRUE;
		if (uid == stbuf.st_uid)
			bits *= S_IXUSR;
		else if (gid == stbuf.st_gid)
			bits *= S_IXGRP;
		return ((bits & stbuf.st_mode) == bits);
	}
}

/*
 *	check for presence of .olsetup, and determine XGUI and REMOTE values
 */
static	void
CheckIfDtm(int uid, int gid, char *homedir)
{
    char	*ptr, buf[BUFSIZ];
    int	state = NO_DTM;
    FILE	*fp;

    if (!FileCheck(homedir, ".olsetup", R_OK, uid, gid))
    {
	dtm_account = False;
	return;
    }
    sprintf(buf, "%s/%s", homedir, ".olsetup");
    if (fp = fopen(buf,"r"))
    {
	dtm_account = True;
	while (fgets(buf, BUFSIZ, fp)) {
	    if (strncmp(buf,"XGUI=",5)==0)
		dtm_style = strstr(buf,"MOTIF")? MOTIF_DTM: OL_DTM;
	    if (strstr(buf,"REMOTE=")) {
		for (ptr = buf+7; !isspace(*ptr); ptr++)
		    ;
		*ptr = '\0';
		if (buf[7])
		    remote = strdup(buf+7);
	    }
	}
	fclose(fp);
    }
    else
	dtm_account = False;	/* FIX: is this line needed? */
    return;
}

static	void
SetGroupValues(GroupPtr gp)
{
    static	char	gid[8];
    char	*gname;
    Arg         arg[8];

    if (gp == NULL) {
	gname = "";
	sprintf(gid, "%d", max_gid+1);
	XtSetArg(arg[0], XtNtitle, (XtArgVal)GetGizmoText(string_addGroupLine));
	XtSetValues(w_gpopup, arg, 1);
	OlVaFlatSetValues(group_menu.child, 0,
			  XtNlabel, GetGizmoText(label_add),
			  XtNmnemonic, *(GetGizmoText(mnemonic_add)), NULL);
    }
    else {
	group = gname = gp->g_name;
	sprintf(gid, "%d", gp->g_gid);
	XtSetArg(arg[0], XtNtitle, (XtArgVal)GetGizmoText(string_groupLine));
	XtSetValues(w_gpopup, arg, 1);
	OlVaFlatSetValues(group_menu.child, 0,
			  XtNlabel, GetGizmoText(label_apply),
			  XtNmnemonic, *(GetGizmoText(mnemonic_apply)), NULL);
    }
    XtVaSetValues(w_gname, XtNstring, (XtArgVal)gname, NULL);
    XtVaSetValues(w_gid, XtNstring, (XtArgVal)gid, NULL);
}

static	void
SetPopupValues(UserPtr	up)
{
    static	char	uid[8];
    struct	group	*gp;
    Arg         arg[8];

    if (remote) {
	free(remote);
	remote = NULL;
    }
    sethome = 1;
    if (up) {
	CheckIfDtm(up->pw_uid, up->pw_gid, up->pw_dir);
	login = up->pw_name;
	desc  = up->pw_comment;
	home  = up->pw_dir;
	shell = up->pw_shell;
	sprintf(uid, "%d", up->pw_uid);
	gp = getgrgid(up->pw_gid);
	group = strdup(gp ? gp->gr_name : "");
    }
    else {
	dtm_account = True;
	dtm_style   = MOTIF_DTM;
	login       = "";
	home        = HOME_DIR;
	desc        = ""; 
	sprintf(uid,  "%d", NextUid());
	group       = "other";
	remote      = "";
	shell       = KSH;
	if (!FileCheck("", shell, X_OK, atoi(uid), 1))
	    shell = BSH;
    }
    OlVaFlatSetValues(w_dtm, (dtm_account? 0: 1), XtNset, TRUE, NULL);
    XtVaSetValues(w_gui, XtNsensitive, dtm_account, NULL);
    XtVaSetValues(XtParent(w_gui), XtNsensitive, dtm_account, NULL);
    XtVaSetValues(w_remote, XtNsensitive, dtm_account, NULL);
    XtVaSetValues(XtParent(w_remote),XtNsensitive,dtm_account,NULL);
    OlVaFlatSetValues(w_gui,(dtm_style==MOTIF_DTM? 0: 1),XtNset,TRUE,NULL);
    if (login)	XtVaSetValues(w_login, XtNstring, login, NULL);
    if (desc)	XtVaSetValues(w_desc,  XtNstring, desc,  NULL);
    if (home)	XtVaSetValues(w_home,  XtNstring, home,  NULL);
    if (shell)	XtVaSetValues(w_shell, XtNstring, shell, NULL);
    XtVaSetValues(w_remote,XtNstring, remote ? remote : "" ,NULL);
    XtVaSetValues(w_uid,   XtNstring, uid,   NULL);
    if (group) {
	int	i, n;
	XtVaSetValues(w_group, XtNstring, (XtArgVal)group, NULL);
	for (n = 0; n < g_cnt; n++)
	    if (strcmp(group,GroupItems[n].label))
		GroupItems[n].set = FALSE;
	    else
		GroupItems[i = n].set = TRUE;
	XtVaSetValues(w_glist,
		      XtNviewItemIndex,	(XtArgVal)i,
		      NULL);
    }
}

static	void
resetCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    Widget     shell_wid;

    if ((shell_wid = _OlGetShellOfWidget(wid)) == NULL)
	shell_wid = (view_type == GROUPS) ? w_gpopup : w_popup;

    FooterMsg(NULL);
    if (shell_wid == w_gpopup)
    {
	moveFocus(w_gname);
	SetPopupMessage(&group_popup, "");
	SetGroupValues(g_reset);
    }
    else
    {
	moveFocus(w_login);
	SetPopupValues(u_reset);
	SetPopupMessage(&prop_popup, "");
    }
    return;
}

static	void
reinitPopup(int popup_type)
{
    FooterMsg(NULL);
    if (popup_type == GROUPS)
    {
	SetPopupMessage(&group_popup, "");
	SetGroupValues(NULL);
	moveFocus(w_gname);
    }
    else
    {
	SetPopupValues(NULL);
	SetPopupMessage(&prop_popup, "");
	moveFocus(w_login);
    }
    return;
}

static	void
cancelCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    Widget     shell_wid;

    if ((shell_wid = _OlGetShellOfWidget(wid)) == NULL)
	shell_wid = (view_type == GROUPS) ? w_gpopup : w_popup;

    if (shell_wid == w_gpopup)
    {
	SetPopupMessage(&group_popup, NULL);
        g_pending = 0;
    }
    else
    {
	SetPopupMessage(&prop_popup, NULL);
        u_pending = 0;
    }

    FooterMsg(NULL);
    SetWMPushpinState(XtDisplay(shell_wid), XtWindow(shell_wid), WMPushpinIsOut);
    BringDownPopup(shell_wid);
}

static	void
LoginToHomeCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	char			*dir, buf[PATH_MAX];
	OlTextFieldVerify	*vf = (OlTextFieldVerify *)call_data;
	Arg			 arg[5];

	XtVaGetValues(w_home, XtNstring, &dir, NULL);
	if (strcmp(dir, HOME_DIR)==0) {
		/*
		 *	i.e., only supply the login as basename if there is
		 *	NOT already something filling in the w_home field
		 */
		strcat(strcpy(buf,dir),vf->string);
		home = strdup(buf);        /* memory leak */
		XtSetArg(arg[0], XtNstring, (XtArgVal)home);
		XtSetValues(w_home, arg, 1);
		sethome = 0;
	}
}

#define	FIELD_WIDTH	27

static	void
ExtraProperties(Widget parent)
{
	Widget	w_cap, w_sc;

	w_extctls[0] = XtVaCreateWidget("control", controlAreaWidgetClass,
			parent,
			XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
			XtNvPad,		(XtArgVal)yinch/6,
			XtNvSpace,		(XtArgVal)yinch/6,
			XtNcenter,		(XtArgVal)FALSE,
			XtNalignCaptions,	(XtArgVal)TRUE,
			NULL);
	w_cap = XtVaCreateManagedWidget("caption",captionWidgetClass,
                        w_extctls[0],
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_home),
			NULL); 
	w_home = XtVaCreateManagedWidget("home", textFieldWidgetClass, w_cap,
			XtNcharsVisible,	(XtArgVal)FIELD_WIDTH,
			NULL);
	w_cap = XtVaCreateManagedWidget("caption",captionWidgetClass,
                        w_extctls[0],
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_remote),
			NULL); 
	w_remote = XtVaCreateManagedWidget("home", textFieldWidgetClass, w_cap,
			XtNcharsVisible,	(XtArgVal)FIELD_WIDTH,
			NULL);
	w_cap = XtVaCreateManagedWidget("caption",captionWidgetClass,
                        w_extctls[0],
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_shell),
			NULL); 
	w_shell = XtVaCreateManagedWidget("shell", textFieldWidgetClass, w_cap,
			XtNcharsVisible,	(XtArgVal)FIELD_WIDTH,
			NULL);
	w_cap = XtVaCreateManagedWidget("caption",captionWidgetClass,
                        w_extctls[0],
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_uid),
			NULL); 
	w_uid = XtVaCreateManagedWidget("uid", textFieldWidgetClass, w_cap,
			XtNcharsVisible,	(XtArgVal)FIELD_WIDTH,
			NULL);
	w_extctls[1] = XtVaCreateWidget("control", controlAreaWidgetClass, 
			parent,
			XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
			XtNhPad,		(XtArgVal)xinch/2,
			XtNcenter,		(XtArgVal)TRUE,
			NULL);
	w_cap = XtVaCreateManagedWidget("caption",captionWidgetClass,
                        w_extctls[1],
			XtNposition,	(XtArgVal)OL_TOP,
			XtNalignment,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_glist),
			NULL);
	w_sc = XtVaCreateManagedWidget("scroller", scrolledWindowWidgetClass,
			w_cap,
			NULL);
	w_glist = XtVaCreateManagedWidget("grouplist", flatListWidgetClass,
			w_sc,
			XtNviewHeight,		GROUPHEIGHT,
			XtNformat,		(XtArgVal)"%12s",
			XtNexclusives,		(XtArgVal)TRUE,
			XtNitemFields,		(XtArgVal)ListFields,
			XtNnumItemFields,	(XtArgVal)2,
			XtNitems,		(XtArgVal)GroupItems,
			XtNnumItems,		(XtArgVal)g_cnt,
			XtNselectProc,		(XtArgVal)SetGroup,
			NULL);
	w_cap = XtVaCreateManagedWidget("caption",captionWidgetClass,
                        w_extctls[1],
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_group),
			NULL); 
	w_group = XtVaCreateManagedWidget("group", staticTextWidgetClass, w_cap,
			XtNstring,	(XtArgVal)"other",
			NULL);
}

static	void
CreatePropSheet(void)
{
static	ExclItem	acctype[2];
static	ExclItem	guitype[2];
static	ExclItem	extraitem[1];
	Widget		w_up, w_cap;

	CreateGizmo(w_baseshell, PopupGizmoClass, &prop_popup, NULL, 0);

	w_popup = GetPopupGizmoShell(&prop_popup);
	XtVaGetValues(w_popup, XtNupperControlArea, &w_up, NULL);
	XtVaSetValues(w_up,
			XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
			XtNmeasure,		(XtArgVal)2,
			XtNvPad,		(XtArgVal)yinch/4,
			XtNvSpace,		(XtArgVal)yinch/4,
			XtNhPad,		(XtArgVal)xinch/4,
			XtNhSpace,		(XtArgVal)xinch/4,
			NULL);
        XtAddCallback(w_popup, XtNpopdownCallback, hideExtraCB, NULL);
	w_cap = XtVaCreateManagedWidget("caption", captionWidgetClass, w_up,
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_login),
			NULL); 
	w_login = XtVaCreateManagedWidget("login", textFieldWidgetClass, w_cap,
			XtNcharsVisible,	(XtArgVal)FIELD_WIDTH,
			NULL);
	XtAddCallback(w_login, XtNverification, LoginToHomeCB, NULL);
	w_cap = XtVaCreateManagedWidget("caption", captionWidgetClass, w_up,
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_type),
			NULL); 
	SET_EXCL(acctype, 0, desktop, TRUE);
	SET_EXCL(acctype, 1, nondesk, FALSE);
	w_dtm = XtVaCreateManagedWidget("desktop",flatButtonsWidgetClass, w_cap,
			XtNtraversalOn,		(XtArgVal)TRUE,
			XtNbuttonType,		(XtArgVal)OL_RECT_BTN,
			XtNexclusives,		(XtArgVal)TRUE,
			XtNitemFields,		(XtArgVal)ExclFields,
			XtNnumItemFields,	(XtArgVal)3,
			XtNitems,		(XtArgVal)acctype,
			XtNnumItems,		(XtArgVal)2,
			XtNselectProc,		(XtArgVal)SetDesktop,
			NULL);
	w_cap = XtVaCreateManagedWidget("caption", captionWidgetClass, w_up,
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_desc),
			NULL); 
	w_desc = XtVaCreateManagedWidget("desc", textFieldWidgetClass, w_cap,
			XtNcharsVisible,	(XtArgVal)FIELD_WIDTH,
			NULL);
	w_cap = XtVaCreateManagedWidget("caption", captionWidgetClass,
			w_up,
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_GUI),
			NULL); 
	SET_EXCL(guitype, 0, motif, TRUE);
	SET_EXCL(guitype, 1, ol,    FALSE);
	w_gui = XtVaCreateManagedWidget("gui", flatButtonsWidgetClass,
			w_cap,
			XtNtraversalOn,		(XtArgVal)TRUE,
			XtNbuttonType,		(XtArgVal)OL_RECT_BTN,
			XtNexclusives,		(XtArgVal)TRUE,
			XtNitemFields,		(XtArgVal)ExclFields,
			XtNnumItemFields,	(XtArgVal)3,
			XtNitems,		(XtArgVal)guitype,
			XtNnumItems,		(XtArgVal)2,
			NULL);
	XtVaCreateManagedWidget("spacer", rectObjClass, w_up,
			XtNwidth,		(XtArgVal)10,
			XtNheight,		(XtArgVal)10,
			NULL);
	extraitem[0].label = GetGizmoText(label_extra);
	extraitem[0].setting = FALSE;
	w_extra = XtVaCreateManagedWidget("checkbox", flatButtonsWidgetClass,
			w_up,
			XtNtraversalOn,		(XtArgVal)TRUE,
			XtNbuttonType,		(XtArgVal)OL_CHECKBOX,
			XtNitemFields,		(XtArgVal)ExclFields,
			XtNnumItemFields,	(XtArgVal)3,
			XtNitems,		(XtArgVal)extraitem,
			XtNnumItems,		(XtArgVal)1,
			XtNselectProc,		(XtArgVal)SelExtraCB,
			XtNunselectProc,	(XtArgVal)UnselExtraCB,
			NULL);
	ExtraProperties(w_up);
}

static	void
CreateGroupProp(void)
{
	Widget		w_up, w_cap;

	CreateGizmo(w_baseshell, PopupGizmoClass, &group_popup, NULL, 0);
	w_gpopup = GetPopupGizmoShell(&group_popup);
	XtVaGetValues(w_gpopup, XtNupperControlArea, &w_up, NULL);
	XtVaSetValues(w_up,
			XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
			XtNvPad,		(XtArgVal)yinch/4,
			XtNvSpace,		(XtArgVal)yinch/4,
			NULL);
	w_cap = XtVaCreateManagedWidget("caption", captionWidgetClass, w_up,
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_gname),
			NULL); 
	w_gname = XtVaCreateManagedWidget("group", textFieldWidgetClass, w_cap,
			XtNcharsVisible,	(XtArgVal)20,
			NULL);
	w_cap = XtVaCreateManagedWidget("caption", captionWidgetClass, w_up,
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_gid),
			NULL); 
	w_gid = XtVaCreateManagedWidget("desc", textFieldWidgetClass, w_cap,
			XtNcharsVisible,	(XtArgVal)20,
			NULL);
}

static	void
MakePermList(void)
{
	FILE	*pfile;
	char	buf[BUFSIZ];

	if (pfile = fopen(GetXWINHome(PRIV_TABLE),"r")) {
		while (fgets(buf, BUFSIZ, pfile)) {
			p_list = p_cnt==0 ?
				(PermPtr)malloc(sizeof(PermRec)) :
				(PermPtr)realloc(p_list,(p_cnt+1)*sizeof(PermRec));
			if (p_list == NULL) {
				FooterMsg(GetGizmoText(string_malloc));
				break;
			}
			p_list[p_cnt].label = strdup(strtok(buf, "\t\n"));
			p_list[p_cnt].cmds  = strdup(strtok(NULL,"\t\n"));
			p_list[p_cnt].help  = strdup(strtok(NULL,"\t\n"));
			p_cnt++;
		}
		pclose(pfile);
	}
	else
		FooterMsg(GetGizmoText(string_permfile));
}

static	void
CheckPermList(UserPtr u_select)
{
	FILE	*fperm;
	char	buf[BUFSIZ];
	int	n;

	this_is_owner = FALSE;
	for (n = 0; n < p_cnt; n++)
		p_list[n].granted = FALSE;
	sprintf(buf, "%s/%s", PERM_FILE, u_select->pw_name);
	if (fperm = fopen(GetXWINHome(buf),"r")) {
		while (fgets(buf, BUFSIZ, fperm)) {
			buf[strlen(buf)-1] = '\0';
			if (strcmp(buf, OWNER) == 0)
				this_is_owner = TRUE;
			else if (*buf == '#' || strncmp(buf,"ICON=",5)==0)
				;
			else for (n = 0; n < p_cnt; n++) {
				if (strcmp(buf, p_list[n].label) == 0) {
					p_list[n].granted = TRUE;
					break;
				}
			}
		}
		fclose (fperm);
	}
}

static	void
resetPermCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	int	n;
	Boolean	ok;

        SetPopupMessage(&perm_popup, NULL);
	OlVaFlatSetValues(w_own, 0, XtNset, (XtArgVal)this_is_owner, NULL);
	for (n = 0; n < p_cnt; n++) {
		ok = (/*this_is_owner ||*/ p_list[n].granted);
		OlVaFlatSetValues(w_checks, n, XtNset, (XtArgVal)ok, NULL);
	}
}

static	void
ChangeOwner(char * new_owner)
{
	char	buf[80];

	u_pending = P_OWN;
	login = new_owner;
	operation = GetGizmoText(tag_xferOp);
	fmt = GetGizmoText(format_ownerFmt);
	sprintf(buf,fmt,owner_set?"add":"remove",operation,owner_set?"to":"from",
		login,GetGizmoText(tag_query));
	DisplayPrompt(buf, w_perm);
}

static	void
applyPermCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	Boolean	set;
	FILE	*fp;
	char	*usr, *ptr, buf[BUFSIZ];
	int	n, change, exit_code;

        SetPopupMessage(&perm_popup, NULL);
	usr = SelectedUser()->pw_name;
	OlVaFlatGetValues(w_own, 0, XtNset, &set, NULL);
	owner_set=set;
	if (this_is_owner != set)
		ChangeOwner(usr);
	strcpy(buf,ADMINUSER);
	ptr = buf+strlen(buf);
	for (change = n = 0; n < p_cnt; n++) {
		OlVaFlatGetValues(w_checks, n, XtNset, &set, NULL);
		if (set == p_list[n].granted)
			continue;
		else if (change == 0) {	/* confirm user in database */
			sprintf(ptr, "%s >/dev/null 2>&1", usr);
			if ((exit_code = system(buf)) != 0) {
				sprintf(ptr, "-n %s", usr);
				if ((exit_code = system(buf)) != 0) {
					ptr = GetGizmoText(string_adm);
					FooterMsg(ptr);
					return;
				}
			}
		}
		/*
		 *	update the changed permission value
		 */
		if (set)
			sprintf(ptr, "-a %s ",p_list[n].cmds);
		else {
			char	*src, *dst;
 			/*
			 * 	removal just uses command *names*
			 */
			strcpy(ptr,"-r ");
			for (src=p_list[n].cmds,dst=ptr+3;*src;++src){
				if (*src != ':')
					*dst++ = *src;
				else {
					do src++;
					while (*src && *src !=  ',');
					--src;
				}
			}
			*dst++ = ' ';
			*dst = '\0';
		}
		strcat(buf, usr);
		if (system(buf) == 0) {
			change = 1;
			p_list[n].granted = set;
		}
		else {
			change = -1;
			break;
		}
	}
	switch (change) {
		case -1:	ptr = GetGizmoText(tag_bad);  break;
		case  0:	ptr = GetGizmoText(tag_null); break;
		case  1:	ptr = GetGizmoText(tag_good); break;
	}
	if (change || owner_set == this_is_owner)
	{
	    sprintf(buf, GetGizmoText(format_permFmt), usr, ptr);
	    FooterMsg(buf);
	}
	/*
	 *	update the record in PERM_FILE
	 */
	if (change) {
		sprintf(buf,"%s/%s",PERM_FILE,usr);
		if (fp=fopen(GetXWINHome(buf),"w")) {
		    if (this_is_owner == TRUE)
			fprintf(fp, "%s\n", OWNER);
		    for (n = 0; n < p_cnt; n++)
			if (p_list[n].granted)
			    fprintf(fp,"%s\n",p_list[n].label);
		    fclose(fp);
		}
                BringDownPopup(w_perm);
	}
}

static	void
cancelPermCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    SetPopupMessage(&perm_popup, NULL);
    SetWMPushpinState(XtDisplay(w_perm), XtWindow(w_perm), WMPushpinIsOut);
    XtPopdown(w_perm);
}

static	void
SetOwnerCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	int	n;

        SetPopupMessage(&perm_popup, NULL);
	for (n = 0; n < p_cnt; n++)
		OlVaFlatSetValues(w_checks, n, XtNset, (XtArgVal)TRUE, NULL);
}

static	void
UnsetOwnerCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	int	n;

        SetPopupMessage(&perm_popup, NULL);
	for (n = 0; n < p_cnt; n++)
		OlVaFlatSetValues(w_checks, n, XtNset,
				(XtArgVal)p_list[n].granted, NULL);
}

static	void
CreatePermSheet(void)
{
static	ExclItem	OwnerItem[1];
static	ExclItem	*PermItems;	/* actually, nonexclusive checkboxes */
	Widget		w_up, w_sc,  w_cap;
	int		n;

	CreateGizmo(w_baseshell, PopupGizmoClass, &perm_popup, NULL, 0);
	w_perm = GetPopupGizmoShell(&perm_popup);
	XtVaGetValues(w_perm, XtNupperControlArea, &w_up, NULL);
	XtVaSetValues(w_up,
			XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
			XtNcenter,		(XtArgVal)TRUE,
			XtNhPad,		(XtArgVal)xinch/2,
			XtNvPad,		(XtArgVal)yinch/3,
			XtNvSpace,		(XtArgVal)yinch/4,
			NULL);
	SET_EXCL(OwnerItem, 0, owner_acct, FALSE);
	w_own = XtVaCreateManagedWidget("checkbox", flatButtonsWidgetClass,
			w_up,
			XtNtraversalOn,		(XtArgVal)TRUE,
			XtNbuttonType,		(XtArgVal)OL_CHECKBOX,
			XtNitemFields,		(XtArgVal)ExclFields,
			XtNnumItemFields,	(XtArgVal)3,
			XtNitems,		(XtArgVal)OwnerItem,
			XtNnumItems,		(XtArgVal)1,
			XtNselectProc,		(XtArgVal)SetOwnerCB,
			XtNunselectProc,	(XtArgVal)UnsetOwnerCB,
			NULL);
	w_cap = XtVaCreateManagedWidget("caption",captionWidgetClass,w_up,
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_user_may),
			NULL); 
	MakePermList();
	PermItems = (ExclItem *)malloc(p_cnt * sizeof(ExclItem));
	for (n = 0; n < p_cnt; n++) {
		char	*ptr 		= strdup(p_list[n].label);
		PermItems[n].label	= DtamGetTxt(ptr);
		PermItems[n].mnem	= 0;
		PermItems[n].setting	= (XtArgVal)FALSE;
	}
	w_sc = p_cnt < 12? w_up:
		XtVaCreateManagedWidget("scrolled", scrolledWindowWidgetClass,
			w_up,
			XtNwidth,	(XtArgVal)((Dimension)3.5*xinch),
			XtNheight,	(XtArgVal)((Dimension)2.5*yinch),
			NULL);
	w_checks = XtVaCreateManagedWidget("checkbox", flatButtonsWidgetClass,
			w_sc,
			XtNtraversalOn,		(XtArgVal)TRUE,
			XtNbuttonType,		(XtArgVal)OL_CHECKBOX,
			XtNlabelJustify,	(XtArgVal)OL_RIGHT,
			XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
			XtNitemFields,		(XtArgVal)ExclFields,
			XtNnumItemFields,	(XtArgVal)3,
			XtNitems,		(XtArgVal)PermItems,
			XtNnumItems,		(XtArgVal)p_cnt,
			NULL);
}

static	void
DoPopup(UserPtr u_select)
{
	resetFocus(view_type);
	if (view_type == GROUPS) {
		g_reset = (GroupPtr)u_select;
		SetGroupValues(g_reset);
		XtPopup(w_gpopup, XtGrabNone);
	}
	else {
		u_reset = u_select;
		SetPopupValues(u_select);
/*		XtVaSetValues(w_popup, XtNpushpin, (XtArgVal)OL_IN, NULL);
*/
		OlVaFlatSetValues(w_extra, 0, XtNset, (XtArgVal)FALSE, NULL);
                XtUnmanageChildren(w_extctls, XtNumber(w_extctls));
		XtPopup(w_popup, XtGrabNone);
	}
}

static	void
addCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    char	buf[80];
    Arg		arg[4];

    FooterMsg(NULL);
    switch(view_type)
    {
    case GROUPS:
	SetPopupMessage(&group_popup, "");
	g_pending = P_ADD;
	break;
    case USERS:
	XtSetArg(arg[0], XtNtitle, (XtArgVal)GetGizmoText(string_addUserLine));
	XtSetValues(w_popup, arg, 1);
	OlVaFlatSetValues(prop_menu.child, 0,
			  XtNlabel, GetGizmoText(label_add),
			  XtNmnemonic, *(GetGizmoText(mnemonic_add)), NULL);
	SetPopupMessage(&prop_popup, "");
	u_pending = P_ADD;
	break;
    case SYSLOG:
	XtSetArg(arg[0], XtNtitle, (XtArgVal)GetGizmoText(string_addSysLine));
	XtSetValues(w_popup, arg, 1);
	OlVaFlatSetValues(prop_menu.child, 0,
			  XtNlabel, GetGizmoText(label_add),
			  XtNmnemonic, *(GetGizmoText(mnemonic_add)), NULL);
	SetPopupMessage(&prop_popup, "");
	u_pending = P_ADD;
	break;
    default:
	break;
    }
    DoPopup((UserPtr)NULL);
}

static	void
deleteCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	char		*name, buf[80];
	UserPtr		u_sel = SelectedUser();

	FooterMsg(NULL);
	operation = GetGizmoText(tag_delOp);
	setContext(view_type);
	if (!u_sel) {
		sprintf(buf, GetGizmoText(format_noSelectFmt), context);
		ErrorNotice(buf, view_type);
	}
	else {
		fmt = GetGizmoText(format_applyFmt);
		if (view_type == GROUPS) {
			GroupPtr	g_sel = (GroupPtr)u_sel;

		        g_pending = P_DEL;
			name = gname = strdup(g_sel->g_name);
		}
		else {
		        u_pending = P_DEL;
			name = login = strdup(u_sel->pw_name);
			home = u_sel->pw_dir;
			home_flag = DEL_HOME;
			XtVaGetValues(w_uid, XtNstring, &uid, NULL);
		}
		sprintf(buf, fmt, operation, context, name, "?");
		ConfirmDelete(buf);
	}
}
    
static	void
propCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    UserPtr	 u_select = SelectedUser();
    PopupGizmo	*pop;
    char 	 buf[80];
    Arg		 arg[4];

    setContext(view_type);
    if (!u_select)
    {
	sprintf(buf, GetGizmoText(format_noSelectFmt), context);
	ErrorNotice(buf, view_type);
	return;
    }
    FooterMsg(NULL);
    if (view_type == GROUPS)
    {
	g_pending = P_CHG;
	g_reset = (GroupPtr)u_select;
	SetGroupValues(g_reset);
    }
    else
    {
	u_pending = P_CHG;
	u_reset = u_select;
	XtSetArg(arg[0], XtNtitle,
		 (XtArgVal)GetGizmoText(view_type == USERS ?
				    string_propLine:string_sysLine)); 
	XtSetValues(w_popup, arg, 1);
	OlVaFlatSetValues(prop_menu.child, 0,
			  XtNlabel, GetGizmoText(label_apply),
			  XtNmnemonic, *(GetGizmoText(mnemonic_apply)), NULL);
	SetPopupValues(u_reset);
    }
    pop = view_type==GROUPS? &group_popup: &prop_popup; 
    SetPopupMessage(pop, NULL);
    resetFocus(view_type);
    MapGizmo(PopupGizmoClass, pop);
}

static	void
permCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    UserPtr	u_select = SelectedUser();
    char	buf[80];
    char       *errContext;

    errContext = (view_type == GROUPS) ? tag_account :
	((view_type == USERS) ? tag_login : tag_sys);
    SetPopupMessage(&perm_popup, NULL);
    if (view_type == GROUPS || !u_select)
    {
	sprintf(buf, GetGizmoText(format_noSelectFmt),
		GetGizmoText(errContext));
	ErrorNotice(buf, view_type);
    }
    else {
	FooterMsg(NULL);
	CheckPermList(u_select);
	resetPermCB(w, NULL, NULL);
	XtPopup(w_perm, XtGrabNone);
    }
}

static	void
SetViewCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData	*d = (OlFlatCallData *)call_data;
    Arg  arg[4];

    FooterMsg(NULL);
    view_type = d->item_index;
    switch (view_type)
    {
    case USERS:
	XtSetArg(arg[0], XtNtitle, GetGizmoText(string_userBaseLine));    
	XtSetValues(w_baseshell, arg, 1);
	OlVaFlatSetValues(menu_bar.child, 2,
			  XtNlabel, GetGizmoText(label_edit),
			  XtNmnemonic,
			  *(GetGizmoText(mnemonic_edit)), NULL);
	OlVaFlatSetValues(GetMenu(&action_menu), action_perm,
			  XtNsensitive, True, NULL);
	break;
    case GROUPS:
	XtSetArg(arg[0], XtNtitle, GetGizmoText(string_groupBaseLine));    
	XtSetValues(w_baseshell, arg, 1);
	OlVaFlatSetValues(menu_bar.child, 2,
			  XtNlabel, GetGizmoText(label_groupBtn),
			  XtNmnemonic,
			  *(GetGizmoText(mnemonic_groupBtn)), NULL);
	OlVaFlatSetValues(GetMenu(&action_menu), action_perm,
			  XtNsensitive, False, NULL);
	break;
    case SYSLOG:
	XtSetArg(arg[0], XtNtitle, GetGizmoText(string_sysBaseLine));    
	XtSetValues(w_baseshell, arg, 1);
	OlVaFlatSetValues(menu_bar.child, 2,
			  XtNlabel, GetGizmoText(label_edit),
			  XtNmnemonic,
			  *(GetGizmoText(mnemonic_edit)), NULL);
	OlVaFlatSetValues(GetMenu(&action_menu), action_perm,
			  XtNsensitive, True, NULL);
	break;
    default:
	break;
    }
    ResetIconBox();
}

static	void
SingleClick(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmObjectPtr	 op;
    OlFlatCallData	*d = (OlFlatCallData *)call_data;
    Arg              arg[4];

    FooterMsg(NULL);
    OlVaFlatGetValues(w, d->item_index, XtNobjectData, &op, NULL);
    if (view_type == GROUPS)
    {
	g_pending = P_CHG;
	XtSetArg(arg[0], XtNtitle,
		 (XtArgVal)GetGizmoText(string_groupLine)); 
	XtSetValues(w_gpopup, arg, 1);
	SetGroupValues(g_reset = (GroupPtr)op->objectdata);
    }
    else
    {
	u_pending = P_CHG;
	SetPopupValues(u_reset = (UserPtr)op->objectdata);
	XtVaSetValues(w_glist, XtNviewHeight, GROUPHEIGHT, NULL);
	XtSetArg(arg[0], XtNtitle,
		 (XtArgVal)GetGizmoText(view_type == USERS ?
				    string_propLine:string_sysLine)); 
	XtSetValues(w_popup, arg, 1);
	CheckPermList(u_reset);
	resetPermCB(w, NULL, NULL);
    }
}

static	DmObjectPtr
AddUserItem(UserPtr p)
{
	FILE		*fperm;
	char		buf[BUFSIZ];
	DmObjectPtr	optr;
	DmGlyphPtr	glyph;

	optr = (DmObjectPtr)calloc(1, sizeof(DmObjectRec));
	optr->container = &u_cntrec;
	optr->name = p->pw_name;
	optr->fcp = &u_fcrec;
	sprintf(buf, "%s/%s", PERM_FILE, optr->name);
	if (fperm = fopen(GetXWINHome(buf),"r")) {
		while (fgets(buf, BUFSIZ, fperm)) {
			if (strncmp(buf,"ICON=",5)==0) {
				buf[strlen(buf)-1] = '\0';
				glyph = DmGetPixmap(theScreen,buf+5);
				if (optr->fcp = (DmFclassPtr)
						malloc(sizeof(DmFclassRec))) {
					optr->fcp[0] = u_fcrec;
					optr->fcp->glyph = glyph;
				}
				break;
			}
		}
		fclose(fperm);
	}
	optr->x = ibx;
	optr->y = iby;
	optr->objectdata = (XtPointer)p;
	if ((ibx += INC_X) > (Dimension)(WIDTH - MARGIN)) {
		ibx = INIT_X;
		iby += INC_Y;
	}
	if (item_count++ == 0)
		u_cntrec.op = optr;
	else {
		DmObjectPtr endp = u_cntrec.op;
		while (endp->next)
			endp = endp->next;
		endp->next = optr;
	}
	u_cntrec.num_objs = item_count;
	return optr;
}

static	DmObjectPtr
AddGroupItem(GroupPtr p)
{
	DmObjectPtr	optr;

	optr = (DmObjectPtr)calloc(1, sizeof(DmObjectRec));
	optr->container = &g_cntrec;
	optr->fcp = &g_fcrec;
	optr->name = p->g_name;
	optr->x = ibx;
	optr->y = iby;
	optr->objectdata = (XtPointer)p;
	if ((ibx += INC_X) > (Dimension)(WIDTH - MARGIN)) {
		ibx = INIT_X;
		iby += INC_Y;
	}
	if (item_count++ == 0)
		g_cntrec.op = optr;
	else {
		DmObjectPtr endp = g_cntrec.op;
		while (endp->next)
			endp = endp->next;
		endp->next = optr;
	}
	g_cntrec.num_objs = item_count;
	return optr;
}

static	Widget
GetIconBox(Widget parent)
{
    int	n;
    Arg	i_arg[8];
    Widget	w_box;

    item_count = 0; ibx = INIT_X; iby = INIT_Y;
    if (view_type == GROUPS) {
	for (n = 0; n < g_cnt; n++)
	    AddGroupItem(&g_list[n]);
    }
    else {
	for (n = 0; n < u_cnt; n++) {
	    if (view_type == USERS && u_list[n].pw_uid >= LOWEST_USER_UID
		&& u_list[n].pw_uid < UID_MAX-2
		||  view_type == SYSLOG && u_list[n].pw_uid < LOWEST_USER_UID
		||  view_type == SYSLOG && u_list[n].pw_uid > UID_MAX-3)
		AddUserItem(&u_list[n]);
	}
    }
    XtSetArg(i_arg[0], XtNexclusives,	(XtArgVal)TRUE);
    XtSetArg(i_arg[1], XtNmovableIcons,	(XtArgVal)FALSE);
    XtSetArg(i_arg[2], XtNminWidth,	(XtArgVal)1);
    XtSetArg(i_arg[3], XtNminHeight,	(XtArgVal)1);
    XtSetArg(i_arg[4], XtNdrawProc,	(XtArgVal)DmDrawIcon);
    XtSetArg(i_arg[5], XtNselectProc,	(XtArgVal)SingleClick);

    if (view_type == GROUPS)
	w_box = DmCreateIconContainer(parent, DM_B_CALC_SIZE, i_arg, 6,
				      g_cntrec.op, g_cntrec.num_objs,
				      &g_itp, g_cntrec.num_objs, NULL, NULL, def_font, 1);
    else
	w_box = DmCreateIconContainer(parent, DM_B_CALC_SIZE, i_arg, 6,
				      u_cntrec.op, u_cntrec.num_objs,
				      &u_itp, u_cntrec.num_objs, NULL, NULL, def_font, 1);
    return w_box;
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

main(int argc, char *argv[])
{   
    char           atom[SYS_NMLN+30]= ApplicationClass;
    struct utsname name;
    Window         another_window;
    Arg arg[5];

    I_am_owner = _DtamIsOwner(OWN_LOGIN);

    edit_menu_item[0].sensitive  = 
	edit_menu_item[1].sensitive  = 
	    prop_menu_item[0].sensitive  = 
		perm_menu_item[0].sensitive  = 
		    group_menu_item[0].sensitive = I_am_owner;

    OlToolkitInitialize(&argc, argv, (XtPointer)NULL);
    w_toplevel = XtInitialize("userad", ApplicationClass, NULL, 0, &argc, argv);
    DtInitialize(w_toplevel);
    InitializeGizmos(ProgramName, ProgramName);
    xinch = OlPointToPixel(OL_HORIZONTAL,72);
    yinch = OlPointToPixel(OL_VERTICAL,72);
    def_font = _OlGetDefaultFont(w_toplevel, OlDefaultFont);

    base.title = GetGizmoText(base.title);
    base.icon_name = GetGizmoText(base.icon_name);
    w_baseshell =
	CreateGizmo(w_toplevel, BaseWindowGizmoClass, &base, NULL, 0); 
	
    XtVaSetValues(base.scroller,
		  XtNwidth,	(XtArgVal)WIDTH,
		  XtNheight,	(XtArgVal)HEIGHT,
		  NULL);

    XtSetArg(arg[0], XtNtitle, GetGizmoText(string_userBaseLine));    
    XtSetArg(arg[1], XtNmappedWhenManaged, (XtArgVal) False);
    XtSetValues(w_baseshell, arg, 2);

    /* if anothe copy of this program running on this system is */
    /* running on this display then pop it to the top and exit  */

    XtRealizeWidget(w_baseshell);
    theScreen  = XtScreen(w_baseshell);
    theDisplay = XtDisplay(w_baseshell);
    if (uname(&name) >0)
    {
	strcat(atom, ":");
	strcat(atom, name.nodename);
    }
    another_window = DtSetAppId(theDisplay, XtWindow(w_baseshell), atom);
    if (another_window != None)
    {    
	XMapWindow(theDisplay, another_window);
	XRaiseWindow(theDisplay, another_window);
	XFlush(theDisplay);
	exit(0);
    }
    XtVaSetValues (w_baseshell, XtNmappedWhenManaged, (XtArgVal) True, 0);

    MakeUserList();
    MakeGroupList();

    CreatePropSheet();
    CreatePermSheet();
    CreateGroupProp();
    /*
     *	create base window icon box with logins
     */
    u_fcrec.glyph = DmGetPixmap(theScreen, "login.glyph");
    u_cntrec.count = 1;
    w_iconbox = GetIconBox(base.scroller);
    g_fcrec.glyph = DmGetPixmap(theScreen, "group.glyph");
    g_cntrec.count = 1;

    MapGizmo(BaseWindowGizmoClass, &base);
    XtMainLoop();
}

/* remove leading and trailing whitespace without moving the pointer */
/* so that the pointer may still be free'd later.                    */
/* returns True if the string was modified; False otherwise          */

Boolean
removeWhitespace(char * string)
{
    register char *ptr = string;
    size_t   len;
    Boolean  changed = False;

    if (string == NULL)
	return False;

    while (isspace(*ptr))
    {
	ptr++;
	changed = True;
    }
    if ((len = strlen(ptr)) == 0)
    {
	*string = EOS;
	return changed;
    }

    if (changed)
	(void)memmove((void *)string, (void *)ptr, len+1); /* +1 to */
							   /* move EOS */
    ptr = string + len - 1;    /* last character before EOS */
    while (isspace(*ptr))
    {
	ptr--;
	changed = True;
    }
    *(++ptr) = EOS;
    
    return changed;
}

static void
hideExtraCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlVaFlatSetValues(w_extra, 0, XtNset, (XtArgVal)FALSE, NULL);
    XtUnmanageChildren(w_extctls, XtNumber(w_extctls));
}

static void 
busyCursor(int popup_type)
{
    Display *disp;
    Screen  *screen;

    switch (popup_type)	
    {
    case GROUPS:
	if (!XtIsRealized(w_gpopup))
	    return;
        XDefineCursor((disp = XtDisplay(w_gpopup)), XtWindow(w_gpopup),
                      GetOlBusyCursor(screen=XtScreen(w_gpopup)));
        XDefineCursor(disp, XtWindow(w_gname), GetOlBusyCursor(screen));
        XDefineCursor(disp, XtWindow(w_gid),   GetOlBusyCursor(screen));
	break;
    case USERS:
	/* FALL THRU */
    case SYSLOG:
	if (!XtIsRealized(w_popup))
	    return;
	XDefineCursor((disp = XtDisplay(w_popup)), XtWindow(w_popup),
		      GetOlBusyCursor(screen=XtScreen(w_popup))); 
	XDefineCursor(disp, XtWindow(w_login),  GetOlBusyCursor(screen)); 
	XDefineCursor(disp, XtWindow(w_desc),   GetOlBusyCursor(screen));
	XDefineCursor(disp, XtWindow(w_home),   GetOlBusyCursor(screen));
	XDefineCursor(disp, XtWindow(w_remote), GetOlBusyCursor(screen));
	XDefineCursor(disp, XtWindow(w_shell),  GetOlBusyCursor(screen));
	XDefineCursor(disp, XtWindow(w_uid),    GetOlBusyCursor(screen));
	break;
    }
}

static void
standardCursor(int popup_type)
{
    Display *disp;
    Screen  *screen;

    switch (popup_type)	
    {
    case GROUPS:
	if (!XtIsRealized(w_gpopup))
	    return;
        XDefineCursor((disp = XtDisplay(w_gpopup)), XtWindow(w_gpopup),
                      GetOlStandardCursor(screen=XtScreen(w_gpopup)));
        XDefineCursor(disp, XtWindow(w_gname), GetOlStandardCursor(screen));
        XDefineCursor(disp, XtWindow(w_gid),   GetOlStandardCursor(screen));
	break;
    case USERS:
	/* FALL THRU */
    case SYSLOG:
	if (!XtIsRealized(w_popup))
	    return;
	XDefineCursor((disp = XtDisplay(w_popup)), XtWindow(w_popup),
		      GetOlStandardCursor(screen=XtScreen(w_popup))); 
	XDefineCursor(disp, XtWindow(w_login),  GetOlStandardCursor(screen)); 
	XDefineCursor(disp, XtWindow(w_desc),   GetOlStandardCursor(screen));
	XDefineCursor(disp, XtWindow(w_home),   GetOlStandardCursor(screen));
	XDefineCursor(disp, XtWindow(w_remote), GetOlStandardCursor(screen));
	XDefineCursor(disp, XtWindow(w_shell),  GetOlStandardCursor(screen));
	XDefineCursor(disp, XtWindow(w_uid),    GetOlStandardCursor(screen));
	break;
    }
}

int	cmpuid(int *m, int *n)
{
	return *m - *n;
}

int	cmplogin(UserPtr x, UserPtr y)
{
	return strcoll(x->pw_name, y->pw_name);
}

int	cmpgroup(GroupPtr x, GroupPtr y)
{
	return strcoll(x->g_name, y->g_name);
}

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xdm:dtlogin.c	1.27"
/*
 *	dtlogin - OPEN LOOK interface to xdm
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <libgen.h>
#include <pwd.h>
#include <grp.h>
#include <shadow.h>
#include <ia.h>
#include <limits.h>
#include <sys/utsname.h>
#include <signal.h>

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/Xlib.h>
#include <X11/Vendor.h>
#include <X11/RectObj.h>

#include <Xol/OpenLookP.h>
#include <Xol/Dynamic.h>
#include <Xol/MenuShell.h>
#include <Xol/PopupWindo.h>
#include <Xol/BaseWindow.h>
#include <Xol/ControlAre.h>
#include <Xol/BulletinBo.h>
#include <Xol/RubberTile.h>
#include <Xol/AbbrevButt.h>
#include <Xol/Flat.h>
#include <Xol/FButtons.h>
#include <Xol/FList.h>
#include <Xol/StaticText.h>
#include <Xol/TextField.h>
#include <Xol/Caption.h>
#include <Xol/FooterPane.h>
#include <Xol/Notice.h>
#include <Xol/ScrolledWi.h>
#include <Xol/buffutil.h>
#include <Xol/textbuff.h>
#include <Xol/OlCursors.h>

#include <libDtI/DtI.h>
#include <Gizmo/Gizmos.h>

#include "dm.h"
#include "dtlogin.h"

#define	GGT	GetGizmoText

extern	char	*getenv();
#ifndef	STANDALONE_DT
extern	StopDisplay ();
extern	ForEachDisplay ();
extern	StopDisplay ();
extern	SIGVAL	StopAll();
extern	void	logCB ();
extern	int	GetPasswdMinLen ();
extern	char	*pidFile;
extern	int	exitSession;
extern	int	StorePasswd ();
extern	int	passwdStored;
extern	int	pwNeedDone;
extern	int	pwSuccess;
extern	int	pwMaxTrys;
extern	int	minlength;
extern	char	newPassword[128];
extern	char	*errStr;
extern	int	loginProblem;
#endif
extern	struct	passwd	*getpwent();
extern	struct	group	*getgrent(), *getgrnam(), *getgrgid();

#define	LOGIN_TEXT	1
#define	AUTH_TEXT	2
static	Boolean		setFocus = FALSE;
static	int		whichText = 0;

static	Widget		GetIconBox();

static	Widget		w_toplevel, w_topmsg, w_canvas, w_buts;
static	Widget		w_passpopup, w_authpass, w_reauthpass, w_chgfail;
static	Widget		w_login_text = NULL;

static	int		w_type = 0;
static	char		badHome[512];

void	AccountAging (), AccountAged (), PasswdAged ();

typedef	struct	{ char	*	label;
		  XtArgVal	mnem;
		  Boolean	sensitive;
		  XtArgVal	selCB;
		  XtPointer	cdata;
} Items;

static	Items	CBar[4];

#define	N_FIELDS	5

static	char    *Fields[]   = { XtNlabel, XtNmnemonic, XtNsensitive,
			XtNselectProc, XtNclientData };

#define	SET_BTN(id,n,name,data) \
				id[n].label = GGT(label##_##name);\
				id[n].mnem = (XtArgVal)*GGT(mnemonic##_##name);\
				id[n].sensitive = TRUE;\
				id[n].selCB = (XtArgVal)name##CB; \
				id[n].cdata = (XtPointer)data;

typedef	struct	passwd	UserRec, *UserPtr;

static	char	*AGE_UID	= "/etc/security/ia/ageduid";

static	UserPtr	u_list = (UserPtr)0;
static	UserPtr	u_reset;
static	int	u_cnt = 0;
static	int	*uid_list;

static	int	maxTrys = 0;

static	int		item_count = 0;

static	DmItemPtr	u_itp;
static	DmFclassRec	u_fcrec;
static	DmContainerRec	u_cntrec;

static	Dimension	xinch, yinch;

static	Boolean	canPopdown = FALSE;

#define WIDTH	(6*xinch)
#define	HEIGHT	(4*yinch)
#define	WIDTH2	(4*xinch)
#define	HEIGHT2	(3*yinch)

#define INIT_X  35
#define INIT_Y  20
#define INC_X   70
#define INC_Y   70
#define	MARGIN	20

Display		*theDisplay;
Screen		*theScreen;

static	Dimension	ibx = INIT_X, iby = INIT_Y;

#ifdef	STANDALONE_DT
static	void
Debug (char *s)
	{
	fprintf (stderr, "%s\n", s);
	return;
	}
#endif

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

int	cmpuid(int *m, int *n)
{
	return *m - *n;
}

int	cmplogin(UserPtr x, UserPtr y)
{
	return strcoll(x->pw_name, y->pw_name);
}


#define	BUNCH	16
#define	U_QUANT	(BUNCH*sizeof(UserRec))

static	void
MakeUserList(void)
{
struct	passwd	*pwd;
	FILE	*fp;
	UserPtr	up;
	char	buf[40];
	int	n;

	if (u_list)
		FreeUserList();
	while (pwd = getpwent()) {
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
		qsort((void *)uid_list, n, sizeof(int), cmpuid);
	}
	qsort((void *)u_list, u_cnt, sizeof(UserRec), cmplogin);
}

void
ShellFail ()
	{
	loginProblem = BADSHELL;
	return;
	}

void
NoHome ()
	{
	loginProblem = NOHOME;
	return;
	}

void
SensitiseLogin (value)
Boolean value;
{
	Arg	args[1];

	XtSetArg(args[0], XtNsensitive, value);
	OlFlatSetValues(w_buts, 0, args, 1);
	return;
}

static	void
AdjustUser(w, client_data, call_data)
	Widget		w;
	XtPointer	client_data;
	XtPointer	call_data;
{
	int		this, last;
	OlFlatCallData	*d = (OlFlatCallData *)call_data;

	XtVaGetValues(w, XtNlastSelectItem, &last, NULL);
	if ((this=d->item_index) == last) {
		DmIconAdjustProc(w, client_data, call_data);
		XtVaSetValues(w, XtNselectCount, (XtArgVal)0, NULL);
	}
	else {
		DmIconSelect1Proc(w, client_data, call_data);
		XtVaSetValues(w, XtNselectCount,	(XtArgVal)1,
				 XtNlastSelectItem,	(XtArgVal)this,
				 NULL);
	}
}

static	void
SingleClick(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmObjectPtr	op;
	Login_info	*info = (Login_info *)client_data;
	OlFlatCallData	*d = (OlFlatCallData *)call_data;
	Time		t = CurrentTime;

	DmIconSelect1Proc(w, client_data, call_data);
	OlVaFlatGetValues(w, d->item_index, XtNobjectData, &op, NULL);

	/* here is where we set the login text field and sensitize the */
	/* password text field */

	OlTextEditClearBuffer (info->login_text);
	OlTextEditClearBuffer (info->password_text);
	XtVaSetValues (info->login_fail, XtNstring, 
		(XtArgVal)GGT(string_noecho), NULL);
	SensitiseLogin (TRUE);
	u_reset = (UserPtr)op->objectdata;
	Debug (u_reset->pw_name);
	OlTextEditInsert (info->login_text, (String)u_reset->pw_name,
				strlen(u_reset->pw_name));
	XtSetSensitive (info->password_text, TRUE);
	t = CurrentTime;
	if (!(XtCallAcceptFocus (info->password_text, &t)))
		Debug ("Could not accept input focus\n");
	return;
}

static	void
okCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	Login_info	*info = (Login_info *)client_data;
	Time		t = CurrentTime;

	if (!(XtCallAcceptFocus (info->login_text, &t)))
		Debug ("Could not accept input focus\n");
	return;
}

static	void
ok1CB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	Time		t = CurrentTime;

	if (!(XtCallAcceptFocus (w_authpass, &t)))
		Debug ("Could not accept input focus\n");
	return;
}

static	void
PassVerifyCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	OlTextFieldVerify	*verify = (OlTextFieldVerify *)call_data;
	Time			t = CurrentTime;

	switch (verify->reason)
		{
		case OlTextFieldReturn:	
			XtVaSetValues (w_chgfail, XtNstring,
				(XtArgVal)GGT(string_noecho), NULL);
			XtSetSensitive (w_reauthpass, TRUE);
			if (!(XtCallAcceptFocus (w_reauthpass, &t)))
				Debug ("Could not accept input focus\n");
			break;
		default:	break;
		}
	return;
}


static	void
RePassVerifyCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	char			*user = (char *)client_data;
	OlTextFieldVerify	*verify = (OlTextFieldVerify *)call_data;
	char			*pass1, buf[50];
	int			len = 0, err = 0;
	Time			t = CurrentTime;

	maxTrys++;
	switch (verify->reason)
		{
		case OlTextFieldReturn:	
			pass1 = OlTextFieldGetString (w_authpass, &len);
			if (!(strcmp (pass1, verify->string)))
				{
				switch (err = StorePasswd (user, pass1)) 
					{
					case TOO_SHORT:
					    sprintf (buf, "%s%d%s", GGT(string_tshort1), GetPasswdMinLen (), GGT(string_tshort2));
						XtVaSetValues (w_chgfail,
							XtNstring,
							(String)buf, NULL);
						break;
					case CIRC:
						XtVaSetValues (w_chgfail,
							XtNstring,
							(String)GGT(string_nocirc), NULL);
						break;
					case SPECIAL_CHARS:
						XtVaSetValues (w_chgfail,
							XtNstring,
							(String)GGT(string_schar), NULL);
						break;
					case DF3CHARS:
						XtVaSetValues (w_chgfail,
							XtNstring,
							(String)GGT(string_d3fpos), NULL);
						break;
					default:
						break;
					}
				if (err) 
					{
					if (len)
						free (pass1);
					XBell (theDisplay, 0);
					OlTextEditClearBuffer (w_authpass);
					OlTextEditClearBuffer (w_reauthpass);
					if (maxTrys == PW_MAXTRYS)
						{
						strcpy (errStr, GGT(string_maxtrys));
						XtPopdown (w_passpopup);
						canPopdown = TRUE;
						XtPopdown (w_passpopup);
						pwSuccess = 0;
						pwNeedDone = 1;
						pwMaxTrys = PW_MAXTRYS;
						}
					else
						{
						canPopdown = FALSE;
						}
					/*XtSetSensitive (w_reauthpass, FALSE);*/
					t = XtLastTimestampProcessed (theDisplay);
					if (!(XtCallAcceptFocus (w_authpass,
						&t)))
						Debug("Could not accept Input focus\n");
					}
				else 
					{
					XtVaSetValues (w_chgfail, XtNstring, (String)"",
							NULL);
					if (len)
						free (pass1);
					canPopdown = TRUE;
					pwNeedDone = 1;
					pwSuccess = 1;
					passwdStored = 1;
					(void) strcpy (newPassword, pass1);
					XtPopdown (w_passpopup);
					}
				}
			else
				{
				if (len)
					free (pass1);
				XBell (theDisplay, 0);
				XtVaSetValues (w_chgfail, XtNstring,
					(XtArgVal)GGT(string_nomatch), NULL);
				OlTextEditClearBuffer (w_authpass);
				OlTextEditClearBuffer (w_reauthpass);
				if (maxTrys == PW_MAXTRYS)
					{
					strcpy (errStr, GGT(string_maxtrys));
					XtPopdown (w_passpopup);
					canPopdown = TRUE;
					XtPopdown (w_passpopup);
					pwSuccess = 0;
					pwNeedDone = 1;
					pwMaxTrys = PW_MAXTRYS;
					}
				else
					{
					canPopdown = FALSE;
					}
				/*XtSetSensitive (w_reauthpass, FALSE);*/
				t = XtLastTimestampProcessed (theDisplay);
				if (!(XtCallAcceptFocus(w_authpass, &t)))
					Debug("Could not accept Input Focus\n");
				}
			break;
		default:	break;
		}
	return;
}

static void
pass_okCB (Widget w, XtPointer client_data, XtPointer call_data)
	{
	char	*user = (char *)client_data;
	int	len1 = 0, len2 = 0, err = 0;
	char	*pass1, *pass2, buf[50];
	Time	t = CurrentTime;

	maxTrys++;
	Debug ("maxTrys = %d\n", maxTrys);
	pass1 = OlTextFieldGetString (w_authpass, &len1);
	if (!len1)
		{
		XBell (theDisplay, 0);
		XtVaSetValues (w_chgfail, XtNstring,
			(XtArgVal)GGT(string_nopass), NULL);
		t =XtLastTimestampProcessed(theDisplay);
		if (!(XtCallAcceptFocus (w_authpass, &t)))
			Debug("Could not accept input focus\n");
		XtSetSensitive (w_reauthpass, FALSE);
		if (maxTrys == PW_MAXTRYS)
			{
			strcpy (errStr, GGT(string_maxtrys));
			pwMaxTrys = PW_MAXTRYS;
			pwSuccess = 0;
			canPopdown = TRUE;
			pwNeedDone = 1;
			XtPopdown (w_passpopup);
			}
		else
			{
			canPopdown = FALSE;
			}
		}
	else
		{
		pass2 = OlTextFieldGetString (w_reauthpass, &len2);
		if (strcmp (pass1, pass2))
			{
			XBell (theDisplay, 0);
			XtVaSetValues (w_chgfail, XtNstring,
				(XtArgVal)GGT(string_nomatch), NULL);
			if (len1)
				free (pass1);
			if (len2)
				free (pass2);
			OlTextEditClearBuffer (w_authpass);
			OlTextEditClearBuffer (w_reauthpass);
			t =XtLastTimestampProcessed(theDisplay);
			if (!(XtCallAcceptFocus (w_authpass, &t)))
				Debug("Could not accept input focus\n");
			XtSetSensitive (w_reauthpass, FALSE);
			if (maxTrys == PW_MAXTRYS)
				{
				strcpy (errStr, GGT(string_maxtrys));
				canPopdown = TRUE;
				pwSuccess = 0;
				pwNeedDone = 1;
				pwMaxTrys = PW_MAXTRYS;
				XtPopdown (w_passpopup);
				}
			else
				{
				canPopdown = FALSE;
				}
			}
		else 
			{
			switch (err = StorePasswd (user, pass1)) 
				{
				case TOO_SHORT:
				    sprintf (buf, "%s%d%s", GGT(string_tshort1), GetPasswdMinLen (), GGT(string_tshort2));
				    XtVaSetValues (w_chgfail, XtNstring,
					(String)buf, NULL);
				    break;
				case CIRC:
				    XtVaSetValues (w_chgfail, XtNstring,
					(String)GGT(string_nocirc), NULL);
				    break;
				case SPECIAL_CHARS:
				    XtVaSetValues (w_chgfail, XtNstring,
					(String)GGT(string_schar), NULL);
				    break;
				case DF3CHARS:
				   XtVaSetValues (w_chgfail, XtNstring,
					(String)GGT(string_d3fpos), NULL);
				    break;
				default:
				    break;
				}
			if (err) 
				{
				if (len1)
					free (pass1);
				XBell (theDisplay, 0);
				OlTextEditClearBuffer (w_authpass);
				OlTextEditClearBuffer (w_reauthpass);
				t =XtLastTimestampProcessed(theDisplay);
				if (!(XtCallAcceptFocus (w_authpass, &t)))
					Debug("Could not accept input focus\n");
				XtSetSensitive (w_reauthpass, FALSE);
				if (maxTrys == PW_MAXTRYS)
					{
					strcpy (errStr, GGT(string_maxtrys));
					XtPopdown (w_passpopup);
					canPopdown = TRUE;
					XtPopdown (w_passpopup);
					pwSuccess = 0;
					pwNeedDone = 1;
					pwMaxTrys = PW_MAXTRYS;
					}
				else
					canPopdown = FALSE;
				}
			else 
				{
				canPopdown = TRUE;
				passwdStored = 1;
				(void) strcpy (newPassword, pass1);
				pwSuccess = 1;
				pwNeedDone = 1;
				if (len1)
					{
					free (pass1);
					free (pass2);
					}
				XtPopdown (w_passpopup);
				}
			}
		}
	return;
	}

static void
pass_resetCB (Widget w, XtPointer client_data, XtPointer call_data)
	{
	Time	t;

	OlTextEditClearBuffer (w_authpass);
	OlTextEditClearBuffer (w_reauthpass);
	XtVaSetValues (w_chgfail, XtNstring, (String)"", NULL);
	t = CurrentTime;
	XtSetSensitive (w_reauthpass, FALSE);
	if (!(XtCallAcceptFocus (w_authpass, &t)))
		Debug ("Could not set input focus\n");
	canPopdown = FALSE;
	return;
	}


static void
pass_verifyCB (Widget w, XtPointer client_data, XtPointer call_data)
	{
	Boolean	*popdown	= (Boolean *)call_data;

	if (canPopdown)
		*popdown = TRUE;
	else
		*popdown = FALSE;
	return;
	}

static void
pass_helpCB (Widget w, XtPointer client_data, XtPointer call_data)
	{
	Login_info	*info = (Login_info *)client_data;
	Widget		popup, control, text, but;
	int		width, height;
	static		Items	PHOk[1];

	popup = XtVaCreatePopupShell ("PassHelp", noticeShellWidgetClass,
				w_toplevel,
				XtNx,		(XtArgVal)20,
				XtNy,		(XtArgVal)20,
				XtNnoticeType,	(XtArgVal)OL_INFORMATION,
				NULL);

	XtVaGetValues (popup, XtNcontrolArea, &control,
			XtNtextArea, &text, NULL);

	if (w_type == PASSWD_NEEDED)
		XtVaSetValues (text, XtNalignment, (XtArgVal)OL_LEFT,
			XtNstring,	(XtArgVal)GGT(string_phelp1),
			NULL);
	else
		XtVaSetValues (text, XtNalignment, (XtArgVal)OL_LEFT,
			XtNstring,	(XtArgVal)GGT(string_phelp2),
			NULL);

	SET_BTN (PHOk, 0, ok1, NULL);
	but = XtVaCreateManagedWidget("PHOk", flatButtonsWidgetClass,
			control,
			XtNalignment,		(XtArgVal)OL_CENTER,
			XtNitemFields,		(XtArgVal)Fields,
			XtNnumItemFields,	(XtArgVal)N_FIELDS,
			XtNitems,		(XtArgVal)PHOk,
			XtNnumItems,		(XtArgVal)1,
			XtNlabelJustify,	(XtArgVal)OL_CENTER,
			NULL);

	XtPopup (popup, XtGrabNone);

	if (!(OlFlatCallAcceptFocus (but, 0, CurrentTime)))
		{
		Debug ("Could not set Input Focus\n");
		}

	XDefineCursor (XtDisplay(popup), XtWindow(popup),
		GetOlStandardCursor (XtScreen(popup)));

	return;
	}

void
PopupPasshelp (w, info)
Widget	w;
Login_info	*info;
	{
	pass_helpCB (w, (XtPointer)info, NULL);
	return;
	}

void
NewPasswd (char *user, int type)
	{
	Pixel		back;
	Widget		upper, lower;
	Widget		w_title, msg, caption, recaption;
	Widget		cbuts;
	Time		t = CurrentTime;
	static Items	buttons[3];
	XtCallbackRec	pass_verify[] =
		{
		{pass_verifyCB, NULL},
		{NULL, NULL},
		};

	maxTrys = 0;
	canPopdown = FALSE;

	w_passpopup = XtVaCreatePopupShell ("NewPasswd",
				popupWindowShellWidgetClass, 
				w_toplevel,
				XtNfocusWidget, (XtArgVal)w_authpass,
				XtNverify, (XtArgVal)pass_verify,
				XtNx, (XtArgVal)20,
				XtNy, (XtArgVal)20,
				XtNshadowThickness, 2,
				XtNalignment, (XtArgVal)OL_CENTER,
				NULL);

	XtVaGetValues (w_passpopup, XtNupperControlArea, &upper, 
			XtNlowerControlArea, &lower, NULL);

	XtVaSetValues (XtParent (upper), XtNshadowThickness,
		(XtArgVal)2, NULL);
	XtVaSetValues (upper, XtNalignment, (XtArgVal)OL_CENTER, NULL);
	XtVaSetValues (lower, XtNlayoutType,  (XtArgVal)OL_FIXEDCOLS, NULL);

	w_type = type;
	if (type == MANDATORY)
		{
		w_title = XtVaCreateManagedWidget ("title",
			staticTextWidgetClass, upper,
			XtNgravity, CenterGravity,
			XtNalignment, OL_CENTER,
			XtNstring, (XtArgVal)GGT(string_passneed),
			NULL);

		msg = XtVaCreateManagedWidget ("pass_needed_msg",
			staticTextWidgetClass, upper,
			XtNgravity, CenterGravity,
			XtNalignment, OL_CENTER,
			XtNstring, (XtArgVal)GGT(string_passwd_needed),
			NULL);
		}
	else if (type == AGED)
		{
		w_title = XtVaCreateManagedWidget ("title",
			staticTextWidgetClass, upper,
			XtNgravity, CenterGravity,
			XtNalignment, OL_CENTER,
			XtNstring, (XtArgVal)GGT(string_expire),
			NULL);

		msg = XtVaCreateManagedWidget ("pass_expired_msg",
			staticTextWidgetClass, upper,
			XtNgravity, CenterGravity,
			XtNalignment, OL_CENTER,
			XtNstring, (XtArgVal)GGT(string_pass_expired),
			NULL);
		}
	else
		{
		w_title = XtVaCreateManagedWidget ("title",
			staticTextWidgetClass, upper,
			XtNgravity, CenterGravity,
			XtNalignment, OL_CENTER,
			XtNstring, (XtArgVal)GGT(string_pflagt),
			NULL);

		msg = XtVaCreateManagedWidget ("pass_pflag_msg",
			staticTextWidgetClass, upper,
			XtNgravity, CenterGravity,
			XtNalignment, OL_CENTER,
			XtNstring, (XtArgVal)GGT(string_pflag),
			NULL);
		}

	caption = XtVaCreateManagedWidget ("verify_caption",
			captionWidgetClass, upper,
			XtNlabel, (XtArgVal)GGT(label_authPass),
			NULL);

	w_authpass = XtVaCreateManagedWidget ("auth_text", 
			textFieldWidgetClass, caption,
			XtNcharsVisible, (XtArgVal)20,
			NULL);
	XtAddCallback (w_authpass, XtNverification, PassVerifyCB, NULL);

	recaption = XtVaCreateManagedWidget ("reverify_caption",
			captionWidgetClass, upper,
			XtNlabel, (XtArgVal)GGT(label_reauthPass),
			NULL);

	w_reauthpass = XtVaCreateManagedWidget ("reauth_text", 
			textFieldWidgetClass, recaption,
			XtNsensitive, (XtArgVal)FALSE,
 			XtNcharsVisible, (XtArgVal)20,
			NULL);
	XtAddCallback (w_reauthpass, XtNverification, RePassVerifyCB,
			(XtPointer)user);
	XtVaGetValues (w_authpass, XtNbackground, &back, NULL);
	XtVaSetValues (w_authpass, XtNforeground, back,
			XtNfontColor, back, NULL);
	XtVaSetValues (w_reauthpass, XtNforeground, back,
			XtNfontColor, back, NULL);

	buttons[0].label = GGT(label_ok);
	buttons[0].mnem = (XtArgVal)*GGT(mnemonic_ok);
	buttons[0].sensitive = TRUE;
	buttons[0].selCB = (XtArgVal)pass_okCB;
	buttons[0].cdata = (XtPointer)user;

	buttons[1].label = GGT(label_reset);
	buttons[1].mnem = (XtArgVal)*GGT(mnemonic_reset);
	buttons[1].sensitive = TRUE;
	buttons[1].selCB = (XtArgVal)pass_resetCB;
	buttons[1].cdata = NULL;

	buttons[2].label = GGT(label_help);
	buttons[2].mnem = (XtArgVal)*GGT(mnemonic_help);
	buttons[2].sensitive = TRUE;
	buttons[2].selCB = (XtArgVal)pass_helpCB;
	buttons[2].cdata = NULL;

	cbuts = XtVaCreateManagedWidget("cbuts", flatButtonsWidgetClass,
			lower,
			XtNalignment,		(XtArgVal)OL_CENTER,
			XtNitemFields,		(XtArgVal)Fields,
			XtNnumItemFields,	(XtArgVal)N_FIELDS,
			XtNitems,		(XtArgVal)buttons,
			XtNnumItems,		(XtArgVal)3,
			XtNlabelJustify,	(XtArgVal)OL_CENTER,
			NULL);

	w_chgfail = XtVaCreateManagedWidget ("passwd_fail",
			staticTextWidgetClass, lower,
			XtNgravity, CenterGravity,
			XtNalignment, OL_CENTER,
			NULL);

	XtPopup (w_passpopup, XtGrabNone);

	XDefineCursor (XtDisplay(w_passpopup), XtWindow(w_passpopup),
		GetOlStandardCursor (XtScreen(w_passpopup)));

	if (!(XtCallAcceptFocus (w_authpass, &t)))
		Debug("Could not accept input focus\n");
	return;
	}

void
AccountAged (info)
Login_info	*info;
	{
	/* Do a notice popup to tell user that his account has aged and
	 * to take care of it.
	 */
	Widget	popup, control, text, but;
	int	width, height;
	static	Items AAOk[1];

	popup = XtVaCreatePopupShell ("AccountAged", noticeShellWidgetClass,
				w_toplevel,
				XtNnoticeType,	(XtArgVal)OL_WARNING,
				XtNx,		(XtArgVal)20,
				XtNy,		(XtArgVal)20,
				NULL);

	XtVaGetValues (popup, XtNcontrolArea, &control,
			XtNtextArea, &text, NULL);

	XtVaSetValues (text, XtNalignment, (XtArgVal)OL_CENTER,
			XtNstring,	(XtArgVal)GGT(string_acct),
			NULL);

	SET_BTN (AAOk, 0, ok, info);
	but = XtVaCreateManagedWidget("AAOk", flatButtonsWidgetClass,
			control,
			XtNalignment,		(XtArgVal)OL_CENTER,
			XtNitemFields,		(XtArgVal)Fields,
			XtNnumItemFields,	(XtArgVal)N_FIELDS,
			XtNitems,		(XtArgVal)AAOk,
			XtNnumItems,		(XtArgVal)1,
			XtNlabelJustify,	(XtArgVal)OL_CENTER,
			NULL);

	XtPopup (popup, XtGrabNone);

	XDefineCursor (XtDisplay(popup), XtWindow(popup),
		GetOlStandardCursor (XtScreen(popup)));

	if (!(OlFlatCallAcceptFocus (but, 0, CurrentTime)))
		{
		Debug ("Could not set Input Focus\n");
		}

	return;
	}

void
PasswdAged (info)
Login_info	*info;
	{
	/* Do a popup notice to tell user that his password has aged for
	 * to long and to seek help.
	 */
	Widget	popup, control, text, but;
	int	width, height;
	static	Items PAOk[1];

	popup = XtVaCreatePopupShell ("PasswordAged", noticeShellWidgetClass,
				w_toplevel,
				XtNnoticeType,	(XtArgVal)OL_WARNING,
				XtNx,		(XtArgVal)20,
				XtNy,		(XtArgVal)20,
				NULL);

	XtVaGetValues (popup, XtNcontrolArea, &control,
			XtNtextArea, &text, NULL);

	XtVaSetValues (text, XtNalignment, (XtArgVal)OL_CENTER,
			XtNstring,	(XtArgVal)GGT(string_pass_time),
			NULL);

	SET_BTN (PAOk, 0, ok, info);
	but = XtVaCreateManagedWidget("PAOk", flatButtonsWidgetClass,
			control,
			XtNalignment,		(XtArgVal)OL_CENTER,
			XtNitemFields,		(XtArgVal)Fields,
			XtNnumItemFields,	(XtArgVal)N_FIELDS,
			XtNitems,		(XtArgVal)PAOk,
			XtNnumItems,		(XtArgVal)1,
			XtNlabelJustify,	(XtArgVal)OL_CENTER,
			NULL);

	XtPopup (popup, XtGrabNone);

	XDefineCursor (XtDisplay(popup), XtWindow(popup),
		GetOlStandardCursor (XtScreen(popup)));

	if (!(OlFlatCallAcceptFocus (but, 0, CurrentTime)))
		{
		Debug ("Could not set Input Focus\n");
		}

	return;
	}


#ifdef	STANDALONE_DT
static	void
logCB (Widget w, XtPointer client_data, XtPointer call_data)
	{
	Login_info	*info = (Login_info *)client_data;

	Debug (OlTextFieldGetString (info->login_text, NULL));
	Debug (OlTextFieldGetString (info->password_text, NULL));
	return;
	}
#endif

static	void
resetCB(Widget w, XtPointer client_data, XtPointer call_data)
	{
	Login_info	*info = (Login_info *)client_data;
	Arg		args[5];
	Time		t;

	XtSetArg(args[0], XtNstring, (String)"");
	XtSetValues (info->login_fail, args, 1);

	OlTextEditClearBuffer (info->login_text);
	OlTextEditClearBuffer (info->password_text);

	SensitiseLogin (FALSE);
	XtSetSensitive (info->password_text, FALSE);
	t = CurrentTime;
	if (!(XtCallAcceptFocus (info->login_text, &t)))
		Debug ("Could not set input focus\n");

	return;
	}

static void
exitCB (Widget w, XtPointer client_data, XtPointer call_data)
	{
	exit (EXIT_XDM);
	}

static void
helpCB (Widget w, XtPointer client_data, XtPointer call_data)
	{
	Login_info	*info = (Login_info *)client_data;
	Widget		popup, control, text, but;
	int		width, height;
	static		Items	HOk[1];

	popup = XtVaCreatePopupShell ("Help", noticeShellWidgetClass,
				w_toplevel,
				XtNx,		(XtArgVal)20,
				XtNy,		(XtArgVal)20,
				XtNnoticeType,	(XtArgVal)OL_INFORMATION,
				NULL);

	XtVaGetValues (popup, XtNcontrolArea, &control,
			XtNtextArea, &text, NULL);

	XtVaSetValues (text, XtNalignment, (XtArgVal)OL_LEFT,
			XtNstring,	(XtArgVal)GGT(string_help),
			NULL);

	SET_BTN (HOk, 0, ok, info);

	but = XtVaCreateManagedWidget("HOk", flatButtonsWidgetClass,
			control,
			XtNalignment,		(XtArgVal)OL_CENTER,
			XtNitemFields,		(XtArgVal)Fields,
			XtNnumItemFields,	(XtArgVal)N_FIELDS,
			XtNitems,		(XtArgVal)HOk,
			XtNnumItems,		(XtArgVal)1,
			XtNlabelJustify,	(XtArgVal)OL_CENTER,
			NULL);

	XtPopup (popup, XtGrabNone);

	if (!(OlFlatCallAcceptFocus (but, 0, CurrentTime)))
		{
		Debug ("Could not accept Input Focus\n");
		}

	XDefineCursor (XtDisplay(popup), XtWindow(popup),
		GetOlStandardCursor (XtScreen(popup)));

	return;
	}

void
PopupHelp (w, info)
Widget		w;
Login_info	*info;
	{
	helpCB (w, info, NULL);
	}

static	void
#ifdef STANDALONE_DT
CreateControlBar(parent, info)
Widget		parent;
Login_info	*info;
#else
CreateControlBar(parent, info, d)
Widget		parent;
Login_info	*info;
struct display	*d;
#endif
{
	Widget		bar, spacer;

#ifdef	STANDALONE_DT
	SET_BTN(CBar, 0, log, info);
#else
	SET_BTN(CBar, 0, log, d);
#endif
	SET_BTN(CBar, 1, reset, info);
	SET_BTN(CBar, 2, exit, NULL);
	SET_BTN(CBar, 3, help, info);

	CBar[0].sensitive = FALSE;

	bar = XtVaCreateManagedWidget ("controlbar", controlAreaWidgetClass,
			parent,
			XtNalignment,		(XtArgVal)OL_CENTER,
			XtNshadowThickness,	(XtArgVal)0,
			XtNcenter,		(XtArgVal)TRUE,
			XtNlayoutType,		(XtArgVal)OL_FIXEDWIDTH,
			NULL);

	w_buts = XtVaCreateManagedWidget("menuitems", flatButtonsWidgetClass,
			bar,
			XtNalignment,		(XtArgVal)OL_CENTER,
			XtNitemFields,		(XtArgVal)Fields,
			XtNnumItemFields,	(XtArgVal)N_FIELDS,
			XtNitems,		(XtArgVal)CBar,
			XtNnumItems,		(XtArgVal)4,
			XtNlabelJustify,	(XtArgVal)OL_CENTER,
			XtNsameWidth,		(XtArgVal)OL_ALL,
			XtNfocusOnSelect,	(XtArgVal)FALSE,
			XtNhSpace,		(XtArgVal)35,
			NULL);
	return;
}

static	DmObjectPtr
AddUserItem(UserPtr p)
{
	DmObjectPtr	optr;

	optr = (DmObjectPtr)calloc(1, sizeof(DmObjectRec));
	optr->container = &u_cntrec;
	optr->fcp = &u_fcrec;
	optr->name = p->pw_name;
	optr->x = ibx;
	optr->y = iby;
	optr->objectdata = (XtPointer)p;
	if ((int)(ibx += INC_X) > (int)(WIDTH - MARGIN)) {
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

static	Widget
#ifdef	STANDALONE_DT
GetIconBox(Widget parent, Login_info *info)
#else
GetIconBox(Widget parent, Login_info *info, struct display *d)
#endif
{
	int	n;
	Arg	i_arg[8];
	Widget	*swinptr, w_box;

	item_count = 0; ibx = INIT_X; iby = INIT_Y;
	for (n = 0; n < u_cnt; n++) {
#ifndef	STANDALONE_DT
		if ((u_list[n].pw_uid == 0 && u_list[n].pw_gid == 1) &&
		    d->showRootIcon)
			AddUserItem(&u_list[n]);
		else if (u_list[n].pw_uid >= 100 && u_list[n].pw_uid < 60000)
			AddUserItem(&u_list[n]);
#else
		if ((u_list[n].pw_uid == 0 && u_list[n].pw_gid == 1) ||
		    (u_list[n].pw_uid >= 100 && u_list[n].pw_uid < 60000))
			AddUserItem(&u_list[n]);
#endif
		}
	XtSetArg(i_arg[0], XtNadjustProc,	(XtArgVal)AdjustUser);
	XtSetArg(i_arg[1], XtNmovableIcons,	(XtArgVal)FALSE);
	XtSetArg(i_arg[2], XtNminWidth,		(XtArgVal)1);
	XtSetArg(i_arg[3], XtNminHeight,	(XtArgVal)1);
	XtSetArg(i_arg[4], XtNdrawProc,		(XtArgVal)DmDrawIcon);
	XtSetArg(i_arg[5], XtNselectProc,	(XtArgVal)SingleClick);
	XtSetArg(i_arg[6], XtNclientData,	(XtArgVal)info);

	swinptr = (parent == w_canvas? &info->swin: (Widget *)NULL);
	w_box = DmCreateIconContainer(parent, DM_B_CALC_SIZE, i_arg, 7,
		u_cntrec.op, u_cntrec.num_objs,
		&u_itp, u_cntrec.num_objs, swinptr, NULL, NULL, 1);
	
	if (parent == w_canvas)
		XtVaSetValues(info->swin,
				XtNy,		(XtArgVal)(yinch/2),
				XtNheight,	(XtArgVal)((int)HEIGHT/(int)3),
				XtNwidth,	(XtArgVal)(WIDTH-xinch/5),
				NULL);
	return w_box;
}

static	void
LoginVerifyCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	Login_info		*info = (Login_info *)client_data;
	OlTextFieldVerify	*verify = (OlTextFieldVerify *)call_data;
	Time			t;

	switch (verify->reason)
		{
		case OlTextFieldReturn:	
			if (strlen (verify->string))
				{
				Debug ("%s\n", verify->string);
				XtVaSetValues (info->login_fail, XtNstring,
					(XtArgVal)GGT(string_noecho), NULL);
				SensitiseLogin (TRUE);
				XtSetSensitive (info->password_text, TRUE);
				t = CurrentTime;
				if (!(XtCallAcceptFocus (info->password_text,
					&t)))
					Debug ("Could not set input focus\n");
				}
				break;
		default:	break;
		}
	return;
}

/*
 *	Create the login window area 
 */
#ifdef	STANDALONE_DT
main (int argc, char **argv)
#else
Login_info *
CreateLoginArea (toplevel, d)
Widget toplevel;
struct display *d;
#endif
{
	int		width, height, center_x, center_y;
	int		w_width, w_height;
	Colormap	cmap;
	char		sysname[64];
	Pixel		back;
	XColor		background, ignore;
	DmGlyphPtr	g;
	XtAppContext	app;
	Widget		footer, login_area, copyright;
	Widget		welcome_message, company_logo;
	Widget		login_caption, login_text;
	Widget		password_caption, password_text;
	struct utsname	name;
	Dimension	w;
	Login_info	*info;

	if (!(info = (Login_info *)malloc (sizeof (Login_info))))
		{
		Debug ("Could not allocate memory for widget structure\n");
		exit (EXIT_XDM);
		}

	uname (&name);
	sprintf (sysname, "%s %s.  %s\0", GGT(string_greet1), name.nodename,
		GGT(string_greet2));

	MakeUserList();

#ifdef	STANDALONE_DT
	OlToolkitInitialize(&argc, argv, NULL);
	w_toplevel = XtInitialize("dtlogin", "Dtlogin", NULL, 0, &argc, argv);
#else
	w_toplevel = toplevel;
#endif

	Debug ("Creating Interface\n");
	theDisplay = XtDisplay(w_toplevel);
	theScreen = XtScreen(w_toplevel);

	xinch = OlPointToPixel(OL_HORIZONTAL,72);
	yinch = OlPointToPixel(OL_VERTICAL,72);

	footer = XtVaCreateManagedWidget("footer", controlAreaWidgetClass,
			w_toplevel,
			XtNlayoutType,	(XtArgVal)OL_FIXEDCOLS,
			XtNcenter,	(XtArgVal)TRUE,
			XtNalignCaptions,(XtArgVal)TRUE,
			XtNvSpace,	(XtArgVal)10,
			NULL);

	company_logo = XtVaCreateManagedWidget ("company_logo",
			staticTextWidgetClass, footer,
			XtNgravity, CenterGravity,
			XtNalignment, OL_CENTER,
			NULL);

	welcome_message = XtVaCreateManagedWidget ("welcome_message",
			staticTextWidgetClass, footer,
			XtNstring, sysname,
			XtNgravity, CenterGravity,
			XtNalignment, OL_CENTER,
			NULL);

	login_caption = XtVaCreateManagedWidget ("login_caption",
			captionWidgetClass, footer,
			XtNlabel, (XtArgVal)GGT(label_login),
			XtNmnemonic, (XtArgVal)*GGT(mnemonic_id),
			XtNalignment, OL_CENTER,
			NULL);
	info->login_text = XtVaCreateManagedWidget ("login_text", 
			textFieldWidgetClass, login_caption,
			XtNcharsVisible, (XtArgVal)20,
			NULL);
	XtAddCallback (info->login_text, XtNverification, LoginVerifyCB,
			(XtPointer)info);
	w_login_text = info->login_text;

	password_caption = XtVaCreateManagedWidget ("password_caption",
			captionWidgetClass, footer,
			XtNmnemonic, (XtArgVal)*GGT(mnemonic_pass),
			XtNlabel, (XtArgVal)GGT(label_password),
			XtNalignment, OL_CENTER,
			NULL);
	info->password_text = XtVaCreateManagedWidget ("password_text", 
			textFieldWidgetClass, password_caption,
			XtNsensitive, (XtArgVal)FALSE,
			XtNcharsVisible, (XtArgVal)20,
			NULL);
	XtVaGetValues (info->password_text, XtNbackground, &back, NULL);
	XtVaSetValues (info->password_text, XtNforeground, back,
			XtNfontColor, back, NULL);

#ifdef	STANDALONE_DT
	CreateControlBar (footer, info);
#else
	if (d->showIcons)
		CreateControlBar (footer, info, d);
	else
		CreateControlBar (footer, info, d);
#endif

	copyright = XtVaCreateManagedWidget ("copyright",
			staticTextWidgetClass, footer,
			XtNgravity, CenterGravity,
			XtNalignment, OL_LEFT,
			XtNstring, GGT(string_copyright),
			NULL);

	info->login_fail = XtVaCreateManagedWidget ("login_fail",
			staticTextWidgetClass, footer,
			XtNgravity, CenterGravity,
			XtNalignment, OL_CENTER,
			NULL);
	switch (loginProblem)
		{
		case BADSHELL:	XtVaSetValues (info->login_fail, XtNstring,
					GGT(string_badshell), NULL);
				break;
		case NOHOME:	XtVaSetValues (info->login_fail, XtNstring,
					GGT(string_nohome), NULL);
				break;
		}

	Debug ("Interface Created\n");
#ifdef	STANDALONE_DT
	XtRealizeWidget(w_toplevel);

	XtMainLoop();
#else
	return info;
#endif
}

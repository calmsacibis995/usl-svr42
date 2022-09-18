/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:floppy/format.c	1.31"
#endif

#include "media.h"

extern	void	FmtInsertMessage();

extern	long	_dtam_flags;
extern	int	backup_flag;

void	fkillCB();
void	formatCB();
void	doformatCB();
void	cancelFmtCB();
void	fhelpCB();

static MenuItems faction_menu_item[] = {
	{ TRUE, label_format,mnemonic_format, 0, formatCB},
	{ TRUE, label_exit,  mnemonic_exit, 0, exitCB},
	{ NULL }
};

static HelpInfo FHelpFormat	= { 0, "", DHELP_PATH, help_format };
static HelpInfo FHelpTOC	= { 0, "", DHELP_PATH, NULL        };
static HelpInfo FHelpDesk	= { 0, "", DHELP_PATH, "HelpDesk"  };

static MenuItems fhelp_menu_item[] = {  
	{ TRUE, label_fmtHlp,mnemonic_fmtHlp,0, fhelpCB, (char *)&FHelpFormat },
	{ TRUE, label_toc,   mnemonic_toc,   0, fhelpCB, (char *)&FHelpTOC },
	{ TRUE, label_hlpdsk,mnemonic_hlpdsk,0, fhelpCB, (char *)&FHelpDesk },
	{ NULL }
};

static MenuGizmo faction_menu = {0, "action_menu", NULL, faction_menu_item};
static MenuGizmo fhelp_menu = {0, "help_menu", NULL, fhelp_menu_item};

static MenuItems fmain_menu_item[] = {
	{ TRUE, label_action, mnemonic_action, (Gizmo) &faction_menu},
	{ TRUE, label_help,   mnemonic_help, (Gizmo) &fhelp_menu},
	{ NULL }
};
static MenuGizmo fmenu_bar = { 0, "menu_bar", NULL, fmain_menu_item};

BaseWindowGizmo fbase = {0, "base", label_format, (Gizmo)&fmenu_bar,
	NULL, 0, label_format, D1ICON_NAME, " ", " ", 90 };

static MenuItems fwatch_menu_item[] = {  
	{ TRUE, label_cancel, mnemonic_cancel, 0, fkillCB, NULL },
	{ TRUE, label_help,   mnemonic_help, 0, fhelpCB, (char *)&FHelpTOC },
	{ NULL }
};
static MenuGizmo fwatch_menu = {0, "fwatch_menu", NULL, fwatch_menu_item};
static PopupGizmo fwatch = {0, "popup", title_doingFmt, (Gizmo)&fwatch_menu};

static MenuItems fconfirm_menu_item[] = {  
	{ TRUE, label_format, mnemonic_format, 0, doformatCB, NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, cancelFmtCB, NULL },
	{ TRUE, label_help,   mnemonic_help, 0, fhelpCB, (char *)&FHelpFormat },
	{ NULL }
};
static MenuGizmo fconfirm_menu = {0, "fconfirm_menu", NULL, fconfirm_menu_item};
static ModalGizmo fconfirm = {0, "", title_confirmFmt, (Gizmo)&fconfirm_menu};

#define	HIGH_DNS	0
#define	LOW_DNS		1

#define	BKUP_FMT	0	/* i.e., no file system */
#define	S5_FMT		1
#define	DOS_FMT		2

int	fmt_type = BKUP_FMT;

FILE		*cmdfp[2];

ExclItem	DnsItems[2];
ExclItem	FmtItems[4];

DevItem		DeviceItem[N_DEVS];

XtIntervalId	gauge_id = 0;		/* also used by backup ??? */
int		g_value = 0;
Widget		w_gauge, w_txt;

Boolean		fmt_done = FALSE;
Widget		w_fdesc, w_dcap;
char		*op;

void	fhelpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	HelpInfo *help = (HelpInfo *) client_data;

	FooterMsg(fbase, NULL);
	help->app_title	= 
	help->title	= GetGizmoText(label_format);
	help->section = GetGizmoText(STRDUP(help->section));
	PostGizmoHelp(fbase.shell, help);
}

void	cancelFmtCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	FooterMsg(fbase, NULL);
	BringDownPopup(fconfirm.shell);
	if (cmdfp[1]) {
		_Dtam_p3close(cmdfp, 0);
		cmdfp[0] = cmdfp[1] = (FILE *)NULL;
	}
}

void	DropWatch(XtPointer closure, XtIntervalId id)
{
	XDefineCursor(theDisplay, XtWindow(fwatch.shell),
                  				GetOlStandardCursor(theScreen));
	XDefineCursor(theDisplay, XtWindow(w_gauge),
                  				GetOlStandardCursor(theScreen));
	XDefineCursor(theDisplay, XtWindow(fbase.shell),
                  				GetOlStandardCursor(theScreen));
	BringDownPopup(fwatch.shell);
}

void	fkillCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	char	buf[80];

	FooterMsg(fbase, NULL);
	_Dtam_p3close(cmdfp, SIGTERM);
	sprintf(buf, GetGizmoText(string_opKilled), op);
	XtSetArg(arg[0], XtNstring, (XtArgVal)buf);
	XtSetValues(w_txt, arg, 1);
	if (gauge_id) {
		XtRemoveTimeOut(gauge_id);
		gauge_id = 0;
		XtSetArg(arg[0], XtNsliderValue,	0);
		XtSetArg(arg[1], XtNmappedWhenManaged,	FALSE);
		XtSetValues(w_gauge, arg, 2);
	}
	FooterMsg(fbase, buf);
	XtAddTimeOut(1500, (XtTimerCallbackProc)DropWatch, (XtPointer)NULL);
}

SetGauge(value)
	int	value;
{
	if (value < 0)
		value = 0;
	else if (value > 100)
		value = 100;
	g_value = value;
	XtSetArg(arg[0], XtNsliderValue, (XtArgVal)g_value);
	XtSetValues(w_gauge, arg, 1);
}

void	UpdateGauge(delta, id)	/* follows a gauge in 1% increments */
	int		delta;
	XtIntervalId	id;
{
	if (cmdfp[1] == NULL || g_value >= 100) {
		g_value = 0;
		gauge_id = NULL;
		XtSetArg(arg[0], XtNsliderValue, 	g_value);
		XtSetArg(arg[1], XtNmappedWhenManaged,	FALSE);
		XtSetValues(w_gauge, arg, 2);
	}
	else {
		++g_value;
		gauge_id = XtAddTimeOut(delta, (XtTimerCallbackProc)UpdateGauge,
							(XtPointer)delta);
		XtSetArg(arg[0], XtNsliderValue, (XtArgVal)g_value);
		XtSetValues(w_gauge, arg, 1);
	}
}


char	*FmtDiagMsg(diagnostic)
	int	diagnostic;
{
extern	char	*FmtInsertMsg();
static	char	buf[128];
	char	*str;
	char	*drive = DtamDevAlias(curdev);

	if (_dtam_flags & DTAM_READ_ONLY) {
		sprintf(buf, GetGizmoText(string_cantWrite), drive);
		str = buf;
	}
	else switch (diagnostic) {

	case NO_DISK:		str = FmtInsertMsg(GetGizmoText(label_format));
				break;
	case UNDIAGNOSED:
	case UNREADABLE:	if ((str=DtamDevAttr(curdev,"volume")) == NULL)
					str = GetGizmoText(string_genMedia);
				if(strncmp(curdev,DISKETTE,strlen(DISKETTE))!=0)
					sprintf(buf, 
						GetGizmoText(string_unreadDisk),
						str, "");
				else
					sprintf(buf, 
						GetGizmoText(string_unreadDisk),
						str, drive+strlen(drive)-1);
				str = buf;
				break;
	case UNFORMATTED:	str = " ";
				break;
	case DOS_DISK:		str = GetGizmoText(string_fmtDOStoUNIX);
				break;
	case UNKNOWN:		sprintf(buf, GetGizmoText(string_isFormatted),
						drive+strlen(drive)-1);
				str = buf;
				break;
	default:		str = GetGizmoText(string_hasData);
	}
	FREE(drive);
	return str;
}

/*
 *	look for error messages, or confirmatory output; show gauge if limited
 */
void	CheckStderr(int limit, XtIntervalId id)
{
static	Boolean	in_mkfs = FALSE;
static	Boolean	in_fmt = FALSE;
static	char	*msg = NULL;
	char	buf[BUFSIZ], *str;
	int	n, status;

	if (cmdfp[1] == NULL)
		return;			/* terminated by cancel; end timeouts */
	n = read(CMDOUT, buf, BUFSIZ);
	switch (n) {
	case 0:	/*
		 *	end of file; command has finished
		 */
		fmt_done = TRUE;
		in_fmt = in_mkfs = FALSE;
		status = _Dtam_p3close(cmdfp, 0);
		/*
		 *	the following scan of the messages should be
		 *	replaced by a switch on status, once exit codes exist.
		 */
		if (strstr(msg, "Can't open")
		||  strstr(msg, "cannot open"))
			sprintf(buf, GetGizmoText(string_cantWrite),
					DtamDevAlias(curdev));
		else if (strstr(msg, "Formatted 0 track") )
			sprintf(buf, GetGizmoText(string_wrongDensity),
					DtamDevAlias(curdev));
		else if (strstr(msg, "Available blocks"))
			sprintf(buf, GetGizmoText(string_opOK), op);
		else if (strstr(msg,"Formatted")
		||  strstr(msg,"Format complete"))
			sprintf(buf, GetGizmoText(string_opOK), op);
		else if (status != 0) {
			sprintf(buf, GetGizmoText(string_opFailed), op);
			if (msg)
				fprintf(stderr, GGT(string_stderr), op, msg);
		}
		else
			sprintf(buf, GetGizmoText(string_opOK), op);
		XtSetArg(arg[0], XtNstring, (XtArgVal)buf);
		XtSetValues(w_txt, arg, 1);
		XtAddTimeOut(1500, (XtTimerCallbackProc)DropWatch,
							(XtPointer)NULL);
		FooterMsg(fbase, buf);
		if (msg) {
			FREE(msg);
			msg = NULL;
		}
		/*
		 * An unformatted diskette was detected during backup operation.
		 */
		if (backup_flag)
			(void)exit(0);
		break;

	default:/*
		 *	save message for later examination
		 */
		if (msg == NULL) {
			msg = (char *)MALLOC(n+2);
			*msg = '\0';
		}
		else
			msg = (char *)REALLOC(msg, strlen(msg)+n+2);
		if (msg == NULL) {
			XtSetArg(arg[0], XtNstring,
					GetGizmoText(string_badMalloc));
			XtSetValues(w_txt, arg, 1); 
			return;
		}
		buf[n] = '\0';
		if (strstr(buf,"ormatting")) {
			in_fmt = TRUE;
			in_mkfs = FALSE;
			if (limit) {
				SetGauge(0);
				XtSetArg(arg[0], XtNmappedWhenManaged, TRUE);
				XtSetValues(w_gauge, arg, 1);
				gauge_id = XtAddTimeOut(100,
					(XtTimerCallbackProc)UpdateGauge,
					(XtPointer)(limit/100));
			}
		}
		strcat(msg,buf);
		if (strstr(msg,"make s5 file system?") && !in_mkfs) {
			in_mkfs = TRUE;
			op = GetGizmoText(string_mkfsOp);
			XtSetArg(arg[0], XtNstring, 
					GetGizmoText(string_doingMkfs));
			XtSetValues(w_txt,  arg, 1);
		}
		/* fall through to wait for more input */
	case -1:
		XtAddTimeOut(1250, (XtTimerCallbackProc)CheckStderr,
							(XtPointer)limit);
		break;
	}
}

int	density = HIGH_DNS;

void	SetDensity(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	char		*ptr;
	OlFlatCallData	*olcd = (OlFlatCallData *) call_data;

	FooterMsg(fbase, NULL);
	ptr = (char *)DnsItems[olcd->item_index].label;
	density = olcd->item_index;
}

void	SetFormatType(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	char		*ptr;
	OlFlatCallData	*olcd = (OlFlatCallData *) call_data;

	FooterMsg(fbase, NULL);
	ptr = (char *)FmtItems[olcd->item_index].label;
	fmt_type = olcd->item_index;
}

/*
 *	The File menu lists formattable devices by alias; when one is
 *	selected, this callback is invoked to update the w_fdesc description
 *	taken from the desc attribute in the device table.
 */
void	SetDevDesc(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	Widget		w_ud;
	OlFlatCallData	*olcd = (OlFlatCallData *) call_data;
	char		*dev, *str;

	FooterMsg(fbase, NULL);
	curalias = DtamMapAlias(str = DeviceItem[olcd->item_index].label);
	if (curdev)
		FREE(curdev);
	curdev = DtamGetDev(curalias,FIRST);
	XtSetArg(arg[0], XtNuserData, &w_ud);
	XtGetValues(wid,  arg, 1);
	XtSetArg(arg[0], XtNlabel, (XtArgVal)DtamDevDesc(curdev));
	XtSetValues(w_ud, arg, 1);
	XtSetArg(arg[0], XtNlabel, (XtArgVal)str);
	XtSetValues(XtParent(w_ud), arg, 1);

	str = DtamDevAttr(curdev,"mdenslist");
	XtSetArg(arg[0], XtNmappedWhenManaged, (str!=NULL));
	XtSetValues(w_dcap, arg, 1);
	if (str)
		FREE(str);
}

void	MapWatchNotice()
{
	Widget	w_up;

	if (!fwatch.shell) {
		CreateGizmo(fbase.shell, PopupGizmoClass, &fwatch, NULL, 0);

		XtSetArg(arg[0], XtNupperControlArea, &w_up);
		XtGetValues(fwatch.shell, arg, 1);

		XtSetArg(arg[0], XtNwindowHeader, FALSE);
		XtSetValues(fwatch.shell, arg, 1);

		XtSetArg(arg[0], XtNlayoutType,		OL_FIXEDCOLS);
		XtSetArg(arg[1], XtNalignCaptions,	TRUE);
		XtSetArg(arg[2], XtNcenter,		TRUE);
		XtSetArg(arg[3], XtNhPad,		x3mm);
		XtSetValues(w_up, arg, 4);

		XtSetArg(arg[0], XtNheight, 3*y3mm);
		XtCreateManagedWidget("spacer", rectObjClass, w_up, arg, 1);

		XtSetArg(arg[0], XtNalignment,		OL_CENTER);
		XtSetArg(arg[1], XtNgravity,		CenterGravity);
		XtSetArg(arg[2], XtNwidth,		(32*x3mm));
		XtSetArg(arg[3], XtNfont, 		bld_font);
		w_txt = XtCreateManagedWidget("text",
				staticTextWidgetClass, w_up, arg, 4);

		XtSetArg(arg[0], XtNheight, 2*y3mm);
		XtCreateManagedWidget("spacer", rectObjClass, w_up, arg, 1);

		XtSetArg(arg[0], XtNspan,        	32*x3mm);
		XtSetArg(arg[1], XtNmappedWhenManaged,	FALSE);
		XtSetArg(arg[2], XtNorientation, 	OL_HORIZONTAL);
		XtSetArg(arg[3], XtNminLabel,    	" 0 %");
		XtSetArg(arg[4], XtNmaxLabel,    	"100 %  ");
		XtSetArg(arg[5], XtNsliderMax,   	100);
		XtSetArg(arg[6], XtNsliderValue, 	0);
		XtSetArg(arg[7], XtNshowValue,   	TRUE);
		XtSetArg(arg[8], XtNtickUnit,		OL_PERCENT);
		XtSetArg(arg[9], XtNticks,		10);
	 	w_gauge = XtCreateManagedWidget("gauge",
				gaugeWidgetClass, w_up, arg, 10);
	}
	XtSetArg(arg[0], XtNstring, (XtArgVal)GGT(string_doingFmt));
	XtSetValues(w_txt, arg, 1);
	MapGizmo(PopupGizmoClass, &fwatch);
}

/*
 *	It is ad hoc, but useful, to take some empirical times for formatting
 *	and making file systems on diskettes, to allow a reasonable timing
 *	gauge.  ISV's who wish may add dtfmttime and dtmkfstime attributes to
 *	the device table for other devices with fmtcmd and mkfscmd options.
 */
struct {char	f_factor;
	int	t_fmt;
	int	t_mkfs;
	int	t_dos;
} ftime[] = 	{
			{'3',  108,  27, 85},	/* 3.5" diskettes */
			{'3',  108,  48, 88},
			{'5',  48,  30, 70},	/* 5.25" diskettes */
			{'5',  92,  49, 72},
			{'\0',  0,   0,  0},
		};

void	formatCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	int	diag;

	FooterMsg(fbase, NULL);
	diag = DtamCheckMedia(curalias);
	if (diag == UNFORMATTED)
		doformatCB(wid, client_data, call_data);
	else {
		if (!fconfirm.shell)
			CreateGizmo(fbase.shell, ModalGizmoClass, &fconfirm,
								NULL, 0);
		SetModalGizmoMessage(&fconfirm, FmtDiagMsg(diag));
		MapGizmo(ModalGizmoClass, &fconfirm);
	}
}

void	doformatCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
static	char	*ltag = "LANG=C ";
	char	*devline, *attr, *cmd1 = NULL, *cmd2 = NULL;
	char	form;
	int	n, time = 0;

	FooterMsg(fbase, NULL);
	fmt_done = FALSE;
	if (fconfirm.shell)
		BringDownPopup(fconfirm.shell);
	MapWatchNotice();
	XDefineCursor(theDisplay, XtWindow(w_gauge),
						GetOlBusyCursor(theScreen));
	XDefineCursor(theDisplay, XtWindow(fwatch.shell),
                  				GetOlBusyCursor(theScreen));
	XDefineCursor(theDisplay, XtWindow(fbase.shell),
                  				GetOlBusyCursor(theScreen));

	/*
	 *	fiddle around if format may be other than default
	 */
	if (strstr(curdev, DISKETTE))
		form = *strpbrk(strstr(curdev,"fmtcmd"),"35");
		/*
		 *	get this now, as mdenslist may obscure the issue
		 */
	devline = curdev;
	if (attr=DtamDevAttr(curdev, "mdensdefault")) {
		if (density==HIGH_DNS && strstr(attr,"HIGH")==NULL
		||  density==LOW_DNS && strstr(attr,"LOW")==NULL) {
			FREE(attr);
			if (attr=DtamDevAttr(curdev,"mdenslist")) {
				char	*ptr, *ptr2;
				if (density==LOW_DNS) {
					ptr = strstr(attr,"LOW");
					if (ptr == NULL)
						ptr = strrchr(attr,',')+1;
					else {
						if (ptr2 = strchr(ptr,','))
							*ptr2 = '\0';
						while (ptr > attr && *ptr!=',')
							--ptr;
						if (ptr > attr)
							++ptr;
					}
				}
				if (density==HIGH_DNS) {
					ptr = strstr(attr,"HIGH");
					if (ptr == NULL)
						ptr = attr;
					else {
						while (ptr > attr && *ptr!=',')
							--ptr;
						if (ptr > attr)
							++ptr;
					}
					if (ptr2 = strchr(ptr,','))
						*ptr2 = '\0';
				}
				devline = DtamGetDev(ptr,NEXT);
				FREE(attr);
			}
		}
	}
	if (fmt_type == DOS_FMT) {
		char	*ptr;
		attr = "/usr/bin/dosformat -f ";
		cmd2 = DtamDevAttr(devline, "fmtcmd");
		ptr = strstr(cmd2, "/dev");
		cmd1 = (char *)MALLOC(strlen(ltag)+strlen(attr)+strlen(ptr)+8);
		sprintf(cmd1, "%s%s%s", ltag, attr, ptr);
		FREE(cmd2);
	}
	else {
		char *temp, *spot;
		temp = DtamDevAttr(devline,"fmtcmd");
		spot = strstr(temp,"-v ");
		attr = (char *) MALLOC(strlen(temp)+10);

		if (spot)
		{ /* "-V" (partial verify) flag after "-v" flag */
			*spot = '\0';
			strcpy(attr,temp);
			strcat(attr,"-v -V");
			strcat(attr,spot+2);
		}
		else
			strcpy (attr,temp);
		FREE(temp);

		if (fmt_type == BKUP_FMT) {
			cmd1 = (char *)MALLOC(strlen(ltag)+strlen(attr)+8);
			strcat(strcat(cmd1,ltag),attr);
			FREE(attr);
		}
		else {
			cmd2 = DtamDevAttr(devline,"mkfscmd");
			cmd1 = (char *)MALLOC(strlen(ltag)+strlen(attr)+
							strlen(cmd2)+12);
			sprintf(cmd1,"(%s%s;%s)", ltag, attr, cmd2);
			FREE(attr);
			FREE(cmd2);
			if (attr=DtamDevAttr(devline,"dtmkfstime")) {
				time = atoi(attr);
				FREE(attr);
			}
		}
		if (attr = DtamDevAttr(devline,"dtfmttime")) {
			time += atoi(attr);
			FREE(attr);
		}
	}
	if (time == 0 && strstr(curalias,DISKETTE)) {
	/*
	 *	 check pre-tabulated values
	 */
		for (n = 0; ftime[n].f_factor; n++) {
			if (form == ftime[n].f_factor) {
				if (density == HIGH_DNS)
					n++;
				switch (fmt_type) {
				case DOS_FMT:	time = ftime[n].t_dos;
						break;
				case S5_FILES:	time = ftime[n].t_mkfs;
						/*
						 * and add t_fmt:
						 */
				case BKUP_FMT:	time += ftime[n].t_fmt;
						break;
				}
				break;
			}
		}
	}
	strcat(cmd1, " 2>&1");
	_Dtam_p3open(cmd1, cmdfp, TRUE);
	XtAddTimeOut(2500, (XtTimerCallbackProc)CheckStderr,
						(XtPointer)(1000*time));
	op = GetGizmoText(string_fmtOp);
	FREE(cmd1);
	if (devline != curdev)
		FREE(devline);
}

CreateFormatWindow(parent)
	Widget	parent;
{
	Widget		w_ctl, w_cap, w_dns, w_fmt, w_devmenu;
	char		*dev, *str;

	if (note.shell)
		XtDestroyWidget(note.shell);
	if (strcmp(curalias,"diskette2")==0)
		fbase.icon_pixmap = D2ICON_NAME;
	else if (strcmp(curalias,"diskette1") != 0)
		fbase.icon_pixmap = GENICON_NAME;
	CreateGizmo(parent, BaseWindowGizmoClass, &fbase, NULL, 0);

	XtSetArg(arg[0], XtNwidth, 44*x3mm);
	XtSetValues(fbase.scroller,  arg, 1);

	XtSetArg(arg[0], XtNlayoutType,	 OL_FIXEDCOLS);
	XtSetArg(arg[1], XtNalignCaptions,TRUE);
	XtSetArg(arg[2], XtNvSpace,	 (2*y3mm));
	XtSetArg(arg[3], XtNvPad,	 (2*y3mm));
	XtSetArg(arg[4], XtNhPad,	 x3mm);
	XtSetArg(arg[5], XtNshadowThickness, 0);

	w_ctl = XtCreateManagedWidget("control",
			controlAreaWidgetClass, fbase.scroller, arg, 6);

	w_fdesc = DevMenu(DeviceItem, 0, N_DEVS, w_ctl, GGT(label_devCaption),
				(XtPointer)SetDevDesc, "fmtcmd", &w_devmenu);
	if (!curalias)
		curalias = DtamMapAlias(DeviceItem[0].label);
	if (!curdev)
		curdev = DtamGetDev(curalias, FIRST);

	XtSetArg(arg[0], XtNlabel, DtamDevDesc(curdev));
	XtSetValues(w_fdesc, arg, 1);
	XtSetArg(arg[0], XtNlabel, DtamDevAlias(curdev));
	XtSetValues(XtParent(w_fdesc), arg, 1);
/*
 *	Density exclusives
 */
	SET_EXCL(DnsItems, 0, highDns, TRUE /*set*/);
	SET_EXCL(DnsItems, 1, lowDns, FALSE /*unset*/);

	str = DtamDevAttr(curdev,"mdenslist");

	XtSetArg(arg[0], XtNposition,		OL_LEFT);
	XtSetArg(arg[1], XtNspace,		x3mm);
	XtSetArg(arg[2], XtNlabel,		GGT(label_dnsCaption));
	XtSetArg(arg[3], XtNmappedWhenManaged,	(str != NULL));
	if (str)
		FREE(str);

	w_dcap = XtCreateManagedWidget("caption",
			captionWidgetClass, w_ctl, arg, 4);

	XtSetArg(arg[0], XtNtraversalOn,	TRUE);
	XtSetArg(arg[1], XtNbuttonType,		OL_RECT_BTN);
	XtSetArg(arg[2], XtNexclusives,		TRUE);
	XtSetArg(arg[3], XtNsameWidth,		OL_ALL);
	XtSetArg(arg[4], XtNitemFields,		ExclFields);
	XtSetArg(arg[5], XtNnumItemFields,	NUM_ExclFields);
	XtSetArg(arg[6], XtNitems,		DnsItems);
	XtSetArg(arg[7], XtNnumItems,		2);
	XtSetArg(arg[8], XtNselectProc,		SetDensity);

	w_dns = XtCreateManagedWidget("density",
			flatButtonsWidgetClass, w_dcap, arg, 9);
/*
 *	type of format (raw, file system or DOS)
 */
	SET_EXCL(FmtItems, 0, bkupFmt, TRUE /*set*/);
	SET_EXCL(FmtItems, 1, s5Fmt, FALSE /*unset*/);
	SET_EXCL(FmtItems, 2, dosFmt, FALSE /*unset*/);

	XtSetArg(arg[0], XtNlabel,		GGT(label_fmtCaption));
	XtSetArg(arg[1], XtNposition,		OL_LEFT);
	XtSetArg(arg[2], XtNspace,		x3mm);

	w_cap = XtCreateManagedWidget("caption",
			captionWidgetClass, w_ctl, arg, 3);

	XtSetArg(arg[0], XtNtraversalOn,	TRUE);
	XtSetArg(arg[1], XtNbuttonType,		OL_RECT_BTN);
	XtSetArg(arg[2], XtNexclusives,		TRUE);
	XtSetArg(arg[3], XtNitemFields,		ExclFields);
	XtSetArg(arg[4], XtNnumItemFields,	NUM_ExclFields);
	XtSetArg(arg[5], XtNitems,		FmtItems);
	XtSetArg(arg[6], XtNnumItems,		3);
	XtSetArg(arg[7], XtNselectProc,		SetFormatType);

	w_fmt = XtCreateManagedWidget("type",
			flatButtonsWidgetClass, w_cap, arg, 8);

	MapGizmo(BaseWindowGizmoClass, &fbase);
}

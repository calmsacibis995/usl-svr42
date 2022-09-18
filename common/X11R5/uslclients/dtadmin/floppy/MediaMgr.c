/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:floppy/MediaMgr.c	1.13.2.28"
#endif
/*
 *	UNIX Desktop Removable Media (Diskette/Tape) Manager
 *
 *	handles mounting/unmounting, formatting and switching to other
 *	dtadmin facilities (backup/restore, package installation, etc.)
 */
#include "media.h"

#define FMT_ATOM_PFX		"DtFmt_"
#define RST_ATOM_PFX		"DtRst_"

enum { Backup_Mode, Restore_Mode, DosFS_Mode, UnixFS_Mode, Format_Mode };

static void	SetOwner (char *atomName, char *fileName);
static Boolean	DropNotify (Widget w, Window win, Position x, Position y, 
			    Atom selection, Time timestamp, 
			    OlDnDDropSiteID drop_site_id, 
			    OlDnDTriggerOperation op, 
			    Boolean send_done, Boolean forwarded,
			    XtPointer closure);
static void	SelectionCB (Widget w, XtPointer client_data,
			     XtPointer call_data);
static void	ContCB (Widget w, XtPointer client_data, XtPointer call_data);

char		*cur_file = NULL;
char		*curdev	= NULL;			/* these maintain the current */
char		*curalias = NULL;		/* getdev line and its alias. */

Widget		w_toplevel;
Widget		w_msg;
Display		*theDisplay;
Screen		*theScreen;
Dimension	x3mm, y3mm;
XFontStruct	*def_font, *bld_font;
Arg		arg[12];
static int	Mode;

char	*tfadminPkg = "/sbin/tfadmin";
Boolean	packageOwner;
char	*CpioCmd;		/* set in main() used by backup & restore */

#define	DO_OPEN		0
#define	DO_COPY		1
#define	DO_BACKUP	2
#define	DO_RESTOR	4
#define	DO_INSTAL	8
#define	DO_FORMAT	16
#define	DO_REWIND	32

char	*save_command[1];
int	save_count = 1;
int	backup_flag = 0;
int	restricted_flag = 0;
ExitValue success_flag  = NoAttempt;
char	*dropstr = NULL;
char	*MBtnFields[] = {XtNlabel, XtNmnemonic,
			 XtNsensitive, XtNselectProc, XtNpopupMenu };
char	*ExclFields[] = {XtNlabel, XtNmnemonic, XtNset };

void	exitCB();
void	retryCB();
void	helpCB();
void	SelectAction();
char	*GenericMsg();

extern BaseWindowGizmo rbase;
extern BaseWindowGizmo bbase;
extern BaseWindowGizmo dbase;
static void FreezeMinWindowSize(Widget shell);

static	HelpInfo HelpIntro	= { 0, "", DHELP_PATH, help_intro };

static MenuItems note_menu_item[] = {  
	{ TRUE, label_continue,mnemonic_continue, 0, retryCB, NULL },
	{ TRUE, label_cancel,mnemonic_cancel, 0, exitCB, NULL },
	{ TRUE, label_help,  mnemonic_help, 0, helpCB, (char *)&HelpIntro },
	{ NULL }
};
static MenuGizmo note_menu = {0, "note_menu", NULL, note_menu_item};
PopupGizmo note = {0, "popup", NULL, (Gizmo)&note_menu};


static MenuItems ShortNote_menu_item[] = {  
	{ TRUE, label_continue,mnemonic_continue, 0, ContCB, NULL },
	{ NULL }
};
static MenuGizmo ShortNote_menu = {0, "ShortNote_menu", NULL,
				       ShortNote_menu_item};
PopupGizmo ShortNote = {0, "popup", NULL, (Gizmo)&ShortNote_menu};

void	helpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	HelpInfo *help = (HelpInfo *) client_data;

	help->app_title	= 
	help->title	= DtamDevAlias(curdev);
	help->section = GetGizmoText(STRDUP(help->section));
	PostGizmoHelp(note.shell, help);
}

void	exitCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	_DtamUnlink();
	if (restricted_flag)
	    exit((int)success_flag);
	else
	    exit((int) client_data);
}

void	ContCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(_OlGetShellOfWidget (wid));
}

void	retryCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	int	diagn;
	char	*msg;

	diagn = DtamCheckMedia(curalias);
	switch (diagn) {

	case DTAM_NO_DISK:
	case DTAM_NOT_OWNER:
	case DTAM_DEV_BUSY:
	case DTAM_UNREADABLE:	
		msg = GenericMsg(diagn);
		XtSetArg(arg[0],  XtNstring, (XtPointer)msg);
		XtSetValues(w_msg, arg, 1);
		return;
	default:
		BringDownPopup(note.shell);
		SelectAction(dropstr? DO_COPY: DO_OPEN, diagn, curalias);
		return;
	}
}

char	*FmtInsertMsg(button_label)
	char	*button_label;
{
static	char	insert_str[80];
	char	*str;

	if ((str = DtamDevAttr(curdev,"volume")) == NULL)
		str = STRDUP(GetGizmoText(string_genMedia));
	sprintf(insert_str, GetGizmoText(string_insMsg), str, button_label);
	FREE(str);
	return insert_str;
}

char	*GenericMsg(diagnostic)
	int	diagnostic;
{
	char	*str, *drive, buf[128];

	drive = DtamDevAlias(curdev);
	switch (diagnostic) {

	case DTAM_CANT_MOUNT:	sprintf(buf, GetGizmoText(string_mountErr),
							drive);
				str = buf;
				break;
	case DTAM_CANT_OPEN:	sprintf(buf, GetGizmoText(string_cantOpen),
							drive);
				str = buf;
				break;
	case DTAM_NO_ROOM:	sprintf(buf, GetGizmoText(string_noRoom),
							drive, dropstr);
				str = buf;
				break;
	case DTAM_NOT_OWNER:	str = GetGizmoText(string_notOwner);
				break;
	case DTAM_UNKNOWN:
	case DTAM_DEV_BUSY:
	case DTAM_UNREADABLE:
	case DTAM_UNFORMATTED:	if ((str=DtamDevAttr(curdev,"volume")) == NULL)
					str = GetGizmoText(string_genMedia);
				if (strncmp(curdev,DISKETTE,strlen(DISKETTE))!=0)
					sprintf(buf, 
						GetGizmoText(string_unreadDisk),
						str, "");
				else
					sprintf(buf, 
						GetGizmoText(string_unreadDisk),
						str, drive+strlen(drive)-1);
				str = buf;
				break;
		default:	str = FmtInsertMsg(GetGizmoText(label_continue));
				break;
	}
	FREE(drive);
	return str;
}

DevItem		Devs[N_DEVS];

void	SetCaptionCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	char		*label;
	Widget		w_ud;
	OlFlatCallData	*olcd = (OlFlatCallData *) call_data;

	XtSetArg(arg[0], XtNuserData, &w_ud);
	XtGetValues(wid, arg, 1);
	curalias = DtamMapAlias(label=Devs[olcd->item_index].label);
	if (curdev)
		FREE(curdev);
	curdev = DtamGetDev(curalias,FIRST);
	XtSetArg(arg[0], XtNlabel, DtamDevDesc(curdev));
	XtSetValues(w_ud, arg, 1);
	XtSetArg(arg[0], XtNlabel, label);
	XtSetValues(XtParent(w_ud), arg, 1);
}

void	BaseNotice(diagn, brief)
	int	diagn;
	Boolean	brief;
{
static	Widget	w_desc;
static	Widget	w_smsg;
	Widget	w_up, w_devmenu;
	PopupGizmo	*noteGizmo;
	char	*label = DtamDevAlias(curdev);

	noteGizmo = (brief) ? &ShortNote : &note;

	if (!noteGizmo->shell) {
		noteGizmo->title = label;
		CreateGizmo(w_toplevel, PopupGizmoClass, noteGizmo, NULL, 0);

		XtSetArg(arg[0], XtNwindowHeader, FALSE);
		XtSetValues(noteGizmo->shell, arg, 1);

		XtSetArg(arg[0], XtNupperControlArea, &w_up);
		XtGetValues(noteGizmo->shell, arg, 1);

		XtSetArg(arg[0], XtNlayoutType,		OL_FIXEDCOLS);
		XtSetArg(arg[1], XtNalignCaptions,	TRUE);
		XtSetArg(arg[2], XtNcenter,		TRUE);
		XtSetArg(arg[3], XtNvPad,		y3mm*3);
		XtSetArg(arg[4], XtNvSpace,		y3mm*2);
		XtSetArg(arg[5], XtNhPad,		x3mm*3);
		XtSetArg(arg[6], XtNhSpace,		x3mm*2);

		XtSetValues(w_up, arg, 7);

		XtSetArg(arg[0], XtNalignment,	OL_CENTER);
		XtSetArg(arg[1], XtNgravity,	CenterGravity);
		XtSetArg(arg[2], XtNwidth,	32*x3mm);
		XtSetArg(arg[3], XtNfont, 	bld_font);

		if (!brief)
		{
		    w_msg = XtCreateManagedWidget("text",
				staticTextWidgetClass, w_up, arg, 4);
		    w_desc = DevMenu(Devs, 0, N_DEVS, w_up, curalias,
				(XtPointer)SetCaptionCB,
				 "removable=\"true", &w_devmenu);
		}
		else
		    w_smsg = XtCreateManagedWidget("text",
				staticTextWidgetClass, w_up, arg, 4);
	}

	if (!brief)
	{
	    XtSetArg(arg[0], XtNlabel, DtamDevDesc(curdev));
	    XtSetValues(w_desc, arg, 1);

	    XtSetArg(arg[0], XtNlabel, label);
	    XtSetValues(XtParent(w_desc), arg, 1);
	}

	XtSetArg(arg[0], XtNstring, GenericMsg(diagn));
	XtSetValues((brief) ? w_smsg : w_msg, arg, 1);

	MapGizmo(PopupGizmoClass, noteGizmo);
}

/* SetOwner -- Attempt to become the "owner" of the device.  If the device
 * is already owned by another manifestation of the media manager, then
 * send a message to it to do the work; die after the message is acknowledged.
 */
static void
SetOwner (char *atomName, char *fileName)
{
    static Window	owner;

    /* Check if we are already the owner */
    if (owner)
	return;

    XtSetArg(arg[0], XtNmappedWhenManaged, FALSE);
    XtSetArg(arg[1], XtNheight, y3mm);
    XtSetArg(arg[2], XtNwidth,  x3mm);
    XtSetValues(w_toplevel, arg, 3);
    XtRealizeWidget(w_toplevel);

    OlDnDRegisterDDI(w_toplevel, OlDnDSitePreviewNone, DropNotify,
		     (OlDnDPMNotifyProc) 0, True, (XtPointer) &Mode);

    owner = DtSetAppId (XtDisplay (w_toplevel), XtWindow (w_toplevel),
			atomName);

    if (owner != None)
    {
	static char	*fileList [] = { 0, NULL };

	fileList [0] = (fileName) ? fileName : "";

	if (DtNewDnDTransaction(w_toplevel, fileList,
				DT_B_SEND_EVENT | DT_B_STATIC_LIST,
				0, 0, CurrentTime, owner, DT_COPY_OP,
				NULL, (XtCallbackProc) exitCB, 0))
	{
	    /* Another MediaMgr is running, and we successfully sent a
	     * message to it.  Wait for death.
	     */
#ifdef UseXtApp
	    XtAppMainLoop(app_con);
#else
	    XtMainLoop();
#endif
	}
    }

    /* Either we are now the owner, or our message failed.  In either event,
     * just continue on doing whatever we would normally do as if we are owner.
     */
    owner = XtWindow (w_toplevel);
}

/* SelectionCB
 *
 * Called when the file list is ready after a drag and drop operation.
 * client_data is a pointer to the operation to perform; call_data is
 * DtDnDInfoPtr drop information.
 */
static void
SelectionCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    int				*mode  = (int *) client_data;
    DtDnDInfoPtr		dip = (DtDnDInfoPtr) call_data;
    int				i;
    extern BaseWindowGizmo	rbase;
    extern BaseWindowGizmo	fbase;

    if (dip->error)
	return;

    switch (*mode) {
    case Backup_Mode:
	BackupTakeDrop (w, client_data, call_data);
	break;
    case Restore_Mode:
	XRaiseWindow (theDisplay, XtWindow (rbase.shell));
	break;
    case Format_Mode:
	XRaiseWindow (theDisplay, XtWindow (fbase.shell));
	break;
    case DosFS_Mode:
	DosTakeDrop (w, client_data, call_data);
	break;
    case UnixFS_Mode:
	UnixTakeDrop (w, client_data, call_data);
	break;
    }
}	/* End of SelectionCB () */

/* DropNotify
 *
 * Called by a pseudo-drop event on the toplevel window.  closure indicates
 * what operation we ought to do on the dropped files.
 */
static Boolean
DropNotify (Widget w, Window win, Position x, Position y, Atom selection,
            Time timestamp, OlDnDDropSiteID drop_site_id,
            OlDnDTriggerOperation op, Boolean send_done, Boolean forwarded, 
            XtPointer closure)
{
    DtGetFileNames (w, selection, timestamp, send_done, SelectionCB, closure);

    return(True);
}	/* End of DropNotify () */

void	SelectAction(int opflag, int diagn, char *atomName)
{
	char			*newAtom;
	extern XtIntervalId	disk_tid;

	switch (opflag) {
	case DO_BACKUP:	Mode = Backup_Mode;
			SetOwner (atomName, dropstr);
			CreateBackupWindow(w_toplevel, dropstr);
			FreezeMinWindowSize(bbase.shell);
			return;
	case DO_FORMAT:	
			Mode = Format_Mode;
			if (!backup_flag)
			{
				newAtom = XtMalloc (strlen (FMT_ATOM_PFX) +
						    strlen (atomName) + 1);
				strcpy (newAtom, FMT_ATOM_PFX);
				strcat (newAtom, atomName);
				SetOwner (newAtom, dropstr);
			}
			CreateFormatWindow(w_toplevel);
			return;
	case DO_RESTOR:	newAtom = XtMalloc (strlen (RST_ATOM_PFX) +
					    strlen (atomName) + 1);
			strcpy (newAtom, RST_ATOM_PFX);
			strcat (newAtom, atomName);
			Mode = Restore_Mode;
			SetOwner (newAtom, dropstr);
			CreateRestoreWindow(w_toplevel);
			FreezeMinWindowSize(rbase.shell);
			return;
	}
	if (diagn == -1)
		diagn = DtamCheckMedia(curalias);
	if (opflag != DO_COPY &&
	    (diagn&DTAM_PACKAGE || diagn&DTAM_INSTALL || diagn==DTAM_CUSTOM)) {

		if (packageOwner)
			execlp (tfadminPkg, tfadminPkg,
				"PackageMgr", "-D", curalias, NULL);
		else {
			char *packager;
			packager = GetXWINHome ("bin/PackageMgr");
			execlp (packager, packager, "-D", curalias);
		}
	}
	else if (diagn & DTAM_FS_TYPE) {
		if (_DtamIsOwner(OWN_FLOPPY)) {
			Mode = UnixFS_Mode;
			SetOwner (atomName, dropstr);
			if (attempt_mount(diagn, curdev) & DTAM_CANT_MOUNT)
				BaseNotice(DTAM_CANT_MOUNT, False);
			else if (attempt_open(curalias, True) & CANT_OPEN)
			{
				if (disk_tid)
					XtRemoveTimeOut(disk_tid);
				(void) DtamUnMount(_dtam_mntpt);
				BaseNotice(CANT_OPEN, False);
			}

			if (opflag == DO_COPY) {
				if (attempt_copy(curdev,dropstr) != 0)
					BaseNotice(NO_ROOM, True);
			}
		}
		else
			BaseNotice(diagn = DTAM_NOT_OWNER, False);
	}
	else {
		switch(diagn) {
	case DTAM_DOS_DISK:	Mode = DosFS_Mode;
				SetOwner (atomName, dropstr);
				CreateDOSWindow(w_toplevel, dropstr);
				FreezeMinWindowSize(dbase.shell);
				return;
	case DTAM_TAR:
	case DTAM_BACKUP:
	case DTAM_CPIO:
	case DTAM_CPIO_BINARY:
	case DTAM_CPIO_ODH_C:	if (opflag != DO_COPY) {
					newAtom =
					    XtMalloc (strlen (RST_ATOM_PFX) +
					    strlen (atomName) + 1);
					strcpy (newAtom, RST_ATOM_PFX);
					strcat (newAtom, atomName);
					Mode = Restore_Mode;
					SetOwner (newAtom, dropstr);
					CreateRestoreWindow(w_toplevel);
					FreezeMinWindowSize(rbase.shell);
					return;
				}
				/* else fall through to backup cases */
	case DTAM_UNFORMATTED:	if (opflag != DO_COPY) {
					newAtom =
					    XtMalloc (strlen (FMT_ATOM_PFX) +
					    strlen (atomName) + 1);
					strcpy (newAtom, FMT_ATOM_PFX);
					strcat (newAtom, atomName);
					Mode = Format_Mode;
					SetOwner (newAtom, dropstr);
					CreateFormatWindow(w_toplevel);
					return;
				}
				/* else fall through to backup cases */
	default:
	case DTAM_UNDIAGNOSED:
	case DTAM_UNKNOWN:	Mode = Backup_Mode;
				SetOwner (atomName, dropstr);
				CreateBackupWindow(w_toplevel, dropstr);
				FreezeMinWindowSize(bbase.shell);
				return;
		}
	}
}

void	main(argc, argv)
	int	argc;
	char	*argv[];
{
extern	char	*optarg;

#ifdef UseXtApp
	XtAppContext	app_con;
#endif
	char	*atomName;
	int	opflag = 0;
	int	n, m;

#ifdef MEMUTIL
	InitializeMemutil();
#endif

        (void)setsid();		/* become a session leader (divorce dtm) */

        /* undo some of the stuff we inherit from dtm */
        sigset(SIGCHLD, SIG_DFL);
        sigset(SIGINT,  SIG_DFL);
        sigset(SIGQUIT, SIG_DFL);
        sigset(SIGTERM, SIG_DFL);
        

	for (m = n = 0; n < argc; n++)
		m += strlen(argv[n])+1;
	*save_command = (char *)CALLOC(m+1,sizeof(char));
	for (n = 0; n < argc; n++)
		strcat(strcat(*save_command,argv[n])," ");
	OlToolkitInitialize(&argc, argv, NULL);

#ifdef UseXtApp
	w_toplevel = XtAppInitialize(
			&app_con,		/* app_context_return	*/
			"MediaMgr",		/* application_class	*/
			(XrmOptionDescList)NULL,/* options		*/
			(Cardinal)0,		/* num_options		*/
			&argc,			/* argc_in_out		*/
			argv,			/* argv_in_out		*/
			(String)NULL,		/* fallback_resources	*/
			(ArgList)NULL,		/* args			*/
			(Cardinal)0		/* num_args		*/
	);
#else
	w_toplevel = XtInitialize("MediaMgr", "MediaMgr", NULL, 0, &argc, argv);
#endif

	/*
	 *	note - this will not be realized UNLESS there is some
	 *	honest work to do in a base window (backup, restore, format
	 *	or package management.  Notices (e.g. to insert floppies)
	 *	and the monitoring of mounted devices can go on without an
	 *	irrelevant visible thingy on the screen.
	 */
	theDisplay = XtDisplay(w_toplevel);
	theScreen = XtScreen(w_toplevel);
	x3mm = OlMMToPixel(OL_HORIZONTAL,3);
	y3mm = OlMMToPixel(OL_VERTICAL,3);
	DtInitialize(w_toplevel);
	def_font = _OlGetDefaultFont(w_toplevel, OlDefaultFont);
	bld_font = _OlGetDefaultFont(w_toplevel, OlDefaultNoticeFont);
	_DtamWMProtocols(w_toplevel);
/*
 *	analyze calling situation; if invoked with a specific flag, go to
 *	the appropriate main window, if by double-clicking on a specific
 *	device, diagnose this for its contents (if any) and again go the
 *	the appropriate window -- or if none is determinable, a notice with
 *	a diagnostic message displayed. 
 */
	while ((n = getopt(argc, argv, "BC:D:FXIO:RWL")) != EOF) {
                switch (n) {
			case 'B':	opflag = DO_BACKUP;
					break;
			case 'C':	/* receipt of dropped object */
                                        dropstr = optarg;
					if (!opflag)
						opflag = DO_COPY;
                                        break;
                        case 'D':       curalias = optarg;
					break;
			case 'F':	opflag = DO_FORMAT;
					break;
			case 'O':	/* "open" the arg (dropped file) */
					cur_file = STRDUP(optarg);
					break;
			case 'R':	opflag = DO_RESTOR;
					break;
			case 'X':	backup_flag = 1;
					break;
			case 'L':	restricted_flag = 1;
			                break;
			default:	break;
                }
        }
        if (system("/sbin/tfadmin -t cpio >/dev/null 2>&1") == 0)
	    CpioCmd = "/sbin/tfadmin cpio";
        else
	    CpioCmd = "/usr/bin/cpio";

        if (system("/sbin/tfadmin -t PackageMgr >/dev/null 2>&1") == 0)
	    packageOwner = True;
        else
	    packageOwner = False;

	if (!curalias) {
		atomName = "DtMedia";
		curalias = "diskette1";
		if (opflag == DO_BACKUP || opflag == DO_RESTOR) {
			curdev = DtamGetDev("tape",FIRST);
			if (curdev) {
				char	*cdev = DtamDevAttr(curdev,CDEVICE);
				if (cdev && access(cdev,R_OK)==0) {
					curalias = DtamDevAttr(curdev,ALIAS);
				}
				else {
					FREE(curdev);
					curdev = NULL;
					if (cdev)
						FREE(cdev); 
				}
			}
		}
	}
	else
		atomName = curalias;

	if (!curdev)
		curdev = DtamGetDev(curalias,FIRST);
	SelectAction(opflag, -1, atomName);	/* go to appropriate base */
						/* window or give notice. */
#ifdef UseXtApp
	XtAppMainLoop(app_con);
#else
	XtMainLoop();
#endif
}
/*
 * FreezeMinWindowSize
 *
 */

static void
FreezeMinWindowSize(Widget shell)
{
	Arg arg[4];
	Dimension width;
	Dimension height;

	XtSetArg(arg[0], XtNwidth, &width);
	XtSetArg(arg[1], XtNheight, &height);
	XtGetValues(shell, arg, 2);

#ifdef DEBUG
	(void)fprintf(stderr,"set min size: %dx%d\n", (int)width, (int)height);
#endif

	XtSetArg(arg[0], XtNminWidth, width);
	XtSetArg(arg[1], XtNminHeight, height);
	XtSetValues(shell, arg, 2);

} /* end of FreezeMinWindowSize */

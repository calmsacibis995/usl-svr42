/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:floppy/restore.c	1.44"
#endif

#include <fcntl.h>
#include <pfmt.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "media.h"

extern	void		NotePidFiles();

extern	FILE		*ptyfp;
extern	char		*ptyname;
extern	char		*flpindex;
extern	char		*pidindex;
extern	char		*EOM;
extern	char		*volume;
extern	int		vol_count;
extern	Boolean		wait;
extern	XtIntervalId	TID;
extern	char            *CpioCmd;

void	restoreCB();
void	showFilesCB();
void	selectCB();
void	unselectCB();
void	insertCB();
void	rhelpCB();
void	rkillCB();
void	CpioPrep();
void	InsertNotice();
int	StoreIndexNumber(char *);
void	prevGroupCB();
void	nextGroupCB();
void	tooManyCB();
void	fileSelectCB();
void	fileUnselectCB();
extern	char	*nbfgets (char *buf, int cnt, FILE *pFile);

static MenuItems raction_menu_item[] = {
	{ TRUE, label_restore,mnemonic_restore, 0, restoreCB},
	{ TRUE, label_show,   mnemonic_show, 0, showFilesCB},
	{ TRUE, label_exit,   mnemonic_exit, 0, exitCB},
	{ NULL }
};

static MenuItems redit_menu_item[] = {
	{ TRUE, label_select,  mnemonic_select, 0, selectCB},
	{ TRUE, label_unselect,mnemonic_unselect, 0, unselectCB},
	{ NULL }
};

static HelpInfo RHelpIntro	= { 0, "", BHELP_PATH, help_rst_intro };
static HelpInfo RHelpConfirm	= { 0, "", BHELP_PATH, help_rst_confirm };
static HelpInfo RHelpTOC	= { 0, "", BHELP_PATH, NULL };
static HelpInfo RHelpDesk       = { 0, "", BHELP_PATH, "HelpDesk"  };

static MenuItems rhelp_menu_item[] = {  
	{ TRUE, label_bkrst, mnemonic_bkrst, 0, rhelpCB, (char *)&RHelpIntro },
	{ TRUE, label_toc,   mnemonic_toc,   0, rhelpCB, (char *)&RHelpTOC },
	{ TRUE, label_hlpdsk,mnemonic_hlpdsk,0, rhelpCB, (char *)&RHelpDesk },
	{ NULL }
};

static MenuGizmo raction_menu = {0, "action_menu", NULL, raction_menu_item};
static MenuGizmo redit_menu   = {0, "edit_menu", NULL, redit_menu_item};
static MenuGizmo rhelp_menu   = {0, "help_menu", NULL, rhelp_menu_item};

static MenuItems rmain_menu_item[] = {
	{ TRUE, label_action, mnemonic_action, (Gizmo) &raction_menu},
	{ TRUE, label_edit,   mnemonic_edit, (Gizmo) &redit_menu},
	{ TRUE, label_help,   mnemonic_help, (Gizmo) &rhelp_menu},
	{ NULL }
};
static MenuGizmo rmenu_bar = { 0, "menu_bar", NULL, rmain_menu_item};

BaseWindowGizmo rbase = {0, "base", label_restore, (Gizmo)&rmenu_bar,
	NULL, 0, label_restore, "restore48.icon", " ", " ", 90 };

static MenuItems rwatch_menu_item[] = {  
	{ TRUE, label_cancel, mnemonic_cancel, 0, rkillCB, NULL },
	{ TRUE, label_help,   mnemonic_help, 0, rhelpCB, (char *)&RHelpIntro },
	{ NULL }
};
static MenuGizmo rwatch_menu = {0, "rwatch_menu", NULL, rwatch_menu_item};
static PopupGizmo rwatch = {0, "popup", title_doingRst, (Gizmo)&rwatch_menu};

static MenuItems rnote_menu_item[] = {  
	{ TRUE, label_continue, mnemonic_continue, 0, insertCB, NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, rkillCB, NULL },
	{ TRUE, label_help,   mnemonic_help, 0, rhelpCB,(char *)&RHelpConfirm },
	{ NULL }
};
static MenuGizmo rnote_menu = {0, "rnote_menu", NULL, rnote_menu_item};
static ModalGizmo rnote = {0, "", title_confirmRst, (Gizmo)&rnote_menu};

ExclItem	RstType[2];
DevItem		RstDevice[N_DEVS];

char	*doc_alias;
char	*rst_doc;
int	last_restore;
int	restore_count;

typedef	struct	{
	XtArgVal	f_name;
	XtArgVal	f_set;	
} ListItem, *ListPtr;

static char * flatMenuFields[] =
   {
      XtNsensitive,  /* sensitive                      */
      XtNlabel,      /* label                          */
      XtNuserData,   /* mnemonic string                */
      XtNuserData,   /* nextTier | resource_value      */
      XtNselectProc, /* function                       */
      XtNclientData, /* client_data                    */
      XtNset,        /* set                            */
      XtNpopupMenu,  /* button                         */
      XtNuserData,   /* mnemonic                       */
   };

#define	INIT_FILELIST_SIZE	50	
#define	INIT_GROUP_SIZE  	5000
static int filelist_size =	0;
static Cardinal group_size = INIT_GROUP_SIZE; /* max items that fit in a list */
static int show_num;
static int group_show;		/* which one to show */
static int group_max;		/* total number to show */
static int select_count;	/* number currently selected */


static MenuItems change_group_menu_items[] = {  
	{ FALSE, NULL, NULL, 0, prevGroupCB, NULL },
	{ FALSE, NULL, NULL, 0, nextGroupCB, NULL }
};
			
#define XtNitemsLimitExceeded "itemsLimitExceeded"

ListPtr	filelist = (ListPtr)NULL;
int	file_count = 0;
int	rst_format;
Boolean	init_cpio;
char	*bkup_index = NULL;
char	rst_buf[256];
char	*ListFields[] = { XtNlabel, XtNset };

Widget	w_rstmsg, w_file, w_list, w_opt, w_group, w_groupcaption;

char	*ERR_fld  = " ERROR:";
char	*ERR_tag  = "UX:cpio: ERROR:";
char	*HALT_tag = "UX:cpio: HALT:";
char	*END_tag  = " blocks";
char	*SKP1_tag = "Existing \"";
char	*SKP2_tag = "\" same age or newer";

void	SetLocaleTags(void)
{
	FILE	*tmpfp;
	char	fname[16];
	char	buf[BUFSIZ];
	char	*ptr;

	sprintf(fname,"/tmp/%d", getpid());
	if (tmpfp = fopen(fname,"w")) {
		setlocale(LC_ALL, "");
		pfmt(tmpfp, MM_ERROR, NULL);
		fputc('\n',tmpfp);
		setlabel("UX:cpio");
		pfmt(tmpfp, MM_ERROR, NULL);
		fputc('\n',tmpfp);
		pfmt(tmpfp, MM_HALT, NULL);
		fputc('\n',tmpfp);
		fclose(tmpfp);
		if (tmpfp = fopen(fname,"r")) {
			if (fgets(buf, BUFSIZ, tmpfp)) {
				ERR_fld = STRDUP(buf);
				ERR_fld[strlen(ERR_fld)-1] = 0;
				if (fgets(buf, BUFSIZ, tmpfp)) {
					ERR_tag = STRDUP(buf);
					ERR_tag[strlen(ERR_tag)-1] = 0;
		 			if (fgets(buf, BUFSIZ, tmpfp)) {
						HALT_tag = STRDUP(buf);
						HALT_tag[strlen(HALT_tag)-1]=0;
					}
				}
			}
			fclose(tmpfp);
		}
		setlabel(NULL);
		unlink(fname);
	}
	setcat("uxcore.abi");
	sprintf(buf, gettxt(":46","Existing \"%s\" same age or newer"), "QQQ");
	if (ptr = strstr(buf, "QQQ")) {
		*ptr = '\0';
		SKP1_tag = STRDUP(buf);
		SKP2_tag = STRDUP(ptr+3);
	}
	sprintf(buf, gettxt(":57","%ld blocks"), 999);
	if (ptr = strstr(buf,"999"))
		END_tag = STRDUP(ptr+3);
	setcat("");
}

void	RstSummary()
{
	char	buf[80];

	sprintf(buf,GetGizmoText(string_rstSummary),GetGizmoText(label_restore),
				curdev? DtamDevDesc(curdev): rst_doc);
	FooterMsg(rbase, buf);
}

void	rhelpCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	HelpInfo *help = (HelpInfo *) client_data;

	FooterMsg(rbase, NULL);
	help->app_title	= 
	help->title	= GetGizmoText(label_restore);
	help->section = GetGizmoText(STRDUP(help->section));
	PostGizmoHelp(rbase.shell, help);
}

void	DropRstWatch(XtPointer closure, XtIntervalId id)
{
	XDefineCursor(theDisplay, XtWindow(rwatch.shell),
						GetOlStandardCursor(theScreen));
/*	XDefineCursor(theDisplay, XtWindow(w_gauge),
/*						GetOlStandardCursor(theScreen));
*/	XDefineCursor(theDisplay, XtWindow(rbase.shell),
						GetOlStandardCursor(theScreen));
	if (rwatch.shell)
		BringDownPopup(rwatch.shell);
	if (rnote.shell)
		BringDownPopup(rnote.shell);
	if (bkup_index) {
		unlink(bkup_index);
		FREE(bkup_index);
		bkup_index = NULL;
	}
}

void	StartRstWatch(char *msg)
{
	Widget	w_up;

	if (!rwatch.shell) {
		CreateGizmo(rbase.shell, PopupGizmoClass, &rwatch, NULL, 0);

		XtSetArg(arg[0], XtNwindowHeader, FALSE);
		XtSetArg(arg[1], XtNwidth,	  36*x3mm);
		XtSetArg(arg[2], XtNresize,	  FALSE);
		XtSetValues(rwatch.shell, arg, 1);

		XtSetArg(arg[0], XtNupperControlArea, &w_up);
		XtGetValues(rwatch.shell, arg, 1);

		XtSetArg(arg[0], XtNlayoutType,		OL_FIXEDCOLS);
		XtSetArg(arg[1], XtNalignCaptions,	TRUE);
		XtSetArg(arg[2], XtNcenter,		TRUE);
		XtSetArg(arg[3], XtNwidth,		36*x3mm);
		XtSetArg(arg[4], XtNhPad,		x3mm);
		XtSetArg(arg[5], XtNvPad,		y3mm);
		XtSetArg(arg[6], XtNvSpace,		y3mm);
		XtSetValues(w_up, arg, 7);

		XtSetArg(arg[0], XtNheight, 3*y3mm);
		XtCreateManagedWidget("spacer", rectObjClass, w_up, arg, 1);

		XtSetArg(arg[0], XtNalignment,		OL_CENTER);
		XtSetArg(arg[1], XtNgravity,		CenterGravity);
		XtSetArg(arg[2], XtNwidth,		36*x3mm);
		XtSetArg(arg[3], XtNfont, 		bld_font);
		w_rstmsg = XtCreateManagedWidget("text",
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
	XtSetArg(arg[0], XtNstring, (XtArgVal)msg);
	XtSetValues(w_rstmsg, arg, 1);
	MapGizmo(PopupGizmoClass, &rwatch);
	XDefineCursor(theDisplay, XtWindow(rwatch.shell),
						GetOlBusyCursor(theScreen));
	XDefineCursor(theDisplay, XtWindow(rbase.shell),
						GetOlBusyCursor(theScreen));
}

void	rkillCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	char	buf[80];

	FooterMsg(rbase, NULL);
	if (cmdfp[1] && cmdfp[0]) {
		_Dtam_p3close(cmdfp, SIGTERM);
		cmdfp[1] = cmdfp[0] = (FILE *)NULL;
	}
	else if (cmdfp[1]) {	/* in midst of reading the index file */
		fclose(cmdfp[1]);
		cmdfp[1] = (FILE *)NULL;
	}
	if (ptyfp) {
		fclose(ptyfp);
		ptyfp = (FILE *)NULL;
	}
	if (TID)
		XtRemoveTimeOut(TID);
	sprintf(buf,GetGizmoText(string_opKilled),GetGizmoText(label_restore));
	FooterMsg(rbase, buf);
	XtAddTimeOut(100, (XtTimerCallbackProc)DropRstWatch, (XtPointer)NULL);
}

static	int
SetOrUnsetFiles(Boolean flag, Boolean touch)
{
	register int	n;

	if (filelist)
		for (n = 0; n < file_count; n++)
			filelist[n].f_set = (XtArgVal)flag;
	if (touch)
	{
		XtSetArg(arg[0], XtNitemsTouched, TRUE);
		XtSetValues(w_list, arg, 1);
	}
}

static	int
CheckTargetFile()
{
	int	n;
	char	buf[80];

	XtSetArg(arg[0], XtNstring, &rst_doc);
	XtGetValues(w_file, arg, 1);
	switch (n = diagnose(rst_doc)) {
		case BAD_DEVICE:
		case UNREADABLE:
			sprintf(buf, GetGizmoText(string_unreadFile), rst_doc);
			FooterMsg(rbase, buf);
			break;
		case TAR:
		case BACKUP:
		case CPIO:
			rst_format = n;
			break;
		default:
			sprintf(buf, GetGizmoText(string_notaBkup), rst_doc);
			FooterMsg(rbase, buf);
			break;
	}
	return n;
}

void	AddToFileList(char *name)
{
	if (name[strlen(name)-1] == '\n')
		name[strlen(name)-1] = '\0';
	if (file_count >= filelist_size) {
		if (filelist_size == 0) filelist_size = INIT_FILELIST_SIZE;
		else filelist_size *= 2;
		filelist = (ListPtr)REALLOC((void *)filelist,
			filelist_size*sizeof(ListItem));
	}
	filelist[file_count].f_name = (XtArgVal)STRDUP(name);
	filelist[file_count++].f_set = (XtArgVal)FALSE;
}

static	char	buf[BUFSIZ];
static	char	BLKS[] = "BLOCKS=";

void	FetchFiles(XtPointer closure, XtIntervalId id)
{
	static		Boolean	restart;
	char		msgbuf[128];
	static char	index_file[50] = { NULL };
	char		index_buf[BUFSIZ];
	FILE		*index_fp;
	char		*dev, *ptr;
	int		timer;
	int		fdmaster;

	if (wait)
		timer = 2000;
	else if (init_cpio) {
		init_cpio = restart = FALSE;
		file_count = 0;	/* should free existing filelist! */
		timer = 2000;

		switch (rst_format) {
		case DTAM_TAR:
			ptr = strstr(rst_buf, "Bct         ");
			strncpy(ptr, "t -H tar ", 9);
			break;
		case DTAM_BACKUP:
			sprintf (index_file,"/tmp/flp_index.%-d", 
						StoreIndexNumber(NULL));
			if (bkup_index) {
				unlink(bkup_index);
				FREE(bkup_index);
				bkup_index = NULL;
			}
			unlink(index_file);
			restart = TRUE;
			ptr = strstr(rst_buf,"Bct   ");
			strncpy (ptr, "Bcv   ", 6);
			if (curdev)
				*strstr(rst_buf, "-M") = '\0';
			strcat (rst_buf," ");
			strcat (rst_buf,index_file);/* just read in one file */
			strcat(rst_buf, " 2>&1");
			break;

		}
		if (curdev == NULL || rst_format == BACKUP)
			ptyfp = (FILE *)NULL;
		else {
		/*
		 *	prepare pseudo-tty to use for volume changes
		 */
			fdmaster = open("/dev/ptmx", O_RDWR);
			grantpt(fdmaster);
			unlockpt(fdmaster);
			ptyname = ptsname(fdmaster);
			fcntl(fdmaster, F_SETFL, O_NONBLOCK);
			ptyfp = fdopen(fdmaster, "r+");
			setbuf(ptyfp, NULL);
			ptr = strstr(rst_buf, "-G ");
			strcpy(ptr+3, ptyname);
		}
		_Dtam_p3open(rst_buf, cmdfp, TRUE);
	}
	else {
		timer = 100;
		buf[0] = '\0';
		if (ptyfp && nbfgets(buf, BUFSIZ, ptyfp)) {
			if (strncmp(buf, EOM, 4)==0) {
				wait = TRUE;
				vol_count = atoi(buf+4);
				InsertNotice(&rnote, NO_DISK);
				timer = 5000;
				rewind(ptyfp);
			}
		}
		buf[0] = '\0';
		while (nbfgets(buf, BUFSIZ, cmdfp[1])) {
			int	n;
			if (*buf == '\0')
				break;
			if (strstr(buf,"BLOCKS=")!=NULL
			||  strstr(buf,END_tag)!=NULL
			|| (*index_file != 0 && 
					strncmp(buf,index_file,strlen(index_file)) == 0))
			{
			/*
			 *	done!
			 */
				if (*index_file == 0)
					_Dtam_p3close(cmdfp, 0);
				else {	/* we have the index file; process it */
					_Dtam_p3close(cmdfp, SIGTERM);
					if ((index_fp = fopen (index_file, "r")) == NULL) {
						/* do something better here */
						fprintf (stderr, "MediaMgr, FetchFiles: unable to read index file\n");
						AddToFileList (index_file);
					}
					else {
						nbfgets(index_buf,BUFSIZ,index_fp); /* skip index file */
						while (nbfgets (index_buf, BUFSIZ, index_fp))
						{
							if (strncmp(index_buf,BLKS,strlen(BLKS)) != 0)
							{ /* find tab then skip it */
								char *index_ptr = index_buf;
								while (*index_ptr != '\t')
									index_ptr++;
								AddToFileList(++index_ptr);
							}
						}
						fclose (index_fp);
						unlink(index_file);
						*index_file = 0;
					}
				}
				if (nbfgets(buf, BUFSIZ, cmdfp[1]))
					;
				cmdfp[1] = cmdfp[0] = (FILE *)NULL;
				if (bkup_index)
				{
					unlink(bkup_index);
					FREE(bkup_index);
					bkup_index = NULL;
				}
				if (ptyfp) {
					fclose(ptyfp);
					ptyfp = (FILE *)NULL;
				}
				XtAddTimeOut(100,
					(XtTimerCallbackProc)DropRstWatch,NULL);
				TID = 0;
				SetOrUnsetFiles(TRUE,FALSE);	/* set true; don't update X */

				/* now display the first group of files */
				show_num = file_count>group_size ? 
							group_size:file_count;
				XtSetArg(arg[0], XtNitems,        filelist);
				XtSetArg(arg[1], XtNnumItems,     show_num);
				XtSetArg(arg[2], XtNviewHeight,   7);
				XtSetArg(arg[3], XtNviewItemIndex,show_num-1);
				XtSetValues(w_list, arg, 4);
				if (file_count > group_size)
				{	/* more than one group of files; display first group */
					show_num = group_size;
					XtSetArg(arg[0],XtNitems,filelist);
					XtSetArg(arg[1],XtNnumItems,show_num);
					XtSetArg(arg[2],XtNviewHeight, 7);
					XtSetArg(arg[3],XtNviewItemIndex,
					    show_num - 1);
					XtSetArg(arg[4],XtNitemsTouched, TRUE);
					XtSetValues(w_list, arg, 5);
					group_show = 0;
					group_max  = file_count/show_num;
					if (file_count % show_num == 0)
						group_max--;
					/* buttons visible; next sensitive */
					XtSetArg(arg[0], 
					    XtNmappedWhenManaged,	TRUE);
					XtSetValues(w_groupcaption, arg, 1);
					XtSetValues(w_group, arg, 1);
					change_group_menu_items[0].sensitive = 
					    FALSE;
					change_group_menu_items[1].sensitive = 
					    TRUE;
					XtSetArg(arg[0], XtNitems,
					    change_group_menu_items);
					XtSetArg(arg[1], XtNnumItems,
					    XtNumber(change_group_menu_items));
					XtSetValues(w_group, arg, 1);
				}
				else
				{
					/* make sure buttons invisible */
					XtSetArg(arg[0], 
					    XtNmappedWhenManaged, FALSE);
					XtSetValues(w_groupcaption, arg, 1);
					XtSetValues(w_group, arg, 1);
				}
				/* set footer message with number of files */
				select_count = file_count;
				sprintf(msgbuf,GetGizmoText(string_selected),
					select_count,file_count);
				FooterMsg(rbase, msgbuf);

				return;
			}
			else if (rst_format == BACKUP && !restart) {
				AddToFileList(strchr(buf,'\t')+1);
			}
			else if (strncmp(buf,ERR_tag,n=strlen(ERR_tag))==0) {
				XtSetArg(arg[0], XtNstring, (XtArgVal)buf+n);
				XtSetValues(w_rstmsg, arg, 1);
			}
			else {
				buf[strlen(buf)-1] = '\0';
				if (restart) {
					_Dtam_p3close(cmdfp, 0);
					cmdfp[0] = (FILE *)NULL;
					cmdfp[1] = fopen(buf,"r");
					fcntl(fileno(cmdfp[1]), F_SETFL,
								O_NONBLOCK);
					restart = FALSE;
					bkup_index = STRDUP(buf);
				}
				else 
					AddToFileList(buf);
			}
		}
	}
	TID = XtAddTimeOut(timer, (XtTimerCallbackProc)FetchFiles, NULL);
}

void	insertCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	char	buf[128];
	int	diagn = DtamCheckMedia(curalias);

	FooterMsg(rbase, NULL);
	if (vol_count == 1)
		switch(diagn) {

	case DTAM_BACKUP:
	case DTAM_CPIO:
	case DTAM_TAR:	BringDownPopup(rnote.shell);
			wait = FALSE;
			rst_format = diagn;
			break; 

	default:	sprintf(buf, GetGizmoText(string_notaBkup), volume);
			SetModalGizmoMessage(&rnote, buf);
			break;
		}
	else
		switch(diagn) {
	/* others? */
	case DTAM_UNKNOWN:	BringDownPopup(rnote.shell);
			wait = FALSE;
			if (ptyfp) {
				fputs("\n", ptyfp);
				fflush(ptyfp);
			}
			break;

	default:	sprintf(buf, GetGizmoText(string_notaBkup), volume);
			SetModalGizmoMessage(&rnote, buf);
			break;
		}
}

void	InsertNotice(ModalGizmo *note, int diagn)
{
	char	*button, volstr[64], msgbuf[64];

	sprintf(volstr, GetGizmoText(string_fmtVolume), volume, vol_count);
	button = note->menu->items->label;
	sprintf(msgbuf, GetGizmoText(string_insMsg), volstr, button);
	SetModalGizmoMessage(note, msgbuf);
	MapGizmo(ModalGizmoClass, note);
}

void	selectCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	char	msgbuf[128];

	select_count = file_count;
	sprintf(msgbuf,GetGizmoText(string_selected),select_count,file_count);
	FooterMsg(rbase, msgbuf);
	SetOrUnsetFiles(TRUE,TRUE);	/* select ALL: set true; update X */
}

void	unselectCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	char	msgbuf[128];

	select_count = 0;
	sprintf(msgbuf,GetGizmoText(string_selected),select_count,file_count);
	FooterMsg(rbase, msgbuf);
	SetOrUnsetFiles(FALSE,TRUE);	/* unselect ALL: set false; update X */
}

void	showFilesCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	int	diagn;
	char	*dev;

	FooterMsg(rbase, NULL);
	CpioPrep();
	dev = curdev? DtamDevAttr(curdev, "cdevice"): rst_doc;
	sprintf(rst_buf, "%s -iBct         -I %s", CpioCmd, dev);
	if (curdev) {
		FREE(dev);
		sprintf(rst_buf+strlen(rst_buf), " -M \"%s\n\" -G %s",
						EOM, "/dev/ptsdummyname");
	}
	if (curdev == NULL) {
		 switch (diagn = CheckTargetFile()) {
			default:	return;
			case DTAM_TAR:
			case DTAM_BACKUP:
			case DTAM_CPIO:	rst_format = diagn;
					break;	/* ok */
		}
	}
	else if ((diagn=DtamCheckMedia(curalias)) == NO_DISK) {
		InsertNotice(&rnote, diagn);
		return;
	}
	XtSetArg(arg[0], XtNmappedWhenManaged, TRUE);
	XtSetValues(w_list, arg, 1);
	StartRstWatch(GetGizmoText(string_readIndex));
	XtAddTimeOut(wait? 2500: 100, (XtTimerCallbackProc)FetchFiles, NULL);
}

static	void
CountSelFile(char *name)
{
	int	n;

	for (n = last_restore; n; n--)
		if (filelist[n].f_set && strcmp((char *)filelist[n].f_name,name)==0) {
			--restore_count;
			return;
		}
}

static	char *
SkipMsg(char *line)
{
#define	TRUNC	64
	char	*ptr, fname[TRUNC], buf[256];
	int	n;

	ptr = strchr(line,'"')+1;
	*strchr(ptr,'"') = '\0';
	if (last_restore)
		CountSelFile(ptr);
	if ((n = strlen(ptr)) >= TRUNC) {
		strcpy(fname,"...");
		strcat(fname,ptr+4+n-TRUNC);
	}
	else {
		strcpy(fname, ptr);
		while (n < TRUNC-1)
			fname[n++] = ' ';
		fname[n] = '\0';
	}
	sprintf(buf, GetGizmoText(string_skipFile), fname);
	return buf;
}

void	StopCpio(char *msg)
{
	char	bbuf[80];

	sprintf(bbuf, GetGizmoText(string_opOK), GetGizmoText(label_restore));
	if (msg)
		strcat(strcat(bbuf, " - "), msg);
	XtSetArg(arg[0], XtNstring, bbuf);
	XtSetValues(w_rstmsg, arg, 1);
	FooterMsg(rbase, bbuf);
	if (nbfgets(buf, BUFSIZ, cmdfp[1]))	/* discard pending msgs */
		;
/*	XtSetArg(arg[0],  XtNmappedWhenManaged, FALSE);
/*	XtSetValues(w_gauge, arg, 1);
*/	_Dtam_p3close(cmdfp, msg ? 0: SIGTERM);
	cmdfp[1] = cmdfp[0] = (FILE *)NULL;
	if (ptyfp) {
		fclose(ptyfp);
		ptyfp = (FILE *)NULL;
	}
	XtAddTimeOut(1000, (XtTimerCallbackProc)DropRstWatch,NULL);
	TID = 0;
}

void	WatchCpio(XtPointer closure, XtIntervalId intid)
{
static	int	count = 0;
	int	timer;
	int	fdmaster;
	char	buf[BUFSIZ];

	if (wait)
		timer = 2000;
	else if (init_cpio) {
		init_cpio = FALSE;
		count = 0;
		timer = 2000;
		if (rst_format == TAR) {
			char	*ptr;
			ptr = strstr(rst_buf, "Bdlcv        ");
			if (ptr) 
				strncpy(ptr, "dv -H tar ", 10);
			else {
				ptr = strstr(rst_buf, "Bdlcvu       ");
				if (ptr) strncpy(ptr, "dvu -H tar ", 11);
				else { /* shouldn't happen, but ... */
					fprintf (stderr, "MediaMgr, WatchCpio: invalid cpio command\n");
					return;	/* is this good enough? */
				}
			}
		}
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
			strcpy(strstr(rst_buf, "-G ")+3, ptyname);
		}
		_Dtam_p3open(rst_buf, cmdfp, TRUE);
	}
	else {
		timer = 100;
		buf[0] = '\0';
		if (ptyfp && nbfgets(buf, BUFSIZ, ptyfp)) {
			if (strncmp(buf, EOM, 4)==0) {
				wait = TRUE;
				vol_count = atoi(buf+4);;
				InsertNotice(&rnote, NO_DISK);
				timer = 5000;
				rewind(ptyfp);
			}
		}
		buf[0] = '\0';
		if (nbfgets(buf, BUFSIZ, cmdfp[1])) {
			int	n;
			if (*buf == '\0')
				;
			else if (strstr(buf, END_tag)) {
			/*
			 *	done!
			 */
				StopCpio(NULL);
				return;
			}
			else if (strncmp(buf,HALT_tag,n=strlen(HALT_tag))==0) {
				StopCpio(buf+n);
				return;
			}
			else if (strncmp(buf,ERR_tag,n=strlen(ERR_tag))==0) {
				XtSetArg(arg[0], XtNstring, buf+n); 
				XtSetValues(w_rstmsg, arg, 1);
			}
			else if (strncmp(buf,SKP1_tag,n=strlen(SKP1_tag))==0
			     &&  strstr(buf+n, SKP2_tag)!=0) {
				XtSetArg(arg[0],XtNstring,SkipMsg(buf));
				XtSetValues(w_rstmsg, arg, 1);
			}
			else {	/* one file was read in; process it */
				buf[strlen(buf)-1] = '\0';
				FooterMsg(rbase, buf);
				if (count++ == 0 && rst_format == BACKUP) {
					if (bkup_index)
					{
						unlink(bkup_index);
						FREE(bkup_index);
						bkup_index = NULL;
					}
				}
				if (last_restore)
					CountSelFile(buf);
			}
			if (last_restore && restore_count == 0) {
				StopCpio(NULL);
				return;
			}
		}
	}
	TID = XtAddTimeOut(timer, (XtTimerCallbackProc)WatchCpio, NULL);
}

#define	NONE_SET	0
#define SOME_SET	1
#define	ALL_SET		2

PartialRestore()
{
	Boolean	anyset = FALSE, anyunset = FALSE;

	if (filelist) {
		register int n;
		for (n= 0; n < file_count; n++) {
			if (filelist[n].f_set)
				anyset = TRUE;
			else
				anyunset = TRUE;
			if (anyset && anyunset)
				return SOME_SET;
		}
	} 
	if (anyunset && !anyset)
		return NONE_SET;
	else
		return ALL_SET;
}

void	CpioPrep()
{
	int	n;
	char	buf[80];

	unlink(pidindex);
	if (bkup_index)
	{
	        FREE(bkup_index);
	        bkup_index = NULL;
	}

	init_cpio = TRUE;
	if (curdev) {
		vol_count = 1;
		if (volume)
			FREE(volume);
		volume = DtamDevAttr(curdev, "volume");
		wait = TRUE;
		switch (n = DtamCheckMedia(curalias)) {
		case DTAM_BACKUP:	
		case DTAM_TAR:
		case DTAM_CPIO:	wait = FALSE;	/* ok */
				rst_format = n;
				break;
		case DTAM_NO_DISK:
				InsertNotice(&rnote, n);
				break;
		case DTAM_UNREADABLE:
				InsertNotice(&rnote, n);
				sprintf(buf, GetGizmoText(string_unreadFile),
								volume);
				SetModalGizmoMessage(&rnote, buf);
				break;
		default:	InsertNotice(&rnote, n);
				sprintf(buf, GetGizmoText(string_notaBkup),
								volume);
				SetModalGizmoMessage(&rnote, buf);
		}
	}
	else  {
		switch (CheckTargetFile()) {
		case DTAM_BACKUP:	
		case DTAM_TAR:
		case DTAM_CPIO:	wait = FALSE;	/* ok */
				rst_format = n;
				break;
		default:	return;
		}
	}
}

void	restoreCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	FILE	*fp;
	char	*dev, *ptr;
	Boolean	chk_flag;
	int	n, rst_flag;

	FooterMsg(rbase, NULL);
	CpioPrep();
	if (PartialRestore() == NONE_SET) {
		FooterMsg(rbase, GetGizmoText(string_noneset));
		return;
	}
	restore_count = last_restore = 0;
	switch (rst_flag = PartialRestore()) {
	case NONE_SET:
		FooterMsg(rbase, GetGizmoText(string_noneset));
		return;
	case SOME_SET:
		fp = fopen(pidindex, "w");
		for (n = 0; n < file_count; n++)
			if (filelist[n].f_set) {
				last_restore = n;
				++restore_count;
				fprintf(fp, "%s\n", filelist[n].f_name);
			}
		fclose(fp);
	}
	XtSetArg(arg[0], XtNset, &chk_flag);
	OlFlatGetValues(w_opt, 0, arg, 1);
	dev = curdev? DtamDevAttr(curdev, "cdevice"): rst_doc;
	sprintf(rst_buf, "%s -iBdlcv%c         -I %s", CpioCmd,
				(chk_flag? 'u': ' '), dev);
	if (rst_flag == SOME_SET)
		strcat(strcat(rst_buf, " -E "), pidindex);
	if (curdev) {
		FREE(dev);
		sprintf(rst_buf+strlen(rst_buf), " -M \"%s\n\" -G %s",
						EOM, "/dev/ptsdummyname");
	}
	strcat(rst_buf, " 2>&1");
	StartRstWatch(GetGizmoText(string_doingRst));
	XtAddTimeOut(wait? 2500: 100, (XtTimerCallbackProc)WatchCpio, NULL);
}

void	RdescCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	Widget		w_ud;
	OlFlatCallData	*olcd = (OlFlatCallData *) call_data;
	char		*desc, *name;
	register  int	n = olcd->item_index;

	FooterMsg(rbase, NULL);
	name = RstDevice[olcd->item_index].label;
	if (curdev)
		FREE(curdev);
	if (n == 0) {		/* restore from directory */
		desc = curalias = name;
		curdev = NULL;
	}
	else {
		curalias = DtamMapAlias(name);
		curdev = DtamGetDev(curalias,FIRST);
		desc = DtamDevDesc(curdev);
	}
	XtSetArg(arg[0], XtNuserData, &w_ud);
	XtGetValues(wid, arg, 1);

	XtSetArg(arg[0], XtNlabel, desc);
	XtSetValues(w_ud, arg, 1);

	XtSetArg(arg[0], XtNmappedWhenManaged, n==0);
	XtSetValues(XtParent(w_file), arg, 1);
	if (n == 0)
		OlSetInputFocus(w_file, RevertToNone, CurrentTime);
	RstSummary();
}


void	prevGroupCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
ListPtr	show_list;

	group_show--;
	show_num = group_size;

	show_list = filelist + group_show * group_size;

	XtSetArg(arg[0],XtNitems,show_list);
	XtSetArg(arg[1],XtNnumItems,show_num);
	XtSetArg(arg[2],XtNviewHeight, 7);
	XtSetArg(arg[3],XtNviewItemIndex, show_num - 1);
	XtSetArg(arg[4],XtNitemsTouched, TRUE);
	XtSetValues(w_list, arg, 5);

	/* buttons visible; next sensitive */
	if (group_show > 0)
		change_group_menu_items[0].sensitive = TRUE;
	else
		change_group_menu_items[0].sensitive = FALSE;
	change_group_menu_items[1].sensitive = TRUE;
	XtSetArg(arg[0], XtNitems, change_group_menu_items);
	XtSetArg(arg[1], XtNitemsTouched, TRUE);
	XtSetValues(w_group, arg, 2);
}


void	nextGroupCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
ListPtr	show_list;

	group_show++;
	if (group_show == group_max)
	{
		show_num = file_count % group_size;
		if (show_num == 0) show_num = group_size;
	}
	else
		show_num = group_size;

	show_list = filelist + group_show * group_size;

	XtSetArg(arg[0],XtNitems,show_list);
	XtSetArg(arg[1],XtNnumItems,show_num);
	XtSetArg(arg[2],XtNviewHeight, 7);
	XtSetArg(arg[3],XtNviewItemIndex, show_num - 1);
	XtSetArg(arg[4],XtNitemsTouched, TRUE);
	XtSetValues(w_list, arg, 5);

	/* buttons visible; next sensitive */
	if (group_show < group_max)
		change_group_menu_items[1].sensitive = TRUE;
	else
		change_group_menu_items[1].sensitive = FALSE;
	change_group_menu_items[0].sensitive = TRUE;
	XtSetArg(arg[0], XtNitems, change_group_menu_items);
	XtSetArg(arg[1], XtNitemsTouched, TRUE);
	XtSetValues(w_group, arg, 2);
}


static	void
SetRstDoc(Widget wid, XtPointer client_data, XtPointer call_data)
{
	struct	stat		fbuf;
	OlTextFieldVerify	*verify = (OlTextFieldVerify *)call_data;
	char			msgbuf[BUFSIZ];

	if (stat(verify->string,&fbuf) == 0) {
		sprintf(msgbuf, GetGizmoText(string_newFile), rst_doc);
		FooterMsg(rbase, msgbuf);
	}
	else {
		rst_doc = STRDUP(verify->string);
		RstSummary();
	}
}

CreateRestoreWindow(parent)
	Widget	parent;
{
static	ExclItem	RCheckBox[2];
	Widget		w_ctl, w_rdesc, w_cap, w_devmenu;
	char		*str, buf[64];

	if (note.shell)
		XtDestroyWidget(note.shell);
	NotePidFiles();
	SetLocaleTags();
/*
 *	create base window
 */
	rbase.icon_name = GetGizmoText(rbase.icon_name);
	w_ctl = CreateMediaWindow(parent, &rbase, NULL, 0);
/*
 *	create doc/device abbreviated button menu
 */
	RstDevice[0].label = doc_alias = GetGizmoText(label_doc);
	w_rdesc = DevMenu(RstDevice, 1, N_DEVS, w_ctl,
			GetGizmoText(label_rstFromCaption),
			(XtPointer)RdescCB, "removable=\"true", &w_devmenu);
	XtSetArg(arg[0], XtNlabel, DtamDevDesc(curdev));
	XtSetValues(w_rdesc, arg, 1);
/*
 *	controls specific to Restore
 */
	XtSetArg(arg[0], XtNlabel,		GGT(label_targetCaption));
	XtSetArg(arg[1], XtNposition,		OL_LEFT);
	XtSetArg(arg[2], XtNspace,		x3mm);
	XtSetArg(arg[3], XtNmappedWhenManaged,	FALSE);
	w_cap = XtCreateManagedWidget("caption",
			captionWidgetClass, w_ctl, arg, 4);

	XtSetArg(arg[0], XtNcharsVisible, 40);
	w_file = XtCreateManagedWidget("textfield",
			textFieldWidgetClass, w_cap, arg, 1);
	XtAddCallback(w_file, XtNverification, SetRstDoc, NULL);

	RCheckBox[0].setting = FALSE;
	RCheckBox[0].label = GetGizmoText(string_overwrite);

	XtSetArg(arg[0], XtNtraversalOn,	TRUE);
	XtSetArg(arg[1], XtNitemFields,		ExclFields);
	XtSetArg(arg[2], XtNnumItemFields,	NUM_ExclFields);
	XtSetArg(arg[3], XtNitems,		RCheckBox);
	XtSetArg(arg[4], XtNnumItems,		1);
	XtSetArg(arg[5], XtNbuttonType,		OL_CHECKBOX);
	w_opt = XtCreateManagedWidget("button",
			flatButtonsWidgetClass, w_ctl, arg, 6);
/*
 *	Restore window: scrolling list of files in "Select Files" mode
 */
	CreateFileList(rbase.scroller);
	
	XtSetArg(arg[0], XtNlabel,		GGT(label_groupCaption));
	XtSetArg(arg[1], XtNposition,		OL_LEFT);
	XtSetArg(arg[2], XtNspace,		x3mm);
	XtSetArg(arg[3], XtNmappedWhenManaged,	FALSE);
	w_groupcaption = XtCreateManagedWidget("groupcaption",
			captionWidgetClass, w_ctl, arg, 4);

	change_group_menu_items[0].label = GGT(label_prev_group);
	change_group_menu_items[1].label = GGT(label_next_group);

	XtSetArg(arg[0], XtNitems, 		change_group_menu_items);
	XtSetArg(arg[1], XtNnumItems, 		XtNumber(change_group_menu_items));
	XtSetArg(arg[2], XtNitemFields, 	flatMenuFields);
	XtSetArg(arg[3], XtNnumItemFields, 	XtNumber(flatMenuFields));
	XtSetArg(arg[4], XtNlayoutType, 	OL_FIXEDROWS);
	XtSetArg(arg[5], XtNexclusives, 	TRUE);
	XtSetArg(arg[6], XtNnoneSet, 		TRUE);
	XtSetArg(arg[7], XtNbuttonType, 	CMD);
	XtSetArg(arg[8], XtNmappedWhenManaged,	FALSE);
	w_group = XtCreateManagedWidget("groupbuttons",
			flatButtonsWidgetClass, w_groupcaption, arg, 9);
	
	RstSummary();
	CreateGizmo(rbase.shell, ModalGizmoClass, &rnote, NULL, 0);
	MapGizmo(BaseWindowGizmoClass, &rbase);
	group_size = INIT_GROUP_SIZE;
}

CreateFileList(parent)
	Widget	parent;
{
	XtSetArg(arg[0], XtNwidth,	32*x3mm);
	XtSetArg(arg[1], XtNheight,	12*y3mm);
	XtSetArg(arg[2], XtNminWidth,	1);
	XtSetArg(arg[3], XtNminHeight,	1);
	XtSetValues(parent, arg, 4);

	XtSetArg(arg[0], XtNmappedWhenManaged,	FALSE);
	XtSetArg(arg[1], XtNviewHeight,		7);
	XtSetArg(arg[2], XtNformat,		"%24s");
	XtSetArg(arg[3], XtNexclusives,		FALSE);
	XtSetArg(arg[4], XtNitemFields,		ListFields);
	XtSetArg(arg[5], XtNnumItemFields,	XtNumber(ListFields));
	XtSetArg(arg[6], XtNitems,		filelist);
	XtSetArg(arg[7], XtNnumItems,		file_count);
	XtSetArg(arg[8], XtNselectProc,		(XtCallbackProc)fileSelectCB);
	XtSetArg(arg[9], XtNunselectProc,	(XtCallbackProc)fileUnselectCB);

	w_list = XtCreateManagedWidget("fileList",
			flatListWidgetClass, parent, arg, 10);

	/* check for maximum number of items allowed in list */
	XtAddCallback(w_list,XtNitemsLimitExceeded,(XtCallbackProc)tooManyCB,NULL);

}

#define NO_ODD_LOT 100

void	tooManyCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{ /* too many items in list widget; check for (and set new) maximum */
OlFListItemsLimitExceededCD *tooMany = call_data;

	group_size = tooMany -> preferred;
	if (group_size > NO_ODD_LOT) 
		group_size -= group_size%NO_ODD_LOT;
	tooMany -> preferred = group_size;
	tooMany -> ok = TRUE;
}

void	fileSelectCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{ /* user selected a file; change count and display */

	char	msgbuf[128];

	select_count++;
	sprintf(msgbuf,GetGizmoText(string_selected),select_count,file_count);
	FooterMsg(rbase, msgbuf);
}

void	fileUnselectCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{ /* user deselected a file; change count and display */

	char	msgbuf[128];

	select_count--;
	sprintf(msgbuf,GetGizmoText(string_selected),select_count,file_count);
	FooterMsg(rbase, msgbuf);
}

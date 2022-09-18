/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:floppy/dos.c	1.30"
#endif

#include "media.h"
#include <Gizmo/InputGizmo.h>

extern	void	SetCaptionCB();

void	copyDOS();
void	deleteDOS();
void	createDOSdirPopup();
void	createDOSDirCB();
void	dhelpCB();
void	DescendDirCB();
void	selectAllCB();
void	DosErrorCB();
void	DosErrorNotice();
void	deleteConfirmCB();
void	cancelCB();
void	dismissErrorCB();

#define WIDTH	((Dimension)(42*x3mm))
#define	HEIGHT	((Dimension)(30*y3mm))

#define INIT_X  ((Dimension)16)
#define INIT_Y  ((Dimension)16)
#define INC_X   ((Dimension)80)
#define INC_Y   ((Dimension)64)
#define	MARGIN	((Dimension)32)

static MenuItems dfile_menu_item[] = {
	{ TRUE, label_copy,  mnemonic_copy, 0, copyDOS},
	{ TRUE, label_delete,mnemonic_delete, 0, deleteConfirmCB},
	{ TRUE, label_createDir, mnemonic_createDir, 0, createDOSdirPopup},
	{ TRUE, label_exit,  mnemonic_exit, 0, exitCB},
	{ NULL }
};

static MenuItems dedit_menu_item[] = {
        { TRUE, label_selectAll,   mnemonic_selectAll,   0, 
	      selectAllCB, (XtPointer)TRUE },
        { TRUE, label_unselectAll, mnemonic_unselectAll, 0,
	      selectAllCB, (XtPointer)FALSE },
        { NULL }
};

static HelpInfo dHelpDOS	= { 0, "", DHELP_PATH, help_dos_intro };
static HelpInfo dHelpTOC	= { 0, "", DHELP_PATH, NULL };
static HelpInfo dHelpDesk	= { 0, "", DHELP_PATH, "HelpDesk"  };
static HelpInfo dHelpCopy	= { 0, "", DHELP_PATH, help_dos_copy };

static MenuItems dhelp_menu_item[] = {  
	{ TRUE, label_dosHlp,mnemonic_dos,   0, dhelpCB, (char *)&dHelpDOS },
	{ TRUE, label_toc,   mnemonic_toc,   0, dhelpCB, (char *)&dHelpTOC },
	{ TRUE, label_hlpdsk,mnemonic_hlpdsk,0, dhelpCB, (char *)&dHelpDesk },
	{ NULL }
};

static MenuGizmo dfile_menu = {0, "file_menu", NULL, dfile_menu_item};
static MenuGizmo dedit_menu = {0, "edit_menu", NULL, dedit_menu_item};
static MenuGizmo dhelp_menu = {0, "help_menu", NULL, dhelp_menu_item};

static MenuItems dmain_menu_item[] = {
	{ TRUE, label_file, mnemonic_file, (Gizmo) &dfile_menu},
	{ TRUE, label_edit, mnemonic_edit, (Gizmo) &dedit_menu},
	{ TRUE, label_help, mnemonic_help, (Gizmo) &dhelp_menu},
	{ NULL }
};
static MenuGizmo dmenu_bar = { 0, "menu_bar", NULL, dmain_menu_item};

BaseWindowGizmo dbase = {0, "base", string_DOStitle, (Gizmo)&dmenu_bar,
	NULL, 0, "", "dos48.icon", " ", " ", 90 };

static Setting DosDirSetting =  { NULL, NULL, NULL, NULL };
static InputGizmo DosDirNameG = 
{
    &dHelpDOS,
    "DosDirname",
    label_name,
    "",
    &DosDirSetting,
    12,
    (void (*)())NULL,
};

static GizmoRec createDirGizmos[] =
{ {InputGizmoClass, &DosDirNameG } };

typedef enum _createDirIndex
{ cd_createOpen, cd_create, cd_cancel, cd_help} createDirIndex;

static MenuItems createDirItems[] =
{
   {TRUE, label_createOpen, mnemonic_createOpen},
   {TRUE, label_create,     mnemonic_create},
   {TRUE, label_cancel,     mnemonic_cancel},
   {TRUE, label_help3dot,   mnemonic_help},
   { NULL }
};

static MenuGizmo createDirMenu =
{ &dHelpDOS, "createDOSDir", "_X_", createDirItems,
      createDOSDirCB};

static PopupGizmo createDOSDirG =
{ &dHelpDOS, "createDOSDirPopup", title_createDir, &createDirMenu,
      createDirGizmos, XtNumber(createDirGizmos)}; 

static MenuItems confirm_item[] = {
   { TRUE, label_delete,   mnemonic_delete, 0, deleteDOS },
   { TRUE, label_cancel,   mnemonic_cancel, 0, cancelCB, (XtPointer)TRUE },
   { TRUE, label_help3dot, mnemonic_help,   0, dhelpCB,  (XtPointer)&dHelpDOS },
   { NULL }
};
static  MenuGizmo confirm_menu = {0, "note", "note", confirm_item };
static  ModalGizmo confirmG = {0, "warn", title_eraseConfirm,
				   (Gizmo)&confirm_menu,
				   string_eraseConfirm };
static MenuItems error_item[] = {
   { TRUE, label_continue, mnemonic_continue, 0, dismissErrorCB },
   { NULL }
};
static  MenuGizmo error_menu = {0, "error", "error", error_item };
static  ModalGizmo errorG = {0, "errorNotice", title_errorNotice,
				   (Gizmo)&error_menu, NULL };

Widget		w_dcap, w_dwin, w_dicon, w_cvt;
Dimension	idx = INIT_X, idy = INIT_Y;
FileList	dos_list = NULL;	
int		dos_count = 0;

DevItem		DOSDevice[N_DEVS];

char		DOSCP[]		= "LANG=C /usr/bin/doscp ";
char		DOSMKDIR[]	= "/usr/bin/dosmkdir ";
char		DOSRMDIR[]	= "/usr/bin/dosrmdir ";
char		DOSRM[]		= "/usr/bin/dosrm ";

typedef enum _opType 
{
    UtoMcopy, MtoUcopy, MixedErase, FileErase, DirErase, DirCreate
} opType;

char		curdosdir[PATH_MAX];
char		*parent_folder;

DmItemPtr	d_itp;
DmContainerRec	d_cntrec;
DmFclassRec	dosdoc_fcrec,
		dosdir_fcrec;

void		ResetDOSIconBox();
void		MtoUcopyCB();

MenuItems	DOScopyMenu[] = {
	{TRUE, label_selectF,mnemonic_selectF, NULL, NULL, NULL },
	{TRUE, label_cancel, mnemonic_cancel,  NULL, NULL, NULL },
	{TRUE, label_help,   mnemonic_help, NULL, dhelpCB, (char *)&dHelpCopy },
	{NULL}
};
MenuGizmo	DOScopy = {
			NULL, "copy_menu", "_X_", DOScopyMenu, MtoUcopyCB, NULL
};
FileGizmo	*copyprompt;
FileGizmo	MtoUPrompt = {
			NULL, "copy_to", NULL, &DOScopy, NULL, NULL, NULL,
			FOLDERS_ONLY, NULL
};

void
dhelpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	HelpInfo *help = (HelpInfo *) client_data;

	help->title = 
	help->app_title = DtamDevDesc(curdev);
	help->section = GetGizmoText(STRDUP(help->section));
	PostGizmoHelp(dbase.shell, help);
}

static	void
SetDevice(char *name)
{
	char	*ptr;

	if (strcmp(curalias,"diskette1") == 0)
		strcpy(curdosdir, "A:");
	else if (strcmp(curalias,"diskette2") == 0)
		strcpy(curdosdir, "B:");
	else {
		strcpy(curdosdir, ptr = DtamDevAttr(curdev, BDEVICE));
		strcat(curdosdir, ":");
		FREE(ptr);
	}
}

/*
 *	look for error messages, or confirmatory output
 *	(should avoid the current passing on of cryptic UNIX messages)
 *	opcode specifies which operation was invoked:
 *		UtoMcopy   => copy from UNIX to DOS
 *		MtoUcopy   => copy from DOS to UNIX
 *		MixedErase => delete DOS file(s) and directories
 *		FileErase  => delete DOS file(s) 
 *		DirErase   => delete DOS directories
 *		...
 */
void	CheckCmd(opType opcode, XtIntervalId id)
{
static	char	*msg = NULL, *msgp;
	char	buf[BUFSIZ], *str;
	int	n;

	n = read(CMDOUT, buf, BUFSIZ);
	switch (n) {
	case 0:	/*
		 *	end of file; command has finished
		 */
		_Dtam_p3close(cmdfp, 0);
		ResetDOSIconBox();
		if (msg) {		/* error; report it */
		        msgp = msg;
			msg = NULL;     /* DosErrorNotice will do FREE() */
		        DosErrorNotice(opcode, msgp);
		}
		else switch(opcode) {
		case UtoMcopy:	FooterMsg(dbase, GetGizmoText(string_UtoMcopy));
			break;
		case MtoUcopy:	FooterMsg(dbase, GetGizmoText(string_MtoUcopy));
			break;
		case MixedErase: /* FALL THRU */
		case FileErase:  /* FALL THRU */
		case DirErase:
		        FooterMsg(dbase, GetGizmoText(string_DOSdelete));
			break;
		}
		break;
	default:/*
		 *	save message for later examination
		 */
		if (msg == NULL) {
			buf[n] = '\0';
			msg = STRDUP(buf);
		}
		else {
			msg = (char *)REALLOC(msg, strlen(msg)+n+1);
			buf[n] = '\0';
			strcat(msg, buf);
		}
		/* fall through *//* to wait for more input */
	case -1:
		XtAddTimeOut(500,(XtTimerCallbackProc)CheckCmd,
			     (XtPointer)opcode);
		break;
	}
}

static	void
deleteDOS(Widget wid, XtPointer client_data, XtPointer call_data)
{
    int	      selectCount, fileCount = 0, dirCount = 0;
    opType    opcode;
    DmItemPtr itp,   item;
    FileList  dosfile;
    char     *cmd, *cmd1, *cmd2;

    cancelCB(wid, (XtPointer)FALSE, call_data);  /* to popdown modal */
    
    FooterMsg(dbase, NULL);

    XtSetArg(arg[0], XtNselectCount, &selectCount);
    XtSetArg(arg[1], XtNitems,       &itp);
    XtGetValues(w_dicon, arg, 2);

    if (selectCount == 0)
    {
	FooterMsg(dbase, GetGizmoText(string_noneset));
	return;
    }
    cmd1 = STRDUP(DOSRM);
    cmd2 = STRDUP(DOSRMDIR);
    for (item = itp; selectCount; item++)
    {
        if ( ITEM_MANAGED(item) )
        {
	    if (ITEM_SELECT(item))
	    {
		selectCount--;
		dosfile = (FileList)(ITEM_OBJ(item)->objectdata); 

		if (dosfile->bk_type == 'f')
		{
		    fileCount++;
		    if ((cmd1 = (char *)REALLOC(cmd1, strlen(cmd1) +
						strlen(dosfile->bk_path) +
						strlen(curdosdir)  +
						8)) == NULL)
			goto cleanup;
		    sprintf(cmd1+strlen(cmd1), " %s%s", curdosdir,
			    dosfile->bk_path); 
		}
		else
		{
		    dirCount++;
		    if ((cmd2 = (char *)REALLOC(cmd2, strlen(cmd2) +
						strlen(dosfile->bk_path) + 
						strlen(curdosdir)  +
						8)) == NULL)
			goto cleanup;
		    sprintf(cmd2+strlen(cmd2), " %s%s", curdosdir,
			    dosfile->bk_path); 
		}
	    }
	}
    }
    if (fileCount == 0)
    {
	opcode = DirErase;
	cmd = cmd2;
    }
    else
    {
	opcode = FileErase;
	cmd = cmd1;
	if (dirCount > 0)
	{
	    opcode = MixedErase;
	    if ((cmd = (char *)REALLOC(cmd,
					strlen(cmd)+strlen(cmd2)+4)) == NULL)
		goto cleanup;
	    strcat(strcat(cmd, " ; "), cmd2);
	}
    }
    if (_Dtam_p3open(cmd, cmdfp, TRUE) == 0)
	XtAddTimeOut(1500,(XtTimerCallbackProc)CheckCmd,(XtPointer)opcode);
    else
	FooterMsg(dbase, GetGizmoText(string_cantDelete));
 cleanup:
    if (cmd1 == NULL || cmd2 == NULL)
	FooterMsg(dbase, GetGizmoText(string_badMalloc));
    if (cmd1) FREE(cmd1);
    if (cmd2) FREE(cmd2);
    return;
}

static	void
copyDOS(Widget wid, XtPointer client_data, XtPointer call_data)
{
	int	count;

	FooterMsg(dbase, NULL);
	XtSetArg(arg[0], XtNselectCount, &count);
	XtGetValues(w_dicon,  arg, 1);
	if (count == 0)
		FooterMsg(dbase, GetGizmoText(string_noneset));
	else {
		SetFileCriteria(copyprompt, NULL, "");
		MapGizmo(FileGizmoClass, copyprompt);
	}
}

static	void
SetupCopyCmd(char *cmdbuf)
{
	Boolean	setting;

	strcpy(cmdbuf, DOSCP);
	XtSetArg(arg[0], XtNset, &setting);
	OlFlatGetValues(w_cvt, 0, arg, 1);
	if (setting)
		strcat(cmdbuf, "-m ");
	else {
		OlFlatGetValues(w_cvt, 1, arg, 1);
		if (setting)
			strcat(cmdbuf, "-r ");
	}
}

static	void
MtoUcopyCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData	*olcd = (OlFlatCallData *)call_data;
    int			 n = olcd->item_index;
    Boolean		 setting;
    char		*name, *cmd = (char *)MALLOC(BUFSIZ);
    int			 dirCopyAttempted = 0, numberOfFilesToCopy = 0;
    int			 nitems;
    DmItemPtr	         itp, item;
    FileList 		 dosfile;
    int			 selectCount;
    Widget		 shell = GetFileGizmoShell(copyprompt);
    
    FooterMsg(dbase, NULL);
    if (n == 1)			/* cancel */
	SetWMPushpinState(XtDisplay(shell), XtWindow(shell), WMPushpinIsOut); 
    BringDownPopup(shell);
    if (n != 0)
    {
	FREE(cmd);
	return;
    }
    SetupCopyCmd(cmd);

    XtSetArg(arg[0], XtNselectCount, &selectCount);
    XtSetArg(arg[1], XtNitems,       &itp);
    XtGetValues(w_dicon, arg, 2);

    if (selectCount == 0)
    {
	FooterMsg(dbase, GetGizmoText(string_noneset));
	FREE(cmd);
	return;
    }

    for (item = itp; selectCount; item++)
    {
        if ( ITEM_MANAGED(item) )
        {
	    if (ITEM_SELECT(item))
	    {
		selectCount --;
		dosfile = (FileList)(ITEM_OBJ(item)->objectdata); 

		if (dosfile->bk_type == 'd')
		    dirCopyAttempted++;
		else
		{
		    int m = strlen(cmd) + strlen(dosfile->bk_path) +
			    strlen(curdosdir);

		    numberOfFilesToCopy++;
		    if (++m > BUFSIZ)
			cmd = (char *)REALLOC(cmd, m+1);
		    sprintf(cmd+strlen(cmd), " %s%s", curdosdir,
			    dosfile->bk_path);
		}
	    }
	}
    }
    if (dirCopyAttempted)
	FooterMsg(dbase, GetGizmoText(string_cantCopyDirs));
    if (numberOfFilesToCopy == 0 && dirCopyAttempted == 0)
	FooterMsg(dbase, GetGizmoText(string_noneset));
    if (numberOfFilesToCopy != 0)
    {
	name = GetFilePath(copyprompt);
	cmd = (char *)REALLOC(cmd, strlen(cmd)+strlen(name)+8);
	sprintf(cmd+strlen(cmd), " %s 2>&1", name);
	if (_Dtam_p3open(cmd, cmdfp, TRUE) == 0)
	    XtAddTimeOut(1500, (XtTimerCallbackProc)CheckCmd,
			 (XtPointer)MtoUcopy); 
	else
	    FooterMsg(dbase, GetGizmoText(string_cantCopy));
	FREE(name);
    }
    FREE(cmd);
}

static	void
SetDevCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	FooterMsg(dbase, NULL);
	SetCaptionCB(wid, client_data, call_data);
	SetDevice(curalias);
}

static	void
MakeDOSList(void)
{
register int	n;
	char	*ptr, buf[80], name[13];
	FILE	*pfp;

	dos_count = 0;
	if (dos_list)
		FREE((char *)dos_list);
	dos_list = NULL;
	sprintf(buf,"/usr/bin/dosdir %s", curdosdir);
	if ((pfp = popen(buf,"r")) == NULL) {
		FooterMsg(dbase, string_cantList);
		return;
	}
	if (curdosdir[strlen(curdosdir)-1] == '/') {
		dos_list = (FileList)MALLOC(sizeof(FileObj));
		dos_list[0].bk_path = STRDUP(parent_folder);
		dos_list[0].bk_type = 'd';
		dos_count = 1;
	}
	while (fgets(buf, 80, pfp)) {
		if (isspace(*buf)) 
			continue;
		buf[12] = '\0';
		for (ptr = name, n = 0; n < 12; n++) {
			if (buf[n] != ' ')
				*ptr++ = buf[n];
			if (n == 8)
				*ptr++ = '.';
		}
		if (ptr[-1] == '.')
			ptr[-1] = '\0';
		else
			*ptr = '\0';
		ptr = strstr(buf+13,"<DIR>");
		if (ptr && (strcmp(name,".")==0 || strcmp(name,"..")==0))
			continue;
		dos_list = (FileList)REALLOC((char *)dos_list,
						(dos_count+1)*sizeof(FileObj));
		dos_list[dos_count].bk_path = STRDUP(name);
		dos_list[dos_count].bk_type = ptr? 'd': 'f';
		dos_count++;
	}
	pclose(pfp);
}

static	void
UNIXtoDOS(char *source)
{
    static char  pat[] = " %s %s 2>&1";
    char        *cmd, *files, *s1, *path, *delimiter = " ";
    int		 len,  status,         numberOfFilesToCopy  = 0;
    int		 dirCopyAttempted = 0, specialCopyAttempted = 0;
    int		 statError = 0;
    struct stat  statbuf;

    FooterMsg(dbase, NULL);
    if ((cmd = (char *)MALLOC((len = strlen(source)) + strlen(pat)   +
			      strlen(curdosdir)      + strlen(DOSCP) +
			      4)) == NULL  ||
	(files = (char *)MALLOC(len)) == NULL)
    {
	if (cmd)   FREE(cmd);
	FooterMsg(dbase, GetGizmoText(string_badMalloc));
	return;
    }
    SetupCopyCmd(cmd);

    *files = '\0';
    s1 = source;
    while (path = strtok(s1, delimiter))
    {
	s1 = NULL;
	status = stat(path, &statbuf);
	if (status != 0)
	{
	    statError++;
	    FooterMsg(dbase, GetGizmoText(string_UtoMerror));
	    continue;
	}
	if ((statbuf.st_mode & S_IFMT) == S_IFDIR) /* it's a folder */
	    dirCopyAttempted++;
	else if ((statbuf.st_mode & S_IFMT) == S_IFREG) /* it's a file   */
	{
	    numberOfFilesToCopy++;
	    strcat(strcat(files, path), " ");
	}
	else		   /* attempt to copy special file (e.g. fifo) */
	    specialCopyAttempted++;
    }

    /* FIX: add messages for special files */
    if (dirCopyAttempted || specialCopyAttempted)
	FooterMsg(dbase, GetGizmoText(string_copyFilesOnly));
    if (numberOfFilesToCopy == 0 && !dirCopyAttempted &&
	!specialCopyAttempted && !statError)
	FooterMsg(dbase, GetGizmoText(string_noneset));
    if (numberOfFilesToCopy != 0)
    {
	sprintf(cmd+strlen(cmd), pat, files, curdosdir);
	if (_Dtam_p3open(cmd, cmdfp, TRUE) == 0)
	    XtAddTimeOut(2500, (XtTimerCallbackProc)CheckCmd,
			 (XtPointer)UtoMcopy);
	else
	    FooterMsg(dbase, GetGizmoText(string_cantCopy));
    }
    FREE(cmd);
    FREE(files);
}

int	ditem_count = 0;

static	DmObjectPtr
AddFile(FileList dosfile)
{
	DmObjectPtr	optr;

	optr = (DmObjectPtr)CALLOC(1, sizeof(DmObjectRec));
	optr->container = &d_cntrec;
	if (dosfile->bk_type == 'd')
		optr->fcp = &dosdir_fcrec;
	else
		optr->fcp = &dosdoc_fcrec;
	optr->name = STRDUP(dosfile->bk_path);
	optr->x = idx;
	optr->y = idy;
	optr->objectdata = (XtPointer)dosfile;
	if ((Dimension)(idx += INC_X) > (Dimension)(WIDTH - MARGIN)) {
		idx = INIT_X;
		idy += INC_Y;
	}
	if (ditem_count++ == 0) {
		d_cntrec.op = optr;
	}
	else {
		DmObjectPtr endp = d_cntrec.op;
		while (endp->next)
			endp = endp->next;
		endp->next = optr;
	}
	d_cntrec.num_objs = ditem_count;
	return optr;
}

void
DosTakeDrop(Widget wid, XtPointer client_data, XtPointer call_data)
{
static	char		*dos_copy = NULL;
	char		**p, *name, *ptr;
	DtDnDInfoPtr	dip = (DtDnDInfoPtr)call_data;
	char		delimitedName[PATH_MAX + 3] = " ";
	char		*dnp = delimitedName, *dnp1 = &delimitedName[1];

	if (dos_copy) {
		FREE(dos_copy);
	}
        dos_copy = STRDUP(" ");
	if (dip->files && *dip->files && **dip->files) {
		for (p=dip->files; *p; p++) {
			name = *p;
			if (name[0] == '/' && name[1] == '/')
				name++;
			strcat(strcpy(dnp1, name), " ");  /* " name " */
			if (ptr=strstr(dos_copy, dnp))
				continue;
			else {
				dos_copy = (char *)REALLOC(dos_copy,
					strlen(dos_copy)+strlen(name)+2);
				strcat(strcat(dos_copy, name), " ");
			}
		}
		UNIXtoDOS(dos_copy);
	}
	else
		XRaiseWindow (theDisplay, XtWindow (dbase.shell));
}

static Boolean
DOSTriggerNtfy(	Widget			w,
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
	DtGetFileNames(w, selection, timestamp, send_done, DosTakeDrop,
		       closure);
}

static	Widget
DOSIconBox(Widget parent)
{
	int	n = 0;
	Widget	w_box;

	idx = INIT_X; idy = INIT_Y;

	/* force previous directory folder onto its own row */
	if (dos_count && strcmp(dos_list[0].bk_path, parent_folder) == 0)
	{
	    ditem_count = 0;
	    AddFile(dos_list);
	    n++;
	    idx = INIT_X;
	    idy = INIT_Y + INC_Y;
	}
	
	for (ditem_count = n; n < dos_count; n++)
		AddFile(dos_list+n);

	XtSetArg(arg[0], XtNmovableIcons,	(XtArgVal)FALSE);
	XtSetArg(arg[1], XtNminWidth,		(XtArgVal)1);
	XtSetArg(arg[2], XtNminHeight,		(XtArgVal)1);
	XtSetArg(arg[3], XtNdrawProc,		(XtArgVal)DmDrawIcon);
	XtSetArg(arg[4], XtNtriggerMsgProc,	(XtArgVal)DOSTriggerNtfy);
	XtSetArg(arg[5], XtNdblSelectProc,	(XtArgVal)DescendDirCB);

	w_box = DmCreateIconContainer(parent, DM_B_CALC_SIZE, arg, 6,
		d_cntrec.op,dos_count,&d_itp,dos_count,NULL,NULL,def_font,1);
	return w_box;
}

static	void
ResetDOSIconBox()
{
	XtPointer	items;

	XtUnmanageChild(w_dicon);
	XtSetArg(arg[0], XtNitems, (XtArgVal) &items);
	XtGetValues(w_dicon, arg, 1);
	XtDestroyWidget(w_dicon);
	MakeDOSList();
	w_dicon = DOSIconBox(dbase.scroller);
}

static	void
DescendDirCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	DmObjectPtr	op;
	FileList	fp;
	OlFlatCallData	*d = (OlFlatCallData *)call_data;
	char		*ptr, *ptr2;
	char		title[BUFSIZ];
	
	FooterMsg(dbase, NULL);
	XtSetArg(arg[0], XtNobjectData, &op);
	OlFlatGetValues(wid, d->item_index, arg, 1);
	fp = (FileList)op->objectdata;
	if (fp->bk_type != 'd')
		return;
	if (strcmp(op->name,parent_folder) != 0)
		strcat(strcat(curdosdir,op->name),"/");
	else {
		/*
		 *	go back up to parent directory
		 */
		curdosdir[strlen(curdosdir)-1] = '\0';
		ptr = strrchr(curdosdir,'/')+1;
		ptr2 = strchr(curdosdir,':');
		if (ptr > ptr2)
			*ptr = '\0';
		else
			ptr2[1] = '\0';
	}
	sprintf(title, GetGizmoText(string_DOStitle), curdosdir);
	XtSetArg(arg[0], XtNtitle, title);
	XtSetValues(dbase.shell, arg, 1);
	ResetDOSIconBox();
}

static	void
CreateCopyPrompt()
{
	copyprompt = CopyGizmo(FileGizmoClass, &MtoUPrompt);
	copyprompt->title = GetGizmoText(string_cpyTitle);
/*
 *	the following causes a segmentation violation:
 *
 *	copyprompt->directory = homedesk;
 */
	CreateGizmo(dbase.shell, FileGizmoClass, copyprompt, NULL, 0);
	SetFileGizmoMessage(copyprompt, GetGizmoText(string_cpyPrompt));
	XtUnmanageChild(XtParent(copyprompt->textFieldWidget));
}

CreateDOSWindow(Widget parent, char *source)
{
static	ExclItem	CvtSwitch[2];
	Widget		w_ctl, w_ref, w_desc, w_cap;
	char		*str;
        char		title[BUFSIZ];

	if (note.shell)
		XtDestroyWidget(note.shell);
	SetDevice(curalias);
	dbase.icon_name = DtamDevAlias(curdev);

	dosdir_fcrec.glyph = DmGetPixmap(theScreen, "dir.icon");
	dosdoc_fcrec.glyph = DmGetPixmap(theScreen, "datafile.icon");
	parent_folder = GetGizmoText(label_parentdir);

	w_ctl = CreateMediaWindow(parent, &dbase, NULL, 0);
/*
 *	create device abbreviated button menu
 */
/*
 *	DOS specific controls (new-line conversion setting)
 */
	XtSetArg(arg[0], XtNlabel,	GGT(label_convertCaption));
	XtSetArg(arg[1], XtNposition,	OL_LEFT);
	XtSetArg(arg[2], XtNspace,	x3mm);
	XtSetArg(arg[3], XtNweight,	0);
	XtSetArg(arg[4], XtNyRefWidget,	w_desc);
	XtSetArg(arg[5], XtNyAddHeight,	TRUE);

	w_cap = XtCreateManagedWidget("convertcaption",
			captionWidgetClass, w_ctl, arg, 6);

	SET_EXCL(CvtSwitch, 0, convert,   FALSE);
	SET_EXCL(CvtSwitch, 1, noconvert, FALSE);

	XtSetArg(arg[0], XtNtraversalOn,	TRUE);
	XtSetArg(arg[1], XtNbuttonType,		OL_RECT_BTN);
	XtSetArg(arg[2], XtNexclusives,		TRUE);
	XtSetArg(arg[3], XtNnoneSet,		TRUE);
	XtSetArg(arg[4], XtNitemFields,		ExclFields);
	XtSetArg(arg[5], XtNnumItemFields,	NUM_ExclFields);
	XtSetArg(arg[6], XtNitems,		CvtSwitch);
	XtSetArg(arg[7], XtNnumItems,		2);

	w_cvt = XtCreateManagedWidget("cvtexcl",
			flatButtonsWidgetClass, w_cap, arg, 8);

/*
 *	DOS window: icon box for files/directories on diskette
 */
	XtSetArg(arg[0], XtNwidth,	WIDTH);
	XtSetArg(arg[1], XtNheight,	(Dimension)(2*HEIGHT)/(Dimension)3);
	XtSetArg(arg[2], XtNyRefWidget,	w_cap);

	XtSetValues(dbase.scroller, arg, 3);

	if (source)
		UNIXtoDOS(source);
	MakeDOSList();
	w_dicon = DOSIconBox(dbase.scroller);

	CreateCopyPrompt();

	sprintf(title, GetGizmoText(string_DOStitle), curdosdir);
	XtSetArg(arg[0], XtNtitle, title);
	XtSetValues(dbase.shell, arg, 1);

	MapGizmo(BaseWindowGizmoClass, &dbase);
}

/*
 *	bastardized CreateBaseGizmo; replace with a "correct" initialization
 *	of the base window by passing in a gizmo array (if I can figure it out)
 */

Widget 
CreateMediaWindow(Widget parent, BaseWindowGizmo *gizmo, Arg *args, int num)
{
	Pixmap		PixmapOfFile();
	MenuGizmo	*menu	= (MenuGizmo *)gizmo->menu;
	Widget		menuparent;
	Widget		w_ctl;
	Dimension	footerHeight;
	Widget		form;
	int		i;
	Widget		newWidget;
	int		error_percent;
	Pixmap		icon_pixmap;
	int		width;
	int		height;
	char		*str;

	XtSetArg(arg[0], XtNborderWidth,       0);
	XtSetArg(arg[1], XtNtranslations,      XtParseTranslationTable(""));
	XtSetArg(arg[2], XtNgeometry,          "32x32");
	XtSetArg(arg[3], XtNmappedWhenManaged, False);
	if (gizmo->icon_pixmap && *(gizmo->icon_pixmap)) {
		gizmo->icon_shell = 
		XtCreateApplicationShell("_X_", vendorShellWidgetClass, arg, 4);

		XtRealizeWidget(gizmo->icon_shell);

		icon_pixmap = PixmapOfFile(gizmo->icon_shell,gizmo->icon_pixmap,
					gizmo->icon_name,&width,&height);

		XtSetArg(arg[0], XtNbackgroundPixmap,  icon_pixmap);
		XtSetArg(arg[1], XtNwidth,  width);
		XtSetArg(arg[2], XtNheight, height);
		XtSetValues(gizmo->icon_shell, arg, 3);
	}
	else
		gizmo->icon_shell = NULL;

	if (gizmo->shell == NULL)
		gizmo->shell = XtCreateApplicationShell(gizmo->name,
					topLevelShellWidgetClass, args, num);

	str = gizmo->title? GetGizmoText(gizmo->title): " ";
	XtSetArg(arg[0], XtNiconName, gizmo->icon_name? gizmo->icon_name: str);
	XtSetArg(arg[1], XtNtitle,    str);
	i = 2;
	if (gizmo->icon_shell) {
		XtSetArg(arg[i], XtNiconWindow, XtWindow(gizmo->icon_shell));
		i++;
	}
	XtSetValues(gizmo->shell, arg, i);

	XtSetArg(arg[0], XtNorientation,   OL_VERTICAL);
	gizmo->form = XtCreateManagedWidget("_X_", rubberTileWidgetClass,
			gizmo->shell, arg, 1);
	/*
	* adjust the gravity and padding
	* note: the padding should be specified in points
	*
	*/
	XtSetArg(arg[0], XtNgravity, NorthWestGravity);
	XtSetArg(arg[1], XtNvPad,  6);
	XtSetArg(arg[2], XtNhPad,  6);
	XtSetArg(arg[3], XtNmenubarBehavior,	TRUE);
	menuparent = CreateGizmo(gizmo->form, MenuBarGizmoClass, (Gizmo)menu,
								arg, 4);
	XtSetArg(arg[0], XtNweight, 		0);
	XtSetArg(arg[1], XtNmenubarBehavior,	TRUE);
	XtSetValues(menuparent, arg, 2);

/*	here's where I shove in my stuff (though evidently it "should" be
 *	done as an array of gizmos through the arguments to CreateGizmo)
 */

	XtSetArg(arg[0], XtNlayoutType,		OL_FIXEDCOLS);
	XtSetArg(arg[1], XtNalignCaptions,	TRUE);
	XtSetArg(arg[2], XtNvSpace,		2*y3mm);
	XtSetArg(arg[3], XtNvPad,		2*y3mm);
	XtSetArg(arg[4], XtNyRefWidget,		menu->child);
	XtSetArg(arg[5], XtNyAddHeight,		TRUE);
	XtSetArg(arg[6], XtNshadowThickness,	0);
	w_ctl = XtCreateManagedWidget("_X_", controlAreaWidgetClass,
						gizmo->form, arg, 7);
/*
 *	this widget will be returned, instead of the shell
 */
	if (gizmo->num_gizmos == 0) {
		XtSetArg(arg[0], XtNxResizable,    True);
		XtSetArg(arg[1], XtNyResizable,    True);
		XtSetArg(arg[2], XtNxAttachRight,  True);
		XtSetArg(arg[3], XtNyAttachBottom, True);
		XtSetArg(arg[4], XtNyAddHeight,    True);
		XtSetArg(arg[5], XtNyRefWidget,    w_ctl);
		XtSetArg(arg[6], XtNyAttachOffset, footerHeight);
		XtSetArg(arg[7], XtNweight, 1);
		gizmo->scroller =
			XtCreateManagedWidget("_X_", scrolledWindowWidgetClass,
							gizmo->form, arg, 8);
	}
	else {
		CreateGizmoArray(gizmo->form, gizmo->gizmos, gizmo->num_gizmos);
	}

	error_percent = gizmo->error_percent == 0 ? 75 : gizmo->error_percent;

	XtSetArg(arg[0], XtNweight,      0);
	XtSetArg(arg[1], XtNleftFoot,    gizmo->error?
						GetGizmoText(gizmo->error):
						" ");
	XtSetArg(arg[2], XtNrightFoot,   gizmo->status?
						GetGizmoText(gizmo->status):
						" ");
	XtSetArg(arg[3], XtNleftWeight,  error_percent);
	XtSetArg(arg[4], XtNrightWeight, 100-error_percent);
	gizmo->message = XtCreateManagedWidget ("footer", footerWidgetClass,
							gizmo->form, arg, 5);

	return (w_ctl);

}

static void
selectAllCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    DmItemPtr	item;
    FileList	dosfile;
    Cardinal	nitems;
    Boolean	select = (Boolean)client_data;
    int		count  = 0;

    XtSetArg(arg[0], XtNnumItems, &nitems);
    XtGetValues(w_dicon, arg, 1);

    for (item = d_itp; item < d_itp + nitems; item++)
    {
	if ( ITEM_MANAGED(item) )
	{
	    dosfile = (FileList)(ITEM_OBJ(item)->objectdata);
	    if ((item->select = (select == TRUE &&
		                 dosfile->bk_type == 'f')))
		count++;
	}
    }
    XtSetArg(arg[0], XtNselectCount,  count);
    XtSetArg(arg[1], XtNitemsTouched, TRUE);
    XtSetValues(w_dicon, arg, 2);
}

static void
createDOSdirPopup(Widget wid, XtPointer client_data, XtPointer call_data)
{
    static Widget w_createDir = NULL;
    
    if (w_createDir == NULL)
    {
	w_createDir = CreateGizmo(wid, PopupGizmoClass, &createDOSDirG,
				  NULL, 0);
	XtSetArg(arg[0], XtNmaximumSize, (XtArgVal)12); 
	XtSetArg(arg[1], XtNcanScroll,   (XtArgVal)FALSE); 
	XtSetValues(DosDirNameG.textFieldWidget, arg, 2);
    }

    XtSetArg(arg[0], XtNfocusWidget, (XtArgVal)(DosDirNameG.textFieldWidget));
    XtSetValues(w_createDir, arg, 1);
    SetPopupMessage(&createDOSDirG, "");
    FooterMsg(dbase, NULL);
    MapGizmo(PopupGizmoClass, &createDOSDirG);
}


/*
 *	upshift(string)  --  convert string to uppercase completely.
 */

static void
upshift(char * string)
{
    if (string == NULL)
	return;

    while (*string)
	*(string++) = toupper(*string);

    return;
}

static void
createDOSDirCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    Widget    shell = GetPopupGizmoShell(&createDOSDirG);
    int       status;
    char     *cmd, *name;
    Cardinal  item_index =
	((OlFlatDropCallData*)call_data)->item_data.item_index;

    switch ((createDirIndex)item_index)
    {
    case cd_createOpen:
	/* FALL THRU */
    case cd_create:
	SetPopupMessage(&createDOSDirG, "");
	ManipulateGizmo(InputGizmoClass, &DosDirNameG, GetGizmoValue);
	name = (char *)DosDirSetting.current_value ;
	if (name == NULL || *name == '\0')
	{
	    SetPopupMessage(&createDOSDirG, GetGizmoText(string_fillInName));
	    break;
	}
	else
	    SetPopupMessage(&createDOSDirG, "");
	upshift(name);
	if ((cmd = MALLOC(4 + strlen(DOSMKDIR) + strlen(curdosdir) +
			  strlen(name))) == NULL)
	{
	    SetPopupMessage(&createDOSDirG,
			    GetGizmoText(string_badMalloc));
	    break;
	}
	sprintf(cmd, "%s %s%s/", DOSMKDIR, curdosdir, name);
	status = system(cmd);
	FREE(cmd);
	if(status == 0)
	    BringDownPopup(shell);
	else
	{
	    DosErrorNotice(DirCreate, NULL);
	    return;
	}
	if ((createDirIndex)item_index == cd_createOpen)
	{
	    char title[BUFSIZ];
	
	    strcat(strcat(curdosdir, name),"/");
	    sprintf(title, GetGizmoText(string_DOStitle), curdosdir);
	    XtSetArg(arg[0], XtNtitle, title);
	    XtSetValues(dbase.shell, arg, 1);
	}
	ResetDOSIconBox();
	break;
    case cd_cancel:
	SetPopupMessage(&createDOSDirG, "");
	SetWMPushpinState(XtDisplay(shell), XtWindow(shell), WMPushpinIsOut); 
	BringDownPopup(shell);
	break;
    case cd_help:
	dhelpCB(wid, (XtPointer)&dHelpDOS, call_data);
	break;
    }
    return;
}

static void
DosErrorNotice(opType opcode, char *raw_stderr)
{
    static Widget w_dosError = NULL;
    char  *file, *dir,  *msg = NULL;

    if (w_dosError == NULL)
	w_dosError = CreateGizmo(dbase.shell, ModalGizmoClass, &errorG,
				 NULL, 0);
    switch (opcode)
    {
    case UtoMcopy:
	if (strstr(raw_stderr, "Warning: renaming"))
	    FooterMsg(dbase, GetGizmoText(string_filesRenamed));
	if (strstr(raw_stderr, "doscp:"))
	    msg = GetGizmoText(string_UtoMcopyFailed);
	break;
    case MtoUcopy:
	msg = GetGizmoText(string_MtoUcopyFailed);
	break;
    case MixedErase:
	file = strstr(raw_stderr, "dosrm:");
	dir  = strstr(raw_stderr, "dosrmdir:");
	if (file && dir)
	    msg = GetGizmoText(string_mixedEraseFailed);
	else if (file)
	{
	    DosErrorNotice(FileErase, raw_stderr);
	    return;
	}
	else
	{
	    DosErrorNotice(DirErase, raw_stderr);
	    return;
	}
	break;
    case FileErase:
	msg = GetGizmoText(string_fileEraseFailed);
	break;
    case DirErase:
	msg = GetGizmoText(string_rmdirFailed);
	break;
    case DirCreate:
	msg = GetGizmoText(string_dirCreateFailed);
	break;
    default:
	break;
    }
    if (raw_stderr)
    {
	FREE(raw_stderr);
    }
    if (msg != NULL)
    {
	SetModalGizmoMessage(&errorG, msg);
	MapGizmo(ModalGizmoClass, &errorG);
    }
    return;
}

static void 
dismissErrorCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    XtPopdown(errorG.shell);
    return;
}

static void 
cancelCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    XtPopdown(confirmG.shell);
    if ((Boolean)client_data == TRUE)
	FooterMsg(dbase, GetGizmoText(string_nothingErased));
    return;
}

static void 
deleteConfirmCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    static Widget w_confirm = NULL;

    if (w_confirm == NULL)
	w_confirm = CreateGizmo(wid, ModalGizmoClass, &confirmG, NULL, 0);
    FooterMsg(dbase, NULL);
    MapGizmo(ModalGizmoClass, &confirmG);

    return;
}

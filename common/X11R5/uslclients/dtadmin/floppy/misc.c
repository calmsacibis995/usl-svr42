/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:floppy/misc.c	1.24"
#endif

#include "media.h"

#include <archives.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define	CORRUPT		0x2900
#define	BUSY_MNT	0xf00

extern void	exitCB (Widget widget, XtPointer client_data,
		     XtPointer call_data);
static void	ReopenCB (Widget widget, XtPointer client_data,
		     XtPointer call_data);
static void	RecloseCB (Widget widget, XtPointer client_data,
		     XtPointer call_data);
static void	attempt_umount (void);

static MenuItems Insert_menu_item[] = {  
	{ TRUE, label_continue, mnemonic_continue, 0, RecloseCB, 0 },
	{ NULL }
};
static MenuGizmo Insert_menu = {0, "Insert_menu", NULL, Insert_menu_item};
static ModalGizmo Insert = {0, "", title_confirmIns, (Gizmo)&Insert_menu};

static MenuItems Busy_menu_item[] = {  
	{ TRUE, label_exit, mnemonic_exit, 0, exitCB, (XtPointer) 1 },
	{ TRUE, label_cancel, mnemonic_cancel, 0, ReopenCB, NULL },
	{ NULL }
};
static MenuGizmo Busy_menu = {0, "Busy_menu", NULL, Busy_menu_item};
static ModalGizmo Busy = {0, "", title_confirmUmt, (Gizmo)&Busy_menu};

extern	char	*_dtam_mntpt;
extern	long	_dtam_flags;

XtIntervalId	disk_tid = NULL;

void	MonitorMount(XtPointer closure, XtIntervalId id)
{
	sync();
	/*
	 *	should probably do a CheckMedia() and issue a popup warning
	 *	in case the disk has been removed, doing the sync only if 
	 *	the disk remains in the drive.
	 */
	disk_tid = XtAddTimeOut(3000,(XtTimerCallbackProc)MonitorMount, NULL);
}

int	attempt_mount(diagn, device)
	int	diagn;
	char	*device;
{
extern	char	*_dtam_fstyp;
	char	*ptr, cmdbuf[BUFSIZ];
	char	*fstype;
	char	*label;
	char	*rdonly;
	FILE	*cmdfp;
	int	result;
	Boolean	retry = TRUE;

	if (_dtam_flags & DTAM_MOUNTED)
		goto success;
	switch(diagn&DTAM_FS_TYPE) {
		case DTAM_S5_FILES:	fstype = "s5";	break;
		case DTAM_UFS_FILES: 	fstype = "ufs";	break;
		default:		fstype = _dtam_fstyp;
	}
	label = DtamDevAlias(device);
	if (!_dtam_mntpt)
		if ((_dtam_mntpt = (char *)MALLOC(strlen(label)+2)) == NULL)
			return DTAM_CANT_MOUNT;
	*_dtam_mntpt = '/';
	strcpy(_dtam_mntpt+1,label);
	ptr = DtamDevAttr(device,BDEVICE);
	if (_dtam_flags & DTAM_TFLOPPY == 0)
		/*
		 *	a file system was recognized on /dev/dsk/f?, NOT f?t
		 */
		ptr[strlen(ptr)-1] = '\0';
	rdonly = (_dtam_flags & DTAM_READ_ONLY)? " -r " : " ";

retry:	sprintf(cmdbuf, "/sbin/tfadmin fmount %s-F %s %s %s",
				rdonly, fstype, ptr, _dtam_mntpt);
	result = system(cmdbuf);
	switch (result) {
	
	case 0:		goto success;
	case CORRUPT:
		if (retry) {
		    switch(diagn&DTAM_FS_TYPE) {
			case DTAM_S5_FILES:	
				sprintf(cmdbuf, "/sbin/fsck -y -q %s",ptr);
				break;
			case DTAM_UFS_FILES: 	
				sprintf(cmdbuf, "/sbin/fsck -F ufs -y %s",ptr);
				break;
			default:		
				sprintf(cmdbuf, "/sbin/fsck -F %s -y %s",
					fstype, ptr);
				break;
		    }
		    system(cmdbuf);
		    retry = FALSE;
		    goto retry;
		}
		/* Fall through!!! */
	default:	FREE(ptr);
			return CANT_MOUNT;
	}
success:
	/* Change the working directory to the mount point to prevent the
	 * disk from being unmounted by someone else behind our back.
	 */
	chdir (_dtam_mntpt);
	disk_tid = XtAddTimeOut(3000, (XtTimerCallbackProc)MonitorMount, NULL);
	return MOUNTED;
}

long	request_no;

void	CloseDisketteHandler(w, client_data, xevent, cont_to_dispatch)
	Widget		w;
	XtPointer	client_data;
	XEvent		*xevent;
	Boolean		*cont_to_dispatch;
{
	DtReply		reply;
	DtPropPtr	pp;

	if ((xevent->type != SelectionNotify)
	||  (xevent->xselection.selection != _DT_QUEUE(theDisplay)))
		return;
	if (disk_tid)
		XtRemoveTimeOut(disk_tid);
        memset(&reply, 0, sizeof(reply));
	DtAcceptReply(theScreen, _DT_QUEUE(theDisplay),
				XRootWindowOfScreen(theScreen), &reply);
	/* Always unmount the floppy, even if we didn't mount it. */
	attempt_umount ();
}

static void
attempt_umount (void)
{
	char	msg [BUFSIZ];
	char	*drive;

	chdir ("/");
	switch (DtamUnMount(_dtam_mntpt)) {
	case 0:
	    exit (0);
	case -1:
	    /* Insert floppy */
	    if (!Insert.shell)
		CreateGizmo(w_toplevel, ModalGizmoClass, &Insert, NULL, 0);

	    if (strncmp (curdev, DISKETTE, strlen (DISKETTE)) != 0)
		sprintf (msg, GetGizmoText (string_reInsMsg), "");
	    else
	    {
		drive = DtamDevAlias(curdev);
		sprintf (msg, GetGizmoText (string_reInsMsg),
			 drive + strlen (drive) - 1);
		FREE (drive);
	    }
	    SetModalGizmoMessage(&Insert, msg);
	    MapGizmo(ModalGizmoClass, &Insert);
	    break;
	default:
	    /* Busy or misc. failure */
	    if (!Busy.shell)
		CreateGizmo(w_toplevel, ModalGizmoClass, &Busy, NULL, 0);

	    sprintf (msg, GetGizmoText (string_busyMsg), _dtam_mntpt,
		     _dtam_mntpt);
	    SetModalGizmoMessage(&Busy, msg);
	    MapGizmo(ModalGizmoClass, &Busy);
	    break;
	}
	chdir (_dtam_mntpt);
}

static void
ReopenCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	attempt_open (curalias, False);
	BringDownPopup(_OlGetShellOfWidget (w));
}

static void
RecloseCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(_OlGetShellOfWidget (w));
	attempt_umount ();
}

int	attempt_open(alias, addEvent)
	char	*alias;
	Boolean	addEvent;
{
	DtRequest       request;

        memset(&request, 0, sizeof(request));
        request.open_folder.rqtype= DT_OPEN_FOLDER;
        request.open_folder.path  = _dtam_mntpt;
        request.open_folder.title = alias;
	request.open_folder.options = DT_NOTIFY_ON_CLOSE;
        request_no = DtEnqueueRequest(theScreen,
			_DT_QUEUE(theDisplay),
			_DT_QUEUE(theDisplay),
			 XtWindow(w_toplevel), &request);
	if (addEvent)
	    XtAddEventHandler(w_toplevel, (EventMask)NoEventMask, True,
			CloseDisketteHandler, (XtPointer)NULL);
	return 0;
}

attempt_copy (device, cpysrc)
	char	*device, *cpysrc;
{
	/*
	 *	The capacity check and error handling should be improved.
	 */
	char	*dev, *attr, cmdbuf[BUFSIZ];
	int	capacity = 0;
struct	stat	st_buf;

	if (dev = DtamGetDev(device, FIRST)) {
		if (attr = DtamDevAttr(dev,"capacity")) {
			capacity = atoi(attr);
			FREE(attr);
		}
		FREE(dev);
		capacity -= FindSize(_dtam_mntpt);
	}
	if (capacity < FindSize(cpysrc))
		return NO_ROOM;
	if (stat(cpysrc, &st_buf) != -1 && (st_buf.st_mode & S_IFMT) == S_IFDIR)
		/* cpio is not invoked by CpioCmd here, because this is
		 * supposed to mimic a folder copy as done by dtm, which
		 * does not run with privilege.  This might be revisited
		 * later.
		 */
		sprintf(cmdbuf, "cd `dirname %s`; "
				"find `basename %s` -print | "
				"/usr/bin/cpio -pd %s 2>/dev/null",
				cpysrc, cpysrc, _dtam_mntpt);
	else
		sprintf(cmdbuf, "cp %s %s", cpysrc, _dtam_mntpt);

	if (system(cmdbuf) != 0)
		return CANT_OPEN;
	else
		return 0;
}

int	FindSize(list)
	char	*list;
{
	FILE	*cmdpipe;
	int	total = 0;
	char	buf[BUFSIZ];

	sprintf(buf, "du -s %s",list);
	if (cmdpipe = popen(buf,"r")) {
		while (fgets(buf,BUFSIZ,cmdpipe)) {
			total += atoi(buf);
		}
		(void) pclose (cmdpipe);
	}
	return	total;
}

void
UnixTakeDrop(Widget wid, XtPointer client_data, XtPointer call_data)
{
	DtDnDInfoPtr    dip = (DtDnDInfoPtr)call_data;
	char		**ppName;
	Boolean		errFlg = False;
	extern char	*dropstr;

	if (dip->files && *dip->files && **dip->files) {
		for (ppName=dip->files; *ppName; ppName++) {
		    if (attempt_copy (curdev, *ppName) != 0)
		    {
			dropstr = *ppName;
			errFlg = True;
		    }
		}

		if (errFlg)
		    BaseNotice (DTAM_NO_ROOM, True);
	}
	else
		(void) attempt_open (curalias, False);
}

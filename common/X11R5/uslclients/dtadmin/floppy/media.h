/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:floppy/media.h	1.27"
#endif

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Vendor.h>
#include <Xol/OpenLookP.h>
#include <Xol/BulletinBo.h>
#include <Xol/FooterPane.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <Xol/StaticText.h>
#include <Xol/ControlAre.h>
#include <Xol/Gauge.h>
#include <Xol/FButtons.h>
#include <Xol/Caption.h>
#include <Xol/Notice.h>
#include <Xol/PopupMenu.h>
#include <Xol/PopupWindo.h>
#include <Xol/AbbrevButt.h>
#include <Xol/ScrolledWi.h>
#include <Xol/RubberTile.h>
#include <Xol/FList.h>
#include <Xol/Footer.h>
#include <Xol/Form.h>

#include <Dt/Desktop.h>
#include <DnD/OlDnDVCX.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/ModalGizmo.h>
#include <Gizmo/FileGizmo.h>
#include <Gizmo/ChoiceGizm.h>
#include <Gizmo/xpm.h>

#include <Memutil/memutil.h>

#include <libDtI/DtI.h>

#include "../dtamlib/dtamlib.h"

#include "media_msgs.h"

#define	DHELP_PATH	"dtadmin/disk.hlp"
#define	THELP_PATH	"dtadmin/tape.hlp"
#define	BHELP_PATH	"dtadmin/backup.hlp"
#define	D1ICON_NAME	"Disk_A.icon"
#define	D2ICON_NAME	"Disk_B.icon"
#define	GENICON_NAME	"gendev.icon"
#define	TICON_NAME	"ctape.glyph"

#define	FooterMsg(basegizmo,txt)	SetBaseWindowMessage(&basegizmo,txt)
#define	GGT(str)			GetGizmoText(str)

typedef enum _exitValue { NoErrs, NoAttempt, FindErrs, CpioErrs } ExitValue;
typedef	struct	{ char	*	label;
		  XtArgVal	mnem;
		  Boolean	sensitive;
		  XtArgVal	selCB;
		  XtArgVal	subMenu;
} MBtnItem;

#define	NUM_MBtnFields	5

#define	SET_BTN(id,n,name,cbf)	id[n].label = GetGizmoText(label##_##name);\
				id[n].mnem = (XtArgVal)*GGT(mnemonic##_##name);\
				id[n].sensitive = TRUE;\
				id[n].selCB = (XtArgVal)cbf

typedef	struct	{ char	*	label;
		  XtArgVal	mnem;
		  Boolean	setting;
} ExclItem;

#define	NUM_ExclFields	3

#define SET_EXCL(id,n,name,st)	id[n].label = GetGizmoText(label##_##name);\
                               	id[n].mnem = (XtArgVal)*GGT(mnemonic##_##name);\
				id[n].setting = st;

typedef struct  { char	*	label;
		  Boolean	deflt;
} DevItem;

#define	NUM_DevFields	2
#define	N_DEVS		8	/* recommended max menu size */

#define	MOUNT_CMD	"/sbin/mount"

typedef	struct {
		char	*bk_path;	
		char	bk_type;	/* 'd' for directory */
		int	bk_blksize;
} FileObj, *FileList;

extern	Widget		w_toplevel, w_note, w_panel, w_gauge, w_txt;
extern	Display		*theDisplay;
extern	Screen		*theScreen;
extern	Dimension	x3mm, y3mm;
extern	Dimension	xinch, yinch;
extern	XFontStruct	*def_font, *bld_font;
extern	XtIntervalId	gauge_id;
extern	PopupGizmo	note;
extern	Arg		arg[];
extern	int		g_value;
extern	char		*MBtnFields[];
extern	char		*ExclFields[];
extern	char		*DevFields[];

extern	int		errno;
extern	FILE		*cmdfp[];

#define	CMDIN		fileno(cmdfp[0])
#define	CMDOUT		fileno(cmdfp[1])

extern	char		*_dtam_mntpt;
extern	char		*_dtam_mntbuf;
extern	long		_dtam_flags;

extern	char		*desc;
extern	char		*curdev;
extern	char		*curalias;
extern	char		*cur_file;
extern	char		*Help_intro;

extern	char		*getenv();
extern	char		*ptsname();
extern	void		exitCB();
extern	Widget		CreateMediaWindow();
extern	Widget		DevMenu();

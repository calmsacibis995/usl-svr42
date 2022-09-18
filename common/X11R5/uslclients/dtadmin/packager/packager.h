/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:packager/packager.h	1.15"
#endif
/*
 *	packager.h
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <libgen.h>
#include <limits.h>
#include <pwd.h>
#include <dirent.h>
#include <fcntl.h>

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/Xlib.h>
#include <X11/Vendor.h>

#include <Xol/OpenLookP.h>
#include <Xol/Dynamic.h>
#include <Xol/PopupMenu.h>
#include <Xol/PopupWindo.h>
#include <Xol/BaseWindow.h>
#include <Xol/RubberTile.h>
#include <Xol/BulletinBo.h>
#include <Xol/ControlAre.h>
#include <Xol/FButtons.h>
#include <Xol/StaticText.h>
#include <Xol/TextField.h>
#include <Xol/TextEdit.h>
#include <Xol/Caption.h>
#include <Xol/FooterPane.h>
#include <Xol/ScrolledWi.h>

#include <Dt/Desktop.h>
#include <Gizmo/Gizmos.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/FileGizmo.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/ModalGizmo.h>
#include <Memutil/memutil.h>
#include <libDtI/DtI.h>

#define	HELP_PATH	"dtadmin/pkgset.hlp"
#define	ICON_NAME	"pkgmgr48.icon"

#include "../dtamlib/dtamlib.h"

#include "pkg_msgs.h"

#define		FooterMsg(txt)	SetBaseWindowMessage(&base,txt)

#define		GGT(type,msgid)		GetGizmoText(type##_##msgid)

typedef	struct	{ char	*	label;
		  XtArgVal	mnem;
		  Boolean	sensitive;
		  XtArgVal	selCB;
		  XtArgVal	subMenu;
} Items;
extern	char    *Fields[];

#define	SET_BTN(id,n,nm,sen,CB)	id[n].label = GGT(label,nm);\
				id[n].mnem = (XtArgVal)*GGT(mnemonic,nm);\
				id[n].sensitive = sen;\
				id[n].selCB = (XtArgVal)CB
#define	NUM_Fields	5

typedef	struct	{ char	*	label;
		  XtArgVal	mnem;
		  Boolean	setting;
} ExclItem;
extern	char    *ExclFields[];

#define SET_EXCL(id,n,name,st)	id[n].label = GGT(label,name);\
                               	id[n].mnem = (XtArgVal)*GGT(mnemonic,name);\
				id[n].setting = st;

typedef	struct	{
	char    *pkg_name;	/* brief package name */
	char    *pkg_desc;      /* longer description */
	char	*pkg_fmt;	/* 4.0, 3.2 or Custom */
	char    *pkg_cat;       /* category: mostly arbitrary except for sets */
	char	*pkg_set;	/* NULL unless part of a set (named here) */
	char	*pkg_vers;
	char	*pkg_arch;
	char	*pkg_vend;
	char	*pkg_date;
	char	*pkg_size;
	char	*pkg_help;
	char	*pkg_class;
	char	pkg_opflag;	/* 'T'/'F' (succeed/fail), 'D' (deleted) */
} PkgRec, *PkgPtr;

typedef	struct	{
	char	*pkg;
	char	*def;
} icon_obj;

extern	char	*GetXWINHome();

extern	long	_dtam_flags;

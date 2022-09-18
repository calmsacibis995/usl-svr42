/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olps:main.h	1.12"
#endif

#include <stdio.h>
#include <errno.h> 
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <sys/stat.h> 		/* must follow Intrinsic.h */
#include <X11/Shell.h>
#include <X11/XWDFile.h>
#include <Xol/OpenLookP.h>
#include <Xol/BaseWindow.h>
#include <Xol/ControlAre.h>
#include <Xol/PopupWindo.h>
#include <Xol/Form.h>
#include <Xol/FooterPane.h>
#include <Xol/Caption.h>
#include <Xol/TextField.h>
#include <Xol/StaticText.h>
#include <Xol/Text.h>
#include <Xol/Stub.h>
#include <Xol/Scrollbar.h>
#include <Xol/ScrolledWi.h>
#include <Xol/OlStrings.h>
#include <Xol/Notice.h>
#include <Xol/MenuShell.h>
#define FEEP_VOLUME 0
#define PRINTCMD_WIDTH	300

#define MM_OLPS_HEIGHT		100	/* olprintscreen initial height limit */
#define MM1     125     /* used by prop: default filename  field */
#define MM2     45      /* used by prop: medium length fields */
#define MM3     12      /* used by prop: integer fields */
#define MM4     70      /* used by 3 popups asking for filename field */

#define TEXTF_WIDTH_1   200     /* used by 3 popups, prop: filename */
#define TEXTF_WIDTH_2   100     /* used by prop: output format */
#define TEXTF_WIDTH_3   10      /* used by prop: integer fields */

#define HELP_OLPS     "olps_general"
#define HELP_PROPERTY "olps_props"

/* Buttons used on popup windows */
typedef struct {
	void (*funcptr)();
	Widget * popup;
} infostruct;

/* Buttons used on popup windows */
typedef struct {
	XtArgVal label;
	XtArgVal callback;
	XtArgVal mnemonic;
} ButtonItems;

typedef struct {
	XtArgVal label;
	XtArgVal callback;
	XtArgVal mnemonic;
} NoticeItems;

/* Menu structure definitions */

typedef struct {
	XtArgVal label;
	XtArgVal p;	/* callback */
	XtArgVal data;	/* client data */
	XtArgVal sensitive;
	XtArgVal mapped;
	XtArgVal popup;	/* popup menu, if any */
	XtArgVal mnemonic;
} MenuItem;

typedef struct Menu {
	String		label;
	MenuItem	*items;
	int		numitems;
	Bool		use_popup;
	int		orientation;	/* OL_FIXEDROWS, OL_FIXEDCOLS */
	int		pushpin;	/* OL_OUT, OL_NONE */
	Widget		flatORshell;	/* Pointer to this menu widget */
} Menu;

/* The ordering of the buttons on the toplevel window */
#define FILEPOP	0
#define PRINT	1
#define CAPTURE	2
#define PROP	3

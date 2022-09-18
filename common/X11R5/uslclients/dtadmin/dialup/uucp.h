/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/uucp.h	1.36"
#endif

#ifndef _UUCP_H
#define _UUCP_H

#include <stdio.h>
#include <ctype.h>
#include <Xol/TextField.h>
#include <libDtI/DtI.h>

/* Some macros */
#define DISPLAY		XtDisplay(sf->toplevel)
#define WINDOW		XtWindow(sf->toplevel)
#define SCREEN		XtScreen(sf->toplevel)
#define ROOT		RootWindowOfScreen(SCREEN)
#define STRUCTASSIGN(d, s)	memcpy(&(d), &(s), sizeof(d))
#define DIRECTORY(f, s)	((stat((f), &s)==0) && ((s.st_mode&(S_IFMT))==S_IFDIR))
#define REGFILE(f, s)	((stat((f), &s)==0) && ((s.st_mode&(S_IFMT))==S_IFREG))
#define	DMODE		(S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#define	MODE		(S_IRWXU | S_IRGRP | S_IROTH) /* 0744 */
#define UUCPUID		(uid_t) 5
#define UUCPGID		(gid_t) 5
#define NIL		(XtPointer)0
#define BLANKLINE	""
#define CLEARMSG()	ClearFooter(sf->footer)
#define PUTMSG(msg)     FooterMsg(sf->footer, "%s", msg)

/*
** Bit manipulating macros
*/
#define BIT_IS_SET(field, bit)  (((field) & (bit)) != (unsigned)0)
#define SET_BIT(field, bit)     ((field) |= (bit))
#define UNSET_BIT(field, bit)   ((field) &= ~(bit))
/*
** Masks for `stat_flags'
*/
#define	FILE_EXISTS	1
#define FILE_READABLE	2
#define FILE_WRITEABLE	4


#define UNAMESIZE	20	/* max chars is a user name */
#define	INCREMENT	20
#define BUF_SIZE	1024
#define MAXLINE		128

#define	FORMAT		"%14s%0s%0s%8s%33s"
#define MAXNAMESIZE	13
#define	MAXPHONESIZE	32

/* Possible return values for field validation */
#define VALID		False
#define INVALID		True

/* request type for the device popup window */
#define	B_ADD		0
#define	B_MOD		1

/*
 * Popup Window actions.  These messages are sent to the popup callback when
 * a user has selected one of the popup window's buttons.
 */
#define APPLY		1
#define RESET		2
#define RESETFACTORY	3
#define SETDEFAULTS	4
#define CANCEL		5

/*
 * Object messages.  These are arbitrary actions we have defined for the
 * popup widget's instance methods (i.e. routines we have defined) to use
 * to control the items in the property window.
 */
#define NEW		1
#define SET		2
#define GET		3
#define DIM		4
#define UNDIM		5
#define DEFAULT		6
#define SELECT		7
#define UNSELECT	8
#define VERIFY		9

/* Fields in Systems file */
#define F_NAME 0
#define F_TIME 1
#define F_TYPE 2
#define F_CLASS 3
#define F_PHONE 4
#define F_EXPECT1 5
#define F_LOGIN 6
#define F_EXPECT2 7
#define F_PASSWD 8
#define F_ARG 9
#define F_MAX 50

/* Fields in Devices file */
#define D_TYPE 0
#define D_LINE 1
#define D_CALLDEV 2
#define D_CLASS 3
#define D_CALLER 4
#define D_ARG 5
#define D_MAX 50

/* bnu object type */
typedef struct {
	char	*path;
	DmObjectPtr	op;
	int		num_objs;
} ContainerRec, *ContainerPtr;

#define	INIT_X	45
#define	INIT_Y	0
#define	INC_X	70
#define	INC_Y	30
#define DIALUP_WIDTH	OlMMToPixel(OL_HORIZONTAL, 140)
#define DIALUP_HEIGHT	OlMMToPixel(OL_VERTICAL, 60)
#define BNU_WIDTH	OlMMToPixel(OL_HORIZONTAL, 110)
#define BNU_HEIGHT	OlMMToPixel(OL_VERTICAL, 60)
#define OFFSET	29

#define VIEWHEIGHT	6

#define XA		XtArgVal

typedef struct _DeviceData {
	String		portNumber;	/* eg. tty00, tty01 ... */
	String		modemFamily;	/* eg. hayes, telebit ...*/
	String		portSpeed;	/* eg. 1200, 2400 ... */
	String		DTP;		/* Dialer-Token-Pair */
	String		holdPortNumber;	/* used for Reset */
	String		holdModemFamily;	/* same as above */
	String		holdPortSpeed;	/* same as above */
} DeviceData;

DeviceData holdData;

typedef struct _DeviceItems {
	String		label;
	String		value;
} DeviceItems;

#define ACU "ACU"
#define DIR "Direct"
#define DK "DK"

#define ACU_ICON	"acu.glyph"
#define DIR_ICON	"dir.glyph"

typedef struct lineRec *LinePtr;
typedef	struct lineRec {
	LinePtr	next;
	char	*text;
} LineRec;

typedef struct {
    XtPointer f_name;
    XtPointer f_time;
    XtPointer f_type;
    XtPointer f_class;
    XtPointer f_phone;
    XtPointer f_expect;
    XtPointer f_expect1;
    XtPointer f_login;
    XtPointer f_expect2;
    XtPointer f_passwd;
    LinePtr   lp;
    Boolean   changed;
} HostData;

typedef struct {
    HostData *pField;
} FlatList;

FlatList *	new;

typedef struct _ExecItem {
	void		(*p)();		/* proc to call when timer expires */
	Widget		popup; 		/* wid used when command exit != 0 */
	Widget		button; 	/* wid used to (un)set sensitivity */
	int		pid;		/* child process id */
	int		exit_code;	/* the exit code		   */
	char *		exec_argv[10];	/* arg0 is command, arg1 is opts   */
} ExecItem;

typedef struct _Items {
	void		(*p)();
	XtArgVal	popup;
	XtArgVal	sensitive;
	XtArgVal	label;
	XtArgVal	mnemonic;
	XtArgVal	client;
} Items;

typedef struct Menus {
	String		label;
	Items		*items;
	int		numitems;
	Bool		use_popup;
	int		orientation;	/* OL_FIXEDROWS, OL_FIXEDCOLS */
	int		pushpin;	/* OL_OUT, OL_NONE */
	Widget		widget;		/* Pointer to this menu widget */
} Menus;

typedef struct {
    char        *title;
    char        *file;
    char        *section;
} HelpText;

typedef struct _SystemFile {
	Boolean		changesMade;
	Boolean		readAllow;
	Boolean		update;
	FlatList *	flatItems;
	Items *		popupMenuItems;
	Widget		cancelNotice;
	Widget		findPopup;
	Widget		findTextField;
	Widget		footer;
	Widget		sfooter;
	Widget		dfooter;
	TextFieldWidget	w_name;	        /* Main body of the entry */
	Widget		w_type;
	Widget		w_class;
	TextFieldWidget	w_phone;
	TextFieldWidget	w_passwd;
	Widget		w_more;
	TextFieldWidget	w_expect1;
	TextFieldWidget	w_expect2;
	Widget		propPopup;
	Widget		devicePopup;
	Widget		quitNotice;
	Widget		scrollingList;
	Widget		toplevel;
	char *		userName;	/* User name of the operator */
	char *		userHome;	/* $HOME of the user */
	char *		nodeName;	/* Node name of the local hosts */
	char *		filename;	/* Used by open and save */
	int		currentItem;	/* Currently selected index */
	int		numFlatItems;
	int		numAllocated;	/* The high water mark of items installed */
	LinePtr 	lp;		/* records holder */
} SystemFile;

SystemFile *sf;

typedef struct _DeviceFile {
	DmItemPtr	itp;
	DmContainerPtr  cp;
	DmObjectPtr	select_op;
	Boolean		changesMade;
	Items *		popupMenuItems;
	Widget		w_modem;
	Widget		w_speed;
	Widget		w_port;
	Widget		w_extra;
	Widget		w_acu;
	Widget		iconbox;
	Widget		cancelNotice;
	Widget		footer;
	Widget		propPopup;
	Widget		QDPopup;
	Widget		openNotice;
	Widget		openTextField;
	Widget		quitNotice;
	Widget		saveTextField;
	Widget		toplevel;
	int		request_type;
	char *		filename;	/* Used by open and save */
	char *		saveFilename;	/* Used by saveas... */
} DeviceFile;

DeviceFile *df;

#endif /* _UUCP_H */

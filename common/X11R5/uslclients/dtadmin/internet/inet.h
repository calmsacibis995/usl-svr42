/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:internet/inet.h	1.23"
#endif

#ifndef _INET_H
#define _INET_H

#include <stdio.h>
#include <libDtI/DtI.h>
#include <Gizmos.h>
#include <MenuGizmo.h>
#include <ModalGizmo.h>
#include <PopupGizmo.h>

/* For expanding /etc/hosts table used by CopyRemoteHostsTable routine */
#define REPLACE	0
#define APPEND	1

/* Some macros */
#define DIRECTORY(f, s)	((stat((f), &s)==0) && ((s.st_mode&(S_IFMT))==S_IFDIR))
#define DISPLAY		XtDisplay(hf->toplevel)
#define WINDOW		XtWindow(hf->toplevel)
#define SCREEN		XtScreen(hf->toplevel)
#define ROOT		RootWindowOfScreen(SCREEN)
#define PUTMSG(msg)	FooterMsg(hf->footer, "%s", msg)
#define CLRMSG()	ClearFooter(hf->footer)
#define STRUCTASSIGN(d, s)	memcpy(&(d), &(s), sizeof(d))
#define NIL		(XtPointer)0
#define BLANKLINE	""
#define	DMODE		(S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#define	MODE		(S_IRWXU | S_IRGRP | S_IROTH) /* 0744 */

#define UUCPPREFIX	"Any TcpCico10103 - \\x00020ace"
#define WORKDIR		"/var/spool/uucppublic"	/* top directory */
#define WORKDIRMODE     (S_ISVTX | S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO) /* 50777 */
#define UNAMESIZE	20	/* max chars is a user name */
#define	INCREMENT	20
#define BUF_SIZE	1024
#define MAXLINE		128
#define MAXNAMESIZE	13
#define	MAXADDRSIZE	14
#define	MAXOCTETSIZE	3
#define	MAXALIASES	35
/* Fields in Host file */
#define F_NAME		0
#define F_IPADDR	1
#define F_COMMENT	2
#define F_NET1_ADDR	3
#define F_NET2_ADDR	4
#define F_NET3_ADDR	5
#define F_NET4_ADDR	6
#define	F_ALIASES	7
#define F_ARG		8

/* Property sheet buttons */
#define	TXT_PROPERTY_APPLY	"Apply"
#define TXT_PROPERTY_RESET	"Reset"
#define TXT_PROPERTY_CANCEL	"Cancel"
#define TXT_PROPERTY_HELP	"Help"

/* view height for the base list */
#define VIEWHEIGHT	6
#define	XA		XtArgVal

/* index to the buttons for the property sheet */
typedef enum {
	PropApply, PropReset, PropCancel, PropHelp
} PropMenuItemIndex;

/* for the internet address container in the property sheet */
typedef struct _IPaddrSettings {
        Setting octet1;
        Setting octet2;
	Setting octet3;
	Setting octet4;
} IPaddrSettings;

/*
 * itemFields[] -  array of choice item resource names
 */
static String  itemFields[] = {
        XtNlabel,
	XtNclientData,
};

static char *system_path = "/etc/hosts";
static char *permission_path = "/etc/hosts.equiv";
static char *uucp_path = "/etc/uucp/Systems.tcp";

struct HostEntry {
	char *h_name;
	char **h_aliases;
	char *h_addr;
	char *h_comment;
};

typedef Boolean                  (*PFB)();
#define	NONE		0
#define	ALL		1
#define	SPECIFY		2

typedef struct {
    XtPointer f_name;
    XtPointer f_IPaddr;
    XtPointer f_comment;
    XtPointer f_net1Addr;
    XtPointer f_net2Addr;
    XtPointer f_net3Addr;
    XtPointer f_net4Addr;
    XtPointer f_aliases;
    int       f_allow;
    char      *equiv;
    Boolean   changed;
    int       index;
} HostData;

typedef struct {
    HostData *pField;
} FlatList;

FlatList *	new;
FlatList *	local;

typedef struct _UucpEntry {
    char * u_name;
    char * u_rest;
} UucpEntry;



typedef struct _Items {
	void		(*p)();
	XtArgVal	popup;
	XtArgVal	sensitive;
	XtArgVal	label;
	XtArgVal	mnemonic;
	XtArgVal	client;
} Items;

typedef struct {
    String      type;
    String      *label;
    String      *mnemonic;
} ItemsLabel;

typedef struct Menu {
	String		label;
	Items		*items;
	int		numitems;
	Bool		use_popup;
	int		orientation;	/* OL_FIXEDROWS, OL_FIXEDCOLS */
	int		pushpin;	/* OL_OUT, OL_NONE */
	Widget		widget;		/* Pointer to this menu widget */
} Menu;

typedef struct {
    char        *title;
    char        *file;
    char        *section;
} HelpText;

typedef struct _SystemFile {
	Items *		popupMenuItems;
	PopupGizmo *	propPrompt;
	ModalGizmo *	appendPrompt;
	ModalGizmo *	expandPrompt;
	FlatList *	flatItems;
	Widget		toplevel;
	Widget		scrollingList;
	Widget		findPopup;
	Widget		propPopup;
	Widget		localPopup;
	Widget		appendPopup;
	Widget		expandPopup;
	Widget		quitNotice;
	Widget		cancelNotice;
	Widget		findTextField;
	Widget		footer;
	int		numFlatItems;
	int		currentItem;	/* Currently selected index */
	int		numAllocated;	/* The high water mark of items installed */
	UucpEntry *	uucpItems;	/* Line buffer to hold new uucp entries */
	int		numUucp;
	char **		Lines;		/* Line buffer to hold all lines */
	int		numLines;
	int		numLinesAllocated;
	Boolean		update;
	Boolean		changesMade;
	Boolean		address;	/* Address mode; full or simple	*/
	char *		userName;	/* User name of the operator */
	char *		userHome;	/* $HOME of the user */
	char *		nodeName;	/* Node name of the local hosts */
	char *		filename;	/* Used by save */
} SystemFile;

SystemFile *hf;

#endif /* _INET_H */

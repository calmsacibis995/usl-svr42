/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/menu.h	1.7"
#endif

#ifndef _MENU_H
#define _MENU_H

/*
 *	Flat menu item fields
 */

typedef struct {
    char	**items;
    int		helpCnt;
} Help;

typedef struct _MenuItem {
    XtArgVal	label;
    XtArgVal	selectProc;
    XtArgVal	clientData;
    XtArgVal	popupMenu;
    XtArgVal	mnemonic;
} MenuItem;

typedef struct _Menu {
    char	*name;		/* widget name */
    char	*title;		/* shell title (menu is pinnable if set) */
    MenuItem	*items;		/* flat menu items */
    int		itemCnt;	/* number of items */
    Help	*help;		/* help data */
} Menu;

extern Widget			CreateMenu();
extern Widget			CreatePopup();
 
#endif

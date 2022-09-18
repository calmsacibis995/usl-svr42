/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_MENUP_H
#define	_MENUP_H
#ident	"@(#)debugger:libol/common/MenuP.h	1.3"

// toolkit specific members of the Menu class
// included by ../../gui.d/common/Menu.h

private:
	Widget		menu;		// list widget
	void		*list;		// flat list table for menu bar
	Boolean		delete_table;	// TRUE if the table was reallocated by add_item

#endif	// _MENUP_H

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_SELECTIONP_H
#define	_SELECTIONP_H
#ident	"@(#)debugger:libol/common/Sel_listP.h	1.2"

// toolkit specific members of the Selection_list class
// included by ../../gui.d/common/Selection.h

private:
	Widget		list;
	const char	***item_data;
	int             visible_items;
	int		total_items;
	int		columns;
	char		**pointers;
	Boolean		overflow;

	void		allocate_pointers(int rows, int columns);

#endif	// _SELECTIONP_H

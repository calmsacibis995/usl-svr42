/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_STATUS_H
#define	_STATUS_H
#ident	"@(#)debugger:gui.d/common/Status.h	1.3"

#include "Windows.h"

class Process;

class Status_pane
{
	Table		*pane;
	Process		*last_process;

public:
			Status_pane(Expansion_box *);
			~Status_pane();

			// display functions
	void		update(Process *);
};

#endif	// _STATUS_H

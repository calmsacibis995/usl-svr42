/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Interface_h
#define Interface_h

#ident	"@(#)debugger:inc/common/Interface.h	1.3"

// Debugger interface structure.

// Enables different interfaces (e.g., screen and line mode)
// to work together.

#include <stdio.h>

#include "Iaddr.h"
#include "Msgtypes.h"
#include "Severity.h"
#include "print.h"

class	LWP;

enum ui_type {
	ui_no_type = 0,
	ui_cli,
	ui_gui,
};

extern	ui_type	get_ui_type();

extern	void	pushoutfile(FILE *);
extern	void	popout();
extern	void	set_interface(const char *interface, char **options, const
			char *gpath);
extern	void	stop_interface();

extern	int	PrintaxGenNL;
extern	int	PrintaxSpeakCount;

// Obslete function - will go away when all messages use printm
void	printx(const char * ... );

// Debugging flags and debugging interface - debugging code disappears
// unless compiled with -DDEBUG
// example - DPRINT(DBG_PARSER, ("token = %s\n", t->print()))

#ifdef DEBUG
extern int debugflag;
#define	DBG_PARSER	0x4
#define	DBG_EXECON	0x8
#define DBG_EXPR	0x40
#define DBG_PROC	0x80
#define	DPRINT(D,M)	if (debugflag & D) { printf M; }
#else
#define	DPRINT(D,M)
#endif

#endif	/* Interface_h */
